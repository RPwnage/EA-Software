/*************************************************************************************************/
/*!
    \file   getentitycount_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetEntityCountCommand

    Retrieves the number of entities in a given category.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "stats/rpc/statsslave/getentitycount_stub.h"

namespace Blaze
{
namespace Stats
{

class GetEntityCountCommand : public GetEntityCountCommandStub
{
public:
    GetEntityCountCommand(Message* message, GetEntityCountRequest* request, StatsSlaveImpl* componentImpl);
    ~GetEntityCountCommand() override {}

private:
    // States
    GetEntityCountCommandStub::Errors execute() override;
    GetEntityCountCommandStub::Errors doRowCount(uint32_t dbId, const char8_t* tableName, int32_t periodType, int32_t periodId);

private:
    // Not owned memory.
    StatsSlaveImpl* mComponent;
  
};

DEFINE_GETENTITYCOUNT_CREATE()

GetEntityCountCommand::GetEntityCountCommand(
    Message* message, GetEntityCountRequest* request, StatsSlaveImpl* componentImpl)
    : GetEntityCountCommandStub(message, request),
    mComponent(componentImpl)
{
}

/* Private methods *******************************************************************************/    
GetEntityCountCommandStub::Errors GetEntityCountCommand::execute()
{
    TRACE_LOG("[GetEntityCountCommand].start() - retrieving count for category: " << mRequest.getCategory() 
               << " period type: " << mRequest.getPeriodType() << " period offset: " << mRequest.getPeriodOffset());

    const StatCategory* cat;
    const StatsConfigData* config = mComponent->getConfigData();
    const char8_t *tableName = nullptr;
    CategoryMap::const_iterator iter =
        config->getCategoryMap()->find(mRequest.getCategory());
    if (iter == config->getCategoryMap()->end())
    {
        ERR_LOG("[GetEntityCountCommand].execute(): Unknown category " << mRequest.getCategory());
        return STATS_ERR_UNKNOWN_CATEGORY;
    }

    cat = iter->second;

    //
    // TODO: Move counting entities in category to stats provider
    //
    // Note: With global categories entity count should be well 
    // known to the designer upfront or if not it shouldn't 
    // be expensive to fetch the whole category table
    //
    if (cat->getCategoryType() == CATEGORY_TYPE_GLOBAL)
        return STATS_ERR_INVALID_OPERATION;

    if (cat->isValidPeriod(mRequest.getPeriodType()))
        tableName = iter->second->getTableName(mRequest.getPeriodType());
    else
    {
        ERR_LOG("[GetEntityCountCommand].start(): Bad period type " << mRequest.getPeriodType() 
                << " for category " << cat->getName());
        return STATS_ERR_BAD_PERIOD_TYPE;
    }

    // validate the scope name and value
    if (cat->validateScopeForCategory(*config, mRequest.getKeyScopeNameValueMap()) == false)
    {
        ERR_LOG("[GetEntityCountCommand].execute(): invalid keyscope name/value map");
        return STATS_ERR_BAD_SCOPE_INFO;
    }

    int32_t periodId = 0;
    int32_t periodType = mRequest.getPeriodType();

    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        periodId = mRequest.getPeriodId();
        if (periodId == 0)
        {
            periodId = mComponent->getPeriodId(periodType);

            if (mRequest.getPeriodOffset()<0 || periodId<mRequest.getPeriodOffset())
            {
                ERR_LOG("[GetEntityCountCommand].execute(): bad period offset value");
                return STATS_ERR_BAD_PERIOD_OFFSET;
            }

            periodId -= mRequest.getPeriodOffset();
        }
    }

    const DbIdList& list = config->getShardConfiguration().getDbIds(cat->getCategoryEntityType());
    for (DbIdList::const_iterator itr = list.begin(); itr != list.end(); ++itr)
    {
        Errors err = doRowCount(*itr, tableName, periodType, periodId);
        if (err != ERR_OK)
            return err;
    }

    return ERR_OK;
}

GetEntityCountCommandStub::Errors GetEntityCountCommand::doRowCount(uint32_t dbId, const char8_t* tableName, int32_t periodType, int32_t periodId)
{
    DbConnPtr conn = gDbScheduler->getReadConnPtr(dbId);
    if (conn == nullptr)
    {
        ERR_LOG("[GetEntityCountCommand].doRowCount() - failed to obtain connection.");
        return ERR_SYSTEM;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SELECT COUNT(`entity_id`) FROM `$s` WHERE TRUE", tableName);

    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        query->append(" AND `period_id` = $d", periodId);
    }

    // scope name
    ScopeNameValueMap::const_iterator scopeItr = mRequest.getKeyScopeNameValueMap().begin();
    ScopeNameValueMap::const_iterator scopeEnd = mRequest.getKeyScopeNameValueMap().end();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        if (scopeItr->second == KEY_SCOPE_VALUE_AGGREGATE)
        {
            // validation already done before db conn obtained
            query->append(" AND `$s` = $D", scopeItr->first.c_str(), mComponent->getConfigData()->getAggregateKeyValue(scopeItr->first.c_str()));
        }
        else if (scopeItr->second != KEY_SCOPE_VALUE_ALL)
        {
            // validation already done before db conn obtained
            query->append(" AND `$s` = $D", scopeItr->first.c_str(), scopeItr->second);
        }
    }

    query->append(";");
    
    DbResultPtr result;
    BlazeRpcError dberr = conn->executeQuery(query, result);

    TRACE_LOG("[GetEntityCountCommand].doRowCount() - done");

    if (dberr != DB_ERR_OK)
    {
        ERR_LOG("[GetEntityCountCommand.doRowCount() - database transaction was not successful: " << getDbErrorString(dberr));
        return ERR_SYSTEM;
    }

    if (!result->empty())
    {    
        TRACE_LOG("[GetEntityCountCommand].doRowCount() - found " << result->size() << " results in this result");
        DbResult::const_iterator itr = result->begin();
        DbResult::const_iterator end = result->end();
        for (; itr != end; ++itr)
        {
            mResponse.setCount(mResponse.getCount() + (*itr)->getInt((uint32_t) 0));
        }
    }

    return ERR_OK;
}

} // Stats
} // Blaze
