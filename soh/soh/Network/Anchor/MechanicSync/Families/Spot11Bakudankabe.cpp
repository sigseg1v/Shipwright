#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Desert Colossus bombable wall.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_SPOT11_BAKUDANKABE, &ShouldSync, nullptr, nullptr }));
