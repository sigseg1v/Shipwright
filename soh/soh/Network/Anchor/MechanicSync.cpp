#include "Anchor.h"
#include "MechanicSync.h"
#include "MechanicSync/Registry.h"
#include "JsonConversions.hpp"
#include <cstdio>
#include <map>
#include <string>
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"
extern "C" {
extern PlayState* gPlayState;
}

// Mechanic sync streams the pos/rot (and optional per-family aux state)
// of moving Bg_*/Obj_* mechanics from the scene authority to peers.
//
// Unlike EnemySync, mechanics are NOT minted with a netId. Every client
// spawns the same scene-defined instance from the room file at the same
// home position, so the receiver matches the sender's payload to a
// local actor by (actorId, home.pos) -- a stable, deterministic key.
//
// Per-family logic (gating shouldSync, optional serializeAux/applyAux)
// lives in MechanicSync/Families/<Name>.cpp; this file just drives the
// tick / receive loops via MechanicFamilyRegistry.

std::string MechanicSync_MakeKey(int16_t actorId, float homeX, float homeY, float homeZ) {
    // Round to nearest int. Scene-spawned home positions are integer-
    // valued in the room files, and any post-spawn shift would be in
    // the actor's world.pos rather than home.pos -- so the integer
    // round is exact, not lossy.
    char buf[64];
    snprintf(buf, sizeof(buf), "%d:%d:%d:%d",
             (int)actorId,
             (int)(homeX < 0 ? homeX - 0.5f : homeX + 0.5f),
             (int)(homeY < 0 ? homeY - 0.5f : homeY + 0.5f),
             (int)(homeZ < 0 ? homeZ - 0.5f : homeZ + 0.5f));
    return std::string(buf);
}

// Walk every actor list that can hold a mechanic. ACTORCAT_BG is the
// primary home for DynaPoly mechanics (push blocks, elevators, doors,
// platforms). ACTORCAT_PROP holds many Obj_* mechanics (Obj_Oshihiki
// for example sits in ACTORCAT_BG, but some Obj_* sit in PROP).
// ACTORCAT_SWITCH covers switch-type mechanics whose pos can move
// (mostly static, but cheap to include).
static const u8 kMechanicCategories[] = { ACTORCAT_BG, ACTORCAT_PROP, ACTORCAT_SWITCH };

