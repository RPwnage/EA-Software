/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DummyModule

    This is a sample module to demostrate how to write a stress module.  Each stress instance
    created for this module will continuously issue an RPC to the dummy component on a blaze
    server.  It uses information in the configuration file to determine which RPC to call and
    how long to wait inbetween calls.  It also performs some primitive timing of the RPC calls.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "stressinstance.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/util/random.h"
#include "utilmodule.h"
#include "util/rpc/utilslave.h"
#include "framework/tdf/userdefines.h"
#include "util/tdf/utiltypes.h"
#include "time.h"

namespace Blaze
{
namespace Stress
{

    typedef BlazeRpcError (UtilInstance::*ActionFun)();

/*
 * List of available RPCs that can be called by the module.
 */

typedef struct {
    const char8_t* scName;
    ActionFun action;
    double hitRate;
    int32_t wMin;
    int32_t wMax;
} ScenarioDescriptor;

typedef struct {
    const char8_t* rpcName;
    uint32_t ctr; // total counter
    uint32_t errCtr; //error counter;
    uint32_t perCtr; // counter per measurement period
    uint32_t perErrCtr; // error counter per period
    int64_t durTotal;
    int64_t durPeriod;
    TimeValue periodStartTime;
} RpcDescriptor;

/* typedef enum { FETCH_CONFIG, USERSETTINGS_SAVE, USERSETTINGS_LOAD, USERSETTINGS_LOADALL, RPC_COUNT } RpcId; */

static ScenarioDescriptor scenarioList[] = {
    {"fetchClientConfig", &UtilInstance::scFetchClientConfig},
    {"loadUserSettings", &UtilInstance::scLoadUserSettings},
    {"loadUserSettingsAll", &UtilInstance::scLoadUserSettingsAll},
    {"saveUserSettings", &UtilInstance::scSaveUserSettings},
    {"lookupUsersByPersona", &UtilInstance::scLookupUsersByPersona},
    {"idle", &UtilInstance::scIdle}
    };

// Order as defined by the enum definitition
static RpcDescriptor rpcList[] = {
    {"fetchConfig"},
    {"userSettingsSave"},
    {"userSettingsLoad"},
    {"userSettingsLoadAll"},
    {"lookupUsersByPersona"}
    };

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

BlazeRpcError UtilInstance::execute()
{
    BlazeRpcError result = ERR_OK;

    int32_t rnd = Random::getRandomNumber(Random::MAX);

    for (int j = 0; j < mOwner->getScenarioCount(); ++j)
    {
        if (scenarioList[j].hitRate == 0) continue;
        if (rnd < scenarioList[j].wMin || rnd > scenarioList[j].wMax) continue;
        Fiber::setCurrentContext(scenarioList[j].scName);
        result = (this->*(scenarioList[j].action))();
        break;
    }

    return result;
}

/*************************************************************************************************/
// Scenarios
/*************************************************************************************************/
BlazeRpcError UtilInstance::scFetchClientConfig()
{
    BlazeRpcError err = fetchConfig();
    return err;
}

BlazeRpcError UtilInstance::scLoadUserSettings()
{
    BlazeRpcError err = userSettingsLoad();
    return err;
}

BlazeRpcError UtilInstance::scLoadUserSettingsAll()
{
    BlazeRpcError err = userSettingsLoadAll();
    return err;
}

BlazeRpcError UtilInstance::scSaveUserSettings()
{
    BlazeRpcError err = userSettingsSave();
    return err;
}

BlazeRpcError UtilInstance::scLookupUsersByPersona()
{
    BlazeRpcError err = lookupUsersByPersona();
    return err;
}

BlazeRpcError UtilInstance::scIdle()
{
    return ERR_OK;
}

/*************************************************************************************************/
/*************************************************************************************************/
// RPC methods
BlazeRpcError UtilInstance::fetchConfig()
{
    mRpcId = UtilModule::FETCH_CONFIG;
    mResult = ERR_OK;

    Util::FetchClientConfigRequest req;
    Util::FetchConfigResponse resp;
    req.setConfigSection("");

    mResult = mProxy->fetchClientConfig(req, resp);

    return mResult;
}

BlazeRpcError UtilInstance::userSettingsLoad()
{
    mRpcId = UtilModule::USERSETTINGS_LOAD;
    mResult = ERR_OK;

    Util::UserSettingsLoadRequest req;
    Util::UserSettingsResponse resp;
    req.setKey("test");

    mResult = mProxy->userSettingsLoad(req, resp);

    return mResult;
}

BlazeRpcError UtilInstance::userSettingsSave()
{
    mRpcId = UtilModule::USERSETTINGS_SAVE;
    mResult = ERR_OK;

    Util::UserSettingsSaveRequest req;
    req.setKey("test");
    req.setData("test");

    mResult = mProxy->userSettingsSave(req);

    return mResult;
}

BlazeRpcError UtilInstance::userSettingsLoadAll()
{
    mRpcId = UtilModule::USERSETTINGS_LOADALL;
    mResult = ERR_OK;

    Util::UserSettingsLoadAllResponse resp;

    mResult = mProxy->userSettingsLoadAll(resp);

    return mResult;
}

BlazeRpcError UtilInstance::lookupUsersByPersona()
{
    mRpcId = UtilModule::LOOKUP_USERS_BY_PERSONA;
    mResult = ERR_OK;
    Blaze::BlazeId minEntityId = 1000000000000;

    if (!mOwner->mCrossplayLookup || (uint32_t)Random::getRandomNumber(2) % 2 == 0) {
        eastl::string personaName;
        Blaze::LookupUsersRequest req;
        Blaze::UserDataResponse resp;
        req.setLookupType(LookupUsersRequest::BLAZE_ID);
        const UtilModule::PersonaLookupParamsList& params = mOwner->getPersonaLookupParams();
        for (UtilModule::PersonaLookupParamsList::const_iterator i = params.begin(), e = params.end(); i != e; ++i)
        {
            for (uint32_t c = 0; c < i->mCount; ++c)
            {
                UserIdentification* iden = req.getUserIdentificationList().pull_back();
                uint32_t range = i->maxIndex - i->minIndex;
                uint32_t rnd = (uint32_t)Random::getRandomNumber((int32_t)range);
                Blaze::BlazeId index = minEntityId + rnd;
                iden->setBlazeId(index);
            }
        }
        mResult = mUsProxy->lookupUsers(req, resp);
    }
    else {
        Blaze::LookupUsersCrossPlatformRequest req;
        Blaze::UserDataResponse resp;
        req.setLookupType(CrossPlatformLookupType::NUCLEUS_ACCOUNT_ID);
        const UtilModule::PersonaLookupParamsList& params = mOwner->getPersonaLookupParams();
        
        for (UtilModule::PersonaLookupParamsList::const_iterator i = params.begin(), e = params.end(); i != e; ++i)
        {
            for (uint32_t c = 0; c < i->mCount; ++c)
            {
                LookupUsersCrossPlatformRequest::LookupOpts opts = req.getLookupOpts();
                PlatformInfo* info = req.getPlatformInfoList().pull_back();
                EaIdentifiers& id = info->getEaIds();
                uint32_t range = i->maxIndex - i->minIndex;
                uint32_t rnd = (uint32_t)Random::getRandomNumber((int32_t)range);
                uint32_t index = i->minIndex + rnd;
                id.setNucleusAccountId(index + minEntityId);
                opts.setOnlineOnly(false);
                opts.setClientPlatformOnly(false);
                opts.setMostRecentPlatformOnly(false);
            }
        }
        mResult = mUsProxy->lookupUsersCrossPlatform(req, resp);
    }
    return mResult;
}

const char8_t* UtilInstance::getName() const
{
    return rpcList[mRpcId].rpcName;
}

/*** Public Methods ******************************************************************************/

// static
StressModule* UtilModule::create()
{
    return BLAZE_NEW UtilModule();
}

UtilModule::~UtilModule()
{
}

/*************************************************************************************************/
/*!
    \brief parseConfig
    Parse configs
    \param[in] - config map
    \return - true on success
*/
/*************************************************************************************************/
bool UtilModule::parseConfig(const ConfigMap* config)
{
    if (EAArrayCount(rpcList) != RPC_COUNT)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util,"[UtilModule].parseConfig: 'rpcList' missmatches enum definitions");
        return false;
    }

