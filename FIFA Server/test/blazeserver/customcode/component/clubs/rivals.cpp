/*************************************************************************************************/
/*!
    \file   rivals.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// clubs includes
#include "clubs/clubsslaveimpl.h" // FIFA SPECIFIC CODE
#include "rivals.h"

// stats include files
#include "stats/rpc/statsslave_stub.h"
#include "stats/statscommontypes.h"
#include "stats/statsconfig.h"

namespace Blaze
{
namespace Clubs
{

/*************************************************************************************************/
/*!
    \brief assignRival

    Custom processing for assigning rivals

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::assignRival(ClubsDatabase * clubsDb, 
        const Club* club, bool useLeaderboard)
{
/*    BlazeRpcError result = ERR_OK;
 

    ClubSettings newClubSettings;
    club->getClubSettings().copyInto(newClubSettings);
    newClubSettings.getCustClubSettings().setCustOpt5(
        static_cast<uint32_t>(CLUBS_RIVAL_PENDING_ASSIGNMENT));
    
    ClubAcceptanceFlags acceptance;
    acceptance.setCLUBS_ACCEPT_RIVALS();
    
    ClubIdList clubIdList;
    BlazeIdList blazeIdList;

    CustClubSettings customSettings;
    customSettings.setCustOpt5(CLUBS_RIVAL_PENDING_ASSIGNMENT);

    bool checkPendingAssignment = false;
    
    // find rival in leaderboards?
    //      - this is used when end of Rival Period and trying to pair up all Ranked clubs
    if (useLeaderboard)
    {
        Stats::StatsSlaveImpl* stats = static_cast<Stats::StatsSlaveImpl*>(
                gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
        if (nullptr == stats)
        {
            ERR_LOG("[assignRival] Could not load StatsSlaveImpl");
            return Blaze::ERR_SYSTEM; // Early Return - ERROR
        }
        
        const Blaze::Stats::StatLeaderboard *lb = stats->getConfigData()->getStatLeaderboard(
            0, "ClubRivalAssignLB");
        if (nullptr == lb)
        {
            ERR_LOG("[assignRival] Could not fetch ClubRivalAssignLB leaderboard.");
            return Blaze::ERR_SYSTEM; // Early Return - ERROR
        }
        
        EntityIdList keys;
        Stats::LeaderboardStatValues lbStatResp;
        Stats::ScopeNameValueListMap scopeNameValueListMap;
        BlazeObjectId userSetId;
        bool sorted = false;
        
        // Game Customize: Change this period type to reflect your configuration for
        //                  season roll-over period and stats period for rivals.
        //                  In this example the rival period is set to DAILY.
        int32_t period = stats->getPeriodId(Stats::STAT_PERIOD_DAILY);   
        
        // fetch leaderboard for last period     
        result = stats->getCenteredLeaderboardEntries(
            *lb, &scopeNameValueListMap, period - 1, static_cast<EntityId>(club->getClubId()), 3, userSetId, false, keys, lbStatResp, sorted);
            
        if (result != Blaze::ERR_OK && result != Blaze::STATS_ERR_DB_DATA_NOT_AVAILABLE)
        {
            ERR_LOG("[assignRival] getCenteredLeaderboardStats returned an error: " << result << ".");
            return Blaze::ERR_SYSTEM;
        }
        
        if (!keys.empty())
        {
            // the club was ranked on the last leaderboard so use it
            Stats::LeaderboardStatValues::RowList &rowList = lbStatResp.getRows();
            Stats::LeaderboardStatValuesRow *row = rowList.at(0);
            if (static_cast<ClubId>(keys[0]) == club->getClubId() && keys.size() > 1)
            {
                // if I am first on leaderboard, assign me second on leaderboard
                // check if available club is ranked and has ranking stat greater than 0
                row = rowList.at(1);
                if (blaze_strcmp(row->getRankedStat(), "0"))
                    clubIdList.push_back(static_cast<ClubId>(keys[1]));
            }
            else if ((keys.size() == 2 && static_cast<ClubId>(keys[1]) == club->getClubId()) 
                        || (keys.size() == 3 && static_cast<ClubId>(keys[2]) == club->getClubId()))
            {
                // if I am last on leaderboard assign me next-to last in case my rank is even
                int i = 0;
                if (keys.size() == 3) 
                    i = 1;
                row = rowList.at(i+1);
                if ((row->getRank() & 0x1) == 0)
                { 
                     // I'm at the even ranked row, going to pair up with the upper odd ranked row
                    clubIdList.push_back(static_cast<ClubId>(keys[i]));
                }
            }
            else if (keys.size() == 3)
            {
                // if I am between two clubs on leaderboard, assign precessor or successor
                row = rowList.at(1);
                if ((row->getRank() & 0x1) == 0)
                {
                    // I'm at the even ranked row, going to pair up with the upper odd ranked row 
                    clubIdList.push_back(static_cast<ClubId>(keys.at(0)));
                }
                else
                {
                    // I'm at the odd ranked row
                    // check if available club is ranked and has ranking stat greater than 0
                    row = rowList.at(2);
                    if (blaze_strcmp(row->getRankedStat(), "0"))
                        clubIdList.push_back(static_cast<ClubId>(keys.at(2)));
                }
            }
        }
    }
    
    ClubList clubList;
    ClubList clubListAfterLock;
    ClubSettings rivalClubSettings;
    
    // find rival pending on assignment?
    if (clubIdList.empty())
    {
        // we are not pairing clubs based on leaderboard so check
        // club availability
        checkPendingAssignment = true;

        ClubTagList clubTagList;
        result = clubsDb->findClubs(false, club->getClubDomainId(), "", "", true, 0, "", acceptance,acceptance, 0, 0, 0, 0, clubIdList,
                            blazeIdList, 1 << 5, &customSettings, 0, 10, 0, true, CLUB_TAG_SEARCH_IGNORE, clubTagList, CLUB_PASSWORD_IGNORE,
                            CLUB_ACCEPTS_ALL, CLUB_ACCEPTS_ALL, clubList);
        
        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[assignRival] Find clubs returned error " << result);
            return result;
        }
    }
    else
    {
        uint64_t* versions = nullptr;
        result = clubsDb->getClubs(clubIdList, clubList, versions);

        if (versions != nullptr)
            delete [] versions;

        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[assignRival] getClubs returned error " << result);
            return result;
        }
    }

    uint64_t version = 0;
    for (ClubList::const_iterator it = clubList.begin(); 
         it != clubList.end(); it++)
    {
        const Club *rivalClub = *it;
        
        // if we found ourself then skip us 
        if (rivalClub->getClubId() == club->getClubId())
            continue;

        // Lock the 2 clubs before trying to modify
        ClubIdList clubIdLockList;
        
        clubIdLockList.push_back(club->getClubId());
        clubIdLockList.push_back(rivalClub->getClubId());
        
        result = clubsDb->lockClubs(&clubIdLockList, &clubListAfterLock);
        if (result != Blaze::ERR_OK || clubListAfterLock.size() < 2)
        {
            ERR_LOG("[assignRival] lockClubs returned an error " << result <<
                    " or club list size was less than 2");
            return result;
        }
        
        ClubList::const_iterator begin = clubListAfterLock.begin();
        if (club->getClubId() == (*begin)->getClubId())
        {
            club = *begin;
            rivalClub = *(begin + 1);
        }
        else
        {
            rivalClub = *begin;
            club = *(begin + 1);
        }
        
        if (checkPendingAssignment)
        {
        
            bool rivalAvailable = false;
            bool clubAvailable = false;
            
            result = checkIsRivalAvailable(clubsDb, rivalClub->getClubId(), rivalAvailable);
            if (result != Blaze::ERR_OK)
            {
                ERR_LOG("[assignRival] Could not check availability for rival " << rivalClub->getClubId());
                return result;
            }
            
            // if club was not eligible then no need to check 
            if (club->getClubSettings().getCustClubSettings().getCustOpt5() != CLUBS_RIVAL_NOT_ELIGIBLE)
            {
                result = checkIsRivalAvailable(clubsDb, club->getClubId(), clubAvailable);
                if (result != Blaze::ERR_OK)
                {
                    ERR_LOG("[assignRival] Could not check availability for club " << club->getClubId());
                    return result;
                }
            }
            else
                clubAvailable = true;
            
            if (!rivalAvailable || !clubAvailable)
            {
                ERR_LOG("[assignRival] Clubs " << club->getClubId() << " and " << rivalClub->getClubId() <<
                        " are not pending assignment after locking, bailing out.");
                return Blaze::ERR_SYSTEM;
            }
        }
        // Both clubs still available for Rival assignment so assign rivals

        //  Need to determinate the Rival challenge type and update both clubs here
        ClubRivalList rivalList;
        result = clubsDb->listClubRivals(club->getClubId(), &rivalList);
        if(result != ERR_OK)
        {
            WARN_LOG("assignRival] get rival list for club [" << club->getClubId() << "] returned error");
            return result;
        }

        ClubRivalList::iterator startIter = rivalList.begin();
        ClubRivalList::iterator endIter = rivalList.end();
        uint64_t uLastChallengeType1 = static_cast<uint32_t>(CLUBS_RIVAL_TYPES_COUNT);
        if(startIter != endIter)
        {
            uLastChallengeType1 = (*startIter)->getCustOpt1();
        }
             

        result = clubsDb->listClubRivals(rivalClub->getClubId(), &rivalList);
        if(result != ERR_OK)
        {
            WARN_LOG("assignRival] get rival list for club [" << rivalClub->getClubId() << "] returned error");
            return result;
        }
        startIter = rivalList.begin();
        endIter = rivalList.end();
        uint64_t uLastChallengeType2 = static_cast<uint32_t>(CLUBS_RIVAL_TYPES_COUNT);

        if(startIter != endIter)
        {
            uLastChallengeType2 = (*startIter)->getCustOpt1();
        }
        
        // Choose the challenge type.  Avoid having the same Challenge type that both clubs had in last Rival Period
        uint64_t uChallengeType = uLastChallengeType1 + 1;
        for(uint32_t uCount = 0; uCount < static_cast<uint32_t>(CLUBS_RIVAL_TYPES_COUNT); uCount++, uChallengeType++)
        {
            if(uChallengeType >= CLUBS_RIVAL_TYPES_COUNT)
            {
                uChallengeType = 0;
            }
            if(uChallengeType != uLastChallengeType2)
            {
                break;
            }
        }

        // Assign both clubs as Rival to each other
        ClubRival rival;
        rival.setRivalClubId(rivalClub->getClubId());
        rival.setCustOpt1(uChallengeType);
        result = insertClubRival(clubsDb->getDbConn(), club->getClubId(), rival);
        
        if (result != ERR_OK)
        {
            ERR_LOG("[assignRival] insertRival returned error " << result);
            return result;
        }
        
        rival.setRivalClubId(club->getClubId());
        rival.setCustOpt1(uChallengeType);
        result = insertClubRival(clubsDb->getDbConn(), rivalClub->getClubId(), rival);
            
        if (result != ERR_OK)
        {
            ERR_LOG("[assignRival] insertRival returned error " << result);
            return result;
        }

        // Update rival club's setting to "Rival Assigned"            
        rivalClub->getClubSettings().copyInto(rivalClubSettings);
        rivalClubSettings.getCustClubSettings().setCustOpt5(
            CLUBS_RIVAL_ASSIGNED);
        
        result = clubsDb->updateClubSettings(rivalClub->getClubId(), rivalClubSettings, version);
        if (result != ERR_OK)
        {
            ERR_LOG("[assignRival] updateClubSettings for rival returned error " << result);
            return result;
        }
        
        updateCachedClubInfo(version, rivalClub->getClubId(), *rivalClub);
        
        newClubSettings.getCustClubSettings().setCustOpt5(
            CLUBS_RIVAL_ASSIGNED);

        // Already paired up with a rival club, can break the for-loop
        break;
    }
    
    if (club->getClubSettings().getCustClubSettings().getCustOpt5() != 
            newClubSettings.getCustClubSettings().getCustOpt5())
    {
        // Successfully paired up with a rival club, update the club setting to "Rival Assigned"
        result = clubsDb->updateClubSettings(club->getClubId(), newClubSettings, version);
        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[assignRival] updateClubSettings returned error " << result);
            return result;
        }

        updateCachedClubInfo(version, club->getClubId(), *club);
    }
*/
    return Blaze::ERR_OK;
}


