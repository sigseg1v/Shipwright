#ifndef NETWORK_ANCHOR_H
#define NETWORK_ANCHOR_H
#ifdef __cplusplus

#include "soh/Network/Network.h"
#include <libultraship/libultraship.h>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <vector>

extern "C" {
#include "variables.h"
#include "z64.h"
}

void DummyPlayer_Init(Actor* actor, PlayState* play);
void DummyPlayer_Update(Actor* actor, PlayState* play);
void DummyPlayer_Draw(Actor* actor, PlayState* play);
void DummyPlayer_Destroy(Actor* actor, PlayState* play);

typedef struct {
    uint32_t clientId;
    std::string name;
    Color_RGB8 color;
    std::string clientVersion;
    std::string teamId;
    bool online;
    bool self;
    uint32_t seed;
    bool isSaveLoaded;
    bool isGameComplete;
    s16 sceneNum;
    s8 curRoomNum;
    s32 entranceIndex;
    std::vector<std::string> features;

    // Only available in PLAYER_UPDATE packets
    s32 linkAge;
    PosRot posRot;
    Vec3s jointTable[24];
    u8 movementFlags;
    Vec3s prevTransl;
    Vec3s upperLimbRot;
    s8 currentBoots;
    s8 currentShield;
    s8 currentTunic;
    u32 stateFlags1;
    u32 stateFlags2;
    u8 buttonItem0;
    s8 itemAction;
    s8 heldItemAction;
    u8 modelGroup;
    s8 invincibilityTimer;
    f32 unk_85C;
    s16 unk_862;
    s8 actionVar1;
    u8 ocarinaNote;
    f32 ocarinaModulator;
    s8 ocarinaBend;

    // Held-rock state for the overhead visual rendered on the dummy
    // player. heldRockType == -1 means "not holding anything"; 0/1 are
    // ROCK_SMALL / ROCK_LARGE. heldRockId is kept so an incoming
    // ROCK_DESTROY for a specific rock can clear the matching client's
    // held-rock overhead without disturbing any other holder.
    s16 heldRockType = -1;
    std::string heldRockId;

    // Tracks whether we've already started carryB_wait on the dummy's
    // upperSkelAnime so DummyPlayer_Update only re-issues PlayLoop on
    // entry (otherwise the anim would reset to frame 0 every tick).
    bool dummyCarryAnimActive = false;

    // Ptr to the dummy player
    Player* player;
} AnchorClient;

typedef struct {
    uint32_t ownerClientId;
    u8 pvpMode;           // 0 = off, 1 = on, 2 = on with friendly fire
    u8 showLocationsMode; // 0 = none, 1 = team, 2 = all
    u8 teleportMode;      // 0 = off, 1 = team, 2 = all
    u8 syncItemsAndFlags; // 0 = off, 1 = on
} RoomState;

class Anchor : public Network {
  private:
    uint32_t spawningDummyPlayerForClientId = 0;
    bool shouldRefreshActors = false;
    bool justLoadedSave = false;
    bool isHandlingUpdateTeamState = false;
    bool isProcessingIncomingPacket = false;
    std::queue<nlohmann::json> incomingPacketQueue;
    std::mutex incomingPacketQueueMutex;
    std::queue<nlohmann::json> outgoingPacketQueue;
    std::mutex outgoingPacketQueueMutex;

    nlohmann::json PrepClientState();
    nlohmann::json PrepRoomState();
    void RegisterHooks();
    void RefreshClientActors();
    void SetDummyPlayerClientId(const Actor* actor, uint32_t clientId);

