/*************************************************************************************************/
/*!
    \file   utilslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UtilSlaveImpl

    Util Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

// framework includes
#include "framework/connection/endpoint.h"
#include "framework/connection/session.h"
#include "framework/component/componentmanager.h"
#include "framework/component/peerinfo.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/database/dbconfig.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/authorization.h"
#include "framework/connection/connectionmanager.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/userinfo.h"
#include "framework/util/locales.h"
#include "framework/util/localization.h"
#include "framework/util/profanityfilter.h"
#include "util/rpc/utilslave/usersettingsloadallforuserid_stub.h"
#include "authentication/authenticationimpl.h"

// util includes
#include "util/utilslaveimpl.h"
#include "time.h"

// md5 support for telemetry
#include "framework/lobby/lobby/cryptmd5.h"
#include "framework/lobby/lobby/lobbytagfield.h"


namespace Blaze
{
namespace Util
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
static const char8_t USER_SETTINGS_SAVE_QUERY[] =
    "INSERT INTO `util_user_small_storage` (`id`, `key`, `data`, `created_date`)"
    " VALUES (?,?,?, NOW())"
    " ON DUPLICATE KEY UPDATE `data`=?";

static const char8_t USER_SETTINGS_LOAD_QUERY[] =
    "SELECT `id`, `key`, `data`"
    " FROM `util_user_small_storage`"
    " WHERE `id` = ? AND `key` = ?";

const char8_t* UTIL_CLIENT = "clientConfigs";
const char8_t* UTIL_CLIENT_BLAZE_SDK = "BlazeSDK";

static const uint32_t TUNEABLE_TELEMETRY_MAX_DEPTH_STEPS = 10;

/*** Factory *************************************************************************************/
// static
UtilSlave* UtilSlave::createImpl()
{
    return BLAZE_NEW_NAMED("UtilSlaveImpl") UtilSlaveImpl();
}

/*** Public Methods ******************************************************************************/
UtilSlaveImpl::UtilSlaveImpl()
    : mLastTickerServIndex(0),
    mClientConnectCount(getMetricsCollection(), "connectionCountClient", Metrics::Tag::blaze_sdk_version, Metrics::Tag::product_name),
    mMaxPsuExceededCount(getMetricsCollection(), "maxPsuExceededCount", Metrics::Tag::client_type),
    mAccessibilityMetrics(getMetricsCollection())
{
}

UtilSlaveImpl::~UtilSlaveImpl()
{
}

bool UtilSlaveImpl::onValidateConfig(UtilConfig& config, const UtilConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    // check telemetry
    checkTelemetryConfig(config, validationErrors);

    // check ticker
    checkTickerConfig(config, validationErrors);

    return validationErrors.getErrorMessages().empty();

} // onValidateConfig

bool UtilSlaveImpl::onConfigure()
{
    return configComponent();

} // onConfigure

bool UtilSlaveImpl::onReconfigure()
{
    return configComponent(true);
} // onReconfigure

bool UtilSlaveImpl::onPrepareForReconfigure(const UtilConfig& config)
{
    return parseAuditPersonasConfig(getConfig().getTelemetry(), mTempAuditBlazeIds);
}

bool UtilSlaveImpl::onResolve()
{
    return true;
}

void UtilSlaveImpl::onShutdown()
{
}

/*** Protected Methods ***************************************************************************/
/*** Private Methods *****************************************************************************/

bool UtilSlaveImpl::configComponent(bool reconfig)
{
    return configUserSmallStorage(reconfig)
           && configTelemetryConfig(reconfig)
           && configTickerConfig();
} // configComponent

bool UtilSlaveImpl::configUserSmallStorage(bool reconfig)
{
    if (reconfig)
        return true;

    // Configure per-platform dbs
    for (DbIdByPlatformTypeMap::const_iterator itr = getDbIdByPlatformTypeMap().begin(); itr != getDbIdByPlatformTypeMap().end(); ++itr)
    {
        if (itr->first != ClientPlatformType::common)
        {
            // Configure prepared statements.
            uint32_t dbId = itr->second;
            QueryIds& arr = mStatementIds.insert(dbId).first->second;
            arr[USER_SETTINGS_LOAD] = gDbScheduler->registerPreparedStatement(dbId,
                "user_settings_load", USER_SETTINGS_LOAD_QUERY);
            arr[USER_SETTINGS_SAVE] = gDbScheduler->registerPreparedStatement(dbId,
                "user_settings_save", USER_SETTINGS_SAVE_QUERY);
        }  // if
    }

    return true;
} // checkUserSmallStorage

PreparedStatementId UtilSlaveImpl::getQueryId(uint32_t dbId, QueryId id) const
{
    QueryIdsByDbIdMap::const_iterator itr = mStatementIds.find(dbId);
    if (itr == mStatementIds.end())
        return INVALID_PREPARED_STATEMENT_ID;

    return itr->second[id];
}

bool UtilSlaveImpl::configTickerConfig()
{
    mLastTickerServIndex = 0;

    return true;
} // configTickerAndTelemetryconfig

bool UtilSlaveImpl::configTelemetryConfig(bool isReconfigure)
{
    if (isReconfigure)
    {
        mAuditBlazeIds.swap(mTempAuditBlazeIds);
    }
    else
    {
        parseAuditPersonasConfig(getConfig().getTelemetry(), mAuditBlazeIds);
    }

    return true;
} // configTelemetryconfig

typedef eastl::vector<const char8_t*> PersonaNameVector;
bool UtilSlaveImpl::parseAuditPersonasConfig(const TelemetryData& config, BlazeIdSet& auditBlazeIds)
{
    //cache off the blazeids for users in the audit list for faster lookup
    const TelemetryData::PersonaNameList& auditPersonaList = config.getDisconnectAuditPersonas();
    if (auditPersonaList.empty())
    {
        auditBlazeIds.clear();
    }
    else
    {
        PersonaNameVector personas;
        StringBuilder personaNamesString(nullptr);
        personas.reserve(auditPersonaList.size());
        for (TelemetryData::PersonaNameList::const_iterator i = auditPersonaList.begin(), e = auditPersonaList.end(); i != e; ++i)
        {
            const char8_t* personaName = i->c_str();
            personas.push_back(personaName);
            personaNamesString << personaName;
            personaNamesString << ",";
        }
        personaNamesString.trim(1); // trim last comma

        // Resolve persona names into blaze IDs
        // Note: Audit logging via UtilConfig is deprecated (replaced by the enableAuditLogging RPC) --
        // so to preserve legacy behaviour and avoid adding new options (e.g. platform) to a deprecated config field,
        // this lookup just fetches all users with the given persona name (in the primary namespace for their platform).
        UserInfoListByPersonaNameByNamespaceMap results;
        BlazeRpcError err = gUserSessionManager->lookupUserInfoByPersonaNamesMultiNamespace(personas, results, true /*primaryNamespaceOnly*/, false /*restrictCrossPlatformResults*/);
 
        if (err == Blaze::ERR_OK)
        {
            auditBlazeIds.clear();
            for (const auto& namespaceItr : results)
            {
                for (const auto& personaItr : namespaceItr.second)
                {
                    for (const auto& userItr : personaItr.second)
                        auditBlazeIds.insert(userItr->getId());
                }
            }
        }
        else
        {
            BLAZE_WARN_LOG(Log::UTIL, "[UtilSlaveImp].parseAuditPersonasConfig: "
                "Failed to lookup audit personas(" << personaNamesString.get() << "), error=" 
                << Blaze::ErrorHelp::getErrorName(err) << " current audit settings remain in effect.");
            return false;
        }
    }
    return true;
}

