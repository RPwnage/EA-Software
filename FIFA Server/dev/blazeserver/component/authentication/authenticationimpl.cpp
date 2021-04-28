#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/util/hashutil.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/oauth/accesstokenutil.h"

// Authentication
#include "authenticationimpl.h"

// Authentication utils
#include "authentication/util/authenticationutil.h"
#include "authentication/util/checkentitlementutil.h"
#include "authentication/util/getlegaldocutil.h"

// Nucleus handlers
#include "authentication/nucleushandler/nucleusgetlegaldocuri.h"

#include "proxycomponent/nucleusconnect/rpc/nucleusconnectslave.h"
#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"

namespace Blaze
{

namespace Metrics
{
namespace Tag
{
TagInfo<Authentication::EntitlementCheckType>* entitlement_check_type = BLAZE_NEW TagInfo<Authentication::EntitlementCheckType>("entitlement_check_type", [](const Authentication::EntitlementCheckType& value, TagValue&) { return Authentication::EntitlementCheckTypeToString(value); });
TagInfo<ProjectId>* project_id = BLAZE_NEW TagInfo<ProjectId>("project_id");
TagInfo<OnlineAccessEntitlementGroup>* online_access_entitlement_group = BLAZE_NEW TagInfo<OnlineAccessEntitlementGroup>("online_access_entitlement_group");
}
}

namespace Authentication
{

const char8_t* AuthenticationSlaveImpl::sPoolNames[ConnectionManagerType::MAX] =
{
    "TosConnMgr"
};

// static
AuthenticationSlave* AuthenticationSlave::createImpl()
{
    return BLAZE_NEW_NAMED("AuthenticationSlaveImpl") AuthenticationSlaveImpl();
}


/*** Public Methods ******************************************************************************/
AuthenticationSlaveImpl::AuthenticationSlaveImpl()
  : mBypassNucleusEnabled(false), mCheckLegalDocId(INVALID_TIMER_ID),
    mStopCheckLegalDoc(false), mTosServiceDown(false),
    mMetricsInfo(getMetricsCollection()),
    mEntitlementStatusCheckTimerId(INVALID_TIMER_ID),
    mEntitlementStatusCheckInProgress(false),
    mBackgroundJobQueue("AuthenticationSlaveImpl::mBackgroundJobQueue")
{
    mBackgroundJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);
}

AuthenticationSlaveImpl::~AuthenticationSlaveImpl()
{
    clearProductEntitlements();
    clearPsuCutoffProductMetrics();
}

void AuthenticationSlaveImpl::clearPsuCutoffProductMetrics()
{
    mMetricsInfo.mPsuCutoff.reset();
}

bool AuthenticationSlaveImpl::getAllowStressLogin() const
{
    return getConfig().getAllowStressLogin();
}

bool AuthenticationSlaveImpl::getAutoEntitlementEnabled(const char8_t* productName) const
{
    const AccessList* accessList = getAccessList(productName);
    if (accessList != nullptr)
        return accessList->front()->getAutoEntitlementEnabled();
    return false;
}

bool AuthenticationSlaveImpl::getAllowUnderage(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
        return productItr->second->getAllowUnderage();
    return false;
}

bool AuthenticationSlaveImpl::getNeedCheckEntitlement(ClientType ctype, EntitlementCheckType entType, const char8_t* productName, bool& softCheck) const
{
    const AccessList* accessList = getAccessList(productName);
    if (accessList != nullptr)
    {
        AccessList::const_iterator accessItr = accessList->begin();
        for (; accessItr != accessList->end(); ++accessItr)
        {
            const Access& access = **accessItr;
            if (access.getType() == entType)
            {
                ClientTypeList::const_iterator ctypeItr = access.getEnforcedClientTypes().begin();
                for (; ctypeItr != access.getEnforcedClientTypes().end(); ++ctypeItr)
                {
                    ClientType clientType = *ctypeItr;
                    if (clientType == ctype)
                    {
                        softCheck = access.getSoftCheck();
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

const char8_t* AuthenticationSlaveImpl::getProductId(const char8_t* productName) const
{
    const AccessList* accessList = getAccessList(productName);
    if (accessList != nullptr)
        return accessList->front()->getProductId();
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getOnlineAccessEntitlementGroup(const char8_t* productName) const
{
    const AccessList* accessList = getAccessList(productName);
    if (accessList != nullptr)
        return accessList->front()->getOnlineAccessEntitlementGroup();
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getProjectId(const char8_t* productName) const
{
    const AccessList* accessList = getAccessList(productName);
    if (accessList != nullptr)
        return accessList->front()->getProjectId();
    return nullptr;
}

const TrialContentIdList* AuthenticationSlaveImpl::getTrialContentIdList(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
        return &productItr->second->getTrialContentIds();
    return nullptr;

}

const char8_t* AuthenticationSlaveImpl::getRegistrationSource(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
        return productItr->second->getRegistrationSource();
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getAuthenticationSource(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
        return productItr->second->getAuthenticationSource();
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getNucleusSourceHeader(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
        return productItr->second->getNucleusSourceHeader();
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getEntitlementSource(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
        return productItr->second->getEntitlementSource();
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getEntitlementTag(const char8_t* productName) const
{
    const AccessList* accessList = getAccessList(productName);
    if (accessList != nullptr)
        return accessList->front()->getEntitlementTag();
    return nullptr;
}

const UnderageLoginOverride* AuthenticationSlaveImpl::getUnderageLoginOverride(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
    {
        if (productItr->second->getAllowUnderage())
        {
            // Don't need an override if under age users are permitted
            return nullptr;
        }

        const UnderageLoginOverride& underageOverride = productItr->second->getUnderageLoginOverride();
        const char8_t* tag = underageOverride.getEntitlementTag();
        const char8_t* group = underageOverride.getEntitlementGroup();
        const char8_t* projectId = underageOverride.getProjectId();
        if ((tag[0] == '\0') || (group[0] == '\0') || (projectId[0] == '\0'))
        {
            // No override is configured
            return nullptr;
        }

        return &underageOverride;
    }
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getLegalDocServer() const
{
    HttpConnectionManagerPtr tosConnMgr = gOutboundHttpService->getConnection(sPoolNames[TOS]);
    if (tosConnMgr == nullptr)
        return nullptr;
    return tosConnMgr->getAddress().getHostname();
}

const char8_t* AuthenticationSlaveImpl::getLegalDocGameIdentifier() const
{
    return getConfig().getLegalDocGameIdentifier();
}

ClientPlatformType AuthenticationSlaveImpl::getPlatform(const Command& callingCommand) const
{
    if (callingCommand.getPeerInfo() != nullptr && callingCommand.getPeerInfo()->getClientInfo() != nullptr)
    {
        return callingCommand.getPeerInfo()->getClientInfo()->getPlatform();
    }
    else if (callingCommand.getPeerInfo() != nullptr)
    {
        return gController->getServicePlatform(callingCommand.getPeerInfo()->getServiceName());
    }
    else
    {
        WARN_LOG("[AuthenticationSlaveImpl].getPlatform: Missing peer info, using common service platform ("<< ClientPlatformTypeToString(gController->getDefaultClientPlatform()) <<").");
        return gController->getDefaultClientPlatform();
    }
}

ClientPlatformType AuthenticationSlaveImpl::getProductPlatform(const char8_t* productName) const
{
    // First, try to find the productName in the serviceNames boot config
    const ServiceNameInfoMap& hostedServicesInfo = gController->getHostedServicesInfo();
    for (const auto& serviceNamePair : hostedServicesInfo)
    {
        if (blaze_strcmp(serviceNamePair.second->getProductName(), productName) == 0)
        {
            // Assumption: a productName will only be configured for one platform
            // (e.g. -pc and -steam service names do not share a productName)
            return serviceNamePair.second->getPlatform();
        }
    }

    // If the productName is not configured for a serviceName, then try looking up via projectId
    const char8_t* projectId = getProjectId(productName);
    TRACE_LOG("[AuthenticationSlaveImpl].getProductPlatform: productName (" << productName << ") is not configured for a serviceName; attempting to look up by projectId (" << projectId << ").");

    ClientPlatformTypeList projectPlatforms;
    gUserSessionManager->getProjectIdPlatforms(projectId, projectPlatforms);

    if (projectPlatforms.size() == 1)
    {
        return projectPlatforms[0];
    }
    else if (projectPlatforms.empty())
    {
        INFO_LOG("[AuthenticationSlaveImpl].getProductPlatform: No PIN platforms configured for projectId (" << projectId << ") / productName (" << productName << ").");
    }
    else
    {
        // PC and Steam platforms typically share the same project id.
        // Titles with that setup will likely need to configure a serviceName for each productName if using EADP Service Mesh.
        INFO_LOG("[AuthenticationSlaveImpl].getProductPlatform: Multiple PIN platforms configured for projectId (" << projectId << ") / productName (" << productName << ").");
    }

    WARN_LOG("[AuthenticationSlaveImpl].getProductPlatform: Unable to determine the platform for productName (" << productName << ").");
    return INVALID;
}

const char8_t* AuthenticationSlaveImpl::getTOSPlatformStringFromClientPlatformType(const ClientPlatformType clientPlatformType, const char8_t* contentType)
{
    const char8_t* result = "";

    switch(clientPlatformType)
    {
    case INVALID:
        result = "invalid";
        break;
    case pc:
    case android:
    case ios:
        // pc2 was created as a plain text version of the TOS, since asking for "Content-Type: text/plain" would still result in wonky formating.
        if (blaze_stricmp(contentType, "PLAIN") == 0)
            result = "pc2";
        else
            result = "pc";
        break;
    case common:
        result = "common";
        break;
    case mobile:
        result = "mobile";
        break;
    case legacyprofileid:
        result = "legacyprofileid";
        break;
    case verizon:
        result = "verizon";
        break;
    case facebook:
        result = "facebook";
        break;
    case facebook_eacom:
        result = "facebook_eacom";
        break;
    case bebo:
        result = "bebo";
        break;
    case friendster:
        result = "friendster";
        break;
    case twitter:
        result = "twitter";
        break;
    case xone:
        result = "xone";
        break;
    case ps4:
        result = "ps4";
        break;
    case nx:
        result = "nx";
        break;
    default:
        result = ClientPlatformTypeToString(clientPlatformType);
        break;
    }

    return result;
}

const AccessList* AuthenticationSlaveImpl::getAccessList(const char8_t* productName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
        return &(productItr->second->getAccess());
    return nullptr;
}

const char8_t* AuthenticationSlaveImpl::getBlazeServerNucleusClientId() const
{
    return gController->getBlazeServerNucleusClientId();
}

bool AuthenticationSlaveImpl::passClientGrantWhitelist(const char8_t* productName, const char8_t* groupName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
    {
        ClientWhitelistList::const_iterator wlItr = productItr->second->getClientEntitlementGrantWhitelist().begin();
        ClientWhitelistList::const_iterator wlEnd = productItr->second->getClientEntitlementGrantWhitelist().end();
        for(; wlItr != wlEnd; ++wlItr)
        {
            if (blaze_stricmp(wlItr->c_str(), groupName) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool AuthenticationSlaveImpl::passClientModifyWhitelist(const char8_t* productName, const char8_t* groupName) const
{
    ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
    if (productItr != getConfig().getProducts().end())
    {
        ClientWhitelistList::const_iterator wlItr = productItr->second->getClientEntitlementModifyWhitelist().begin();
        ClientWhitelistList::const_iterator wlEnd = productItr->second->getClientEntitlementModifyWhitelist().end();
        for(; wlItr != wlEnd; ++wlItr)
        {
            if (blaze_stricmp(wlItr->c_str(), groupName) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool AuthenticationSlaveImpl::shouldBypassRateLimiter(ClientType clientType) const
{
    const ClientTypeList& types = getConfig().getBypassRateLimiterClientTypes();
    ClientTypeList::const_iterator itr = types.begin();
    ClientTypeList::const_iterator end = types.end();
    for(; itr != end; ++itr)
    {
        if (*itr == clientType)
            return true;
    }
    return false;
}

bool AuthenticationSlaveImpl::isBelowPsuLimitForClientType(ClientType clientType) const
{
    return (gUserSessionManager->getUserSessionCountByClientType(clientType) < getPsuLimitForClientType(clientType));
}

uint32_t AuthenticationSlaveImpl::getPsuLimitForClientType(ClientType clientType) const
{
    AuthenticationConfig::PsuLimitsMap::const_iterator itr = getConfig().getPsuLimits().find(clientType);
    if (itr != getConfig().getPsuLimits().end())
        return itr->second;

    return UINT32_MAX;
}

bool AuthenticationSlaveImpl::getEnableBypassForDedicatedServers() const
{
    return getConfig().getEnableBypassForDedicatedServers();
}

BlazeRpcError AuthenticationSlaveImpl::commonEntitlementCheck(AccountId accountId, CheckEntitlementUtil& checkEntitlementUtil, ClientType clientType, const char8_t* productName,
                                                              const Command* callingCommand/* = nullptr*/, const char8_t* userAccessToken/* = nullptr*/,
                                                              const char8_t* clientId/* = nullptr*/, bool skipAutoGrantEntitlement /* = false*/)
{
    BlazeRpcError entError = Blaze::ERR_OK;
    Blaze::Nucleus::EntitlementStatus::Code entitlementStatus = Blaze::Nucleus::EntitlementStatus::ACTIVE;
    EntitlementCheckType type = STANDARD;
    uint32_t psuCutOff = 0;
    bool softCheck = false;

    for (; type < INVALID_TYPE; type = Authentication::EntitlementCheckType(type + 1))
    {
        if (getNeedCheckEntitlement(clientType, type, productName, softCheck))
        {
            if (softCheck) //softCheck on any one is softCheck for all.
                checkEntitlementUtil.setSoftCheck(true); 
            
            entitlementStatus = Blaze::Nucleus::EntitlementStatus::UNKNOWN;
            if (Blaze::ERR_OK == (entError = checkEntitlement(type, accountId, checkEntitlementUtil, entitlementStatus, productName, psuCutOff, isAutoGrantEnabled(type, productName),callingCommand, userAccessToken)))
            {
                break;
            }
            else if (entError == AUTH_ERR_NO_SUCH_ENTITLEMENT)
            {
                if (isAutoGrantEnabled(type, productName) && !skipAutoGrantEntitlement)
                {
                    if (ERR_OK == (entError = postEntitlement(type, accountId, entitlementStatus, productName, psuCutOff, callingCommand, userAccessToken, clientId)))
                    {
                        break;
                    }
                }
            }
        }
    }

    if (entError == ERR_OK && psuCutOff != 0)
    {
        if (psuCutOff <= gUserSessionManager->getConnectedUserCount())
            entError = AUTH_ERR_EXCEEDS_PSU_LIMIT;
    }

    if (entError != Blaze::ERR_OK)
    {
        return entError;
    }

    switch (entitlementStatus)
    {
    case Blaze::Nucleus::EntitlementStatus::ACTIVE:
        //entitlement in good status
        break;
    case Blaze::Nucleus::EntitlementStatus::BANNED:
        ERR_LOG("[AuthenticationSlaveImpl].commonEntitlementCheck(): Entitlement(" << getEntitlementTag(productName) << ") exists with status(" <<
            Blaze::Nucleus::EntitlementStatus::CodeToString(entitlementStatus) << ") for nucleus account id (" << accountId << ")");
        return static_cast<BlazeRpcError>(AUTH_ERR_BANNED);
    default:
        ERR_LOG("[AuthenticationSlaveImpl].commonEntitlementCheck(): Entitlement(" << getEntitlementTag(productName) << ") exists with status(" <<
            Blaze::Nucleus::EntitlementStatus::CodeToString(entitlementStatus) << ") for nucleus account id (" << accountId << ")");
        return static_cast<BlazeRpcError>(AUTH_ERR_NO_SUCH_ENTITLEMENT);
    }

    return ERR_OK;
}

BlazeRpcError AuthenticationSlaveImpl::checkListEntitlementsRequest(const GroupNameList& groupNameList, const uint16_t pageSize, const uint16_t pageNo)
{
    BlazeRpcError error = ERR_OK;
    if ((error = isValidGroupNameList(groupNameList)) != ERR_OK)
        return error;

    if ((error = checkListEntitlementsPageSizeAndNumber(pageSize, pageNo)) != ERR_OK)
        return error;

    return error;
}

BlazeRpcError AuthenticationSlaveImpl::isValidGroupNameList(const GroupNameList& groupNameList)
{
    if (groupNameList.empty())
    {
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_LIST_ALL_ENTITLEMENTS))
        {
            INFO_LOG("[AuthenticationSlaveImpl].isValidGroupNameList: Group name list is empty and user does not have permission to list all group names.");
            return ERR_AUTHORIZATION_REQUIRED;
        }
    }
    else
    {
        for (GroupNameList::const_iterator it = groupNameList.begin(),itEnd = groupNameList.end(); it != itEnd; ++it)
        {
            if (it->empty() || it->c_str()[0] == '\0')
            {
                INFO_LOG("[AuthenticationSlaveImpl].isValidGroupNameList: Empty group name in the list. Please specify NONE if they mean to specify empty group name.");
                return AUTH_ERR_GROUPNAME_REQUIRED;
            }
        }
    }

    return ERR_OK;
}

BlazeRpcError AuthenticationSlaveImpl::checkListEntitlementsPageSizeAndNumber(const uint16_t pageSize, const uint16_t pageNo)
{
    if (pageSize > 0 && pageNo == 0)
    {
        TRACE_LOG("[AuthenticationSlaveImpl].checkListEntitlementsPageSizeAndNumber: Page number must be > 0 when page size is > 0");
        return AUTH_ERR_PAGENO_ZERO;
    }

    if (pageNo > 0 && pageSize == 0)
    {
        TRACE_LOG("[AuthenticationSlaveImpl].checkListEntitlementsPageSizeAndNumber: Page size must be > 0 when page number is > 0");
        return AUTH_ERR_PAGESIZE_ZERO;
    }

    return ERR_OK;
}

BlazeRpcError AuthenticationSlaveImpl::getError(const NucleusResult* result)
{
    return AuthenticationUtil::authFromOAuthError(oAuthSlaveImpl()->getErrorCodes()->getError(result->getCode(), result->getField(), result->getCause()));
}

Blaze::OAuth::OAuthSlaveImpl* AuthenticationSlaveImpl::oAuthSlaveImpl() 
{
    // Get the local OAuth instance. Every instance with Authentication component necessarily has OAuth component so the code we have refactored from here 
    // can be called as if that component is local.
    // This function call should be used only within Authentication module. 
    return static_cast<Blaze::OAuth::OAuthSlaveImpl*>(
        gController->getComponent(Blaze::OAuth::OAuthSlaveImpl::COMPONENT_ID, true/*reqLocal*/, true/*reqActive*/));
}

bool AuthenticationSlaveImpl::areExpressCommandsDisabled(const char8_t* context, const PeerInfo* peerInfo, const char8_t* username) const
{
    // This constant is defined and hard-coded here intentionally.  This not configurable, nor advertised in a header file because
    // we want to make it as hard as possible (yet, remain practical) for game teams to re-enable the express auth commands in
    // the production environment.
    // NOTE: It is unlikely that the hostname will change, but if/when it does, this constant may also need to change.
    static const char8_t wellknownProdNucleusConnectHost[] = "https://accounts.internal.ea.com";

    const HttpServiceConnectionMap& httpServices = gController->getFrameworkConfigTdf().getHttpServiceConfig().getHttpServices();
    HttpServiceConnectionMap::const_iterator it = httpServices.find(NucleusConnect::NucleusConnectSlave::COMPONENT_INFO.name);
    if (it != httpServices.end())
    {
        const HttpServiceConnection* nucleusConectConfig = it->second;
        if (blaze_strnicmp(nucleusConectConfig->getUrl(), wellknownProdNucleusConnectHost, sizeof(wellknownProdNucleusConnectHost) - 1) == 0)
        {
            WARN_LOG("[AuthenticationSlaveImpl].areExpressCommandsDisabled: The express authentication commands cannot be used against the production Nucleus environment '" << wellknownProdNucleusConnectHost << "'. "
                "Call context: " << context << "; "
                "Peer info: " << (peerInfo?peerInfo->getPeerAddress():InetAddress()) << "; "
                "Username: " << username);
            return true;
        }
    }

    // We also check the explicit configuration found in authentication.cfg
    if (!getConfig().getEnableExpressCommands())
    {
        WARN_LOG("[AuthenticationSlaveImpl].areExpressCommandsDisabled: The express commands are disabled via the authentication.enableExpressCommands config. "
            "Call context: " << context << "; "
            "Peer info: " << (peerInfo ? peerInfo->getPeerAddress() : InetAddress()) << "; "
            "Username: " << username);
        return true;
    }

    return false;
}

BlazeRpcError AuthenticationSlaveImpl::verifyConsoleEnv(ClientPlatformType platform, const char8_t *productName, const char8_t *consoleEnv)
{
    if ((platform != xone) && (platform != xbsx) && (platform != nx))
        return ERR_OK;

    if (((platform == xone) || (platform == xbsx)) && !blaze_strcmp(consoleEnv, gController->getFrameworkConfigTdf().getSandboxId()))
    {
        // Success - Matched base consoleEnv
        mMetricsInfo.mSandboxSuccesses.increment(1, productName, consoleEnv);
        return ERR_OK;
    }

    BlazeRpcError err = AUTH_ERR_INVALID_SANDBOX_ID;
    const ConsoleEnvironments& consoleEnvs = (((platform == xone) || (platform == xbsx)) ? getConfig().getAltSandboxIds() : getConfig().getNintendoEnvironments());
    for (const auto& curEnv : consoleEnvs)
    {
        if (!blaze_strcmp(consoleEnv, curEnv.c_str()))
        {
            err = ERR_OK;
            break;
        }
    }

    if (err == ERR_OK)
    {
        mMetricsInfo.mSandboxSuccesses.increment(1, productName, consoleEnv);
        if ((platform == xone) || (platform == xbsx))
        {
            WARN_LOG("[AuthenticationSlaveImpl].verifyConsoleEnv: Sandbox id returned from getTokenInfo (" << consoleEnv << ") "
                "did not match the currently configured Sandbox (" << gController->getFrameworkConfigTdf().getSandboxId() << "), but is an alternative Sandbox Id. This may prevent first-party functionality.");
        }
    }
    else
    {
        mMetricsInfo.mSandboxFailures.increment(1, productName, consoleEnv);
        if ((platform == xone) || (platform == xbsx))
        {
            ERR_LOG("[AuthenticationSlaveImpl].verifyConsoleEnv: Sandbox id returned from getTokenInfo (" << consoleEnv <<
                ") did not match the currently configured Sandbox (" << gController->getFrameworkConfigTdf().getSandboxId() << ") or any Alternative Sandbox Ids.");
        }
        else
        {
            ERR_LOG("[AuthenticationSlaveImpl].verifyConsoleEnv: Console environment returned from getTokenInfo (" << consoleEnv <<
                ") did not match any currently configured Nintendo environments.");
        }
    }

    return err;
}

bool AuthenticationSlaveImpl::isTrustedSource(const Command* callingCommand) const
{
    if (callingCommand == nullptr ||  callingCommand->getPeerInfo() == nullptr)
        return false;

    if (!mTrustedFilter.match(callingCommand->getPeerInfo()->getRealPeerAddress()))
    {
        return false;
    }

    return true;
}

bool AuthenticationSlaveImpl::isValidProductName(const char8_t* productName) const
{
    bool isValid = false;
    if (productName != nullptr && productName[0] != '\0')
    {
        ProductMap::const_iterator productItr = getConfig().getProducts().find(productName);
        isValid = (productItr != getConfig().getProducts().end());
    }

    if (!isValid && gController->isSharedCluster())
    {
        // For a shared cluster, the common default product is a dummy value which is only useful for metrics purpose and not something that is configured with C&I. So it is not part of
        // above map. However, we consider it valid for this function's purpose.
        isValid = (blaze_stricmp(productName, gController->getDefaultProductNameFromPlatform(common)) == 0);
    }
    
    return isValid; 
}

void AuthenticationSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    // retrieve metrics for generic information
    mMetricsInfo.getStatusInfo(status);

} // getStatusInfo



BlazeRpcError AuthenticationSlaveImpl::tickAndCheckLoginRateLimit()
{
    /****************************************
    // Do NOT change casting in this code from double to float etc. 
    // All the casting is necessary to avoid rounding errors.
    // You have been warned!!!
    ******************************************/

    if (mLoginRateLimiter.mQueueMutex.lock() == Blaze::ERR_OK)
    {
        const double SECONDS_TO_MICROSECONDS = 1000.0 * 1000.0;
        const double MICROSECONDS_TO_SECONDS = 1.0 / (SECONDS_TO_MICROSECONDS);

        // Take the current time and align it to previous second
        int64_t boundary = (int64_t) (floor(((double)TimeValue::getTimeOfDay().getMicroSeconds()) * MICROSECONDS_TO_SECONDS));
        TRACE_LOG("Destined boundary aligned to current second: " << boundary);
        
        // If there hasn't been an attempt in the window starting from last second, reset our boundary and counter. 
        if (mLoginRateLimiter.mCurrentWindowStart != boundary) 
        {
            TRACE_LOG("Current active window boundary: " << mLoginRateLimiter.mCurrentWindowStart << " does not match current second: "
                << boundary << ". Reset the counters.");

            mLoginRateLimiter.mCurrentWindowStart = boundary;
            mLoginRateLimiter.mCurrentWindowCounter = 0;
        }

        if (++mLoginRateLimiter.mCurrentWindowCounter <= mLoginRateLimiter.mLimitPerSecond)
        {
            mLoginRateLimiter.mCurrentWindowStart = boundary;
            mLoginRateLimiter.mQueueMutex.unlock(); 
            
            TRACE_LOG("Login attempt allowed at active window boundary: " << mLoginRateLimiter.mCurrentWindowStart
            << ", Current counter: "<< mLoginRateLimiter.mCurrentWindowCounter);
            return Blaze::ERR_OK;
        }
        else
        {
            // Sleep until the next second boundary
            double now = (double)(TimeValue::getTimeOfDay().getMicroSeconds())* MICROSECONDS_TO_SECONDS;
            TimeValue sleepUntil = TimeValue((int64_t)((ceil(now)- now)*SECONDS_TO_MICROSECONDS));
            BlazeRpcError sleepErr = gSelector->sleep(sleepUntil);

            // We are awake now. As the head of the line blocker, reset the counter and boundary.
            mLoginRateLimiter.mCurrentWindowStart = (int64_t)(floor(((double)TimeValue::getTimeOfDay().getMicroSeconds()) * MICROSECONDS_TO_SECONDS));
            mLoginRateLimiter.mCurrentWindowCounter = 1; // For the purpose of this function, we ignore if sleepErr != ERR_OK

            TRACE_LOG("Woke up at now active window boundary:" << mLoginRateLimiter.mCurrentWindowStart << ". Reset the counters.");
            
            mLoginRateLimiter.mQueueMutex.unlock();
            return sleepErr; 
        }
    }

    return Blaze::ERR_SYSTEM;
}

NucleusBypassType AuthenticationSlaveImpl::isBypassNucleusEnabled(ClientType clientType) const
{
    NucleusBypassType type = BYPASS_NONE;
    if (!mBypassNucleusEnabled &&
        clientType == CLIENT_TYPE_DEDICATED_SERVER &&
        getEnableBypassForDedicatedServers())
    {
        type = BYPASS_DEDICATED_SERVER_CLIENT_TYPE;
    }

    if (mBypassNucleusEnabled)
    {
        type = BYPASS_ALL;
    }

    return type;
}

const LegalDocContentInfo *AuthenticationSlaveImpl::getLastestLegalDocContentInfoByContentType(const char8_t* isolanguage, const char8_t *isoCountry, const ClientPlatformType clientPlatformType,
    const char8_t *docType, GetLegalDocContentRequest::ContentType contentType, const Command* callingCommand)
{
    char8_t legalDocKey[LEGAL_DOC_KEY_MAX_LENGTH];
    convertLegalDocKey(isolanguage, isoCountry, clientPlatformType, docType, GetLegalDocContentRequest::ContentTypeToString(contentType), legalDocKey);

    const LegalDocContentInfo *legalDocContentInfo = getLegalDocContentInfo(legalDocKey);

    // we need valid content for display here so trigger immediate update if the current info is not valid
    if (legalDocContentInfo == nullptr ||
        !legalDocContentInfo->isValid())
    {
        //legalDocKey is not in map
        if (updateLegalDocContent(isolanguage, isoCountry, clientPlatformType, docType, GetLegalDocContentRequest::ContentTypeToString(contentType), false, callingCommand) == Blaze::ERR_OK)
        {
            //get lastest LegalDoc content
            legalDocContentInfo = getLegalDocContentInfo(legalDocKey);
        }
    }
    else
    {
        //legalDocKey is in map
        if (legalDocContentInfo->getIsStale())
        {
            // send HTTP_HEAD to check stale.
            if (isLegalDocStale(legalDocContentInfo, callingCommand))
            {
                // content stale, we need to send HTTP_GET to update.
                if (updateLegalDocContent(isolanguage, isoCountry, clientPlatformType, docType, GetLegalDocContentRequest::ContentTypeToString(contentType), false, callingCommand) == Blaze::ERR_OK)
                {
                    legalDocContentInfo = getLegalDocContentInfo(legalDocKey);
                }
            }
            else
            {
                //content not stale, flag it.
                const_cast<LegalDocContentInfo *>(legalDocContentInfo)->setLegalDocContentLastTime(static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec()));
                const_cast<LegalDocContentInfo *>(legalDocContentInfo)->setIsStale(false);
            }
        }

        if (TimeValue::getTimeOfDay().getSec() - legalDocContentInfo->getLegalDocContentLastTime() > getConfig().getStopLegalDocServiceIntervalTime().getSec())
        {
            //Legal Web Service is down longer and large than StopLegalDocServiceIntervalTime
            return nullptr;
        }
    }

    return legalDocContentInfo;
}

/***************************************** PRIVATE METHODS *****************************************/

BlazeRpcError AuthenticationSlaveImpl::updateLegalDocContent(const char8_t *language, const char8_t *country, const ClientPlatformType clientPlatformType, const char8_t *docType,
                                                             const char8_t *contentType, bool delayUpdate, const Command* callingCommand)
{
    BlazeRpcError err = Blaze::ERR_OK;
    char8_t legalDocKey[LEGAL_DOC_KEY_MAX_LENGTH];
    convertLegalDocKey(language, country, clientPlatformType, docType, contentType, legalDocKey);
    const char8_t *legalDocUri = nullptr;
    const char8_t *legalDocContent = nullptr;
    uint32_t legalDocContentSize = 0;
    uint32_t currentTime = static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec());

    if (delayUpdate || mTosServiceDown)
    {
        LegalDocContentInfoMap::insert_return_type inserted = mLegalDocContentInfoMap.insert(legalDocKey);
        LegalDocContentInfo &legalDocContentInfo = inserted.first->second;
        if (inserted.second)
        {
            // If the legal doc is not in cache, just add the combo to the cache so that it will definitely be part of regular updates later.
            // in case this is a delayUpdate == true request or we're skipping this update because legal doc service is down
            if (!legalDocContentInfo.initialize(language, country, clientPlatformType, docType, contentType, legalDocContentSize, currentTime - (uint32_t)getConfig().getCheckCachedLegalDocInterval().getSec(), legalDocUri, legalDocContent))
            {
                WARN_LOG("[AuthenticationSlaveImpl].updateLegalDocContent(): Failed to initialize legal doc content info - " <<
                    "LegalDocKey(" << legalDocKey << "), language(" << language << "), country(" << country << "), clientPlatformType(" << ClientPlatformTypeToString(clientPlatformType) << "), " <<
                    "docType(" << docType << "), contentType(" << contentType << "), legalDocContentSize(" << legalDocContentSize << "), " <<
                    "currentTime(" << currentTime << "), legalDocUri(" << legalDocUri << "), legalDocContent(" << legalDocContent << ")");
                return err;
            }
        }

        // If the found legalDocContentInfo is actually a valid doc, then return (this would also mean delayUpdate==false, and therefore the mTosServiceDown==true)
        // If we're delaying the update (typically a normal login), then just return
        // If the TOS service was down and it hasn't been longer than checkCachedLegalDocInterval since we last tried to hit the TOS service, just return
        // If all of the above are false, then we don't return and we try to actually go out and get content for this legal doc.
        if (legalDocContentInfo.isValid() || delayUpdate || (mTosServiceDown && (legalDocContentInfo.getLegalDocContentLastTime() + getConfig().getCheckCachedLegalDocInterval().getSec() > currentTime)))
        {
            return err;
        }
    }

    GetLegalDocUtil getLegalDocUtil(*this, CLIENT_TYPE_INVALID);
    err = getLegalDocUtil.getLegalDocContent(docType, language, country, clientPlatformType, contentType, callingCommand);

    if (err == Blaze::ERR_OK)
    {
        legalDocUri = getLegalDocUtil.getLegalDocUri();
        legalDocContent = getLegalDocUtil.getLegalDocContentData();
        legalDocContentSize = getLegalDocUtil.getLegalDocContentDataSize();
        currentTime = static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec());

        if ((mLegalDocContentInfoMap.find(legalDocKey) == mLegalDocContentInfoMap.end()) &&
            (!mLegalDocContentInfoMap[legalDocKey].initialize(language, country, clientPlatformType, docType, contentType, legalDocContentSize, currentTime, legalDocUri, legalDocContent)))
        {
            WARN_LOG("[AuthenticationSlaveImpl].updateLegalDocContent(): Failed to initialize legal doc content info - " <<
                "LegalDocKey(" << legalDocKey << "), language(" << language << "), country(" << country << "), clientPlatformType(" << ClientPlatformTypeToString(clientPlatformType) << "), " <<
                "docType(" << docType << "), contentType(" << contentType << "), legalDocContentSize(" << legalDocContentSize << "), " <<
                "currentTime(" << currentTime << "), legalDocUri(" << legalDocUri << "), legalDocContent(" << legalDocContent << ")");
        }
        else
        {
            mLegalDocContentInfoMap[legalDocKey].set(legalDocContentSize, currentTime, legalDocUri, legalDocContent);
        }
        return err;
    }
    else
    {
        mTosServiceDown = Fiber::isCurrentlyCancelled() || getLegalDocUtil.hasTosServerError();
        // we only want to add this combo to the cache if we failed due to TOS service being down or unavailable (timeout)
        // we don't want to add combos to the cache that cause 404 errors to be returned by the TOS server (invalid language/country/platform)
        if (mTosServiceDown)
        {
            if (mLegalDocContentInfoMap.find(legalDocKey) == mLegalDocContentInfoMap.end())
            {
                // If the legal doc is not in cache, just add the combo to the cache so that it will definitely be part of regular updates later.
                // in case we're skipping this update because legal doc service is down
                currentTime = static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec());
                if (!mLegalDocContentInfoMap[legalDocKey].initialize(language, country, clientPlatformType, docType, contentType, legalDocContentSize, currentTime, legalDocUri, legalDocContent))
                {
                    WARN_LOG("[AuthenticationSlaveImpl].updateLegalDocContent(): Failed to initialize legal doc content info - " <<
                        "LegalDocKey(" << legalDocKey << "), language(" << language << "), country(" << country << "), clientPlatformType(" << ClientPlatformTypeToString(clientPlatformType) << "), " <<
                        "docType(" << docType << "), contentType(" << contentType << "), legalDocContentSize(" << legalDocContentSize << "), " <<
                        "currentTime(" << currentTime << "), legalDocUri(" << legalDocUri << "), legalDocContent(" << legalDocContent
                        << ") failed with error[" << ErrorHelp::getErrorName(err) << "]");
                }
                else
                {
                    mLegalDocContentInfoMap[legalDocKey].setLegalDocContentLastTime(currentTime);
                }
            }
        }
    }

    return err;
}

void AuthenticationSlaveImpl::convertLegalDocKey(const char8_t *language, const char8_t *country, const ClientPlatformType clientPlatformType, const char8_t *docType, const char8_t* contentType,
                                                 char8_t *legalDocKey, size_t length) const
{
    const char8_t *platform = getTOSPlatformStringFromClientPlatformType(clientPlatformType, contentType);
    if (*getLegalDocGameIdentifier() == '\0')
        blaze_snzprintf(legalDocKey, length, "%s_%s_%s_%s_%s", language, country, platform, docType, contentType);
    else
        blaze_snzprintf(legalDocKey, length, "%s_%s_%s_%s_%s_%s", language, country, platform, docType, contentType, getLegalDocGameIdentifier());
    blaze_strlwr(legalDocKey);
}

void AuthenticationSlaveImpl::scheduleNextCheckLegalDoc()
{
    if(mStopCheckLegalDoc)
        return;

    checkLegalDocMap();

    mCheckLegalDocId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + getConfig().getCheckLegalDocIntervalTime(),
        this, &AuthenticationSlaveImpl::scheduleNextCheckLegalDoc,
        "AuthenticationSlaveImpl::scheduleNextCheckLegalDoc");
    if (mCheckLegalDocId == INVALID_TIMER_ID)
    {
        WARN_LOG("[AuthenticationSlaveImpl].scheduleNextCheckLegalDoc(): Failed to scheduleFiberTimerCall");
    }
}

void AuthenticationSlaveImpl::checkLegalDocMap()
{
    // reset the mTosServiceDown flag
    mTosServiceDown = false;

    eastl::vector<eastl::string> mapKey;
    LegalDocContentInfoMap::const_iterator itr;
    for(itr = mLegalDocContentInfoMap.begin(); itr != mLegalDocContentInfoMap.end(); ++itr)
    {
        mapKey.push_back(itr->first);
    }

    eastl::vector<eastl::string>::const_iterator it;
    const LegalDocContentInfo *legalDocContentInfo = nullptr;
    for(it = mapKey.begin(); it != mapKey.end(); ++it)
    {
        legalDocContentInfo = getLegalDocContentInfo(it->c_str());
        if(legalDocContentInfo != nullptr &&
            !mTosServiceDown)
        {
            // lazy evaluation. rather than update content, just flag it.
            const_cast<LegalDocContentInfo *>(legalDocContentInfo)->setIsStale(true);
        }
    }

    mapKey.clear();
}

bool AuthenticationSlaveImpl::isLegalDocStale(const LegalDocContentInfo* curContentInfo, const Command* callingCommand)
{
    NucleusGetLegalDocUri getLegalDocUri(this->getConnectionManagerPtr(AuthenticationSlaveImpl::TOS), &(this->getNucleusBypassList()));
    
    if (getLegalDocUri.getLegalDocUri(curContentInfo->getLegalDocType(), curContentInfo->getLanguage(),
        curContentInfo->getCountry(), curContentInfo->getClientPlatformType(), curContentInfo->getContentType(), getLegalDocGameIdentifier(), callingCommand))
    {
        if (blaze_strcmp(curContentInfo->getLegalDocUri().c_str(), getLegalDocUri.getResult()->getURI()) == 0)
        {
            // same version, the content is not stale.
            return false;
        }
    }

    return true;
}

bool AuthenticationSlaveImpl::onConfigure()
{
    return configure();
}

bool AuthenticationSlaveImpl::onReconfigure()
{
    return configure();
}

bool AuthenticationSlaveImpl::configure()
{
    clearProductEntitlements();
    setProductEntitlements();

    ProductTrialContentIdMap productTrialContentIdMap;
    ProductProjectIdMap productProjectIdMap;

    ProductMap::const_iterator prodItr = getConfig().getProducts().begin();
    ProductMap::const_iterator prodEnd = getConfig().getProducts().end();

    for (; prodItr != prodEnd; ++prodItr)
    {
        const char8_t* productName = prodItr->first;
        const Product* product = prodItr->second;
        const char8_t* authSource = product->getAuthenticationSource();
        const char8_t* regSource = product->getRegistrationSource();

        auto trialContent = productTrialContentIdMap.allocate_element();
        product->getTrialContentIds().copyInto(*trialContent);
        productTrialContentIdMap.insert(eastl::make_pair(productName, trialContent));

        productProjectIdMap[productName] = getProjectId(productName);
        
        TRACE_LOG("[AuthenticationSlaveImpl].onConfigure Product: " << prodItr->first << ", Authentication Source: " << authSource << ", Registration Source: " << regSource);
    }

    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->setProductInfoMaps(productTrialContentIdMap, productProjectIdMap);

    mLoginRateLimiter.mLimitPerSecond = (uint32_t) (ceil(getConfig().getTotalLoginsPerMinute() / 60.0f)); // ceil it to err on the side of allowing a higher limit.

    mStopCheckLegalDoc = false;

    if (mCheckLegalDocId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mCheckLegalDocId);
        mCheckLegalDocId = INVALID_TIMER_ID;
    }
    gSelector->scheduleFiberCall<AuthenticationSlaveImpl>(this, &AuthenticationSlaveImpl::scheduleNextCheckLegalDoc, "AuthenticationSlaveImpl::scheduleNextCheckLegalDoc");

    
    if (mEntitlementStatusCheckTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mEntitlementStatusCheckTimerId);

    
    bool platformEntitlementCheckEnabled = false;
    if (!getConfig().getEnableEntitlementCheckPlatforms().empty())
    {
        for (auto& platform : getConfig().getEnableEntitlementCheckPlatforms())
        {
            if (gController->isPlatformHosted(platform))
            {
                platformEntitlementCheckEnabled = true;
                break;
            }
        }
    }

    if (getConfig().getEntitlementStatusCheckPeriod() == 0 || !platformEntitlementCheckEnabled)
    {
        INFO_LOG("[AuthenticationSlaveImpl].onConfigure: entitlement status check is turned off. Check Period ("
            << getConfig().getEntitlementStatusCheckPeriod().getSec() << "s). platformEntitlementCheckEnabled(" << platformEntitlementCheckEnabled << ")");
    }
    else
    {
        if (gUserSessionMaster != nullptr)
        {
            mEntitlementStatusCheckTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + getConfig().getEntitlementStatusCheckPeriod(),
                    this, &AuthenticationSlaveImpl::entitlementStatusCheck, "entitlementStatusCheck");        
        }
        else
        {
            // TODO_MC: The authentication component gets loaded in an auxSlave, and not a coreSlave in some arson tests.  This is no longer supported.
            //          Consider tagging components (somehow, codegen?) that can must run on a coreSlave, or at least along side the gUserSessionManager/gUserSessionMaster.
            INFO_LOG("[AuthenticationSlaveImpl].onConfigure: entitlement status check is turned off due to no gUserSessionMaster available.");
        }
    }

    mTrustedFilter.initialize(false, getConfig().getTrustedSources());

    return true;
}

bool AuthenticationSlaveImpl::onResolve()
{
    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->addLocalUserSessionSubscriber(*this);

    return true;
}

void AuthenticationSlaveImpl::onShutdown()
{
    mBackgroundJobQueue.join();

    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->removeLocalUserSessionSubscriber(*this);
}

bool AuthenticationSlaveImpl::onActivate()
{
    // verify that the authentication http services that don't have proxy components are also registered
    for (uint16_t i = 0; i < ConnectionManagerType::MAX; i++)
    {
        if (gOutboundHttpService->getConnection(sPoolNames[i]) == nullptr)
        {
            FATAL_LOG("[AuthenticationSlaveImpl].onActivate: Authentication component has dependency on "<< sPoolNames[i]
                << "HTTP service to be defined in framework.cfg.  Please make sure this service is defined");
            return false;
        }
    }

    return true;
}

void AuthenticationSlaveImpl::onLocalUserSessionLogout(const UserSessionMaster& userSession)
{
    oAuthSlaveImpl()->onLocalUserSessionLogout(userSession.getUserSessionId());
}

bool AuthenticationSlaveImpl::onValidateConfig(AuthenticationConfig& config, const AuthenticationConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (config.getProducts().empty())
    {
        eastl::string msg;
        msg.sprintf("[AuthenticationSlaveImpl].onValidateConfig() : No products specified.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    ProductMap::const_iterator prodItr = config.getProducts().begin();
    ProductMap::const_iterator prodEnd = config.getProducts().end();
    for (; prodItr != prodEnd; ++prodItr)
    {
        const Product* product = prodItr->second;
        const char8_t* authSource = product->getAuthenticationSource();
        const char8_t* regSource = product->getRegistrationSource();
        if ((authSource[0] == '\0') || (regSource[0] == '\0'))
        {
            eastl::string msg;
            msg.sprintf("[AuthenticationSlaveImpl].onValidateConfig() : Authentication source or Registration Source not specified for product %s.", prodItr->first.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        if (!product->getAllowUnderage())
        {
            const UnderageLoginOverride& underageOverride = product->getUnderageLoginOverride();
            const char8_t* tag = underageOverride.getEntitlementTag();
            const char8_t* group = underageOverride.getEntitlementGroup();
            const char8_t* projectId = underageOverride.getProjectId();
            if ((tag[0] == '\0') || (group[0] == '\0') || (projectId[0] == '\0'))
            {
                WARN_LOG("[AuthenticationSlaveImpl].onValidateConfig() : Underage accounts not allowed on the server but the entitlement tag, group or product ID has not been specified "
                    "for the underage override for product " << prodItr->first.c_str() << ". No override will be possible.");
            }
        }
    }

    
    bool entitlementCheckEnabled = false;
    eastl::string entitlementCheckMsg;

    if (!config.getEnableEntitlementCheckPlatforms().empty())
    {
        for (auto& platform : config.getEnableEntitlementCheckPlatforms())
        {
            if (gController->isPlatformHosted(platform))
            {
                entitlementCheckMsg.append_sprintf("%s, ", ClientPlatformTypeToString(platform));
                entitlementCheckEnabled = true;
            }
        }
    }

    if (config.getEntitlementStatusCheckPeriod() == 0 && entitlementCheckEnabled)
    {
        eastl::string msg(eastl::string::CtorSprintf(),
            "[AuthenticationSlaveImpl.onValidateConfig()] : entitlement status check is disabled but platforms (%s) are configured for entitlement check.", entitlementCheckMsg.c_str());
        validationErrors.getErrorMessages().push_back().set(msg.c_str());
    }

    return validationErrors.getErrorMessages().empty();
}

void AuthenticationSlaveImpl::setProductEntitlements()
{
    clearPsuCutoffProductMetrics();

    for (ProductMap::const_iterator productItr = getConfig().getProducts().begin(), itrEnd = getConfig().getProducts().end(); productItr != itrEnd; ++productItr)
    {
        const char8_t* productName = productItr->first;
        EntitlementCheckMap& entitlementCheckMap = mProductEntitlementMap[productName];
        for (AccessList::const_iterator accessItr = productItr->second->getAccess().begin(); accessItr != productItr->second->getAccess().end(); ++accessItr)
        {
            const Access& accessRef = **accessItr;
            EntitlementCheckType type = accessRef.getType();
            if (entitlementCheckMap.find(type) == entitlementCheckMap.end())
            {
                entitlementCheckMap.insert(eastl::make_pair(type, AccessListPtr(BLAZE_NEW AccessList())));
            }

            AccessListPtr& accessListPtr = entitlementCheckMap[type];
            Access* access = accessListPtr->pull_back();

            if (type != STANDARD && !accessListPtr->empty())
            {
                AccessList::iterator aItr = accessListPtr->begin();

                if (type == PERIOD_TRIAL)
                {
                    TimeValue tv;
                    tv.parseGmDateTime(accessRef.getPeriod().getEnd());

                    if (accessRef.getUseManagedLifecycle() && tv < TimeValue::getTimeOfDay())
                    {
                        accessListPtr->erase(accessListPtr->end() - 1);
                        delete access;
                        continue;
                    }

                    for (; aItr != accessListPtr->end() - 1; ++aItr)
                    {
                        if (accessRef.getPeriod().getEnd() < (*aItr)->getPeriod().getEnd())
                            break;
                    }
                }
                else if (type == TIMED_TRIAL)
                {
                    for (; aItr != accessListPtr->end() - 1; ++aItr)
                    {
                        if (accessRef.getTime() > (*aItr)->getTime())
                            break;
                    }
                }

                for (AccessList::iterator it = accessListPtr->end() - 1; it != aItr; --it)
                {
                    (*(it - 1))->copyInto(**it);
                }

                accessRef.copyInto(**aItr);
            }
            else
            {
                accessRef.copyInto(*access);
            }

            mMetricsInfo.mPsuCutoff.set((*accessItr)->getPsuCutOff(), productName, type, accessRef.getProjectId(), accessRef.getOnlineAccessEntitlementGroup());
        }
    }
}

void AuthenticationSlaveImpl::clearProductEntitlements()
{
    mProductEntitlementMap.clear();
}

BlazeRpcError AuthenticationSlaveImpl::checkEntitlement(EntitlementCheckType type, AccountId accountId, CheckEntitlementUtil& checkEntitlementUtil,
                                                        Blaze::Nucleus::EntitlementStatus::Code& entitlementStatus, const char8_t* productName, uint32_t& psuCutOff,
                                                        bool autoGrantEnabled, const Command* callingCommand/* = nullptr*/, const char8_t* accessToken/* = nullptr*/)
{
    TRACE_LOG("[AuthenticationSlaveImpl].checkEntitlement: checking Entitlement for nucleus id (" << accountId << ")");
    
    ProductEntitlementMap::const_iterator semItr = mProductEntitlementMap.find(productName);
    if (semItr == mProductEntitlementMap.end())
    {
        ERR_LOG("[AuthenticationSlaveImpl].checkEntitlement(): Product name " << productName << " is not defined in products.cfg");
        return ERR_SYSTEM;
    }

    const EntitlementCheckMap& entitlementCheckMap = semItr->second;

    EntitlementCheckMap::const_iterator eItr = entitlementCheckMap.find(type);

    BlazeRpcError entError = AUTH_ERR_NO_SUCH_ENTITLEMENT;

    bool foundActive = false;
    if (eItr != entitlementCheckMap.end())
    {
        // have to make a temp copy of the AccessListPtr intrusive_ptr since we might be reconfiguring after the subsequent blocking call
        AccessListPtr accessListPtr(eItr->second);

        // Loop through all entitlements for multiple projectId, return if a BANNED status entitlement is found.
        for (AccessList::const_iterator accessItr = accessListPtr->begin(), accessItrEnd = accessListPtr->end(); accessItr != accessItrEnd; ++accessItr)
        {
            GroupNameList groupNameList;
            if ((*accessItr)->getOnlineAccessEntitlementGroup()[0] != '\0')
            {
                groupNameList.push_back((*accessItr)->getOnlineAccessEntitlementGroup());
            }

            char8_t startDate[64];
            char8_t endDate[64];

            startDate[0] = '\0';
            endDate[0] = '\0';

            TimeValue tv;

            tv.parseGmDateTime((*accessItr)->getPeriod().getStart());
            tv.toAccountString(startDate, sizeof(startDate));

            tv.parseGmDateTime((*accessItr)->getPeriod().getEnd());
            tv.toAccountString(endDate, sizeof(endDate));

            TRACE_LOG("[AuthenticationSlaveImpl].checkEntitlement: checking Entitlement(" << (*accessItr)->getEntitlementTag() << ") type(" << Blaze::Authentication::EntitlementCheckTypeToString((*accessItr)->getType()) << ")  for nucleus id (" << accountId << ")");
            
            entError = checkEntitlementUtil.checkEntitlement(
                accountId,
                "",
                (*accessItr)->getEntitlementTag(),
                groupNameList,
                &entitlementStatus,
                "",
                (*accessItr)->getTime(),
                autoGrantEnabled,
                Blaze::Nucleus::EntitlementSearchType::USER_OR_PERSONA,
                (*accessItr)->getEntitlementType(),
                (*accessItr)->getType(),
                (*accessItr)->getUseManagedLifecycle(),
                startDate,
                endDate,
                callingCommand,
                accessToken
                );

            if (entError == Blaze::ERR_OK)
            {
                TRACE_LOG("[AuthenticationSlaveImpl].checkEntitlement: Entitlement(" << (*accessItr)->getEntitlementTag() << ") exists with status(" << Blaze::Nucleus::EntitlementStatus::CodeToString(entitlementStatus)
                    << ") for nucleus id (" << accountId << ")");
                
                psuCutOff = (*accessItr)->getPsuCutOff();
                if (entitlementStatus ==  Blaze::Nucleus::EntitlementStatus::BANNED)
                    return entError;
                else if (entitlementStatus ==  Blaze::Nucleus::EntitlementStatus::ACTIVE)
                    foundActive = true;
            }
        }
    }
    if (foundActive)
    {
        entitlementStatus =  Blaze::Nucleus::EntitlementStatus::ACTIVE;
        return Blaze::ERR_OK;
    }
    
    if (entError != Blaze::ERR_OK )
    {
        BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[AuthenticationSlaveImpl].checkEntitlement: Entitlement check ran into (" << Blaze::ErrorHelp::getErrorName(entError)
            << ") error for nucleus id (" << accountId << ")");
    }
    
    return entError;
}

BlazeRpcError AuthenticationSlaveImpl::postEntitlement(EntitlementCheckType type, AccountId accountId, Blaze::Nucleus::EntitlementStatus::Code& entitlementStatus, const char8_t* productName,
                                              uint32_t& psuCutOff, const Command* callingCommand/* = nullptr*/, const char8_t* userAccessToken/* = nullptr*/, const char8_t* clientId/* = nullptr*/)
{
    ProductEntitlementMap::const_iterator semItr = mProductEntitlementMap.find(productName);
    if (semItr == mProductEntitlementMap.end())
    {
        WARN_LOG("[AuthenticationSlaveImpl].postEntitlement(): Product name " << productName << " is not defined in products.cfg");
        return AUTH_ERR_NO_SUCH_ENTITLEMENT;
    }

    const EntitlementCheckMap entitlementCheckMap = semItr->second;

    BlazeRpcError err = AUTH_ERR_NO_SUCH_ENTITLEMENT;

    EntitlementCheckMap::const_iterator eItr = entitlementCheckMap.find(type);
    if (eItr != entitlementCheckMap.end())
    {
        // have to make a temp copy of the AccessListPtr intrusive_ptr since we might be reconfiguring after the subsequent blocking call
        AccessListPtr accessListPtr(eItr->second);

        for (AccessList::const_iterator accessItr = accessListPtr->begin(), accessItrEnd = accessListPtr->end(); accessItr != accessItrEnd; ++accessItr)
        {
            const Access& access = **accessItr;
            if (!(access.getAutoEntitlementEnabled()))
                continue;

            char8_t grantDate[64];
            char8_t terminationDate[64];

            grantDate[0] = '\0';
            terminationDate[0] = '\0';
            TimeValue tv = TimeValue::getTimeOfDay();

            TimeValue startTime;
            TimeValue endTime;

            startTime.parseGmDateTime(access.getPeriod().getStart());
            endTime.parseGmDateTime(access.getPeriod().getEnd());

            if (type == PERIOD_TRIAL && (endTime < tv || startTime > tv))
            {
                err = AUTH_ERR_TRIAL_PERIOD_CLOSED;
                continue;
            }

            if (access.getUseManagedLifecycle())
            {
                if (type == PERIOD_TRIAL)
                {
                    endTime.toAccountString(terminationDate, sizeof(terminationDate));
                }
                else if (type == TIMED_TRIAL)
                {
                    tv.toAccountString(grantDate, sizeof(grantDate));
                    tv = tv + access.getTime();
                    tv.toAccountString(terminationDate, sizeof(terminationDate));
                }
            }

            TRACE_LOG("[AuthenticationSlaveImpl].postEntitlement: Auto granting entitlement(" << access.getEntitlementTag() << ") for accountId(" << accountId << ")");

            OAuth::AccessTokenUtil tokenUtil;
            {
                //retrieve full scope server access token in a block to avoid changing function context permission
                UserSession::SuperUserPermissionAutoPtr permission(true);
                err = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[HasEntitlementUtil].getEntitlement: Failed to retrieve server access token");
                    break;
                }
            }

            NucleusIdentity::PostEntitlementRequest req;
            NucleusIdentity::UpsertEntitlementResponse resp;
            NucleusIdentity::IdentityError error;
            req.setReleaseType(gUserSessionManager->getProductReleaseType(productName));
            req.setPid(accountId);

            req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
            if (userAccessToken != nullptr)
                req.getAuthCredentials().setUserAccessToken(userAccessToken);

            if (clientId == nullptr)
                req.getAuthCredentials().setClientId(tokenUtil.getClientId());
            else
                req.getAuthCredentials().setClientId(clientId);

            req.getAuthCredentials().setIpAddress(AuthenticationUtil::getRealEndUserAddr(callingCommand).getIpAsString());
         
            NucleusIdentity::UpsertEntitlementInfo &info = req.getEntitlementInfo();
            info.setProductId(access.getProductId());
            info.setEntitlementTag(access.getEntitlementTag());
            info.setStatus(Blaze::Nucleus::EntitlementStatus::CodeToString(Blaze::Nucleus::EntitlementStatus::ACTIVE));
            info.setGroupName(access.getOnlineAccessEntitlementGroup());
            info.setProjectId(access.getProjectId());
            info.setEntitlementSource(getEntitlementSource(productName));
            info.setEntitlementType(Blaze::Nucleus::EntitlementType::CodeToString(access.getEntitlementType()));
            info.setManagedLifecycle(access.getUseManagedLifecycle());
            info.setTerminationDate(terminationDate);
            info.setGrantDate(grantDate);

            NucleusIdentity::NucleusIdentitySlave *comp = (NucleusIdentity::NucleusIdentitySlave*)gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
            if (comp == nullptr)
            {
                ERR_LOG("[AuthenticationSlaveImpl].postEntitlement: nucleusconnect is nullptr");
                err = ERR_SYSTEM;
                break;
            }

            err = comp->searchOrPostEntitlement(req, resp, error);
            if (err == ERR_OK || err == NUCLEUS_IDENTITY_ENTITLEMENT_GRANTED)
            {
                if (err == NUCLEUS_IDENTITY_ENTITLEMENT_GRANTED)
                {
                    TRACE_LOG("[AuthenticationSlaveImpl].postEntitlement(): Online Play Entitlement granted for accountId(" << accountId << "), productId("
                        << (access.getProductId() ? access.getProductId() : "") << "), entitlementTag(" << access.getEntitlementTag() << "), group("
                        << (access.getOnlineAccessEntitlementGroup() ? access.getOnlineAccessEntitlementGroup() : "")
                        << "), projectId(" << (access.getProjectId() ? access.getProjectId() : "") << "), entitlementType(ONLINE_ACCESS)");
                }

                err = ERR_OK;

                entitlementStatus = Blaze::Nucleus::EntitlementStatus::ACTIVE;
                psuCutOff = access.getPsuCutOff();
                break;
            }
            else
            {
                err = AuthenticationUtil::parseIdentityError(error, err);
                ERR_LOG("[AuthenticationSlaveImpl].postEntitlement(): Can't create entitlement for accountId(" << accountId << "), productId("
                    << (access.getProductId() ? access.getProductId() : "") << "), entitlementTag(" << access.getEntitlementTag() << "), group("
                    << (access.getOnlineAccessEntitlementGroup() ? access.getOnlineAccessEntitlementGroup() : "")
                    << "), projectId(" << (access.getProjectId() ? access.getProjectId() : "") << "), "
                    << "entitlementType(ONLINE_ACCESS) failed with error[" << ErrorHelp::getErrorName(err) << "]");
                continue;
            }
        }
    }

    return err;
}

bool AuthenticationSlaveImpl::isAutoGrantEnabled(EntitlementCheckType type, const char8_t* productName) const
{
    ProductEntitlementMap::const_iterator semItr = mProductEntitlementMap.find(productName);
    if (semItr == mProductEntitlementMap.end())
        return false;

    const EntitlementCheckMap& entitlementCheckMap = semItr->second;

    EntitlementCheckMap::const_iterator eItr = entitlementCheckMap.find(type);
    if (eItr != entitlementCheckMap.end())
    {
        for (AccessList::const_iterator accessItr = eItr->second->begin(); accessItr != eItr->second->end(); ++accessItr)
        {
            if ((*accessItr)->getAutoEntitlementEnabled())
                return true;
        }
    }
    return false;
}

// Loop through list of local user ids and perform entitlement status checks, logout the user should the status check failed.
// Trusted and expressLogins are excluded from entitlementStatusCheck.
void AuthenticationSlaveImpl::entitlementStatusCheck()
{
    TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: Scheduling the next status check to start in " << getConfig().getEntitlementStatusCheckPeriod().getSec() << " seconds.");
    mEntitlementStatusCheckTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + getConfig().getEntitlementStatusCheckPeriod(),
            this, &AuthenticationSlaveImpl::entitlementStatusCheck, "entitlementStatusCheck");
    
    // If another check is in progress, that means that either the server just has been reconfigured or the server isn't able to complete the check during the configured period, 
    // simply yield and we'll pick up the next check in the next scheduled fiber above
    if (mEntitlementStatusCheckInProgress)
    {
        WARN_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: duplicate entitlementStatusCheck is detected, skipping the current entitlementStatusCheck. This is normal to happen once during a reconfiguration, otherwise it implies that the entitlementStatusCheck cannot be completed during the configured entitlementStatusCheckPeriod:" << getConfig().getEntitlementStatusCheckPeriod().getSec() << " seconds which should be investigated.");
        return;
    }
    
    mEntitlementStatusCheckInProgress = true;
    BlazeRpcError err = ERR_SYSTEM;

    // Consolidate a temp copy of the local usersession ids and iterate through it
    eastl::vector<UserSessionId> usersessionIds;
    usersessionIds.reserve(gUserSessionMaster->getOwnedUserSessionsMap().size());

    // Get all local usersessions
    for (UserSessionMasterPtrByIdMap::const_iterator itr = gUserSessionMaster->getOwnedUserSessionsMap().begin(), end = gUserSessionMaster->getOwnedUserSessionsMap().end(); itr != end; ++itr)
    {
        const UserSessionMaster* userSession = itr->second.get();
        const UserSessionId userSessionId = userSession->getUserSessionId();

        // entitlementStatusCheck is skipped if skipPeriodicEntitlementStatusCheck is enabled for this usersession (eg. trustedLogins and expressLogins)
        if (userSession->getSessionFlags().getSkipPeriodicEntitlementStatusCheck())
        {
            TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: bypassing entitlement status check for UserSessionId(" << userSessionId << ") as SkipPeriodicEntitlementStatusCheck is enabled for this usersession.");
            continue;
        }

        if (eastl::find(getConfig().getEnableEntitlementCheckPlatforms().begin(), getConfig().getEnableEntitlementCheckPlatforms().end(), userSession->getPlatformInfo().getClientPlatform()) 
            == getConfig().getEnableEntitlementCheckPlatforms().end())
        {
            TRACE_LOG("[AuthenticationSlaveImpl.entitlementStatusCheck] bypassing entitlement status check for UserSessionId(" << userSessionId << ") as entitlement check for platform ("
                << ClientPlatformTypeToString(userSession->getPlatformInfo().getClientPlatform()) << ") is not enabled via config.");
            continue;
        }

        const char8_t* productName = userSession->getProductName();

        // check if the product the usersession is connecting to has entitlementStatusCheck enabled
        ProductMap::const_iterator productsIt = getConfig().getProducts().find(productName);
        if (productsIt != getConfig().getProducts().end())
        {
            // if the product doesn't have entitlement status check enabled, skip it
            if (!productsIt->second->getEntitlementStatusCheckEnabled())
            {
                TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: bypassing entitlement status check for UserSessionId(" << userSessionId << ") as product(" << productsIt->first << ") has entitlement status check disabled.");
                continue;
            }
        }
        else
        {
            // if the service name isn't in the list of products somehow, log an erorr but don't kick the user (safest route is to keep them online).
            ERR_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: usersession productName(" << productName << ") not found in list of products. Skipping entitlement status check for UserSessionId(" << userSessionId << ").");
            continue;
        }
           
        // if the above checks passed, insert it into the temp list for later process
        usersessionIds.push_back(userSessionId);
    }

    INFO_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: total qualified local usersessions:" << usersessionIds.size() << ".");

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////                
    // Throttle nucleus calls for each usersession with config's mTotalEntitlementStatusCheckPerSecond
    uint32_t count = 0;
    TimeValue entitlementStatusChecknRateInterval = TimeValue::getTimeOfDay();

    while (!usersessionIds.empty())
    {
        const TimeValue now = TimeValue::getTimeOfDay();

        // if within the 1-sec interval, check if within the rate limit
        if ((now - entitlementStatusChecknRateInterval) <= 1000*1000)
        {
            // within rate limit, simply update count and continue
            if (count <= getConfig().getTotalEntitlementStatusCheckPerSecond())
            {
                count++;
            }
            else
            {
                // sleep for 1 sec
                WARN_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck rate exceeded: mTotalEntitlementStatusCheckPerSecond(" << getConfig().getTotalEntitlementStatusCheckPerSecond() << "). Throttling entitlement status check requests. Please check if mTotalEntitlementStatusCheckPerSecond is configured properly.");
                err = gSelector->sleep(1000*1000);
                if (err != ERR_OK)
                {
                    ERR_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: attempted to sleep for throttling status check failed with error:" << ErrorHelp::getErrorName(err) << ".");
                }
                
                count = 1;
                entitlementStatusChecknRateInterval = now;
            }
        }
        else
        {
            // reset count and intervalDelta
            count = 1;
            entitlementStatusChecknRateInterval = now;
        }
        
        const UserSessionId userSessionId = usersessionIds.back();
        usersessionIds.pop_back();  // remove as we process

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////                
        // Get all the relevant info for this usersession id first (eg. blaze id, current service name, client id)
        if (!gUserSessionManager->getSessionExists(userSessionId))
        {
            // if a player's usersession is not found, meaning that he is logged out during an earlier blocking call made
            TRACE_LOG("[AuthenticationSlaveImpl].statusCheck: attempted to check entitlement status but already logged out for UserSessionId(" << userSessionId << ").");
            continue;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////                
        // Refresh access token if needed based on the blazeId
        NucleusConnect::GetAccessTokenResponse getAccessTokenResponse;

        TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: check and refresh cached token for UserSessionId(" << userSessionId << "), BlazeId(" << gUserSessionManager->getBlazeId(userSessionId) << ").");
        
        ServiceName serviceName;
        gUserSessionManager->getServiceName(userSessionId, serviceName);
        ProductName productName;
        gUserSessionManager->getProductName(userSessionId, productName);

        UserSession::SuperUserPermissionAutoPtr su(true);
        OAuth::AccessTokenUtil tokenUtil;
        err = tokenUtil.retrieveUserAccessToken(OAuth::TOKEN_TYPE_NONE, userSessionId);
        if (err != ERR_OK)
        {
            // User is already logged out or other reasons (Nucleus communication issue). 
            // If we can't fetch their token that means we can't perform verification with Nucleus. Safest route is to keep them online.
            TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: failed to refresh the auth token with error:" << ErrorHelp::getErrorName(err) << " for UserSessionId(" << userSessionId << ").");
            continue;
        }

        const char8_t* userAccessToken = tokenUtil.getAccessToken();
        const AccountId accountId = gUserSessionManager->getAccountId(userSessionId);
        const ClientType clientType = gUserSessionManager->getClientType(userSessionId);
        
        
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Access entitlement check
        bool entitlementStatusCheckPassed = true;
        
        // skipAutoGrantEntitlement=true to skip auto entitlement granting as potentially this can be enabled for consoles as well, and we don't want auto-entitlement granting to apply unexpectedly while we're performing entitlement check on consoles
        CheckEntitlementUtil checkEntUtil(*this, clientType);
        TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: performing entitlement check for UserSessionId(" << userSessionId << "), clientType(" << clientType << ").");
        err = commonEntitlementCheck(accountId, checkEntUtil, clientType, productName, nullptr, userAccessToken, tokenUtil.getClientId(), true/* skipAutoGrantEntitlement */);
        if (err != ERR_OK)
        {
            //Nucleus Identity Resilience: Soft dependency on entitlement check. If rpc failed because of unavailable Nucleus service, blaze should ignore the error.
            if (oAuthSlaveImpl()->isNucleusServiceUnavailable(err))
            {
                WARN_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: Entitlement check failed with error(" << ErrorHelp::getErrorName(err) << ") for UserSessionId(" << userSessionId << "). Ignoring the error due to Nucleus service is unavailable.");
            }
            else
            {
                ERR_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: Entitlement check failed with error(" << ErrorHelp::getErrorName(err) << ") for UserSessionId(" << userSessionId << ")");
                entitlementStatusCheckPassed = false;
            }
        }        

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // If entitlement status check failed, logout the user
        if (entitlementStatusCheckPassed)
        {
            TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: status check passed for UserSessionId(" << userSessionId << ").");
        }
        else
        {
            // Retrieve the local session again as the user might have been logged out by the previous blocking calls
            UserSessionMasterPtr localUserSession = gUserSessionMaster->getLocalSession(userSessionId);
            if (localUserSession)
            {
                TRACE_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: at least one status check failed. Kicking out user with UserSessionId(" << userSessionId << ").");
                gUserSessionMaster->destroyUserSession(userSessionId, DISCONNECTTYPE_ACCESS_REVOKED, 0, FORCED_LOGOUTTYPE_ACCESS_REVOKED);

                getMetricsInfoObj()->mForcedLogouts.increment(1, localUserSession->getProductName(), localUserSession->getClientType());
            }            
        }
    }

    INFO_LOG("[AuthenticationSlaveImpl].entitlementStatusCheck: entitlement status check completed for the qualified local usersessions.");
    
    mEntitlementStatusCheckInProgress = false;
}


// Authentication metrics
void AuthenticationMetrics::setLastAuthenticated(const char8_t* lastAuth)
{
    blaze_strnzcpy(mGaugeLastAuthenticated, lastAuth, sizeof(mGaugeLastAuthenticated));
}

void AuthenticationMetrics::addPerProductStatusInfo(const char8_t* metricName, ComponentStatus& status, const Metrics::TagPairList& tagList, uint64_t value)
{
    // the first tag *should* be ProductName
    auto& productMap = status.getInfoByProductName();
    Blaze::ComponentStatus::InfoMap* infoByProduct;
    const char8_t* productName = tagList[0].second.c_str();
    auto prodItr = productMap.find(productName);
    if (prodItr == productMap.end())
    {
        infoByProduct = productMap.allocate_element();
        productMap[productName] = infoByProduct;
    }
    else
    {
        infoByProduct = prodItr->second;
    }
    char8_t valueStr[64];
    blaze_snzprintf(valueStr, sizeof(valueStr), "%" PRIu64, value);

    char8_t key[256];
    size_t len = blaze_snzprintf(key, sizeof(key), "%s", metricName);

    for (auto i = 1; i < tagList.size(); i++)
    {
        len += blaze_snzprintf(key + len, sizeof(key) - len, "_%s", tagList[i].second.c_str());
    }

    (*infoByProduct)[key] = valueStr;
}

void AuthenticationMetrics::getStatusInfo(ComponentStatus& status) const
{
    Blaze::ComponentStatus::InfoMap& parentStatusMap=status.getInfo();
    char8_t buf[64];

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mLogins.getTotal());
    parentStatusMap["TotalLogins"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mGuestLogins.getTotal());
    parentStatusMap["TotalGuestLogins"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mLoginFailures.getTotal());
    parentStatusMap["TotalLoginFailures"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mLogouts.getTotal());
    parentStatusMap["TotalLogouts"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mForcedLogouts.getTotal());
    parentStatusMap["TotalForcedLogouts"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mSucceedCreatedAccounts.getTotal());
    parentStatusMap["TotalSucceedCreatedAccounts"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mFailedCreatedAccountsErrorUnknown.getTotal());
    parentStatusMap["TotalFailedCreatedAccountsErrorUnknown"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mSucceedKeyConsumption.getTotal());
    parentStatusMap["TotalAlreadyUsedCDKeysConsumptions"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mInvalidKeyConsumption.getTotal());
    parentStatusMap["TotalInvalidCDKeysConsumptions"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mUsedKeyConsumption.getTotal());
    parentStatusMap["TotalOtherCDKeyConsumptionError"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mUnexpectedKeyConsumption.getTotal());
    parentStatusMap["TotalSucceedCDKeysConsumptions"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mAlreadyUsedCodeConsumptions.getTotal());
    parentStatusMap["TotalAlreadyUsedCodeConsumptions"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mInvalidCodeConsumptions.getTotal());
    parentStatusMap["TotalInvalidCodeConsumptions"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mSucceedCodeConsumptions.getTotal());
    parentStatusMap["TotalSucceedCodeConsumptions"]=buf;

    blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, mOtherConsumptionError.getTotal());
    parentStatusMap["TotalOtherConsumptionError"]=buf;

    mLogins.iterate([&status](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { addPerProductStatusInfo("TotalLogins", status, tagList, value.getTotal()); });

    mGuestLogins.iterate([&status](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { addPerProductStatusInfo("TotalGuestLogins", status, tagList, value.getTotal()); });

    mLoginFailures.iterate([&status](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { addPerProductStatusInfo("TotalLoginFailures", status, tagList, value.getTotal()); });

    mLogouts.iterate([&status](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { addPerProductStatusInfo("TotalLogouts", status, tagList, value.getTotal()); });

    mForcedLogouts.iterate([&status](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { addPerProductStatusInfo("TotalForcedLogouts", status, tagList, value.getTotal()); });

    mPsuCutoff.iterate([&status](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { addPerProductStatusInfo("PSUCutoff", status, tagList, value.get()); });

    mSandboxSuccesses.iterate([this, &parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            char8_t key[256];
            blaze_snzprintf(key, sizeof(key), "%s_%s_%s", mSandboxSuccesses.getName().c_str(), tagList[0].second.c_str(), tagList[1].second.c_str());
            char8_t buf[64];
            blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, value.getTotal());
            parentStatusMap[key]=buf;
        }
    );

    mSandboxFailures.iterate([this, &parentStatusMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            char8_t key[256];
            blaze_snzprintf(key, sizeof(key), "%s_%s_%s", mSandboxFailures.getName().c_str(), tagList[0].second.c_str(), tagList[1].second.c_str());
            char8_t buf[64];
            blaze_snzprintf(buf,sizeof(buf),"%" PRIu64, value.getTotal());
            parentStatusMap[key]=buf;
        }
    );
}

HttpConnectionManagerPtr AuthenticationSlaveImpl::getConnectionManagerPtr(ConnectionManagerType type) const
{
    if (type != ConnectionManagerType::MAX)
        return gOutboundHttpService->getConnection(sPoolNames[type]);
    else
        return HttpConnectionManagerPtr();
}

void AuthenticationSlaveImpl::obfuscatePlatformInfo(ClientPlatformType platform, EA::TDF::Tdf& tdf) const
{
    switch (tdf.getTdfId())
    {
    case UserLoginInfo::TDF_ID:
        return UserSessionManager::obfuscate1stPartyIds(platform, static_cast<UserLoginInfo*>(&tdf)->getPlatformInfo());
    case LoginResponse::TDF_ID:
        return UserSessionManager::obfuscate1stPartyIds(platform, static_cast<LoginResponse*>(&tdf)->getUserLoginInfo().getPlatformInfo());
    default:
        ASSERT_LOG("[AuthenticationSlaveImpl].obfuscatePlatformInfo: Platform info obfuscation not implemented for Tdf class " << tdf.getFullClassName());
        break;
    }
}

} // Authentication
} // Blaze
