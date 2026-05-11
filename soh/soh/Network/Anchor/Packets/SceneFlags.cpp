#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

// SCENE_FLAGS (FEATURE_SCENE_FLAGS_SYNC)
//
// Per-frame snapshot of gPlayState->actorCtx.flags broadcast by the
// scene authority. Catches runtime-only flag transitions that the
// SET_FLAG / UNSET_FLAG path misses:
//   - tempClear (no GameInteractor hook on Flags_SetTempClear)
//   - unk0 / unk1 (no GameInteractor hook on Flags_SetUnknown)
//   - any direct `actorCtx.flags.x |= bit` writes that bypass Flags_Set*
//
// Persistent fields (swch < 0x20, chest, clear, collect < 0x20) are
// also covered by SET_FLAG; the overlap is harmless because the
// snapshot is idempotent. We deliberately keep all 9 fields in the
// payload to keep the apply path a single memcpy-equivalent assignment
// and to absorb any future engine paths that mutate flags directly.
//
// Authority-only on the send side. Receivers blindly apply if the
// snapshot's sceneNum matches their current scene -- if a peer has
// just transitioned out, the next snapshot for the new scene will
// arrive shortly. We never apply a snapshot for a different scene
// because actorCtx.flags is per-scene-visit state.

namespace {

constexpr size_t kSceneFlagsCount = 9;

void ReadFlags(std::array<u32, kSceneFlagsCount>& out) {
    auto& f = gPlayState->actorCtx.flags;
    out[0] = f.swch;
    out[1] = f.tempSwch;
    out[2] = f.unk0;
    out[3] = f.unk1;
    out[4] = f.chest;
    out[5] = f.clear;
    out[6] = f.tempClear;
    out[7] = f.collect;
    out[8] = f.tempCollect;
}

void WriteFlags(const std::array<u32, kSceneFlagsCount>& in) {
    auto& f = gPlayState->actorCtx.flags;
    f.swch = in[0];
    f.tempSwch = in[1];
    f.unk0 = in[2];
    f.unk1 = in[3];
    f.chest = in[4];
    f.clear = in[5];
    f.tempClear = in[6];
    f.collect = in[7];
    f.tempCollect = in[8];
}

} // namespace

void Anchor::SendPacket_SceneFlags(s16 sceneNum) {
    if (!IsSaveLoaded() || gPlayState == nullptr) {
        return;
    }
    std::array<u32, kSceneFlagsCount> live;
    ReadFlags(live);

    nlohmann::json payload;
    payload["type"] = SCENE_FLAGS;
    payload["sceneNum"] = sceneNum;
    payload["swch"] = live[0];
    payload["tempSwch"] = live[1];
    payload["unk0"] = live[2];
    payload["unk1"] = live[3];
    payload["chest"] = live[4];
    payload["clear"] = live[5];
    payload["tempClear"] = live[6];
    payload["collect"] = live[7];
    payload["tempCollect"] = live[8];
    payload["quiet"] = true;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_SceneFlags(nlohmann::json payload) {
    if (!IsSaveLoaded() || gPlayState == nullptr) {
        return;
    }
    if (!payload.contains("sceneNum")) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }

    std::array<u32, kSceneFlagsCount> incoming{};
    incoming[0] = payload.value("swch", 0u);
    incoming[1] = payload.value("tempSwch", 0u);
    incoming[2] = payload.value("unk0", 0u);
    incoming[3] = payload.value("unk1", 0u);
    incoming[4] = payload.value("chest", 0u);
    incoming[5] = payload.value("clear", 0u);
    incoming[6] = payload.value("tempClear", 0u);
    incoming[7] = payload.value("collect", 0u);
    incoming[8] = payload.value("tempCollect", 0u);

    isApplyingRemoteSceneFlags = true;
    WriteFlags(incoming);
    isApplyingRemoteSceneFlags = false;

    // Refresh our own broadcast cache so a non-authority that later
    // becomes authority (handoff, original authority drops) doesn't
    // immediately diff this just-received state and re-broadcast it.
    lastBroadcastSceneFlags[sceneNum] = incoming;
}

void Anchor::SceneFlagsSnapshot_Tick() {
    if (!isConnected || !IsSaveLoaded() || gPlayState == nullptr) {
        return;
    }
    if (isApplyingRemoteSceneFlags) {
        return;
    }
    if (!IsAuthorityForCurrentScene()) {
        return;
    }
    s16 sceneNum = gPlayState->sceneNum;
    std::array<u32, kSceneFlagsCount> live;
    ReadFlags(live);

    auto& cached = lastBroadcastSceneFlags[sceneNum];
    if (cached == live) {
        return;
    }
    cached = live;
    SendPacket_SceneFlags(sceneNum);
}
