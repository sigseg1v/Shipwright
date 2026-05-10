#ifndef NETWORK_ANCHOR_ENEMY_SYNC_H
#define NETWORK_ANCHOR_ENEMY_SYNC_H
#ifdef __cplusplus

#include <cstdint>

// Per-actor side state attached to every synced enemy via ObjectExtension.
// Definition lives in a header so all packet handlers see the same type
// (ObjectExtension keys on Register<T>::Id, and that template's static is
// keyed by the type, so all TUs must agree on the type's identity).
//
// LERP fields are populated on non-authority when ENEMY_UPDATE arrives:
// the actor's current pos/rot becomes the prev sample, the packet payload
// becomes the target sample, and EnemySync_TickNonAuthorityLerp walks
// alpha from 0 -> 1 over lerpInterval frames. Without this, snapping on
// each packet at 30Hz looks like a 30Hz strobe; lerping smooths it back
// to the 60fps render rate.
struct EnemyNetState {
    uint32_t enemyNetId = 0;
    bool isAuthority = false;
    bool isSynced = false;

    float prevPosX = 0.0f, prevPosY = 0.0f, prevPosZ = 0.0f;
    float targetPosX = 0.0f, targetPosY = 0.0f, targetPosZ = 0.0f;
    int16_t prevRotX = 0, prevRotY = 0, prevRotZ = 0;
    int16_t targetRotX = 0, targetRotY = 0, targetRotZ = 0;
    int32_t lerpFrame = 0;
    int32_t lerpInterval = 0;
};

#endif // __cplusplus
#endif // NETWORK_ANCHOR_ENEMY_SYNC_H
