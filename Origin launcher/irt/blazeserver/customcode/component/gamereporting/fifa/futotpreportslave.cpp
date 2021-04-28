/*************************************************************************************************/
/*!
    \file   fifaotpreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "futotpreportslave.h"

#include "useractivitytracker/stats_updater.h"

//#include "framework/controller/controller.h"


namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FutOtpReportSlave
    Constructor
*/
/*************************************************************************************************/
FutOtpReportSlave::FutOtpReportSlave(GameReportingSlaveImpl& component)
	: GameOTPReportSlave(component)
	, mWinEntityId(Blaze::INVALID_BLAZE_ID)
	, mLossEntityId(Blaze::INVALID_BLAZE_ID)
	, mLowestScore(INVALID_HIGH_SCORE)
	, mHighestScore(INT32_MIN)
	, mGameFinish(false)
	, mReportGameTypeId(71)
	, mShouldLogLastOpponent(false) // TODO, wendy, disable last opponent for now
	//, mEnableValidation( false ) // TODO, wendy, remove collator for now
{
}
/*************************************************************************************************/
/*! \brief FutOtpReportSlave
    Destructor
*/
/*************************************************************************************************/
FutOtpReportSlave::~FutOtpReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FutOtpReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FutOtpReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FutOtpReportSlave") FutOtpReportSlave(component);
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
BlazeRpcError FutOtpReportSlave::validate(GameReport& report) const
{
    if (NULL == report.getReport())
    {
        WARN_LOG("[FutOtpReportSlave:" << this << "].validate() : NULL report.");
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
BlazeRpcError FutOtpReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

	if(isFUTMode(gameType.getGameReportName().c_str()) == true)
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[FutOtpReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

		// Add the FUT match reporting specific request to the mBuilder
		initMatchReportingStatsSelect(); // TODO, wendy, not sure if need keep this

        // common processing
        processCommon();

        // initialize game parameters
        initGameParams();

        if (true == skipProcess())
        {
			//updatePreMatchUserRating();// TODO, wendy, remove collator for now

			processErr = commitMatchReportingStats();

			// Game History is not defined here because UTAS does not use Blaze Bridge in no_contest cases.

            delete mReportParser;
            return processErr;  // EARLY RETURN
        }
		else if (isValidReport() == false)
		{
			// Mimic the FUT 18 client outcome
			updateUnadjustedScore();
			determineFUTGameResult();

			processErr = commitMatchReportingStats();

			updateGameHistory(processedReport, report);

			//NOTE We should be using GAMEREPORTING_COLLATION_ERR_NO_REPORTS, but game history is not written when we do.
			return processErr;  // EARLY RETURN
		}

		//setup the game status
		initGameStatus();

        // determine win/loss/tie
        determineGameResult(); 
		determineFUTGameResult();

        // update player keyscopes
        updatePlayerKeyscopes();

		// parse the keyscopes from the configuration
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
		{
			WARN_LOG("[FutOtpReportSlave::" << this << "].process(): Error parsing keyscopes");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

        if(EA_UNLIKELY(true == skipProcessConclusionType()))
        {
            updateSkipProcessConclusionTypeStats();
            delete mReportParser;
            return processErr;  // EARLY RETURN
        }

        // update player stats
        updatePlayerStats();

        // update game stats
        updateGameStats();

		// NOTE This function has a dependency on updateGameStats()
		selectCustomStats();

		UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
		statsUpdater.updateGamesPlayed(playerIds);
		statsUpdater.selectStats();

		// extract and set stats
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
		{
			WARN_LOG("[FutOtpReportSlave::" << this << "].process(): Error parsing stats");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

        bool strict = getComponent().getConfig().getBasicConfig().getStrictStatsUpdates();
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
			// Fetch the stats
			if (Blaze::ERR_OK == transformStats(getCustomOTPStatsCategoryName()))
			{
				statsUpdater.transformStats();
			}

            TRACE_LOG("[FutOtpReportSlave:" << this << "].process() - commit stats");
            processErr = mUpdateStatsHelper.commitStats();
        }

		updateGameHistory(processedReport, report);

        // post game reporting processing()
        processErr = (true == processUpdatedStats()) ? Blaze::ERR_OK : Blaze::ERR_SYSTEM;

		if (false == processPostStatsCommit())
		{
			processErr = ERR_SYSTEM;
		}

		// send end game mail
		sendEndGameMail(playerIds);

		// Customize GameReports for PIN Event Reporting
		CustomizeGameReportForEventReporting(processedReport, playerIds);
    }

    return processErr;
}
 
// GameCommonReportSlave implementation start
/*************************************************************************************************/
/*! \brief skipProcess
    Indicate if the rest of report processing should continue
	FUT override so we can proccess unranked games.

    \return - true if the rest of report processing is not needed

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
bool FutOtpReportSlave::skipProcess()
{   
	// Bring back the logic to count the match only when match time MaxTime for deducing UTAS to Blaze
	// load.
	if (false == mIsOfflineReport)
	{
		// stage 1 disconnect
		// no stats will be tracked
		if (mGameTime < 1)
		{
			TRACE_LOG("[FutOtpReportSlave:" << this << "].skipProcess(): End Phase 1 - Nothing counts");
			mProcessedReport->enableHistorySave(false);

			// NOTE We are not recording any failure in this case because UTAS does not read from the game history table
			// If it is necessary, enable the history save.

			return true;
		}
		// stage 2 disconnect
		// all stats will be tracked
		else
		{
			TRACE_LOG("[FutOtpReportSlave:" << this << "].skipProcess(): End Phase 2 -  Everything counts");
			return false;
		}
	}

    return false;
}

/*************************************************************************************************/
/*! \brief skipProcessConclusionTypeCommon

	\Reference: GameCommonReportSlave::skipProcessConclusionTypeCommon
*/
/*************************************************************************************************/
bool FutOtpReportSlave::skipProcessConclusionType()
{
	// We current do not have a reason to skip the game history for exceptional cases (quit, disc, desync, etc.)
	return false;
}

/*************************************************************************************************/
/*! \brief initGameParams

	\Reference: GameOTPReportSlave::initGameParams()
*/
/*************************************************************************************************/
void FutOtpReportSlave::initGameParams()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FutOTPReportBase::FutOTPTeamSummaryReport* teamSummaryReport = static_cast<FutOTPReportBase::FutOTPTeamSummaryReport*>(osdkReport.getTeamReports());
	FutOTPReportBase::FutOTPTeamReportMap& teamReportMap = teamSummaryReport->getFutOTPTeamReportMap();

    mCategoryId = osdkReport.getGameReport().getCategoryId();
    mRoomId = osdkReport.getGameReport().getRoomId();

    // could be a tie game if there is more than one squad
    mTieGame = (teamReportMap.size() > 1);

	// TODO, wendy, remove collator for now
	//const char8_t* serviceEnv = gController->getServiceEnvironment();
	//mEnableValidation = (blaze_stricmp(serviceEnv, "test") != 0 || blaze_stricmp(serviceEnv, "dev"));
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
	\Reference: GameCommonReportSlave::updatePlayerKeyscopes
*/
/*************************************************************************************************/
void FutOtpReportSlave::updatePlayerKeyscopes()
{
	// Do nothing
}

/*************************************************************************************************/
/*! \brief processUpdatedStats
    Perform post stats update processing

    \return - true if the post-game processing is performed successfully

    \Customizable - Virtual function to override default behavior.
    \Customizable - processCustomUpdatedStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool FutOtpReportSlave::processUpdatedStats()
{
	// Follw GameOTPReportSlave
	bool success = true;
    success = success ? processCustomUpdatedStats() : success;
	updateNotificationReport();
    return success;
}

/*************************************************************************************************/
/*! \brief updateNotificationReport
    Update Notification Report.

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomNotificationReport() to provide additional custom behavior.
*/
/*************************************************************************************************/
void FutOtpReportSlave::updateNotificationReport() 
{ 
    updateCustomNotificationReport(); 
};
// GameCommonReportSlave implementation end

// GameOTPReportSlave implementation start
/*************************************************************************************************/
/*! \brief getCustomOTPGameTypeName

    \return - the OTP game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FutOtpReportSlave::getCustomOTPGameTypeName() const
{
    return "gameType71";
}

/*************************************************************************************************/
/*! \brief FutOtpReportSlave
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for OTP game
*/
/*************************************************************************************************/
const char8_t* FutOtpReportSlave::getCustomOTPStatsCategoryName() const
{
    return "FUTOTPStats";
}

/*************************************************************************************************/
/*! \brief updateCustomGameStats

    Update custom player stats for FUT OPT game
*/
/*************************************************************************************************/
void FutOtpReportSlave::updateCustomGameStats()
{
	HeadToHeadLastOpponent::ExtensionData data;
	data.mProcessedReport = mProcessedReport;

	mOtpLastOpponentExtension.setExtensionData( &data );
	mFifaLastOpponent.setExtension( &mOtpLastOpponentExtension ); 
	mFifaLastOpponent.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
	\Reference: FutH2HReportSlave::updatePlayerStats()
*/
/*************************************************************************************************/
void FutOtpReportSlave::updatePlayerStats()
{
	TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerStats()");

	// aggregate players' stats
	aggregatePlayerStats();

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
			TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerStats(), TIE, playerId=" << playerId);
			playerReport.setWins(0);
			playerReport.setLosses(0);
			playerReport.setTies(1);
		}
		else if (mWinnerSet.find(playerId) != mWinnerSet.end())
		{
			TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerStats(), WIN, playerId=" << playerId);
			playerReport.setWins(1);
			playerReport.setLosses(0);
			playerReport.setTies(0);
		}
		else if (mLoserSet.find(playerId) != mLoserSet.end())
		{
			TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerStats(), LOSS, playerId=" << playerId);
			playerReport.setWins(0);
			playerReport.setLosses(1);
			playerReport.setTies(0);
		}
	}
}

/*************************************************************************************************/
/*! \brief updateGameStats
	\Reference: GameH2HReportSlave::updateGameStats

*/
/*************************************************************************************************/
void FutOtpReportSlave::updateGameStats()
{
	GameOTPReportSlave::updateGameStats();

	updateCustomGameStats();
}

/*************************************************************************************************/
/*! \brief selectCustomStats
    Select stats for processing.
*/
/*************************************************************************************************/
void FutOtpReportSlave::selectCustomStats()
{
	if (mShouldLogLastOpponent)
	{
		mFifaLastOpponent.selectLastOpponentStats();
	}
}

/*************************************************************************************************/
/*! \brief updateCustomTransformStats
	\Reference:  FutH2HReportSlave::updateCustomTransformStats

*/
/*************************************************************************************************/
void FutOtpReportSlave::updateCustomTransformStats(const char8_t* statsCategory)
{
	if (mShouldLogLastOpponent)
	{
		mFifaLastOpponent.transformLastOpponentStats();
	}

	// To be enabled if collator support is added
	// updateMatchReportingStats
}

// GameOTPReportSlave implementation end

// FifaBaseReportSlave implementation start
void FutOtpReportSlave::determineGameResult()
{
	updateUnadjustedScore();

	adjustRedCardedPlayers();
	determineWinPlayer();

	determineTeamResult(); // private

	adjustPlayerResult();

	updateSquadResults(); // private

	updatePlayerUnadjustedScore(); // private
}

Fifa::CommonGameReport* FutOtpReportSlave::getGameReport()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport.getGameReport();
	FutOTPReportBase::FutOTPGameReport* otpGameReport = static_cast<FutOTPReportBase::FutOTPGameReport*>(osdkGameReport.getCustomGameReport());

	return &otpGameReport->getCommonGameReport();
}

