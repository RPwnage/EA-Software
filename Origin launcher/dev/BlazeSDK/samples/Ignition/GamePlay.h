#ifndef GAME_PLAY_H
#define GAME_PLAY_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/game/connapi.h"

#if !defined(EA_PLATFORM_NX)
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"
#endif

#include "BlazeSDK/gamemanager/gamemanagerapi.h"

#include "Messaging.h"
#include "Transmission.h"

#if defined(EA_PLATFORM_XBOXONE)
#include "Ignition/xboxone/XdkVersionDefines.h"
#endif

namespace Ignition
{

enum IngameMessageType
{
    INGAME_MESSAGE_TYPE_HIT,
    INGAME_MESSAGE_TYPE_MISS,
    INGAME_MESSAGE_TYPE_TORCHE
};

#pragma pack(1)
class IngameMessage
{
public:
    void setShooter(Blaze::BlazeId shooter);
    void setVictim(Blaze::BlazeId victim);
    void setIngameMessageType(IngameMessageType messageType);

    Blaze::BlazeId getShooter();
    Blaze::BlazeId getVictim();
    IngameMessageType getIngameMessageType();

    char* getMessagePacket();
    void  readMessagePacket(void* buffer);
    uint32_t messageSize();
private:

    uint64_t Int64toNetwrokOrder(uint64_t iValue);
    uint64_t NetworkOrderToInt64(unsigned char* iValue);

    Blaze::BlazeId mShooter;
    Blaze::BlazeId mVictim;
    IngameMessageType mIngameMessageType;
    char mMessage[2 * sizeof(Blaze::BlazeId) + sizeof(uint64_t)];
};
#pragma pack()

class GamePlay;
class GamePlayLocalUserActionGroup :
    public LocalUserActionGroup
{

friend class GameManagement;

public:
    GamePlayLocalUserActionGroup(const char8_t *id, uint32_t userIndex, Blaze::GameManager::Game *game, GamePlay *gameWindow);
    virtual ~GamePlayLocalUserActionGroup() { }

    PYRO_ACTION(GamePlayLocalUserActionGroup, JoinGame);
    PYRO_ACTION(GamePlayLocalUserActionGroup, ChangePlayerSlotType);
    PYRO_ACTION(GamePlayLocalUserActionGroup, LeaveGame);
    PYRO_ACTION(GamePlayLocalUserActionGroup, SendMessage);
    PYRO_ACTION(GamePlayLocalUserActionGroup, SetPlayerAttribute);
    #if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
    PYRO_ACTION(GamePlayLocalUserActionGroup, SendSessionInvite);
    #endif

    virtual void onAuthenticated();
    virtual void onDeAuthenticated();

    void setGame(Blaze::GameManager::Game *game) { mGame = game; }

private:
    Blaze::GameManager::Game *mGame;

    void JoinGameCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game, const char8_t *errorMsg);
    void ChangePlayerTeamAndRoleCb(Blaze::BlazeError blazeError, Blaze::GameManager::Player *player);
    void LeaveGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
    void SetPlayerAttributeValueCb(Blaze::BlazeError blazeError, Blaze::GameManager::Player* player);

    GamePlay *mGameWindow;
};


class GamePlay :
    public Pyro::UiNodeWindow,
    protected Blaze::Idler,
    protected Blaze::GameManager::GameListener
{
    public:
        GamePlay(const char8_t *id, Blaze::GameManager::Game *game, bool bUseNetGameDist = false);
        virtual ~GamePlay();

        Blaze::GameManager::Game *getGame() { return mGame; }
        Blaze::GameManager::Game *getGameGroup() { return mGameGroup; }

        void setupConnApi();
        void setupGame(Blaze::GameManager::Player *player);
        void setupPlayer(Blaze::GameManager::Player *player);
        void gameDestroyed();

        virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);
#if !defined(EA_PLATFORM_NX)
        void voipIdle();
