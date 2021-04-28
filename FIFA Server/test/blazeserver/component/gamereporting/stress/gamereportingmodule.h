/*************************************************************************************************/
/*!
    \file gamereportingmodule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_GAMEREPORTINGMODULE_H
#define BLAZE_STRESS_GAMEREPORTINGMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "framework/util/random.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"

#include "gamereportingutil.h"

#include "gamereporting/rpc/gamereportingslave.h"
#include "gamereporting/gamehistoryconfig.h"
#include "gamereporting/tdf/gamehistory.h"

#include "stressinstance.h"

using namespace Blaze::Stats;

namespace Blaze
{
namespace Stress
{
class StressInstance;
class Login;
class GameReportingInstance;

class GameReportingModule : public StressModule
{
    NON_COPYABLE(GameReportingModule);

public:
    typedef enum
    {
        ACTION_SUBMITOFFLINEGAMEREPORT,
        ACTION_GETGAMEREPORTS,
        ACTION_GETGAMEREPORTVIEW,
        ACTION_GETGAMEREPORTVIEWINFOLIST,
        ACTION_SIMULATEPROD
    } Action;

    struct Probabilities 
    {
        uint32_t submitOfflineGameReport;
        uint32_t getGameReports;
        uint32_t getGameReportView;
        uint32_t getGameReportViewInfoList;
    };   

    struct FilterId
    {
        FilterId(const char8_t* table, const char8_t* name, const char8_t* entityType)
            : mTable(table), mColName(name), mEntityType(entityType) {}
        FilterId(const FilterId& id) : mTable(id.mTable), mColName(id.mColName), mEntityType(id.mEntityType) {}
        const char8_t* mTable;
        const char8_t* mColName;
        const char8_t* mEntityType;
        bool isNull() { return (mTable == nullptr || mColName == nullptr); }
    };

    class LessFilterId : eastl::binary_function<FilterId, FilterId, bool>
    {
    public:
        bool operator() (FilterId id1, FilterId id2) const 
        {
            if (id1.isNull() || id2.isNull())
                return true;
            int32_t result = blaze_strcmp(id1.mTable, id2.mTable);
            if (result < 0) return true;
            if (result > 0) return false;

            result = blaze_strcmp(id1.mColName, id2.mColName);
            if (result < 0) return true;
            if (result > 0) return false;

            result = blaze_strcmp(id1.mEntityType, id2.mEntityType);
            if (result < 0) return true;
            if (result > 0) return false;

            return false;
        }
    };

    struct QueryDataValues
    {
        QueryDataValues(const char8_t* value = nullptr, int64_t startRange = INT64_MIN, int64_t endRange = INT64_MIN) :
            mValue(value), mStartRange(startRange), mEndRange(endRange) {}
        void addPossibleValue(const char8_t* possibleValue) { mPossibleValues.push_back(possibleValue); }
        const char8_t* mValue;
        int64_t mStartRange;
        int64_t mEndRange;
        eastl::list<const char8_t*> mPossibleValues;
    };


    typedef eastl::list<const GameReporting::GameReportQuery*> GameReportQueryList;
    typedef eastl::map<GameManager::GameReportName, GameReportQueryList> GameReportQueriesMap;
    typedef eastl::list<const GameReporting::GameReportViewConfig*> GameReportViewConfigList;
    typedef eastl::map<GameManager::GameReportName, GameReportViewConfigList> GameReportViewsMap;
    typedef eastl::map<FilterId, QueryDataValues, LessFilterId> FilterValuesMap;

    ~GameReportingModule() override;

    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override { return "gamereporting"; }

    Action getAction() const { return mAction; }

    static StressModule* create();

    Probabilities getProbabilities() const { return mProbablities; }
    uint32_t getMaxSleepTime() const { return mMaxSleepTime; }

    GameReportingUtil& getGameReportingUtil() {
        return mGameReportingUtil;
    }

protected:
    GameReportingModule();

    bool parseStressConfig(const ConfigMap& config);
    bool parseServerConfig(const ConfigMap& config);

    
    GameReportQueryList* getQueryListByTypeId(const GameManager::GameReportName& typeName)
    {
        GameReportQueriesMap::iterator iter, end;
        iter = mGameReportQueriesMap.find(typeName);
        if (iter != mGameReportQueriesMap.end())
        {
            return &(iter->second);
        }
        return nullptr;
    }

    GameReportViewConfigList* getViewListByTypeId(const GameManager::GameReportName& typeName)
    {
        GameReportViewsMap::iterator iter, end;
        iter = mGameReportViewsMap.find(typeName);
        if (iter != mGameReportViewsMap.end())
        {
            return &(iter->second);
        }
        return nullptr;
    }

    void addQueryIntoList(const GameReporting::GameReportQuery* query)
    {
        GameManager::GameReportName gameReportName(query->getTypeName());
        GameReportQueryList* queryList = getQueryListByTypeId(gameReportName);
        if (queryList != nullptr)
            queryList->push_back(query);
    }

    void addViewIntoList(const GameReporting::GameReportViewConfig* view)
    {
        GameManager::GameReportName gameReportName(view->getGameReportView()->getViewInfo().getTypeName());
        GameReportViewConfigList* viewList = getViewListByTypeId(gameReportName);
        if (viewList != nullptr)
            viewList->push_back(view);
    }

    const GameReporting::GameReportQuery* selectRandomGameReportQuery()
    {
        GameReportQueryList::const_iterator iter, end;
        iter = mQueryList.begin();
        end = mQueryList.end();
        int index = Blaze::Random::getRandomNumber(static_cast<int>(mQueryList.size()));

        for (int i = 0; i < index && iter != end; ++i, ++iter)
        {
        }
        return *iter;
    }

    const GameReporting::GameReportViewConfig* selectRandomGameReportView()
    {
        GameReportViewConfigList::const_iterator iter, end;
        iter = mViewList.begin();
        end = mViewList.end();
        int index = Blaze::Random::getRandomNumber(static_cast<int>(mViewList.size()));

        for (int i = 0; i < index && iter != end; ++i, ++iter)
        {
        }
        return *iter;
    }

    char8_t* getQueryValue(FilterId& id, BlazeId blazeId, const char8_t* gameReportName)
    {
        char8_t outputString[64];
        outputString[0] = '\0';

        FilterValuesMap::const_iterator iter = mFilterValuesMap.find(id);
        if (iter != mFilterValuesMap.end())
        {
            if (iter->second.mValue != nullptr)
            {
                if (blaze_strcmp(iter->second.mValue, "$BLAZEID$") == 0 || blaze_strcmp(iter->second.mValue, "$USERID$") == 0)
                    blaze_snzprintf(outputString, sizeof(outputString), "%" PRIi64, blazeId);
                else if (blaze_strcmp(iter->second.mValue, "$GAMEID$") == 0 && gameReportName != nullptr)
                    blaze_snzprintf(outputString, sizeof(outputString), "%" PRId64, mGameReportingUtil.pickRandomSubmittedGameId(gameReportName));
                else
                    blaze_strnzcpy(outputString, iter->second.mValue, sizeof(outputString));
            }
            else if (iter->second.mStartRange != INT64_MIN && iter->second.mEndRange != INT64_MIN)
            {
                if (iter->second.mEndRange > iter->second.mStartRange)
                {
                    uint32_t randomIndex = (uint32_t)Random::getRandomNumber((int)(iter->second.mEndRange - iter->second.mStartRange + 1));        
                    blaze_snzprintf(outputString, sizeof(outputString), "%" PRIi64, iter->second.mStartRange + randomIndex);
                }
            }
            else if (!iter->second.mPossibleValues.empty())
            {
                uint32_t randomIndex = (uint32_t)Random::getRandomNumber(iter->second.mPossibleValues.size());
                eastl::list<const char8_t*>::const_iterator valueIter = iter->second.mPossibleValues.begin();
                for (uint32_t i = 0; i < randomIndex && valueIter != iter->second.mPossibleValues.end(); ++i, ++valueIter) {}
                blaze_strnzcpy(outputString, *valueIter, sizeof(outputString));
            }
        }

        return blaze_strdup(outputString);
    }

private:
    friend class GameReportingInstance;

    typedef eastl::hash_map<int32_t,GameReportingInstance*> InstancesById;

    GameReportingUtil mGameReportingUtil;

    Action mAction;  // Which command to issue
    Probabilities mProbablities;
    uint32_t mMaxSleepTime;

    const GameReporting::GameTypeCollection *mGameTypeCollection;
    GameReporting::GameReportQueriesCollection *mGameReportQueriesCollection;
    GameReporting::GameReportViewsCollection *mGameReportViewsCollection;

    GameReportQueriesMap mGameReportQueriesMap;
    GameReportViewsMap mGameReportViewsMap;
    GameReportQueryList mQueryList;
    GameReportViewConfigList mViewList;
    FilterValuesMap mFilterValuesMap;

    InstancesById mActiveInstances;
};

class GameReportingInstance : public StressInstance
{
    NON_COPYABLE(GameReportingInstance);

public:
    GameReportingInstance(GameReportingModule* owner, StressConnection* connection, Login* login);

    ~GameReportingInstance() override
    {
        delete mProxy;
        delete mGameReportingInstance;
    }

    void onLogin(BlazeRpcError result) override;

private:
    const char8_t *getName() const override { return mName; }
    BlazeRpcError execute() override;

    BlazeRpcError submitOfflineGameReport();
    BlazeRpcError getGameReports();
    BlazeRpcError getGameReportView();
    BlazeRpcError getGameReportViewInfoList();
    BlazeRpcError simulateProd();
    void sleep();

    GameReportingModule* mOwner;
    GameReporting::GameReportingSlave* mProxy;
    const char8_t *mName;
    BlazeId mBlazeId;
    GameReportingUtilInstance *mGameReportingInstance;

    GameReporting::GameReportViewInfosList mGameReportViewInfosList;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_GameHistoryLegacyModule_H
