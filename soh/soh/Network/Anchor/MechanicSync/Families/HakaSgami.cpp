#include "../Family.h"
#include "../Registry.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
}

// Shadow Temple spinning scythe trap.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_HAKA_SGAMI, &ShouldSync, nullptr, nullptr }));