    const ConfigMap* scenarioMap = config->getMap("scenarioList");
    if (scenarioMap == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::util,"[UtilModule].parseConfig: 'scenarioList' section is not found");
        return false;
    }
    
    mCrossplayLookup = config->getBool("crossplayLookup", "false");

    TimeValue time = TimeValue::getTimeOfDay();
    mRpcCount = EAArrayCount(rpcList);
    for (int32_t j = 0; j < mRpcCount; ++j)
    {
        rpcList[j].periodStartTime = time;
        rpcList[j].ctr = 0;
        rpcList[j].errCtr = 0;
        rpcList[j].perCtr = 0;
        rpcList[j].perErrCtr = 0;
        rpcList[j].durPeriod = 0;
        rpcList[j].durTotal = 0;
    }

    double total = 0; 
    mScenarioCount = EAArrayCount(scenarioList);
    for (int32_t j = 0; j < mScenarioCount; ++j)
    {
        const ConfigMap* desc = scenarioMap->getMap(scenarioList[j].scName);
        if (desc == nullptr)
        {
            scenarioList[j].hitRate = 0;
            continue;
        }

        scenarioList[j].hitRate = desc->getDouble("hitRate", 0);
        total += scenarioList[j].hitRate;
    }

    double kx = Random::MAX/total;
    int32_t range = 0;

    for (int32_t j = 0; j < mScenarioCount; ++j)
    {
        if (scenarioList[j].hitRate == 0) continue;

        scenarioList[j].wMin = range; 
        range += static_cast<int32_t>(kx*scenarioList[j].hitRate);
        scenarioList[j].wMax = range; 

        BLAZE_TRACE_LOG(BlazeRpcLog::util,"[UtilModule].parseConfig: " << scenarioList[j].scName << ": min: " << scenarioList[j].wMin 
                        << "  max " << scenarioList[j].wMax << "  total: " << total);
    }

    const ConfigMap* personaLookupMap = config->getMap("personaLookup");
    if (personaLookupMap != nullptr)
    {
        PersonaLookupParams* params;
        params = &mPersonaLookupParams.push_back();
        params->mPersona = personaLookupMap->getString("online.persona", "s%05x");
        params->mCount = personaLookupMap->getUInt32("online.count", 1);
        params->minIndex = personaLookupMap->getUInt32("online.minIndex", 1);
        params->maxIndex = personaLookupMap->getUInt32("online.maxIndex", 1);
        
        params = &mPersonaLookupParams.push_back();
        params->mPersona = personaLookupMap->getString("offline.persona", "s%05x");
        params->mCount = personaLookupMap->getUInt32("offline.count", 1);
        params->minIndex = personaLookupMap->getUInt32("offline.minIndex", 1);
        params->maxIndex = personaLookupMap->getUInt32("offline.maxIndex", 1);
        
        params = &mPersonaLookupParams.push_back();
        params->mPersona = personaLookupMap->getString("invalid.persona", "fake%05x");
        params->mCount = personaLookupMap->getUInt32("invalid.count", 1);
        params->minIndex = personaLookupMap->getUInt32("invalid.minIndex", 1);
        params->maxIndex = personaLookupMap->getUInt32("invalid.maxIndex", 1);
    }


    return true;
}

bool UtilModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil)) return false;

    if (!parseConfig(&config)) return false;

    return true;
}

// -------------------------------------------------------------------------
StressInstance* UtilModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW UtilInstance(this, connection, login);
}

/*** Private Methods *****************************************************************************/

UtilModule::UtilModule()
    : mScenarioCount(0)
{
}

} // Stress
} // Blaze
