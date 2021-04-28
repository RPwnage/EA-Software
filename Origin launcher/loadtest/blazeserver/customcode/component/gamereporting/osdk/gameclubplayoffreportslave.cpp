/*************************************************************************************************/
/*!
    \file   gameclubplayoffreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/util/reportparserutil.h"

#include "gamereporting/osdk/osdkseasonalplayutil.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "gameclubplayoffreportslave.h"
#include "gamecommonlobbyskill.h"

#include "osdktournaments/tdf/osdktournamentstypes.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameClubPlayoffReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameClubPlayoffReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameClubPlayoffReportSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief GameClubPlayoffReportSlave
    Constructor
*/
/*************************************************************************************************/
GameClubPlayoffReportSlave::GameClubPlayoffReportSlave(GameReportingSlaveImpl& component) :
    GameClubReportSlave(component)
{
    // Initialize the arrays for club seasonal play and club playoff
    for (int32_t index = 0; index < MAX_CLUB_NUMBER; index++)
    {
        mTournamentId[index] = OSDKTournaments::INVALID_TOURNAMENT_ID;
        mClubSeasonState[index] = OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_NONE;
    }
}

/*************************************************************************************************/
/*! \brief GameClubPlayoffReportSlave
    Destructor
*/
/*************************************************************************************************/
GameClubPlayoffReportSlave::~GameClubPlayoffReportSlave()
{
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted to master for collation or direct to 
    processing for offline or trusted reports. Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameClubPlayoffReportSlave::validate(GameReport& report) const
{
    return Blaze::ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameClubPlayoffReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* CLUBPLAYOFF_GAME_TYPE_NAME = getCustomClubPlayoffGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), CLUBPLAYOFF_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameClubPlayoffReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // common processing
        processCommon();

        // initialize club utility with the Club IDs in the game report
        if (false == initClubUtil())
        {
            WARN_LOG("[GameClubPlayoffReportSlave::" << this << "].process(): Error initializing club util");
            delete mReportParser;
            return Blaze::ERR_SYSTEM;  // EARLY RETURN
        }

        // initialize game parameters
        initGameParams();

        if (true == skipProcess())
        {
            delete mReportParser;
            return processErr;  // EARLY RETURN
        }

        // determine win/loss/tie
        determineGameResult(); 

        // update player keyscopes
        updatePlayerKeyscopes();

        // update club keyscopes
        updateClubKeyscope();

        // parse the keyscopes from the configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
        {
            WARN_LOG("[GameClubPlayoffReportSlave::" << this << "].process(): Error parsing keyscopes");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // determine conclusion type
        if(true == skipProcessConclusionType())
        {
            updateSkipProcessConclusionTypeStats();
            delete mReportParser;
            return processErr;  // EARLY RETURN
        }

        // update player stats
        updatePlayerStats();

        // update club stats
        updateClubStats();

        // update seasonal stats
        updateClubSeasonalStats();

        // extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[GameClubPlayoffReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
            // Club Playoff does not have skill points and skill stats so do not call transformStats

            processErr = mUpdateStatsHelper.calcDerivedStats();
            TRACE_LOG("[GameClubPlayoffReportSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
        }

        if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
        {
            //  extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            mReportParser->setGameHistoryReport(&historyReport);
            mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
            delete mReportParser;
        }

        if(Blaze::ERR_OK == processErr)
        {
            // post game reporting processing()
            if (false == processUpdatedStats()) 
            {
                processErr = Blaze::ERR_SYSTEM;
                TRACE_LOG("[GameClubPlayoffReportSlave:" << this << "].process() - processUpdatedStats() returned false. Failing with ERR_SYSTEM.");
            } 
            else 
            {
                processErr = mUpdateStatsHelper.commitStats();  
                TRACE_LOG("[GameClubPlayoffReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
            }            
        }

        // send end game mail
        sendEndGameMail(playerIds);
    }

    return processErr;
}
 
/*******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void GameClubPlayoffReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief determineGameResult
    Determine the game results

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubPlayoffReportSlave::determineGameResult()
{
    determineWinClub();
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubPlayoffReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    // update player's DNF stat
    updatePlayerDNF(playerReport);
}

/*************************************************************************************************/
/*! \brief processUpdatedStats
    Perform post stats update processing

    \return - true if the post-game processing is performed successfully

    \Customizable - Virtual function to override default behavior.
    \Customizable - processCustomUpdatedStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool GameClubPlayoffReportSlave::processUpdatedStats()
{
    bool bSuccess = processCustomUpdatedStats();
    updateNotificationReport();
    return bSuccess;
}

/*************************************************************************************************/
/*! \brief updateClubKeyscope
    Update relevant club stats in the map

    \Customizable - updateCustomClubKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubPlayoffReportSlave::updateClubKeyscope()
{
    TRACE_LOG("[GameClubPlayoffReportSlave:" << this << "].updateClubKeyscope()");

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    OSDKSeasonalPlayUtil seasonalPlayUtil;
    for(int32_t index = 0; clubIter != clubIterEnd && index < MAX_CLUB_NUMBER; ++clubIter)
    {
        seasonalPlayUtil.setMember(static_cast<OSDKSeasonalPlay::MemberId>(clubIter->first), OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);
        OSDKClubGameReportBase::OSDKClubClubReport& clubReportInner = *clubIter->second;

        OSDKSeasonalPlay::SeasonState thisSeasonState = seasonalPlayUtil.getSeasonState();
        mClubSeasonState[index] = thisSeasonState;
        
        // only playoff stats are reported
        if (OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_PLAYOFF == thisSeasonState)
        {
            OSDKTournaments::TournamentId thisTournamentId = seasonalPlayUtil.getSeasonTournamentId();
            mTournamentId[index] = thisTournamentId;

            // Set the tournament ID to the Club report to be used as keyscope value
			clubReportInner.setTournamentId(thisTournamentId);
        }
    }

    updateCustomClubKeyscopes();
}

/*************************************************************************************************/
/*! \brief updateClubStats
    Update relevant club stats in the map

    \Customizable - updateCustomClubStatsPerClub() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubPlayoffReportSlave::updateClubStats()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    for(; clubIter != clubIterEnd; ++clubIter)
    {
        Clubs::ClubId clubId = clubIter->first;
        OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;

        int32_t resultValue = 0;
        bool didClubFinish = (clubReport.getClubDisc() == 0) ? true : false;

        if (mTieGame)
        {
            clubReport.setWins(0);
            clubReport.setLosses(0);
            clubReport.setTies(1);
            resultValue |= WLT_TIE;
        }
        else if (clubId == mWinClubId)
        {
            clubReport.setWins(1);
            clubReport.setLosses(0);
            clubReport.setTies(0);
            resultValue |= WLT_WIN;
        }
        else
        {
            clubReport.setWins(0);
            clubReport.setLosses(1);
            clubReport.setTies(0);
            resultValue |= WLT_LOSS;
        }

        if (false == didClubFinish)
        {
            resultValue |= WLT_DISC;
        }

        // Track the win-by-dnf
        if (clubId == mWinClubId && !mClubGameFinish)
        {
            resultValue |= WLT_OPPOQCTAG;
        }

        // Store the result value in the map
        clubReport.setGameResult(resultValue);  // STATNAME_RESULT

        updateCustomClubStatsPerClub(clubId, clubReport);
    }
}

/*************************************************************************************************/
/*! \brief updateClubSeasonalStats
    Update seasonal stats for players.

    \Customizable - updateCustomClubSeasonalStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubPlayoffReportSlave::updateClubSeasonalStats()
{
    TRACE_LOG("[GameClubPlayoffReportSlave:" << this << "].updateClubSeasonalStats()");

    for (int32_t index = 0; index < MAX_CLUB_NUMBER; index++)
    {
        if (OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_PLAYOFF ==  mClubSeasonState[index])
        {
            updatePlayoffStats(mTournamentId[index]);
        }
    }

    updateCustomClubSeasonalStats();
}

/*************************************************************************************************/
/*! \brief updatePlayoffStats
    Update the playoff stats based on game result provided that this is a tournament game

    \Customizable - updateCustomPlayoffStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubPlayoffReportSlave::updatePlayoffStats(OSDKTournaments::TournamentId tournamentId)
{
    if (OSDKTournaments::INVALID_TOURNAMENT_ID != tournamentId)
    {
        OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
        OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
        OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

        if (MAX_CLUB_NUMBER != static_cast<int32_t>(OSDKClubReportMap.size()))
        {
            return; // EARLY RETURN
        }

        OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator club0Iter, club1Iter;
        club0Iter = club1Iter = OSDKClubReportMap.begin();
        ++club1Iter;

        Clubs::ClubId clubId0 = club0Iter->first;
        Clubs::ClubId clubId1 = club1Iter->first;

        OSDKClubGameReportBase::OSDKClubClubReport& club0Map = *club0Iter->second;
        OSDKClubGameReportBase::OSDKClubClubReport& club1Map = *club1Iter->second;

        Clubs::ClubSettings club0Settings;
        mUpdateClubsUtil.getClubSettings(clubId0, club0Settings);
        Clubs::ClubSettings club1Settings;
        mUpdateClubsUtil.getClubSettings(clubId1, club1Settings);

        uint32_t team0 = club0Settings.getTeamId();
        uint32_t team1 = club1Settings.getTeamId();

        uint32_t score0 = club0Map.getScore();
        uint32_t score1 = club1Map.getScore();

        // meta-data will be filled out by custom functions in tournaments (per mode)
        char8_t metadata0[OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH];
        char8_t metadata1[OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH];
        memset(metadata0, 0, OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH);
        memset(metadata1, 0, OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH);

        updateCustomPlayoffStats(tournamentId, clubId0, clubId1, team0, team1, score0, score1, metadata0, metadata1);

		// this needs to be done *BEFORE* updating tournaments, so seasonal play can access the clubs tables.
		mUpdateClubsUtil.completeTransaction(true);

        TRACE_LOG("[GameClubPlayoffReportSlave].updatePlayoffStats() tournamentId: " << tournamentId << ", clubId0: [" << clubId0 << "], clubId1: [" << clubId1 <<
                  "], team0: " << team0 << ", team1: " << team1 << ", score0: " << score0 << ", score1: " << score1);
        mTournamentsUtil.reportMatchResult(tournamentId, clubId0, clubId1, team0, team1, score0, score1, metadata0, metadata1);
    }
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the player stats based on game result

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubPlayoffReportSlave::updatePlayerStats()
{
    TRACE_LOG("[GameClubPlayoffReportSlave:" << this << "].updatePlayerStats()");

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

        // allow update custom player's stats
        updateCustomPlayerStats(playerId, playerReport);
    }

}

}   // namespace GameReporting

}   // namespace Blaze
