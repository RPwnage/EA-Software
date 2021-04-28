/*************************************************************************************************/
/*!
    \file   clubrivals.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
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

BlazeRpcError ClubsSlaveImpl::insertClubRival(DbConnPtr& dbCon, const ClubId clubId, const ClubRival &clubRival)
{
    BlazeRpcError result = ERR_OK;
    
    if (dbCon == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);
        
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbCon);

    Club club;
    uint64_t version = 0;
    result = clubsDb.getClub(clubId, &club, version);
    
    bool incCount = true;
    
    if (result == Blaze::ERR_OK)
    {
        if (club.getClubInfo().getRivalCount() >= static_cast<uint32_t>(getConfig().getMaxRivalsPerClub()))
        {
            bool handled = false;
            result = onMaxRivalsReached(&clubsDb, clubId, handled);
            
            if (result != Blaze::ERR_OK)
                return result;
            
            if (handled)
                return Blaze::ERR_OK;
                
            result = clubsDb.deleteOldestRival(clubId);
            
            if (result != Blaze::ERR_OK)
            {
                ERR_LOG("[ClubsSlaveImpl::insertClubRival] Could not remove oldest rival for club " << clubId << ".");
                return result;
            }
            
            incCount = false;
            
        }
         
        result = clubsDb.insertRival(clubId, clubRival);
        
        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[insertClubRival] Could not insert rival for club " << clubId << ", error " << result);
            return result;
        }
        
        if (incCount)
        {
            result = clubsDb.incrementRivalCount(clubId);
            if (result != Blaze::ERR_OK)
            {
                ERR_LOG("[insertClubRival] Could not increment rival count for club " << clubId << ", error " << result);
                return result;
            }
        }
    }
    return result;
}

BlazeRpcError ClubsSlaveImpl::updateClubRival(DbConnPtr& dbCon, const ClubId clubId, const ClubRival &clubRival) const
{
    BlazeRpcError result = ERR_OK;
    
    if (dbCon == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);
        
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbCon);

    result = clubsDb.updateRival(clubId, clubRival);
    
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[updateClubRival] Could not update rival for club " << clubId << ", error " << result);
    }
           
    return result;
}

BlazeRpcError ClubsSlaveImpl::deleteClubRivals(DbConnPtr& dbCon, const ClubId clubId, const ClubIdList &rivalIdList) const
{
    BlazeRpcError result = ERR_OK;
    
    if (dbCon == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);
        
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbCon);

    result = clubsDb.deleteRivals(clubId, rivalIdList);
    
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[deleteClubRivals] Could not delete rival for club " << clubId << ", error " << result);
    }
           
    return result;
}

BlazeRpcError ClubsSlaveImpl::deleteOldestClubRival(DbConnPtr& dbCon, const ClubId clubId) const
{
    BlazeRpcError result = ERR_OK;
    
    if (dbCon == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);
        
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbCon);

    result = clubsDb.deleteOldestRival(clubId);
    
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[deleteOldestClubRival] Could not delete oldest rival for club " << clubId << ", error " << result);
    }
           
    return result;
}

BlazeRpcError ClubsSlaveImpl::listRivals(DbConnPtr& dbConn, ClubId clubId, ClubRivalList &rivalList) const
{
    BlazeRpcError result = ERR_OK;
    
    if (dbConn == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);
        
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    result = clubsDb.listClubRivals(clubId, &rivalList);
    
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[listRivals] Could not get list of rivals for club " << clubId << ", error " << SbFormats::HexLower(result) );
    }
           
    return result;
}

} // Clubs
} // Blaze

