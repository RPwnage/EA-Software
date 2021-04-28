
#ifndef GAMEMANAGEMENT_H
#define GAMEMANAGEMENT_H

#include "Ignition/Ignition.h"

#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/matchmakingsession.h"
#include "BlazeSDK/component/gamereportingcomponent.h"

#if defined(EA_PLATFORM_XBOXONE)
#include <wrl.h>                            // for ComPtr
#include <robuffer.h>                       // for IBufferByteAccess
#include "Ignition/xboxone/ProtocolActivation.h"
#include "Ignition/xboxone/XdkVersionDefines.h"
#endif

#include "BlazeSDK/connectionmanager/connectionmanager.h" // for ConnectionManagerListener

#include "EASTL/map.h"

namespace Ignition
{

INTRUSIVE_PTR(GamePlay);
class MatchmakingManagement;

class GameManagement :
    public BlazeHubUiBuilder,
    public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener,
    protected Blaze::GameManager::GameManagerAPIListener,
    protected Blaze::ConnectionManager::ConnectionManagerListener // for onQosPingSiteLatencyRetrieved
{
    public:
        GameManagement();
        virtual ~GameManagement();

        const Blaze::GameManager::GetMatchmakingConfigResponse *getMatchmakingConfig();

        void matchmakingSessionStarted(Blaze::GameManager::MatchmakingSessionId matchmakingSessionId);
        void matchmakingScenarioStarted(Blaze::GameManager::MatchmakingScenarioId matchmakingScenarioId);


        void runCreateNewGame(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup);
        void runCreateNewGamePJD(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup);
        void runCreateDedicatedServerGame(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup);
        void runJoinGameById(Blaze::GameManager::GameId gameId, const Blaze::UserGroup* userGroup, 
            Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::GameEntryType gameEntryType, Blaze::GameManager::RoleNameToPlayerMap *joiningRoles, Blaze::GameManager::SlotType slotType = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT,
            const Blaze::UserIdentificationList* reservedExternalPlayerIdentities = nullptr, const Blaze::GameManager::RoleNameToUserIdentificationMap* externalPlayerIdentityJoiningRoles = nullptr, Blaze::GameManager::JoinMethod joinMethod = Blaze::GameManager::JOIN_BY_BROWSING);
        void runJoinGameByIdPJD(Blaze::GameManager::GameId gameId, const Blaze::UserGroup* userGroup, Blaze::GameManager::PlayerJoinDataHelper& pjd,
                                Blaze::GameManager::JoinMethod joinMethod = Blaze::GameManager::JOIN_BY_BROWSING);
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
        void runJoinGameByIdWithInvite(Blaze::GameManager::GameId gameId, const Blaze::UserGroup* userGroup, 
            Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::GameEntryType gameEntryType, Blaze::GameManager::RoleNameToPlayerMap *joiningRoles, Blaze::GameManager::SlotType slotType = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, const Blaze::Collections::AttributeMap* playerAttribs = nullptr,
            const Blaze::UserIdentificationList* reservedExternalIds = nullptr, const Blaze::GameManager::RoleNameToUserIdentificationMap* userIdentificationJoiningRoles = nullptr);
#endif
#if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN)
        void runCreateOrJoinGameByPlayerSessionId(const char8_t* playerSessionId, const char8_t* activityId);
#endif

        void runJoinGameByUserName(const char8_t *userName, const Blaze::UserGroup* userGroup, 
            Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::GameEntryType gameEntryType, Blaze::GameManager::RoleNameToPlayerMap *joiningRoles, Blaze::GameManager::SlotType slotType = Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT,
            const Blaze::UserIdentificationList* reservedPlayerIdentifications = nullptr, const Blaze::GameManager::RoleNameToUserIdentificationMap* userIdentificationJoiningRoles = nullptr, Blaze::GameManager::GameType gameType = Blaze::GameManager::GAME_TYPE_GAMESESSION);
        void runJoinGameByUserList(Blaze::GameManager::GameId gameId, Blaze::GameManager::TeamIndex teamIndex, const Blaze::UserIdentificationList &reservedPlayerIdentifications, const Blaze::GameManager::RoleNameToUserIdentificationMap *externalPlayerIdentificationRoles, Blaze::GameManager::JoinMethod joinMethod = Blaze::GameManager::JOIN_BY_INVITES);
        void runStartMatchmakingSession(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup);
        void runScenarioMatchmake(Pyro::UiNodeParameterStructPtr parameters, const Blaze::UserGroup *userGroup);
        void runCreateOrJoinGame(Pyro::UiNodeParameterStruct &parameters, const Blaze::UserGroup *userGroup);
        void runCreateGameTemplate(Pyro::UiNodeParameterStructPtr parameters, const Blaze::UserGroup *userGroup);

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

        static void addScenarioMatchmakeActions(Pyro::UiNodeActionContainer& outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction);
        static void addGameBrowserScenarioActions(Pyro::UiNodeActionContainer& outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction);
        static void addScenarioActions(Pyro::UiNodeActionContainer& outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction);
        static void addCreateGameTemplateActions(Pyro::UiNodeActionContainer& outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction);
        static void addTemplateActions(Pyro::UiNodeActionContainer& outActions, Pyro::UiNodeAction::ExecuteEventHandler executeAction);


        static void setupTeamsCreateParameters(Pyro::UiNodeParameterStruct &parameters, const char* tabName = "");
        static void setupTeamIdsAndCapacityParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::TeamIdVector& teamIdVectorParam, uint16_t &teamCapacity);
        static void setupGameSettings(Pyro::UiNodeParameterStruct &parameters, const char* tabName = "");
        static void setupCreateGameParameters(Pyro::UiNodeParameterStruct &parameters, bool resetGameParamsOnly = false);
        static void setupCreateGameParametersPJD(Pyro::UiNodeParameterStruct &parameters, bool resetGameParamsOnly = false);
        static void setupCreateGameBaseParameters(Pyro::UiNodeParameterStruct &parameters, bool resetGameParamsOnly = false);
        static void setupCommonJoinGameParameters(Pyro::UiNodeParameterStruct &parameters);
        static void setupJoinGameByIdParameters(Pyro::UiNodeParameterStruct &parameters);
        static void setupJoinGameByUserNameParameters(Pyro::UiNodeParameterStruct &parameters);
        static void setupJoinGameByUserListParameters(Pyro::UiNodeParameterStruct &parameters);
        static void setupExternalSessionCreateParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t * tabName = nullptr);
