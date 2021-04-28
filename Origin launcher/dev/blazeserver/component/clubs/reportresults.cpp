/*************************************************************************************************/
/*!
    \file   reportresults.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \method ClubsSlaveImpl::reportResults

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

BlazeRpcError ClubsSlaveImpl::reportResults(
        DbConnPtr& dbConn,
        ClubId upperId,
        const char8_t *gameResultForUpperClub,
        ClubId lowerId,
        const char8_t *gameResultForLowerClub)
{
    BlazeRpcError result = ERR_OK;

    if (dbConn == nullptr)
        return ERR_SYSTEM;

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    Club upperClub, lowerClub;

    do
    {
        result = clubsDb.updateLastGame(upperId, lowerId, gameResultForUpperClub);
        if (result != ERR_OK)
            break;

        result = clubsDb.updateLastGame(lowerId, upperId, gameResultForLowerClub);
        if (result != ERR_OK)
            break;

        result = clubsDb.updateClubLastActive(lowerId);
        if (result != ERR_OK)
            break;

        result = clubsDb.updateClubLastActive(upperId);
        if (result != ERR_OK)
            break;

    } while (false); // elegantlly avoiding goto

    if (result != ERR_OK)
    {
        ERR_LOG("[ReportResults] Database error (" << result << ")");
    }

    mPerfClubGamesFinished.increment();

    return result;
}

BlazeRpcError ClubsSlaveImpl::getClub(DbConnPtr& dbConn, Club &club) const
{
    BlazeRpcError result = ERR_OK;

    if (dbConn == nullptr)
        return ERR_SYSTEM;

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    uint64_t version = 0;
    result = clubsDb.getClub(club.getClubId(), &club, version);

    return result;
}

BlazeRpcError ClubsSlaveImpl::getClubSettings(DbConnPtr& dbConn, ClubId clubId, ClubSettings &clubSettings)
{
    BlazeRpcError result = ERR_OK;

    ClubDataCache::const_iterator itr = mClubDataCache.find(clubId);
    if (itr != mClubDataCache.end())
    {
        itr->second->getClubSettings().copyInto(clubSettings);
    }
    else
    {
        if (dbConn == nullptr)
            return ERR_SYSTEM;

        ClubsDatabase clubsDb;
        clubsDb.setDbConn(dbConn);
        Club club;

        uint64_t version = 0;
        result = clubsDb.getClub(clubId, &club, version);
        if (result == ERR_OK)
        {
            addClubDataToCache(clubId, club.getClubDomainId(), club.getClubSettings(), club.getName(), version);
            club.getClubSettings().copyInto(clubSettings);
        }
    }

    return result;
}

bool ClubsSlaveImpl::checkClubId(DbConnPtr& dbConn, ClubId clubId)
{
    BlazeRpcError result = ERR_OK;

    // check in cache first
    ClubDataCache::const_iterator itr = mClubDataCache.find(clubId);

    // if not found in cache go to database
    if (itr == mClubDataCache.end())
    {
        if (dbConn == nullptr)
        {
            ERR_LOG("[checkClubId] Invalid database connection");
            return false;
        }

        ClubsDatabase clubsDb;
        clubsDb.setDbConn(dbConn);

        Club club;
        uint64_t version = 0;
        result = clubsDb.getClub(clubId, &club, version);
        if (result == ERR_OK)
            addClubDataToCache(clubId, club.getClubDomainId(), club.getClubSettings(), club.getName(), version);
    }

    return result == Blaze::ERR_OK;
}

bool ClubsSlaveImpl::checkMembership(DbConnPtr& dbConn, ClubId clubId, BlazeId blazeId)
{
    BlazeRpcError result = ERR_OK;

    if (dbConn == nullptr)
    {
        ERR_LOG("[checkMembership] Invalid database connection");
        return false;
    }

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);
    MembershipStatus mshipStatus;
    MemberOnlineStatus onlStatus;
    TimeValue memberSince;
    result = getMemberStatusInClub(
        clubId, 
        blazeId,
        clubsDb, 
        mshipStatus, 
        onlStatus,
        memberSince);

    return result == Blaze::ERR_OK;
}


} // Clubs
} // Blaze

