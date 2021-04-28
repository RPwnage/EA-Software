/*************************************************************************************************/
/*!
    \file   fifacoopreportslave.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/util/gamehistoryutil.h"
#include "gamereporting//util/skilldampingutil.h"
#include "gamereporting/osdk/osdkseasonalplayutil.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "osdk/gameotp.h"
#include "fifacoopreportslave.h"
#include "osdk/gamecommonlobbyskill.h"
#include "fifa/tdf/fifacoopreport.h"

#include "osdk/tdf/gameosdkreport.h"
//TODO: remove after refactor this file for the hierarchy change
#include "gamereporting/ping/gamecustom.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FifaCoopReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaCoopReportSlave::create(GameReportingSlaveImpl& component)
{
    // FifaCoopReportSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief FifaCoopReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaCoopReportSlave::FifaCoopReportSlave(GameReportingSlaveImpl& component) :
    GameOTPReportSlave(component),
	mWinCoopId(CoopSeason::COOP_ID_HOME),
    mLossCoopId(CoopSeason::COOP_ID_AWAY),
    mCoopGameFinish(false),
    mLowestCoopScore(INVALID_HIGH_SCORE),
    mHighestCoopScore(INT32_MIN),
	mIsHomeCoopSolo(false),
	mHomeUpperBlazeId(INVALID_BLAZE_ID),
	mHomeLowerBlazeId(INVALID_BLAZE_ID),
	mIsAwayCoopSolo(false),
	mAwayUpperBlazeId(INVALID_BLAZE_ID),
	mAwayLowerBlazeId(INVALID_BLAZE_ID)
{
}

/*************************************************************************************************/
/*! \brief FifaCoopReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaCoopReportSlave::~FifaCoopReportSlave()
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
BlazeRpcError FifaCoopReportSlave::validate(GameReport& report) const
{
    if (NULL == report.getReport())
    {
        WARN_LOG("[FifaCoopReportSlave:" << this << "].validate() : NULL report.");
        return Blaze::ERR_SYSTEM; // EARLY RETURN
    }

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*report.getReport());
    FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
    FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaCoopSquadReportMap = squadReport.getSquadReports();

    if (2 != fifaCoopSquadReportMap.size())
    {
        WARN_LOG("[FifaCoopReportSlave:" << this << "].validate() : There must have been exactly two squads in this game!");
        return Blaze::ERR_SYSTEM; // EARLY RETURN
    }

    return Blaze::ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError FifaCoopReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* COOP_GAME_TYPE_NAME = getCustomSquadGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), COOP_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[FifaCoopReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // common processing
        processCommon();

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

        // update squad keyscopes
        updateSquadKeyscopes();

        // parse the keyscopes from the configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
        {
            WARN_LOG("[FifaCoopReportSlave::" << this << "].process(): Error parsing keyscopes");

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

        // update squad stats
        if (false == updateSquadStats())
        {
            WARN_LOG("[FifaCoopReportSlave::" << this << "].process() Failed to commit stats");
            delete mReportParser;
            return Blaze::ERR_SYSTEM; // EARLY RETURN
        }

		selectCustomStats();

        // extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[FifaCoopReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
            if (Blaze::ERR_OK == transformStats())
            {
            }

            TRACE_LOG("[FifaCoopReportSlave:" << this << "].process() - commit stats");
            processErr = mUpdateStatsHelper.commitStats();
        }

        if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
        {
			char8_t buf[1024];
			report.getReport()->print(buf, sizeof(buf));
			TRACE_LOG("[ArsonBasicGameReportSlave:" << this << "].process() Game report right before parsing game history: " << buf);

            // extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            mReportParser->setGameHistoryReport(&historyReport);
            mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
            delete mReportParser;
        }

        // post game reporting processing()
        processErr = (true == processUpdatedStats()) ? ERR_OK : ERR_SYSTEM;

        // send end game mail
        sendEndGameMail(playerIds);
    }

    return processErr;
}
 
/*******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void FifaCoopReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief initGameParams
    Initialize game parameters that are needed for later processing

    \Customizable - Virtual function to override default behavior.
    \Customizable - initCustomGameParams() to provide additional custom behavior.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::initGameParams()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& squadReportMap = squadReport.getSquadReports();

    mCategoryId = OsdkReport.getGameReport().getCategoryId();
    mRoomId = OsdkReport.getGameReport().getRoomId();

    // could be a tie game if there is more than one squad
    mTieGame = (squadReportMap.size() > 1);

    initCustomGameParams();
}

/*************************************************************************************************/
/*! \brief determineGameResult
    Determine the game results

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::determineGameResult()
{
    determineWinSquad();
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Update Player Keyscopes

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::updatePlayerKeyscopes() 
{ 
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		GameManager::PlayerId playerId = playerIter->first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

		// Adding player's country keyscope info to the player report for reports that have set keyscope "accountcountry" in gamereporting.cfg
		const GameInfo* reportGameInfo = mProcessedReport->getGameInfo();
		if (reportGameInfo != NULL)
		{
			GameInfo::PlayerInfoMap::const_iterator citPlayer = reportGameInfo->getPlayerInfoMap().find(playerId);
			if (citPlayer != reportGameInfo->getPlayerInfoMap().end())
			{
				GameManager::GamePlayerInfo& playerInfo = *citPlayer->second;
				uint16_t accountLocale = LocaleTokenGetCountry(playerInfo.getAccountLocale());
				playerReport.setAccountCountry(accountLocale);
				TRACE_LOG("[FifaCoopReportSlave:" << this << "].updatePlayerKeyscopes() AccountCountry " << accountLocale << " for Player " << playerId);
			}
		}
	}

    updateCustomPlayerKeyscopes(); 
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    FifaCoopReportBase::FifaCoopPlayerReportBase& coopPlayerReport = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase&>(*playerReport.getCustomPlayerReport());
    coopPlayerReport.setCoopGames(1);

    // update player's DNF stat
    updatePlayerDNF(playerReport);

    updateCustomPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeWinPlayerStats
    Update winner's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateWinPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeLossPlayerStats
    Update loser's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateLossPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeTiePlayerStats
    Update tied player's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::updatePlayerStats()
{
    TRACE_LOG("[FifaCoopReportSlave:" << this << "].updatePlayerStats()");

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

        if (mTieGame)
        {
            updateGameModeTiePlayerStats(playerId, playerReport);
        }
        else if (mWinnerSet.find(playerId) != mWinnerSet.end())
        {
            updateGameModeWinPlayerStats(playerId, playerReport);
        }
        else if (mLoserSet.find(playerId) != mLoserSet.end())
        {
            updateGameModeLossPlayerStats(playerId, playerReport);
        }

        // update common player's stats
        updateCommonStats(playerId, playerReport);
    }
}

/*************************************************************************************************/
/*! \brief processUpdatedStats
    Perform post stats update processing

    \return - true if the post-game processing is performed successfully

    \Customizable - Virtual function to override default behavior.
    \Customizable - processCustomUpdatedStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool FifaCoopReportSlave::processUpdatedStats()
{
    bool success = true;

    success = success ? processCustomUpdatedStats() : success;

    return success;
}

/*************************************************************************************************/
/*! \brief determineWinSquad
    Determine the winning squad by the highest squad score

    \Customizable - None.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::determineWinSquad()
{
    int32_t prevSquadScore = INT32_MIN;
    mHighestCoopScore = INT32_MIN;
    mLowestCoopScore = INVALID_HIGH_SCORE;

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
    FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& squadReportsMap = squadReport.getSquadReports();

    FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::const_iterator squadIter, squadIterFirst, squadIterEnd;
    squadIter = squadIterFirst = squadReportsMap.begin();
    squadIterEnd = squadReportsMap.end();

    // set the first squad in the squad report map as winning squad
    mWinCoopId = squadIterFirst->first;
    mLossCoopId = squadIterFirst->first;

	mCoopGameFinish = true;
    for (; squadIter != squadIterEnd; ++squadIter)
    {
		CoopSeason::CoopId coopId = squadIter->first;
        FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadReportDetails = *squadIter->second;

        bool didSquadFinish = (0 == squadReportDetails.getSquadDisc()) ? true : false;
        int32_t squadScore = static_cast<int32_t>(squadReportDetails.getScore());
		TRACE_LOG("[FifaCoopReportSlave:" << this << "].determineWinCoop(): Coop Id:" << coopId << " score:" << squadScore << " coopFinish:" << didSquadFinish <<" ");

        if (true == didSquadFinish && squadScore > mHighestCoopScore)
        {
            // update the winning coop id and highest squad score
            mWinCoopId = coopId;
            mHighestCoopScore = squadScore;
        }
        if (true == didSquadFinish && squadScore < mLowestCoopScore)
        {
            // update the losing coop id and lowest squad score
            mLossCoopId = coopId;
            mLowestCoopScore = squadScore;
        }

        // if some of the scores are different, it isn't a tie.
        if (squadIter != squadIterFirst && squadScore != prevSquadScore)
        {
            mTieGame = false;
        }

        prevSquadScore = squadScore;
    }

	squadIter = squadIterFirst = squadReportsMap.begin();
	for (; squadIter != squadIterEnd; ++squadIter)
	{
		FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadReportDetails = *squadIter->second;
		if (1 == squadReportDetails.getSquadDisc())
		{
			mCoopGameFinish = false;
			mLossCoopId = squadIter->first;
		}
	}

    adjustSquadScore();

    //determine who the winners and losers are and write them into the report for stat update
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        FifaCoopReportBase::FifaCoopPlayerReportBase& squadPlayerReport = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase&>(*playerReport.getCustomPlayerReport());

        CoopSeason::CoopId coopId = squadPlayerReport.getCoopId();
        bool winner = (coopId == mWinCoopId);

        if (mTieGame)
		{
			TRACE_LOG("[FifaCoopReportSlave:" << this << "].determineWinCoop(): tie set - Player Id:" << playerId << " ");
            mTieSet.insert(playerId);
		}
        else if (winner)
		{
			TRACE_LOG("[FifaCoopReportSlave:" << this << "].determineWinCoop(): winner set - Player Id:" << playerId << " ");
            mWinnerSet.insert(playerId);
		}
        else
		{
			TRACE_LOG("[FifaCoopReportSlave:" << this << "].determineWinCoop(): loser set - Player Id:" << playerId << " ");
            mLoserSet.insert(playerId);
		}
    }
}

/*************************************************************************************************/
/*! \brief adjustSquadScore
    Make sure there is a minimum spread between the squad that disconnected and the other
    squad find the lowest score of those who did not cheat or quit and assign a default
    score if necessary

    \Customizable - None.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::adjustSquadScore()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
    FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& squadReportsMap = squadReport.getSquadReports();

    FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::const_iterator squadIter, squadIterEnd;
    squadIter = squadReportsMap.begin();
    squadIterEnd = squadReportsMap.end();

    bool zeroScore = false;

	TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore()");

    for(; squadIter != squadIterEnd; ++squadIter)
    {              
        FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadReportDetails = *squadIter->second;

        bool didSquadFinish = (0 == squadReportDetails.getSquadDisc()) ? true : false;
		TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - didSquadFinish: " << didSquadFinish << " ");
        if (false == didSquadFinish)
        {
            int32_t squadScore = static_cast<int32_t>(squadReportDetails.getScore());
			TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - squadScore: " << squadScore << " LowestCOOPScore: " << mLowestCoopScore << " ");

            if (mLowestCoopScore != INVALID_HIGH_SCORE && ((squadScore >= mLowestCoopScore) ||
                (mLowestCoopScore - squadScore) < DISC_SCORE_MIN_DIFF))
            {
                mTieGame = false;
                if (mLowestCoopScore - DISC_SCORE_MIN_DIFF <= 0)
                {
                    // new score for cheaters or quitters is 0
                    squadScore = 0;
                    zeroScore = true;
					TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore - setting squadScore to 0, zeroScore to true" << " ");
                }
                else
                {
                    squadScore = mLowestCoopScore - DISC_SCORE_MIN_DIFF;
					TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore - setting squadScore to LowestCoopScore - DISC_SCORE_MIN_DIFF" << " ");
                }

				TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - setting squadScore: " << squadScore << " ");
                squadReportDetails.setScore(static_cast<uint32_t>(squadScore));
            }
        }
    }

    // need adjustment on scores of non-disconnected coop
	TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore: " << zeroScore << " ");
    if (zeroScore)
    {
        squadIter = squadReportsMap.begin();
        squadIterEnd = squadReportsMap.end();

        for (; squadIter != squadIterEnd; ++squadIter)
        {
            FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadReportDetails = *squadIter->second;

            bool didSquadFinish = (0 == squadReportDetails.getSquadDisc()) ? true : false;
			TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore - didSquadFinish: " << didSquadFinish << " ");
            if (true == didSquadFinish)
            {
                int32_t squadScore = static_cast<int32_t>(squadReportDetails.getScore());
				TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore - squadScore: " << squadScore << " ");

                if (squadScore < DISC_SCORE_MIN_DIFF)
				{
					TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore - setting squadScore to DISC_SCORE_MIN_DIFF" << " ");
                    squadScore = DISC_SCORE_MIN_DIFF;
				}

                if (squadScore > mHighestCoopScore)
				{
					TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore - setting mHighestCoopScore to squadScore" << " ");
                    mHighestCoopScore = squadScore;
				}

                squadReportDetails.setScore(static_cast<int32_t>(squadScore));
				TRACE_LOG("[FifaCoopReportSlave:" << this << "].adjustSquadScore() - zeroScore - setting squadScore: " << squadScore << " ");
            }
        }
    }
}

/*************************************************************************************************/
/*! \brief updateSquadKeyscopes
    Set the value of squad keyscopes defined in config and needed for config based stat update

    \Customizable - updateCustomSquadKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void FifaCoopReportSlave::updateSquadKeyscopes()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& squadReportsMap = squadReport.getSquadReports();

    FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::const_iterator squadIter, squadIterEnd;
    squadIter = squadReportsMap.begin();
    squadIterEnd = squadReportsMap.end();

    updateCustomSquadKeyscopes();
}

/*************************************************************************************************/
/*!
    \brief updateSquadStats
    Update the squad stats, records and awards based on game result 
    provided that this is a squad game

    \Customizable - updateCustomSquadStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool FifaCoopReportSlave::updateSquadStats()
{
    bool success = true;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& squadReportsMap = squadReport.getSquadReports();

    FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::const_iterator squadIter, squadIterEnd;
    squadIter = squadReportsMap.begin();
    squadIterEnd = squadReportsMap.end();

    for(int32_t i = 0; squadIter != squadIterEnd; ++squadIter, i++)
    {
		CoopSeason::CoopId coopId = squadIter->first;
        FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadReportDetails = *squadIter->second;

        mBuilder.startStatRow(CATEGORYNAME_COOP_RANKING, coopId, NULL);

        uint32_t resultValue = 0;
        bool didSquadFinish = (squadReportDetails.getSquadDisc() == 0) ? true : false;

        if (true == mTieGame)
        {
            squadReportDetails.setWins(0);
            squadReportDetails.setLosses(0);
            squadReportDetails.setTies(1);

            resultValue |= WLT_TIE;
            mBuilder.assignStat(STATNAME_WSTREAK, (int64_t)0);
            mBuilder.assignStat(STATNAME_LSTREAK, (int64_t)0);
			mBuilder.incrementStat(STATNAME_USTREAK, 1);
        }
        else if (mWinCoopId == coopId)
        {
            squadReportDetails.setWins(1);
            squadReportDetails.setLosses(0);
            squadReportDetails.setTies(0);

            resultValue |= WLT_WIN;
            mBuilder.incrementStat(STATNAME_WSTREAK, 1);
			mBuilder.incrementStat(STATNAME_USTREAK, 1);
			mBuilder.assignStat(STATNAME_LSTREAK, (int64_t)0);
        }
        else
        {
            squadReportDetails.setWins(0);
            squadReportDetails.setLosses(1);
            squadReportDetails.setTies(0);

            resultValue |= WLT_LOSS;
            mBuilder.assignStat(STATNAME_WSTREAK, (int64_t)0);
			mBuilder.assignStat(STATNAME_USTREAK, (int64_t)0);
			mBuilder.incrementStat(STATNAME_LSTREAK, (int64_t)1);
        }

        if (false == didSquadFinish)
        {
            resultValue |= WLT_DISC;
        }

        // Track the win-by-dnf
        if ((mWinCoopId == coopId)&& (false == mCoopGameFinish))
        {
            squadReportDetails.setWinnerByDnf(1);
            resultValue |= WLT_OPPOQCTAG;
        }
        else
        {
            squadReportDetails.setWinnerByDnf(0);
        }

        // Store the result value in the map
        squadReportDetails.setGameResult(resultValue);  // STATNAME_RESULT

        updateCustomSquadStatsPerSquad(coopId, squadReportDetails);
        mBuilder.completeStatRow();
    }

    updateCustomSquadStats(OsdkReport);

    TRACE_LOG("[FifaCoopReportSlave].updateSquadStats() Processing complete");

    return success;
}

/*************************************************************************************************/
/*! \brief calcSquadDampingPercent
    Calculate the squad damping skills "wins" vs "losses"

    \return - damping skill percentage

    \Customizable - None.
*/
/*************************************************************************************************/
uint32_t FifaCoopReportSlave::calcSquadDampingPercent(CoopSeason::CoopId coopId)
{
    TRACE_LOG("[FifaCoopReportSlave].calcSquadDampingPercent() coopId: [" << coopId << "]");

    //set damping to 100% as default case
    uint32_t dampingPercent = 100;

    GameHistoryUtil gameHistoryUtil(mComponent);
    QueryVarValuesList queryVarValues;
    GameReportsList gameReportsList;

    if (coopId == mWinCoopId)
    {
        char8_t strWinClubId[32];
        blaze_snzprintf(strWinClubId, sizeof(strWinClubId), "%" PRIu64, mWinCoopId);
        queryVarValues.push_back(strWinClubId);

        if (gameHistoryUtil.getGameHistory(QUERY_COOPGAMESTATS, 10, queryVarValues, gameReportsList))
        {
            uint32_t rematchWins = 0;
            uint32_t dnfWins = 0;

            GameReportsList rematchedWinningReportsList;
            Collections::AttributeMap lossSquadMap;
            char8_t strLossClubId[32];
            blaze_snzprintf(strLossClubId, sizeof(strLossClubId), "%" PRIu64, mLossCoopId);
            (lossSquadMap)["coop_id"] = strLossClubId;
            (lossSquadMap)[STATNAME_LOSSES] = "1";

            gameHistoryUtil.getGameHistoryMatchingValues("squad", lossSquadMap, gameReportsList, rematchedWinningReportsList);
            rematchWins = rematchedWinningReportsList.getGameReportList().size();
            rematchedWinningReportsList.getGameReportList().release();

            // only check this if the current win is via DNF
            if (false == mProcessedReport->didAllPlayersFinish())
            {
                GameReportsList winnerByDnfReportsList;
                Collections::AttributeMap winnerByDnfSearchMap;
                winnerByDnfSearchMap["squad_id"] = strWinClubId;
                winnerByDnfSearchMap[ATTRIBNAME_WINNERBYDNF] = "1";

                // could use winningGameReportsList from above if the desire was to count consecutive wins by dnf in the player's last "x" winning games, rather than all consecutive games
                gameHistoryUtil.getGameHistoryConsecutiveMatchingValues("squad", winnerByDnfSearchMap, gameReportsList, winnerByDnfReportsList);
                dnfWins = winnerByDnfReportsList.getGameReportList().size();
                winnerByDnfReportsList.getGameReportList().release();
            }

            SkillDampingUtil skillDampingUtil(mComponent);
            uint32_t rematchDampingPercent = skillDampingUtil.lookupDampingPercent(rematchWins, "rematchDamping");
            uint32_t dnfWinDampingPercent = skillDampingUtil.lookupDampingPercent(dnfWins, "dnfDamping");

            TRACE_LOG("[FifaCoopReportSlave].calcSquadDampingPercent(): rematchWins: " << rematchWins << " - rematchDampingPercent: " << rematchDampingPercent);
            TRACE_LOG("[FifaCoopReportSlave].calcSquadDampingPercent(): dnfWins: " << dnfWins << " - dnfWinDampingPercent: " << dnfWinDampingPercent);

            dampingPercent = (rematchDampingPercent * dnfWinDampingPercent) / 100; // int division is fine here because the stat transform is expecting an int
            TRACE_LOG("[FifaCoopReportSlave:" << this << "].calcSquadDampingPercent(): Skill damping value " << dampingPercent << " applied to squad " << coopId << "'s stat transform.");
        }
        gameReportsList.getGameReportList().size();
    }

    return dampingPercent;
}

