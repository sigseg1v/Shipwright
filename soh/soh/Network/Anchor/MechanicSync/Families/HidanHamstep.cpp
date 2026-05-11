#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Fire Temple Megaton-Hammer pound steps.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_HIDAN_HAMSTEP, &ShouldSync, nullptr, nullptr }));
