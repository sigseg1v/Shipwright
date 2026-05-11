#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/EnemySync.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

#include "macros.h"
#include "variables.h"
extern "C" {
extern PlayState* gPlayState;
}

// PEER_ENTERED_SCENE
//
// Server-initiated notification that another client just transitioned
// into a scene that we (the recipient) are already authority for. We
// react by pushing an ENEMY_FULL_SNAPSHOT to the named peer so they
// spawn the existing live enemies immediately, instead of having to
// wait for the next ENEMY_UPDATE tick (which only refreshes already-
// known actors and would leave the joiner with an empty scene).

void Anchor::HandlePacket_PeerEnteredScene(nlohmann::json payload) {
    if (!IsSaveLoaded() || gPlayState == nullptr) {
        return;
    }
    if (!payload.contains("sceneNum") || !payload.contains("peerClientId")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }
    if (!IsAuthorityForCurrentScene()) {
        return;
    }
    uint32_t peerClientId = payload["peerClientId"].get<uint32_t>();
    if (peerClientId == 0 || peerClientId == ownClientId) {
        return;
    }
    SendPacket_EnemyFullSnapshot(peerClientId);
}
