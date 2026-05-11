#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Shadow Temple guillotine and spike traps.

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_BG_HAKA_TRAP, &ShouldSync, nullptr, nullptr }));
