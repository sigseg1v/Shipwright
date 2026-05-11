#include "../Family.h"
#include "../Registry.h"

#include "macros.h"
#include "variables.h"
#include "functions.h"

// Per-room countdown timer (e.g. Fire Temple heat room, GTG silver rupee rooms).

namespace {

bool ShouldSync(const Actor* actor) {
    (void)actor;
    return true;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_OBJ_ROOMTIMER, &ShouldSync, nullptr, nullptr }));
