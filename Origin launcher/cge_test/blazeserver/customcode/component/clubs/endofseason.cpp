/*************************************************************************************************/
/*!
    \file   endofseason.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
// FIFA SPECIFIC CODE START
#include "stats/statsconfig.h"
#include "framework/util/shared/blazestring.h"
// FIFA SPECIFIC CODE END

// clubs includes
#include "clubs/clubsslaveimpl.h"
#include "stats/statsslaveimpl.h"
// FIFA SPECIFIC CODE START
#include "rivals.h"

/*** Defines *******************************************************************************/
const uint32_t TOP_PERCENTAGE_SEGMENT = 10; //! this is the percentage we want to give the award to 
const uint32_t MAX_LEADER_BORAD_NAME  = 32; //! this is the max number of chars that this leader board can have in it's name 
const uint32_t GOLD_AWARD_ID = 101;           //! This is the value of the gold award given to the top 10%
const uint32_t SILVER_AWARD_ID = 102;         //! This is the value of the silver award given to the top 10-20%
const uint32_t BRONZE_AWARD_ID = 103;         //! This is the value of the bronze award given to the top 20-30%
const uint32_t PREVIOUS_SEASON_OFFSET = 1000; //! This is how much to add to the season trophy in order to get the previous season ID

// We define the loc strings here instead of the clubs.cfg file because adding a loc string mapping
// requires GOS intervention (ie. clubsslaveimpl.cpp).
#define LOG_EVENT_TROPHY_SEASON_GOLD "SDB_CLUBS_SERVEREVENT_8"
#define LOG_EVENT_TROPHY_SEASON_SILVER "SDB_CLUBS_SERVEREVENT_9"
#define LOG_EVENT_TROPHY_SEASON_BRONZE "SDB_CLUBS_SERVEREVENT_10"
// FIFA SPECIFIC CODE END

namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief processEndOfSeason

    Custom end of season processing.

