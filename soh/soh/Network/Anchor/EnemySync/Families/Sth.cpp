#include "../Family.h"
#include "../Registry.h"

// En_Sth. Stub registration: peers receive pos/HP broadcasts but no
// custom collider re-register or AC clear. Tighten per-family later if
// peers need to take damage from this actor or hit it.

ANCHOR_REGISTER_ENEMY_FAMILY((EnemyFamily{ ACTOR_EN_STH, nullptr, nullptr, nullptr, nullptr }));
