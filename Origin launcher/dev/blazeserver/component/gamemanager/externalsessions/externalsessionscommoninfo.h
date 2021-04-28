/*! ************************************************************************************************/
/*!
    \file   externalsessionscommoninfo.h

    Helpers for populating the data passed between GM/MM and ExternalSessionUtil.

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_EXTERNAL_SESSIONS_COMMON_INFO_H
#define BLAZE_GAMEMANAGER_EXTERNAL_SESSIONS_COMMON_INFO_H

#include "framework/system/fiber.h" // for DEFINE_ASYNC_RET in initRequestExternalSessionData
#include "framework/blazedefines.h"
#include "gamemanager/tdf/matchmaker_types.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/externalsessiontypes_server.h"
#include "gamemanager/tdf/gamemanager_server.h"
#include "EABase/eabase.h"

namespace Blaze
{
    class ExternalSessionServerConfig;
namespace GameManager
{
    class GameSessionServerConfig;
    class MatchmakingServerConfig;
    class ExternalSessionMasterRequestData;
    class ExternalSessionUtilManager;
    class UserSessionInfo;
    class GameSessionMaster;
    class ReplicatedGameDataServer;
    class GameManagerSlaveImpl;

    typedef GameManager::UpdateExternalSessionPresenceForUserResponse UpdatePrimaryExternalSessionForUserResult;
    typedef GameManager::UpdateExternalSessionPresenceForUserErrorInfo UpdatePrimaryExternalSessionForUserErrorResult;

    uint64_t getNextGameExternalSessionUniqueId();

    /*! \brief convert the error code from external service util to game manager specific error */
    BlazeRpcError convertExternalServiceErrorToGameManagerError(BlazeRpcError err);

    bool shouldRetryAfterExternalSessionUpdateError(BlazeRpcError err);

    bool isExternalSessionJoinRestrictionError(BlazeRpcError err);

    /*! \brief convert the error code from external service util to matchmaking specific error */
    BlazeRpcError convertExternalServiceErrorToMatchmakingError(BlazeRpcError err);

    /*! \brief get the GameType's appropriate ExternalSessionType */
    ExternalSessionType gameTypeToExternalSessionType(GameType gameType);

    /*! \brief get the ExternalSessionType's appropriate GameType */
    GameType externalSessionTypeToGameType(ExternalSessionType externalSessionType);

    /*! \brief if adding an external session, get its name and correlation id to initialize the blaze game session request with */
    BlazeRpcError initRequestExternalSessionDataXbox(ExternalSessionMasterRequestData& req, const char8_t* sessionTemplateName, GameType gameType, const UserSessionInfo& caller,
        const GameSessionServerConfig& gameSessionConfig, const ExternalSessionUtilManager& extSessionUtilManager);
    BlazeRpcError initRequestExternalSessionData(ExternalSessionMasterRequestData& req, const ExternalSessionIdentification& externalSessionIdentSetup, GameType gameType, const UserJoinInfoList& usersInfo,
        const GameSessionServerConfig& gameSessionConfig, const ExternalSessionUtilManager& externalSessionUtilMgr, GameManagerSlaveImpl* slaveImpl = nullptr);
    BlazeRpcError initRequestExternalSessionData(ExternalSessionMasterRequestData& req, const ExternalSessionIdentification& externalSessionIdentSetup, GameType gameType, const UserJoinInfoList& usersInfo,
        const GameSessionServerConfig& gameSessionConfig, const ExternalSessionUtilManager& extSessionUtilManager, GameManagerSlaveImpl& gmSlave);

    /*! brief (DEPRECATED: future implns use updatePresenceForUser()) */
    void setExternalUserJoinInfoFromUserSessionInfo(ExternalUserJoinInfo& externalInfo, const UserSessionInfo& userInfo, bool isReserved, TeamIndex teamIndex = UNSPECIFIED_TEAM_INDEX);

    /*! brief populate the game's creation info from the game session. Note: We decouple GM from knowledge of info's mappings to the actual first party specific external session properties here. The actual mappings are done at the external session util. */
    void setExternalSessionCreationInfoFromGame(ExternalSessionCreationInfo& createInfo, const GameSessionMaster& gameSession);

    /*! brief populate the game's update info for updating its external session properties. Note: We decouple GM from knowledge of info's mappings to the actual first party specific external session properties here. The actual mappings are done at the external session util. */
    void setExternalSessionUpdateInfoFromGame(ExternalSessionUpdateInfo& updateInfo, const GameSessionMaster& gameSession);

    /*! brief whether the external session properties update is for the final snapshot before game cleanup. */
    bool isFinalExtSessUpdate(const ExternalSessionUpdateEventContext& context);

    /*! brief get the simplified game state from the game session. */
    SimplifiedGamePlayState getSimplifiedGamePlayState(const GameSessionMaster& gameSession);

    /*! brief (DEPRECATED: future implns use updatePresenceForUser()) append user to param's user list, and add any other missing fields for the external session update, from game session. */
    void updateExternalSessionJoinParams(JoinExternalSessionParameters& params, const UserSessionInfo& userToAdd, bool isReserved, TeamIndex teamIndex, const GameSessionMaster& gameSession);

    /*! brief (DEPRECATED: future implns use updatePresenceForUser()) Helper for adding copy of users to another result's list. Note: this *appends* users to the target */
    void externalSessionJoinParamsToOtherResponse(const JoinExternalSessionParameters& source, JoinExternalSessionParameters& target);

    /*! brief (DEPRECATED: future implns use updatePresenceForUser()) clear param's fields so that they can be re-set via updateExternalSessionJoinParams() */
    void clearExternalSessionJoinParams(JoinExternalSessionParameters& params);

    void setExternalUserLeaveInfoFromUserSessionInfo(ExternalUserLeaveInfo& externalInfo, const UserSessionInfo& userInfo);

    /*! brief append user to param's user list, and add any other missing fields for the external session update, from game session. */
    void updateExternalSessionLeaveParams(LeaveGroupExternalSessionParameters& params, const UserSessionInfo& userToAdd, const GameSessionMaster& gameSession);

    /*! brief overload based on user's external id. */
    void updateExternalSessionLeaveParams(LeaveGroupExternalSessionParameters& params, ExternalId externalId, const GameSessionMaster& gameSession, ClientPlatformType platform);

    /*! brief whether error from an external session join attempt indicates external session possibly got out of sync with Blaze game.*/
    bool shouldResyncExternalSessionMembersOnJoinError(BlazeRpcError error);

    /*! brief whether the external session identification is initialized/set. */
    bool isExtSessIdentSet(const ExternalSessionIdentification& sessionIdentification);

    /*! brief whether the external session identification is initialized/set, for the platform activity type. */
    bool isExtSessIdentSet(const ExternalSessionIdentification& sessionIdentification, const ExternalSessionActivityTypeInfo& forType);

    /*! brief whether the external session identifications have equivalent values, for the platform activity type. */
    bool isExtSessIdentSame(const ExternalSessionIdentification& a, const ExternalSessionIdentification& b, const ExternalSessionActivityTypeInfo& forType);

    /*! brief copies one external session identification's values, for the platform activity type, to another. */
    void setExtSessIdent(ExternalSessionIdentification& to, const ExternalSessionIdentification& from, const ExternalSessionActivityTypeInfo& forType);

    /*! brief transfer deprecated external session parameters to the ExternalSessionIdentification. For backward compatibility with older clients.*/
    void oldToNewExternalSessionIdentificationParams(const char8_t* sessionTemplate, const char8_t* externalSessionName, const char8_t* externalSessionCorrelationId, const char8_t* npSessionId, ExternalSessionIdentification& newParam);

    /*! brief transfer the replicated game data's ExternalSessionIdentification to its deprecated external session members. For backward compatibility with older clients. Pre: replicatedGameData's mExternalSessionIdentification has been populated.*/
    void newToOldReplicatedExternalSessionData(ReplicatedGameData& replicatedGameData);

    /*! brief transfer deprecated external session GameActivity parameters to the new members. For backward compatibility with older clients.*/
    void oldToNewGameActivityParams(GameActivity& gameActivity);

    /*! brief transfer new external session GameActivity parameters to the deprecated members. For backward compatibility with older clients.*/
    void newToOldGameActivityResults(UpdatePrimaryExternalSessionForUserResult& result);
    void newToOldGameActivityResults(UpdatePrimaryExternalSessionForUserErrorResult& result);

    /*! brief return the ExternalSessionIdentification's string for logging.*/
    const Blaze::FixedString256 toLogStr(const ExternalSessionIdentification& sessionIdentification);

    /*! brief return the ExternalSessionIdentification's string for logging, for the platform activity type.*/
    const Blaze::FixedString256 toLogStr(const ExternalSessionIdentification& sessionIdentification, const ExternalSessionActivityTypeInfo& forType);

    /*! brief return game's string for logging.*/
    const Blaze::FixedString256 toLogStr(const ExternalSessionUpdateInfo& gameInfo);