#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        static void setupExternalSessionUpdateParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t * tabName = nullptr);
        static void setupExternalSessionStatusParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t * tabName = nullptr);
#endif
        static void setupReservedExternalPlayersParameters(Pyro::UiNodeParameterStruct &parameters);
        static void setupPlayerJoinDataParameters(Pyro::UiNodeParameterStruct &parameters);
        static void setupGameAttributeParameters(Pyro::UiNodeParameterStruct &parameters, bool addSearchParams, const char* tabName = "");
        static void setupPreferredGamesParameters(Pyro::UiNodeParameterStruct &parameters);
        static void setupPreferredPlayersParameters(Pyro::UiNodeParameterStruct &parameters);
        static void parseCreateGameOnlyParams(Blaze::GameManager::Game::CreateGameParametersBase& createGameParams, Pyro::UiNodeParameterStruct &parameters);
        static void parsePlayerJoinDataParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::PlayerJoinData &playerJoinData);

        void parseExternalSessionCreateParams(Pyro::UiNodeParameterStruct &parameters, Blaze::XblSessionTemplateName& sessionTemplateName, Blaze::ExternalSessionCustomDataString& customData);
        void parseExternalSessionCreateParams(Pyro::UiNodeParameterStruct &parameters, Blaze::ExternalSessionCustomData& customData, Blaze::ExternalSessionStatus& status);
        void parseExternalSessionStatus(Pyro::UiNodeParameterStruct &parameters, Blaze::ExternalSessionStatus& status);
        static void parseReservedExternalPlayersParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::UserIdentificationList &reservedPlayerIdentifications);
        static void parsePreferredGamesParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::MatchmakingCriteriaData& mCriteria);
        static void parsePreferredPlayersParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::MatchmakingCriteriaData& mCriteria);
        static void parseGameSettings(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::GameSettings &gameSettings);
        static void setupRoleInformationParameters(Pyro::UiNodeParameterStruct &parameters, const char8_t* tabName = "");
        static void parseRoleInformationParameters(Pyro::UiNodeParameterStruct &parameters, Blaze::GameManager::RoleInformation &roleInformation);

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
        static Blaze::GameManager::GameId getGameIdFromMps(Microsoft::Xbox::Services::Multiplayer::MultiplayerSession^ const mps);
        static Blaze::GameManager::GameType getGameTypeFromMps(Microsoft::Xbox::Services::Multiplayer::MultiplayerSession^ const mps);
        static Platform::String^ getCustomDataStringFromMps(Microsoft::Xbox::Services::Multiplayer::MultiplayerSession^ const mps);
