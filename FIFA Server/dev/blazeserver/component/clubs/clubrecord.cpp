/*************************************************************************************************/
/*!
    \file   updateclubrecord.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \method ClubsSlaveImpl::updateClubRecord

    This method is used by the game reporting to send in the results of club game so that it can
    be stored into the clubs database and passed on to stats component to update club stats.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"

// clubs includes
#include "clubs/clubsidentityprovider.h"
#include "clubs/clubsslaveimpl.h"
#include "clubs/clubsdb.h"

namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/
BlazeRpcError ClubsSlaveImpl::updateClubRecord(DbConnPtr& dbCon, ClubRecordbookRecord &record)
{
    BlazeRpcError result = ERR_OK;

    if (dbCon == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbCon);

    result = clubsDb.updateClubRecord(record);

    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubRecord] Could not update club record, database error " << result);
    }
    else
        result = onRecordUpdated(&clubsDb, &record);

    return result;
}

BlazeRpcError ClubsSlaveImpl::resetClubRecords(DbConnPtr& dbCon, ClubId clubId, const ClubRecordIdList &recordIdList)
{
    BlazeRpcError result = Blaze::ERR_OK;

    if (dbCon == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbCon);

    result = clubsDb.resetClubRecords(clubId, recordIdList);

    if (result != Blaze::ERR_OK && result != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
    {
        ERR_LOG("[resetClubRecords] Database error.");
    }
    else
    {
        ClubRecordIdList::const_iterator it = recordIdList.begin();
        ClubRecordIdList::const_iterator itend = recordIdList.end();

        for (; it != itend; it++)
        {
            ClubRecordbookRecord record = { 0 };
            record.recordId = *it;
            record.clubId = clubId;
            record.blazeId = 0;
            
            result = onRecordUpdated(&clubsDb, &record);
            
            if (result != Blaze::ERR_OK)
            {
                ERR_LOG("[resetClubRecords] onRecordUpdated failed for club " << record.recordId << ", record " << record.clubId << ".");
                break;
            }
        }
     }

     return result;
}

} // Clubs
} // Blaze

