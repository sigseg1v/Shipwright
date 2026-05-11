#ifndef NETWORK_ANCHOR_ENEMYSYNC_FAMILY_H
#define NETWORK_ANCHOR_ENEMYSYNC_FAMILY_H

#include <nlohmann/json.hpp>

extern "C" {
#include "z64.h"
}

// Per-family hooks for enemy sync. One static EnemyFamily instance is
// declared per overlay file under EnemySync/Families/, registered into
// the EnemyFamilyRegistry by a static initializer in that file. The
// dispatch helpers in EnemySync.cpp look the family up by actor id and
// call into these function pointers, so adding a new enemy family does
// not require touching EnemySync.cpp / HookHandlers.cpp / EnemyUpdate.cpp
// switch statements.
//
// Required: actorId, registerAC, clearACHits.
//   - registerAC re-registers the actor's collider(s) every frame on
//     non-authority clients (the engine's per-frame Setup pass is
//     suppressed there because we set ShouldActorUpdate=false). Without
//     it, AC/AT collision wouldn't land on the synced corpse.
//   - clearACHits zeroes the AC_HIT bit on every collider after we've
//     forwarded the hit to the authority, so the next collision pass
//     can detect a fresh strike.
//
// Optional: serializeAI, applyAI for per-family AI state piggybacked
// on ENEMY_UPDATE (e.g. EnSkb's actionState / breakFlags). May be null.
//
// Optional: shouldSpawnLocally — if non-null and returns false, the
// authority side will skip broadcasting an ENEMY_SPAWN for this actor
// even though it's syncable. Defaults to "always sync" when null.

struct EnemyFamily {
    s16 actorId;
    void (*registerAC)(Actor* actor);
    void (*clearACHits)(Actor* actor);
    void (*serializeAI)(const Actor* actor, nlohmann::json& payload);
    void (*applyAI)(Actor* actor, const nlohmann::json& payload);
};

#endif // NETWORK_ANCHOR_ENEMYSYNC_FAMILY_H