FutOTPReportBase::FutOTPTeamReportMap* FutOtpReportSlave::getEntityReportMap()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FutOTPReportBase::FutOTPTeamSummaryReport* teamSummaryReport = static_cast<FutOTPReportBase::FutOTPTeamSummaryReport*>(osdkReport.getTeamReports());
	FutOTPReportBase::FutOTPTeamReportMap& otpeamReportMap = teamSummaryReport->getFutOTPTeamReportMap();

	return &otpeamReportMap;
}

Fifa::CommonPlayerReport* FutOtpReportSlave::getCommonPlayerReport(FutOTPReportBase::FutOTPTeamReport& entityReport)
{
	return &entityReport.getCommonTeamReport();
}

bool FutOtpReportSlave::didEntityFinish(int64_t entityId, FutOTPReportBase::FutOTPTeamReport& entityReport)
{
	return (entityReport.getSquadDisc() == 0);
}

FutOTPReportBase::FutOTPTeamReport* FutOtpReportSlave::findEntityReport(int64_t entityId)
{
	EA::TDF::tdf_ptr<FutOTPReportBase::FutOTPTeamReport> teamReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FutOTPReportBase::FutOTPTeamSummaryReport* teamSummaryReport = static_cast<FutOTPReportBase::FutOTPTeamSummaryReport*>(osdkReport.getTeamReports());
	FutOTPReportBase::FutOTPTeamReportMap& teamReportMap = teamSummaryReport->getFutOTPTeamReportMap();

	FutOTPReportBase::FutOTPTeamReportMap::const_iterator teamIter;
	teamIter = teamReportMap.find(entityId);
	if (teamIter != teamReportMap.end())
	{
		teamReport = teamIter->second;
	}

	return teamReport;
}

