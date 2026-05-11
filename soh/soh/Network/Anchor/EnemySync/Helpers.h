#ifndef NETWORK_ANCHOR_ENEMYSYNC_HELPERS_H
#define NETWORK_ANCHOR_ENEMYSYNC_HELPERS_H

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
// LERP actor->world.pos every frame from ENEMY_UPDATE; without
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

}  // namespace EnemySyncHelpers

#endif  // NETWORK_ANCHOR_ENEMYSYNC_HELPERS_H
