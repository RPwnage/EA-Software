/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_ARSONMODULE_H
#define BLAZE_STRESS_ARSONMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"

namespace Blaze
{
namespace Stress
{

class StressInstance;
class Login;
class ArsonInstance;

class ArsonModule : public StressModule
{
    NON_COPYABLE(ArsonModule);

public:
    typedef eastl::pair<const char8_t*, const char8_t*> UrlPair;

    typedef eastl::vector<eastl::string> QueryList;
    typedef eastl::vector<UrlPair> UrlList;
    typedef eastl::vector<TimeValue> SquelchDbQueryList;

    typedef enum
    {
        ACTION_POKESLAVE,
        ACTION_MAPGET,
        ACTION_LOOKUPIDENTITY,
        ACTION_QUERYDB,
        ACTION_HTTPREQUEST,
        ACTION_SQUELCHDB
    } Action;

    ~ArsonModule() override;

    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "arson";}

    Action getAction() const { return mAction; }
    const QueryList& getQueries() const { return mQueries; }
    uint32_t getDbQueryIterationCount() const { return mDbQueryIterationCount; }

    TimeValue getSquelchDbQueryIntervalTime() const { return  mSquelchDbQueryIntervalTime; }
    const SquelchDbQueryList& getSquelchDbQueryRanges() const { return mSquelchDbQueryRanges; }

    const UrlList& getUrls() const { return mUrls; }
    uint32_t getUrlIterationCount() const { return mUrlIterationCount; }

    static StressModule* create();

private:
    friend class ArsonInstance;

    typedef eastl::hash_map<int32_t,ArsonInstance*> InstancesById;

    Action mAction;  // Which command to issue
    InstancesById mActiveInstances;

    QueryList mQueries;
    uint32_t mDbQueryIterationCount;

    TimeValue mSquelchDbQueryIntervalTime;
    SquelchDbQueryList mSquelchDbQueryRanges;

    UrlList mUrls;
    uint32_t mUrlIterationCount;

    ArsonModule();

};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_ARSONMODULE_H

