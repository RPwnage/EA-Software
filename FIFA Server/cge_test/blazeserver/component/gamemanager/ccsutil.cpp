/*! ************************************************************************************************/
/*!
    \file ccsutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "component/gamemanager/ccsutil.h"
#include "framework/controller/controller.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/connection/socketutil.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/matchmaker/rules/expandedpingsiterule.h"
#include "proxycomponent/ccs/rpc/ccsslave.h"
#include "framework/oauth/accesstokenutil.h"

namespace Blaze
{
namespace GameManager
{

CCSUtil::CCSUtil()
{

}

CCSUtil::~CCSUtil()
{
}

eastl::string CCSUtil::getClientId() const
{
    return eastl::string(eastl::string::CtorSprintf(),"%s/%s/%s","blaze", gController->getServiceEnvironment(), gController->getDefaultServiceName());
}

void CCSUtil::generateTunnelKey(const GameManager::GameSessionMaster& gameSessionMaster, char* tunnelKey, size_t tunnelKeySize, const UUID& gameUUID, ConnectionGroupId consoleConnGroupId) const
{
    memset(tunnelKey,0, tunnelKeySize);
    const PlayerInfoMaster* player = getAnyPlayerInConnectionGroup(gameSessionMaster, consoleConnGroupId);
    if (player != nullptr)
    {
        UUID consoleUUID = player->getUUID();
        if (gameUUID < consoleUUID)
        {
            blaze_snzprintf(tunnelKey, tunnelKeySize, "%s-%s-%s", gameUUID.c_str(), gameUUID.c_str(), consoleUUID.c_str());
        }
        else
        {
            blaze_snzprintf(tunnelKey, tunnelKeySize, "%s-%s-%s", gameUUID.c_str(), consoleUUID.c_str(), gameUUID.c_str());
        }
    }
}

const char8_t* CCSUtil::getProtoTunnelVer(const GameManager::GameSessionMaster& gameSessionMaster, ConnectionGroupId consoleConnGroupId) const
{
    const PlayerInfoMaster* player = getAnyPlayerInConnectionGroup(gameSessionMaster, consoleConnGroupId);
    if (player != nullptr)
    {
        return player->getProtoTunnelVer();
    }
        
    return nullptr;
}


void CCSUtil::fillConsoleConnectionInfo(const GameManager::GameSessionMaster& gameSessionMaster, CCS::ConnectionInfo* cInfo, ConnectionGroupId consoleConnGroupId, SlotId consoleSlotId) const
{
    char8_t tunnelKey[MAX_TUNNELKEY_LEN];
    UUID gameUUID(gameSessionMaster.getReplicatedGameSession().getUUID());
    generateTunnelKey(gameSessionMaster, tunnelKey, sizeof(tunnelKey), gameUUID, consoleConnGroupId);

    cInfo->setProtoTunnelKey(tunnelKey);
    cInfo->setProtoTunnelVer(getProtoTunnelVer(gameSessionMaster, consoleConnGroupId));
    cInfo->setGameConsoleIndex(consoleSlotId);
    
    char8_t consoleUniqueTag[64];
    blaze_snzprintf(consoleUniqueTag,sizeof(consoleUniqueTag),"%" PRIu64,consoleConnGroupId);
    cInfo->setGameConsoleUniqueTag(consoleUniqueTag);
}

int32_t CCSUtil::fillConsoleConnectionInfoV2(const GameManager::GameSessionMaster& gameSessionMaster, CCS::Consoles& consoles, ConnectionGroupId consoleConnGroupId, SlotId consoleSlotId, const char8_t* consoleIpAddress, const PingSiteLatencyByAliasMap& pingSiteInfo) const
{
    // Check if this console was already added into the list:
    char8_t consoleUniqueTag[64];
    blaze_snzprintf(consoleUniqueTag, sizeof(consoleUniqueTag), "%" PRIu64 "_%" PRIu8, consoleConnGroupId, consoleSlotId);
    int32_t curConsoleIndex = 0;
    for (auto& curConsole : consoles)
    {
        if (blaze_strcmp(curConsole->getGameConsoleUniqueTag(), consoleUniqueTag) == 0)
        {
            return curConsoleIndex;
        }
        ++curConsoleIndex;
    }

    // If it's not, then grab a new one: 
    auto cInfo = consoles.pull_back();

    char8_t tunnelKey[MAX_TUNNELKEY_LEN];
    UUID gameUUID(gameSessionMaster.getReplicatedGameSession().getUUID());
    generateTunnelKey(gameSessionMaster, tunnelKey, sizeof(tunnelKey), gameUUID, consoleConnGroupId);

    cInfo->setProtoTunnelKey(tunnelKey);
    cInfo->setProtoTunnelVer(getProtoTunnelVer(gameSessionMaster, consoleConnGroupId));
    cInfo->setGameConsoleIndex(consoleSlotId);
    cInfo->setGameConsoleUniqueTag(consoleUniqueTag);

    cInfo->setGameConsoleIp(consoleIpAddress);

    // iterate over all the ping sites, fill in the latency:
    for (auto& inPingInfo : pingSiteInfo)
    {
        int32_t latency = inPingInfo.second;

        CCS::PingSiteInfo* outPingInfo = cInfo->getPingSites().pull_back();
        outPingInfo->setPingSite(inPingInfo.first);
        outPingInfo->setLatency(latency > 0 ? latency : Blaze::CCS::INVALID_CCS_PING_SITE_LATENCY);  // Clamp latency to -1 for unresponsive ping sites. 
    }

    return curConsoleIndex;
}

BlazeRpcError CCSUtil::getNucleusAccessToken(eastl::string& accessToken)
{
    UserSession::SuperUserPermissionAutoPtr ptr(true);
    OAuth::AccessTokenUtil tokenUtil;
    BlazeRpcError tokErr = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NEXUS_S2S);
    if (tokErr == ERR_OK)
    {
        accessToken.sprintf("%s", tokenUtil.getAccessToken());
    }
    return tokErr;
}

SlotId CCSUtil::getSlotId(const GameManager::GameSessionMaster& gameSessionMaster, ConnectionGroupId connGrpId) const
{
    SlotId slotId =  DEFAULT_JOINING_SLOT;
    GameDataMaster::ConnectionSlotMap::const_iterator it =  gameSessionMaster.getGameDataMaster()->getConnectionSlotMap().find(connGrpId);
    if (it !=  gameSessionMaster.getGameDataMaster()->getConnectionSlotMap().end() )
    {
        slotId = it->second;
    }
    return slotId;
}


eastl::string getBestPingSiteByRule(const GameManager::GameSessionMaster& gameSessionMaster, PingSiteSelectionMethod selectionMethod)
{
    // If somehow connection group's players don't have one set, or we're fetching for a dedicated server host, use game's best ping site:
    eastl::string pingSite = gameSessionMaster.getBestPingSiteAlias();

    Matchmaker::ExpandedPingSiteRule::PingSiteOrderCalculator calc = Matchmaker::ExpandedPingSiteRule::getPingSiteOrderCalculator(selectionMethod);
    if (calc != nullptr)
    {
        // Fill the Latency information:
        Matchmaker::LatenciesByAcceptedPingSiteIntersection pingSiteIntersection;
        const PlayerRoster::PlayerInfoList playerList = gameSessionMaster.getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        for (auto& playerInfo : playerList)
        {
            for (auto& latencyValue : playerInfo->getUserInfo().getLatencyMap())
            {
                pingSiteIntersection[latencyValue.first].push_back(latencyValue.second);
            }
        }

        // Choose the first ping site, based on the selection method used:
        Matchmaker::OrderedPreferredPingSiteList orderedPreferredPingSites;
        calc(pingSiteIntersection, orderedPreferredPingSites);
        eastl::quick_sort(orderedPreferredPingSites.begin(), orderedPreferredPingSites.end());
        if (!orderedPreferredPingSites.empty())
        {
            pingSite = orderedPreferredPingSites.begin()->second;
        }
    }

    if (pingSite.empty())
        pingSite = UNKNOWN_PINGSITE;
    return pingSite;
}

eastl::string CCSUtil::getDataCenter(const GameManager::GameSessionMaster& gameSessionMaster) const
{
    // Choose a ping site based on the config settings' provided selection method. 
    eastl::string pingSite = getBestPingSiteByRule(gameSessionMaster, gGameManagerMaster->getConfig().getGameSession().getCcsPingSiteSelectionMethod());
    if (pingSite == UNKNOWN_PINGSITE || pingSite == INVALID_PINGSITE || pingSite.empty())
    {
        ERR_LOG("[CCSUtil].getDataCenter: game("<< gameSessionMaster.getGameId() <<"). The data center could not be located as the optimal ping site for the host may not have been configured correctly. Failing the request.");
    }
    return pingSite;
}

const PlayerInfoMaster* CCSUtil::getAnyPlayerInConnectionGroup(const GameManager::GameSessionMaster& gameSessionMaster, ConnectionGroupId consoleConnGroupId) const
{
    GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator idListIt = gameSessionMaster.getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(consoleConnGroupId);
    if (idListIt != gameSessionMaster.getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
    {
        const PlayerIdList* playerIds = (idListIt->second);
        if (playerIds != nullptr && !playerIds->empty())
        {
           for (PlayerIdList::const_iterator it = playerIds->begin(), itEnd = playerIds->end(); it != itEnd; ++it)
           {
               if ((*it) != INVALID_BLAZE_ID)
               {
                   return gameSessionMaster.getPlayerRoster()->getPlayer((*it));
               }
           }
        }
    }

    return nullptr;
}

BlazeRpcError CCSUtil::buildGetHostedConnectionRequest(const GameManager::GameSessionMaster& gameSessionMaster, CCSAllocateRequest& ccsAllocateRequest) const
{
    if (blaze_stricmp(gGameManagerMaster->getConfig().getCcsConfig().getCcsAPIVer(), "v1") == 0)
    {
        ccsAllocateRequest.getRequest().switchActiveMember(CCSAllocateRequestInternal::MEMBER_V1REQUEST);
        CCS::GetHostedConnectionRequest& request = *ccsAllocateRequest.getRequest().getV1Request();
    
        if (!gGameManagerMaster->isCCSConfigValid())
        {
            ERR_LOG("[CCSUtil].buildGetHostedConnectionRequest: game("<< gameSessionMaster.getGameId() <<"). request failed because of bad CCS configuration."
                << gGameManagerMaster->getConfig().getCcsConfig());
            return Blaze::ERR_SYSTEM;
        }
        else
        {
            eastl::string clientId = getClientId();
            request.getRequestBody().setCCSClientId(clientId.c_str());
            request.setAPIVersion(gGameManagerMaster->getConfig().getCcsConfig().getCcsAPIVer()); 
        }
    
        const char8_t* connSetId =  gameSessionMaster.getGameDataMaster()->getConnectionSetId();
        if ((connSetId != nullptr) && connSetId[0] != '\0')
        {
            request.setConnectionSetId(connSetId);
        }
        else
        {
            // We don't have a valid connection set id yet so we provide CCS service with our desired pool and data center
            if (gameSessionMaster.getCCSPool()[0] != '\0')
                request.setPool(gameSessionMaster.getCCSPool());
            else
                request.setPool(gGameManagerMaster->getConfig().getCcsConfig().getCcsPool());
            eastl::string dataCenter = getDataCenter(gameSessionMaster); 
            if (!dataCenter.empty() && dataCenter != UNKNOWN_PINGSITE && dataCenter != INVALID_PINGSITE)
            {
                request.setDataCenter(dataCenter.c_str());
            }
            else
            {
                return Blaze::ERR_SYSTEM;
            }
        }
        
        request.getRequestBody().setLeasetime(gGameManagerMaster->getCCSLeaseTimeMinutes());
        
        auto& conns = request.getRequestBody().getConnections();
        for (CCSRequestPairs::const_iterator it = ccsAllocateRequest.getCCSRequestPairs().begin(), itEnd = ccsAllocateRequest.getCCSRequestPairs().end(); it != itEnd; ++it)
        {
            SlotId console1SlotId = getSlotId(gameSessionMaster, (*it)->getConsoleFirstConnGrpId());
            SlotId console2SlotId = getSlotId(gameSessionMaster, (*it)->getConsoleSecondConnGrpId());
    
            if (console1SlotId == Blaze::GameManager::DEFAULT_JOINING_SLOT || console2SlotId == Blaze::GameManager::DEFAULT_JOINING_SLOT)
            {
                ERR_LOG("[CCSUtil].buildGetHostedConnectionRequest: game("<< gameSessionMaster.getGameId() <<").  request failed due to invalid connection slot id."
                    <<" console1(" << (*it)->getConsoleFirstConnGrpId() <<"," << console1SlotId 
                    <<"), console2(" << (*it)->getConsoleSecondConnGrpId() << "," << console2SlotId <<")"
                    <<" Request "<< ccsAllocateRequest);
                return Blaze::ERR_SYSTEM;
            }
    
            auto map = conns.pull_back();
    
            CCS::ConnectionInfo* cinfo1 = map->allocate_element();
            fillConsoleConnectionInfo(gameSessionMaster, cinfo1, (*it)->getConsoleFirstConnGrpId(), console1SlotId);
            map->insert(eastl::make_pair("game_console_1", cinfo1));
    
            CCS::ConnectionInfo* cinfo2 = map->allocate_element();
            fillConsoleConnectionInfo(gameSessionMaster, cinfo2, (*it)->getConsoleSecondConnGrpId(), console2SlotId);
            map->insert(eastl::make_pair("game_console_2", cinfo2));
        }

        request.getRequestBody().setRequestId(gGameManagerMaster->getNextCCSRequestId()); 
    }
    else
    {
        ccsAllocateRequest.getRequest().switchActiveMember(CCSAllocateRequestInternal::MEMBER_V2REQUEST);
        CCS::GetHostedConsoleConnectionRequest& request = *ccsAllocateRequest.getRequest().getV2Request();

        if (!gGameManagerMaster->isCCSConfigValid())
        {
            ERR_LOG("[CCSUtil].buildGetHostedConnectionRequest: game("<< gameSessionMaster.getGameId() <<"). request failed because of bad CCS configuration."
                << gGameManagerMaster->getConfig().getCcsConfig());
            return Blaze::ERR_SYSTEM;
        }
        else
        {
            eastl::string clientId = getClientId();
            request.getRequestBody().setCCSClientId(clientId.c_str());
            request.setAPIVersion(gGameManagerMaster->getConfig().getCcsConfig().getCcsAPIVer()); 
        }
    
        const char8_t* connSetId =  gameSessionMaster.getGameDataMaster()->getConnectionSetId();
        if ((connSetId != nullptr) && connSetId[0] != '\0')
        {
            request.setConnectionSetId(connSetId);
        }
        else
        {
            // We don't have a valid connection set id yet so we provide CCS service with our desired pool and data center
            if (gameSessionMaster.getCCSPool()[0] != '\0')
                request.setPool(gameSessionMaster.getCCSPool());
            else
                request.setPool(gGameManagerMaster->getConfig().getCcsConfig().getCcsPool());
            eastl::string dataCenter = getDataCenter(gameSessionMaster);
            if (!dataCenter.empty() && dataCenter != UNKNOWN_PINGSITE && dataCenter != INVALID_PINGSITE)
            {
                request.setDataCenter(dataCenter.c_str());
            }
            else
            {
                return Blaze::ERR_SYSTEM;
            }
        }
        
        request.getRequestBody().setLeasetime(gGameManagerMaster->getCCSLeaseTimeMinutes());

        // New Logic: Connection Pairs - NO INFO
        auto& conns = request.getRequestBody().getConnections();
        for (CCSRequestPairs::const_iterator it = ccsAllocateRequest.getCCSRequestPairs().begin(), itEnd = ccsAllocateRequest.getCCSRequestPairs().end(); it != itEnd; ++it)
        {
            SlotId console1SlotId = getSlotId(gameSessionMaster, (*it)->getConsoleFirstConnGrpId());
            SlotId console2SlotId = getSlotId(gameSessionMaster, (*it)->getConsoleSecondConnGrpId());

            if (console1SlotId == Blaze::GameManager::DEFAULT_JOINING_SLOT || console2SlotId == Blaze::GameManager::DEFAULT_JOINING_SLOT)
            {
                ERR_LOG("[CCSUtil].buildGetHostedConnectionRequest: game(" << gameSessionMaster.getGameId() << ").  request failed due to invalid connection slot id."
                    << " console1(" << (*it)->getConsoleFirstConnGrpId() << "," << console1SlotId
                    << "), console2(" << (*it)->getConsoleSecondConnGrpId() << "," << console2SlotId << ")"
                    << " Request " << ccsAllocateRequest);
                return Blaze::ERR_SYSTEM;
            }

            PlayerRoster::PlayerInfoList playerList = gameSessionMaster.getPlayerRoster()->getPlayers(PlayerRoster::ALL_PLAYERS);
            PlayerInfo* player1 = nullptr;
            PlayerInfo* player2 = nullptr;
            for (auto& curPlayer : playerList)
            {
                if (curPlayer->getConnectionGroupId() == (*it)->getConsoleFirstConnGrpId())
                    player1 = curPlayer;
                if (curPlayer->getConnectionGroupId() == (*it)->getConsoleSecondConnGrpId())
                    player2 = curPlayer;
            }

            if (player1 == nullptr || player2 == nullptr)
            {
                ERR_LOG("[CCSUtil].buildGetHostedConnectionRequest: game(" << gameSessionMaster.getGameId() << ").  request failed due to missing player."
                    << " console1(" << (*it)->getConsoleFirstConnGrpId() << "," << console1SlotId
                    << "), console2(" << (*it)->getConsoleSecondConnGrpId() << "," << console2SlotId << ")"
                    << " Request " << ccsAllocateRequest);
                return Blaze::ERR_SYSTEM;
            }

            // Add the pair to the connection map:
            auto& map = *conns.pull_back();
            map["game_console_1_index_in_list"] = fillConsoleConnectionInfoV2(gameSessionMaster, request.getRequestBody().getConsoles(), (*it)->getConsoleFirstConnGrpId(), console1SlotId, player1->getUserInfo().getConnectionAddr(), player1->getUserInfo().getLatencyMap());
            map["game_console_2_index_in_list"] = fillConsoleConnectionInfoV2(gameSessionMaster, request.getRequestBody().getConsoles(), (*it)->getConsoleSecondConnGrpId(), console2SlotId, player2->getUserInfo().getConnectionAddr(), player2->getUserInfo().getLatencyMap());
        }

        request.getRequestBody().setRequestId(gGameManagerMaster->getNextCCSRequestId()); 
    }

    return Blaze::ERR_OK;
}

BlazeRpcError CCSUtil::buildFreeHostedConnectionRequest(const GameManager::GameSessionMaster& gameSessionMaster, CCSFreeRequest& ccsFreeRequest, uint32_t console1Id /* = INVALID_CCS_CONNECTIVITY_ID */, uint32_t console2Id /* = INVALID_CCS_CONNECTIVITY_ID */)
{
    CCS::FreeHostedConnectionRequest& request = ccsFreeRequest.getFreeHostedConnectionRequest();
    
    if (!gGameManagerMaster->isCCSConfigValid())
    {
        ERR_LOG("[CCSUtil].buildFreeHostedConnectionRequest: game("<< gameSessionMaster.getGameId() <<").  request failed because of bad CCS configuration."
            <<" Configuration "<< gGameManagerMaster->getConfig().getCcsConfig());
        return Blaze::ERR_SYSTEM;
    }
    else
    {
        eastl::string clientId = getClientId();
        request.getRequestBody().setCCSClientId(clientId.c_str());
        request.setAPIVersion(gGameManagerMaster->getConfig().getCcsConfig().getCcsAPIVer()); 
    }

    const char8_t* connSetId =  gameSessionMaster.getGameDataMaster()->getConnectionSetId();
    if ((connSetId != nullptr) && connSetId[0] != '\0')
    {
        request.setConnectionSetId(connSetId);
    }
    else
    {
        ERR_LOG("[CCSUtil].buildFreeHostedConnectionRequest: game("<< gameSessionMaster.getGameId() <<").  request failed as ConnectionSetId not available. Request "<< ccsFreeRequest);
        return Blaze::ERR_SYSTEM;
    }

    if ((console1Id != INVALID_CCS_CONNECTIVITY_ID) && (console2Id != INVALID_CCS_CONNECTIVITY_ID)) // both ids are valid, we have a free request for a specific console pair
    {
        request.setGameConsoleId1(console1Id);
        request.setGameConsoleId2(console2Id);
    }
    else if ((console1Id != INVALID_CCS_CONNECTIVITY_ID) || (console2Id != INVALID_CCS_CONNECTIVITY_ID))
    {
        ERR_LOG("[CCSUtil].buildFreeHostedConnectionRequest: game("<< gameSessionMaster.getGameId() <<").  request failed as one Id is valid but other is not. console1Id("<<console1Id<<") console2Id("<<console2Id<<")"
            <<" Request "<<ccsFreeRequest);
        return Blaze::ERR_SYSTEM;
    }
   
    request.getRequestBody().setRequestId(gGameManagerMaster->getNextCCSRequestId()); 

    return Blaze::ERR_OK;
}

