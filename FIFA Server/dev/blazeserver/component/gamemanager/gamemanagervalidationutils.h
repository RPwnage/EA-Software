/*! ************************************************************************************************/
/*!
    \file gamemanagervalidationutils.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_VALIDATION_UTILS_H
#define BLAZE_GAMEMANAGER_VALIDATION_UTILS_H

#include "gamemanager/tdf/gamemanager.h"
#include "component/gamemanager/externalsessions/externalsessionutil.h"

namespace Blaze
{
namespace GameManager
{
    class GameManagerSlaveImpl;

    typedef eastl::hash_map<PersistedGameId, GameId, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > GameIdByPersistedIdMap;
    typedef eastl::map<SlotType, uint16_t> SlotTypeSizeMap;
namespace ValidationUtils
{

    BlazeRpcError validateUserGroup(UserSessionId callingUserSessionId, const UserGroupId& userGroupId, UserSessionIdList &userSessionIdList);

    bool isUserSessionInList(UserSessionId userSessionId, const UserJoinInfoList& joiningUserInfoList, const UserJoinInfo** outSelfInfo = nullptr);
    BlazeRpcError setJoinUserInfoFromSession(UserSessionInfo& userInfo, const UserSessionExistence& joiningSession, const ExternalSessionUtilManager* extSessionUtilManager);
    void setJoinUserInfoFromSessionExistence(UserSessionInfo& userInfo, UserSessionId userSessionId, const BlazeId newBlazeId, const PlatformInfo& platformInfo, const Locale newAccountLocale, const uint32_t newAccountCountry, const char8_t* serviceName, const char8_t* productName);

    void insertHostSessionToPlayerJoinData(PlayerJoinData& playerJoinData, const UserSessionMaster* session);
    BlazeRpcError userIdListToJoiningInfoList(PlayerJoinData& playerJoinData, const UserIdentificationList& groupUserIds, UserJoinInfoList& usersInfo, const ExternalSessionUtilManager* extSessionUtilManager, bool autoHostAssign = false);

    BlazeRpcError loadGroupUsersIntoPlayerJoinData(PlayerJoinData& playerJoinData, UserIdentificationList& userIds, GameManagerSlaveImpl& component);

    BlazeRpcError validateGameCapacity(const SlotTypeSizeMap& joiningPlayerMap, const CreateGameRequest& createGameRequest);

    BlazeRpcError validateTeamCapacity(const SlotTypeSizeMap& joiningPlayerMap, const CreateGameRequest& createGameRequest, bool isVirtualWithReservation);

    void validatePlayersInCreateRequest(const CreateGameMasterRequest& createGameRequest, BlazeRpcError& error, SlotTypeSizeMap& sizeMap, bool isVirtualWithReservation);

    void populateSlotTypeSizes(const CreateGameMasterRequest& createGameRequest, SlotTypeSizeMap& sizeMap, bool isVirtualWithReservation = false);

    BlazeRpcError validatePlayerRolesForCreateGame(const CreateGameMasterRequest& request, bool isReset = false);

    bool validateAttributeIsPresent(const Collections::AttributeMap& attributeMap, const Collections::AttributeName& attributeName);

    bool hasValidCCSConfig(const GameManagerServerConfig& config);

    bool isSetupToUseCCS(const GameManagerServerConfig& config);

    bool validateFindDedicatedServerRules(const char8_t* createGameTemplate, const GameManagerServerConfig& config, ConfigureValidationErrors& validationErrors);

    BlazeRpcError validateRequestCrossplaySettings(bool& crossplayEnabled, const UserJoinInfoList& usersInfo, bool checkTopologies, const GameCreationData& gameCreationData,
        const GameSessionServerConfig& config, ClientPlatformTypeList& clientPlatformListOverride, BlazeId ownerBlazeId, bool crossplayMustMatch);

    void getCCSModeToUse(const GameSessionServerConfig& config, const char8_t* gameMode, bool crossplayEnabled, CCSMode& ccsMode);
    void getCCSModeAndPoolToUse(const GameManagerServerConfig& config, const char8_t* gameMode, bool crossplayEnabled, CCSMode& ccsMode, const char8_t*& ccsPool);
    BlazeRpcError validateExternalOwnerParamsInReq(const CreateGameRequest& request, const UserSessionInfo& extOwnerUsersessInfo);
    void fixupPjdByRemovingCaller(PlayerJoinData& playerJoinData);

} // namespace ValidationUtils
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_VALIDATION_UTILS_H
