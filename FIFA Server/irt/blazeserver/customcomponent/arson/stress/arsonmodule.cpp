/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ArsonModule

    This is a sample module to demostrate how to write a stress module.  Each stress instance
    created for this module will continuously issue an RPC to the arson component on a blaze
    server.  It uses information in the configuration file to determine which RPC to call and
    how long to wait inbetween calls.  It also performs some primitive timing of the RPC calls.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "arsonmodule.h"
#include "stressinstance.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/rpc/usersessions_defines.h"
#include "arson/tdf/arson.h"
#include "arson/rpc/arsonslave.h"

using namespace Blaze::Arson;

namespace Blaze
{
namespace Stress
{

/*
 * List of available RPCs that can be called by the module.
 */
static const char8_t* ACTION_STR_POKESLAVE = "pokeSlave";
static const char8_t* ACTION_STR_MAPGET = "mapGet";
static const char8_t* ACTION_STR_LOOKUPIDENTITY = "lookupIdentity";
static const char8_t* ACTION_STR_QUERYDB = "queryDb";
static const char8_t* ACTION_STR_SQUELCHDB = "squelchDb";
static const char8_t* ACTION_STR_HTTPREQUEST = "httpRequest";

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

class ArsonInstance : public StressInstance
{
    NON_COPYABLE(ArsonInstance);

public:
    ArsonInstance(ArsonModule* owner, StressConnection* connection, Login* login)
        : StressInstance(owner, connection, login, BlazeRpcLog::arson),
          mOwner(owner),
          mProxy(BLAZE_NEW ArsonSlave(*getConnection())),
          mSquelchDbNextTransitionTime(0),
          mSquelchDbRangeIndex(0)
    {
        mName = "";
    }

    ~ArsonInstance() override
    {
        delete mProxy;
    }


private:
    ArsonModule* mOwner;
    ArsonSlave* mProxy;
    const char8_t *mName;

    TimeValue mSquelchDbNextTransitionTime;
    uint32_t mSquelchDbRangeIndex;

    const char8_t *getName() const override { return mName; }