BlazeRpcError CCSUtil::buildLeaseExtensionRequest(const GameManager::GameSessionMaster& gameSessionMaster, CCSLeaseExtensionRequest& ccsLeaseExtensionRequest) const
{
    CCS::LeaseExtensionRequest& request = ccsLeaseExtensionRequest.getLeaseExtensionRequest();

    if (!gGameManagerMaster->isCCSConfigValid())
    {
        ERR_LOG("[CCSUtil].buildLeaseExtensionRequest: game("<< gameSessionMaster.getGameId() <<").  request failed because of bad CCS configuration."
            << gGameManagerMaster->getConfig().getCcsConfig());
        return Blaze::ERR_SYSTEM;
    }
    else
    {
        eastl::string clientId = getClientId();
        request.getRequestBody().setCCSClientId(clientId.c_str());
        request.setAPIVersion(gGameManagerMaster->getConfig().getCcsConfig().getCcsAPIVer()); 
    }

    const char8_t* connSetId =  gameSessionMaster.getGameDataMaster()->getConnectionSetId();
    if ((connSetId != nullptr) && connSetId[0] != '\0')
    {
        request.setConnectionSetId(connSetId);
    }
    else
    {
        ERR_LOG("[CCSUtil].buildLeaseExtensionRequest: game("<< gameSessionMaster.getGameId() <<").  request failed as ConnectionSetId not available. Request "<< ccsLeaseExtensionRequest);
        return Blaze::ERR_SYSTEM;
    }
    
    request.getRequestBody().setLeasetime(gGameManagerMaster->getCCSLeaseTimeMinutes());

    request.getRequestBody().setRequestId(gGameManagerMaster->getNextCCSRequestId()); 

    return Blaze::ERR_OK;
}

