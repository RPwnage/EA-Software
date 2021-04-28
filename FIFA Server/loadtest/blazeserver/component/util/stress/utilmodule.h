/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_UTIL_H
#define BLAZE_STRESS_UTIL_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "util/rpc/utilslave.h"
#include "framework/rpc/usersessionsslave.h"


namespace Blaze
{
namespace Stress
{

class StressInstance;
class Login;
class UtilInstance;

typedef eastl::pair<const char8_t*, const char8_t*> UrlPair;
typedef eastl::vector<eastl::string> QueryList;
typedef eastl::vector<UrlPair> UrlList;

class UtilModule : public StressModule
{
    NON_COPYABLE(UtilModule);

public:
    typedef enum { FETCH_CONFIG, USERSETTINGS_SAVE, USERSETTINGS_LOAD, USERSETTINGS_LOADALL, LOOKUP_USERS_BY_PERSONA, RPC_COUNT } RpcId;
    
    struct PersonaLookupParams
    {
        const char8_t* mPersona;
        uint32_t mCount;
        uint32_t minIndex;
        uint32_t maxIndex;
    };
    typedef eastl::vector<PersonaLookupParams> PersonaLookupParamsList;
    
    ~UtilModule() override;

    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    bool parseConfig(const ConfigMap* config);

    static StressModule* create();
    const char8_t* getModuleName() override {return "util";}

    int32_t getScenarioCount() {return mScenarioCount;}
    const QueryList& getQueries() const { return mQueries; }
    uint32_t getDbQueryIterationCount() const { return mDbQueryIterationCount; }
    const UrlList& getUrls() const { return mUrls; }
    uint32_t getUrlIterationCount() const { return mUrlIterationCount; }
    const PersonaLookupParamsList& getPersonaLookupParams() const { return mPersonaLookupParams; }
    
private:
    friend class UtilInstance;
    typedef eastl::hash_map<int32_t,UtilInstance*> InstancesById;
    InstancesById mActiveInstances;
    PersonaLookupParamsList mPersonaLookupParams;

    int32_t mScenarioCount;
    int32_t mRpcCount;

    QueryList mQueries;
    uint32_t mDbQueryIterationCount;

    UrlList mUrls;
    uint32_t mUrlIterationCount;
    
    bool mCrossplayLookup;

    UtilModule();
};

// ==================================================
class UtilInstance : public StressInstance
{
    NON_COPYABLE(UtilInstance);

public:
    UtilInstance(UtilModule* owner, StressConnection* connection, Login* login)
        : StressInstance(owner, connection, login, Util::UtilSlave::LOGGING_CATEGORY),
        mOwner(owner),
        mProxy(BLAZE_NEW Util::UtilSlave(*getConnection())),
        mUsProxy(BLAZE_NEW UserSessionsSlave(*getConnection())),
        mRpcId(UtilModule::FETCH_CONFIG),
        mResult(ERR_OK),
        mCounter(0)
    {
    }

    ~UtilInstance() override
    {
        delete mProxy;
        delete mUsProxy;
    }

    BlazeRpcError scFetchClientConfig();
    BlazeRpcError scLoadUserSettings();
    BlazeRpcError scLoadUserSettingsAll();
    BlazeRpcError scSaveUserSettings();
    BlazeRpcError scLookupUsersByPersona();
    BlazeRpcError scIdle();

// *************************************

    BlazeRpcError fetchConfig();
    BlazeRpcError userSettingsLoad();
    BlazeRpcError userSettingsSave();
    BlazeRpcError userSettingsLoadAll();
    BlazeRpcError lookupUsersByPersona();

    const char8_t *getName() const override; // { return mName; }

private:
    UtilModule* mOwner;
    Util::UtilSlave* mProxy;
    UserSessionsSlave* mUsProxy;
    UtilModule::RpcId mRpcId;
    BlazeRpcError mResult;
    int mCounter;

    BlazeRpcError execute() override;
};

} // Stress
} // Blaze

#endif //BLAZE_STRESS_UTILMODULE_H
