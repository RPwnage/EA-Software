/*************************************************************************************************/
/*!
    \file   seasonrollover.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// seasonal play includes
#include "osdkseasonalplay/osdkseasonalplayslaveimpl.h"
#include "logclubnewsutil.h"

// stats includes
#include "component/stats/statsconfig.h"
#include "component/stats/rpc/statsslave.h"


namespace Blaze
{
namespace OSDKSeasonalPlay
{
        
    // Ping's award "news" strings. These are the string id's of the messages inserted into
    // the club's news when it has won an award
    static const char8_t PING_CLUB_NEWS_DIV1_CHAMP[] = "PSDB_SEASONAL_CLUB_DIV1_CHAMP";
    static const char8_t PING_CLUB_NEWS_DIV2_CHAMP[] = "PSDB_SEASONAL_CLUB_DIV2_CHAMP";
    static const char8_t PING_CLUB_NEWS_DIV3_CHAMP[] = "PSDB_SEASONAL_CLUB_DIV3_CHAMP";
    static const char8_t PING_CLUB_NEWS_DIV1_REG_SEASON_CHAMP[] = "PSDB_SEASONAL_CLUB_DIV1_REG_SEASON_CHAMP";
    static const char8_t PING_CLUB_NEWS_DIV1_REG_SEASON_TOP10[] = "PSDB_SEASONAL_CLUB_DIV1_REG_SEASON_TOP10";


/*************************************************************************************************/
/*!
    \brief onSeasonRollover

    Custom processing for when a season rolls over.

    Note: this method will be called for each season id. For all seasons that roll over in the same 
    stats period (eg, daily), this method will be called sequentially for each season (the processing
    for season "m" must complete before the processing for season "n" starts). However it 
    is theoretically possible that this method could be called simultaneously for different 
    seasons if the seasons have different rollover periods and those periods happen to rollover
    at the same time.

*/
/*************************************************************************************************/
    void OSDKSeasonalPlaySlaveImpl::onSeasonRollover(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId, uint32_t oldSeasonNumber, int32_t oldPeriodId, int32_t newPeriodId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. seasonId = " << seasonId << ", oldPeriodId = "
              << oldPeriodId << ", newPeriodId = " << newPeriodId << "");

    // Ping's award configuration is below. These apply to both Club and User seasons
    //  1 = Division 1 Champ
    //  2 = Division 2 Champ
    //  3 = Division 3 Champ
    //  4 = Division 1 Regular Season Champ
    //  5 = Division 1 Regular Season Top 10
    //  6 = Won a Division 1 Playoff
    //  7 = Won a Division 2 Playoff
    //  8 = Finished in Division 1
    //  9 = Finished in Division 2

    BlazeRpcError error = ERR_OK;

    MemberType seasonMemberType = getSeasonMemberType(seasonId);
    LeagueId   seasonLeagueId = getSeasonLeagueId(seasonId);

    // 
    // Even though seasonal play supports seasons that have users as members, we will not do any processing
    // of awards for user based seasons as only Club awards are being surfaced on the consoles. However the
    // concepts below do apply to users as well as clubs.
    //
    if (SEASONALPLAY_MEMBERTYPE_USER == seasonMemberType)
    {
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Season id " << seasonId << " is a user based season. Skipping end of season processing");
        return;
    }

    // 
    // Utility object for logging club news
    //
    LogClubNewsUtil logClubNewUtil;


    //
    // the season leaderboard name that we will use to retrieve the member's stats from
    //
    const char8_t *seasonLeaderboardName = getSeasonLeaderboardName(seasonId);
    if (NULL == seasonLeaderboardName)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Unable to get the leaderboard name for season id " << seasonId <<
                ". Exiting end of season processing");
        return;
    }
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Using leaderboard " << seasonLeaderboardName <<
              " for season id " << seasonId << "");


    //
    // the stats slave 
    //
    Stats::StatsSlave* statsComponent
        = static_cast<Stats::StatsSlave*>(
        gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false));
    if (NULL == statsComponent)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Unable to get the stats slave component. Exiting end of season processing");
        return;
    }

    // In order to get the member's record for the regular season, we first need to retrieve the 
    // the Leaderboard Group. This will contain the column names of the leaderboard. With the
    // column names, we can retrieve the appropriate stat values to put into the meta data of the
    // award for the member's regular season record.
    //
    Stats::LeaderboardGroupRequest  lbGroupRequest;
    lbGroupRequest.setBoardName(seasonLeaderboardName);
    Stats::LeaderboardGroupResponse lbGroupResponse;
    
    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    error = statsComponent->getLeaderboardGroup(lbGroupRequest, lbGroupResponse);
    UserSession::popSuperUserPrivilege();

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Unable to get the leaderboard group for leaderboard "
                << seasonLeaderboardName << ". Exiting end of season processing");
        return;
    }

    //
    // determine if the leaderboard has a keyscope. Need to know this when retrieving the
    // filtered leaderboard later on
    //
    bool8_t leaderBoardHasScope = (lbGroupResponse.getKeyScopeNameValueListMap().size()!=0);

    // 

    //
    // Some temporary variable for storing member id's and rank for determining the
    // divisional champs
    //
    MemberId    div1Champ = -1, div2Champ = -1, div3Champ = -1;
    uint32_t    div1ChampRank = 0, div2ChampRank = 0, div3ChampRank = 0;
    char8_t     div1MemberRecord[24], div2MemberRecord[24], div3MemberRecord[24];

    //
    // get the list of all members registered in the season
    //
    eastl::vector<MemberId> memberList;
    error = getRegisteredMembers(dbHelper, seasonId, memberList);
    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Unable to get registered members. Exiting end of season processing");
        return;
    }

    uint32_t memberCount = memberList.size();

    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Number of members registered in the season = " << memberCount << "");

    for (uint32_t index = 0; index < memberCount; ++index)
    {
        // get the member
        MemberId memberId = memberList[index];

        // get the member's ranks and divisions
        uint32_t divisionRank = 0, divisionStartingRank = 0, overallRank =0;
        uint8_t division = 0;
        error = calculateRankAndDivision(seasonId, memberId, divisionRank, division, divisionStartingRank, overallRank, true); // true for previous season flag
        if (Blaze::ERR_OK != error)
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Skipping member. unable to calculate rank and division for memberId = " << memberId);
            continue;
        }

        // get the member's playoff status
        TournamentStatus tournStatus = getMemberTournamentStatus(dbHelper, memberId, seasonMemberType, error);
        if (Blaze::ERR_OK != error)
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Skipping member. unable to get tournament status for memberId = " << memberId);
            continue;
        }

        // member has to be ranked to get awards
        if (division > 0 && divisionRank > 0)
        {
            TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Processing memberId = " << memberId << "");

            // 
            // Get the members win/loss/games played record from the filtered leaderboard
            //
            int32_t memberWins = 0, memberLosses =0, memberGamesPlayed = 0;

            Stats::FilteredLeaderboardStatsRequest filteredLBRequest;
            filteredLBRequest.setBoardName(seasonLeaderboardName);
            filteredLBRequest.setPeriodId(oldPeriodId);
            filteredLBRequest.getListOfIds().push_back(memberId);
            if (true == leaderBoardHasScope)
            {
                Stats::ScopeName scopeName = "season";
                Stats::ScopeValue scopeValue = seasonId;
                filteredLBRequest.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(scopeName, scopeValue));
            }
            Stats::LeaderboardStatValues filteredLBResponse;
            
            // to invoke this RPC will require authentication
            UserSession::pushSuperUserPrivilege();
            error = statsComponent->getFilteredLeaderboard(filteredLBRequest, filteredLBResponse);
            UserSession::popSuperUserPrivilege();

            if (Blaze::ERR_OK != error)
            {
                WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover(): Failed to retrieve filtered leaderboard:  leaderboard = "
                          << seasonLeaderboardName << ", memberId = " << memberId);
            }

            Stats::LeaderboardStatValues::RowList::const_iterator rowItr = filteredLBResponse.getRows().begin();
            Stats::LeaderboardStatValues::RowList::const_iterator rowItr_end = filteredLBResponse.getRows().end();
            for ( ; rowItr != rowItr_end; ++rowItr)
            {
                const Stats::LeaderboardStatValuesRow *lbRow = *rowItr;

                // 
                // the name of the stats columns are in the "KeyColumnList" while the actual stat values
                // are in the "StringStatValueList".
                //
                Stats::StatDescSummaryList::const_iterator keyColumnItr = 
                    lbGroupResponse.getStatDescSummaries().begin();
                Stats::LeaderboardStatValuesRow::StringStatValueList::const_iterator columnValueItr =
                    lbRow->getOtherStats().begin();
                Stats::LeaderboardStatValuesRow::StringStatValueList::const_iterator columnValueItr_end =
                    lbRow->getOtherStats().end();

                for (; columnValueItr != columnValueItr_end; ++columnValueItr, ++keyColumnItr)
                {
                    //
                    // looking for the "wins" and "losses" columns 
                    //
                    if (!blaze_strcmp((*keyColumnItr)->getName(), "wins"))
                    {
                        memberWins = atoi(*columnValueItr);
                    }
                    else if (!blaze_strcmp((*keyColumnItr)->getName(), "losses"))
                    {
                        memberLosses = atoi(*columnValueItr);
                    }
                }
            }
            memberGamesPlayed = memberWins + memberLosses;

            // 
            // now put the record into a string which can be inserted into the award meta data.
            // The expected format by EAC Screenworks is "<wins>-<losses>|<games played>"
            //
            char8_t memberRecord[24];
            blaze_snzprintf(memberRecord, sizeof(memberRecord), "%d-%d|%d",
                memberWins, memberLosses, memberGamesPlayed);
            TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. The record for memberId = " << memberId <<
                      " is " << memberRecord << "");
            
            //
            // now start processing what awards this member may have won
            //

            // finished in division awards
            if (1 == division)
            {
                // finished in division 1
                TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Finished in Division 1 award to memberId = "
                          << memberId << "");
                assignAward(dbHelper, 8, memberId, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, false, memberRecord);
            }
            else if (2 == division)
            {
                // finished in division 2
                TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Finished in Division 2 award to memberId = "
                          << memberId << "");
                assignAward(dbHelper, 9, memberId, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, false, memberRecord);
            }

            // won a playoff awards
            if (SEASONALPLAY_TOURNAMENT_STATUS_WON == tournStatus)
            {
                if (1 == division)
                {
                    // won playoff in division 1
                    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Won Playoff in Division 1 award to memberId = " << memberId << "");
                    assignAward(dbHelper, 6, memberId, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, false, memberRecord);
                }
                else if (2 == division)
                {
                    // won playoff in division 2
                    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Won Playoff in Division 2 award to memberId = " << memberId << "");
                    assignAward(dbHelper, 7, memberId, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, false, memberRecord);
                }
            }

            // finishing in the regular season awards for division 1
            if (1 == division && 10 >= divisionRank)
            {
                // Finished in the Top 10 for division 1
                TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Division 1 Regular Season Top 10 award to memberId = " << memberId << "");
                assignAward(dbHelper, 5, memberId, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, true, memberRecord);
                logClubNewUtil.logNews(memberId, PING_CLUB_NEWS_DIV1_REG_SEASON_TOP10);

                if (1 == divisionRank)
                {
                    // Division 1 Regular Season Champ
                    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Division 1 Regular Season Champ award to memberId = " << memberId << "");
                    assignAward(dbHelper, 4, memberId, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, true, memberRecord);
                    logClubNewUtil.logNews(memberId, PING_CLUB_NEWS_DIV1_REG_SEASON_CHAMP);
                }
            }
            
            //
            // check if the member is a candidate for division champ (highest ranking regular season that has won playoff)
            // Note that "highest ranking" really means the member with the lowest rank value. 1 is higher ranked than 2.
            //
            if (SEASONALPLAY_TOURNAMENT_STATUS_WON == tournStatus)
            {
                if (1 == division)
                {
                    if (div1ChampRank == 0 || divisionRank < div1ChampRank)
                    {
                        div1Champ = memberId;
                        div1ChampRank = divisionRank;
                        blaze_strnzcpy(div1MemberRecord, memberRecord, sizeof(div1MemberRecord));
                    }
                }
                else if (2 == division)
                {
                    if (div2ChampRank == 0 || divisionRank < div2ChampRank)
                    {
                        div2Champ = memberId;
                        div2ChampRank = divisionRank;
                        blaze_strnzcpy(div2MemberRecord, memberRecord, sizeof(div1MemberRecord));
                    }
                }
                else if (3 == division)
                {
                    if (div3ChampRank == 0 || divisionRank < div3ChampRank)
                    {
                        div3Champ = memberId;
                        div3ChampRank = divisionRank;
                        blaze_strnzcpy(div3MemberRecord, memberRecord, sizeof(div1MemberRecord));
                    }
                }
            }
        }
        else
        {
            TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Skipping member. Member not ranked. memberId = " << memberId << "");
            continue;
        }
    }

    //
    // now that we're finished scanning through all the members, assign the divisional champ awards
    // TODO - how to deal with ties?
    //
    if (-1 != div1Champ)
    {
        // Division 1 Champ
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Division 1 Champ award to memberId = " << div1Champ << "");
        assignAward(dbHelper, 1, div1Champ, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, true, div1MemberRecord);
        logClubNewUtil.logNews(div1Champ, PING_CLUB_NEWS_DIV1_CHAMP);
    }
    if (-1 != div2Champ)
    {
        // Division 2 Champ
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Division 2 Champ award to memberId = " << div2Champ << "");
        assignAward(dbHelper, 2, div2Champ, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, true, div2MemberRecord);
        logClubNewUtil.logNews(div2Champ, PING_CLUB_NEWS_DIV2_CHAMP);
    }
    if (-1 != div3Champ)
    {
        // Division 3 Champ
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSeasonRollover. Assigning Division 3 Champ award to memberId = " << div3Champ << "");
        assignAward(dbHelper, 3, div3Champ, seasonMemberType, seasonId, seasonLeagueId, oldSeasonNumber, true, div3MemberRecord);
        logClubNewUtil.logNews(div3Champ, PING_CLUB_NEWS_DIV3_CHAMP);
    }

    return;
}

} // namespace OSDKSeasonalPlay
} // namespace Blaze

