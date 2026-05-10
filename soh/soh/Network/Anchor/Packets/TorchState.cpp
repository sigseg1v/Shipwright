#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include <cstdio>

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_Obj_Syokudai/z_obj_syokudai.h"
extern PlayState* gPlayState;
}

// TORCH_STATE (FEATURE_WORLD_EVENT_SYNC)
//
// Real-time relay of OBJ_SYOKUDAI ignite/extinguish edges so peers see
// torches lit by another player without each having to relight. Switch-
// flag-driven persistence (e.g. lighting all torches in a group flips a
// switch that re-init reads) still rides SET_FLAG/UNSET_FLAG; this packet
// only covers the per-actor litTimer that lives entirely in the actor
// instance.
//
// Sender hooks ignite (litTimer transitions 0 -> nonzero) and
// extinguish (litTimer transitions nonzero -> 0). The receiver writes
// the carried litTimer verbatim and lets the local actor tick it down
// from there. Drift is acceptable: the timer is short (a few seconds
// to a few minutes) and the only downstream consumer is "is it still
// glowing" plus a switch-flag set on group completion, which is its
// own packet.

std::string Anchor::MakeTorchId(const Actor* actor) {
    char buf[80];
    std::snprintf(buf, sizeof(buf), "%d:%d:%.1f:%.1f:%.1f", (int)actor->id, (int)actor->params,
                  actor->home.pos.x, actor->home.pos.y, actor->home.pos.z);
    return buf;
}

void Anchor::SendPacket_TorchState(s16 sceneNum, const std::string& torchId, s16 litTimer) {
    nlohmann::json payload;
    payload["type"] = TORCH_STATE;
    payload["sceneNum"] = sceneNum;
    payload["torchId"] = torchId;
    payload["litTimer"] = (int)litTimer;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_TorchState(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("torchId") || !payload.contains("litTimer")) {
        return;
    }
    if (!IsSaveLoaded() || gPlayState == nullptr) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }
    std::string torchId = payload["torchId"].get<std::string>();
    s16 litTimer = (s16)payload["litTimer"].get<int>();

    // Update lastTorchLit alongside the litTimer write so the per-frame
    // edge detector won't misread our remote-applied transition as a
    // fresh local ignite/extinguish and echo TORCH_STATE back.
    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].head;
    while (actor != NULL) {
        Actor* next = actor->next;
        if (actor->id == ACTOR_OBJ_SYOKUDAI && actor->update != NULL) {
            if (MakeTorchId(actor) == torchId) {
                ObjSyokudai* torch = (ObjSyokudai*)actor;
                torch->litTimer = litTimer;
                lastTorchLit[actor] = (litTimer != 0);
                break;
            }
        }
        actor = next;
    }
}
