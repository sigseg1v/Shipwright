#ifndef NETWORK_ANCHOR_MECHANICSYNC_H
#define NETWORK_ANCHOR_MECHANICSYNC_H

#include <string>
#include <cstdint>

// Stable in-scene identity for a mechanic actor. Mechanics (push blocks,
// elevators, spike traps, etc.) are scene-defined: every client spawns
// the same instances from the room file at the same home positions,
// so (actorId, home.pos) is deterministic across clients and survives
// reconnects -- no minted netId needed.
std::string MechanicSync_MakeKey(int16_t actorId, float homeX, float homeY, float homeZ);

#endif // NETWORK_ANCHOR_MECHANICSYNC_H