    BlazeRpcError execute() override
    {
        BlazeRpcError result = ERR_OK;
        switch (mOwner->getAction())
        {
            case ArsonModule::ACTION_POKESLAVE:
            {
                mName = "PokeSlave";
                ArsonRequest req;
                ArsonResponse response;
                ArsonError errorResponse;
                result = mProxy->pokeSlave(req, response, errorResponse);
                break;
            }

            case ArsonModule::ACTION_MAPGET:
            {
                mName = "MapGet";
                ExceptionMapValueRequest req;
                ExceptionMapEntry response;
                result = mProxy->mapGet(req, response);             
                break;
            }

            case ArsonModule::ACTION_LOOKUPIDENTITY:
            {
                mName = "LookupIdentity";
                Arson::IdentityRequest req;
                Arson::IdentityResponse  response;
                req.setBlazeObjectType(ENTITY_TYPE_USER);
                req.getIds().push_back(1);
                req.getIds().push_back(2);
                req.getIds().push_back(3);
                req.getIds().push_back(4);
                result = mProxy->lookupIdentity(req, response);
                break;
            }
            case ArsonModule::ACTION_QUERYDB:
            {
                mName = "QueryDb";
                QueryRequest req;
                req.setDbName("main");
                const char8_t* query
                    = mOwner->getQueries()[rand() % mOwner->getQueries().size()].c_str();
                req.setQuery(query);
                req.setTransaction(0);
                req.setReadOnly(rand() % 2);
                req.setCount(mOwner->getDbQueryIterationCount());
                result = mProxy->queryDb(req);
                break;
            }
            case ArsonModule::ACTION_SQUELCHDB:
            {
                mName = "SquelchDb";

                TimeValue now = TimeValue::getTimeOfDay();
                const ArsonModule::SquelchDbQueryList& ranges = mOwner->getSquelchDbQueryRanges();
                if (now > mSquelchDbNextTransitionTime)
                {
                    mSquelchDbRangeIndex = (mSquelchDbRangeIndex + 1) % ranges.size();
                    mSquelchDbNextTransitionTime = now + mOwner->getSquelchDbQueryIntervalTime();
                }

                QueryRequest req;
                req.setDbName("main");
                char8_t query[256];
                float sleepTime = ((float)ranges[mSquelchDbRangeIndex].getMillis()) / 1000;
                float deviation = ((float)(rand() % (int32_t)(((float)ranges[mSquelchDbRangeIndex].getMillis()) * 0.10))) / 1000;
                if ((rand() % 2) == 0)
                    deviation = -deviation;
                sleepTime += deviation;
                blaze_snzprintf(query, sizeof(query), "SELECT sleep(%f);", sleepTime);
                req.setQuery(query);
                req.setTransaction(0);
                req.setReadOnly(rand() % 2);
                req.setCount(1);
                result = mProxy->queryDb(req);
                break;
            }
            case ArsonModule::ACTION_HTTPREQUEST:
            {
                mName = "HttpRequest";
                HttpRequest req;
                const ArsonModule::UrlPair& url
                    = mOwner->getUrls()[rand() % mOwner->getUrls().size()];
                req.setAddress(url.first);
                req.setUri(url.second);
                req.setCount(mOwner->getUrlIterationCount());
                result = mProxy->httpRequest(req);
                break;
            }
        }

        return result;
    }
};



/*** Public Methods ******************************************************************************/

// static
StressModule* ArsonModule::create()
{
    return BLAZE_NEW ArsonModule();
}


ArsonModule::~ArsonModule()
{
}

bool ArsonModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil)) return false;

    const char8_t* action = config.getString("action", ACTION_STR_POKESLAVE);
    if (blaze_stricmp(action, ACTION_STR_POKESLAVE) == 0)
    {
        mAction = ACTION_POKESLAVE;
    }
    else if (blaze_stricmp(action, ACTION_STR_MAPGET) == 0)
    {
        mAction = ACTION_MAPGET;
    }
    else if (blaze_stricmp(action, ACTION_STR_LOOKUPIDENTITY) == 0)
    {
        mAction = ACTION_LOOKUPIDENTITY;
    }
    else if (blaze_stricmp(action, ACTION_STR_QUERYDB) == 0)
    {
        mAction = ACTION_QUERYDB;
        const ConfigSequence* queries = config.getSequence("dbQueries");
        if (queries != nullptr)
        {
            const char8_t* query;
            while ((query = queries->nextString(nullptr)) != nullptr)
                mQueries.push_back(query);
        }
        if (mQueries.empty())
        {
            BLAZE_ERR_LOG(BlazeRpcLog::arson, "[ArsonModule].initialize: Action set to " << ACTION_STR_QUERYDB << " but no queries "
                    "specified in dbQueries parameter.");
            return false;
        }
        mDbQueryIterationCount = config.getUInt32("dbQueryIterationCount", 1);
    }
    else if (blaze_stricmp(action, ACTION_STR_SQUELCHDB) == 0)
    {
        mAction = ACTION_SQUELCHDB;

        if (!config.isDefined("squelchDbQueryIntervalTime"))
        {
            BLAZE_ERR_LOG(BlazeRpcLog::arson, "[ArsonModule].initialize: Action set to " << ACTION_STR_SQUELCHDB << " but missing "
                    "squelchDbQueryIntervalTime parameter.");
            return false;
        }
        bool isDefault = false;
        mSquelchDbQueryIntervalTime = config.getTimeInterval("squelchDbQueryIntervalTime", TimeValue(0LL), isDefault);

        const ConfigSequence* rangeSeq = config.getSequence("squelchDbQueryRanges");
        if (rangeSeq == nullptr)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::arson, "[ArsonModule].initialize: Action set to " << ACTION_STR_SQUELCHDB << " but missing "
                    "squelchDbQueryRanges parameter.");
            return false;
        }
        while (rangeSeq->hasNext())
        {
            TimeValue t = rangeSeq->nextTimeInterval(TimeValue(0LL));
            mSquelchDbQueryRanges.push_back(t);
        }
    }
    else if (blaze_stricmp(action, ACTION_STR_HTTPREQUEST) == 0)
    {
        mAction = ACTION_HTTPREQUEST;
        const ConfigSequence* urls = config.getSequence("urls");
        if (urls != nullptr)
        {
            const ConfigMap* urlMap;
            while ((urlMap = urls->nextMap()) != nullptr)
            {
                mUrls.push_back(eastl::make_pair(
                            urlMap->getString("addr", nullptr), urlMap->getString("uri", nullptr)));
            }
        }
        if (mUrls.empty())
        {
            BLAZE_ERR_LOG(BlazeRpcLog::arson, "[ArsonModule].initialize: Action set to " << ACTION_STR_HTTPREQUEST << " but no URLs "
                    "specified in 'urls' parameter.");
            return false;
        }
        mUrlIterationCount = config.getUInt32("urlIterationCount", 1);
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::arson, "[ArsonModule].initialize: Unrecognized action: '" << action << "'");
        return false;
    }

    BLAZE_INFO_LOG(BlazeRpcLog::arson, "[ArsonModule].initialize: " << action << " action selected.");

    return true;
}

StressInstance* ArsonModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW ArsonInstance(this, connection, login);
}

/*** Private Methods *****************************************************************************/

ArsonModule::ArsonModule()
    : mAction(ACTION_POKESLAVE)
{
}

} // Stress
} // Blaze