BlazeRpcError CCSUtil::allocateConnectivity(GameId gameId, const Blaze::CCS::GetHostedConnectionRequest& request, Blaze::CCS::GetHostedConnectionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse)
{
    TRACE_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). Hosted Connectivity requested for (" << request.getRequestBody().getConnections().size() << ") pair(s). Full request is " << request);
    
    CCS::CCSSlave* comp = (CCS::CCSSlave*)gOutboundHttpService->getService(CCS::CCSSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed as CCS component not available.");
        return Blaze::ERR_SYSTEM;
    }

    BlazeRpcError rc = comp->getHostedConnection(request,response,errResponse);
    if (rc == Blaze::ERR_OK)
    {
        // The CCS has claimed a successful response but we further error check to ensure that the response is as desired. Otherwise, missing pieces can lead to mysterious bugs or crashes.
        // These errors should be relatively rare. 
        if (request.getRequestBody().getRequestId() != response.getRequestId())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to incorrect request id in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        if (request.getRequestBody().getLeasetime() != response.getLeasetime())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to unexpected change in lease time. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        const char8_t* responseConnSetId = response.getConnectionSetId();
        const char8_t* requestConnSetId = request.getConnectionSetId();
        if ((responseConnSetId == nullptr || responseConnSetId[0] == '\0') && (requestConnSetId == nullptr || requestConnSetId[0] == '\0'))
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed because both the request and response have invalid connection set id. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }
        
        if (response.getAllocatedResources().empty())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to no info on the consoles. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        if (response.getAllocatedResources().size() != request.getRequestBody().getConnections().size())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to mismatch in request and response console pairs. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }
        
        uint32_t hostingServerConnectivityId = response.getHostingServerConnectivityId();
        if (!((hostingServerConnectivityId & 0x80000000) == 0) && ((hostingServerConnectivityId & 0x40000000) != 0))
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to invalid server id. The most significant two bits of hosting server id need to be 0 and 1. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        const char8_t* hostingServerIP = response.getHostingServerIP();
        if (hostingServerIP == nullptr || hostingServerIP[0] == '\0')
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to invalid hosting server IP in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        const char8_t* hostingServerPort = response.getHostingServerPort();
        if (hostingServerPort == nullptr || hostingServerPort[0] == '\0')
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to invalid hosting server port in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        if ((requestConnSetId != nullptr) && requestConnSetId[0] != '\0')
        {
            TRACE_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request succeeded. Added ("<< request.getRequestBody().getConnections().size()<<") connections to Connection set (" << requestConnSetId 
                <<"). Request Id("<<response.getRequestId()<<") Response "<<response);
        }
        else
        {
            TRACE_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request succeeded. Created Connection set (" << responseConnSetId 
                <<"). Added ("<< request.getRequestBody().getConnections().size()<<") connections to Connection set."
                << " Request Id("<<response.getRequestId()<<") Response "<<response);
        }
    }
    else
    {
        ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to error in CCS component. Error("<< ErrorHelp::getErrorName(rc) <<") Request "<<request <<"Response "<<errResponse);
    }

    return rc;
}