#endif

        bool UseNetGameDist() { return mUseNetGameDist; }

        GamePlayLocalUserActionGroup &getLocalUserActionGroup(uint32_t userIndex);

    protected:
        virtual void onGameGroupInitialized(Blaze::GameManager::Game *game);
        virtual void onPreGame(Blaze::GameManager::Game *game);
        virtual void onPostGame(Blaze::GameManager::Game *game);
        virtual void onGameStarted(Blaze::GameManager::Game *game);
        virtual void onGameEnded(Blaze::GameManager::Game *game);
        virtual void onGameReset(Blaze::GameManager::Game *game);
        virtual void onReturnDServerToPool(Blaze::GameManager::Game *game);
        virtual void onGameAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap);
        virtual void onDedicatedServerAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap);
        virtual void onGameSettingUpdated(Blaze::GameManager::Game *game);
        virtual void onGameModRegisterUpdated(Blaze::GameManager::Game *game);
        virtual void onGameEntryCriteriaUpdated(Blaze::GameManager::Game *game);
        virtual void onPlayerCapacityUpdated(Blaze::GameManager::Game *game);
        virtual void onPlayerJoining(Blaze::GameManager::Player *player);
        virtual void onPlayerJoiningQueue(Blaze::GameManager::Player *player);
        virtual void onPlayerJoinComplete(Blaze::GameManager::Player *player);
        virtual void onQueueChanged(Blaze::GameManager::Game *game);
        virtual void onPlayerRemoved(Blaze::GameManager::Game *game,
            const Blaze::GameManager::Player *removedPlayer,
            Blaze::GameManager::PlayerRemovedReason playerRemovedReason,
            Blaze::GameManager::PlayerRemovedTitleContext titleContext);
        virtual void onAdminListChanged(Blaze::GameManager::Game *game,
            const Blaze::GameManager::Player *adminPlayer,
            Blaze::GameManager::UpdateAdminListOperation  operation,
            Blaze::GameManager::PlayerId updaterId);
        virtual void onPlayerAttributeUpdated(Blaze::GameManager::Player *player, const Blaze::Collections::AttributeMap *changedAttributeMap);
        virtual void onPlayerTeamUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::TeamIndex teamIndex);
        virtual void onGameTeamIdUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::TeamId oldTeamId, Blaze::GameManager::TeamId newTeamId);
        virtual void onPlayerRoleUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::RoleName playerRole);
        virtual void onPlayerSlotUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::SlotType slotType);
        virtual void onPlayerCustomDataUpdated(Blaze::GameManager::Player *player);
        virtual void onHostMigrationStarted(Blaze::GameManager::Game *game);
        virtual void onHostMigrationEnded(Blaze::GameManager::Game *game);
        virtual void onNetworkCreated(Blaze::GameManager::Game *game);

        /* TO BE DEPRECATED - will never be notified if you don't use GameManagerAPI::receivedMessageFromEndpoint() */
        virtual void onMessageReceived(Blaze::GameManager::Game *game, const void *buf, size_t bufSize, Blaze::BlazeId senderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags);

        virtual void onPlayerJoinedFromQueue(Blaze::GameManager::Player *player);
        virtual void onPlayerJoinedFromReservation(Blaze::GameManager::Player *player);
        virtual void onPlayerStateChanged(Blaze::GameManager::Player *player, Blaze::GameManager::PlayerState playerState);
        virtual void onProcessGameQueue(Blaze::GameManager::Game *game);
        virtual void onGameNameChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::GameName newName);
        virtual void onGameRecreateRequested(Blaze::GameManager::Game *game);
        virtual void onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState);
        virtual void onBackFromUnresponsive(Blaze::GameManager::Game *game);
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        virtual void onNpSessionIdAvailable(Blaze::GameManager::Game *game, const char8_t* npSessionId);
#endif
#if defined(EA_PLATFORM_XBOXONE)|| defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        virtual void onActivePresenceUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::PresenceActivityStatus status, uint32_t userIndex);
#endif
    private:
        Blaze::GameManager::Game *mGame;
        Blaze::GameManager::Game *mGameGroup;
        Blaze::BlazeId mActivePlayerId;

        uint32_t mLastStatsUpdate;

        int mIdlerRefCount;
        bool mUseNetGameDist;
        NetGameDistPrivateData mPrivateGameData;
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        // holds image the local user has posted. On PS4 max 160 kibibytes
        uint8_t mExternalSessionImgBuf[163840];
        size_t mExternalSessionImgSize;