int64_t FutOtpReportSlave::getWinningEntity()
{
	return mWinEntityId;
}

int64_t FutOtpReportSlave::getLosingEntity()
{
	return mLossEntityId;
}

bool FutOtpReportSlave::isTieGame()
{
	return mTieGame;
}

bool FutOtpReportSlave::isEntityWinner(int64_t entityId)
{
	return (mWinEntityId == entityId);
}

bool FutOtpReportSlave::isEntityLoser(int64_t entityId)
{
	return (mLossEntityId == entityId);
}

int32_t FutOtpReportSlave::getHighestEntityScore()
{
	return mHighestScore;
}

int32_t FutOtpReportSlave::getLowestEntityScore()
{
	return mLowestScore;
}

void FutOtpReportSlave::setCustomEntityResult(FutOTPReportBase::FutOTPTeamReport& entityReport, StatGameResultE gameResult)
{
	if (gameResult == GAME_RESULT_CONCEDE_QUIT)
	{
		entityReport.setSquadDisc(1);
	}
}

void FutOtpReportSlave::setCustomEntityScore(FutOTPReportBase::FutOTPTeamReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst)
{
	Fifa::CommonPlayerReport& commonTeamReport = entityReport.getCommonTeamReport();
	commonTeamReport.setGoals(goalsFor);
	commonTeamReport.setGoalsConceded(goalsAgainst);
}
// FifaBaseReportSlave implementation end


// Port from FutH2HReportSlave start
/*************************************************************************************************/
/*! \brief IsFUTMode
	Parse the game type name to determine whether the current mode is FUT and set the member to determine whether the opponent 
	should be logged for match making.

	\return - true if the gametype is FUT
*/
/*************************************************************************************************/
bool FutOtpReportSlave::isFUTMode(const char8_t* gameTypeName)
{
	TRACE_LOG("[FutOtpReportSlave:" << this << "].isFUTMode() " << gameTypeName << " ");

	mReportGameTypeId = FUT::GetGameTypeId(gameTypeName);

	mShouldLogLastOpponent = true;

	switch(mReportGameTypeId)
	{
	case 71:	//FUT_ONLINE_SQUAD_BATTLE
		mShouldLogLastOpponent = false;
	case 76:	//FUT_ONLINE_RIVALS
	case 73:	//FUT_ONLINE_HOUSERULES
		return true;
	}

	return false;
}

/*************************************************************************************************/
/*! \brief IsValidReport

	When the fut collator is used, we need to know if either user reported a complete report.  
	If neither do, then skip over the general processing.
*/
/*************************************************************************************************/
bool FutOtpReportSlave::isValidReport()
{
	// TODO, wendy, remove collator for now
	//OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	//FUT::CollatorReport* enhancedReport = static_cast<FUT::CollatorReport*>(OsdkReport.getEnhancedReport());

	//if (NULL != enhancedReport)
	//{
	//	return enhancedReport->getValidReport();
	//}

	// This is to mimic original behavior.  Ignore this check.
	return true;
}

