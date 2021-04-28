/*************************************************************************************************/
/*!
    \file gamereportingmodule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/string.h"
#include "gamereportingmodule.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/util/shared/rawbuffer.h"

using namespace Blaze::GameReporting;

namespace Blaze
{
namespace Stress
{

/*
 * List of available RPCs that can be called by the module.
 */
static const char8_t* ACTION_STR_SUBMITOFFLINEGAMEREPORT   = "submitOfflineGameReport";
static const char8_t* ACTION_STR_GETGAMEREPORTS            = "getGameReports";
static const char8_t* ACTION_STR_GETGAMEREPORTVIEW         = "getGameReportView";
static const char8_t* ACTION_STR_GETGAMEREPORTVIEWINFOLIST = "getGameReportViewInfoList";
static const char8_t* ACTION_STR_SIMULATEPROD              = "simulateProd";

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

// static
StressModule* GameReportingModule::create()
{
    return BLAZE_NEW GameReportingModule();
}

GameReportingModule::~GameReportingModule()
{
    delete mGameReportQueriesCollection;
    delete mGameReportViewsCollection;
}

bool GameReportingModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil))
        return false;

    if (!mGameReportingUtil.initialize(config, bootUtil))
        return false;

    if (!mGameReportingUtil.getSubmitReports())
        return false;

    if (!parseServerConfig(config)) 
        return false;

    if (!parseStressConfig(config))
        return false;

    return true;
}