/*************************************************************************************************/
/*! \brief transformStats
    calculate lobby skill and lobby skill points

    \Customizable - updateCustomTransformStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
BlazeRpcError FifaCoopReportSlave::transformStats()
{
    BlazeRpcError processErr = Blaze::ERR_OK;
    if (DB_ERR_OK == mUpdateStatsHelper.fetchStats())
    {
        updateCustomTransformStats();
    }
    else
    {
        processErr = Blaze::ERR_SYSTEM;
    }
    return processErr;
}


/*************************************************************************************************/
/*! \brief getCustomOTPGameTypeName
 
    \Customizable - Virtual function to override default behavior.
    \Customizable - getCustomSquadGameTypeName() to provide additional custom behavior.
*/
/*************************************************************************************************/
const char8_t* FifaCoopReportSlave::getCustomOTPGameTypeName() const 
{ 
    return getCustomSquadGameTypeName(); 
};

/*************************************************************************************************/
/*! \brief getCustomOTPStatsCategoryName
    \Customizable - Virtual function.
*/
/*************************************************************************************************/
const char8_t* FifaCoopReportSlave::getCustomOTPStatsCategoryName() const 
{ 
    // squad has it's own stats to update, doesn't need to update the OTP stats category
    return ""; 
};

}   // namespace GameReporting

}   // namespace Blaze
