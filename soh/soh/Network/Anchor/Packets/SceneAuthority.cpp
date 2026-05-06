#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "macros.h"
}

// SCENE_AUTHORITY
//
// Server-driven per-scene authority assignment. The server elects "first
// connected client to enter a scene" as that scene's authority and
// broadcasts the result to every client in the room. Authority persists
// until the owning client leaves the scene or disconnects, at which
// point the server re-elects (lowest clientId tiebreaker among remaining
// clients in the scene) and re-broadcasts.
//
// Payload: { sceneNum, authorityClientId }. authorityClientId == 0 means
// "no current authority" (everyone has left the scene).
//
// Older anchor servers never emit this packet, in which case the
// sceneAuthorities map stays empty and GetSceneAuthorityClientId returns
// 0 -- the IsAuthorityForCurrentScene fallback then treats the local
// client as authority, matching pre-server-election behavior.

void Anchor::HandlePacket_SceneAuthority(nlohmann::json payload) {
    if (!payload.contains("sceneNum")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    uint32_t authorityClientId = payload.value("authorityClientId", (uint32_t)0);

    if (authorityClientId == 0) {
        sceneAuthorities.erase(sceneNum);
    } else {
        sceneAuthorities[sceneNum] = authorityClientId;
    }
}
