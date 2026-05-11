#ifndef NETWORK_ANCHOR_MECHANICSYNC_FAMILY_H
#define NETWORK_ANCHOR_MECHANICSYNC_FAMILY_H

#include <nlohmann/json.hpp>

#include "z64.h"

// Per-family hooks for mechanic sync (moving Bg_* / Obj_* actors that
// the player can interact with but which aren't combat enemies).
//
// One MechanicFamily instance is registered per overlay file under
// MechanicSync/Families/<Name>.cpp. Mechanic sync streams the actor's
// pos/rot from the scene authority to peers each tick; the engine
// keeps the actor's own action funcs running on both sides, so this
// is purely a "pin position" overlay.
//
// Required:
//   actorId       -- ACTOR_OBJ_* / ACTOR_BG_* the family covers.
//   shouldSync    -- gate on a per-actor basis. e.g. push blocks only
//                    sync while moving (stateFlags & PUSHBLOCK_PUSH).
//                    pass nullptr to always sync.
//
// Optional:
//   serializeAux / applyAux
//     for extra per-family state (action timer, stateFlags, etc.).
//     pass nullptr if pos+rot is enough.

struct MechanicFamily {
    s16 actorId;
    bool (*shouldSync)(const Actor* actor);
    void (*serializeAux)(const Actor* actor, nlohmann::json& payload);
    void (*applyAux)(Actor* actor, const nlohmann::json& payload);
};

#endif // NETWORK_ANCHOR_MECHANICSYNC_FAMILY_H