BlazeRpcError CCSUtil::allocateConnectivity(GameId gameId, const Blaze::CCS::GetHostedConsoleConnectionRequest& request, Blaze::CCS::GetHostedConnectionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse)
{
    TRACE_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). Hosted Connectivity requested for (" << request.getRequestBody().getConnections().size() << ") pair(s). Full request is " << request);
    
    CCS::CCSSlave* comp = (CCS::CCSSlave*)gOutboundHttpService->getService(CCS::CCSSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed as CCS component not available.");
        return Blaze::ERR_SYSTEM;
    }

    BlazeRpcError rc = comp->getHostedConsoleConnection(request,response,errResponse);
    if (rc == Blaze::ERR_OK)
    {
        // The CCS has claimed a successful response but we further error check to ensure that the response is as desired. Otherwise, missing pieces can lead to mysterious bugs or crashes.
        // These errors should be relatively rare. 
        if (request.getRequestBody().getRequestId() != response.getRequestId())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to incorrect request id in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        if (request.getRequestBody().getLeasetime() != response.getLeasetime())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to unexpected change in lease time. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        const char8_t* responseConnSetId = response.getConnectionSetId();
        const char8_t* requestConnSetId = request.getConnectionSetId();
        if ((responseConnSetId == nullptr || responseConnSetId[0] == '\0') && (requestConnSetId == nullptr || requestConnSetId[0] == '\0'))
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed because both the request and response have invalid connection set id. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }
        
        if (response.getAllocatedResources().empty())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to no info on the consoles. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        if (response.getAllocatedResources().size() != request.getRequestBody().getConnections().size())
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to mismatch in request and response console pairs. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }
        
        uint32_t hostingServerConnectivityId = response.getHostingServerConnectivityId();
        if (!((hostingServerConnectivityId & 0x80000000) == 0) && ((hostingServerConnectivityId & 0x40000000) != 0))
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to invalid server id. The most significant two bits of hosting server id need to be 0 and 1. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        const char8_t* hostingServerIP = response.getHostingServerIP();
        if (hostingServerIP == nullptr || hostingServerIP[0] == '\0')
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to invalid hosting server IP in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        const char8_t* hostingServerPort = response.getHostingServerPort();
        if (hostingServerPort == nullptr || hostingServerPort[0] == '\0')
        {
            ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to invalid hosting server port in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        if ((requestConnSetId != nullptr) && requestConnSetId[0] != '\0')
        {
            TRACE_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request succeeded. Added ("<< request.getRequestBody().getConnections().size()<<") connections to Connection set (" << requestConnSetId 
                <<"). Request Id("<<response.getRequestId()<<") Response "<<response);
        }
        else
        {
            TRACE_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request succeeded. Created Connection set (" << responseConnSetId 
                <<"). Added ("<< request.getRequestBody().getConnections().size()<<") connections to Connection set."
                << " Request Id("<<response.getRequestId()<<") Response "<<response);
        }
    }
    else
    {
        ERR_LOG("[CCSUtil].allocateConnectivity: game("<< gameId <<"). request failed due to error in CCS component. Error("<< ErrorHelp::getErrorName(rc) <<") Request "<<request <<"Response "<<errResponse);
    }

    return rc;
}