/*************************************************************************************************/
/*!
    \brief setIsRankedOnRivalLeaderboard

    Custom processing for making club eligible or non-eligible for rivial assignment based on
    rivalry leaderboard

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::setIsRankedOnRivalLeaderboard(const ClubId clubId, bool isRanked)
{
    Stats::StatsSlave* component = static_cast<Stats::StatsSlave*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID));
    Stats::UpdateStatsRequest req;
    Stats::StatRowUpdate *row;
    
    row = req.getStatUpdates().pull_back();
    // FIFA SPECIFIC CODE START
    if (nullptr == component || nullptr == row)
    {
        ERR_LOG("[ClubsSlaveImpl].setIsRankedOnRivalLeaderboard - unable to load StatsSlaveImpl");
        return Blaze::ERR_SYSTEM; // Early Return - ERROR
    }
    // FIFA SPECIFIC CODE END
    
    if (row == nullptr)
        return Blaze::ERR_SYSTEM;
    
    row->setCategory("ClubStats");
    row->setEntityId(static_cast<EntityId>(clubId));
    
    Stats::StatUpdate *update = row->getUpdates().pull_back();
    
    if (update == nullptr)
        return Blaze::ERR_SYSTEM;
        
    update->setName("rivalEligible");
    update->setValue(isRanked ? "1" : "0");
    update->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
    
    BlazeRpcError result = component->updateStats(req);
    
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].setIsRankedOnRivalLeaderboard - updateStats for club " << clubId <<
                " returned error " << result);
    }
    
    return result;
}

/*************************************************************************************************/
/*!
    \brief checkIsAvailable

    Custom processing for checking if club is available for rival assignment

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::checkIsRivalAvailable(ClubsDatabase * clubsDb, 
    const ClubId clubId, bool &isAvailable)
{
    BlazeRpcError result = Blaze::ERR_OK;
    isAvailable = false;
    Club club;
    uint64_t version = 0;

    result = clubsDb->getClub(clubId, &club, version);
    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].checkIsRivalAvailable - Failed to obtain club " << clubId << ".");
        return Blaze::ERR_SYSTEM;
    }

    uint32_t rivalOpt = club.getClubSettings().getCustClubSettings().getCustOpt5();
    if (rivalOpt == CLUBS_RIVAL_ASSIGNED)
    {
        // if club has rival assigned check if there was season rollover
        ClubRivalList rivalList;
        result = clubsDb->listClubRivals(clubId, &rivalList);
        
        if (result != Blaze::ERR_OK || rivalList.size() == 0)
        {
            ERR_LOG("[ClubsSlaveImpl].checkIsRivalAvailable - Failed to obtain rivals for club " << clubId << ".");
            return Blaze::ERR_SYSTEM;
        }
        
        const ClubRival *rival = *rivalList.begin();
        if (static_cast<int64_t>(rival->getCreationTime()) < mSeasonRollover.getSeasonStart())
        {
            // we need new rival
            if (club.getClubSettings().getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS())
                isAvailable = true;
        }
    }
    else if (rivalOpt == CLUBS_RIVAL_PENDING_ASSIGNMENT)
    {
        isAvailable = true;
    }

    return result;
}


} // Clubs
} // Blaze
