/*************************************************************************************************/
/*!
    \file   getleaderboardentitycount_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetLeaderboardEntityCountCommand

    Retrieves number of entries leaderboard has.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"


#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "stats/rpc/statsslave/getleaderboardentitycount_stub.h"
#include "leaderboardhelper.h"
#include "scope.h"

namespace Blaze
{
namespace Stats
{


class GetLeaderboardEntityCountCommand : public GetLeaderboardEntityCountCommandStub
{
public:
    GetLeaderboardEntityCountCommand(Message* message, LeaderboardEntityCountRequest* request, StatsSlaveImpl* componentImpl);
    ~GetLeaderboardEntityCountCommand() override {}
    
private:
    // States
    GetLeaderboardEntityCountCommandStub::Errors execute() override;
    Errors getDbEntityCount(uint32_t dbId, const StatLeaderboard* leaderboard, ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId);

    GetLeaderboardEntityCountCommandStub::Errors combineScopes(const StatLeaderboard& leaderboard, 
        ScopeNameValueMap& userScopeNameMap, ScopeNameValueListMap& combinedScopeNameValueListMap);

    StatsSlaveImpl* mComponent;
};

DEFINE_GETLEADERBOARDENTITYCOUNT_CREATE()

GetLeaderboardEntityCountCommand::GetLeaderboardEntityCountCommand(
    Message* message, LeaderboardEntityCountRequest* request, StatsSlaveImpl* componentImpl)
    : GetLeaderboardEntityCountCommandStub(message, request),
    mComponent(componentImpl)
{
}

/*************************************************************************************************/
/*!
    \brief execute
    Command entry point
    \param[in] - none
    \return - none
*/
/*************************************************************************************************/
GetLeaderboardEntityCountCommandStub::Errors GetLeaderboardEntityCountCommand::execute()
{
    TRACE_LOG("[GetLeaderboardEntityCountCommand].execute() - board ID: " << mRequest.getBoardId() << " boardName: <" 
               << mRequest.getBoardName() << "> period offset: " << mRequest.getPeriodOffset());

    const StatLeaderboard* leaderboard = mComponent->getConfigData()->getStatLeaderboard(mRequest.getBoardId(), mRequest.getBoardName());
    if (leaderboard == nullptr)
    {
        TRACE_LOG("[GetLeaderboardEntityCountCommand].execute() - invalid leaderboard ID: " << mRequest.getBoardId() 
                  << "  or Name: " << mRequest.getBoardName() << ".");        
        return GetLeaderboardEntityCountCommandStub::STATS_ERR_INVALID_LEADERBOARD_ID;
    }

    // The request need only have the user-specified keyscopes, so build the actual keyscopes to use
    // from the lb config and fill in the user-specified keyscopes from the request.
    ScopeNameValueListMap combinedScopeNameValueListMap;
    Errors err = combineScopes(*leaderboard, mRequest.getKeyScopeNameValueMap(), combinedScopeNameValueListMap);
    if (err != ERR_OK)
    {
        return err;
    }

    int32_t periodId = 0;
    if (leaderboard->getPeriodType() != STAT_PERIOD_ALL_TIME)
    {
        periodId = mRequest.getPeriodId();
        if (periodId == 0)
        {
            periodId =  mComponent->getPeriodId(leaderboard->getPeriodType());

            if (mRequest.getPeriodOffset()<0 || periodId<mRequest.getPeriodOffset())
            {
                ERR_LOG("[GetLeaderboardEntityCountCommand].execute(): bad period offset value");
                return STATS_ERR_BAD_PERIOD_OFFSET;
            }

            periodId -= mRequest.getPeriodOffset();
        }
    }

    if (leaderboard->isInMemory())
    {
        return commandErrorFromBlazeError(mComponent->getEntityCount(leaderboard, 
            &combinedScopeNameValueListMap, periodId, &mResponse));
    }
    else
    {
        const DbIdList& list = mComponent->getConfigData()->getShardConfiguration().getDbIds(leaderboard->getStatCategory()->getCategoryEntityType());
        for (DbIdList::const_iterator itr = list.begin(); itr != list.end(); ++itr)
        {
            if ((err = getDbEntityCount(*itr, leaderboard, &combinedScopeNameValueListMap, periodId)) != ERR_OK)
                return err;
        }
    }

    return ERR_OK;
}    
    