// Helper function to configure a telemetry server
bool UtilSlaveImpl::checkTelemetryServers(const TelemetryElement& pTelElement, ConfigureValidationErrors& validationErrors) const
{
    if (pTelElement.getAddress()[0] == '\0')
    {
        eastl::string msg;
        msg.sprintf("[UtilSlaveImpl].checkTelemetryServers: Server address is missing in configuration file.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if (pTelElement.getPort() == 0)
    {
        eastl::string msg;
        msg.sprintf("[UtilSlaveImpl].checkTelemetryServers: Server port is missing in configuration file.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if (pTelElement.getSecret()[0] == '\0')
    {
        eastl::string msg;
        msg.sprintf("[UtilSlaveImpl].checkTelemetryServers: Secret string is missing in configuration file.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if (pTelElement.getDomain()[0] == '\0')
    {
        eastl::string msg;
        msg.sprintf("[UtilSlaveImpl].checkTelemetryServers: Telemetry server's domain is missing in configuration file.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    
    if (strlen(pTelElement.getDomain()) > MAX_TELEM_DOMAIN_LEN)
    {
        INFO_LOG("[UtilSlaveImpl].checkTelemetryServers: Telemetry domain '" << pTelElement.getDomain() 
                 << "' is too long (" << strlen(pTelElement.getDomain()) << " chars).  Truncating to " << MAX_TELEM_DOMAIN_LEN << " chars.");
    }

    return true;
} // checkTelemetryServers

// Check to see whether a giving group definition applies to a user
// blazeId represents either a NucleusId or a PersonaId, but this is irrelevant and checking whether both the blazeId and groupDefinition are dealing with the same
// type of id should be done before using this function
// written by AMakoukji, Jan 2011

 
bool UtilSlaveImpl::checkUserInGroup(const char* userIdType, const char* groupDefinition, uint32_t uStep) const 
{
    // First, check the uStep, to make sure we haven't passed a reasonable number.
    // If the step gets too big we might have 2 groups that contain each other in their definitions.
    // This would cause an infinite loop if left unchecked.
    if (uStep > TUNEABLE_TELEMETRY_MAX_DEPTH_STEPS)
    {
        INFO_LOG("WARNING: Telemetry: checkUserInGroup() Maximum depth surpassed. Possible recursive group definition.\n" );
        return false;
    }

    if (strlen(groupDefinition) < 1)
        return false;

    // Get Persona and Nucleus IDs
    char strPersona[64];
    char strNucleusId[64];
    const char *blazeId = nullptr;

    blaze_snzprintf(strPersona, 64, "%" PRIi64,   gCurrentUserSession->getBlazeId());
    blaze_snzprintf(strNucleusId, 64, "%" PRIi64, gCurrentUserSession->getAccountId());

    if (blaze_stricmp(userIdType, "PersonaId") == 0)
        blazeId = strPersona;
    else if (blaze_stricmp(userIdType, "NucleusId") == 0 || blaze_stricmp(userIdType, "AccountId") == 0)
        blazeId = strNucleusId;
    else if (blaze_stricmp(userIdType, "Group") != 0)
        INFO_LOG("WARNING: Telemetry: checkUserInGroup() Undefined ID type.\n" );

    // The 3rd possibility is userIdType == "Group".
    // This possibility is covered checking  the data.
    // If, for example this function is called with userIdType "Persona", but groupDefinition looks like a 
    // group name, it will be treated as such. This allows other Ids in definitions to be fixed with 
    // group names, and even group names nested within group names

    // A pointer to the current groupDefinition member, used for parsing
    const char* currentTokenStart = groupDefinition;
    const char* currentTokenEnd = currentTokenStart;
    const char* currentLastRelevantChar = currentTokenEnd;
    uint32_t tokenLength = 0;

    // Get the first 'Token'
    currentTokenEnd = strchr(groupDefinition, ',');

    // Case: There are no delimiters, only a single item in the group definition
    while (currentTokenStart != nullptr && currentTokenStart[0] != '\0')
    {
        // Get the 'Token' edges
        currentTokenEnd = strchr(currentTokenStart, ',');

        // If this is the last or only token in the definition
        if (currentTokenEnd == nullptr)
        {
            currentTokenEnd = strchr(groupDefinition, '\0');
        }

        // Get rid of whitespace at the end of the token
        currentLastRelevantChar = currentTokenEnd;
        while (currentLastRelevantChar > currentTokenStart && currentLastRelevantChar[-1] == ' ')
        {
            --currentLastRelevantChar;
        }

        // Ignore whitespace
        while (currentTokenStart[0] != '\0' && currentTokenStart[0] == ' ')
        {
            ++currentTokenStart;
        }

        // Empty token check again, to see if it was all whitespace
        if (currentLastRelevantChar - currentTokenStart < 1)
        {
            currentTokenStart = currentTokenEnd;
            if (currentTokenStart != nullptr && currentTokenStart[0] != '\0')
                ++currentTokenStart;
            continue;
        }

        // Now, we will check for a group name match.
        // If the token starts with an letter between 'a' and 'z' or 'A' and 'Z', assume it is a group name
        // Cycle through the list of groups, if you find a match, recursively call this function with the group's groupDefinition
        if (currentTokenStart != nullptr)
        {
            if ((currentTokenStart[0] >= 'a' && currentTokenStart[0] <= 'z') || (currentTokenStart[0] >= 'A' && currentTokenStart[0] <= 'Z'))
            {
                const TuneableTelemetryGroupList& tuneableTelemGroups = getConfig().getTelemetry().getTuneableTelemetryGroups();
                tokenLength = (uint32_t)(currentLastRelevantChar - currentTokenStart);
                for (TuneableTelemetryGroupList::const_iterator it = tuneableTelemGroups.begin(); it != tuneableTelemGroups.end(); ++it)
                {
                    //Look for a match
                    if (tokenLength == strlen((*it)->getGroupName()) && blaze_strnicmp((*it)->getGroupName(), currentTokenStart, tokenLength) == 0)
                    {
                        // Found a matching name
                        if (checkUserInGroup((*it)->getIdentifierType(), (*it)->getGroupDefinition(), uStep + 1))
                        {
                            return true;
                        }
                        else
                        {
                            INFO_LOG("WARNING: Telemetry: Tuneable telemetry configuration makes use of an undefined group name.\n");
                            currentTokenStart = currentTokenEnd;
                            if (currentTokenStart != nullptr && currentTokenStart[0] != '\0')
                                ++currentTokenStart;
                            continue;
                        }
                    }
                }
            }
        }

        // Now, error check, if blazeId is nullptr, and the groups are check, there is no possible match with the AccountId or PersonaId
        if (blazeId == nullptr)
        {
            currentTokenStart = currentTokenEnd;
            if (currentTokenStart != nullptr && currentTokenStart[0] != '\0')
                ++currentTokenStart;
            continue;
        }

        // Next do a wildcard check: if the first character is a wildcard '%'
        if (currentTokenStart != nullptr && currentTokenStart[0] == '%')
        {
            ++currentTokenStart;
            tokenLength = (uint32_t)(currentLastRelevantChar - currentTokenStart);

            // Again an empty for for a token that is JUST '%'
            if (tokenLength < 1)
            {
                currentTokenStart = currentTokenEnd;
                if (currentTokenStart != nullptr && currentTokenStart[0] != '\0')
                    ++currentTokenStart;
                continue;
            }

            if (blaze_strnicmp(blazeId + strlen(blazeId) - tokenLength, currentTokenStart, tokenLength) == 0)
            {
                return true ;
            }
        }

        // Finaly, do the exact match compare, check if the token is identical to the blazeId
        tokenLength = (uint32_t)(currentLastRelevantChar - currentTokenStart);
        if (strlen(blazeId) == tokenLength)
        {
            if (blaze_strnicmp(blazeId, currentTokenStart, tokenLength ) == 0)
            {
                return true;
            }
        }

        currentTokenStart = currentTokenEnd;
        if (currentTokenStart != nullptr && currentTokenStart[0] != '\0')
            ++currentTokenStart;
    }

    return false;
} // checkUserInGroup


bool UtilSlaveImpl::checkUserInGroup(const char* userIdType, const char* groupDefinition) const 
{
    // Call the checkUserInGroup function with a step of ZERO
    return checkUserInGroup(userIdType, groupDefinition, 0);
}

// Configures all telemetry servers 
bool UtilSlaveImpl::checkTelemetryConfig(UtilConfig& config, ConfigureValidationErrors& validationErrors) const
{
    for (auto platform : gController->getHostedPlatforms())
    {
        if (config.getTelemetry().getTelemetryServers().find(platform) != config.getTelemetry().getTelemetryServers().end())
        {
            const TelemetryElementsByProduct& telServersByProd = *(config.getTelemetry().getTelemetryServers().find(platform)->second);
            for (TelemetryElementsByProduct::const_iterator it = telServersByProd.begin(); it != telServersByProd.end(); ++it)
            {
                const char8_t* serviceName = it->first;
                bool validServiceName = gController->isServiceHosted(serviceName);

                if (!validServiceName)
                {
                    eastl::string msg;
                    msg.sprintf("[UtilSlaveImpl].checkTelemetryConfig: service name (%s) specified in util.telemetry.telemetryServers is not a configured service name for this server", serviceName);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }

                const TelemetryElementList& seqTelServs = *it->second;
                for (TelemetryElementList::const_iterator itr = seqTelServs.begin(); itr != seqTelServs.end(); ++itr)
                {
                    if (checkTelemetryServers(**itr, validationErrors) == false)
                    {
                        eastl::string msg;
                        msg.sprintf("[UtilSlaveImpl].checkTelemetryConfig: neither util.telemetry.anonymousServers or util.telemetry.telemetryServers found in config file.");
                        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                        str.set(msg.c_str());
                    }
                }
            }
        }

        if (config.getTelemetry().getAnonymousServers().find(platform) != config.getTelemetry().getAnonymousServers().end())
        {
            const TelemetryElementsByProduct& anonServersByProd = *(config.getTelemetry().getAnonymousServers().find(platform)->second);
            for (TelemetryElementsByProduct::const_iterator it = anonServersByProd.begin(); it != anonServersByProd.end(); ++it)
            {
                const char8_t* serviceName = it->first;
                bool validServiceName = gController->isServiceHosted(serviceName);

                if (!validServiceName)
                {
                    eastl::string msg;
                    msg.sprintf("[UtilSlaveImpl].checkTelemetryConfig: service name (%s) specified in util.telemetry.anonymousServers is not a configured service name for this server", serviceName);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
                const TelemetryElementList& seqAnonServs = *it->second;
                for (TelemetryElementList::const_iterator itr = seqAnonServs.begin(); itr != seqAnonServs.end(); ++itr)
                {
                    if (checkTelemetryServers(**itr, validationErrors) == false)
                    {
                        return false;
                    }
                }
            }
        }
    }
    
   

    return true;
} // checkTelemetryConfig

bool UtilSlaveImpl::checkTickerConfig(UtilConfig& config, ConfigureValidationErrors& validationErrors) const
{
    const TickerData& tickConfig = config.getTicker();
    const TickerServerDataList& seqTickServs = tickConfig.getTickerServers();

    for(TickerServerDataList::const_iterator it = seqTickServs.begin(); it != seqTickServs.end(); ++it)
    {
        if ((*it)->getAddress()[0] == '\0')
        {
            eastl::string msg;
            msg.sprintf("[UtilSlaveImpl].checkTickerConfig: Server address is missing in configuration file.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        if ((*it)->getPort() == 0)
        {
            eastl::string msg;
            msg.sprintf("[UtilSlaveImpl].checkTickerConfig: Server port is missing in configuration file.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        if ((*it)->getContext()[0] == '\0')
        {
            eastl::string msg;
            msg.sprintf("[UtilSlaveImpl].checkTickerConfig: Ticker server context string is missing in configuration file.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    return true;
} // checkTickerConfig

/*************************************************************************************************/
/*!
    \brief processFetchConfig

    Provides configuration data for the client.  Receives request from the client,
    retrieves the section requested and sends it to the client.

    \param[in]  request - Section
    \param[out] response - Config values from that section.

    \return - FetchConfigError::Error
*/
/*************************************************************************************************/
FetchClientConfigError::Error UtilSlaveImpl::processFetchClientConfig(
        const FetchClientConfigRequest &request, FetchConfigResponse &response, const Message *message)
{
    // get the overall config map
    const PlatformClientConfigMap& platformClientConfigMap = getConfig().getClientConfigs();

    // if we have a message, we lookup the platform and pull that config
    if (message != nullptr)
    {
        // get the service name's platform
        ClientPlatformType callerPlatform = gController->getServicePlatform(message->getPeerInfo().getServiceName());
        if (callerPlatform != ClientPlatformType::common)
        {
            // get the config block for the specific platform
            const auto platformClientConfigsBlockItr = platformClientConfigMap.find(callerPlatform);
            if (platformClientConfigsBlockItr != platformClientConfigMap.end())
            {
                const auto platformConfigMapItr = platformClientConfigsBlockItr->second->find(request.getConfigSection());
                if (platformConfigMapItr != platformClientConfigsBlockItr->second->end())
                {
                    //Copy the response to point to the config map values
                    response.setConfig(*platformConfigMapItr->second);
                    return FetchClientConfigError::ERR_OK;
                }
            }
        }
    }
    
    // we expect we'll have a common block
    const auto commonClientConfigsBlockItr = platformClientConfigMap.find(ClientPlatformType::common);
    if (commonClientConfigsBlockItr != platformClientConfigMap.end())
    {
        const auto commonConfigMapItr = commonClientConfigsBlockItr->second->find(request.getConfigSection());
        if (commonConfigMapItr != commonClientConfigsBlockItr->second->end())
        {
            //Copy the response to point to the config map values
            response.setConfig(*commonConfigMapItr->second);
            return FetchClientConfigError::ERR_OK;
        }
    }
    
    // the config section is not present in the common or platform specific map
    return FetchClientConfigError::UTIL_CONFIG_NOT_FOUND;

} // processFetchConfig


/*************************************************************************************************/
/*!
    \brief processLocalizeStrings
    message->getConnection().setLocale(request.getLocale());
    message->getConnection().setClientType(request.getClientType());

    Dunno.

    \param[in]  request - List of strings to localize
    \param[out] response - List of localized strings

    \return - LocalizeStringsError::Error
*/
/*************************************************************************************************/
LocalizeStringsError::Error UtilSlaveImpl::processLocalizeStrings(
    const LocalizeStringsRequest &request, LocalizeStringsResponse &response, const Message *message)
{
    uint32_t locale = request.getLocale();

    // if locale is set to zero, set locale to current connection's clocale
    if (0 == locale && message != nullptr)
        locale = message->getLocale();

    // loop through list of string ids and fill response map with answers
    const LocalizeStringsRequest::StringIdsList &pIdList= request.getStringIds();
    LocalizeStringsRequest::StringIdsList::const_iterator it = pIdList.begin();
    LocalizeStringsRequest::StringIdsList::const_iterator itend = pIdList.end();

    LocalizeStringsResponse::LocalizedStringsMap &pResponseMap = response.getLocalizedStrings();

    while(it != itend)
    {
        const char8_t *pId = it->c_str();
        it++;
        const char8_t *pRes = gLocalization->localize(pId, locale);

        // If the string id has been located in localization database (file loclize.csv)
        // the localized value will be returned; if not - the value would be the same as
        // string ID
        LocalizeStringsResponse::LocalizedStringsMap::value_type value(pId, pRes);
        pResponseMap.insert(value);
    }

    return LocalizeStringsError::ERR_OK;
} // processLocalizeStrings


/*************************************************************************************************/
/*!
    \brief processPing

    Accepts client pings to the server to keep the connection alive.

    \param[out] response - Sends back the server time.

    \return - PingError::Error
*/
/*************************************************************************************************/
PingError::Error UtilSlaveImpl::processPing(PingResponse& response, const Message *message)
{
    response.setServerTime(static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec()));
    return PingError::ERR_OK;
} // processPing


/*************************************************************************************************/
/*!
    \brief processSetClientData

    Sets some client data.

    \param[in]  request - Some client data (e.g. locale, type)

    \return - SetClientDataError::Error
*/
/*************************************************************************************************/
SetClientDataError::Error UtilSlaveImpl::processSetClientData(const ClientData &request, const Message *message)
{
    if (message == nullptr)
    {
        return SetClientDataError::ERR_SYSTEM;
    }

    PeerInfo& peerInfo = message->getPeerInfo();
    if (!peerInfo.isClient())
    {
        return SetClientDataError::ERR_SYSTEM;
    }

    const char8_t* serviceName = gController->verifyServiceName(request.getServiceName());
    if (serviceName == nullptr)
    {
        ERR_LOG("[UtilSlaveImpl::processSetClientData]: The service (" << request.getServiceName() << ") is not valid.");
        return SetClientDataError::ERR_SYSTEM;
    }
    peerInfo.setServiceName(serviceName);

    peerInfo.setBaseLocale(request.getLocale());
    uint32_t country = request.getCountry();
    if (country == 0)
    {
        // backward compatibility for clients that don't have (or won't provide) the country setting
        country = (uint32_t)LocaleTokenGetCountry(request.getLocale());
    }
    peerInfo.setBaseCountry(country);
    peerInfo.setClientType(request.getClientType());
    peerInfo.setIgnoreInactivityTimeout(request.getIgnoreInactivityTimeout()); //Inactivity timeout is only enabled if allowed by server configuration.

    if (gCurrentUserSession != nullptr)
    {
        gCurrentUserSession->setSessionLocale(request.getLocale());
    }

    return SetClientDataError::ERR_OK;
} // processSetClientData

/*************************************************************************************************/
/*!
     \brief telemetrySecretCreate

     Create a telemetry secret key based on the provided information.

     \param[in] pSecret Secret string shared between servers.
     \param[in] uAddr Address of client that secret is being created for.
     \param[in] pPers Persona of client.
     \param[in] pUid EA.com Nucleus Account ID of client.
     \param[in] pDP Domain partition of server.
     \param[in] uTime Current time in epoch seconds.
     \param[in] uLocality Client's locality.
     \param[in] pOut Output buffer to encode secret into.
     \param[in] iLen Length of pOut.

     \return Length of encoded secret.

*/
/*************************************************************************************************/
size_t UtilSlaveImpl::telemetrySecretCreate(
        const char *pSecret,
        uint32_t uAddr,
        const char *pPers,
        const char *pUid,
        const char *pDP,
        uint32_t uTime,
        int32_t uLocality,
        char *pOut,
        int32_t iLen)
{
    CryptMD5T md5;
    char strIP[4];
    char strTmp[256];
    char strTime[4];
    int32_t iTmpLen;

    // ATTENTION
    // IP Addresses in lobby are in reverse byte order
    // the highest byte is the the first number in IP address
    // whereas in blaze first number is the last number in IP address
    strIP[3] = (char)((uAddr >> 24) & 0xff);
    strIP[2] = (char)((uAddr >> 16) & 0xff);
    strIP[1] = (char)((uAddr >> 8) & 0xff);
    strIP[0] = (char)(uAddr & 0xff);

    strTime[0] = (char)((uTime >> 24) & 0xff);
    strTime[1] = (char)((uTime >> 16) & 0xff);
    strTime[2] = (char)((uTime >> 8) & 0xff);
    strTime[3] = (char)(uTime & 0xff);

    // Build MD5 sum from provided info
    CryptMD5Init(&md5);
    CryptMD5Update(&md5, pSecret, -1);
    CryptMD5Update(&md5, pPers, -1);
    CryptMD5Update(&md5, pUid, -1);
    CryptMD5Update(&md5, pDP, -1);
    CryptMD5Update(&md5, strIP, 4);
    CryptMD5Update(&md5, strTime, 4);
    CryptMD5Final(&md5, strTmp, MD5_BINARY_OUT);

    // As of GOSREQ-625, don't send MAC address (2nd field).
    // But add a comma in its place so the secret can still be parsed correctly
    iTmpLen = TagFieldPrintf(strTmp + MD5_BINARY_OUT,
            sizeof(strTmp) - MD5_BINARY_OUT, "%s,,%s,%s,%d,%t",
            pPers, pUid, pDP, uTime, uLocality);
    TagFieldSetBinary7(pOut, iLen, nullptr, strTmp, MD5_BINARY_OUT + iTmpLen + 1);
    return (strlen(pOut));
}

/*************************************************************************************************/
/*!
     \brief encodeTelemetrySessionID

     Encode a telemetry session ID into a char buffer.

     \param[in] pOut Session ID buffer.
     \param[in] iLen Length of pBuf.
     \param[in] uSessID Session ID to encode.

     \return True if successful.

*/
/*************************************************************************************************/
bool UtilSlaveImpl::encodeTelemetrySessionID(char8_t *pOut, int32_t iLen, uint64_t uSessID)
{
    if(pOut == nullptr || iLen < 1) 
        return false;

    char *pBuf = pOut;
    const char *pLast = pOut + iLen - 1;

    char code = 0;
    while(pBuf < pLast && uSessID != 0)
    {
        //Each char is a 6-bit character code
        code = (char)(uSessID & 0x3F);
        if(code <= 25)  // 'A' - 'Z'
        {
            *(pBuf++) = 'A' + code;
        } 
        else if(code <= 51)  // 'a' - 'z'
        {
            *(pBuf++) = 'a' + (code - 26);
        } 
        else if(code <= 61)  // '0' - '9'
        {
            *(pBuf++) = '0' + (code - 52);
        } 
        else if(code == 62) // '+'
        {
            *(pBuf++) = '+';
        } 
        else    // '/'
        {
            *(pBuf++) = '/';
        }
        uSessID = (uSessID >> 6);
    }
    *pBuf = 0;    //Null-terminate

    return true;
}

/*************************************************************************************************/
/*!
    \brief processGetTelemetryServer

    Gets parameters for connecting to telemetry server.

    \param[out] response - Parameters to connect to telemetry server.

    \return - GetTelemetryServerError::Error
*/
/*************************************************************************************************/
GetTelemetryServerError::Error UtilSlaveImpl::processGetTelemetryServer(
        const GetTelemetryServerRequest &request,
        GetTelemetryServerResponse &response,
        const Message* message)
{
    if (message == nullptr)
        return GetTelemetryServerError::ERR_SYSTEM;
    return processGetTelemetryServerImpl(request, response, message->getPeerInfo());
}

GetTelemetryServerError::Error UtilSlaveImpl::processGetTelemetryServerImpl(const GetTelemetryServerRequest &request,
        GetTelemetryServerResponse &response, 
        Blaze::PeerInfo& peerInfo)
{

    const int32_t cstKeyBufLen = 512;
    const int32_t cstSessBufLen = 32;
    GetTelemetryServerError::Error res = GetTelemetryServerError::ERR_OK;

    // Common settings that apply to all servers
    response.setDisable(getConfig().getTelemetry().getTelemetryDisable());
    response.setNoToggleOk(getConfig().getTelemetry().getTelemetryNoToggleOk());
    response.setSendPercentage(getConfig().getTelemetry().getTelemetrySendPercentage());
    response.setSendDelay(getConfig().getTelemetry().getTelemetrySendDelay());
    response.setUseServerTime(getConfig().getTelemetry().getTelemetryUseServerTime());

    char8_t filterBuffer[1024];
    filterBuffer[0] = '\0';

    blaze_strnzcat(filterBuffer, getConfig().getTelemetry().getTelemetryFilter(), sizeof(filterBuffer));
    const TuneableTelemetryFilterList& tuneableTelemFilters = getConfig().getTelemetry().getTuneableTelemetryFilters();

    for (TuneableTelemetryFilterList::const_iterator it = tuneableTelemFilters.begin(); it != tuneableTelemFilters.end(); ++it)
    {
        // For each filter, check if it applies to you
        if (!(*it)->getActionAsTdfString().empty())
        {
            if (checkUserInGroup((*it)->getIdentifierType(), (*it)->getBlazeIds()))
            {
                // Matched user's id defined by identifier type
                if (filterBuffer[0] != '\0')
                    blaze_strnzcat(filterBuffer, ",", sizeof(filterBuffer));
                blaze_strnzcat(filterBuffer, (*it)->getAction(), sizeof(filterBuffer));
            }
        }
    }

    // Set the filter setting now thats it is built
    response.setFilter(filterBuffer);


    // Create secret key buffer
    char *pKeyBuf = (char8_t*) BLAZE_ALLOC(cstKeyBufLen);
    if (pKeyBuf == nullptr)
    {
        return  GetTelemetryServerError::UTIL_TELEMETRY_OUT_OF_MEMORY;
    }
    pKeyBuf[0] = '\0';

    // Create session ID buffer
    char *pSessBuf = (char8_t*) BLAZE_ALLOC(cstSessBufLen);
    if (pSessBuf == nullptr)
    {
        return  GetTelemetryServerError::UTIL_TELEMETRY_OUT_OF_MEMORY;
    }
    pSessBuf[0] = '\0';

    // Use the default service name if the client is not logged in
    
    const char8_t *serviceName = 
        (gCurrentUserSession != nullptr) ? gCurrentUserSession->getServiceName() : peerInfo.getServiceName(); // Note: peerInfo is available for unauthenticated clients.

    if (request.getServiceName()[0] != '\0')
    {
        // Override the service name with the one the client sends up
        serviceName = request.getServiceName();
    }

    ClientPlatformType platform = gController->getServicePlatform(serviceName);
    if (!gController->isPlatformHosted(platform)
        && platform != common)
    {
        ERR_LOG("[UtilSlaveImpl::processGetTelemetryServerImpl]: The platform (" << ClientPlatformTypeToString(platform) << ") of caller's service (" << serviceName << ") is not hosted by this instance.");
        return GetTelemetryServerError::UTIL_CALLER_PLATFORM_NOT_FOUND;
    }

    if (getConfig().getTelemetry().getTelemetryServers().find(platform) == getConfig().getTelemetry().getTelemetryServers().end()
        && getConfig().getTelemetry().getAnonymousServers().find(platform) == getConfig().getTelemetry().getAnonymousServers().end())
    {
        ERR_LOG("[UtilSlaveImpl::processGetTelemetryServerImpl]: The platform (" << ClientPlatformTypeToString(platform) << ") of caller's service (" << serviceName << ") is not configured with any telemetry servers.");
        return GetTelemetryServerError::UTIL_CALLER_PLATFORM_NOT_FOUND;
    }

    uint32_t locale = peerInfo.getLocale();
    uint32_t country = peerInfo.getCountry();
    // use the account settings if we have it
    if (gCurrentUserSession != nullptr && gCurrentUserSession->getAccountLocale() != 0 && gCurrentUserSession->getAccountLocale() != LOCALE_BLANK)
    {
        // assume we have country if we have locale
        locale = gCurrentUserSession->getAccountLocale();
        country = gCurrentUserSession->getAccountCountry();
    }
    // replacing the language qualifier with country here to maintain pre-locale-cleanup behavior (which used an amalgamated "locale"),
    // but we'll need to work with Telemetry to see if this is what they really want
    LocaleTokenSetCountry(locale, country);

    // Return either authenticated telemetry config or anonymous telemetry config depending on whether user is logged in.
    bool hasTelemetryServerByProd = false;
    {
        auto platformMapIter = getConfig().getTelemetry().getTelemetryServers().find(platform);
        if (platformMapIter != getConfig().getTelemetry().getTelemetryServers().end())
        {
            const TelemetryElementsByProduct& telServersByProd = *(platformMapIter->second);
            hasTelemetryServerByProd = (telServersByProd.find(serviceName) != telServersByProd.end());
        }
    }

    bool hasTelemetryServerAnon = false;
    {
        auto platformMapIter = getConfig().getTelemetry().getAnonymousServers().find(platform);
        if (platformMapIter != getConfig().getTelemetry().getAnonymousServers().end())
        {
            const TelemetryElementsByProduct& anonServersByProd = *(platformMapIter->second);
            hasTelemetryServerAnon = (anonServersByProd.find(serviceName) != anonServersByProd.end());
        }
    }


    size_t keyLen = 0;
    if ((gCurrentUserSession != nullptr) && hasTelemetryServerByProd)
    {
        const TelemetryElementList& seqTelServs = *(getConfig().getTelemetry().getTelemetryServers().find(platform)->second->find(serviceName)->second); // This giant unreadable sentence is being protected by hasTelemetryServerByProd to avoid changing too much of the current unreadable code.   
        if (seqTelServs.empty())
        {
            res = GetTelemetryServerError::UTIL_TELEMETRY_NO_SERVERS_AVAILABLE;
        }
        else
        {
            // Choose a random telemetry server
            const TelemetryElement *telElement = seqTelServs.at(Random::getRandomNumber(seqTelServs.size())); // getRandomNumber is optimized for size <=1 and does not include the upper bound in the range.

            // Save telemetry server settings to response
            response.setAddress(telElement->getAddress());
            response.setPort(telElement->getPort());
            response.setLocale(locale);
            response.setCountry(country);
            response.setIsAnonymous(false);
            response.setTelemetryServiceName(getConfig().getTelemetry().getTelemetryServiceName());
            if (gCurrentLocalUserSession)
            {
                response.setUnderage(gCurrentLocalUserSession->getUserInfo().getIsChildAccount());
            }
            else
            {
                response.setUnderage(true);
            }

            // Get data necessary for the key generation
            uint32_t uAddr = 0;
            const char *pPers = gCurrentLocalUserSession->getUserInfo().getPersonaName();
            const UserInfo& pInfo = gCurrentLocalUserSession->getUserInfo();
            char pUid[32];
            blaze_snzprintf(pUid, sizeof(pUid), "$%016" PRIx64, pInfo.getId());
            char8_t pDP[MAX_TELEM_DOMAIN_LEN];
            pDP[0] = '\0';
            blaze_strnzcat(pDP, telElement->getDomain(), MAX_TELEM_DOMAIN_LEN);

            uint32_t uTime = static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec());
            uint32_t uLocality = gCurrentUserSession->getSessionLocale();

            // Generate the session ID
            uint64_t uSessID = (((uint64_t)pInfo.getId()) << 32) + uTime;
            if(encodeTelemetrySessionID(pSessBuf, (size_t)cstSessBufLen, uSessID))
            {
                response.setSessionID(pSessBuf);
            }

            keyLen = telemetrySecretCreate(
                telElement->getSecret(),
                uAddr,
                pPers,
                pUid,
                pDP,
                uTime,
                uLocality,
                pKeyBuf,
                cstKeyBufLen);

            if (keyLen >= (size_t)cstKeyBufLen)
                res = GetTelemetryServerError::UTIL_TELEMETRY_KEY_TOO_LONG;
        }
    }
    else
    {
        if (hasTelemetryServerAnon)
        {
            const TelemetryElementList& seqAnonServs = *(getConfig().getTelemetry().getAnonymousServers().find(platform)->second->find(serviceName)->second); // This giant unreadable sentence is being protected by hasTelemetryServerAnon to avoid changing too much of the current unreadable code.
            if (seqAnonServs.empty())
            {
                res = GetTelemetryServerError::UTIL_TELEMETRY_NO_SERVERS_AVAILABLE;
            }
            else
            {
                // Choose a random telemetry server
                const TelemetryElement *telElement = seqAnonServs[Random::getRandomNumber(seqAnonServs.size())];

                // Save telemetry server settings to response
                response.setAddress(telElement->getAddress());
                response.setPort(telElement->getPort());
                response.setLocale(locale);
                response.setCountry(country);
                response.setIsAnonymous(true);

                //For anonymous telemetry, supply the domain and time as-is instead of a shared secret
                uint32_t uTime = static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec());
                char8_t pDP[MAX_TELEM_DOMAIN_LEN];
                pDP[0] = '\0';
                blaze_strnzcat(pDP, telElement->getDomain(), MAX_TELEM_DOMAIN_LEN);
                keyLen = blaze_snzprintf(pKeyBuf,cstKeyBufLen,"%s:%u",pDP,uTime);
            }
        }
        else
        {
            res = (hasTelemetryServerByProd)
                ? GetTelemetryServerError::ERR_AUTHORIZATION_REQUIRED : GetTelemetryServerError::UTIL_TELEMETRY_NO_SERVERS_AVAILABLE;
        }
    }

    if (keyLen < cstKeyBufLen)
    {
        if (request.getUseKey2())
            response.getKey2().setData((uint8_t*)(pKeyBuf), keyLen);
        else
            response.setKey(pKeyBuf);
    }

    BLAZE_FREE(pKeyBuf);
    BLAZE_FREE(pSessBuf);

    DisconnectTelemetryLogType telemType = getConfig().getTelemetry().getDisconnectTelemetryType();
    bool disconnectTelemetryEnabled = (telemType == DISCONNECT_TELEMETRY_ALL_USERS) || ((telemType == DISCONNECT_TELEMETRY_AUDIT_USERS) && 
        (gUserSessionManager->shouldAuditSession(gCurrentUserSession->getSessionId()) || (mAuditBlazeIds.find(gCurrentUserSession->getBlazeId()) != mAuditBlazeIds.end())));

    response.setEnableDisconnectTelemetry(disconnectTelemetryEnabled);

    // Specify PIN Configurations
    response.setPinEnvironment(getConfig().getTelemetry().getPINConfig().getEnvironment());
    response.setPinUrl(getConfig().getTelemetry().getPINConfig().getUrl());

    return res;
}

/*************************************************************************************************/
/*!
    \brief processGetTickerServer

    Gets parameters for connecting to ticker server.

    \param[out] response - Parameters to connect to ticker server.

    \return - GetTickerServerError::Error
*/
/*************************************************************************************************/
GetTickerServerError::Error UtilSlaveImpl::processGetTickerServer(GetTickerServerResponse &response, const Message* message)
{
    // buffer length for generated key
    const int32_t cstKeyBufLen = 512;

    const TickerData& tickConfig = getConfig().getTicker();
    const TickerServerDataList& seqTickServs = tickConfig.getTickerServers();

    if (seqTickServs.size() == 0)
        return GetTickerServerError::UTIL_TICKER_NO_SERVERS_AVAILABLE;

    const TickerServerData *tsConf = seqTickServs[mLastTickerServIndex++];

    if (mLastTickerServIndex == seqTickServs.size())
        mLastTickerServIndex = 0;

    response.setAddress(tsConf->getAddress());
    response.setPort(tsConf->getPort());

    // key generation
    eastl::string strKey;
    strKey.sprintf("%" PRId64 ",%s:%d,%s,%d,%d,%d,%d,%d,%d,%d",
            gCurrentUserSession->getBlazeId(),
            tsConf->getAddress(),
            tsConf->getPort(),
            tsConf->getContext(),                      // cutom part
            tsConf->getHelloperiod(),
            tsConf->getBgred(),
            tsConf->getBggreen(),
            tsConf->getBgblue(),
            tsConf->getBgalpha(),
            tsConf->getHfilter(),
            tsConf->getLfilter());

    GetTickerServerError::Error res = GetTickerServerError::ERR_OK;

    if (strKey.length() < (eastl::string::size_type)cstKeyBufLen)
        response.setKey(strKey.c_str());
    else
        res = GetTickerServerError::UTIL_TICKER_KEY_TOO_LONG;

    return res;
}

FilterForProfanityError::Error UtilSlaveImpl::processFilterForProfanity(const Blaze::Util::FilterUserTextResponse &request, Blaze::Util::FilterUserTextResponse &response, const Message* message)
{
    Blaze::Util::FilterUserTextResponse::FilteredUserTextList::const_iterator it=request.getFilteredTextList().begin();    
    Blaze::Util::FilterUserTextResponse::FilteredUserTextList::const_iterator itEnd=request.getFilteredTextList().end();
    for (; it != itEnd; ++it)
    {
        TRACE_LOG("To be filtered: [" << (*it)->getFilteredText() << "]");

        Blaze::Util::FilteredUserText* filteredText = BLAZE_NEW Blaze::Util::FilteredUserText();  
        filteredText->setFilteredText((*it)->getFilteredText());
        if (gProfanityFilter->inplaceFilter(filteredText->getFilteredTextAsTdfString()) != 0)
        {
            TRACE_LOG(" -> [" << filteredText->getFilteredText() << "]");
            filteredText->setResult(Blaze::Util::FILTER_RESULT_OFFENSIVE);
        }
        else
        {
            if ((*it)->getResult() != Blaze::Util::FILTER_RESULT_UNPROCESSED)
            {
                //use the result provided by the prefilter
                filteredText->setResult((*it)->getResult());
            }
            else 
            {    
                filteredText->setResult(Blaze::Util::FILTER_RESULT_PASSED);
            }

        }
        response.getFilteredTextList().push_back(filteredText);
    }
    return FilterForProfanityError::ERR_OK;
}

FetchQosConfigError::Error UtilSlaveImpl::processFetchQosConfig(Blaze::QosConfigInfo &response, const Message* message)
{
    gUserSessionManager->getQosConfig().getQosSettings().copyInto(response);
    return FetchQosConfigError::ERR_OK;
}

PreAuthError::Error UtilSlaveImpl::processPreAuth(const PreAuthRequest &request, PreAuthResponse &response, const Message* message)
{
    const char8_t* serviceName = gController->verifyServiceName(request.getClientData().getServiceName());
    
    if (serviceName == nullptr || serviceName[0] == '\0')
    {
        // When hosting a shared cluster, our requirement on the service name is much stricter as sane defaults can't be established.
        // We expect the client to provide the service name that is actually hosted by this instance. 
        
        // To preserve backward compatibility behavior with the single platform deployment, gController->verifyServiceName fall backs to the default service name if the specified
        // service name is not hosted (or the service name is not even specified).

        EA_ASSERT(gController->isSharedCluster()); // We should only ever get here if hosting a shared cluster.

        const char8_t* originalServiceName = request.getClientData().getServiceName();
        if (originalServiceName != nullptr && originalServiceName[0] != '\0')
        {
            ERR_LOG("[UtilSlaveImpl].processPreAuth: service name (" << originalServiceName << ")is not hosted by this instance. Failing preauth");
            return PreAuthError::UTIL_SERVICENAME_NOT_HOSTED;
        }
        
        ERR_LOG("[UtilSlaveImpl].processPreAuth: service name is a required parameter when hosting multiple platforms. Failing preauth");
        return PreAuthError::UTIL_SERVICENAME_NOT_SPECIFIED;
    }

    bool useServicePlatform = false;
    ClientPlatformType servicePlatform = gController->getServicePlatform(serviceName);
    if (message != nullptr && message->getPeerInfo().isClient())
    {
        message->getPeerInfo().setClientInfo(&request.getClientInfo());

        ClientPlatformType clientPlatform = message->getPeerInfo().getClientInfo()->getPlatform();
        if (servicePlatform != clientPlatform)
        {
            if (request.getClientData().getClientType() == CLIENT_TYPE_GAMEPLAY_USER || request.getClientData().getClientType() == CLIENT_TYPE_LIMITED_GAMEPLAY_USER)
            {
                // For Game play clients, only allow mismatch if we are in the mock mode.
                // This is code that allows mock pc users to use a different platform.
                // It has companion code in expresslogin_command.cpp that sets the external id to a mock value. 
                if (gController->getUseMockConsoleServices())
                    useServicePlatform = true;
            }
            else 
            {
                // non-gameplay clients are allowed to connect to any service. This could be due to a variety of reasons (tool clients connecting to a platform specific service
                // for some platform specific operations or dedicated servers that still have dependency on being platform specific). While it'd be cleaner to have these clients
                // connect to platform agnostic services, following allows for backward compatbile behavior.
                // Note that authentication componenent will still be in charge of validating the credentials correctly and ensure they are sufficient to log on to a particular service. 
                useServicePlatform = true;
            }
           
            if (useServicePlatform)
                message->getPeerInfo().getClientInfo()->setPlatform(servicePlatform);
            else
            {
                ERR_LOG("[UtilSlaveImpl].processPreAuth: client's platform (" << ClientPlatformTypeToString(clientPlatform) << ")"
                    << " does not match service's platform (" << ClientPlatformTypeToString(servicePlatform) << ") and not allowed to be overriden by the service.");
                return PreAuthError::UTIL_CALLER_PLATFORM_MISMATCH;
            }
        }
    }

    TRACE_LOG("[UtilSlaveImpl].processPreAuth: Peer using (" << ClientPlatformTypeToString(message->getPeerInfo().getClientInfo()->getPlatform()) << ") platform.");

    // Use the productName for the serviceName since the user has not authenticated yet.
    // (Might differ from upcoming UserSession if LoginRequest specifies an alternate productName.)
    const char8_t* productName = gController->getServiceProductName(serviceName);
    const auto* serverVersionString = gProcessController->getVersion().getVersion();
    const Blaze::ClientInfo &clientInfo = request.getClientInfo();

    if (getConfig().getPreAuthFailsBeyondPsuLimit())
    {
        const ClientType clientType = request.getClientData().getClientType();
        Authentication::AuthenticationSlaveImpl* authComp =
            static_cast<Authentication::AuthenticationSlaveImpl*>(gController->getComponent(Authentication::AuthenticationSlave::COMPONENT_ID, true /*reqLocal*/, true /*reqActive*/));
        if ((authComp != nullptr) && !authComp->isBelowPsuLimitForClientType(clientType))
        {
            mMaxPsuExceededCount.increment(1, clientType);
            TRACE_LOG("processPreAuth: The PSU limit for client type " << ClientTypeToString(clientType) << " has been reached."
                "  Fast failing at preAuth to prevent client from hitting Nucleus");

            if (message != nullptr)
            {
                ClientPlatformType clientPlatform = clientInfo.getPlatform();
                const char8_t* clientVersion = clientInfo.getClientVersion();
                const char8_t* sdkVersion = clientInfo.getBlazeSDKVersion();

                AccountId accountId = INVALID_ACCOUNT_ID;
                PersonaId personaId = INVALID_BLAZE_ID;
                ExternalId externalId = INVALID_EXTERNAL_ID;
                eastl::string externalString;
                eastl::string personaName;

                char8_t addrStr[Login::MAX_IPADDRESS_LEN] = { 0 };
                message->getPeerInfo().getRealPeerAddress().toString(addrStr, sizeof(addrStr));

                PlatformInfo platformInfo;
                convertToPlatformInfo(platformInfo, externalId, externalString.c_str(), accountId, clientPlatform);

                gUserSessionMaster->sendLoginPINError(PreAuthError::UTIL_PSU_LIMIT_EXCEEDED, UserSession::INVALID_SESSION_ID, platformInfo, personaId,
                    message->getPeerInfo().getLocale(), message->getPeerInfo().getCountry(), sdkVersion, clientType, addrStr, serviceName, productName, clientVersion);
            }
            return PreAuthError::UTIL_PSU_LIMIT_EXCEEDED;
        }
    }

    if (!clientInfo.getBlazeSDKVersionAsTdfString().empty())
    {
        if (isOlderBlazeServerVersion(clientInfo.getBlazeSDKVersion(), serverVersionString))
        {
            ERR_LOG("processPreAuth: ClientName = (" << clientInfo.getClientName() << ");ClientSkuId = (" << clientInfo.getClientSkuId()
                << ");ClientVersion = (" << clientInfo.getClientVersion() << ");  BlazeSDKVersion(" << clientInfo.getBlazeSDKVersion()
                << ") cannot be newer than the ServerVersion(" << serverVersionString << "): older server version\n");
        }

        mClientConnectCount.increment(1, clientInfo.getBlazeSDKVersion(), productName);
    }

    // Set client data
    processSetClientData(request.getClientData(), message);

    // Fetch Qos config
    processFetchQosConfig(response.getQosSettings(), message);

    // Get server version
    response.setServerVersion(serverVersionString);

    // Get server name
    response.setServiceName(serviceName);

    // Get server platform
    response.setPlatform(ClientPlatformTypeToString(servicePlatform));

    response.setPersonaNamespace(gController->getNamespaceFromPlatform(servicePlatform));

    // Gather status for each of the configured components
    
    const ComponentManager::ComponentStubMap& localComponents = gController->getComponentManager().getLocalComponents();

    response.getComponentIds().reserve(localComponents.size());
    for(ComponentManager::ComponentStubMap::const_iterator i = localComponents.begin(), e = localComponents.end(); i != e; ++i)
    {
        response.getComponentIds().push_back(i->first);
    }

    // Fetch client config
    FetchClientConfigError::Error err = processFetchClientConfig(request.getFetchClientConfig(), response.getConfig(), message);

    if (err != FetchClientConfigError::ERR_OK) 
    {
        return static_cast<PreAuthError::Error>(err);
    }

    Blaze::Authentication::AuthenticationSlaveImpl* authSlave = static_cast<Blaze::Authentication::AuthenticationSlaveImpl*>(gController->getComponentManager().getComponent(Authentication::AuthenticationSlave::COMPONENT_ID,true));
    if (authSlave)
    {
        response.setRegistrationSource(authSlave->getRegistrationSource(productName));
        response.setAuthenticationSource(authSlave->getAuthenticationSource(productName));
        response.setLegalDocGameIdentifier(authSlave->getLegalDocGameIdentifier());
        response.setUnderageSupported(authSlave->getAllowUnderage(productName));
        response.setEntitlementSource(authSlave->getEntitlementSource(productName));
    }

    response.setClientId(gController->getBlazeServerNucleusClientId());
    response.setReleaseType(gUserSessionManager->getProductReleaseType(productName));
    response.setMachineId(generateMachineId(request, message));

    return PreAuthError::ERR_OK;
}

uint32_t UtilSlaveImpl::generateMachineId(const PreAuthRequest &request, const Message* message)
{
    //put internal and external address into a 64bit field
    uint64_t hashData = static_cast<uint64_t>(message->getPeerInfo().getPeerAddress().getIp());
    hashData = (hashData << (sizeof(request.getLocalAddress()) * 8 )) + request.getLocalAddress();
    
    //make the data fit in 32 bits
    uint32_t hashOutput = static_cast<uint32_t>(hashData % UINT32_MAX);

    return hashOutput;
}

SetClientMetricsError::Error UtilSlaveImpl::processSetClientMetrics(const Blaze::ClientMetrics &request, const Message* message)
{
    request.copyInto(gCurrentLocalUserSession->getClientMetrics());
    return SetClientMetricsError::ERR_OK;
}

SetClientUserMetricsError::Error UtilSlaveImpl::processSetClientUserMetrics(const Blaze::ClientUserMetrics &request, const Message* message)
{
    //we received a metrics update for stt/tts, from a client 
    Blaze::ClientUserMetrics previousMetrics = gCurrentLocalUserSession->getClientUserMetrics();
    ClientPlatformType platform = gCurrentLocalUserSession->getPlatformInfo().getClientPlatform();

    //we need the delta in the metric values to see how much usage happened in this update
    //the sum of all the delta's of each user will be fed to OI regularly, so we can see how much the system is being used realtime
    
    //stt metrics have changed
    if (previousMetrics.getSttEventCount() < request.getSttEventCount())
    {
        //apply the deltas to the server wide OI metrics
        mAccessibilityMetrics.SttEventCount.increment(request.getSttEventCount() - previousMetrics.getSttEventCount(), platform);
        mAccessibilityMetrics.SttEmptyResultCount.increment(request.getSttEmptyResultCount() - previousMetrics.getSttEmptyResultCount(), platform);
        mAccessibilityMetrics.SttErrorCount.increment(request.getSttErrorCount() - previousMetrics.getSttErrorCount(), platform);
        mAccessibilityMetrics.SttDelay.increment(request.getSttDelay() - previousMetrics.getSttDelay(), platform);
        mAccessibilityMetrics.SttDurationMsSent.increment(request.getSttDurationMsSent() - previousMetrics.getSttDurationMsSent(), platform);
        mAccessibilityMetrics.SttCharCountRecv.increment(request.getSttCharCountRecv() - previousMetrics.getSttCharCountRecv(), platform);
   }

    //tts metrics have changed
    if (previousMetrics.getTtsEventCount() < request.getTtsEventCount())
    {
        //apply the deltas to the server wide OI metrics
        mAccessibilityMetrics.TtsEventCount.increment(request.getTtsEventCount() - previousMetrics.getTtsEventCount(), platform);
        mAccessibilityMetrics.TtsEmptyResultCount.increment(request.getTtsEmptyResultCount() - previousMetrics.getTtsEmptyResultCount(), platform);
        mAccessibilityMetrics.TtsErrorCount.increment(request.getTtsErrorCount() - previousMetrics.getTtsErrorCount(), platform);
        mAccessibilityMetrics.TtsDelay.increment(request.getTtsDelay() - previousMetrics.getTtsDelay(), platform);
        mAccessibilityMetrics.TtsCharCountSent.increment(request.getTtsCharCountSent() - previousMetrics.getTtsCharCountSent(), platform);
        mAccessibilityMetrics.TtsDurationMsRecv.increment(request.getTtsDurationMsRecv() - previousMetrics.getTtsDurationMsRecv(), platform);
    }

    //the user session only cares about the latest values (reported to PIN and binary logs on logout)
    request.copyInto(gCurrentLocalUserSession->getClientUserMetrics());
    return SetClientUserMetricsError::ERR_OK;
}

bool UtilSlaveImpl::readUserSettingsForUserIdFromDb(BlazeId blazeId, Blaze::Util::UserSettingsLoadAllResponse *response)
{
    ClientPlatformType platform = INVALID;
    if (gCurrentUserSession != nullptr && gCurrentUserSession->getBlazeId() == blazeId)
    {
        platform = gCurrentUserSession->getPlatformInfo().getClientPlatform();
    }
    else
    {
        UserInfoPtr userInfo = nullptr;
        BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo);
        if (lookupError != Blaze::ERR_OK)
        {
            ERR_LOG("[UserSettingsLoadAllCommand].start: Failed to look up user with BlazeId " << blazeId);
            return false;
        }
        platform = userInfo->getPlatformInfo().getClientPlatform();
    }

    DbResultPtr dbresult;

    // Get a database connection.
    uint32_t dbid = getDbId(platform);
    if (dbid==DbScheduler::INVALID_DB_ID)
    {
        ERR_LOG("[UserSettingsLoadAllCommand].start: Failed to obtain a valid dbId for platform '" << ClientPlatformTypeToString(platform) << "'");
        return false;
    }
    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(dbid);
    if (dbConn == nullptr)
    {
        ERR_LOG("[UserSettingsLoadAllCommand].start: Failed to obtain a connection. (quit)");
        return false;
    } // if

    // Build the query.
    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
    query->append("SELECT `key`, `data`"
        " FROM `util_user_small_storage`"
        " WHERE `id` = $D", blazeId);

    // Execute query.
    BlazeRpcError error = dbConn->executeQuery(query, dbresult);
    if (error != DB_ERR_OK)
    {
        ERR_LOG("[UserSettingsLoadAllCommand].start:"
            " Failed to read records for user [id] " << blazeId << ".");
        return false;
    }
    else
    {
        UserSettingsDataMap &map = response->getDataMap();
        map.reserve(dbresult->size());
        // iterate resultset and build response
        for (DbResult::const_iterator i = dbresult->begin(), e = dbresult->end(); i != e; ++i)
        {
            DbRow *dbRow = *i;
            const char8_t* key = dbRow->getString((uint32_t)0);
            const char8_t* data = dbRow->getString((uint32_t)1);
            map[key] = data;
        }
    }
    return true;
}

SetConnectionStateError::Error UtilSlaveImpl::processSetConnectionState(const Blaze::Util::SetConnectionStateRequest &request, const Message* message)
{
    return SetConnectionStateError::ERR_OK;
}

BlazeRpcError UtilSlaveImpl::getUserOptionsFromDb(Blaze::Util::UserOptions &response)
{
    BlazeId currentBlazeId = INVALID_BLAZE_ID;
    ClientType clientType = CLIENT_TYPE_INVALID;
    if (gCurrentUserSession != nullptr)
    {
        currentBlazeId = gCurrentUserSession->getBlazeId();
        clientType = gCurrentUserSession->getClientType();
    }

    bool isOtherUser = (response.getBlazeId() != 0 && response.getBlazeId() != currentBlazeId);

    // check permission
    if (isOtherUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TELEMETRY_VIEW))
    {
        WARN_LOG("[UtilSlaveImpl].getUserOptionsFromDb: User [" << currentBlazeId << "] attempted to get user options for [" << response.getBlazeId() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    DbResultPtr dbresult;
    BlazeId blazeId = 0;
    
    if (0 == response.getBlazeId())
    {
        blazeId = currentBlazeId;
        response.setBlazeId(currentBlazeId);
    }
    else
    {
        blazeId = response.getBlazeId();
    }

    if (blazeId > 0)
    {
        // Get a database connection.
        uint32_t dbId = getDbId();

        if (dbId == DbScheduler::INVALID_DB_ID)
        {
            return ERR_SYSTEM;
        }

        // To prevent deadlock, make sure the read-only DbConnPtr goes out of scope
        // before we request a read-write DbConnPtr later
        {
            DbConnPtr dbConn = gDbScheduler->getReadConnPtr(dbId);

            if (dbConn == nullptr)
            {
                ERR_LOG("[UtilSlaveImpl].getUserOptionsFromDb: Failed to obtain a read-only connection. (quit)");
                return ERR_SYSTEM;
            }

            // Build the query.
            QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
            query->append("SELECT `telemetryopt`"
                " FROM `util_user_options`"
                " WHERE `blazeid` = $D", blazeId);

            // Execute query.
            BlazeRpcError error = dbConn->executeQuery(query, dbresult);
            if (error != DB_ERR_OK)
            {
                ERR_LOG("[UtilSlaveImpl].getUserOptionsFromDb: Failed to read telemetryopt for user [id] " << currentBlazeId << ".");
                return ERR_SYSTEM;
            }
        }
        
        if (dbresult->empty())
        {
            DbConnPtr dbConn = gDbScheduler->getConnPtr(dbId);

            if (dbConn == nullptr)
            {
                ERR_LOG("[UtilSlaveImpl].getUserOptionsFromDb: Failed to obtain a read-write connection. (quit)");
                return ERR_SYSTEM;
            }

            QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
            query->append("INSERT INTO `util_user_options` (blazeid,telemetryopt) VALUES ($D, $u)", blazeId, TELEMETRY_OPT_IN);
            BlazeRpcError error = dbConn->executeQuery(query, dbresult);

            if (error != DB_ERR_OK)
            {
                ERR_LOG("[UtilSlaveImpl].getUserOptionsFromDb: Failed to write user options for id(" << blazeId << "). (quit)");
                return ERR_SYSTEM;
            }

            response.setTelemetryOpt(TELEMETRY_OPT_IN);
        }
        else
        {
            DbResult::const_iterator it = dbresult->begin();
            DbRow *dbRow = *it;
            TelemetryOpt telOpt;
            uint32_t telValue = dbRow->getUInt((uint32_t)0);
            if (telValue == 1)
                telOpt = TELEMETRY_OPT_IN;
            else if (telValue == 0)
                telOpt = TELEMETRY_OPT_OUT;
            else
            {
                return ERR_SYSTEM;
            }

            response.setTelemetryOpt(telOpt);
        }

        if (isOtherUser)
        {
            INFO_LOG("[UtilSlaveImpl].getUserOptionsFromDb: User [" << currentBlazeId << "] got user options for [" << response.getBlazeId() << "], had permission.");
        }
    }
    else if (clientType == CLIENT_TYPE_DEDICATED_SERVER)
        response.setTelemetryOpt(TELEMETRY_OPT_IN);

    return ERR_OK;
}

BlazeRpcError UtilSlaveImpl::setUserOptionsFromDb(const Blaze::Util::UserOptions &request)
{
    BlazeId currentBlazeId = INVALID_BLAZE_ID;
    if (gCurrentUserSession != nullptr)
    {
        currentBlazeId = gCurrentUserSession->getBlazeId();
    }

    bool isOtherUser = (request.getBlazeId()!=0 && request.getBlazeId()!=currentBlazeId);

    // check permission
    if (isOtherUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TELEMETRY_TOGGLE))
    {
        WARN_LOG("[UtilSlaveImpl].setUserOptionsFromDb: User [" << currentBlazeId << "] attempted to set user options for [" << request.getBlazeId() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    DbResultPtr dbresult;
    BlazeId blazeId = 0;

    if ( request.getBlazeId() == 0 )
    {
        blazeId = currentBlazeId;
    }
    else
    {
        blazeId = request.getBlazeId();
    }

    // Get a database connection.
    uint32_t dbid = getDbId();
    if (dbid==DbScheduler::INVALID_DB_ID)
    {
        return ERR_SYSTEM;
    }

    DbConnPtr dbConn = gDbScheduler->getConnPtr(dbid);
    if (dbConn == nullptr)
    {
        ERR_LOG("[UtilSlaveImpl].setUserOptionsFromDb: Failed to obtain a connection. (quit)");
        return ERR_SYSTEM;
    }

    // Build the query.
    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
    query->append("INSERT INTO `util_user_options` (blazeid,telemetryopt) VALUES ($D, $u) ON DUPLICATE KEY UPDATE telemetryopt=VALUES(telemetryopt)", blazeId, static_cast<uint32_t>(request.getTelemetryOpt()));

    // Execute query.
    BlazeRpcError error = dbConn->executeQuery(query, dbresult);
    if (error != DB_ERR_OK)
    {
        ERR_LOG("[UtilSlaveImpl].setUserOptionsFromDb:"
            " Failed to write user options for id(" << blazeId << "). (quit)");
        return ERR_SYSTEM;
    }

    if (isOtherUser)
    {
        INFO_LOG("[UtilSlaveImpl].setUserOptionsFromDb: User [" << currentBlazeId << "] set user options for [" << request.getBlazeId() << "], had permission.");
    }
    return ERR_OK;
}

BlazeRpcError UtilSlaveImpl::postAuthExc(PostAuthRequest &request, PostAuthResponse &response, const Command* callingCommand)
{
    processGetTickerServer(response.getTickerServer(), nullptr);

    GetUserOptionsRequest optInReq;
    if (!gCurrentUserSession->isGuestSession())
    {
        getUserOptions(optInReq, response.getUserOptions());
    }

    const char8_t* uniqueDeviceId = request.getUniqueDeviceId();

    if (gCurrentUserSession != nullptr)
    {
        // set unique device id for ios
        if (uniqueDeviceId[0] != '\0')
        {
            gUserSessionManager->insertUniqueDeviceIdForUserSession(gCurrentUserSession->getSessionId(), uniqueDeviceId);
        }

        gUserSessionManager->updateDsUserIndexForUserSession(gCurrentUserSession->getSessionId(), request.getDirtySockUserIndex());
    }

    if (callingCommand->getPeerInfo() != nullptr)
    {
        GetTelemetryServerRequest telemetryReq;
        telemetryReq.setUseKey2(request.getUseKey2());
        // fill the response with the telemetry server info
        processGetTelemetryServerImpl(telemetryReq, response.getTelemetryServer(), *callingCommand->getPeerInfo());
    }
    else
    {
        // connection is nullptr
        WARN_LOG("[UtilSlaveImpl].postAuthExc: nullptr connection");
    }

    return ERR_OK;
}

/*************************************************************************************************/
/*!
\brief getStatusInfo

Called by controller during getStatus RPC 

*/
/*************************************************************************************************/
void UtilSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    mClientConnectCount.iterate([&status](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            char8_t version[256];
            // format string
            blaze_snzprintf(version, sizeof(version), "TotalConnectionCountClient_%s_%s", tagList[0].second.c_str(), tagList[1].second.c_str());
            char8_t valueStr[64];
            blaze_snzprintf(valueStr, sizeof(valueStr), "%" PRIu64, value.getTotal());
            status.getInfo()[version] = valueStr;
        }
    );

    for (int32_t i = 0; i < CLIENT_TYPE_INVALID; ++i)
    {
        const char8_t* reason = ClientTypeToString((ClientType)i);
        uint64_t value = mMaxPsuExceededCount.getTotal({ { Metrics::Tag::client_type, reason } });

        char8_t keyStr[256];
        blaze_snzprintf(keyStr, sizeof(keyStr), "TotalMaxPsuExceeded_%s", reason);
        char8_t valueStr[64];
        blaze_snzprintf(valueStr, sizeof(valueStr), "%" PRIu64, value);

        status.getInfo()[keyStr] = valueStr;
    }
}

void UtilSlaveImpl::getValidVersionList(const char8_t* versionStr, uint32_t* version, uint32_t length)
{
    const char8_t* origin = versionStr;
    const char8_t* nextPos; 
    uint32_t i = 0;

    if ((origin == nullptr) ||
        (*origin == '\0'))
        return;

    while ((i < length) &&
           (*origin != '\0'))
    {
        nextPos = blaze_str2int(origin, &version[i]);
        if (nextPos != origin)
        {
            // we parsed out a version number.  nextPos now points to the first non-numeric char in the string.
            i++;
            origin = nextPos;
        }
        else
        {
            // origin points to a non-numeric char, continue to increment until we find a numeric char that we can parse out.
            origin++;
        }
    }

    return;
}

bool UtilSlaveImpl::isOlderBlazeServerVersion(const char8_t* blazeSdkVersion, const char8_t* serverVersion)
{
    uint32_t blazeSdk[4];
    uint32_t server[4];
    //"3.7.0.0 (mainline)"
    getValidVersionList(blazeSdkVersion, blazeSdk, 4);
    //"Blaze 3.06.00.00-development-HEAD (CL# unknown)" 
    getValidVersionList(serverVersion, server, 4);

    for (uint32_t i = 0; i < 4; ++i)
    {
        if (blazeSdk[i] > server[i])
            return true;
        else if (blazeSdk[i] == server[i])
        {
            continue;
        }
        else
            return false;
    }

    //the same as
    return false;
}

SuspendUserPingError::Error UtilSlaveImpl::processSuspendUserPing(const Blaze::Util::SuspendUserPingRequest &request, const Message* message)
{
    Connection *conn = nullptr;

    // The following cast to LegacyPeerInfo is safe as suspendUserPing rpc is not supported on grpc endpoint.
    if (gCurrentLocalUserSession != nullptr)
    {
        conn = &((static_cast<LegacyPeerInfo*>(gCurrentLocalUserSession->getPeerInfo()))->getDeprecatedLegacyConnection());
    }
    if (conn == nullptr && message != nullptr)
    {
        conn = &((static_cast<LegacyPeerInfo&>(message->getPeerInfo())).getDeprecatedLegacyConnection());
    }

    if (conn == nullptr)
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[UtilSlaveImpl].processSuspendUserPing: requires a connection.");
        return SuspendUserPingError::ERR_SYSTEM;
    }

    if (request.getSuspendTime() > conn->getEndpoint().getMaximumPingSuspensionPeriod())
    {
        char8_t timeBuf[64];
        BLAZE_TRACE_LOG(Log::SYSTEM, "[ConnectionUtil].suspendUserPing: You cannot suspended a User Ping for more than " << conn->getEndpoint().getMaximumPingSuspensionPeriod().toIntervalString(timeBuf, sizeof(timeBuf)) << ".");
        return SuspendUserPingError::UTIL_SUSPEND_PING_TIME_TOO_LARGE;
    }
    else if (request.getSuspendTime() < conn->getEndpoint().getInactivityTimeout())
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[ConnectionUtil].suspendUserPing: You cannot have a suspended User Ping Time value less than the inActivityTimeout of the Connection on the BlazeServer. This value is too small.");
        return SuspendUserPingError::UTIL_SUSPEND_PING_TIME_TOO_SMALL;
    }

    conn->setInactivitySuspensionTimeout(request.getSuspendTime());
    conn->setInactivitySuspensionTimeoutEndTime((TimeValue::getTimeOfDay() + request.getSuspendTime()));
    return SuspendUserPingError::ERR_OK;
}

SetClientStateError::Error UtilSlaveImpl::processSetClientState(const Blaze::ClientState &request, const Message* message)
{
    if (gUserSessionMaster == nullptr)
    {
        return SetClientStateError::ERR_SYSTEM;
    }

    return SetClientStateError::commandErrorFromBlazeError(gUserSessionMaster->updateClientState(gCurrentUserSession->getUserSessionId(), request));
}

} // Util
} // Blaze