    void HandlePacket_AllClientState(nlohmann::json payload);
    void HandlePacket_SceneAuthority(nlohmann::json payload);
    void HandlePacket_EnemySpawn(nlohmann::json payload);
    void HandlePacket_EnemyUpdate(nlohmann::json payload);
    void HandlePacket_EnemyDamage(nlohmann::json payload);
    void HandlePacket_EnemyDeath(nlohmann::json payload);
    void HandlePacket_EnemyFullSnapshot(nlohmann::json payload);
    void HandlePacket_ConsumeAdultTradeItem(nlohmann::json payload);
    void HandlePacket_DamagePlayer(nlohmann::json payload);
    void HandlePacket_DisableAnchor(nlohmann::json payload);
    void HandlePacket_EntranceDiscovered(nlohmann::json payload);
    void HandlePacket_GameComplete(nlohmann::json payload);
    void HandlePacket_GiveItem(nlohmann::json payload);
    void HandlePacket_OcarinaSfx(nlohmann::json payload);
    void HandlePacket_PlayerSfx(nlohmann::json payload);
    void HandlePacket_PlayerUpdate(nlohmann::json payload);
    void HandlePacket_RequestTeamState(nlohmann::json payload);
    void HandlePacket_RequestTeleport(nlohmann::json payload);
    void HandlePacket_ServerMessage(nlohmann::json payload);
    void HandlePacket_SetCheckStatus(nlohmann::json payload);
    void HandlePacket_SetFlag(nlohmann::json payload);
    void HandlePacket_TeleportTo(nlohmann::json payload);
    void HandlePacket_UnsetFlag(nlohmann::json payload);
    void HandlePacket_UpdateBeansCount(nlohmann::json payload);
    void HandlePacket_UpdateClientState(nlohmann::json payload);
    void HandlePacket_UpdateDungeonItems(nlohmann::json payload);
    void HandlePacket_UpdateRoomState(nlohmann::json payload);
    void HandlePacket_UpdateTeamState(nlohmann::json payload);
    void HandlePacket_RupeesSet(nlohmann::json payload);
    void HandlePacket_FoliageDestroy(nlohmann::json payload);
    void HandlePacket_FoliageSnapshot(nlohmann::json payload);
    void HandlePacket_RockDestroy(nlohmann::json payload);
    void HandlePacket_RockSnapshot(nlohmann::json payload);
    void HandlePacket_RockLift(nlohmann::json payload);
    void HandlePacket_ItemSpawn(nlohmann::json payload);
    void HandlePacket_ItemCollect(nlohmann::json payload);

  public:
    uint32_t ownClientId;
    inline static const std::string clientVersion = (char*)gGitCommitHash;

    // Feature flags this build advertises in HANDSHAKE / UPDATE_CLIENT_STATE.
    // Peers gracefully ignore feature-gated packets from clients that don't
    // advertise the matching feature, allowing forward-compatible additions
    // (e.g., enemy_sync) to coexist with vanilla SoH on the public Anchor server.
    inline static const std::string FEATURE_ENEMY_SYNC = "enemy_sync_v1";
    inline static const std::string FEATURE_SHARED_RUPEES = "shared_rupees_v1";
    inline static const std::string FEATURE_FOLIAGE_SYNC = "foliage_sync_v1";
    inline static const std::string FEATURE_ROCK_SYNC = "rock_sync_v1";
    inline static const std::vector<std::string> selfFeatures = {
        FEATURE_ENEMY_SYNC, FEATURE_SHARED_RUPEES, FEATURE_FOLIAGE_SYNC, FEATURE_ROCK_SYNC,
    };
    bool ClientHasFeature(uint32_t clientId, const std::string& feature);

