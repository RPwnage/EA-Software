/*************************************************************************************************/
/*!
    \file   updatepingsitelatency_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdatePingSiteLatencyCommand 

    process user session extended data retrieval requests from the client.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
//user includes
#include "framework/usersessions/usersessionmanager.h"
#include "framework/rpc/usersessionsslave/updatenetworkinfo_stub.h"
#include "framework/rpc/usersessionsslave/refreshqospingsitemap_stub.h"
#include "framework/rpc/usersessionsslave/getqospingsites_stub.h"
#include "framework/tdf/qosdatatypes.h"
#include "framework/util/connectutil.h"

namespace Blaze
{
    class UpdateNetworkInfoCommand : public UpdateNetworkInfoCommandStub
    {
    public:

        UpdateNetworkInfoCommand(Message* message, Blaze::UpdateNetworkInfoRequest* request, UserSessionsSlave* componentImpl)
            : UpdateNetworkInfoCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~UpdateNetworkInfoCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        UpdateNetworkInfoCommandStub::Errors execute() override
        {
            if(EA_UNLIKELY(gUserSessionManager == nullptr))
            {                
                return ERR_SYSTEM;
            }

            NetworkInfo& info = mRequest.getNetworkInfo();

            // GDK versions of the SDK do not have Xboxone addresses set. 
            if (gController->getXboxOneRequiresXboxClientAddress())
            {
                // Mock PC use non Xbox addresses for faked Xbox clients.  This may break some connection logic, but mock services can't support everything.
                if (!gController->getUseMockConsoleServices() && (gCurrentLocalUserSession->getClientType() == CLIENT_TYPE_GAMEPLAY_USER || gCurrentLocalUserSession->getClientType() == CLIENT_TYPE_LIMITED_GAMEPLAY_USER))
                {
                    const char8_t* serviceName = getPeerInfo()->getServiceName();
                    ClientPlatformType serviceNamePlatform = gController->getServicePlatform(serviceName);

                    // prevent bad client behavior that can result from non Xbox One addresses being faked for Xbox One clients
                    const ClientInfo* clientInfo = ((gCurrentLocalUserSession->getPeerInfo() != nullptr) ? gCurrentLocalUserSession->getPeerInfo()->getClientInfo() : nullptr);
                    bool runningOnXboxOne = (clientInfo != nullptr) && (clientInfo->getPlatform() == xone || (clientInfo->getPlatform() == NATIVE && serviceNamePlatform == xone));
                    bool networkAddressSet = info.getAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET;
                    bool xboxOneNetworkAddress = info.getAddress().getActiveMember() == NetworkAddress::MEMBER_XBOXCLIENTADDRESS;
                    if (networkAddressSet && (clientInfo != nullptr) && (runningOnXboxOne != xboxOneNetworkAddress))
                    {
                        BLAZE_WARN_LOG(Log::USER, "[UpdateNetworkInfoCommand] User(" << gCurrentLocalUserSession->getBlazeId() << ", session id " << gCurrentLocalUserSession->getId()
                            << ") attempted to use address type(" << info.getAddress().getActiveMember() << ") which is not compatible with its connected client platform type("
                            << (clientInfo ? ClientPlatformTypeToString(gCurrentLocalUserSession->getPeerInfo()->getClientInfo()->getPlatform()) : "missing client info") << ")." 
                            << " If the SDK was compiled with the xb1gdk, set XboxOneRequiresXboxClientAddress to false to skip this check. (CCS will fail otherwise).");
                        return ERR_SYSTEM;
                    }
                }
            }

            if (info.getAddress().getActiveMember()==NetworkAddress::MEMBER_IPPAIRADDRESS)
            {
                if (info.getAddress().getIpPairAddress()->getExternalAddress().getIp()==0)
                {
                    BLAZE_TRACE_LOG(Log::USER, "[UpdateNetworkInfoCommand] Client doesn't know its external address. Use connection address(" 
                                   << (getPeerInfo()->getPeerAddress().getIp(InetAddress::HOST)) << ")");
                    ConnectUtil::setExternalAddr(info.getAddress(), getPeerInfo()->getPeerAddress());
                }
            }
            UpdateNetworkInfoMasterRequest request;
            request.setSessionId(gCurrentUserSession->getSessionId());
            request.setNetworkInfo(info);
            request.getOpts().setBits(mRequest.getOpts().getBits());
            return (commandErrorFromBlazeError(gUserSessionManager->getMaster()->updateNetworkInfoMaster(request)));
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_UPDATENETWORKINFO_CREATE_COMPNAME(UserSessionManager)

    class RefreshQosPingSiteMapCommand : public RefreshQosPingSiteMapCommandStub
    {
    public:

        RefreshQosPingSiteMapCommand(Message* message, Blaze::RefreshQosPingSiteMapRequest* request, UserSessionsSlave* componentImpl)
            : RefreshQosPingSiteMapCommandStub(message, request),
            mComponent(componentImpl)
        {
        }

        ~RefreshQosPingSiteMapCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        RefreshQosPingSiteMapCommandStub::Errors execute() override
        {
            if (EA_UNLIKELY(gUserSessionManager == nullptr))
            {
                return ERR_SYSTEM;
            }

            if (gUserSessionManager->getQosConfig().validatePingSiteAliasMap(mRequest.getPingSiteAliasMap()))
            {
                BLAZE_TRACE_LOG(Log::USER, "[RefreshQosPingSiteMapCommand] PingSiteInfo is already up to date.");
                return ERR_OK;
            }

            if (gUserSessionManager->getQosConfig().attemptRefreshQosPingSiteMap() == false)
            {
                BLAZE_WARN_LOG(Log::USER, "[RefreshQosPingSiteMapCommand] Attempted to refresh QoS info on a 1.0 server. (Only supported on QoS 2.0 servers.) No op.");
                return ERR_SYSTEM;
            }

            return ERR_OK;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_REFRESHQOSPINGSITEMAP_CREATE_COMPNAME(UserSessionManager)

    class GetQosPingSitesCommand : public GetQosPingSitesCommandStub
    {
    public:

        GetQosPingSitesCommand(Message* message, UserSessionsSlave* componentImpl)
            : GetQosPingSitesCommandStub(message),
            mComponent(componentImpl)
        {
        }

        ~GetQosPingSitesCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetQosPingSitesCommandStub::Errors execute() override
        {
            if (EA_UNLIKELY(gUserSessionManager == nullptr))
            {
                return ERR_SYSTEM;
            }

            gUserSessionManager->getQosConfig().getQosPingSites(mResponse);
            
            return ERR_OK;
        }

    private:
        UserSessionsSlave* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETQOSPINGSITES_CREATE_COMPNAME(UserSessionManager)

} // Blaze
