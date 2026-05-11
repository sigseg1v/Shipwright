#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Forest Temple elevator. Pure pos.y oscillation driven by the local
// switch flag, but two clients pulling the switch at slightly different
// times will desync the platform y. Always sync; the cost (one entry
// per scene in Forest Temple) is negligible.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_MORI_ELEVATOR, &ShouldSync, nullptr, nullptr }));