    // Packet types //
    inline static const std::string ALL_CLIENT_STATE = "ALL_CLIENT_STATE";
    inline static const std::string DAMAGE_PLAYER = "DAMAGE_PLAYER";
    inline static const std::string DISABLE_ANCHOR = "DISABLE_ANCHOR";
    inline static const std::string ENTRANCE_DISCOVERED = "ENTRANCE_DISCOVERED";
    inline static const std::string GAME_COMPLETE = "GAME_COMPLETE";
    inline static const std::string GIVE_ITEM = "GIVE_ITEM";
    inline static const std::string HANDSHAKE = "HANDSHAKE";
    inline static const std::string OCARINA_SFX = "OCARINA_SFX";
    inline static const std::string PLAYER_SFX = "PLAYER_SFX";
    inline static const std::string PLAYER_UPDATE = "PLAYER_UPDATE";
    inline static const std::string REQUEST_TEAM_STATE = "REQUEST_TEAM_STATE";
    inline static const std::string REQUEST_TELEPORT = "REQUEST_TELEPORT";
    inline static const std::string SERVER_MESSAGE = "SERVER_MESSAGE";
    inline static const std::string SET_CHECK_STATUS = "SET_CHECK_STATUS";
    inline static const std::string SET_FLAG = "SET_FLAG";
    inline static const std::string TELEPORT_TO = "TELEPORT_TO";
    inline static const std::string UNSET_FLAG = "UNSET_FLAG";
    inline static const std::string UPDATE_BEANS_COUNT = "UPDATE_BEANS_COUNT";
    inline static const std::string UPDATE_CLIENT_STATE = "UPDATE_CLIENT_STATE";
    inline static const std::string UPDATE_DUNGEON_ITEMS = "UPDATE_DUNGEON_ITEMS";
    inline static const std::string UPDATE_ROOM_STATE = "UPDATE_ROOM_STATE";
    inline static const std::string UPDATE_TEAM_STATE = "UPDATE_TEAM_STATE";

    // Enemy-sync packet types (FEATURE_ENEMY_SYNC). Version-mismatch tolerant.
    inline static const std::string ENEMY_SPAWN = "ENEMY_SPAWN";
    inline static const std::string ENEMY_UPDATE = "ENEMY_UPDATE";
    inline static const std::string ENEMY_DAMAGE = "ENEMY_DAMAGE";
    inline static const std::string ENEMY_DEATH = "ENEMY_DEATH";
    inline static const std::string ENEMY_FULL_SNAPSHOT = "ENEMY_FULL_SNAPSHOT";

    // Server-driven authority assignment. The anchor server tracks which
    // client owns each scene (first-to-enter, persisted across re-entries
    // until the owner leaves or disconnects) and announces the result
    // here. authorityClientId == 0 means "no authority"; older servers
    // simply never send this packet, in which case GetSceneAuthorityClientId
    // returns 0 and clients fall back to their own behavior.
    inline static const std::string SCENE_AUTHORITY = "SCENE_AUTHORITY";

    // Shared-rupees packet types (FEATURE_SHARED_RUPEES). The client
    // sends UPDATE_RUPEES with a signed delta whenever its local
    // wallet changes; the server applies the delta to a single
    // per-room counter and broadcasts RUPEES_SET back with the new
    // total. Older anchor servers never emit RUPEES_SET, in which
    // case the local wallet just behaves the way it always has.
    inline static const std::string UPDATE_RUPEES = "UPDATE_RUPEES";
    inline static const std::string RUPEES_SET = "RUPEES_SET";

    // Foliage-sync packet types (FEATURE_FOLIAGE_SYNC). FOLIAGE_DESTROY
    // is sent any time a client cuts a foliage actor (currently just
    // ACTOR_EN_KUSA -- grass clumps and bushes); the server records
    // the cut in a per-scene set and rebroadcasts. FOLIAGE_SNAPSHOT
    // is sent by the server when a client transitions into a scene
    // so newly-arriving peers can hide grass that was already cut
    // before they got there.
    inline static const std::string FOLIAGE_DESTROY = "FOLIAGE_DESTROY";
    inline static const std::string FOLIAGE_SNAPSHOT = "FOLIAGE_SNAPSHOT";