// ExternalMemberInfo tracking list helpers:

    /*! brief fetch the list by platform activity type, or insert new record if missing, to the map. */
    ExternalMemberInfoList& getOrAddListInMap(ExternalMemberInfoListByActivityTypeInfo& map, const ExternalSessionActivityTypeInfo& forType);

    /*! brief fetch the list by platform activity type from the map. */
    ExternalMemberInfoList* getListFromMap(const ExternalMemberInfoListByActivityTypeInfo& map, const ExternalSessionActivityTypeInfo& forType);

    /*! brief whether there is a non empty members list for the platform activity type in the map. */
    bool hasExternalMembersInMap(const ExternalMemberInfoListByActivityTypeInfo& map, const ExternalSessionActivityTypeInfo& forType);

    /*! brief whether specified user is in the external session members list. */
    bool getUserInExternalMemberInfoList(PlayerId blazeId, ExternalMemberInfoList& list, ExternalMemberInfoList::iterator& found);
    const ExternalMemberInfo* getUserInExternalMemberInfoList(PlayerId blazeId, const ExternalMemberInfoList& list);

    /*! brief return whether the lists have the same set of member infos. */
    bool areExternalMemberListsEquivalent(const ExternalMemberInfoList& a, const ExternalMemberInfoList& b);

// End of ExternalMemberInfo tracking list helpers

} // namespace GameManager
} // namespace Blaze

#endif //BLAZE_GAMEMANAGER_EXTERNAL_SESSIONS_COMMON_INFO_H