BlazeRpcError CCSUtil::freeConnectivity(GameId gameId, const Blaze::CCS::FreeHostedConnectionRequest& request, Blaze::CCS::FreeHostedConnectionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse)
{
    TRACE_LOG("[CCSUtil].freeConnectivity: game("<< gameId <<"). Hosted Connectivity free requested. "<< request);

    CCS::CCSSlave* comp = (CCS::CCSSlave*)gOutboundHttpService->getService(CCS::CCSSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        ERR_LOG("[CCSUtil].freeConnectivity: game("<< gameId <<"). request failed as CCS component not available.");
        return Blaze::ERR_SYSTEM;
    }

    bool deletingConnSet = (request.getGameConsoleId1() == INVALID_CCS_CONNECTIVITY_ID && request.getGameConsoleId2() == INVALID_CCS_CONNECTIVITY_ID);
    
    BlazeRpcError rc = comp->freeHostedConnection(request,response,errResponse);
    if (rc == Blaze::ERR_OK)
    {
        if (deletingConnSet)
        {
            TRACE_LOG("[CCSUtil].freeConnectivity: game("<< gameId <<"). free request succeeded. Freed entire connection set("<< request.getConnectionSetId() 
                <<"). Request Id(" <<request.getRequestBody().getRequestId() <<") "<< response);
        }
        else
        {
            TRACE_LOG("[CCSUtil].freeConnectivity: game("<< gameId <<"). free request succeeded. Freed 1 connection from connection set(" << request.getConnectionSetId()
                <<"). Request Id(" <<request.getRequestBody().getRequestId() <<") "<< response);
        }
    }
    else
    {
        if (deletingConnSet)
        {
            ERR_LOG("[CCSUtil].freeConnectivity: game("<< gameId <<"). free request failed due to error in CCS component. Error("<< ErrorHelp::getErrorName(rc) <<") Request "<<request <<"Response "<<errResponse);
        }
        else
        {
            TRACE_LOG("[CCSUtil].freeConnectivity: game("<< gameId <<"). Non fatal: free request failed due to error in CCS component. Error("<< ErrorHelp::getErrorName(rc) <<") Request "<<request <<"Response "<<errResponse);
            return Blaze::ERR_OK;
        }
    }

    return rc;
}