    // Rock-sync packet types (FEATURE_ROCK_SYNC). Same shape as the
    // foliage pair but for liftable/breakable rock actors (currently
    // ACTOR_EN_ISHI -- small grey rocks that drop a collectible when
    // smashed, plus large silver boulders). ROCK_DESTROY is sent on
    // local Actor_Kill of a synced rock; the server records and
    // rebroadcasts. ROCK_SNAPSHOT is sent to clients on scene entry.
    inline static const std::string ROCK_DESTROY = "ROCK_DESTROY";
    inline static const std::string ROCK_SNAPSHOT = "ROCK_SNAPSHOT";
    // Sent when a player picks up a rock locally. Peers hide the world
    // rock immediately and start rendering an overhead held-rock visual
    // on the lifting player's dummy. The eventual ROCK_DESTROY clears
    // the visual once the rock is thrown and breaks.
    inline static const std::string ROCK_LIFT = "ROCK_LIFT";
    // Replicates a collectible drop (currently rock-smash drops only).
    // Sent by the smashing client with the actual rolled item params so
    // every peer spawns the same drop. Relayed by the server with no
    // dedup; peers' EnItem00 instances despawn naturally on their own
    // timers and rupee count is reconciled via FEATURE_SHARED_RUPEES.
    inline static const std::string ITEM_SPAWN = "ITEM_SPAWN";
    // Sent on local Actor_Kill of any synced EnItem00. Peers find the
    // matching local actor by itemId and Actor_Kill it so the rupee /
    // heart / etc. disappears in lockstep with the player who picked it
    // up. Stateless server relay; no snapshot since spawned items are
    // ephemeral (despawn naturally on their own ~13s timer).
    inline static const std::string ITEM_COLLECT = "ITEM_COLLECT";

    static Anchor* Instance;
    std::map<uint32_t, AnchorClient> clients;
    // Server-authoritative per-scene authority. Populated/updated by
    // HandlePacket_SceneAuthority. Read by IsAuthorityForCurrentScene
    // and GetSceneAuthorityClientId; never written from gameplay code.
    std::map<s16, uint32_t> sceneAuthorities;

    // Shared-rupees state. lastSyncedRupees tracks the (rupees +
    // accumulator) value we've already reported as a delta to the
    // server; the per-frame poll in HookHandlers sends a delta when
    // local total drifts. isApplyingRemoteRupees suppresses recursion
    // when we apply a server RUPEES_SET. receivedFirstRupeesSet gates
    // the per-frame poll so we don't fire a bogus delta against the
    // initial 0 baseline before we've heard anything from the server;
    // the explicit ping in OnConnected/OnLoadGame is what actually
    // seeds an empty room from our wallet.
    s32 lastSyncedRupees = 0;
    bool receivedFirstRupeesSet = false;
    bool isApplyingRemoteRupees = false;

    // Per-scene set of destroyed-foliage IDs (string-encoded
    // "actorId:params:x:y:z"). Populated by FOLIAGE_SNAPSHOT and
    // FOLIAGE_DESTROY packets from the server. Read on actor init to
    // immediately kill foliage that was cut before we got here.
    std::map<s16, std::set<std::string>> destroyedFoliage;
    // Same encoding/lifecycle as destroyedFoliage but for rocks.
    std::map<s16, std::set<std::string>> destroyedRocks;

    // Per-scene set of rockIds for which we (this client) have already
    // broadcast a ROCK_LIFT in the current scene visit. Prevents
    // re-broadcasting on every frame while the rock is held. Cleared on
    // scene transition. Note: this is *not* the same as destroyedRocks --
    // we deliberately do NOT add to destroyedRocks when broadcasting LIFT
    // so the eventual local Actor_Kill (smash on impact) still fires
    // ROCK_DESTROY for peers to clear the held-rock visual.
    std::set<std::string> liftedRocksBroadcast;
    // Per-scene set of EnItem00 actor pointers we've already broadcast as
    // ITEM_SPAWN, to dedup re-scans. Pointers are stable for the actor's
    // lifetime; cleared on scene transition.
    std::set<Actor*> rockItemDropsBroadcast;

    // Bidirectional mapping for synced EnItem00 collection. Both sender
    // and receiver bind their local EnItem00 actor pointer to the same
    // itemId so either can broadcast ITEM_COLLECT on local Actor_Kill
    // and any peer can resolve it back to their own local actor to kill.
    // Cleared on scene transition; entries also erased when the actor
    // dies. itemSpawnSeq is monotonically increasing on the originator;
    // we mix in our clientId so concurrent rolls from two clients don't
    // collide on the wire.
    std::map<Actor*, uint64_t> itemActorToId;
    std::map<uint64_t, Actor*> itemIdToActor;
    uint64_t itemSpawnSeq = 0;
    RoomState roomState;

