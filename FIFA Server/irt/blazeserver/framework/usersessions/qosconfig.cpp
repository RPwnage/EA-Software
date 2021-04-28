/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"
#include "framework/usersessions/qosconfig.h"
#include "framework/connection/endpoint.h"
#include "framework/controller/controller.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/tdf/qosdatatypes.h"

#include "framework/grpc/outboundgrpcmanager.h"
#include "framework/grpc/outboundgrpcjobhandlers.h"
#include "framework/oauth/accesstokenutil.h"

#include "framework/rpc/oauthslave.h"
#include "eadp/qoscoordinator/admin.grpc.pb.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
const char8_t* UNKNOWN_PINGSITE = "unknown_pingsite";
const char8_t* INVALID_PINGSITE = "invalid_pingsite";


/*** Public Methods ******************************************************************************/

QosConfig::QosConfig()
{
    mRefreshTimerId = INVALID_TIMER_ID;
    mLastRefreshProfile = "(non-empty default)";
}
QosConfig::~QosConfig()
{
    if (mRefreshTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mRefreshTimerId);
    }
}
const PingSiteInfoByAliasMap& QosConfig::getPingSiteInfoByAliasMap() const
{
    return mQosConfigInfo.getPingSiteInfoByAliasMap();
}
const QosConfigInfo& QosConfig::getQosSettings() const
{
    return mQosConfigInfo;
}