BlazeRpcError CCSUtil::extendLease(GameId gameId, const Blaze::CCS::LeaseExtensionRequest& request, Blaze::CCS::LeaseExtensionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse)
{
    TRACE_LOG("[CCSUtil].extendLease: game("<< gameId <<"). lease extension requested. "<< request);

    CCS::CCSSlave* comp = (CCS::CCSSlave*)gOutboundHttpService->getService(CCS::CCSSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        ERR_LOG("[CCSUtil].extendLease game("<< gameId <<").: request failed as CCS component not available.");
        return Blaze::ERR_SYSTEM;
    }

    BlazeRpcError rc = comp->extendLease(request,response,errResponse);
    if (rc == Blaze::ERR_OK)
    {
        // The CCS has claimed a successful response but we further error check to ensure that the response is as desired. Otherwise, missing pieces can lead to mysterious bugs or crashes.
        // These errors should be relatively rare. 
        if (request.getRequestBody().getRequestId() != response.getRequestId())
        {
            ERR_LOG("[CCSUtil].extendLease: game("<< gameId <<"). request failed due to incorrect request id in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        if (request.getRequestBody().getLeasetime() != response.getLeasetime())
        {
            ERR_LOG("[CCSUtil].extendLease: game("<< gameId <<"). request failed due to unexpected change in lease time. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        TRACE_LOG("[CCSUtil].extendLease game("<< gameId <<").: lease extension request succeeded. Request Id(" <<request.getRequestBody().getRequestId() <<") "<< response);
    }
    else
    {
        ERR_LOG("[CCSUtil].extendLease: game("<< gameId <<"). lease extension failed due to error in CCS component. Error("<< ErrorHelp::getErrorName(rc) <<" Request "<<request <<"Response "<<errResponse);
    }

    return rc;
}

BlazeRpcError CCSUtil::freeClientHostedConnections(const Blaze::CCS::FreeClientHostedConnectionsRequest& request, Blaze::CCS::FreeClientHostedConnectionsResponse& response, Blaze::CCS::CCSErrorResponse& errResponse)
{
    TRACE_LOG("[CCSUtil].freeClientHostedConnections: free all connections requested. "<< request);

    CCS::CCSSlave* comp = (CCS::CCSSlave*)gOutboundHttpService->getService(CCS::CCSSlave::COMPONENT_INFO.name);
    if (comp == nullptr)
    {
        ERR_LOG("[CCSUtil].freeClientHostedConnections: request failed as CCS component not available.");
        return Blaze::ERR_SYSTEM;
    }

    BlazeRpcError rc = comp->freeClientHostedConnections(request,response,errResponse);
    if (rc == Blaze::ERR_OK)
    {
        // The CCS has claimed a successful response but we further error check to ensure that the response is as desired. Otherwise, missing pieces can lead to mysterious bugs or crashes.
        // These errors should be relatively rare. 
        if (request.getRequestId() != response.getRequestId())
        {
            ERR_LOG("[CCSUtil].freeClientHostedConnections: request failed due to incorrect request id in response. Request "<<request <<"Response "<<response);
            return Blaze::ERR_SYSTEM;
        }

        TRACE_LOG("[CCSUtil].freeClientHostedConnections: free all connections request succeeded. Request Id(" <<request.getRequestId() <<") "<< response);
    }
    else
    {
        ERR_LOG("[CCSUtil].freeClientHostedConnections: free all connections request failed due to error in CCS component. Error("<< ErrorHelp::getErrorName(rc) <<") Request "<<request <<"Response "<<errResponse);
    }

    return rc;
}

}//GameManager
}//Blaze
