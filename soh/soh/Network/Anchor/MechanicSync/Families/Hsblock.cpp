#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Hookshot target post that pops up out of the floor (e.g. Forest Temple courtyard).

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_OBJ_HSBLOCK, &ShouldSync, nullptr, nullptr }));