#endif
        eastl::string mPresenceStatus;

        void registerIdler();
        void unregisterIdler();

        void refreshProperties();
        void refreshPlayers();
        void refreshConnections();
        void updateConnectionStats(Pyro::UiNodeValueContainer &perConnectionStatsGroupNode, NetGameLinkStatT * pNetGameLinkStats);

        void PlayerActionsShowing(Pyro::UiNodeTable *sender, Pyro::UiNodeTableRow *row);
        
        PYRO_ACTION(GamePlay, SetActivePlayer);
        PYRO_ACTION(GamePlay, HitPlayer);
        PYRO_ACTION(GamePlay, MissPlayer);
        PYRO_ACTION(GamePlay, TorchePlayer);
        PYRO_ACTION(GamePlay, KickPlayer);
        PYRO_ACTION(GamePlay, KickPlayerPoorConnection);
        PYRO_ACTION(GamePlay, KickPlayerUnresponsive);
        PYRO_ACTION(GamePlay, KickPlayerWithBan);
        PYRO_ACTION(GamePlay, KickPlayerPoorConnectionWithBan);
        PYRO_ACTION(GamePlay, KickPlayerUnresponsiveWithBan);
        PYRO_ACTION(GamePlay, BanPlayer);
        PYRO_ACTION(GamePlay, AddPlayerToAdminList);
        PYRO_ACTION(GamePlay, RemovePlayerFromAdminList);
#if !defined(EA_PLATFORM_NX)
        PYRO_ACTION(GamePlay, MuteThisPlayer);
        PYRO_ACTION(GamePlay, UnmuteThisPlayer);
#endif
        PYRO_ACTION(GamePlay, ChangeTeamId);
        PYRO_ACTION(GamePlay, ChangePlayerTeamIndex);
        PYRO_ACTION(GamePlay, ChangePlayerTeamIndexToUnspecified);
        PYRO_ACTION(GamePlay, ChangePlayerRole);
        PYRO_ACTION(GamePlay, BecomeSpectator);
        PYRO_ACTION(GamePlay, PromoteFromQueue);
        PYRO_ACTION(GamePlay, DemoteToQueue);

        PYRO_ACTION(GamePlay, LeaveGame);
        PYRO_ACTION(GamePlay, LeaveGameGroup);
        PYRO_ACTION(GamePlay, LeaveGameLUGroup);
        PYRO_ACTION(GamePlay, StartGame);
        PYRO_ACTION(GamePlay, EndGame);
        PYRO_ACTION(GamePlay, ReplayGame);
        PYRO_ACTION(GamePlay, UpdateGameName);
        PYRO_ACTION(GamePlay, SetPlayerCapacity);
        PYRO_ACTION(GamePlay, SwapPlayers);
        PYRO_ACTION(GamePlay, DirectReset);
        PYRO_ACTION(GamePlay, ReturnDSToPool);
        PYRO_ACTION(GamePlay, EjectHost);
        PYRO_ACTION(GamePlay, DestroyGame);
        PYRO_ACTION(GamePlay, SetGameMods);
        PYRO_ACTION(GamePlay, SetGameSettings);
        PYRO_ACTION(GamePlay, SetGameEntryCriteria);
        PYRO_ACTION(GamePlay, ClearGameEntryCriteria);

        PYRO_ACTION(GamePlay, GameGroupCreateGame);
        PYRO_ACTION(GamePlay, GameGroupCreateGamePJD);
        PYRO_ACTION(GamePlay, GameGroupJoinGame);
        PYRO_ACTION(GamePlay, GameGroupJoinGamePJD);
        void actionGameGroupScenarioMatchmake(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);
        void actionGameGroupCreateGameTemplate(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);
        PYRO_ACTION(GamePlay, GameGroupCreateDedicatedServerGame);
        PYRO_ACTION(GamePlay, GameGroupLeaveGame);

        PYRO_ACTION(GamePlay, SetGameLockedAsBusy);
        PYRO_ACTION(GamePlay, SetGameAttribute);
        PYRO_ACTION(GamePlay, SetDedicatedServerAttribute);
        PYRO_ACTION(GamePlay, SetAllowAnyReputation);
        PYRO_ACTION(GamePlay, SetDynamicReputationRequirement);
        PYRO_ACTION(GamePlay, SetOpenToJoinByPlayer);
        PYRO_ACTION(GamePlay, SetOpenToJoinByInvite);
        PYRO_ACTION(GamePlay, SetFriendsBypassClosedToJoinByPlayer);
