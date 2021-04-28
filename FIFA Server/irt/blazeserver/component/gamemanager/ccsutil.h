/*! ************************************************************************************************/
/*!
    \file ccsutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_CCS_UTIL_H 
#define BLAZE_CCS_UTIL_H

#include "framework/tdf/userdefines.h"
#include "EASTL/string.h"
#include "proxycomponent/ccs/tdf/ccs.h"
#include "gamemanager/tdf/gamemanager_server.h"

namespace Blaze
{
class NetworkAddress;

namespace GameManager
{
class PlayerInfoMaster;
class GameSessionMaster;


/*!************************************************************************************************************************************/
/*! \brief This class is a collection of utility methods to interact with the CC REST service. The object can be instantiated on both 
            master and slave instances. The methods called on the master generally build the requests due to its authority on the data
            and the methods called on the slave interact directly with the service (http calls). The master is never blocked while 
            interacting with CCS.
***************************************************************************************************************************************/ 
class CCSUtil
{
    NON_COPYABLE(CCSUtil);
public:
    CCSUtil();
    ~CCSUtil();
    
    /*!************************************************************************************************/
    /*! \brief Returns the client id for the local server instance
    ***************************************************************************************************/
    eastl::string getClientId() const;

    /*!************************************************************************************************/
    /*! \brief call the CCS REST endpoint to allocate the connectivity between console pairs identified in the request.
    ***************************************************************************************************/
    // DEPRECATED - Use GetHostedConsoleConnectionRequest version. 
    BlazeRpcError allocateConnectivity(GameId gameId, const Blaze::CCS::GetHostedConnectionRequest& request, Blaze::CCS::GetHostedConnectionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse);
    BlazeRpcError allocateConnectivity(GameId gameId, const Blaze::CCS::GetHostedConsoleConnectionRequest& request, Blaze::CCS::GetHostedConnectionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse);
    
    /*!************************************************************************************************/
    /*! \brief call the CCS REST endpoint to free the connectivity between console pairs identified in the request.
    ***************************************************************************************************/
    BlazeRpcError freeConnectivity(GameId gameId, const Blaze::CCS::FreeHostedConnectionRequest& request, Blaze::CCS::FreeHostedConnectionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse);
    
    /*!************************************************************************************************/
    /*! \brief call the CCS REST endpoint to extend the lease of the connection set identified in the request.
    ***************************************************************************************************/
    BlazeRpcError extendLease(GameId gameId, const Blaze::CCS::LeaseExtensionRequest& request, Blaze::CCS::LeaseExtensionResponse& response, Blaze::CCS::CCSErrorResponse& errResponse);

    /*!************************************************************************************************/
    /*! \brief call the CCS REST endpoint to free the hosted connections for this client. 
    ***************************************************************************************************/
    BlazeRpcError freeClientHostedConnections(const Blaze::CCS::FreeClientHostedConnectionsRequest& request, Blaze::CCS::FreeClientHostedConnectionsResponse& response, Blaze::CCS::CCSErrorResponse& errResponse);
    
    /*!************************************************************************************************/
    /*! \brief Build the hosted connection allocation request partially using the data from the master.
        The blocking part (acquiring nucleus token) of the request is built on the slave.
    ***************************************************************************************************/
    BlazeRpcError buildGetHostedConnectionRequest(const GameManager::GameSessionMaster& gameSessionMaster, CCSAllocateRequest& ccsAllocateRequest) const;
    
    /*!************************************************************************************************/
    /*! \brief Build the hosted connection free request partially using the data from the master.
        The blocking part (acquiring nucleus token) of the request is built on the slave.
    ***************************************************************************************************/
    BlazeRpcError buildFreeHostedConnectionRequest(const GameManager::GameSessionMaster& gameSessionMaster, CCSFreeRequest& ccsFreeRequest, uint32_t console1Id = INVALID_CCS_CONNECTIVITY_ID, uint32_t console2Id = INVALID_CCS_CONNECTIVITY_ID);
    
    /*!************************************************************************************************/
    /*! \brief Build the lease extension request partially using the data from the master.
        The blocking part (acquiring nucleus token) of the request is built on the slave.
    ***************************************************************************************************/
    BlazeRpcError buildLeaseExtensionRequest(const GameManager::GameSessionMaster& gameSessionMaster, CCSLeaseExtensionRequest& ccsLeaseExtensionRequest) const;

    /*!************************************************************************************************/
    /*! \brief Acquire the nucleus token that will be sent to the CCS REST endpoint as part of our request.
    ***************************************************************************************************/
    BlazeRpcError getNucleusAccessToken(eastl::string& accessToken);
private:
    void generateTunnelKey(const GameManager::GameSessionMaster& gameSessionMaster, char* tunnelKey, size_t tunnelKeySize, const UUID& gameUUID, ConnectionGroupId consoleConnGroupId) const;
    const char8_t* getProtoTunnelVer(const GameManager::GameSessionMaster& gameSessionMaster, ConnectionGroupId consoleConnGroupId) const;
    // DEPRECATED - Use V2 version. 
    void fillConsoleConnectionInfo(const GameManager::GameSessionMaster& gameSessionMaster, CCS::ConnectionInfo* cInfo, ConnectionGroupId consoleConnGroupId, SlotId consoleSlotId) const;
    int32_t fillConsoleConnectionInfoV2(const GameManager::GameSessionMaster& gameSessionMaster, CCS::Consoles& consoles, ConnectionGroupId consoleConnGroupId, SlotId consoleSlotId, const char8_t* consoleIpAddress, const PingSiteLatencyByAliasMap& pingSiteInfo) const;
    SlotId getSlotId(const GameManager::GameSessionMaster& gameSessionMaster, ConnectionGroupId connGrpId) const;
    eastl::string getDataCenter(const GameManager::GameSessionMaster& gameSessionMaster) const;
    const PlayerInfoMaster* getAnyPlayerInConnectionGroup(const GameManager::GameSessionMaster& gameSessionMaster, ConnectionGroupId consoleConnGroupId) const;

};

}
}

#endif
