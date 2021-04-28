/*! ************************************************************************************************/
/*!
\file externalsessionutilmanager.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_MANAGER_H
#define BLAZE_GAMEMANAGER_EXTERNAL_SESSION_UTIL_MANAGER_H

#include "framework/tdf/userdefines.h"
#include "EASTL/string.h"
#include "gamemanager/tdf/externalsessiontypes_server.h" 
#include "gamemanager/tdf/externalsessionconfig_server.h" 
#include "component/gamemanager/rpc/gamemanager_defines.h"
#include "framework/controller/controller.h"

namespace Blaze
{
class ExternalMemberInfo;
class UserSessionExistenceData;
class UserIdentification;
class UserSessionInfo;


namespace GameManager
{
class GameSessionMaster; 
class GameManagerSlaveImpl; 
class GameManagerMasterImpl;
class ExternalSessionUtil;

typedef eastl::map<ClientPlatformType, ExternalSessionUtil* > ExternalSessionUtilMap;
        
typedef UpdateExternalSessionPresenceForUserResponse UpdateExternalSessionPresenceForUserResult;
typedef UpdateExternalSessionPresenceForUserErrorInfo UpdateExternalSessionPresenceForUserErrorResult;

class ExternalSessionUtilManager
{
public: 
    ExternalSessionUtilManager();
    ~ExternalSessionUtilManager();

    const ExternalSessionUtilMap& getExternalSessionUtilMap() const;
    ExternalSessionUtil* getUtil(ClientPlatformType platform) const;

    bool isMockPlatformEnabled(ClientPlatformType platform) const;

    bool onPrepareForReconfigure(const ExternalSessionServerConfigMap& config) const;
    void onReconfigure(const ExternalSessionServerConfigMap& config, const TimeValue reservationTimeout);

    Blaze::BlazeRpcError getAuthToken(const UserIdentification& ident, const char8_t* serviceName, eastl::string& buf) const;
    
    Blaze::BlazeRpcError join(const JoinExternalSessionParameters& params, Blaze::ExternalSessionResult* result, bool commit, const ExternalSessionJoinInitError& joinInitErr, ExternalSessionApiError& apiErr) const;
    Blaze::BlazeRpcError leave(const LeaveGroupExternalSessionParameters& params, ExternalSessionApiError& apiErr) const;

    Blaze::BlazeRpcError setPrimary(const SetPrimaryExternalSessionParameters& params, SetPrimaryExternalSessionResult& result, SetPrimaryExternalSessionErrorResult* errorResult) const;
    Blaze::BlazeRpcError clearPrimary(const ClearPrimaryExternalSessionParameters& params, ClearPrimaryExternalSessionErrorResult* errorResult) const;
    Blaze::BlazeRpcError updatePresenceForUser(const UpdateExternalSessionPresenceForUserParameters& params, UpdateExternalSessionPresenceForUserResult& result, UpdateExternalSessionPresenceForUserErrorResult& errorResult) const;
    bool hasUpdatePrimaryExternalSessionPermission(const UserSessionExistenceData& userSession) const;
 
    typedef eastl::map<ClientPlatformType, ExternalIdList> ExternalIdsByPlatform;
    Blaze::BlazeRpcError getMembers(const ExternalSessionIdentification& sessionIdentification, ExternalIdsByPlatform& memberExternalIds, ExternalSessionApiError& apiErr) const;
    void isFullForExternalSessions(const GameSessionMaster& gameSession, uint16_t& maxUsers, PlatformBoolMap& outFullMap) const;

    bool isUpdateRequired(const ExternalSessionUpdateInfo& origValues, const ExternalSessionUpdateInfo& newValues, const ExternalSessionUpdateEventContext& context) const;
    bool doesCustomDataExceedMaxSize(const ClientPlatformTypeList& gamePlatforms, uint32_t count) const;

    Blaze::BlazeRpcError updateUserProfileNames(UserJoinInfoList& usersInfo, const ExternalIdList& externalUsersToUpdate, const char8_t* externalProfileToken) const;

    Blaze::BlazeRpcError checkRestrictions(const CheckExternalSessionRestrictionsParameters& params, ExternalSessionApiError& apiErr) const;


    Blaze::BlazeRpcError handleUpdateExternalSessionImage(const UpdateExternalSessionImageRequest& request, const GetExternalSessionInfoMasterResponse& getRsp, GameManagerSlaveImpl& component, UpdateExternalSessionImageErrorResult& errorResult) const;
    void handleUpdateExternalSessionProperties(GameManagerMasterImpl* gmMaster, GameId gameId, ExternalSessionUpdateInfoPtr origValues, ExternalSessionUpdateEventContextPtr context) const;

    void prepForReplay(GameSessionMaster& gameSession) const;
    Blaze::BlazeRpcError prepForCreateOrJoin(CreateGameRequest& request) const;

    friend ExternalSessionUtil;
private:
    ExternalSessionUtilMap mUtilMap;
};

}
}

#endif
