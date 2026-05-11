#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Wall/floor torch stand that can be lit with Din's Fire or a fire arrow.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_OBJ_SYOKUDAI, &ShouldSync, nullptr, nullptr }));
