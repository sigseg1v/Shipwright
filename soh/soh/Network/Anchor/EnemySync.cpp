#include "Anchor.h"
#include "EnemySync.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include "soh/ObjectExtension/ObjectExtension.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
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

// Stalchild is the only actor type wired in Phase 2. Future actor families
// extend this list (and add per-family update-payload extensions).
static bool IsSyncableEnemy(const Actor* actor) {
    if (actor == nullptr) {
        return false;
    }
    return actor->id == ACTOR_EN_SKB;
}

// Pure function over Anchor::clients: lowest clientId of any peer in `sceneNum`
// that's online and save-loaded. Returns 0 if no eligible client (treat as "no
// authority needed"; caller should bail).
uint32_t Anchor::GetSceneAuthorityClientId(s16 sceneNum) {
    uint32_t winner = 0;
    for (auto& [clientId, client] : clients) {
        if (!client.online || !client.isSaveLoaded) {
            continue;
        }
        if (client.sceneNum != sceneNum) {
            continue;
        }
        if (winner == 0 || clientId < winner) {
            winner = clientId;
        }
    }
    return winner;
}

bool Anchor::IsAuthorityForCurrentScene() {
    if (!IsSaveLoaded()) {
        return false;
    }
    uint32_t authority = GetSceneAuthorityClientId(gPlayState->sceneNum);
    // If we're the only client in our own clients map, we're authority.
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

    // Reset per-scene network-id table; old IDs do not survive scene
    // transitions because the Actor* pointers don't either.
    enemyNetIdToActor.clear();

    bool authority = IsAuthorityForCurrentScene();

    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_ENEMY].head;
    while (actor != NULL) {
        Actor* next = actor->next;

        if (IsSyncableEnemy(actor)) {
            EnemyNetState* state = GetOrCreateNetState(actor);
            state->isSynced = true;
            state->isAuthority = authority;

            if (authority) {
                if (state->enemyNetId == 0) {
                    state->enemyNetId = MintEnemyNetId();
                }
                enemyNetIdToActor[state->enemyNetId] = actor;
                SendPacket_EnemySpawn(actor, state->enemyNetId);
            } else {
                // Non-authority: kill the engine-spawned local copy. The
                // authority's ENEMY_SPAWN broadcast will recreate it.
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
    if (actor->id == ACTOR_EN_SKB) {
        EnSkb* skb = reinterpret_cast<EnSkb*>(actor);
        skb->collider.base.acFlags &= ~AC_HIT;
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
