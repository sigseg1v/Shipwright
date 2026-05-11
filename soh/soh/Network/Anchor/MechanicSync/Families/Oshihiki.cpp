#include "../Family.h"
#include "../Registry.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_Obj_Oshihiki/z_obj_oshihiki.h"
}

// Push block. Static the moment Link isn't pushing it, so we gate the
// broadcast on PUSHBLOCK_PUSH | PUSHBLOCK_FALL -- only states where
// world.pos is actually changing. The receiver mirrors world.pos
// directly so peers see the block slide in lockstep with the pusher.
//
// Note: we don't sync stateFlags. Each peer derives its own pushing
// state from local Link's input; the visible result (block position)
// is what authoritatively flows over the wire.

namespace {

bool ShouldSync(const Actor* actor) {
    auto* o = reinterpret_cast<const ObjOshihiki*>(actor);
    return (o->stateFlags & (PUSHBLOCK_PUSH | PUSHBLOCK_FALL | PUSHBLOCK_MOVE_UNDER)) != 0;
}

}  // namespace

ANCHOR_REGISTER_MECHANIC_FAMILY((MechanicFamily{ ACTOR_OBJ_OSHIHIKI, &ShouldSync, nullptr, nullptr }));
