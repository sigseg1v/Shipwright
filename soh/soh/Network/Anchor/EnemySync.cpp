#include "Anchor.h"
#include "EnemySync.h"
#include "EnemySync/Registry.h"
#include "JsonConversions.hpp"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include "soh/ObjectExtension/ObjectExtension.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

// Per-actor sync logic (collider register, AC clear, optional AI field
// serialize/apply) lives in EnemySync/Families/<Family>.cpp -- one file
// per actor family, each pushing an EnemyFamily entry into
// EnemyFamilyRegistry via static initializer. Code in this file
// dispatches through the registry rather than growing a switch
// statement for every new enemy.

// Actor categories we walk for sync. Enemies live in ACTORCAT_ENEMY;
// bosses live in ACTORCAT_BOSS. Order doesn't matter -- the registry
// matches on actorId, so each actor only ever dispatches once.
static const u8 kEnemySyncCategories[] = { ACTORCAT_ENEMY, ACTORCAT_BOSS };

// EnemyNetState definition lives in EnemySync.h; the registration token is
// here (single TU) so the Id is allocated exactly once.
static ObjectExtension::Register<EnemyNetState> EnemyNetStateRegister;

static EnemyNetState* GetOrCreateNetState(Actor* actor) {
    EnemyNetState* state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
    if (state == nullptr) {
        ObjectExtension::GetInstance().Set<EnemyNetState>(actor, EnemyNetState{});
        state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
    }
    return state;
}

// Membership lookup for host-authoritative sync. Identity of "synced"
// actor families is owned by the registry; adding a new family is a
// matter of dropping a Families/<Name>.cpp file rather than editing
// this function.
static bool IsSyncableEnemy(const Actor* actor) {
    if (actor == nullptr) {
        return false;
    }
    return EnemyFamilyRegistry::Find(actor->id) != nullptr;
}

// Reads the server-elected authority for `sceneNum` from the
// sceneAuthorities map populated by SCENE_AUTHORITY packets. Returns 0
// if the server hasn't (yet) assigned an authority for this scene --
// callers should treat 0 as "I'm authority by default" so the local
// client still functions when running against a pre-election anchor
// server, or before the first SCENE_AUTHORITY arrives.
uint32_t Anchor::GetSceneAuthorityClientId(s16 sceneNum) {
    auto it = sceneAuthorities.find(sceneNum);
    return it != sceneAuthorities.end() ? it->second : 0;
}

bool Anchor::IsAuthorityForCurrentScene() {
    if (!IsSaveLoaded()) {
        return false;
    }
    uint32_t authority = GetSceneAuthorityClientId(gPlayState->sceneNum);
    // 0 = server hasn't elected anyone (or older server): act as authority.
    return authority == 0 || authority == ownClientId;
}

uint32_t Anchor::MintEnemyNetId() {
    // Embed our clientId in the high 16 bits so network IDs stay unique even
    // after authority handoff or reconnects. Counter rolls past 0 -> 1 to
    // avoid the "0 means unset" sentinel.
    if (++enemyNetIdCounter == 0) {
        enemyNetIdCounter = 1;
    }
    return ((ownClientId & 0xFFFF) << 16) | (enemyNetIdCounter & 0xFFFF);
}

