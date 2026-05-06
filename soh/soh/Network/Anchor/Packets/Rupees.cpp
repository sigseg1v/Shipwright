#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "z64save.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

// UPDATE_RUPEES / RUPEES_SET (FEATURE_SHARED_RUPEES)
//
// The room shares one rupee total across all clients. Locally each
// client's wallet is just a window into that shared value: when the
// player picks up a rupee or pays a shopkeeper, we send the resulting
// signed delta to the server, which folds it into the room counter
// and rebroadcasts the new authoritative total to everyone.
//
// On first contact (receivedFirstRupeesSet still false), we do NOT
// overwrite the local wallet -- otherwise we'd reset a player's save-
// loaded balance to whatever the room already had and lose rupees.
// Instead we record the server total as our "last synced" baseline,
// which makes the per-frame poll in HookHandlers.cpp see (local -
// baseline) as a delta and donate the difference back to the room.

void Anchor::SendPacket_UpdateRupees(s32 delta, s32 seed) {
    nlohmann::json payload;
    payload["type"] = UPDATE_RUPEES;
    payload["delta"] = delta;
    payload["seed"] = seed;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_RupeesSet(nlohmann::json payload) {
    if (!payload.contains("total")) {
        return;
    }
    s32 total = payload["total"].get<s32>();

    if (!receivedFirstRupeesSet) {
        // First snapshot: just record where the server stands. The
        // local wallet keeps its save-loaded value; the next polling
        // tick will surface (local - total) as a delta and seed the
        // room from this client's save.
        lastSyncedRupees = total;
        receivedFirstRupeesSet = true;
        return;
    }

    if (!IsSaveLoaded()) {
        // Defer applying until the save is up so we don't write into
        // gSaveContext fields that are about to be clobbered by a
        // file load.
        lastSyncedRupees = total;
        return;
    }

    s32 currentLocal = (s32)gSaveContext.rupees + (s32)gSaveContext.rupeeAccumulator;
    if (currentLocal == total) {
        // RUPEES_SET that matches what we already have locally -- this
        // is the echo of a delta we just sent up. Touching the
        // accumulator here would zero the engine's in-progress count-up
        // and silence the rupee pickup jingle, so leave gSaveContext
        // alone and just refresh the baseline.
        lastSyncedRupees = total;
        return;
    }

    // Push the diff into the accumulator instead of overwriting rupees
    // outright. The engine drains the accumulator one rupee per frame
    // (or 10/frame for negative drains), playing NA_SE_SY_GET_RUPEE on
    // each tick -- mirroring the natural pickup feel for amounts that
    // arrived from a peer. Bumping lastSyncedRupees in lockstep keeps
    // the per-frame poll from re-broadcasting our own application.
    s32 diff = total - currentLocal;
    isApplyingRemoteRupees = true;
    s32 newAccumulator = (s32)gSaveContext.rupeeAccumulator + diff;
    if (newAccumulator > 32767) newAccumulator = 32767;
    if (newAccumulator < -32768) newAccumulator = -32768;
    gSaveContext.rupeeAccumulator = (s16)newAccumulator;
    isApplyingRemoteRupees = false;

    lastSyncedRupees = total;
}
