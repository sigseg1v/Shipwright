#include "../Family.h"
#include "../Registry.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
}

// Spirit Temple lift platform driven by switch state and timers.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_JYA_LIFT, &ShouldSync, nullptr, nullptr }));