/*************************************************************************************************/
/*! \brief updatePlayerFUTOTPMatchResult

	Generate the FUT match result for the player.

*/
/*************************************************************************************************/
void FutOtpReportSlave::updatePlayerFUTOTPMatchResult(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKReport& osdkReport, FUT::MatchResult playerMatchResult)
{
	TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerFUTOTPMatchResult(), playerId=" << playerId << " playerMatchResult=" << playerMatchResult);

	using namespace OSDKGameReportBase;
	
	const int64_t NORMAL_MATCH_STATUS = 0;
	
	const OSDKReport::OSDKPlayerReportsMap& playerReportMap = osdkReport.getPlayerReports();
	Fifa::GameEndedReason reason = Fifa::GAMEENDED_LOCALDISCONNECT;
	int64_t matchStatusFlags = NORMAL_MATCH_STATUS;

	// Make sure that playerId belongs to the game session.
	const GameInfo* gameInfo = mProcessedReport->getGameInfo();
	if (gameInfo != NULL)
	{
		const GameManager::GameInfo::PlayerInfoMap& playerInfoMap = gameInfo->getPlayerInfoMap();
		GameManager::GameInfo::PlayerInfoMap::const_iterator playerIt = playerInfoMap.find(playerId);
		if (playerIt == playerInfoMap.end())
		{
			WARN_LOG("[FutOtpReportSlave::" << this << "].updatePlayerFUTOTPMatchResult(): (" << playerId << ") was not found in GameInfo::PlayerInfoMap.");
			
			// Remove the following for FIFA 19 final QSPR.  This was added to investigate missing data in the Load Test Script.
			//INFO_LOG("[FutOtpReportSlave::" << this << "].updatePlayerFUTOTPMatchResult(): osdkReport: " << osdkReport);

			//collatorReport.setInvalidPlayerMap(true);

			// Leave the update when an invalid player is used.
			return;
		}
	}

	bool reasonFound = false;
	OSDKReport::OSDKPlayerReportsMap::const_iterator playerReportIt = playerReportMap.find(playerId);
	if (playerReportIt != playerReportMap.end())
	{
		OSDKPlayerReport& playerReport = *(playerReportIt->second);

		IntegerAttributeMap& playerAttributeMap = playerReport.getPrivatePlayerReport().getPrivateIntAttributeMap();
		IntegerAttributeMap::iterator playerGameEndReasonIt = playerAttributeMap.find(STR_GAMEENDREASON);
		if (playerGameEndReasonIt != playerAttributeMap.end())
		{
			reason = static_cast<Fifa::GameEndedReason>(playerGameEndReasonIt->second);
			reasonFound = true;
		}

		IntegerAttributeMap::iterator statusFlagIt = playerAttributeMap.find(STR_FUTMATCHFLAGS);
		if (statusFlagIt != playerAttributeMap.end())
		{
			matchStatusFlags = statusFlagIt->second;
		}

		TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerFUTOTPMatchResult(), playerId=" << playerId << " reasonFound=" << reasonFound << " reason=" << reason << " matchStatusFlags=" << matchStatusFlags);
	}
	else
	{
		WARN_LOG("[FutOtpReportSlave::" << this << "].updatePlayerFUTOTPMatchResult(): (" << playerId << ") was not found in OSDKPlayerReportsMap.");
		INFO_LOG("[FutOtpReportSlave::" << this << "].updatePlayerFUTOTPMatchResult(): osdkReport: " << osdkReport);
	}
	
	FUT::MatchResult matchResult = FUT::DNF;

	switch (reason) 
	{
	
	// Game Completed Normally
	case Fifa::GAMEENDED_COMPLETE:
	{
		matchResult = playerMatchResult;
	}
	break;

	// User Quit
	case Fifa::GAMEENDED_LOCALQUIT:
	{
		matchResult = FUT::QUIT;
	}
	break;

	// Teammate Left
	case Fifa::GAMEENDED_NOTENOUGHPLAYERS:
	{
		matchResult = FUT::DNF_TEAMMATE_LEFT;
	}
	break;

	// Opponent Left Mid Match
	case Fifa::GAMEENDED_REMOTEDISCONNECT:
	case Fifa::GAMEENDED_REMOTEQUIT:
	case Fifa::GAMEENDED_OPPONENT_NOTENOUGHPLAYERS:
	{
		matchResult = convertToDnf(playerMatchResult);

		// Adjust match result for FUT Online Squad Battle
		// FUT Online Squad Battle is 2v0, remote diconnect = teammate left
		if (mReportGameTypeId == 71 && reason == Fifa::GAMEENDED_REMOTEDISCONNECT)
		{
			matchResult = FUT::DNF_TEAMMATE_LEFT;
		}
	}
	break;

	// Local Disconnect
	case Fifa::GAMEENDED_LOCALDISCONNECT:
	{
		matchResult = convertToDnf(playerMatchResult);
	}
	break;

	// Something went wrong, we don't know who's fault it is
	case Fifa::GAMEENDED_DESYNC:
	{
		matchResult = convertToDnf(playerMatchResult);
	}
	break;

	case Fifa::GAMEENDED_SQUADMISMATCH:
	case Fifa::GAMEENDED_MISMATCHCHANGELIST:
	case Fifa::GAMEENDED_OVRATTR_MISMATCH:
	{
		matchResult = FUT::NO_CONTEST;
	}
	break;

	// Locally did something bad
	case Fifa::GAMEENDED_LOCALIDLE:
	case Fifa::GAMEENDED_LOCALIDLE_H2H:
	{
		matchResult = FUT::DNF_AFK;
	}
	break;

	case Fifa::GAMEENDED_OWNGOALS:
	case Fifa::GAMEENDED_OWNGOALS_H2H:
	{
		matchResult = FUT::DNF_OG;
	}
	break;

	case Fifa::GAMEENDED_CONSTRAINED:
	{
		matchResult = FUT::DNF_CONSTRAINED;
	}
	break;

	case Fifa::GAMEENDED_ALTTAB:
	{
		matchResult = FUT::QUIT;
	}
	break;

	default:
	{
		WARN_LOG("[FutOtpReportSlave::" << this << "].updatePlayerFUTOTPMatchResult(): Unknown Outcome Reported.");
		
		matchResult = FUT::NO_CONTEST;
	}
	break;
	}

	// Adjust Match Results with match status flags
	const FUT::CollatorConfig* collatorConfig = GetCollatorConfig();
	EA_ASSERT(NULL != collatorConfig);
	if (EA_LIKELY(NULL != collatorConfig) && EA_UNLIKELY(matchStatusFlags != NORMAL_MATCH_STATUS))
	{
		for (const FUT::MatchStatusConfigPtr& config : collatorConfig->getMatchStatusConfigList())
		{
			for (const FUT::ModeList::value_type& mode : config->getModeList())
			{
				if (mode == mReportGameTypeId)
				{
					int64_t maskedStatusFlags = matchStatusFlags & config->getMask();
					if (maskedStatusFlags == config->getFlag())
					{
						switch (matchResult)
						{
						case FUT::DNF_WIN:
							matchResult = FUT::DNF_DRAW;
							break;
						case FUT::DNF_LOSS:
							matchResult = FUT::QUIT;
							break;
						default:
							// Keep the result the same.
							break;
						}
					}
				}
			}
		}
	}

	TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerFUTOTPMatchResult(), playerId=" << playerId << " matchResult=" << matchResult);

	// TODO, wendy, remove collator for now
	//FUT::IndividualPlayerReportPtr& futPlayerReportPtr = collatorReport.getPlayerReportMap()[playerId];
	//if( futPlayerReportPtr == NULL )
	//{
	//	WARN_LOG("[FutOtpReportSlave::" << this << "].updatePlayerFUTOTPMatchResult(): Adding missing player id:" << playerId);
	//	INFO_LOG("[FutOtpReportSlave::" << this << "].updatePlayerFUTOTPMatchResult(): osdkReport: " << osdkReport);

	//	FUT::IndividualPlayerReport playerInfo;
	//	playerInfo.setId(playerId);
	//	playerInfo.setIsRecovered(true);

	//	futPlayerReportPtr = playerInfo.clone();
	//}

	if (reasonFound)
	{
		OSDKReport::OSDKPlayerReportsMap::const_iterator playerReportIt2 = playerReportMap.find(playerId);
		if (playerReportIt2 != playerReportMap.end())
		{
			OSDKPlayerReport& playerReport2 = *(playerReportIt2->second);
			FutOTPReportBase::FutOTPPlayerReport& otpPlayerReport = static_cast<FutOTPReportBase::FutOTPPlayerReport&>(*playerReport2.getCustomPlayerReport());
			otpPlayerReport.setMatchResult(static_cast<FutOTPReportBase::MatchResult>(matchResult));
		}
	}

	// TODO, wendy, remove collator for now
	//updateFUTUserRating(reason, *futPlayerReportPtr, collatorConfig);
}

/*************************************************************************************************/
/*! \brief updateGameHistory
	Prepare the game history data for commit.
*/
/*************************************************************************************************/
void FutOtpReportSlave::updateGameHistory(ProcessedGameReport& processedReport, GameReport& report)
{	
	// TODO, wendy, remove collator for now
	//OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*processedReport.getGameReport().getReport());
	//FUT::CollatorReport* collatorReport = static_cast<FUT::CollatorReport*>(osdkReport.getEnhancedReport());

	//if (NULL != collatorReport)
	//{
	//	FUT::PlayerReportMap& playerReportMap = collatorReport->getPlayerReportMap();
	//	for (FUT::PlayerReportMap::iterator playerReportIt = playerReportMap.begin(); playerReportIt != playerReportMap.end(); ++playerReportIt)
	//	{
	//		// Fix invalid entries in the collator report.
	//		// This was causing copies to fail in subsequent processing.
	//		if (playerReportIt->second == NULL)
	//		{
	//			WARN_LOG("[FutOtpReportSlave:" << this << "].updateGameHistory(): Player(" << playerReportIt->first << ") does not belong to the collator report for game(" << processedReport.getGameId() << ").");
	//			playerReportIt = playerReportMap.erase(playerReportIt);
	//		}
	//	}
	//}

	if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
	{
		// extract game history attributes from report.
		GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
		mReportParser->setGameHistoryReport(&historyReport);
		mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
		delete mReportParser;

		// NOTE Game history will be written after the return from process()
	}
}