// On scene spawn: walk the enemy actor list, mark authority/non-authority,
// and on the authority side broadcast a spawn for each pre-existing enemy.
// On non-authority clients, kill any pre-existing syncable enemies the engine
// spawned -- we'll receive ENEMY_SPAWN packets from the authority instead.
//
// Note: this is the simple v1. A more efficient design suppresses the spawn
// at the source via `VB_SHOULD` hooks per actor family, but that requires
// per-family work. For Phase 2 (Stalchild), waiting for engine spawn and
// then killing on non-authority is acceptable -- Stalchildren use
// `VB_ENCOUNT1_SPAWN_STALCHILD_OR_WOLFOS` for runtime spawning, which we
// can intercept later in Phase 3.
void Anchor::EnemySync_OnSceneSpawnActors() {
    if (!IsSaveLoaded() || !isConnected) {
        return;
    }

    bool authority = IsAuthorityForCurrentScene();

    for (u8 cat : kEnemySyncCategories) {
        Actor* actor = gPlayState->actorCtx.actorLists[cat].head;
        while (actor != NULL) {
            Actor* next = actor->next;

            if (IsSyncableEnemy(actor)) {
                EnemyNetState* state = GetOrCreateNetState(actor);

                // Already accounted for: either we broadcast this actor on
                // a previous pass (authority side) or it was instantiated
                // locally from a remote ENEMY_SPAWN / ENEMY_FULL_SNAPSHOT
                // (non-authority side). Skip it; touching state would
                // either re-broadcast a duplicate or kill a remote-owned
                // actor we just spawned.
                if (state->enemyNetId != 0) {
                    actor = next;
                    continue;
                }

                state->isSynced = true;
                state->isAuthority = authority;

                if (authority) {
                    state->enemyNetId = MintEnemyNetId();
                    enemyNetIdToActor[state->enemyNetId] = actor;
                    SendPacket_EnemySpawn(actor, state->enemyNetId);
                } else {
                    // Non-authority: kill the engine-spawned local copy. The
                    // authority's ENEMY_SPAWN / ENEMY_FULL_SNAPSHOT will
                    // recreate it.
                    Actor_Kill(actor);
                }
            }

            actor = next;
        }
    }
}