bool GameReportingModule::parseStressConfig(const ConfigMap& config)
{
    const char8_t* action = config.getString("action", ACTION_STR_SUBMITOFFLINEGAMEREPORT);
    if (blaze_stricmp(action, ACTION_STR_SUBMITOFFLINEGAMEREPORT) == 0)
    {
        mAction = ACTION_SUBMITOFFLINEGAMEREPORT;
    }
    else if (blaze_stricmp(action, ACTION_STR_GETGAMEREPORTS) == 0)
    {
        mAction = ACTION_GETGAMEREPORTS;
    }
    else if (blaze_stricmp(action, ACTION_STR_GETGAMEREPORTVIEW) == 0)
    {
        mAction = ACTION_GETGAMEREPORTVIEW;
    }
    else if (blaze_stricmp(action, ACTION_STR_GETGAMEREPORTVIEWINFOLIST) == 0)
    {
        mAction = ACTION_GETGAMEREPORTVIEWINFOLIST;
    }
    else if (blaze_stricmp(action, ACTION_STR_SIMULATEPROD) == 0)
    {
        mAction = ACTION_SIMULATEPROD;
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingModule].parseStressConfig: Unrecognized action: '" << action << "'");
        return false;
    }

    BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, "[GameReportingModule].parseStressConfig: " << action << " action selected.");

    const ConfigSequence* queryNameSequence = config.getSequence("queries");
    if (queryNameSequence == nullptr)
    {
        if (blaze_stricmp(action, ACTION_STR_GETGAMEREPORTS) == 0)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingModule].parseStressConfig: Missing queries");
            return false;
        }
    }
    else
    {
        while (queryNameSequence->hasNext())
        {
            const char8_t* queryName = queryNameSequence->nextString(nullptr);
            if (queryName != nullptr)
            {
                const GameReportQueryConfig *queryConfig = mGameReportQueriesCollection->getGameReportQueryConfig(queryName);
                mQueryList.push_back(queryConfig->getGameReportQuery());
            }
        }
    }

    GameReportQueryConfigMap::const_iterator queryIter, queryEnd;
    queryIter = mGameReportQueriesCollection->getGameReportQueryConfigMap()->begin();
    queryEnd = mGameReportQueriesCollection->getGameReportQueryConfigMap()->end();

    for (; queryIter != queryEnd; ++queryIter)
    {
        addQueryIntoList(queryIter->second->getGameReportQuery());
    }

    const ConfigSequence* viewNameSequence = config.getSequence("views");
    if (viewNameSequence == nullptr)
    {
        if (blaze_stricmp(action, ACTION_STR_GETGAMEREPORTVIEW) == 0)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingModule].parseStressConfig: Missing views");
            return false;
        }
    }
    else
    {
        while (viewNameSequence->hasNext())
        {
            const char8_t* viewName = viewNameSequence->nextString(nullptr);
            if (viewName != nullptr)
            {
                const GameReportViewConfig* view = mGameReportViewsCollection->getGameReportViewConfig(viewName);
                mViewList.push_back(view);
            }
        }
    }

    GameReportViewConfigMap::const_iterator viewIter, viewEnd;
    viewIter = mGameReportViewsCollection->getGameReportViewConfigMap()->begin();
    viewEnd = mGameReportViewsCollection->getGameReportViewConfigMap()->end();

    for (; viewIter != viewEnd; ++viewIter)
    {
        addViewIntoList(viewIter->second);
    }

    const ConfigSequence* filterValuesSequence = config.getSequence("filterValues");
    if (filterValuesSequence != nullptr)
    {
        while (filterValuesSequence->hasNext())
        {
            const ConfigMap* map = filterValuesSequence->nextMap();

            const char8_t* table = map->getString("table", nullptr);
            const char8_t* name = map->getString("name", nullptr);
            const char8_t* entityType = map->getString("entityType", "0.0");
            const char8_t* value = map->getString("value", nullptr);
            const ConfigSequence* range = map->getSequence("range");
            const ConfigSequence* possibleValues = map->getSequence("possibleValues");

            if (table != nullptr && name != nullptr && (value != nullptr || range != nullptr || possibleValues != nullptr))
            {
                FilterId id(table, name, entityType);
                FilterValuesMap::insert_return_type inserter = mFilterValuesMap.insert(id);
                if (value != nullptr)
                    inserter.first->second.mValue = value;
                if (range != nullptr && range->getSize() == 2)
                {
                    inserter.first->second.mStartRange = range->nextInt64(INT64_MIN);
                    inserter.first->second.mEndRange = range->nextInt64(INT64_MIN);
                }
                else
                {
                    // warn log message to indicate that the range format is not valid
                }
                while (possibleValues != nullptr && possibleValues->hasNext())
                {
                    inserter.first->second.addPossibleValue(possibleValues->nextString(nullptr));
                }
            }
        }
    }

    const ConfigMap* simMap = config.getMap("sim");
    if (simMap != nullptr)
    {
        const ConfigMap* probabilityMap = simMap->getMap("probability");

        if (probabilityMap != nullptr)
        {
            mProbablities.submitOfflineGameReport = probabilityMap->getUInt32("submitOfflineGameReport", 0);
            mProbablities.getGameReports = probabilityMap->getUInt32("getGameReports", 0);
            mProbablities.getGameReportView = probabilityMap->getUInt32("getGameReportView", 0);
            mProbablities.getGameReportViewInfoList = probabilityMap->getUInt32("getGameReportViewInfoList", 0);
        }

        if (mProbablities.submitOfflineGameReport + mProbablities.getGameReports +
            mProbablities.getGameReportView + mProbablities.getGameReportViewInfoList > 100)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingModule].parseStressConfig: All probabilities combined are greater than 100");
            return false;
        }

        mMaxSleepTime = simMap->getUInt32("maxSleep", 5);
    }

    return true;
}

bool GameReportingModule::parseServerConfig(const ConfigMap& config)
{
    mGameTypeCollection = mGameReportingUtil.getGameTypeCollection();
    mGameReportQueriesCollection = BLAZE_NEW GameReportQueriesCollection();    
    if (!mGameReportQueriesCollection->init(*mGameReportingUtil.getComponentConfig(), mGameTypeCollection->getGameTypeMap()))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingModule].parseServerConfig: Game history queries configuration invalid.");
        delete mGameReportQueriesCollection;
        return false;
    }
    mGameReportViewsCollection = BLAZE_NEW GameReportViewsCollection();
    if (!mGameReportViewsCollection->init(*mGameReportingUtil.getComponentConfig(), mGameTypeCollection->getGameTypeMap()))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingModule].parseServerConfig: Game history views configuration invalid.");
        delete mGameReportQueriesCollection;
        delete mGameReportViewsCollection;
        return false;
    }
    
    return true;
}