GetLeaderboardEntityCountCommandStub::Errors  GetLeaderboardEntityCountCommand::getDbEntityCount(uint32_t dbId,
    const StatLeaderboard* leaderboard, ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId)
{
    DbConnPtr conn = gDbScheduler->getReadConnPtr(dbId);
    if (conn == nullptr)
    {
        ERR_LOG("[GetLeaderboardEntityCountCommand]:getDbEntityCount() - failed to obtain connection.");
        return ERR_SYSTEM;
    }
    
    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    if (query == nullptr)
    {
        ERR_LOG("[GetLeaderboardEntityCountCommand]:getDbEntityCount() - failed to create new query.");
        return ERR_SYSTEM;
    }

    // only not-in-memory leaderboards come here
    query->append("SELECT COUNT(entity_id) FROM `$s` WHERE TRUE",
        leaderboard->getDBTableName());

    if (leaderboard->getPeriodType() != STAT_PERIOD_ALL_TIME)
    {
        query->append(" AND period_id = $d", periodId);
    }

    /// @todo refactor to use similar code in leaderboardhelper.cpp, leaderboardindex.cpp or statscommontypes.cpp
    if (!scopeNameValueListMap->empty())
    {
        ScopeNameValueListMap::const_iterator scopeMapItr = scopeNameValueListMap->begin();
        ScopeNameValueListMap::const_iterator scopeMapEnd = scopeNameValueListMap->end();
        for (; scopeMapItr != scopeMapEnd; ++scopeMapItr)
        {
            const ScopeValues* scopeValues = scopeMapItr->second;
            bool first = true;

            query->append(" AND `$s` IN (", scopeMapItr->first.c_str());

            ScopeStartEndValuesMap::const_iterator valuesItr = scopeValues->getKeyScopeValues().begin();
            ScopeStartEndValuesMap::const_iterator valuesEnd = scopeValues->getKeyScopeValues().end();
            for (; valuesItr != valuesEnd; ++valuesItr)
            {
                ScopeValue scopeValue;
                for (scopeValue = valuesItr->first; scopeValue <= valuesItr->second; ++scopeValue)
                {
                    if (!first)
                    {
                        query->append(",");
                    }

                    query->append("$D", scopeValue);
                    first = false;
                }
            }

            query->append(")");
        }
    }
    
    query->append(";\n");
    
    DbResultPtr result;
    BlazeRpcError dberr = conn->executeQuery(query, result);

    if (dberr != DB_ERR_OK)
    {
        ERR_LOG("[GetLeaderboardEntityCountCommand]:getDbEntityCount() - database error: " << getDbErrorString(dberr));
        return ERR_SYSTEM;
    }

    if (!result->empty())
    {    
        TRACE_LOG("[GetLeaderboardEntityCountCommand]:getDbEntityCount() - found " << result->size() << " results in this result");
        DbResult::const_iterator itr = result->begin();
        DbResult::const_iterator end = result->end();
        for (; itr != end; ++itr)
        {
            mResponse.setCount(mResponse.getCount() + (*itr)->getInt((uint32_t) 0));
        }
    }

    return ERR_OK;
}

GetLeaderboardEntityCountCommandStub::Errors GetLeaderboardEntityCountCommand::combineScopes(
    const StatLeaderboard& leaderboard, ScopeNameValueMap& userScopeNameValueMap, ScopeNameValueListMap& combinedScopeNameValueListMap)
{
    // Combine leaderboard-defined and user-specified keyscopes in order defined by lb config.
    // Note that we don't care if there are more user-specified keyscopes than needed.
    if (leaderboard.getHasScopes())
    {
        const ScopeNameValueListMap* lbScopeNameValueListMap = leaderboard.getScopeNameValueListMap();
        ScopeNameValueListMap::const_iterator lbScopeMapItr = lbScopeNameValueListMap->begin();
        ScopeNameValueListMap::const_iterator lbScopeMapEnd = lbScopeNameValueListMap->end();
        for (; lbScopeMapItr != lbScopeMapEnd; ++lbScopeMapItr)
        {
            if (mComponent->getConfigData()->getKeyScopeSingleValue(lbScopeMapItr->second->getKeyScopeValues()) != KEY_SCOPE_VALUE_USER)
            {
                combinedScopeNameValueListMap.insert(eastl::make_pair(lbScopeMapItr->first, lbScopeMapItr->second->clone()));
                continue;
            }

            ScopeNameValueMap::const_iterator userScopeMapItr = userScopeNameValueMap.find(lbScopeMapItr->first);
            if (userScopeMapItr == userScopeNameValueMap.end())
            {
                ERR_LOG("[GetLeaderboardEntityCountCommand].combineScopes() - leaderboard: <" << leaderboard.getBoardName() 
                        << ">: scope value for <" << lbScopeMapItr->first.c_str() << "> is missing in request");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            if (!mComponent->getConfigData()->isValidScopeValue(userScopeMapItr->first, userScopeMapItr->second))
            {
                ERR_LOG("[GetLeaderboardEntityCountCommand].combineScopes() - leaderboard: <" << leaderboard.getBoardName() 
                        << ">: scope value for <" << lbScopeMapItr->first.c_str() << "> is not defined");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            ScopeValuesPtr scopeValues = BLAZE_NEW ScopeValues();
            (scopeValues->getKeyScopeValues())[userScopeMapItr->second] = userScopeMapItr->second;
            combinedScopeNameValueListMap.insert(eastl::make_pair(lbScopeMapItr->first, scopeValues));
        }
    }

    return ERR_OK;
}

} // Stats
} // Blaze
