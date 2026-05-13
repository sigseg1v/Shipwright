#ifndef NETWORK_ANCHOR_ENEMYSYNC_FAMILY_H
#define NETWORK_ANCHOR_ENEMYSYNC_FAMILY_H

#include <nlohmann/json.hpp>

#include "z64.h"

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
// Optional: setACHits is the authority-side mirror of clearACHits. when
// a peer reports a hit via ENEMY_DAMAGE, the authority writes the
// damage into colChkInfo and then calls this to raise AC_HIT on every
// collider the actor's UpdateDamage / action-dispatch fn polls, so the
// vanilla AI routes the hit through the stagger / death paths. may be
// null for families whose AI doesn't gate on AC_HIT (boss scripts that
// read colChkInfo.damage directly).
//
// Optional: getAttackerActorId is the peer-side companion to setACHits.
// some overlays read `collider.base.ac->id` inside their ColliderCheck
// (EnHintnuts is the puzzle-scrub example: the reflected-nutsball path
// vs the burrow path branches on `ac->id == ACTOR_EN_NUTSBALL`). The
// engine's CollisionCheck_AC fills `base.ac` on a real collision pass
// but our synthesized AC_HIT (via SetCyl/JntSphAcHit) leaves it stale.
// If this hook is provided, the peer reads the attacker id off the
// relevant collider and we ship it in the ENEMY_DAMAGE packet so the
// authority can fabricate a dummy Actor with that id and point
// `base.ac` at it before the actor's next Update. May be null for
// families whose damage handler never dereferences `base.ac`.

struct EnemyFamily {
    s16 actorId;
    void (*registerAC)(Actor* actor);
    void (*clearACHits)(Actor* actor);
    void (*serializeAI)(const Actor* actor, nlohmann::json& payload);
    void (*applyAI)(Actor* actor, const nlohmann::json& payload);
    void (*setACHits)(Actor* actor);
    u16 (*getAttackerActorId)(Actor* actor);
};

#endif // NETWORK_ANCHOR_ENEMYSYNC_FAMILY_H
