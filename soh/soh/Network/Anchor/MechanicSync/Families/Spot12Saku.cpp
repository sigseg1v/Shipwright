#include "../Family.h"
#include "../Registry.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
}

// Gerudo Fortress fence/bars.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_SPOT12_SAKU, &ShouldSync, nullptr, nullptr }));