// TODO, wendy, remove collator for now
/*************************************************************************************************/
/*! \brief updateFUTUserRating

	Generate the FUT match report rating for the user.

*/
/*************************************************************************************************/
//void FutOtpReportSlave::updateFUTUserRating(Fifa::GameEndedReason reason, FUT::IndividualPlayerReport& futPlayerReport, const FUT::CollatorConfig* collatorConfig)
//{
//	bool isCompMode = false;
//
//	if (EA_LIKELY(NULL != collatorConfig))
//	{
//		for (int32_t compModeId : collatorConfig->getCompetitiveModes())
//		{
//			if (compModeId == mReportGameTypeId)
//			{
//				isCompMode = true;
//			}
//		}
//
//		const char8_t * endedReasonString = Fifa::GameEndedReasonToString(reason);
//
//		if (isCompMode)
//		{
//			const FUT::EventAdjustmentMap& adjustmentMap = collatorConfig->getCompetitiveEventAdjustment();
//			FUT::EventAdjustmentMap::const_iterator adjustmentIt = adjustmentMap.find(endedReasonString);
//			if (adjustmentIt != adjustmentMap.end())
//			{
//				futPlayerReport.setCompetitiveRating(static_cast<FUT::UserRating>(adjustmentIt->second));
//			}
//		}
//		else
//		{
//			const FUT::EventAdjustmentMap& adjustmentMap = collatorConfig->getStandardEventAdjustment();
//			FUT::EventAdjustmentMap::const_iterator adjustmentIt = adjustmentMap.find(endedReasonString);
//			if (adjustmentIt != adjustmentMap.end())
//			{
//				futPlayerReport.setStandardRating(static_cast<FUT::UserRating>(adjustmentIt->second));
//			}
//		}
//	}
//}

