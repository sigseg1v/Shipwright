#include "Anchor.h"
#include "EnemySync.h"
#include "JsonConversions.hpp"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include "soh/ObjectExtension/ObjectExtension.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "src/overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"
#include "src/overlays/actors/ovl_En_Dekunuts/z_en_dekunuts.h"
#include "src/overlays/actors/ovl_En_Goma/z_en_goma.h"
// z_en_st.h declares an action-func typedef using `this` as the parameter
// name, which is a reserved word in C++. Locally rename it during include
// only -- the redefinition is identifier-name only, so the struct layout
// and any `this` callers in .c files are unaffected.
#define this thisx
#include "src/overlays/actors/ovl_En_St/z_en_st.h"
#undef this
#include "src/overlays/actors/ovl_En_Sw/z_en_sw.h"
extern PlayState* gPlayState;
}

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

// Actor types covered by host-authoritative sync. Adding an entry here is
// the minimum to start syncing pos/rot/hp/vel; per-family AI fields (like
// Stalchild's actionState) need their own branch in the spawn/update
// handlers as well. Visible animation may stutter on non-authority for
// actors whose anim is driven inside their update fn -- that's the known
// v1 gap noted in planning/oot-coop-enemy-sync-plan.md (Phase 3 polish).
static bool IsSyncableEnemy(const Actor* actor) {
    if (actor == nullptr) {
        return false;
    }
    switch (actor->id) {
        case ACTOR_EN_SKB:       // Stalchild
        case ACTOR_EN_DEKUBABA:  // Deku Baba (deku stick plant)
        case ACTOR_EN_KAREBABA:  // Big/Withered Deku Baba
        case ACTOR_EN_DEKUNUTS:  // Mad Scrub
        case ACTOR_EN_GOMA:      // Gohma Larva
        case ACTOR_EN_ST:        // Skulltula (web-hanging)
        case ACTOR_EN_SW:        // Skullwalltula (wall-crawler)
            return true;
        default:
            return false;
    }
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

    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_ENEMY].head;
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

    // ~15Hz at 60fps. Stalchild AI is deliberate enough that this is plenty.
    enemySyncTickCounter++;
    if ((enemySyncTickCounter & 0x3) != 0) {
        return;
    }

    nlohmann::json enemies = nlohmann::json::array();

    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_ENEMY].head;
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

                if (actor->id == ACTOR_EN_SKB) {
                    EnSkb* skb = reinterpret_cast<EnSkb*>(actor);
                    e["skbActionState"] = skb->actionState;
                    e["skbBreakFlags"] = skb->breakFlags;
                    e["skbHeadlessYaw"] = skb->headlessYawOffset;
                }

                enemies.push_back(e);
            }
        }
        actor = actor->next;
    }

    if (enemies.empty()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_UPDATE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["enemies"] = enemies;
    payload["quiet"] = true;

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
    if (state == nullptr || !state->isSynced || state->isAuthority || state->enemyNetId == 0) {
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
        return;
    }

    SendPacket_EnemyDamage(state->enemyNetId, authority, damage, damageEffect);

    // Clear the hit locally. Vanilla update is suppressed, so the actor's own
    // AC-flag reset code never runs; without this, the same hit would
    // re-forward every frame until the engine clears it some other way.
    actor->colChkInfo.damage = 0;
    actor->colChkInfo.damageEffect = 0;
    switch (actor->id) {
        case ACTOR_EN_SKB: {
            EnSkb* a = reinterpret_cast<EnSkb*>(actor);
            a->collider.base.acFlags &= ~AC_HIT;
            break;
        }
        case ACTOR_EN_DEKUBABA: {
            EnDekubaba* a = reinterpret_cast<EnDekubaba*>(actor);
            a->collider.base.acFlags &= ~AC_HIT;
            break;
        }
        case ACTOR_EN_KAREBABA: {
            EnKarebaba* a = reinterpret_cast<EnKarebaba*>(actor);
            a->headCollider.base.acFlags &= ~AC_HIT;
            a->bodyCollider.base.acFlags &= ~AC_HIT;
            break;
        }
        case ACTOR_EN_DEKUNUTS: {
            EnDekunuts* a = reinterpret_cast<EnDekunuts*>(actor);
            a->collider.base.acFlags &= ~AC_HIT;
            break;
        }
        case ACTOR_EN_GOMA: {
            EnGoma* a = reinterpret_cast<EnGoma*>(actor);
            a->colCyl1.base.acFlags &= ~AC_HIT;
            a->colCyl2.base.acFlags &= ~AC_HIT;
            break;
        }
        case ACTOR_EN_ST: {
            EnSt* a = reinterpret_cast<EnSt*>(actor);
            a->colSph.base.acFlags &= ~AC_HIT;
            for (int i = 0; i < 6; i++) {
                a->colCylinder[i].base.acFlags &= ~AC_HIT;
            }
            break;
        }
        case ACTOR_EN_SW: {
            EnSw* a = reinterpret_cast<EnSw*>(actor);
            a->collider.base.acFlags &= ~AC_HIT;
            break;
        }
        default:
            break;
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

void Anchor::EnemySync_OnEnemyDefeat(Actor* actor) {
    if (actor == nullptr || !isConnected) {
        return;
    }
    EnemyNetState* state = ObjectExtension::GetInstance().Get<EnemyNetState>(actor);
    if (state == nullptr || !state->isSynced || !state->isAuthority || state->enemyNetId == 0) {
        return;
    }
    SendPacket_EnemyDeath(state->enemyNetId);
}
