/*************************************************************************************************/
/*!
    \file   getstats_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetStatsCommand

    Retrieves statistics allowing the client to explicitly specify the columns of interest.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "scope.h"
#include "stats/rpc/statsslave/getstats_stub.h"
#include "globalstatscache.h"
#include "statscacheprovider.h"
#include "globalstatsprovider.h"

namespace Blaze
{
namespace Stats
{

class GetStatsCommand : public GetStatsCommandStub
{
public:
    GetStatsCommand(Message* message, GetStatsRequest* request, StatsSlaveImpl* componentImpl);
    ~GetStatsCommand() override {}
    
private:
    // Not owned memory.
    StatsSlaveImpl* mComponent;

    // States
    GetStatsCommandStub::Errors execute() override;
};

DEFINE_GETSTATS_CREATE()

GetStatsCommand::GetStatsCommand(
    Message* message, GetStatsRequest* request, StatsSlaveImpl* componentImpl)
    : GetStatsCommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/    
GetStatsCommandStub::Errors GetStatsCommand::execute()
{
    const StatCategory* category;

    CategoryMap::const_iterator iter =
        mComponent->getConfigData()->getCategoryMap()->find(mRequest.getCategory());
    if (iter != mComponent->getConfigData()->getCategoryMap()->end())
    {
        category = iter->second;
        if (!category->isValidPeriod(mRequest.getPeriodType()))
        {
            TRACE_LOG("[GetStatsCommand].execute: bad period type <"
                << mRequest.getPeriodType() << "> for category <" << category->getName() << ">");
            return STATS_ERR_BAD_PERIOD_TYPE;
        }

        // validate the scope name and value
        if (category->validateScopeForCategory(*mComponent->getConfigData(), mRequest.getKeyScopeNameValueMap()) == false)
        {
            return STATS_ERR_BAD_SCOPE_INFO;
        }
    }
    else
    {
        TRACE_LOG("[GetStatsCommand].execute: unknown category <" << mRequest.getCategory() << ">");
        return STATS_ERR_UNKNOWN_CATEGORY;
    }

    GetStatsRequest::StringStatNameList::const_iterator snIter = mRequest.getStatNames().begin();
    GetStatsRequest::StringStatNameList::const_iterator snEnd = mRequest.getStatNames().end();

    const StatMap* statMap = category->getStatsMap();

    for(; snIter != snEnd; ++snIter)
    {
        // validate the requested stats
        StatMap::const_iterator statItr = statMap->find(snIter->c_str());
        if (statItr == statMap->end())
        {
            TRACE_LOG("[GetStatsCommand].execute: stat <" << snIter->c_str()
                << "> not found in category <" << category->getName() << ">");
            return ERR_SYSTEM;
        }
    }

    StatsProviderPtr statsProvider;

    if (category->getCategoryType() == CATEGORY_TYPE_GLOBAL)
    {
        statsProvider = StatsProviderPtr(BLAZE_NEW GlobalStatsProvider(*mComponent, *mComponent->getGlobalStatsCache()));
    }
    else if (mComponent->getStatsCache().isLruEnabled())
    {
        statsProvider = StatsProviderPtr(BLAZE_NEW StatsCacheProvider(*mComponent, mComponent->getStatsCache()));
    }
    else
    {
        statsProvider = StatsProviderPtr(BLAZE_NEW DbStatsProvider(*mComponent));
    }

    int32_t periodId = 0;
    if (mRequest.getPeriodType() != STAT_PERIOD_ALL_TIME)
    {
        periodId = (mRequest.getPeriodId() != 0 ? mRequest.getPeriodId() : mComponent->getPeriodId(mRequest.getPeriodType()) - mRequest.getPeriodOffset());
    }
    
    BlazeRpcError err = statsProvider->executeFetchRequest(
        *category,
        mRequest.getEntityIds().asVector(),
        mRequest.getPeriodType(),
        periodId,
        1,
        mRequest.getKeyScopeNameValueMap());

    if (err != Blaze::ERR_OK)
    {
        return ERR_SYSTEM;
    }

    statsProvider->reset();

    while (statsProvider->hasNextResultSet())
    {
        StatsProviderResultSet* resultSet = statsProvider->getNextResultSet();

        if (resultSet != nullptr && resultSet->size() > 0)
        {
            TRACE_LOG("[GetStatsCommand].execute: found <" << resultSet->size() << "> rows in this result");
            ScopeNameValueMap tempScopeMap;

            const char8_t* mapKey;
            char8_t tempKey[STATS_SCOPESTRING_LENGTH] = {0};
            resultSet->reset();
            while (resultSet->hasNextRow())
            {
                StatsProviderRowPtr row = resultSet->getNextRow();
                tempScopeMap.clear();
                EntityStatsPtr entityStats = BLAZE_NEW EntityStats();
                entityStats->setEntityId(row->getEntityId());
                entityStats->setEntityType(category->getCategoryEntityType());

                if (!mRequest.getKeyScopeNameValueMap().empty())
                {
                    tempKey[0] = '\0';
                    
                    ScopeNameValueMap::const_iterator scopeItr;
                    ScopeNameValueMap::const_iterator scopeEnd;

                    // get map key for the row
                    scopeItr = mRequest.getKeyScopeNameValueMap().begin();
                    scopeEnd = mRequest.getKeyScopeNameValueMap().end();
                    for (; scopeItr != scopeEnd; ++scopeItr)
                    {
                        /// @todo validate value ???
                        ScopeValue scopeValue = row->getValueInt64(scopeItr->first.c_str());
                        tempScopeMap.insert(eastl::make_pair(scopeItr->first, scopeValue));
                    }

                    // generate map key for this scope value combination for this row
                    genStatValueMapKeyForUnitMap(tempScopeMap, tempKey, sizeof(tempKey));
                    mapKey = tempKey;
                }
                else
                {
                    mapKey = SCOPE_NAME_NONE;
                }

                StatValues* statValues = nullptr;
                KeyScopeStatsValueMap::iterator itFind = mResponse.getKeyScopeStatsValueMap().find(mapKey);
                if (itFind == mResponse.getKeyScopeStatsValueMap().end())
                {
                    statValues = BLAZE_NEW StatValues();
                    mResponse.getKeyScopeStatsValueMap().insert(eastl::make_pair(mapKey, statValues));
                }
                else
                {
                    statValues = itFind->second;
                }
                statValues->getEntityStatsList().push_back(entityStats);

                char asStr[256];

                if (mRequest.getStatNames().size() == 0)
                {
                    // send back all the stats according to the config
                    StatList::const_iterator statItr = category->getStats().begin();
                    StatList::const_iterator statEnd = category->getStats().end();
                    entityStats->getStatValues().reserve(category->getStats().size());
                    for (; statItr != statEnd; ++statItr)
                    {
                        const Stat* stat = *statItr;
                        const char8_t* statName = stat->getName();

                        if (stat->getDbType().getType() == STAT_TYPE_INT)
                        {
                            if (stat->getTypeSize() == 8)
                                blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), row->getValueInt64(statName));
                             else
                                 blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), row->getValueInt(statName));
                        }
                        else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                            blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), (double_t) row->getValueFloat(statName));
                        else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                            blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), row->getValueString(statName));
                        entityStats->getStatValues().push_back(asStr);
                    }
                }
                else
                {
                    GetStatsRequest::StringStatNameList::const_iterator it = mRequest.getStatNames().begin();
                    GetStatsRequest::StringStatNameList::const_iterator end = mRequest.getStatNames().end();
                    entityStats->getStatValues().reserve(mRequest.getStatNames().size());
                    while (it != end)
                    {
                        const char8_t* statName = (it++)->c_str();
                        const StatMap* statMapTemp = category->getStatsMap();
                        StatMap::const_iterator statIter = statMapTemp->find(statName);
                        // stat names have been validated already 
                        const Stat* stat = statIter->second;

                        if (stat->getDbType().getType() == STAT_TYPE_INT)
                        {
                            if (stat->getTypeSize() == 8)
                                blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), row->getValueInt64(statName));
                             else
                                 blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), row->getValueInt(statName));
                        }
                        else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                            blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), (double_t) row->getValueFloat(statName));
                        else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                        {
                            const char *strVal = row->getValueString(statName);
                            blaze_snzprintf(asStr, sizeof(asStr), stat->getStatFormat(), strVal);
                        }
                        entityStats->getStatValues().push_back(asStr);
                    }
                }
            }
        }
    }

    return ERR_OK;
}

} // Stats
} // Blaze
