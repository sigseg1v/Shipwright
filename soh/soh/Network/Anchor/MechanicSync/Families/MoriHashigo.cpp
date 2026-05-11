#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Forest Temple ladder that drops when block falls.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_MORI_HASHIGO, &ShouldSync, nullptr, nullptr }));
