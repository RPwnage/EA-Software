/*************************************************************************************************/
/*!
    \file   purgeinactiveclubs.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \brief purgeInactiveClubs

    purge inactive clubs

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// clubs includes
#include "clubsslaveimpl.h"

// stats includes
#include "stats/statsslaveimpl.h"
#include "framework/controller/controller.h"
namespace Blaze
{
namespace Clubs
{

// utility impl
uint32_t ClubsSlaveImpl::purgeInactiveClubs(DbConnPtr& conn, ClubDomainId domainId, uint32_t inactiveThreshold)
{
    TRACE_LOG("[ClubsSlaveImpl].purgeInactiveClubs() - start");

    // ASSUMING conn != nullptr !!!

    DbResultPtrs dbResults;
    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    query->append("SELECT clubId FROM clubs_clubs WHERE domainId = $u AND lastactivetime < DATE_ADD(UTC_TIMESTAMP(), INTERVAL - $u DAY);"
        " DELETE FROM clubs_clubs WHERE domainId = $u AND lastactivetime < DATE_ADD(UTC_TIMESTAMP(), INTERVAL - $u DAY);",
        domainId, inactiveThreshold, domainId, inactiveThreshold);

    TRACE_LOG("[ClubsSlaveImpl].purgeInactiveClubs() - executeQuery(): " << query->get());
    BlazeRpcError dbError = conn->executeMultiQuery(query, dbResults);
    if (dbError != DB_ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].purgeInactiveClubs() - DB error purging clubs: " << getDbErrorString(dbError));
        return 0; // assuming no club records were removed if error
    }

    TRACE_LOG("[ClubsSlaveImpl].purgeInactiveClubs() - clubs data purge successful");

    // remove purged clubs from identity cache
    DbResultPtr dbResult = *(dbResults.begin());
    DbResult::const_iterator itr = dbResult->begin();
    DbResult::const_iterator end = dbResult->end();
    
    Stats::StatsSlaveImpl* stats = 
        static_cast<Stats::StatsSlaveImpl*>
        (gController->getComponent(Stats::StatsSlave::COMPONENT_ID));   

    Stats::ScopeName scopeName = "club";
    Stats::DeleteAllStatsByKeyScopeRequest delClubMemberStatsRequest;
    Stats::DeleteAllStatsByEntityRequest delClubStatsRequest;

    for (; itr != end; ++itr)
    {
        const DbRow* row = *itr;
        uint32_t clubId = row->getUInt((uint32_t)0);
        invalidateCachedClubInfo(clubId, true);

        if (stats != nullptr)
        {
            // delete stats
            if (isUsingClubKeyscopedStats())
            {
                Stats::ScopeValue scopeValue = clubId;
                delClubMemberStatsRequest.getKeyScopeNameValueMap().clear();
                delClubMemberStatsRequest.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(scopeName, scopeValue));
                stats->deleteAllStatsByKeyScope(delClubMemberStatsRequest);
            }

            delClubStatsRequest.setEntityType(ENTITY_TYPE_CLUB);
            delClubStatsRequest.setEntityId(clubId);
            stats->deleteAllStatsByEntity(delClubStatsRequest);
        }
    }

    return (uint32_t) (dbResult->size());
}

} // Clubs
} // Blaze
