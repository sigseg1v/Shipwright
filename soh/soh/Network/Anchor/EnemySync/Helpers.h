#ifndef NETWORK_ANCHOR_ENEMYSYNC_HELPERS_H
#define NETWORK_ANCHOR_ENEMYSYNC_HELPERS_H

// Pre-resolve every C++ stdlib header that libultraship's resourcebridge
// transitively pulls in. Several boss overlay headers (z_boss_*.h) and
// z_en_st.h include "global.h" -> libultraship bridge -> <variant> /
// <optional> / etc., and the family .cpp files include those overlay
// headers under a `#define this thisx` to dodge the OoT typedef that
// names a struct pointer "this". If <variant> first parses while that
// define is live, its member functions become `thisx->emplace(...)` and
// the build dies in libstdc++. Pulling libultraship.h in here, before
// any family file gets a chance to `#define this`, lets the header
// guards short-circuit the later transitive include.
#include <libultraship/libultraship.h>

// z64.h / macros.h / functions.h / variables.h each contain their own
// `#ifdef __cplusplus extern "C"` blocks. Wrapping them again in an
// outer extern "C" makes z64.h's `#include <memory>` (which lives inside
// its own __cplusplus guard) inherit C linkage, which the C++ stdlib
// rejects with "template with C linkage" errors on gcc 13.
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern "C" {
extern PlayState* gPlayState;
}

namespace EnemySyncHelpers {

// AC/AT toggling for a non-authority client. AC_HARD is cleared so
// melee weapons pass through after a hit instead of pinging off the
// remote-puppet collider, AC/AT are re-enabled (the engine's per-frame
// Setup pass that would have done this is suppressed for non-authority
// actors via ShouldActorUpdate=false), and we re-register with
// ColChkCtx so the next collision pass tests against this collider.
inline void RegisterColliderCommon(Collider* base) {
    base->acFlags = (base->acFlags | AC_ON) & ~AC_HARD;
    base->atFlags |= AT_ON;
    CollisionCheck_SetAC(gPlayState, &gPlayState->colChkCtx, base);
    CollisionCheck_SetAT(gPlayState, &gPlayState->colChkCtx, base);
}

// ColliderCylinders need their stored position refreshed because we
// overwrite actor->world.pos from each ENEMY_UPDATE; without
// Collider_UpdateCylinder the collider stays at the spawn pose and AC
// checks miss the visible enemy.
inline void RegisterCyl(Actor* actor, ColliderCylinder* c) {
    Collider_UpdateCylinder(actor, c);
    RegisterColliderCommon(&c->base);
}

// ColliderJntSph positions are kept in sync from inside the actor's
// draw fn (which still runs on non-authority), so just re-register.
inline void RegisterJntSph(ColliderJntSph* c) {
    RegisterColliderCommon(&c->base);
}

// Static placeholder used when we raise AC_HIT manually from a synced
// ENEMY_DAMAGE packet. Engine-driven AC hits would have CollisionCheck_AC
// fill in `info.acHitInfo` with the attacker's element; several enemy
// overlays (e.g. EnSt_CheckHitBackside) then dereference
// `info.acHitInfo->toucher.dmgFlags` the moment they see AC_HIT. Without a
// non-NULL pointer here the authority crashes on the very next Update.
// Zero-initialized so `dmgFlags == 0` -- the actor routes through its
// generic melee path rather than the arrow / fire / light death variant,
// which is a reasonable fallback until we plumb the attacker's real
// dmgFlags through the packet.
inline ColliderInfo& AcHitDummy() {
    static ColliderInfo dummy = {};
    return dummy;
}

// Set AC_HIT on a cylinder collider and point its acHitInfo at the
// dummy. Use this from EnemyFamily::setACHits instead of poking
// `c->base.acFlags |= AC_HIT` directly when the overlay reads
// `info.acHitInfo` in its damage handler.
inline void SetCylAcHit(ColliderCylinder* c) {
    c->base.acFlags |= AC_HIT;
    c->info.acHitInfo = &AcHitDummy();
}

// JntSph version: also points every element's per-info acHitInfo at the
// dummy. Most overlays only read the base AC_HIT for JntSph, but the
// element pointer is cheap to seed and saves us from a future crash if
// a family member ever does dereference it.
inline void SetJntSphAcHit(ColliderJntSph* c) {
    c->base.acFlags |= AC_HIT;
    for (int i = 0; i < c->count; i++) {
        c->elements[i].info.acHitInfo = &AcHitDummy();
    }
}

}  // namespace EnemySyncHelpers

#endif  // NETWORK_ANCHOR_ENEMYSYNC_HELPERS_H