// Called every game frame on the game thread. Throttled to 15 Hz (every 4
// frames) for ENEMY_UPDATE broadcast. This is the central authority->peers
// data path during steady-state combat.
void Anchor::EnemySync_TickAuthorityBroadcast() {
    if (!IsSaveLoaded() || !isConnected) {
        return;
    }
    if (!IsAuthorityForCurrentScene()) {
        return;
    }

    // ~30Hz at 60fps. Receivers LERP between consecutive snapshots in
    // EnemySync_TickNonAuthorityLerp so visible motion stays smooth at
    // 60fps render rate even though we only push twice per tick.
    enemySyncTickCounter++;
    if ((enemySyncTickCounter & 0x1) != 0) {
        return;
    }

    nlohmann::json enemies = nlohmann::json::array();

    for (u8 cat : kEnemySyncCategories) {
        Actor* actor = gPlayState->actorCtx.actorLists[cat].head;
        while (actor != NULL) {
            if (IsSyncableEnemy(actor)) {
                EnemyNetState* state = GetOrCreateNetState(actor);
                if (state->isAuthority && state->enemyNetId != 0) {
                    nlohmann::json e;
                    e["id"] = state->enemyNetId;
                    e["pos"] = actor->world.pos;
                    e["rot"] = actor->shape.rot;
                    e["velX"] = actor->velocity.x;
                    e["velY"] = actor->velocity.y;
                    e["velZ"] = actor->velocity.z;
                    e["hp"] = actor->colChkInfo.health;

                    const EnemyFamily* family = EnemyFamilyRegistry::Find(actor->id);
                    if (family != nullptr && family->serializeAI != nullptr) {
                        family->serializeAI(actor, e);
                    }

                    enemies.push_back(e);
                }
            }
            actor = actor->next;
        }
    }

    if (enemies.empty()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_UPDATE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["enemies"] = enemies;
    payload["quiet"] = true;

    // Diagnostic: log every ~5s (150 ticks at 30Hz). Tick counter + count
    // gives enough signal to correlate against peer-side receive logs and
    // server-side relay counters.
    if ((enemySyncTickCounter & 0x12C) == 0x12C) {
        SPDLOG_INFO("[Anchor:diag] EnemySync broadcast scene={} count={} tick={}",
                    gPlayState->sceneNum, (int)enemies.size(), enemySyncTickCounter);
    }

    // Broadcast to room (no targetClientId / targetTeamId): the server will
    // fan out to every other client in the room. Receivers self-filter on
    // sceneNum and FEATURE_ENEMY_SYNC.
    SendJsonToRemote(payload);
}

// Non-authority path: the engine flagged AC_HIT on a synced enemy. We must
// (a) consume the hit so it's not processed locally, and (b) forward it to
// the authority so it can replay the hit through vanilla AI.
void Anchor::EnemySync_HandleNonAuthorityHit(Actor* actor) {
    if (actor == nullptr) {
        return;
    }
    EnemyNetState* state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
    if (state == nullptr) {
        SPDLOG_INFO("[Anchor:diag] HandleNonAuthorityHit actorId=0x{:x} state=null (no NetState)", actor->id);
        return;
    }
    if (!state->isSynced) {
        SPDLOG_INFO("[Anchor:diag] HandleNonAuthorityHit actorId=0x{:x} early-return: !isSynced (netId={})",
                    actor->id, state->enemyNetId);
        return;
    }
    if (state->isAuthority) {
        SPDLOG_INFO("[Anchor:diag] HandleNonAuthorityHit actorId=0x{:x} early-return: isAuthority (netId={})",
                    actor->id, state->enemyNetId);
        return;
    }
    if (state->enemyNetId == 0) {
        SPDLOG_INFO("[Anchor:diag] HandleNonAuthorityHit actorId=0x{:x} early-return: enemyNetId==0", actor->id);
        return;
    }

    u8 damage = actor->colChkInfo.damage;
    u8 damageEffect = actor->colChkInfo.damageEffect;

    if (damage == 0) {
        // No damage was actually applied this frame; nothing to forward.
        return;
    }

    uint32_t authority = GetSceneAuthorityClientId(gPlayState->sceneNum);
    if (authority == 0) {
        SPDLOG_INFO("[Anchor:diag] HandleNonAuthorityHit actorId=0x{:x} netId={} damage={} early-return: authority==0",
                    actor->id, state->enemyNetId, damage);
        return;
    }

    SPDLOG_INFO("[Anchor:diag] HandleNonAuthorityHit forwarding actorId=0x{:x} netId={} damage={} -> authority={}",
                actor->id, state->enemyNetId, damage, authority);
    SendPacket_EnemyDamage(state->enemyNetId, authority, damage, damageEffect);

    // Clear the hit locally. Vanilla update is suppressed, so the actor's own
    // AC-flag reset code never runs; without this, the same hit would
    // re-forward every frame until the engine clears it some other way.
    actor->colChkInfo.damage = 0;
    actor->colChkInfo.damageEffect = 0;

    const EnemyFamily* family = EnemyFamilyRegistry::Find(actor->id);
    if (family != nullptr && family->clearACHits != nullptr) {
        family->clearACHits(actor);
    }
}

void Anchor::EnemySync_OnActorDestroy(Actor* actor) {
    if (actor == nullptr) {
        return;
    }
    EnemyNetState* state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
    if (state == nullptr) {
        return;
    }
    if (state->enemyNetId != 0) {
        auto it = enemyNetIdToActor.find(state->enemyNetId);
        if (it != enemyNetIdToActor.end() && it->second == actor) {
            enemyNetIdToActor.erase(it);
        }
    }
    state->enemyNetId = 0;
    state->isSynced = false;
}

// Non-authority side: walks every synced enemy in the current scene and
// LERPs world.pos / shape.rot from the last sample toward the most recent
// ENEMY_UPDATE target. Without this the actor snaps once per packet at
// 30Hz, which reads as a stutter against 60fps render. Health, velocity,
// and per-family AI fields are still applied directly in
// HandlePacket_EnemyUpdate; we only smooth pose here.
void Anchor::EnemySync_TickNonAuthorityLerp() {
    if (!IsSaveLoaded() || !isConnected) {
        return;
    }
    if (IsAuthorityForCurrentScene()) {
        return;
    }

    for (u8 cat : kEnemySyncCategories) {
        Actor* actor = gPlayState->actorCtx.actorLists[cat].head;
        while (actor != NULL) {
            if (IsSyncableEnemy(actor)) {
                EnemyNetState* state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
                if (state != nullptr && state->isSynced && !state->isAuthority && state->lerpInterval > 0) {
                if (state->lerpFrame < state->lerpInterval) {
                    state->lerpFrame++;
                }
                float alpha = (float)state->lerpFrame / (float)state->lerpInterval;
                if (alpha > 1.0f) {
                    alpha = 1.0f;
                }
                actor->world.pos.x = state->prevPosX + (state->targetPosX - state->prevPosX) * alpha;
                actor->world.pos.y = state->prevPosY + (state->targetPosY - state->prevPosY) * alpha;
                actor->world.pos.z = state->prevPosZ + (state->targetPosZ - state->prevPosZ) * alpha;
                // Shortest-path s16 LERP: cast the delta to s16 so the
                // wraparound across +/- 0x8000 stays correct (e.g. going
                // from 0x7FFF to 0x8001 takes 2 steps, not 0xFFFE).
                int16_t dRotX = (int16_t)(state->targetRotX - state->prevRotX);
                int16_t dRotY = (int16_t)(state->targetRotY - state->prevRotY);
                int16_t dRotZ = (int16_t)(state->targetRotZ - state->prevRotZ);
                    actor->shape.rot.x = state->prevRotX + (int16_t)(dRotX * alpha);
                    actor->shape.rot.y = state->prevRotY + (int16_t)(dRotY * alpha);
                    actor->shape.rot.z = state->prevRotZ + (int16_t)(dRotZ * alpha);
                    actor->world.rot.y = actor->shape.rot.y;
                }
            }
            actor = actor->next;
        }
    }
}

// Non-authority: per-family colliders have to be (re)registered with
// the collision check context every frame so the engine's
// CollisionCheck_AC / CollisionCheck_AT passes actually test against
// this enemy. Vanilla actors do this from inside their own update fn
// (see e.g. z_en_skb.c:514/520), but we suppress that update on non-
// authority -- without re-registering here, AC_HIT would never get
// set (so peer hits never forward to authority), and AT collisions
// against the player would never fire (so the peer's player would
// walk through enemies untouched).
//
// We also force AC_ON+AT_ON and clear AC_HARD on every collider before
// registering. The actor's update fn is what normally toggles those
// flags between hittable and "block all damage" states (e.g. Deku Baba
// in its retracted Wait state sets AC_HARD so swords bounce off; some
// enemies gate AT on by attack state). Since we suppress the update
// on peers, the flags would stay stuck at whatever the engine seeded
// them with on init. Authority runs vanilla AI so its own collider
// state stays correct; we only diverge here on peers. The trade-off
// is that peers may take touch damage from enemies whose authority
// considers them currently inactive (e.g. retracted Karebaba) -- a
// minor mechanical desync acceptable for v1.
//
// For ColliderCylinder we also call Collider_UpdateCylinder so the
// collider's stored position tracks actor->world.pos (which is being
// LERPed every frame from ENEMY_UPDATE). Without this the collider
// stays at the spawn position and AC/AT both check against stale
// coordinates. ColliderJntSph positions are updated from inside the
// actor's draw fn (still runs on peer), so no manual update needed.
//
// The actual per-collider register calls live in EnemySync/Helpers.h
// (RegisterCyl / RegisterJntSph / RegisterColliderCommon) so each
// Families/<Name>.cpp can call into them without needing this file.

void Anchor::EnemySync_RegisterAC(Actor* actor) {
    if (actor == nullptr || gPlayState == nullptr) {
        return;
    }
    const EnemyFamily* family = EnemyFamilyRegistry::Find(actor->id);
    if (family != nullptr && family->registerAC != nullptr) {
        family->registerAC(actor);
    }
}

// Authority side: scan ACTORCAT_MISC for new EnItem00 actors that were
// spawned near a synced enemy and broadcast them as ITEM_SPAWN so peers
// see the same drop. Synced enemies (Deku Baba, Stalchild, etc.) call
// Item_DropCollectibleRandom from inside their dying action funcs --
// that fn runs only on authority because peers have ShouldActorUpdate
// suppressed, so without this rebroadcast peers never see the dropped
// stick / nut / heart.
//
// Proximity check: the dying enemy is still in ACTORCAT_ENEMY for the
// frames between drop and Actor_Kill (death anim plays out first), so
// we just check distance against any live synced enemy. 80-unit radius
// is generous enough to cover Deku Baba's drop offset (it spawns the
// item slightly above its body) without bleeding into unrelated drops.
void Anchor::EnemySync_TrackEnemyDrops() {
    if (!IsSaveLoaded() || !isConnected) {
        return;
    }
    if (!IsAuthorityForCurrentScene()) {
        return;
    }

    Actor* item = gPlayState->actorCtx.actorLists[ACTORCAT_MISC].head;
    while (item != NULL) {
        Actor* nextItem = item->next;
        if (item->id != ACTOR_EN_ITEM00 ||
            enemyItemDropsBroadcast.count(item) ||
            itemActorToId.count(item)) {
            item = nextItem;
            continue;
        }

        // Gate on hp == 0 (dying or dead) so we don't accidentally replicate
        // a grass-cut rupee that happened next to a live synced enemy. The
        // synced enemy's death anim runs across many frames between hp
        // hitting 0 and Actor_Kill removing it from the list, which is the
        // window during which the drop spawns.
        bool nearSyncedEnemy = false;
        Actor* enemy = gPlayState->actorCtx.actorLists[ACTORCAT_ENEMY].head;
        while (enemy != NULL) {
            if (IsSyncableEnemy(enemy) && enemy->colChkInfo.health == 0) {
                EnemyNetState* st = ObjectExtension::GetInstance().Get<EnemyNetState>(enemy);
                if (st != nullptr && st->isSynced && st->enemyNetId != 0) {
                    f32 dx = item->world.pos.x - enemy->world.pos.x;
                    f32 dy = item->world.pos.y - enemy->world.pos.y;
                    f32 dz = item->world.pos.z - enemy->world.pos.z;
                    if (dx * dx + dy * dy + dz * dz < 80.0f * 80.0f) {
                        nearSyncedEnemy = true;
                        break;
                    }
                }
            }
            enemy = enemy->next;
        }

        if (nearSyncedEnemy) {
            enemyItemDropsBroadcast.insert(item);
            uint64_t itemId = ((uint64_t)ownClientId << 32) | (++itemSpawnSeq);
            itemActorToId[item] = itemId;
            itemIdToActor[itemId] = item;
            SendPacket_ItemSpawn(gPlayState->sceneNum, itemId, item->world.pos.x, item->world.pos.y,
                                 item->world.pos.z, (s16)item->params);
        }

        item = nextItem;
    }
}

void Anchor::EnemySync_OnEnemyDefeat(Actor* actor) {
    if (actor == nullptr || !isConnected) {
        return;
    }
    EnemyNetState* state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
    if (state == nullptr) {
        SPDLOG_INFO("[Anchor:diag] OnEnemyDefeat actorId=0x{:x} state=null (no NetState)", actor->id);
        return;
    }
    if (!state->isSynced) {
        SPDLOG_INFO("[Anchor:diag] OnEnemyDefeat actorId=0x{:x} early-return: !isSynced", actor->id);
        return;
    }
    if (!state->isAuthority) {
        SPDLOG_INFO("[Anchor:diag] OnEnemyDefeat actorId=0x{:x} netId={} early-return: !isAuthority",
                    actor->id, state->enemyNetId);
        return;
    }
    if (state->enemyNetId == 0) {
        SPDLOG_INFO("[Anchor:diag] OnEnemyDefeat actorId=0x{:x} early-return: enemyNetId==0", actor->id);
        return;
    }
    SPDLOG_INFO("[Anchor:diag] OnEnemyDefeat broadcasting ENEMY_DEATH actorId=0x{:x} netId={}",
                actor->id, state->enemyNetId);
    SendPacket_EnemyDeath(state->enemyNetId);
}