// TODO, wendy, remove collator for now
/*************************************************************************************************/
/*! \brief updatePreMatchUserRating

	Generate the FUT match report rating for the user before the match start.

*/
/*************************************************************************************************/
//void FutOtpReportSlave::updatePreMatchUserRating()
//{
//	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
//	FUT::CollatorReport* collatorReport = static_cast<FUT::CollatorReport*>(osdkReport.getEnhancedReport());
//	const FUT::CollatorConfig* collatorConfig = GetCollatorConfig();
//
//	EA_ASSERT(NULL != collatorConfig);
//
//	if (collatorReport == NULL)
//	{
//		INFO_LOG("[FutOtpReportSlave::" << this << "].updatePreMatchUserRating(): Skip due to enhanced report missing.");
//		return;
//	}
//
//	for (OSDKReport::OSDKPlayerReportsMap::value_type playerPair : osdkReport.getPlayerReports())
//	{
//		OSDKPlayerReport& playerReport = *(playerPair.second);
//		IntegerAttributeMap& playerAttributeMap = playerReport.getPrivatePlayerReport().getPrivateIntAttributeMap();
//		IntegerAttributeMap::iterator playerGameEndReasonIt = playerAttributeMap.find(STR_GAMEENDREASON);
//		if (playerGameEndReasonIt != playerAttributeMap.end())
//		{
//			Fifa::GameEndedReason reason = static_cast<Fifa::GameEndedReason>(playerGameEndReasonIt->second);
//
//			FUT::IndividualPlayerReportPtr futPlayerReportPtr = GetIndividualPlayerReportPtr(collatorReport, playerPair.first);
//			if (NULL != futPlayerReportPtr)
//			{
//				futPlayerReportPtr->setMatchResult(FUT::NO_CONTEST);
//				updateFUTUserRating(reason, *futPlayerReportPtr, collatorConfig);
//			}
//		}
//	}
//}

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FutOtpReportSlave::updateCustomNotificationReport()
{
    // Obtain the custom data for report notification
    OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	Blaze::GameReporting::FutOTPReportBase::FUTOTPNotificationCustomGameData *gameCustomData = BLAZE_NEW Blaze::GameReporting::FutOTPReportBase::FUTOTPNotificationCustomGameData();
 	
	gameCustomData->setGameReportingId(mProcessedReport->getGameReport().getGameReportingId());

	TRACE_LOG("[FutOtpReportSlave:" << this << "].updateCustomNotificationReport() gameReportId:" << gameCustomData->getGameReportingId());

    // Set the gameCustomData to the OsdkReportNotification
    OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

/*************************************************************************************************/
/*! \brief initMatchReportingStatsSelect
	Initialize the stats request with fields required for FUT match reporting analysis.
*/
/*************************************************************************************************/
void FutOtpReportSlave::initMatchReportingStatsSelect()
{	
	// Get all the match reporting stats
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	Stats::ScopeNameValueMap playerKeyScopeMap;
	playerKeyScopeMap[FUT::MATCHREPORTING_gameType] = mReportGameTypeId;

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();
	for (; playerIter != playerEnd; ++playerIter)
	{
		Blaze::GameManager::PlayerId playerId = playerIter->first;

		mBuilder.startStatRow(FUT::MATCHREPORTING_FUTMatchReportStats, playerId, &playerKeyScopeMap);
		{
			mBuilder.selectStat(FUT::MATCHREPORTING_version);
			mBuilder.selectStat(FUT::MATCHREPORTING_total);
			mBuilder.selectStat(FUT::MATCHREPORTING_mismatch);
			mBuilder.selectStat(FUT::MATCHREPORTING_singleReport);
			mBuilder.selectStat(FUT::MATCHREPORTING_dnfMatchResult);
		}
		mBuilder.completeStatRow();

		mBuilder.startStatRow(FUT::MATCHREPORTING_FUTReputationStats, playerId);
		{
			mBuilder.selectStat(FUT::MATCHREPORTING_version);
			mBuilder.selectStat(FUT::MATCHREPORTING_stdRating);
			mBuilder.selectStat(FUT::MATCHREPORTING_cmpRating);
		}
		mBuilder.completeStatRow();
	}
}

// TODO, wendy, remove collator for now
/*************************************************************************************************/
/*! \brief updateMatchReportingStats
	Update the match reporting stats and reset the values according to gameCustomConfig::statResetList 
	in gamereporting.cfg.

	This functions depends on (DB_ERR_OK == mUpdateStatsHelper.fetchStats()).
*/
/*************************************************************************************************/
//void FutOtpReportSlave::updateMatchReportingStats()
//{
//	// Update the match reporting stats based on collation results.
//	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
//	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
//	FUT::CollatorReport* enhancedReport = static_cast<FUT::CollatorReport*>(OsdkReport.getEnhancedReport());
//
//	const FUT::CollatorConfig* collatorConfig = GetCollatorConfig();
//
//	if (NULL != enhancedReport && NULL != collatorConfig)
//	{
//		const FUT::Range& stdRange = collatorConfig->getStandardRatingRange();
//		const FUT::Range& cmpRange = collatorConfig->getCompetitiveRatingRange();
//
//		Stats::ScopeNameValueMap playerKeyScopeMap;
//		playerKeyScopeMap[FUT::MATCHREPORTING_gameType] = mReportGameTypeId;
//
//		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
//		playerIter = OsdkPlayerReportsMap.begin();
//		playerEnd = OsdkPlayerReportsMap.end();
//		for (; playerIter != playerEnd; ++playerIter)
//		{
//			//OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = playerIter->second;
//			Blaze::GameManager::PlayerId playerId = playerIter->first;
//			FUT::IndividualPlayerReportPtr futPlayerReportPtr = GetIndividualPlayerReportPtr(enhancedReport, playerId);
//
//			const Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(FUT::MATCHREPORTING_FUTMatchReportStats, playerId, &playerKeyScopeMap);
//			if(NULL != key)
//			{				
//				// The number of elements in the configuration is the version
//				const FUT::StatResetList& statResetList = collatorConfig->getStatResetList();
//				int64_t globalStatsVersion = statResetList.size();
//
//				for (int32_t periodTypeIt = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodTypeIt < Stats::STAT_NUM_PERIODS; ++periodTypeIt)
//				{
//					Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(periodTypeIt);
//					const char* colName = NULL;
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_version;
//					int64_t version = mUpdateStatsHelper.getValueInt(key, FUT::MATCHREPORTING_version, periodType, true);
//
//					// Fix test version edits
//					if (version > globalStatsVersion)
//					{
//						WARN_LOG("[FutOtpReportSlave::" << this << "].updateCustomTransformStats(): Version inversion. new:" << globalStatsVersion << " old:" << version);
//						version = globalStatsVersion;
//					}
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_total;
//					int64_t total = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);
//
//					checkColumnReset(total, colName, statResetList, version, globalStatsVersion, collatorConfig);
//
//					total += 1;  // Increment total matches
//
//					mUpdateStatsHelper.setValueInt(key, colName, periodType, total);
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_mismatch;
//					int64_t mismatch = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);
//
//					checkColumnReset(mismatch, colName, statResetList, version, globalStatsVersion, collatorConfig);
//
//					if (enhancedReport->getCollision() == true)
//					{
//						++mismatch;
//
//						if (Stats::STAT_PERIOD_ALL_TIME == periodType)
//						{
//							Blaze::GameReporting::UpdateMetricRequest metricRequest;
//							metricRequest.setMetricName("FUT_mismatch");
//							metricRequest.setValue(mReportGameTypeId);
//
//							getComponent().processUpdateMetric(metricRequest, NULL);
//						}
//					}
//
//					mUpdateStatsHelper.setValueInt(key, colName, periodType, mismatch);
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_singleReport;
//					int64_t singleReport = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);
//
//					checkColumnReset(singleReport, colName, statResetList, version, globalStatsVersion, collatorConfig);
//
//					if (enhancedReport->getReportCount() < 2)
//					{
//						++singleReport;
//					}
//
//					mUpdateStatsHelper.setValueInt(key, colName, periodType, singleReport);
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_dnfMatchResult;
//					int64_t dnfMatchResult = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);
//
//					checkColumnReset(dnfMatchResult, colName, statResetList, version, globalStatsVersion, collatorConfig);
//
//					if (NULL != futPlayerReportPtr)
//					{
//						switch (futPlayerReportPtr->getMatchResult())
//						{
//						case FUT::DNF_WIN:
//						case FUT::DNF_DRAW:
//						case FUT::DNF_LOSS:
//							++dnfMatchResult;
//							break;
//
//						default:
//							// Don't count it
//							break;
//						}
//					}
//					
//					mUpdateStatsHelper.setValueInt(key, colName, periodType, dnfMatchResult);
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_critMissMatch;
//					int64_t critMissMatch = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);
//
//					checkColumnReset(critMissMatch, colName, statResetList, version, globalStatsVersion, collatorConfig);
//
//					if (enhancedReport->getCritMissMatch() == true)
//					{
//						++critMissMatch;
//					}
//
//					mUpdateStatsHelper.setValueInt(key, colName, periodType, critMissMatch);
//
//					//----------------------------------------------------------------------------------
//					// Update the version after all the stats have been updated
//					mUpdateStatsHelper.setValueInt(key, FUT::MATCHREPORTING_version, periodType, globalStatsVersion);
//
//				} // for periodTypeIt in FUTMatchReportStats
//			}
//
//			key = mBuilder.getUpdateRowKey(FUT::MATCHREPORTING_FUTReputationStats, playerId, NULL);
//			
//			if (NULL != key)
//			{
//				const FUT::StatResetList& statResetList = collatorConfig->getRatingResetList();
//				int64_t globalStatsVersion = statResetList.size();
//
//				for (int32_t periodTypeIt = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); NULL != key && periodTypeIt < Stats::STAT_NUM_PERIODS; ++periodTypeIt)
//				{
//					Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(periodTypeIt);
//					const char* colName = NULL;
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_version;
//					int64_t version = mUpdateStatsHelper.getValueInt(key, FUT::MATCHREPORTING_version, periodType, true);
//
//					// Fix test version edits
//					if (version > globalStatsVersion)
//					{
//						WARN_LOG("[FutOtpReportSlave::" << this << "].updateCustomTransformStats(): Version inversion. new:" << globalStatsVersion << " old:" << version);
//						version = globalStatsVersion;
//					}
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_stdRating;
//					int64_t stdRating = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, colName, periodType, true));
//
//					checkColumnReset(stdRating, colName, statResetList, version, globalStatsVersion, collatorConfig);
//
//					if( NULL != futPlayerReportPtr)
//					{
//						stdRating = ClampToRange(stdRating, futPlayerReportPtr->getStandardRating(), stdRange);
//					}
//
//					mUpdateStatsHelper.setValueInt(key, colName, periodType, stdRating);
//
//					//----------------------------------------------------------------------------------
//					colName = FUT::MATCHREPORTING_cmpRating;
//					int64_t cmpRating = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, colName, periodType, true));
//
//					checkColumnReset(cmpRating, colName, statResetList, version, globalStatsVersion, collatorConfig);
//
//					if (NULL != futPlayerReportPtr)
//					{
//						cmpRating = ClampToRange(cmpRating, futPlayerReportPtr->getCompetitiveRating(), cmpRange);
//					}
//
//					mUpdateStatsHelper.setValueInt(key, colName, periodType, cmpRating);
//
//					//----------------------------------------------------------------------------------
//					// Update the version after all the stats have been updated
//					mUpdateStatsHelper.setValueInt(key, FUT::MATCHREPORTING_version, periodType, globalStatsVersion);
//
//				} // for periodTypeIt in FUTReputationStats
//			}
//		} // for playerIter
//	} // if enhancedReport is valid
//}

/*************************************************************************************************/
/*! \brief determineFUTGameResult

	Generate the match results for UTAS.

*/
/*************************************************************************************************/
void FutOtpReportSlave::determineFUTGameResult()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	int32_t playerScore = 0;

	for (OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::value_type playerIter : OsdkPlayerReportsMap)
	{
		GameManager::PlayerId playerId = playerIter.first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = *(playerIter.second);
		FutOTPReportBase::FutOTPPlayerReport& otpPlayerReport = static_cast<FutOTPReportBase::FutOTPPlayerReport&>(*playerReport.getCustomPlayerReport());
		
		FUT::MatchResult matchResult = FUT::NO_CONTEST;

		// FUT Online Squad Battle is 2v0, since AI side without player, so determine result by team result
		if (mReportGameTypeId == 71)
		{
			if (mTieGame)
			{
				TRACE_LOG("[FutOtpReportSlave:" << this << "].determineFUTGameResult() for Online Squad Battle: TIE - Player Id:" << playerId << " isHome=" << playerReport.getHome() << " TeamEntityId=" << otpPlayerReport.getTeamEntityId());
				matchResult = FUT::DRAW;
				mTieSet.insert(playerId);
			}
			else if (otpPlayerReport.getTeamEntityId() == mWinEntityId)
			{
				TRACE_LOG("[FutOtpReportSlave:" << this << "].determineFUTGameResult() for Online Squad Battle: WIN - Player Id:" << playerId << " isHome=" << playerReport.getHome() << " TeamEntityId=" << otpPlayerReport.getTeamEntityId());
				matchResult = FUT::WIN;
				mWinnerSet.insert(playerId);
			}
			else
			{
				TRACE_LOG("[FutOtpReportSlave:" << this << "].determineFUTGameResult() for Online Squad Battle: LOSS - Player Id:" << playerId << " isHome=" << playerReport.getHome() << " TeamEntityId=" << otpPlayerReport.getTeamEntityId());
				matchResult = FUT::LOSS;
				mLoserSet.insert(playerId);
			}
		}
		else
		{
			// Follow GameCommonReportSlave::determineWinPlayer() logic
			playerScore = static_cast<int32_t>(playerReport.getScore());

			bool winner = (playerScore > 0);
			if (mPlayerReportMapSize > 1)
				winner = (playerScore == mHighestPlayerScore);

			if (mTieGame)
			{
				TRACE_LOG("[FutOtpReportSlave:" << this << "].determineFUTGameResult(): TIE - Player Id:" << playerId << " isHome=" << playerReport.getHome() << " TeamEntityId=" << otpPlayerReport.getTeamEntityId());
				matchResult = FUT::DRAW;
			}
			else if (winner)
			{
				TRACE_LOG("[FutOtpReportSlave:" << this << "].determineFUTGameResult(): WIN - Player Id:" << playerId << " isHome=" << playerReport.getHome() << " TeamEntityId=" << otpPlayerReport.getTeamEntityId());
				matchResult = FUT::WIN;
			}
			else
			{
				TRACE_LOG("[FutOtpReportSlave:" << this << "].determineFUTGameResult(): LOSS - Player Id:" << playerId << " isHome=" << playerReport.getHome() << " TeamEntityId=" << otpPlayerReport.getTeamEntityId());
				matchResult = FUT::LOSS;
			}
		}

		updatePlayerFUTOTPMatchResult(playerId, OsdkReport, matchResult);
	}
}