#if defined(EA_PLATFORM_XBOXONE)|| defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
        PYRO_ACTION(GamePlay, SendSessionInvite);
#endif
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
        PYRO_ACTION(GamePlay, SubmitTournamentMatchResult);
#endif
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        PYRO_ACTION(GamePlay, UpdateExternalSession);
#endif

        void ChangeGameStateCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void UpdateGameNameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void QueuePlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player);
        void KickPlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player);
        void BanPlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, Blaze::BlazeId blazeId);
        void LeaveGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void UpdateAdminListCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, Blaze::GameManager::Player *player);
        void ChangeGameTeamIdCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void ChangePlayerTeamAndRoleCb(Blaze::BlazeError blazeError, Blaze::GameManager::Player *player);
        void SubmitGameReportCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void GameMessageCb(const Blaze::Messaging::ServerMessage *serverMessage, Blaze::GameManager::Game *game);
        void SetPlayerCapacityCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::SwapPlayersErrorInfo *errorInfo);
        void DestroyGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void AddQueuedPlayerCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player);
        void EjectHostCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void ChangeGameModRegisterCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void ChangeGameEntryCriteriaCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void ChangeGameSettingsCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void SetGameAttributeValueCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game* game, Blaze::JobId jobId);
        void SetDedicatedServerAttributeValueCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game* game, Blaze::JobId jobId);
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        void UpdateExternalSessionImageCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game, const Blaze::ExternalSessionErrorInfo *errorInfo);
        void UpdateExternalSessionStatusCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void parseExternalSessionUpdateParams(Pyro::UiNodeParameterStruct &parameters, Blaze::Ps4NpSessionImage& imageBlob, Blaze::ExternalSessionStatus& status);
#endif
#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4) && defined(EA_PLATFORM_PS4_CROSSGEN))
        static void sendPlayerSessionInviteCb(int32_t iSceResult, int32_t iUserIndex, int32_t iStatusCode, const char *pRespBody, int32_t iRespBodyLength, void *pUserData);
#endif
        void pumpGameQueue(Blaze::GameManager::Game *game);
        void endAndReturnDedicatedServerToPool(Blaze::GameManager::Game *game);

        void ReceiveHandlerCb(Blaze::GameManager::Game *game, char8_t *buffer, int bufferSize);

#if !defined(EA_PLATFORM_NX)
        int16_t getVoipConnIndexForPlayer(Blaze::GameManager::Player *player);
#endif
        uint8_t getConnApiIndexForPlayer(Blaze::GameManager::Game *game, Blaze::GameManager::Player *player);

        static void ConnApiCb(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData);
#if !defined(EA_PLATFORM_NX)
        static void gameVoipCallback(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData);
#endif
        static int32_t gameTranscribedTextCallback(int32_t iConnId, int32_t iUserIndex, const char *pStrUtf8, void *pUserData);

        void buildPlayerList(eastl::string &playerIds);
        static void setupSwapPlayersParameters(Pyro::UiNodeParameterStruct &parameters, const char* tabName = "", const char* playerIds = "");

        typedef struct InternalPlayerData
        {
            InternalPlayerData() {memset(this, 0, sizeof(*this));}
            int mHits;
            int mMisses;
            int mTorchings;
            bool mVoipAvailable; // Voip Information sent from Blaze
            bool mHeadset;
            bool mVoipAvailableDS; // Voip Information sent from DS
            bool mTalking;
            bool mHeadsetDS;
        } InternalPlayerData;

        /*typedef struct InteralPlayerVoipData
        {
           
        } InteralPlayerVoipData;

        eastl::map<Blaze::BlazeId, InteralPlayerVoipData> mPlayerVoipDataMap;*/

        void attributeMapToLogStr(const Blaze::Collections::AttributeMap *changedAttributeMap, eastl::string &logStr);
#if defined(EA_PLATFORM_PS4)
        const eastl::string& sonyIdToStr(const Blaze::GameManager::Player& player, eastl::string& logStr) const;
#endif
};

}

#endif