    void Enable();
    void Disable();
    void OnIncomingJson(nlohmann::json payload);
    void OnConnected();
    void OnDisconnected();
    void ProcessOutgoingPackets();
    void DrawMenu();
    void ProcessIncomingPacketQueue();
    void SendJsonToRemote(nlohmann::json packet);
    bool IsSaveLoaded();
    bool CanTeleportTo(uint32_t clientId);
    uint32_t GetDummyPlayerClientId(const Actor* actor);

    void SendPacket_ClearTeamState(std::string teamId);
    void SendPacket_DamagePlayer(u32 clientId, u8 damageEffect, u8 damage);
    void SendPacket_EntranceDiscovered(u16 entranceIndex);
    void SendPacket_GameComplete();
    void SendPacket_GiveItem(u16 modId, s16 getItemId);
    void SendPacket_Handshake();
    void SendPacket_OcarinaSfx(uint8_t note, float modulator, int8_t bend);
    void SendPacket_PlayerSfx(u16 sfxId);
    void SendPacket_PlayerUpdate();
    void SendPacket_RequestTeamState();
    void SendPacket_RequestTeleport(u32 clientId);
    void SendPacket_SetCheckStatus(RandomizerCheck rc);
    void SendPacket_SetFlag(s16 sceneNum, s16 flagType, s16 flag);
    void SendPacket_TeleportTo(u32 clientId);
    void SendPacket_UnsetFlag(s16 sceneNum, s16 flagType, s16 flag);
    void SendPacket_UpdateBeansCount();
    void SendPacket_UpdateClientState();
    void SendPacket_UpdateDungeonItems();
    void SendPacket_UpdateRoomState();
    void SendPacket_UpdateTeamState();
    void SendPacket_UpdateRupees(s32 delta, s32 seed);
    void SendPacket_FoliageDestroy(s16 sceneNum, const std::string& foliageId);
    std::string MakeFoliageId(const Actor* actor);
    void SendPacket_RockDestroy(s16 sceneNum, const std::string& rockId);
    void SendPacket_RockLift(s16 sceneNum, const std::string& rockId, s16 rockType);
    void SendPacket_ItemSpawn(s16 sceneNum, uint64_t itemId, f32 x, f32 y, f32 z, s16 params);
    void SendPacket_ItemCollect(uint64_t itemId);
    std::string MakeRockId(const Actor* actor);

    // Enemy sync helpers (Phase 2 PoC)
    bool IsAuthorityForCurrentScene();
    uint32_t GetSceneAuthorityClientId(s16 sceneNum);
    uint32_t MintEnemyNetId();
    void EnemySync_OnSceneSpawnActors();
    void EnemySync_TickAuthorityBroadcast();
    void EnemySync_HandleNonAuthorityHit(Actor* actor);
    void EnemySync_OnEnemyDefeat(Actor* actor);
    void EnemySync_OnActorDestroy(Actor* actor);
    void SendPacket_EnemySpawn(Actor* actor, uint32_t enemyNetId);
    void SendPacket_EnemyDamage(uint32_t enemyNetId, uint32_t targetClientId, u8 damage, u8 damageEffect);
    void SendPacket_EnemyDeath(uint32_t enemyNetId);
    void SendPacket_EnemyFullSnapshot(uint32_t targetClientId);

  private:
    uint32_t enemyNetIdCounter = 0;
    uint32_t enemySyncTickCounter = 0;
    // network-id -> Actor* (only valid while actor is alive in current scene)
    std::map<uint32_t, Actor*> enemyNetIdToActor;
};

typedef enum {
    // Starting at 5 to continue from the last value in the PlayerDamageResponseType enum
    DUMMY_PLAYER_HIT_RESPONSE_STUN = 5,
    DUMMY_PLAYER_HIT_RESPONSE_FIRE,
    DUMMY_PLAYER_HIT_RESPONSE_NORMAL,
} DummyPlayerDamageResponseType;

class AnchorRoomWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override;
    void Draw() override;
    void UpdateElement() override{};
};

#endif // __cplusplus
#endif // NETWORK_ANCHOR_H
