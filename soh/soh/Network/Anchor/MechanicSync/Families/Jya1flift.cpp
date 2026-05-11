#include "../Family.h"
#include "../Registry.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
}

// Spirit Temple first-floor lift platform.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_JYA_1FLIFT, &ShouldSync, nullptr, nullptr }));