#endif
        static void setDefaultRole(Blaze::GameManager::RoleNameToPlayerMap& joiningRoles, const char8_t* roleNameStr);

        void parseGameAttributes(const char* attrValue, char* result);
        static void setupTeamCriteria(Pyro::UiNodeParameterStruct& parameterse, uint16_t teamCount, uint16_t gameCapacity, Blaze::GameManager::RoleCriteriaMap& roleCriteria);
        static void setUpRoles(Pyro::UiNodeParameterStruct& parameters, Blaze::GameManager::RoleInformation& roleInformation, Blaze::GameManager::RoleNameToPlayerMap& desiredRoles);
        static const char8_t* getInitialPlayerRole(const char8_t* strGameMode, Blaze::GameManager::GameType gameType);

    protected:
        virtual void onGameCreated(Blaze::GameManager::Game *createdGame);
        virtual void onGameDestructing(Blaze::GameManager::GameManagerAPI *gameManagerApi, Blaze::GameManager::Game *dyingGame, Blaze::GameManager::GameDestructionReason reason);
        virtual void onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult,
            const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
            Blaze::GameManager::Game *game);
        void updateMatchmakingResult(Blaze::GameManager::MatchmakingResult matchmakingResult, 
                                             const Blaze::GameManager::DebugFindGameResults& mmFindGameResults, 
                                             const Blaze::GameManager::DebugCreateGameResults& mmCreateGameResults,
                                             Blaze::GameManager::Game *game);
        virtual void onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
            const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList);
        virtual void onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList *gameBrowserList,
            const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *removedGames,
            const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *updatedGames);
        virtual void onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList *gameBrowserList);
        virtual void onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList);
        virtual void onUserGroupJoinGame(Blaze::GameManager::Game *game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId groupId);
        virtual void onTopologyHostInjected(Blaze::GameManager::Game *game);
        virtual void onReservedPlayerIdentifications(Blaze::GameManager::Game *game, uint32_t userIndex, const Blaze::UserIdentificationList *reservedPlayerIdentifications);

        virtual void onRemoteMatchmakingStarted(const Blaze::GameManager::NotifyRemoteMatchmakingStarted *matchmakingStartedNotification, uint32_t userIndex);
        virtual void onRemoteMatchmakingEnded(const Blaze::GameManager::NotifyRemoteMatchmakingEnded *matchmakingEndedNotification, uint32_t userIndex);
        virtual void onRemoteJoinFailed(const Blaze::GameManager::NotifyRemoteJoinFailed *remoteJoinFailedNotification, uint32_t userIndex);

        virtual void onQosPingSiteLatencyRetrieved(bool success);
    private:
        Blaze::GameManager::GetMatchmakingConfigResponse mMatchmakingConfigResponse;

        static Blaze::GameManager::GetGameBrowserScenariosAttributesResponse& getGameBrowserScenarioAttributesResponse()
        {
            static Blaze::GameManager::GetGameBrowserScenariosAttributesResponse mGameBrowserScenarioAttributesResponse;
            return mGameBrowserScenarioAttributesResponse;
        }

        static Blaze::GameManager::GetScenariosAndAttributesResponse& getScenariosAndAttributesResponse()
        {
            static Blaze::GameManager::GetScenariosAndAttributesResponse mScenariosAndAttributesResponse;
            return mScenariosAndAttributesResponse;
        }

        static Blaze::GameManager::GetTemplatesAndAttributesResponse& getTemplatesAndAttributesResponse()
        {
            static Blaze::GameManager::GetTemplatesAndAttributesResponse mTemplatesAndAttributesResponse;
            return mTemplatesAndAttributesResponse;
        }

        typedef eastl::map <Blaze::GameManager::MatchmakingSessionId, time_t> MatchmakingSessionTimeMap;
        typedef eastl::pair <Blaze::GameManager::MatchmakingSessionId, time_t> MatchmakingSessionTimeMapPair;
        MatchmakingSessionTimeMap mMatchmakingSessionTimeMap;
        typedef eastl::map <Blaze::GameManager::MatchmakingScenarioId, time_t> MatchmakingScenarioTimeMap;
        typedef eastl::pair <Blaze::GameManager::MatchmakingScenarioId, time_t> MatchmakingScenarioTimeMapPair;
        MatchmakingScenarioTimeMap mMatchmakingScenarioTimeMap;

        void refreshMatchmakingConfig();
        void refreshTemplateConfigs();      // Scenarios and CreateGame Templates

        void removeWindow(GamePlayPtr gamePlayWindow);
        GamePlay &getGamePlayWindow(Blaze::GameManager::Game *game);
        MatchmakingManagement &getMatchmakingWindow(const Blaze::GameManager::GameId gameId);

        void setupBasicGameBrowserParameters(Pyro::UiNodeParameterStruct &parameters);

        void fillGameBrowserListParameters(Pyro::UiNodeParameterStructPtr parameters, Blaze::GameManager::GameBrowserList::GameBrowserListParameters& gameListParams);
        void addGameAttributeSearchCriteria(Blaze::GameManager::GameAttributeRuleCriteriaMap& gameAttrCriteriaMap, const char8_t* ruleName, const char8_t* matchPref, const char8_t* desiredValue, bool gamebrowser = false);

        PYRO_ACTION(GameManagement, ListJoinedGames);
        PYRO_ACTION(GameManagement, DisplayMyDisconnectReservations);
        PYRO_ACTION(GameManagement, BrowseGames);
        PYRO_ACTION(GameManagement, CreateNewGame);
        PYRO_ACTION(GameManagement, CreateNewGamePJD);
        PYRO_ACTION(GameManagement, CreateDedicatedServerGame);
        void actionScenarioMatchmake(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);    // Only used as a hook for the scenarios (no generic command).
        void actionCreateGameTemplate(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);    // Only used as a hook for the templates (no generic command).
        void actionGameBrowserScenarios(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);    // Only used as a hook for the scenarios (no generic command).
        PYRO_ACTION(GameManagement, CancelScenarioMatchmake);
        PYRO_ACTION(GameManagement, JoinGameById);
        PYRO_ACTION(GameManagement, JoinGameByUserName);
        PYRO_ACTION(GameManagement, JoinGameByUserNamePJD);
        PYRO_ACTION(GameManagement, JoinGameByUserList);
        PYRO_ACTION(GameManagement, CreateOrJoinGame);
        PYRO_ACTION(GameManagement, JoinBrowsedGame);
        PYRO_ACTION(GameManagement, SpectateBrowsedGame);
        PYRO_ACTION(GameManagement, JoinTeamGame);
        PYRO_ACTION(GameManagement, LeaveGame);
        PYRO_ACTION(GameManagement, LeaveGameByLUGroup);
        PYRO_ACTION(GameManagement, ClaimReservation);
        PYRO_ACTION(GameManagement, OverrideGeoip);
        void CreateGameBrowserListScenarioCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::GameBrowserList* gameBrowserList, const char8_t* errDetails);
        void CreateGameBrowserListCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::GameManager::GameBrowserList* gameBrowserList, const char8_t* errDetails);
        void JoinGameCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game, const char8_t *errorMsg);
        void JoinGameByUserListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game, const char8_t *errorMsg);
        void LeaveGameCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void LeaveGameByLUGroupCb(Blaze::BlazeError blazeError, Blaze::GameManager::Game *game);
        void CreateGameCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::Game *game);
        void GetMatchmakingConfigCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::GameManager::GetMatchmakingConfigResponse *matchmakingConfigResponse);
        void StartMatchmakingScenarioCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::GameManager::MatchmakingScenario* scenario, const char8_t* errDetails);
        void LookupDisconnectReservationsCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::GameManager::GameBrowserDataList* gameBrowserList);
        void GetScenarioAttributesConfigCb(const Blaze::GameManager::GetScenariosAttributesResponse* scenarioInfo, Blaze::BlazeError err, Blaze::JobId jobId);
        void GetGameBrowserScenarioAttributesConfigCb(const Blaze::GameManager::GetGameBrowserScenariosAttributesResponse* scenarioInfo, Blaze::BlazeError err, Blaze::JobId jobId);
        void GetScenariosAndAttributesCb(const Blaze::GameManager::GetScenariosAndAttributesResponse* scenarioInfo, Blaze::BlazeError err, Blaze::JobId jobId);
        void GetCreateGameTemplateAttributesConfigCb(const Blaze::GameManager::GetTemplatesAttributesResponse* templateInfo, Blaze::BlazeError err, Blaze::JobId jobId);
        void GetTemplatesAndAttributesCb(const Blaze::GameManager::GetTemplatesAndAttributesResponse* templateInfo, Blaze::BlazeError err, Blaze::JobId jobId);


        void GameWindowClosing(Pyro::UiNodeWindow *sender);
        void MatchmakingWindowClosing(Pyro::UiNodeWindow *sender);
        void ResultNotificationCb(const Blaze::GameReporting::ResultNotification *resultNotification, uint32_t userIndex);

        void OverrideGeoIpCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
        bool shouldRecreate(const Blaze::GameManager::Game &dyingGame, Blaze::GameManager::GameDestructionReason destructionReason) const;

        bool mUseNetGameDist;

};

}

#endif