*/
/*************************************************************************************************/
bool ClubsSlaveImpl::processEndOfSeason(Stats::StatsSlave* component, DbConnPtr& dbConn)
{
    // FIFA SPECIFIC CODE START
    TRACE_LOG("[ClubsSlaveImpl].processEndOfSeason");
    // FIFA SPECIFIC CODE END

    BlazeRpcError error = Blaze::ERR_OK;
    
    // FIFA SPECIFIC CODE START
    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    /************************
        GARBAGE CODE BELOW JUST TO KEEP INTEGRATION TO WORK PROPERLY
        CLUB RIVAL MODE
    ************************/

/*
    {
        // This is the logic to assign online club with Rival club.
        // It has potential effect on server performance at the rollover time, 
        //  so the rollover time of each game team has to be set carefully to avoid heavy traffic time.

        // Get the Club Id list of the clubs that currently have member online
        const ClubDataCache& clubDataCache = getClubDataCache();

        ClubDataCache::const_iterator curIter = clubDataCache.begin();
        ClubDataCache::const_iterator endIter = clubDataCache.end();
        
        ClubIdList clubIdList;
        for(; curIter != endIter; curIter++)
        {
            ClubId uClubId = curIter->first;
            clubIdList.push_back(uClubId);
        }
        
        Club currentClub;
        ClubId currentClubId;
        ClubRivalList rivalList;
        int32_t iLastRolloverTime = static_cast<int32_t>(mSeasonRollover.getRollover());

        // Loop through the online club, check if they need to assign new rival pair
        for(uint32_t i = 0; i < clubIdList.size(); i++)
        {
            // Get the club
            currentClubId = clubIdList.at(i);
            error = clubsDb.getClub(currentClubId, &currentClub, version);
            if (error != Blaze::ERR_OK)
            {
                WARN_LOG("[Clubs::EndOfSeason] - Failed to obtain club " << currentClubId << 
                         ".  But, continue to process the rest of the clubs.");
                continue;
            }

            // Check and see if we need to have a new rival
            // we need new rival if we are eligible
            if (currentClub.getClubSettings().getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS()
                && currentClub.getClubInfo().getMemberCount() >= minMemberCountReqForRivals)
            {
                error = clubsDb.listClubRivals(currentClubId, &rivalList);

                if (error != Blaze::ERR_OK)
                {
                    WARN_LOG("[Clubs::EndOfSeason] - Failed to obtain rivals for club " << currentClubId <<
                             ".  But, continue to process the rest of the clubx.");
                    continue;
                }
                const ClubRival *rival = *rivalList.begin();

                if(rivalList.size() > 0)
                {
                    if(static_cast<int64_t>(rival->getCreationTime()) > iLastRolloverTime)
                    {
                        // We just got paired up in this new Season, continue to the next club
                        continue;
                    }
                }
                // This club is eligible, try to find a rival pair now
                error = assignRival(&clubsDb, &currentClub, true);
                if(error != Blaze::ERR_OK)
                {
                    WARN_LOG("[Clubs::EndOfSeason] - Failed to Assign Rival to club [" << currentClubId << "].");
                }
                else
                {
                    error = setIsRankedOnRivalLeaderboard(currentClubId, true);
                    if(error != Blaze::ERR_OK)
                    {
                        WARN_LOG("[Clubs::EndOfSeason] error listing club [" << currentClubId << "] on rival leaderboard.");
                    }
                }
            }
            else
            {
                // This club does NOT eligible for Rival pair up because
                //      either no longer play Rival mode or no longer has enough player
                ClubSettings settings;
                currentClub.getClubSettings().copyInto(settings);
                settings.getCustClubSettings().setCustOpt5(CLUBS_RIVAL_NOT_ELIGIBLE);
                error = clubsDb.updateClubSettings(currentClubId, settings, version);
                if (error != Blaze::ERR_OK)
                {
                    WARN_LOG("[Clubs::EndOfSeason] - Failed to update club settings for club " << currentClubId << ".");
                } 
                else
                {
                    error = setIsRankedOnRivalLeaderboard(currentClubId, false);
                    if(error != Blaze::ERR_OK)
                    {
                        WARN_LOG("[Clubs::EndOfSeason] - Failed to set club [" << currentClubId << "] as ranked on Rival Leaderboard");
                    }
                }
                updateCachedClubInfo(version, currentClub.getClubId(), currentClub);
            }
        }
    }
*/
    for(int32_t i=1; i<12; i++)
    {
        char8_t strLeaderBoardName[MAX_LEADER_BORAD_NAME];
        blaze_snzprintf(strLeaderBoardName, sizeof(strLeaderBoardName), "ClubRankLeague%dLB", i);

        // get the size of the actual leaderboard
        Stats::ScopeName seasonScopeKey = "season";
        Stats::ScopeValue seasonScopeValue = i;
        Stats::LeaderboardEntityCountRequest countReq;
        Stats::EntityCount countResponse;
        countReq.setBoardName(strLeaderBoardName);
        countReq.setPeriodOffset(1);
        countReq.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(seasonScopeKey, seasonScopeValue));
        error = component->getLeaderboardEntityCount(countReq, countResponse);
        if (error != Blaze::ERR_OK && error != Blaze::STATS_ERR_DB_DATA_NOT_AVAILABLE)
        {
            ERR_LOG("ClubsSlaveImpl].processEndOfSeason: error fetching the leaderboard size");
            return false;
        }
        int32_t leaderboardRowCount = countResponse.getCount();

        if (leaderboardRowCount == 0)
        {
            // There were no values for this league. Go on to the next
            TRACE_LOG("ClubsSlaveImpl].processEndOfSeason:  leaderboard "<<i<<" empty");
            continue;
        }
         
        int32_t iNumRecordsInEachSegment = 1;

        Stats::LeaderboardStatsRequest req;
        Stats::LeaderboardStatValues response;

        // Do the actual fetching of Club leader board
        req.setBoardName(strLeaderBoardName);
        req.setPeriodOffset(1);
        req.setRankStart(0);
        // to get the top 3 : Gold , Silver, Bronze and award them in on go
        req.setCount(iNumRecordsInEachSegment * 3); 
        req.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(seasonScopeKey, seasonScopeValue));
        error = component->getLeaderboard(req, response);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("ClubsSlaveImpl].processEndOfSeason: error fetching the leaderboard");
            return false;
        }
        
        // it is possible that the data that we are requesting for is not available on this leaderboard
        // in that case, we don't want to fail, but we should skip processing
        if (Blaze::ERR_OK == error)
        {
            int32_t iResponseSize = static_cast<int32_t>(response.getRows().size());

            // there are less than 30 clubs in the leader board make the awards grouping smaller
            if ((iNumRecordsInEachSegment * 3) > iResponseSize)
            {
                iNumRecordsInEachSegment = static_cast<int32_t>((response.getRows().size()) / 3);
                
                // if the division got us to zero then just set one medal to each position in the 
                // leader board
                if(0 == iNumRecordsInEachSegment)
                {
                    iNumRecordsInEachSegment = 1;
                }
            }
            
            // Award the awards based on the query
            Stats::LeaderboardStatValues::RowList::const_iterator itRows = response.getRows().begin();
            Stats::LeaderboardStatValues::RowList::const_iterator itEnd = response.getRows().end();
            for(int32_t iCurrentRank = 0; itRows != itEnd; ++itRows, ++iCurrentRank)
            {
                int32_t iAwardId = GOLD_AWARD_ID; 
                if(iCurrentRank >= iNumRecordsInEachSegment)
                {
                    iAwardId = SILVER_AWARD_ID;
                }
                if(iCurrentRank >= iNumRecordsInEachSegment*2)
                {
                    iAwardId = BRONZE_AWARD_ID;
                }

                // Award the club the award based on his standing 
                error = awardClub(dbConn, (*itRows)->getEntityId(), ClubAwardId(iAwardId));
                
                // give the previous season award 
                error = error ? awardClub(dbConn, (*itRows)->getEntityId(), ClubAwardId(iAwardId + PREVIOUS_SEASON_OFFSET)) : error; 

                // Award the top 3 with gold, silver, bronze accordingly.
                const char8_t *strAwardName = nullptr;
                switch(iCurrentRank)
                {
                case 0:
                    strAwardName = LOG_EVENT_TROPHY_SEASON_GOLD;
                    break;
                case 1:
                    strAwardName = LOG_EVENT_TROPHY_SEASON_SILVER;
                    break;
                case 2:
                    strAwardName = LOG_EVENT_TROPHY_SEASON_BRONZE;
                    break;
                default:
                    break;
                }

                if (nullptr != strAwardName)
                {
                    logEvent(&clubsDb, (*itRows)->getEntityId(), strAwardName, NEWS_PARAM_NONE, nullptr);
                }
                
                if (error != Blaze::ERR_OK)
                {
                    ERR_LOG("ClubsSlaveImpl].processEndOfSeason: error awarding club "<<(*itRows)->getEntityId()<<" ");
                    return false;
                }

            }
        }
    }
    // FIFA SPECIFIC CODE END
    return true;
}
} // Clubs
} // Blaze
