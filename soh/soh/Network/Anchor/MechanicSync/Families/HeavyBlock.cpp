#include "../Family.h"
#include "../Registry.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
}

// Heavy Block (the silver throwing pillar in Kakariko's windmill area
// and the Forest stage). World.pos moves only while it's airborne after
// the player throws it; otherwise it's static. Always-sync is cheap
// since there's at most one of these per scene.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_HEAVY_BLOCK, &ShouldSync, nullptr, nullptr }));