StressInstance* GameReportingModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW GameReportingInstance(this, connection, login);
}

/*** Private Methods *****************************************************************************/

GameReportingModule::GameReportingModule()
    : mAction(ACTION_SUBMITOFFLINEGAMEREPORT),
      mGameTypeCollection(nullptr),
      mGameReportQueriesCollection(nullptr), 
      mGameReportViewsCollection(nullptr),
      mQueryList(BlazeStlAllocator("GameReportingModule::mQueryList", GameReportingSlave::COMPONENT_MEMORY_GROUP)),
      mViewList(BlazeStlAllocator("GameReportingModule::mViewList", GameReportingSlave::COMPONENT_MEMORY_GROUP))
{
}

GameReportingInstance::GameReportingInstance(GameReportingModule* owner, StressConnection* connection, Login* login)
    : StressInstance(owner, connection, login, BlazeRpcLog::gamereporting),
      mOwner(owner),
      mProxy(BLAZE_NEW GameReportingSlave(*getConnection()))
{
    mName = "";
    mGameReportingInstance = BLAZE_NEW GameReportingUtilInstance(&(owner->getGameReportingUtil()));
    mGameReportingInstance->setStressInstance(this);
}


void GameReportingInstance::onLogin(BlazeRpcError result)
{
    if (result == Blaze::ERR_OK)
    {
        mBlazeId = getLogin()->getUserLoginInfo()->getBlazeId();
    }
}

BlazeRpcError GameReportingInstance::execute()
{
    BlazeRpcError result = ERR_OK;

    if (mBlazeId > 0)
    {
        switch (mOwner->getAction())
        {
            case GameReportingModule::ACTION_SUBMITOFFLINEGAMEREPORT:
            {
                result = submitOfflineGameReport();
                mName = ACTION_STR_SUBMITOFFLINEGAMEREPORT;
                break;
            }
            case GameReportingModule::ACTION_GETGAMEREPORTS:
            {
                result = getGameReports();
                mName = ACTION_STR_GETGAMEREPORTS;
                break;
            }
            case GameReportingModule::ACTION_GETGAMEREPORTVIEW:
            {
                result = getGameReportView();
                mName = ACTION_STR_GETGAMEREPORTVIEW;
                break;
            }
            case GameReportingModule::ACTION_GETGAMEREPORTVIEWINFOLIST:
            {
                result = getGameReportViewInfoList();
                mName = ACTION_STR_GETGAMEREPORTVIEWINFOLIST;
                break;
            }
            case GameReportingModule::ACTION_SIMULATEPROD:
            {
                result = simulateProd();
                break;
            }
            default:
                 break;
        }

        if (result != ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingInstance].execute: Action finished with result " << (Blaze::ErrorHelp::getErrorName(result)));
        }
    }

    return result;
}

BlazeRpcError GameReportingInstance::submitOfflineGameReport()
{
    return mOwner->getGameReportingUtil().submitOfflineReport(getLogin()->getUserLoginInfo()->getBlazeId(), mProxy);
}

BlazeRpcError GameReportingInstance::getGameReports()
{
    BlazeRpcError result = ERR_OK;
    GetGameReports request;
    GameReportsList response;

    const GameReportQuery* query = mOwner->selectRandomGameReportQuery();

    if (query != nullptr)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingInstance].getGameReports: User " << mBlazeId << " is fetching '" 
                        << query->getName() << "' game report query");

        request.setQueryName(query->getName());

        // Fill in client params
        const GameReportFilterList& filterList = query->getFilterList();

        GameReportFilterList::const_iterator fIter, fEnd;
        fIter = filterList.begin();
        fEnd = filterList.end();
        
        for (; fIter != fEnd; ++fIter)
        {
            eastl::string entitTypeName = ((*fIter)->getEntityType().toString('.')).c_str();
            GameReportingModule::FilterId id((*fIter)->getTable(), (*fIter)->getAttributeName(), entitTypeName.c_str());
            const char8_t* queryVarValueStr = mOwner->getQueryValue(id, mBlazeId, query->getTypeName());
            request.getQueryVarValues().push_back(queryVarValueStr);
        }

        result = mProxy->getGameReports(request, response);
        BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingInstance].getGameReports: Result = \n" << response);
    }

    return result;
}