void QosConfig::validateQosSettingsConfig(const QosConfigInfo& config, ConfigureValidationErrors& validationErrors) const
{
    // qos latency information
    const PingSiteInfoByAliasMap& pSeqLatencyServers = config.getPingSiteInfoByAliasMap();
    if (pSeqLatencyServers.empty())
    {
        // This indicates that the 2.0 config should be used: 
        BLAZE_TRACE_LOG(Log::USER, "[QosConfig].validateQosSettingsConfig: Using QoS 2.0 logic since ping site info by alias map has no any latency host in qossettings.cfg config file.");

        // Address information comes from the framework.cfg, we use the service names as the qos service name by default. 
        if (config.getQosCoordinatorInfo().getProfile()[0] == '\0')
        {
            eastl::string msg;
            msg.sprintf("[QosConfig].validateQosSettingsConfig: QoS profile name is missing in configuration file.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }
    else
    {
        // We don't support 1.0 + 2.0 configs:
        if (config.getQosCoordinatorInfo().getProfile()[0] != '\0')
        {
            eastl::string msg;
            msg.sprintf("[QosConfig].validateQosSettingsConfig: QoS profile name provided in configuration file along with PingSiteInfo map (QoS 1.0).  Use one or the other, not both.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        BLAZE_WARN_LOG(Log::USER, "[QosConfig].validateQosSettingsConfig: Using deprecated QoS 1.0 logic since ping site info by alias map was provided in qossettings.cfg config file.");

        for (PingSiteInfoByAliasMap::const_iterator it = pSeqLatencyServers.begin(); it != pSeqLatencyServers.end(); it++)
        {
            // make sure the alias is not empty
            const char8_t* psAliasStr = (*it).first.c_str();
            if (psAliasStr[0] == '\0')
            {
                eastl::string msg;
                msg.sprintf("[QosConfig].validateQosSettingsConfig: QOS latency server's alias is missing in configuration file.");
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            // if the dc name length is over the size, config fails
            if (strlen(psAliasStr) >= Blaze::MAX_PINGSITEALIAS_LEN)
            {
                eastl::string msg;
                msg.sprintf("[QosConfig].validateQosSettingsConfig: QOS latency server's alias: %s is too long, over the size: %d.",
                    psAliasStr, Blaze::MAX_PINGSITEALIAS_LEN - 1);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }

            // qos server address
            const char8_t* addressStr = (*it).second->getAddress();
            // validate the address, make sure it's not empty
            if (addressStr[0] == '\0')
            {
                eastl::string msg;
                msg.sprintf("[QosConfig].validateQosSettingsConfig: QOS latency server address is missing in configuration file for alias: %s.",
                    addressStr);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }

            // port
            uint16_t latencyServerPort = (*it).second->getPort();
            if (latencyServerPort == 0)
            {
                eastl::string msg;
                msg.sprintf("[QosConfig].validateQosSettingsConfig: Qos Server port for data center: %s in qossettings.cfg is invalid.",
                    psAliasStr);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }
}
bool QosConfig::isQosVersion1()
{
    // If the PingSiteInfoByAliasMap was set in the config file, then it's qos 1.0:
    auto& qosSettings = gUserSessionManager->getConfig().getQosSettings();
    return (!qosSettings.getPingSiteInfoByAliasMap().empty());
}

void QosConfig::configure(const QosConfigInfo& config)
{
    auto& qosSettings = gUserSessionManager->getConfig().getQosSettings();
    if (isQosVersion1())
    {
        // (QoS 1.0) If the alias map came from the config, just update the values:
        qosSettings.copyInto(mQosConfigInfo);
    }
    else
    {
        // (QoS 2.0) We get the alias map from the server, so don't use any of the values in the config:
        PingSiteInfoByAliasMap tempAliasMap;
        getPingSiteInfoByAliasMap().copyInto(tempAliasMap);
        qosSettings.copyInto(mQosConfigInfo);
        tempAliasMap.copyInto(mQosConfigInfo.getPingSiteInfoByAliasMap());

        attemptRefreshQosPingSiteMap(true);
    }
}

bool QosConfig::attemptRefreshQosPingSiteMap(bool configChange)
{
    // Return False for QoS 1.0 configs, since the refresh is not supported there:
    if (isQosVersion1())
    {
        return false;
    }

    // Refresh in progress - ignore 
    if (mRefreshTimerId != INVALID_TIMER_ID)
        return true;

    // Recent refresh, 
    TimeValue refreshStartDelay;

    if (mLastRefreshTime == 0 || configChange)
    {
        refreshStartDelay = 0;
    }
    else
    {
        TimeValue timeSinceLastRefresh = TimeValue::getTimeOfDay() - mLastRefreshTime;
        if (timeSinceLastRefresh < getQosSettings().getServerQosRefreshRate())
            refreshStartDelay = getQosSettings().getServerQosRefreshRate() - timeSinceLastRefresh;
    }

    // Grab the info from the qos server:
    mRefreshTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + refreshStartDelay, this, &QosConfig::refreshQosPingSiteMap, "QosConfig::refreshQosPingSiteMap");
    return true;
}

void QosConfig::refreshQosPingSiteMap()
{
    mLastRefreshTime = TimeValue::getTimeOfDay();

    auto& qosSettings = getQosSettings();

    // Wait for the OAuthSlave to init: 
    int loopCount = 0;
    Blaze::BlazeRpcError waitErr = ERR_OK;
    do 
    {
        ++loopCount;
        OAuth::OAuthSlave *auth = static_cast<OAuth::OAuthSlave*>(gController->getComponent(OAuth::OAuthSlave::COMPONENT_ID, false));
        if (auth != nullptr)
            break;
        waitErr = Fiber::sleep(TimeValue(10000), "WaitingForOAuthSlave");
    } while (waitErr == ERR_OK);

    // Connection Name, Module, RPC
    auto qosPingSiteRpc = CREATE_GRPC_UNARY_RPC("qoscoordinator", eadp::qoscoordinator::QOSCoordinatorAdmin, AsyncAvailablePingSites);


    if (qosPingSiteRpc == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: QoS Coordinator was unable to create an outbound gRPC, despite being enabled.  Please ensure that your framework.cfg contains the qoscoordinator outboundgrpc config.");
        mRefreshTimerId = INVALID_TIMER_ID;
        return;
    }

    OAuth::AccessTokenUtil util;
    UserSession::SuperUserPermissionAutoPtr ptr(true);
    BlazeRpcError error = util.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NEXUS_S2S);
    if (error == ERR_OK)
    {
        qosPingSiteRpc->getClientContext().AddMetadata("authorization", util.getAccessToken());
        
        Blaze::Grpc::ResponsePtr responsePtr;
        eadp::qoscoordinator::AvailablePingSitesRequest request;
        const char8_t* profileName = qosSettings.getQosCoordinatorInfo().getProfile();
        *request.mutable_profilename() = profileName;


        error = qosPingSiteRpc->sendRequest(&request, responsePtr);

        if (error != ERR_OK)
        {
            const GrpcServiceConnectionInfo* connInfo = gController->getOutboundGrpcManager()->getServiceInfo("qoscoordinator");
            BLAZE_ERR_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: Failed to get QoS information for profile: " << profileName << " (Request failed with error "<< error << " )");
            BLAZE_ERR_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: Make sure that you have decypted versions of your SSL keys, that your client id has the 'gs_qos_coordinator_trusted' scope, " 
                << "and that your machine can connect to the QoS coordinator at " << (connInfo ? connInfo->getServiceAddress() : "unknown") << "." );
        }
        else
        {
            eadp::qoscoordinator::AvailablePingSitesResponse* response = static_cast<eadp::qoscoordinator::AvailablePingSitesResponse*>(responsePtr.get());

            if (response->status().code() != 0) // Okay 
            {
                // Error occurred.  We can try again, but that might fail too. 
                BLAZE_ERR_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: Failed to get QoS information for profile: " << profileName <<
                    " Code: " << response->status().code() << " Message: " << response->status().message().c_str());
                mRefreshTimerId = INVALID_TIMER_ID;
                return;
            }

            if (blaze_stricmp(response->selectedqosprofile().c_str(), profileName) != 0)
            {
                // avoid spamming if the same profile mismatch is detected every time
                if (mLastRefreshProfile != response->selectedqosprofile().c_str())
                {
                    BLAZE_WARN_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: The desired profile: " << profileName <<
                        " was not found on the qos server.  Using profile: " << response->selectedqosprofile().c_str() << " instead.  Note: This is not a catastrophic error, and does not prevent QoS usage.");
                }
			}

            mLastRefreshProfile = response->selectedqosprofile().c_str(); 
            bool pingSiteMapChanged = false;

            // Check the ping sites that were provided:
            if (response->pingsites().size() == getPingSiteInfoByAliasMap().size())
            {
                // Check each value: 
                for (auto& curPingSite : response->pingsites())
                {
                    auto iter = getPingSiteInfoByAliasMap().find(curPingSite.sitename().c_str());
                    if (iter == getPingSiteInfoByAliasMap().end())
                    {
                        BLAZE_INFO_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: Pingsite - '" << curPingSite.sitename().c_str() << "' address (" << curPingSite.ip().c_str() << ") is new, triggering update.");
                        pingSiteMapChanged = true;
                        break;
                    }
                }
            }
            else
            {
                // Don't clear the ping site map.  That's not a valid use case.
                pingSiteMapChanged = (response->pingsites().size() != 0);
            }

            // Update the map as needed:
            if (pingSiteMapChanged)
            {
                PingSiteInfoByAliasMap& pingSiteAliasMap = mQosConfigInfo.getPingSiteInfoByAliasMap();

                eastl_size_t numPingSitesInMap = pingSiteAliasMap.size();
                pingSiteAliasMap.clear();
                for (auto& curPingSite : response->pingsites())
                {
                    // The Map doesn't need to store any info, we just want the names of the sites:
                    auto pingSite = pingSiteAliasMap.allocate_element();
                    pingSite->setAddress(curPingSite.ip().c_str());
                    pingSite->setPort(qosSettings.getQosCoordinatorInfo().getDefaultPingSitesPort());  // Defaults to 4001
                    pingSiteAliasMap[curPingSite.sitename().c_str()] = pingSite;

                    BLAZE_TRACE_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: Pingsite - '" << curPingSite.sitename().c_str() << "' ip: " << curPingSite.ip().c_str());
                }

                BLAZE_INFO_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: Updating ping site map from " << numPingSitesInMap << " to " << getPingSiteInfoByAliasMap().size() << 
                                          " ping sites using profile: " << profileName << " (" << response->selectedqosprofile().c_str() << ") " );
            }
            else
            {
                if (getPingSiteInfoByAliasMap().empty())
                {
                    BLAZE_ERR_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: No ping sites were found for profile: " << profileName << " (" << response->selectedqosprofile().c_str() << ") " << 
                                             "  Map still has "<< getPingSiteInfoByAliasMap().size() << " ping sites set since last successful refresh." );
                }
                else
                {
                    BLAZE_WARN_LOG(Log::USER, "[QosConfig].refreshQosPingSiteMap: Ping sites were not changed from last update for profile: " << profileName << " (" << response->selectedqosprofile().c_str() << ") ");
                }
            }

            // Inform the other systems that the ping sites have changed, and tell the other servers to update their ping sites if needed
            // Technically, only one server needs to do this command, and the rest can be updated from the data it received, but it's more consistent to always get the data directly from the coordinator.
            // (and use the timer to prevent rapid QoS ping site changes)
            if (pingSiteMapChanged)
            {
                // Tell internal systems: 
                mDispatcher.dispatch<const PingSiteInfoByAliasMap&>(&PingSiteUpdateSubscriber::onPingSiteMapChanges, getPingSiteInfoByAliasMap());

                // Tell all the other sessions to refresh:
                Component::InstanceIdList umInstances;
                gUserSessionManager->getComponentInstances(umInstances, true);
                Component::MultiRequestResponseHelper<void, void> responses(umInstances, false);
                Blaze::RefreshQosPingSiteMapRequest pingSiteRequest;
                getPingSiteInfoByAliasMap().copyInto(pingSiteRequest.getPingSiteAliasMap());
                gUserSessionManager->sendMultiRequest(Blaze::UserSessionsSlave::CMD_INFO_REFRESHQOSPINGSITEMAP, &pingSiteRequest, responses);
            }
        }
    }

    mRefreshTimerId = INVALID_TIMER_ID;

    // Special case:  If we didn't get a response, and we still don't have any qos sites, schedule another attempt.
    if (getPingSiteInfoByAliasMap().empty())
    {
        attemptRefreshQosPingSiteMap();
    }
}

void QosConfig::getQosPingSites(QosPingSitesResponse& response)
{
    response.setProfile(getQosSettings().getQosCoordinatorInfo().getProfile());
    getPingSiteInfoByAliasMap().copyInto(response.getPingSiteInfoByAliasMap());
}
/*!*********************************************************************************/
/*! \brief check if the passed in alias is a valid alias we have in our config file

    Note: the check is case insensitive

    \param[in] pingSiteAlias - ping site alias to be checked
    \return true if it's a valid alias, otherwise false
************************************************************************************/
bool QosConfig::isKnownPingSiteAlias(const char8_t* pingSiteAlias) const
{
    PingSiteAlias alias(pingSiteAlias);
    if (getPingSiteInfoByAliasMap().find(alias) != getPingSiteInfoByAliasMap().end())
    {
        return true;
    }

    return false;
}

bool QosConfig::isAcceptablePingSiteAliasName(const char8_t* pingSiteAlias) const
{
    // If it's one of the configured values, we don't do the alphanum check:
    if (isKnownPingSiteAlias(pingSiteAlias))
        return true;

    // Same check as game mode: 
    auto strLength = strlen(pingSiteAlias);
    for (auto i = 0; i < strLength; ++i)
    {
        if (!(isalnum(pingSiteAlias[i]) || pingSiteAlias[i] == '-' || pingSiteAlias[i] == '_' || pingSiteAlias[i] == ':'))
        {
            return false;
        }
    }

    return true;
}

/*!*********************************************************************************/
/*! \brief Makes sure the passed in map has values for the correct set of ping sites.

\param[in] map - The map to examine.
\return true if the set of ping site aliases in the map matches the configured set of aliases; false otherwise
************************************************************************************/
bool QosConfig::validateAndCleanupPingSiteAliasMap(PingSiteLatencyByAliasMap &map) const
{
    bool allPingsitesWereKnown = true;

    // First validate all supplied pingsites
    for (Blaze::PingSiteLatencyByAliasMap::iterator itMap = map.begin(); itMap != map.end();)
    {
        if (isKnownPingSiteAlias(itMap->first.c_str()))
        {
            ++itMap;
        }
        else
        {
            BLAZE_WARN_LOG(Log::USER, "QOSConfig.validatePingSiteAliasMap validation failed for ping site alias:" << itMap->first.c_str());
            allPingsitesWereKnown = false;

            if (!isAcceptablePingSiteAliasName(itMap->first.c_str()))
            {
                BLAZE_WARN_LOG(Log::USER, "[QosConfig].isAcceptablePingSiteAliasName: Non alpha numeric character other than - _ : found in the pingsite name.  "
                            << "Removing the pingsite for metrics purpose.  The specified pingsite is (" << itMap->first.c_str() << ").");

                itMap = map.erase(itMap);
            }
            else
            {
                ++itMap;
            }
        }
    }

    if (map.size() != getPingSiteInfoByAliasMap().size())
    {
        BLAZE_WARN_LOG(Log::USER, "QOSConfig.number of latency(" << (int32_t)map.size()
            << ") in map does not match number of alias(" << (int32_t)getPingSiteInfoByAliasMap().size() << ") in qossettings.cfg");
        return false;
    }

    return allPingsitesWereKnown;
}

bool QosConfig::validatePingSiteAliasMap(const PingSiteInfoByAliasMap &map)
{
    if (getPingSiteInfoByAliasMap().empty() && !map.empty())
    {
        // If we don't have any alias map, and the validation request here has one, then assume that it's useable: 
        // (This helps prevent an issue where only one server is unable to communicate with the QoS server).
        map.copyInto(mQosConfigInfo.getPingSiteInfoByAliasMap());
        return false;
    }

    // Just check for equality: 
    if (map.size() != getPingSiteInfoByAliasMap().size())
        return false;

    for (auto& iter : map)
    {
        auto iter2 = getPingSiteInfoByAliasMap().find(iter.first);
        if (iter2 == getPingSiteInfoByAliasMap().end())
            return false;
    }

    return true;
}

// helper function
PingSiteAlias QosConfig::pickNextBestQosPingSite(const PingSiteLatencyByAliasMap& latencyMap)
{
    PingSiteAlias bestAlias;
    PingSiteLatencyByAliasMap::const_iterator iter = latencyMap.begin();
    PingSiteLatencyByAliasMap::const_iterator itend = latencyMap.end();

    PingSiteLatencyByAliasMap::const_iterator itBest = iter;
    for (; iter != itend; ++iter)
    {
        if (iter->second < itBest->second)
            itBest = iter;
    }

    if (itBest)
        bestAlias = itBest->first;

    return bestAlias;
}

} //namespace Blaze