// Authority side, 15Hz throttle. Walks the mechanic categories, asks
// each registered family whether it should currently sync, packs
// pos+rot+aux into a json array, and broadcasts MECHANIC_STATE.
void Anchor::MechanicSync_TickAuthorityBroadcast() {
    if (!IsSaveLoaded() || !isConnected) {
        return;
    }
    if (!IsAuthorityForCurrentScene()) {
        return;
    }

    // ~15Hz at 60fps. Mechanics generally move slower than enemies
    // (push blocks: ~1.5 units/frame, elevators: smooth lerps), so
    // 15Hz with no receive-side LERP reads as smooth enough. Bumping
    // to 30Hz with receiver LERP is a follow-up if needed.
    mechanicSyncTickCounter++;
    if ((mechanicSyncTickCounter & 0x3) != 0) {
        return;
    }

    nlohmann::json mechanics = nlohmann::json::array();

    for (u8 cat : kMechanicCategories) {
        Actor* actor = gPlayState->actorCtx.actorLists[cat].head;
        while (actor != NULL) {
            const MechanicFamily* family = MechanicFamilyRegistry::Find(actor->id);
            if (family != nullptr) {
                bool gate = family->shouldSync == nullptr || family->shouldSync(actor);
                if (gate) {
                    nlohmann::json m;
                    m["key"] = MechanicSync_MakeKey(actor->id, actor->home.pos.x, actor->home.pos.y,
                                                   actor->home.pos.z);
                    m["pos"] = actor->world.pos;
                    m["rot"] = actor->shape.rot;
                    if (family->serializeAux != nullptr) {
                        family->serializeAux(actor, m);
                    }
                    mechanics.push_back(m);
                }
            }
            actor = actor->next;
        }
    }

    if (mechanics.empty()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = MECHANIC_STATE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["mechanics"] = mechanics;
    payload["quiet"] = true;

    // Diagnostic: log every ~5s (75 ticks @ 15Hz). Quoting the tick
    // counter and scene num gives enough fingerprint to correlate the
    // outbound batch against a peer-side "Mechanic state apply" log.
    if ((mechanicSyncTickCounter & 0x12C) == 0x12C) {
        SPDLOG_INFO("[Anchor:diag] MechanicSync broadcast scene={} count={} tick={}",
                    gPlayState->sceneNum, (int)mechanics.size(), mechanicSyncTickCounter);
    }

    SendJsonToRemote(payload);
}

// Receiver: looks up each (actorId, homePos) key against the live
// actor lists in the current scene and writes the authority's pos/rot
// into the matching local actor. Aux apply is delegated to the family.
void Anchor::HandlePacket_MechanicState(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }
    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }
    if (!payload.contains("mechanics") || !payload["mechanics"].is_array()) {
        return;
    }

    // We're a peer in the broadcasting scene -- but if we happen to be
    // the elected authority for this scene (shouldn't happen, server
    // gates that), skip. Cheap defense against late packets after a
    // handoff.
    if (IsAuthorityForCurrentScene()) {
        return;
    }

    // Build a one-shot map of key -> Actor* by scanning the mechanic
    // actor lists once. Avoids an O(N*M) scan when many mechanics are
    // syncing in the same scene.
    std::map<std::string, Actor*> byKey;
    for (u8 cat : kMechanicCategories) {
        Actor* actor = gPlayState->actorCtx.actorLists[cat].head;
        while (actor != NULL) {
            if (MechanicFamilyRegistry::Find(actor->id) != nullptr) {
                byKey[MechanicSync_MakeKey(actor->id, actor->home.pos.x, actor->home.pos.y,
                                          actor->home.pos.z)] = actor;
            }
            actor = actor->next;
        }
    }

    int matched = 0;
    int unmatched = 0;
    for (auto& m : payload["mechanics"]) {
        std::string key = m.value("key", std::string{});
        if (key.empty()) {
            continue;
        }
        auto it = byKey.find(key);
        if (it == byKey.end() || it->second == nullptr) {
            // Local instance hasn't spawned yet (room async load) or has
            // been killed. Skip; we'll catch up next tick.
            unmatched++;
            continue;
        }
        matched++;
        Actor* actor = it->second;

        Vec3f pos = m.value("pos", Vec3f{ 0, 0, 0 });
        Vec3s rot = m.value("rot", Vec3s{ 0, 0, 0 });
        actor->world.pos = pos;
        actor->shape.rot = rot;
        actor->world.rot.y = rot.y;

        const MechanicFamily* family = MechanicFamilyRegistry::Find(actor->id);
        if (family != nullptr && family->applyAux != nullptr) {
            family->applyAux(actor, m);
        }
    }

    // Log if any payload entries failed to match a local actor: usually
    // means a scene-spawn race or a missing family registration for an
    // actor the authority is broadcasting. Always log when nonzero so
    // mismatched authority/peer scenes are visible without setting log
    // level to DEBUG.
    if (unmatched > 0) {
        SPDLOG_INFO("[Anchor:diag] MechanicSync apply scene={} matched={} unmatched={}",
                    sceneNum, matched, unmatched);
    }
}

// Authority-side, one-shot. Called from OnActorKill for mechanic
// families that permanently destruct (e.g. Obj_Lift falling). We send
// the same (actorId, home.pos) key the streaming MECHANIC_STATE uses
// so peers can resolve it back to their local instance.
void Anchor::SendPacket_MechanicDestroyed(s16 sceneNum, s16 actorId, f32 homeX, f32 homeY, f32 homeZ) {
    if (!IsSaveLoaded() || !isConnected) {
        return;
    }
    nlohmann::json payload;
    payload["type"] = MECHANIC_DESTROYED;
    payload["sceneNum"] = sceneNum;
    payload["key"] = MechanicSync_MakeKey(actorId, homeX, homeY, homeZ);
    SendJsonToRemote(payload);
}

// Receiver: matches the key against local actors in the current scene
// and kills the one that matches. Unlike MECHANIC_STATE this is not
// gated on `!IsAuthorityForCurrentScene()` -- the server is gating
// who sends, and if we receive one for our own scene we should still
// honor it (it shouldn't happen, but the alternative is two clients
// where one's lift is dead and the other's is alive forever).
void Anchor::HandlePacket_MechanicDestroyed(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }
    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }
    std::string key = payload.value("key", std::string{});
    if (key.empty()) {
        return;
    }

    for (u8 cat : kMechanicCategories) {
        Actor* actor = gPlayState->actorCtx.actorLists[cat].head;
        while (actor != NULL) {
            Actor* next = actor->next;
            if (MechanicFamilyRegistry::Find(actor->id) != nullptr) {
                std::string localKey = MechanicSync_MakeKey(actor->id, actor->home.pos.x, actor->home.pos.y,
                                                            actor->home.pos.z);
                if (localKey == key) {
                    Actor_Kill(actor);
                }
            }
            actor = next;
        }
    }
}