BlazeRpcError GameReportingInstance::getGameReportView()
{
    BlazeRpcError result = ERR_OK;
    GetGameReportViewRequest request;
    GetGameReportViewResponse response;

    const GameReportViewConfig* view = mOwner->selectRandomGameReportView();

    if (view != nullptr)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingInstance].getGameReportView: User " << mBlazeId << " is fetching '" 
                        << view->getGameReportView()->getViewInfo().getName() << "' game report view");

        request.setName(view->getGameReportView()->getViewInfo().getName());

        // Fill in client params
        const GameReportFilterList& filterList = view->getGameReportView()->getFilterList();

        GameReportFilterList::const_iterator fIter, fEnd;
        fIter = filterList.begin();
        fEnd = filterList.end();

        for (; fIter != fEnd; ++fIter)
        {
            eastl::string entitTypeName = ((*fIter)->getEntityType().toString('.')).c_str();
            GameReportingModule::FilterId id((*fIter)->getTable(), (*fIter)->getAttributeName(), entitTypeName.c_str());
            const char8_t* queryVarValueStr = mOwner->getQueryValue(id, mBlazeId, view->getGameReportView()->getViewInfo().getTypeName());
            request.getQueryVarValues().push_back(queryVarValueStr);
        }

        result = mProxy->getGameReportView(request, response);
        BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingInstance].getGameReportView: Result = \n" << response);
    }

    return result;
}

BlazeRpcError GameReportingInstance::getGameReportViewInfoList()
{
    BlazeRpcError result = ERR_OK;
    result = mProxy->getGameReportViewInfoList(mGameReportViewInfosList);
    return result;
}

BlazeRpcError GameReportingInstance::simulateProd()
{
    BlazeRpcError result = ERR_OK;

    uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(100)) + 1;

    uint32_t submitProb = mOwner->getProbabilities().submitOfflineGameReport;
    uint32_t queryProb = mOwner->getProbabilities().getGameReports + submitProb;
    uint32_t viewProb = mOwner->getProbabilities().getGameReportView + queryProb;
    uint32_t viewInfoListProb = mOwner->getProbabilities().getGameReportViewInfoList + viewProb;

    if ((rand > 0) && (rand <= submitProb))
    {
        result = submitOfflineGameReport();
        mName = ACTION_STR_SUBMITOFFLINEGAMEREPORT;
    }
    else if ((rand > submitProb) && (rand <= queryProb))
    {
        result = getGameReports();
        mName = ACTION_STR_GETGAMEREPORTS;
    }
    else if ((rand > queryProb) && (rand <= viewProb))
    {
        result = getGameReportView();
        mName = ACTION_STR_GETGAMEREPORTVIEW;
    }
    else if ((rand > viewProb) && (rand <= viewInfoListProb))
    {
        result = getGameReportViewInfoList();
        mName = ACTION_STR_GETGAMEREPORTVIEWINFOLIST;
    }
    else
    {
        sleep();
    }

    return result;
}

void GameReportingInstance::sleep()
{
    int32_t sleepTime = Blaze::Random::getRandomNumber(mOwner->getMaxSleepTime());
    BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingInstance].sleep: User " << mBlazeId << " sleeping for " << sleepTime << " ms!");
    if (gSelector->sleep(sleepTime*1000) != ERR_OK)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingInstance].sleep: Fail");
    }
}

} // Stress
} // Blaze

