#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include <cstdio>

#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Kanban/z_en_kanban.h"
extern "C" {
extern PlayState* gPlayState;
}

// SIGN_CUT (FEATURE_WORLD_EVENT_SYNC)
//
// One-shot relay: when a local sword swing chops an EN_KANBAN signpost,
// the originating client broadcasts (sceneNum, signId, cutType). Peers
// in the matching scene find the same signpost by id and apply the same
// part-mask reduction the engine would have applied locally, plus the
// cut-mark visual. They do NOT spawn a flying piece child actor (the
// piece is purely cosmetic and would desync without full actor sync).
//
// signId: same scheme as foliage/rocks. Pieces (params == ENKANBAN_PIECE)
// are excluded -- only the persistent signpost is synced.
//
// Identical to the engine cut-flag tables in z_en_kanban.c. Duplicated
// here so the receive path can mask partFlags without pulling the static
// arrays out of the actor overlay.
static const u16 KANBAN_PART_UPPER_LEFT = (1 << 0);
static const u16 KANBAN_PART_LEFT_UPPER = (1 << 1);
static const u16 KANBAN_PART_LEFT_LOWER = (1 << 2);
static const u16 KANBAN_PART_RIGHT_UPPER = (1 << 3);
static const u16 KANBAN_PART_RIGHT_LOWER = (1 << 4);
static const u16 KANBAN_PART_LOWER_LEFT = (1 << 5);
static const u16 KANBAN_PART_UPPER_RIGHT = (1 << 6);
static const u16 KANBAN_PART_LOWER_RIGHT = (1 << 7);
static const u16 KANBAN_PART_POST_UPPER = (1 << 8);
static const u16 KANBAN_PART_POST_LOWER = (1 << 9);
static const u16 KANBAN_LEFT_HALF =
    (KANBAN_PART_UPPER_LEFT | KANBAN_PART_LEFT_UPPER | KANBAN_PART_LEFT_LOWER | KANBAN_PART_LOWER_LEFT);
static const u16 KANBAN_RIGHT_HALF =
    (KANBAN_PART_UPPER_RIGHT | KANBAN_PART_RIGHT_UPPER | KANBAN_PART_RIGHT_LOWER | KANBAN_PART_LOWER_RIGHT);
static const u16 KANBAN_UPPER_HALF = (KANBAN_PART_POST_UPPER | KANBAN_PART_UPPER_RIGHT | KANBAN_PART_RIGHT_UPPER |
                                      KANBAN_PART_UPPER_LEFT | KANBAN_PART_LEFT_UPPER);
static const u16 KANBAN_UPPERLEFT_HALF = (KANBAN_PART_POST_UPPER | KANBAN_PART_UPPER_RIGHT | KANBAN_PART_LEFT_LOWER |
                                          KANBAN_PART_UPPER_LEFT | KANBAN_PART_LEFT_UPPER);
static const u16 KANBAN_UPPERRIGHT_HALF = (KANBAN_PART_POST_UPPER | KANBAN_PART_UPPER_RIGHT | KANBAN_PART_RIGHT_UPPER |
                                           KANBAN_PART_UPPER_LEFT | KANBAN_PART_RIGHT_LOWER);
static const u16 KANBAN_ALL_PARTS = (KANBAN_LEFT_HALF | KANBAN_RIGHT_HALF | KANBAN_PART_POST_UPPER | KANBAN_PART_POST_LOWER);

static const u16 KANBAN_CUT_FLAGS[] = {
    /* CUT_POST   */ KANBAN_ALL_PARTS,       /* CUT_VERT_L */ KANBAN_LEFT_HALF,
    /* CUT_HORIZ  */ KANBAN_UPPER_HALF,      /* CUT_DIAG_L */ KANBAN_UPPERLEFT_HALF,
    /* CUT_DIAG_R */ KANBAN_UPPERRIGHT_HALF, /* CUT_VERT_R */ KANBAN_RIGHT_HALF,
};

std::string Anchor::MakeKanbanId(const Actor* actor) {
    char buf[80];
    std::snprintf(buf, sizeof(buf), "%d:%d:%.1f:%.1f:%.1f", (int)actor->id, (int)actor->params,
                  actor->home.pos.x, actor->home.pos.y, actor->home.pos.z);
    return buf;
}

void Anchor::SendPacket_SignCut(s16 sceneNum, const std::string& signId, u8 cutType) {
    nlohmann::json payload;
    payload["type"] = SIGN_CUT;
    payload["sceneNum"] = sceneNum;
    payload["signId"] = signId;
    payload["cutType"] = (int)cutType;
    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_SignCut(nlohmann::json payload) {
    if (!payload.contains("sceneNum") || !payload.contains("signId") || !payload.contains("cutType")) {
        return;
    }
    if (!IsSaveLoaded() || gPlayState == nullptr) {
        return;
    }
    s16 sceneNum = payload["sceneNum"].get<s16>();
    if (sceneNum != gPlayState->sceneNum) {
        return;
    }
    std::string signId = payload["signId"].get<std::string>();
    u8 cutType = (u8)payload["cutType"].get<int>();
    if (cutType >= ARRAY_COUNT(KANBAN_CUT_FLAGS)) {
        return;
    }

    // Find the matching live signpost (skip pieces) and apply the same
    // mask the engine would have applied locally. We update lastKanbanPartFlags
    // immediately so the subsequent OnActorUpdate diff detector doesn't
    // see this drop as a fresh local cut and echo SIGN_CUT back.
    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].head;
    while (actor != NULL) {
        Actor* next = actor->next;
        if (actor->id == ACTOR_EN_KANBAN && actor->params != ENKANBAN_PIECE && actor->update != NULL) {
            if (MakeKanbanId(actor) == signId) {
                EnKanban* kanban = (EnKanban*)actor;
                kanban->partFlags &= ~KANBAN_CUT_FLAGS[cutType];
                kanban->cutType = cutType;
                kanban->cutMarkTimer = 5;
                Audio_PlayActorSound2(actor, NA_SE_IT_SWORD_STRIKE);
                lastKanbanPartFlags[actor] = kanban->partFlags;
                break;
            }
        }
        actor = next;
    }
}
