/*! ************************************************************************************************/
/*!
    \file gamemanagerhelperutils.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_HELPER_UTILS_H
#define BLAZE_GAMEMANAGER_HELPER_UTILS_H

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/gamebrowser_server.h"

namespace Blaze
{
namespace GameManager
{
class GameManagerSlave;

BlazeRpcError findDedicatedServerHelper(GameManagerSlave* component, GameIdsByFitScoreMap& outGamesToReset, 
                                        bool allowMismatchedPingSites,
                                        const PingSiteAliasList* preferredPingSites,
                                        GameProtocolVersionString gameProtocolVersionString,
                                        uint16_t maxPlayerCapacity,
                                        GameNetworkTopology netTopology,
                                        const char8_t* templateName,
                                        const FindDedicatedServerRulesMap* findDedicatedServerRulesMap,
                                        const UserSessionInfo* hostUserSessionInfo,
                                        const ClientPlatformTypeList& overrideList);

UserSessionInfo* addHostSessionInfo(UserJoinInfoList& usersInfo, UserJoinInfoPtr* hostJoinInfoPtr = nullptr);
UserSessionInfo* getHostSessionInfo(UserJoinInfoList& usersInfo, UserJoinInfoPtr* hostJoinInfoPtr = nullptr);
const UserSessionInfo* getHostSessionInfo(const UserJoinInfoList& usersInfo, UserJoinInfoPtr* hostJoinInfoPtr = nullptr);
UserSessionId getHostUserSessionId(const UserJoinInfoList& usersInfo);

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_HELPER_UTILS_H