/*************************************************************************************************/
/*! \brief commitMatchReportingStats
	Commit match reporting stats to the db on early exits.
*/
/*************************************************************************************************/
BlazeRpcError FutOtpReportSlave::commitMatchReportingStats()
{
	bool strict = getComponent().getConfig().getBasicConfig().getStrictStatsUpdates();
	BlazeRpcError processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

	if (Blaze::ERR_OK == processErr)
	{
		// Force the fetch stats here since it isn't done in the transformStats
		if( DB_ERR_OK == mUpdateStatsHelper.fetchStats())
		{
			//updateMatchReportingStats(); // TODO, wendy, remove collator for now

			TRACE_LOG("[FutOtpReportSlave:" << this << "].commitMatchReportingStats() - commit stats");
			processErr = mUpdateStatsHelper.commitStats();
		}
	}

	return processErr;
}

/*************************************************************************************************/
/*! \brief GetCollatorConfig
    Return the configuration for FUT match reporting

    \return - Return the configuration for FUT match reporting
*/
/*************************************************************************************************/
const FUT::CollatorConfig* FutOtpReportSlave::GetCollatorConfig()
{
	const EA::TDF::Tdf *customTdf = getComponent().getConfig().getCustomGlobalConfig();
	const FUT::CollatorConfig* collatorConfig = NULL;

	if (NULL != customTdf)
	{
		const OSDKGameReportBase::OSDKCustomGlobalConfig& customConfig = static_cast<const OSDKGameReportBase::OSDKCustomGlobalConfig&>(*customTdf);
		collatorConfig = static_cast<const FUT::CollatorConfig*>(customConfig.getGameCustomConfig());
	}

	return collatorConfig;
}

