#ifndef NETWORK_ANCHOR_ENEMY_SYNC_H
#define NETWORK_ANCHOR_ENEMY_SYNC_H
#ifdef __cplusplus

#include <cstdint>

// Per-actor side state attached to every synced enemy via ObjectExtension.
// Definition lives in a header so all packet handlers see the same type
// (ObjectExtension keys on Register<T>::Id, and that template's static is
// keyed by the type, so all TUs must agree on the type's identity).
struct EnemyNetState {
    uint32_t enemyNetId = 0;
    bool isAuthority = false;
    bool isSynced = false;
};

#endif // __cplusplus
#endif // NETWORK_ANCHOR_ENEMY_SYNC_H