void FutOtpReportSlave::determineTeamResult()
{
	int32_t prevSquadScore = INT32_MIN;
	mHighestScore = INT32_MIN;
	mLowestScore = INVALID_HIGH_SCORE;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FutOTPReportBase::FutOTPTeamSummaryReport* teamSummaryReport = static_cast<FutOTPReportBase::FutOTPTeamSummaryReport*>(osdkReport.getTeamReports());
	FutOTPReportBase::FutOTPTeamReportMap& teamReportMap = teamSummaryReport->getFutOTPTeamReportMap();

	FutOTPReportBase::FutOTPTeamReportMap::const_iterator squadIter, squadIterFirst, squadIterEnd;
	squadIter = squadIterFirst = teamReportMap.begin();
	squadIterEnd = teamReportMap.end();

	// set the first squad in the squad report map as winning squad
	mWinEntityId = squadIterFirst->first;
	mLossEntityId = squadIterFirst->first;

	mGameFinish = true;
	for (; squadIter != squadIterEnd; ++squadIter)
	{
		int64_t teamEntityId = squadIter->first;
		FutOTPReportBase::FutOTPTeamReport& squadReportDetails = *squadIter->second;

		bool didSquadFinish = (0 == squadReportDetails.getSquadDisc()) ? true : false;
		int32_t squadScore = static_cast<int32_t>(squadReportDetails.getScore());
		TRACE_LOG("[FutOtpReportSlave:" << this << "].determineTeamResult(), Team Entity Id:" << teamEntityId << " score:" << squadScore << " didSquadFinish:" << didSquadFinish);

		if (true == didSquadFinish && squadScore > mHighestScore)
		{
			// update the winning team entity id and highest squad score
			mWinEntityId = teamEntityId;
			mHighestScore = squadScore;
		}
		if (true == didSquadFinish && squadScore < mLowestScore)
		{
			// update the losing team entity id and lowest squad score
			mLossEntityId = teamEntityId;
			mLowestScore = squadScore;
		}

		// if some of the scores are different, it isn't a tie.
		if (squadIter != squadIterFirst && squadScore != prevSquadScore)
		{
			mTieGame = false;
		}

		prevSquadScore = squadScore;
	}

	squadIter = squadIterFirst = teamReportMap.begin();
	for (; squadIter != squadIterEnd; ++squadIter)
	{
		FutOTPReportBase::FutOTPTeamReport& squadReportDetails = *squadIter->second;
		if (1 == squadReportDetails.getSquadDisc())
		{
			mGameFinish = false;
			mLossEntityId = squadIter->first;
		}
	}

	TRACE_LOG("[FutOtpReportSlave:" << this << "].determineTeamResult(), mWinEntityId=" << mWinEntityId << " mLossEntityId=" << mLossEntityId);
}

bool FutOtpReportSlave::updateSquadResults()
{
	bool success = true;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FutOTPReportBase::FutOTPTeamSummaryReport* teamSummaryReport = static_cast<FutOTPReportBase::FutOTPTeamSummaryReport*>(osdkReport.getTeamReports());
	FutOTPReportBase::FutOTPTeamReportMap& futTeamReportMap = teamSummaryReport->getFutOTPTeamReportMap();

	FutOTPReportBase::FutOTPTeamReportMap::const_iterator squadIter, squadIterEnd;
	squadIter = futTeamReportMap.begin();
	squadIterEnd = futTeamReportMap.end();

	for (int32_t i = 0; squadIter != squadIterEnd; ++squadIter, i++)
	{
		int64_t teamEntityId = squadIter->first;
		FutOTPReportBase::FutOTPTeamReport& squadReportDetails = *squadIter->second;

		uint32_t resultValue = 0;
		bool didSquadFinish = (squadReportDetails.getSquadDisc() == 0) ? true : false;

		if (mTieGame)
		{
			squadReportDetails.setWins(0);
			squadReportDetails.setLosses(0);
			squadReportDetails.setTies(1);

			resultValue |= WLT_TIE;
			TRACE_LOG("[FutOtpReportSlave:" << this << "].updateSquadResults() TIE, teamEntityId=" << teamEntityId);
		}
		else if (mWinEntityId == teamEntityId)
		{
			squadReportDetails.setWins(1);
			squadReportDetails.setLosses(0);
			squadReportDetails.setTies(0);

			resultValue |= WLT_WIN;
			TRACE_LOG("[FutOtpReportSlave:" << this << "].updateSquadResults() WIN, teamEntityId=" << teamEntityId);
		}
		else
		{
			squadReportDetails.setWins(0);
			squadReportDetails.setLosses(1);
			squadReportDetails.setTies(0);

			resultValue |= WLT_LOSS;
			TRACE_LOG("[FutOtpReportSlave:" << this << "].updateSquadResults() LOSS, teamEntityId=" << teamEntityId);
		}

		if (false == didSquadFinish)
		{
			resultValue |= WLT_DISC;
		}

		// Track the win-by-dnf
		if ((mWinEntityId == teamEntityId) && (false == mGameFinish))
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
	}

	//TRACE_LOG("[FutOtpReportSlave].updateSquadResults() Processing complete");

	return success;
}

void FutOtpReportSlave::updatePlayerUnadjustedScore()
{
	// Copy unadjusted scroe from team report to player report
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	
	// team report
	FutOTPReportBase::FutOTPTeamSummaryReport* teamSummaryReport = static_cast<FutOTPReportBase::FutOTPTeamSummaryReport*>(osdkReport.getTeamReports());
	FutOTPReportBase::FutOTPTeamReportMap& teamReportMap = teamSummaryReport->getFutOTPTeamReportMap();

	// player report
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	// loop team report
	FutOTPReportBase::FutOTPTeamReportMap::const_iterator squadIter, squadIterFirst, squadIterEnd;
	squadIter = squadIterFirst = teamReportMap.begin();
	squadIterEnd = teamReportMap.end();
	
	for (; squadIter != squadIterEnd; ++squadIter)
	{
		// get unadjusted score from team
		int64_t teamEntityId = squadIter->first;
		FutOTPReportBase::FutOTPTeamReport& squadReportDetails = *squadIter->second;		
		uint16_t unadjustedScore = squadReportDetails.getCommonTeamReport().getUnadjustedScore();
		TRACE_LOG("[FutOtpReportSlave:" << this << "].updatePlayerUnadjustedScore(), teamEntityId:" << teamEntityId << ", unadjustedScore:" << unadjustedScore);

		// loop player report
		for (OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::value_type playerIter : osdkPlayerReportsMap)
		{
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *(playerIter.second);
			FutOTPReportBase::FutOTPPlayerReport& otpPlayerReport = static_cast<FutOTPReportBase::FutOTPPlayerReport&>(*playerReport.getCustomPlayerReport());

			// set unadjusted score to player
			if (otpPlayerReport.getTeamEntityId() == teamEntityId)
			{
				otpPlayerReport.getCommonPlayerReport().setUnadjustedScore(static_cast<uint16_t>(unadjustedScore));				
			}
		}
	}
}

}   // namespace GameReporting

}   // namespace Blaze

