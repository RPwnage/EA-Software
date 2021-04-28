/*************************************************************************************************/
/*!
    \file   futh2hcupsreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "futh2hreportslave.h"

#include "fifa_hmac/fifa_hmac.h"
#include "fifa/tdf/fifah2hreport.h"

#include "framework/controller/controller.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
const int NUM_FUT_PLAYERS = 2;

const char* STR_GAMEENDREASON = "GAMEENDREASON";
const char* STR_FUTMATCHFLAGS = "futMatchFlags";

/*************************************************************************************************/
/*! \brief toString
	Convert FUT::MatchResult to a string.

	\return - The string representing the FUT::MatchResult
*/
/*************************************************************************************************/
const char* toString(FUT::MatchResult reason)
{
	const char* reasonStr = "NO_CONTEST";
	switch (reason)
	{
	case FUT::WIN:
		reasonStr = "WIN";
		break;
	case FUT::DRAW:
		reasonStr = "DRAW";
		break;
	case FUT::LOSS:
		reasonStr = "LOSS";
		break;
	case FUT::NO_CONTEST:
		reasonStr = "NO_CONTEST";
		break;
	case FUT::DNF:
		reasonStr = "DNF";
		break;
	case FUT::DNF_WIN:
		reasonStr = "DNF_WIN";
		break;
	case FUT::DNF_DRAW:
		reasonStr = "DNF_DRAW";
		break;
	case FUT::DNF_LOSS:
		reasonStr = "DNF_LOSS";
		break;
	case FUT::DNF_OG:
		reasonStr = "DNF_OG";
		break;
	case FUT::DNF_AFK:
		reasonStr = "DNF_AFK";
		break;
	case FUT::QUIT:
		reasonStr = "QUIT";
		break;
	default:
		WARN_LOG("[FutH2HReportSlave].toString(): Unknown FUT::MatchResult:" << reason);
		reasonStr = "NO_CONTEST";
	break;
	}

	return reasonStr;
}

/*************************************************************************************************/
/*! \brief convertToDnf
	Convert FUT::MatchResult WIN/DRAW/LOSS to the DNF version.

	\return - DNF_WIN/DNF_DRAW/DNF_LOSS
*/
/*************************************************************************************************/
FUT::MatchResult convertToDnf(FUT::MatchResult& reason)
{
	switch (reason)
	{
	case FUT::WIN:
		reason = FUT::DNF_WIN;
		break;
	case FUT::DRAW:
		reason = FUT::DNF_DRAW;
		break;
	case FUT::LOSS:
		reason = FUT::DNF_LOSS;
		break;
	default:
		WARN_LOG("[FutH2HReportSlave].convertToDnf(): Unknown FUT::MatchResult:" << reason);
		// Do not change the result otherwise.
		break;
	}

	return reason;
}


/*************************************************************************************************/
/*! \brief checkColumnReset
	Resets the value of the column if the version is mismatched with the config file.

	\return - The new value after the reset, if necessary
*/
/*************************************************************************************************/
int64_t checkColumnReset(int64_t& colValue, const char8_t* colName, const FUT::StatResetList& statResetList, int64_t version, int64_t globalStatsVersion, const FUT::CollatorConfig* collatorConfig)
{
	int64_t defaultValue = 0;

	if (EA_UNLIKELY(version < globalStatsVersion) && EA_LIKELY(NULL != collatorConfig))
	{
		const FUT::ColDefaultMap& defaultMap = collatorConfig->getStatDefault();
		FUT::ColDefaultMap::const_iterator it = defaultMap.find(colName);
		if (it != defaultMap.end())
		{
			defaultValue = it->second;
		}

		for (int64_t index = version; index < globalStatsVersion; ++index)
		{
			FUT::ColumnList& columnList = *statResetList[index];
			FUT::ColumnList::iterator iterator = eastl::find(columnList.begin(), columnList.end(), colName);
			if (iterator != columnList.end())
			{
				colValue = defaultValue;
			}
		}
	}
	
	return colValue;
}

/*************************************************************************************************/
/*! \brief ClampToRange
	Create the final value to be written to the DB so the value remains in range

	\return - The new value after the reset, if necessary
*/
/*************************************************************************************************/
int64_t ClampToRange(int64_t curValue, int64_t adjustement, const FUT::Range& range)
{
	const int64_t lower = range.getLower();
	const int64_t upper = range.getUpper();

	int64_t adjValue = curValue + adjustement;
	
	// Clamp the number to the range
	if (adjValue < lower)
	{
		adjValue = lower;
	}
	else if (adjValue > upper)
	{
		adjValue = upper;
	}

	return adjValue;
}

/*************************************************************************************************/
/*! \brief GetIndividualPlayerReportPtr
	Get the player report the enhanced game reporting

\return - Return the pointer to the report, it may contain NULL
*/
/*************************************************************************************************/
FUT::IndividualPlayerReportPtr GetIndividualPlayerReportPtr(FUT::CollatorReport* enhancedReport, Blaze::GameManager::PlayerId playerId)
{
	FUT::IndividualPlayerReportPtr futPlayerReportPtr;
	
	if (NULL != enhancedReport)
	{
		FUT::PlayerReportMap& playerReportMap = enhancedReport->getPlayerReportMap();
		FUT::PlayerReportMap::iterator playerReportIt = playerReportMap.find(playerId);
		if (playerReportIt != playerReportMap.end())
		{
			futPlayerReportPtr = playerReportIt->second;
		}
		else
		{
			WARN_LOG("[FutH2HReportSlave].GetIndividualPlayerReportPtr(): Player(" << playerId << ") does not belong to the collator report.");
		}
	}

	return futPlayerReportPtr;
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Constructor
*/
/*************************************************************************************************/
FutH2HReportSlave::FutH2HReportSlave(GameReportingSlaveImpl& component) 
	: FifaH2HBaseReportSlave(component)
	, mReportGameTypeId(82)
	, mShouldLogLastOpponent(true)
	, mEnableValidation( false )
{}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Destructor
*/
/*************************************************************************************************/
FutH2HReportSlave::~FutH2HReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FutH2HReportSlave::create(GameReportingSlaveImpl& component)
{
	FifaHMAC::Create(component);

    return BLAZE_NEW_NAMED("FutH2HReportSlave") FutH2HReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head game used in gamereporting.cfg

    \return - the H2H game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FutH2HReportSlave::getCustomH2HGameTypeName() const
{
    return "gameType82";
}

/*************************************************************************************************/
/*! \brief getCustomH2HStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for H2H game
*/
/*************************************************************************************************/
const char8_t* FutH2HReportSlave::getCustomH2HStatsCategoryName() const
{
    return "NormalGameStats";
}

const uint16_t FutH2HReportSlave::getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const
{
	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
	return h2hPlayerReport.getH2HCustomPlayerData().getControls();
}

/*************************************************************************************************/
/*! \brief GetCollatorConfig
    Return the configuration for FUT match reporting

    \return - Return the configuration for FUT match reporting
*/
/*************************************************************************************************/
const FUT::CollatorConfig* FutH2HReportSlave::GetCollatorConfig()
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

/*************************************************************************************************/
/*! \brief updateCustomPlayerStats
    Update custom stats that are regardless of the game result
*/
/*************************************************************************************************/
void FutH2HReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
//	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());

}

/*************************************************************************************************/
/*! \brief updateCustomH2HSeasonalPlayerStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void FutH2HReportSlave::updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateCustomPlayerStats(/*playerId*/0, playerReport);
}

/*************************************************************************************************/
/*! \brief updateCustomGameStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void FutH2HReportSlave::updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport)
{
	HeadToHeadLastOpponent::ExtensionData data;
	data.mProcessedReport = mProcessedReport;

	mH2HLastOpponentExtension.setExtensionData( &data );
	mFifaLastOpponent.setExtension( &mH2HLastOpponentExtension );
	mFifaLastOpponent.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
}

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FutH2HReportSlave::updateCustomNotificationReport(const char8_t* statsCategory)
{
    // Obtain the custom data for report notification
    OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	Fifa::H2HNotificationCustomGameData *gameCustomData = BLAZE_NEW Fifa::H2HNotificationCustomGameData();
    Fifa::H2HNotificationCustomGameData::PlayerCustomDataMap &playerCustomDataMap = gameCustomData->getPlayerCustomDataReports();

    // Obtain the player report map
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;

        // Obtain the Skill Points
        Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(statsCategory, playerId);
        if (NULL != key)
        {
            int64_t overallSkillPoints = mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, Stats::STAT_PERIOD_ALL_TIME, false);

            // Update the report notification custom data with the overall SkillPoints
			Fifa::H2HNotificationPlayerCustomData* playerCustomData = BLAZE_NEW Fifa::H2HNotificationPlayerCustomData();
            playerCustomData->setSkillPoints(static_cast<uint32_t>(overallSkillPoints));

            // Insert the player custom data into the player custom data map
            playerCustomDataMap.insert(eastl::make_pair(playerId, playerCustomData));
        }
    }
	
	gameCustomData->setGameReportingId(mProcessedReport->getGameReport().getGameReportingId());

	// HMAC the client match stats for FOS/LiON to verify.
	char8_t homeHash[Fifa::HASH_MAX_LENGTH + 1] = ""; // size needs to match Blaze::HashUtil::SHA512_HEXSTRING_OUT
	char8_t awayHash[Fifa::HASH_MAX_LENGTH + 1] = "";
	
	FifaHMAC* hmac = FifaHMAC::Instance();
	if(hmac != NULL)
	{
		hmac->GenerateHMACFromGameReport(mProcessedReport->getGameReport(), homeHash, awayHash);
	}

	gameCustomData->setHomeMatchHash(homeHash);
	gameCustomData->setAwayMatchHash(awayHash);

	// Include additional data for debugging
	if (OsdkReport.getEnhancedReport() != NULL)
	{
		if (mEnableValidation == true)
		{
			char specialBuf[Fifa::H2HNotificationCustomGameData::MAX_SPECIAL_LEN];
			OsdkReport.getEnhancedReport()->print(specialBuf, sizeof(specialBuf));
			gameCustomData->setSpecial(specialBuf);
		}
	}

    // Set the gameCustomData to the OsdkReportNotification
    OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

/*************************************************************************************************/
/*! \brief initMatchReportingStatsSelect
	Initialize the stats request with fields required for FUT match reporting analysis.
*/
/*************************************************************************************************/
void FutH2HReportSlave::initMatchReportingStatsSelect()
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

/*************************************************************************************************/
/*! \brief selectCustomStats
    Select stats for processing.
*/
/*************************************************************************************************/
void FutH2HReportSlave::selectCustomStats()
{
	if (mShouldLogLastOpponent)
	{
		mFifaLastOpponent.selectLastOpponentStats();
	}
}

/*************************************************************************************************/
/*! \brief updateCustomTransformStats
	Update the stats from selected data.

	Called when (DB_ERR_OK == mUpdateStatsHelper.fetchStats())
*/
/*************************************************************************************************/
void FutH2HReportSlave::updateCustomTransformStats(const char8_t* statsCategory)
{
	if (mShouldLogLastOpponent)
	{
		mFifaLastOpponent.transformLastOpponentStats();
	}

	updateMatchReportingStats();
}

/*************************************************************************************************/
/*! \brief updateMatchReportingStats
	Update the match reporting stats and reset the values according to gameCustomConfig::statResetList 
	in gamereporting.cfg.

	This functions depends on (DB_ERR_OK == mUpdateStatsHelper.fetchStats()).
*/
/*************************************************************************************************/
void FutH2HReportSlave::updateMatchReportingStats()
{
	// Update the match reporting stats based on collation results.
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
	FUT::CollatorReport* enhancedReport = static_cast<FUT::CollatorReport*>(OsdkReport.getEnhancedReport());

	const FUT::CollatorConfig* collatorConfig = GetCollatorConfig();

	if (NULL != enhancedReport && NULL != collatorConfig)
	{
		const FUT::Range& stdRange = collatorConfig->getStandardRatingRange();
		const FUT::Range& cmpRange = collatorConfig->getCompetitiveRatingRange();

		Stats::ScopeNameValueMap playerKeyScopeMap;
		playerKeyScopeMap[FUT::MATCHREPORTING_gameType] = mReportGameTypeId;

		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
		playerIter = OsdkPlayerReportsMap.begin();
		playerEnd = OsdkPlayerReportsMap.end();
		for (; playerIter != playerEnd; ++playerIter)
		{
			//OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = playerIter->second;
			Blaze::GameManager::PlayerId playerId = playerIter->first;
			FUT::IndividualPlayerReportPtr futPlayerReportPtr = GetIndividualPlayerReportPtr(enhancedReport, playerId);

			const Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(FUT::MATCHREPORTING_FUTMatchReportStats, playerId, &playerKeyScopeMap);
			if(NULL != key)
			{				
				// The number of elements in the configuration is the version
				const FUT::StatResetList& statResetList = collatorConfig->getStatResetList();
				int64_t globalStatsVersion = statResetList.size();

				for (int32_t periodTypeIt = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodTypeIt < Stats::STAT_NUM_PERIODS; ++periodTypeIt)
				{
					Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(periodTypeIt);
					const char* colName = NULL;

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_version;
					int64_t version = mUpdateStatsHelper.getValueInt(key, FUT::MATCHREPORTING_version, periodType, true);

					// Fix test version edits
					if (version > globalStatsVersion)
					{
						WARN_LOG("[FutH2HReportSlave::" << this << "].updateCustomTransformStats(): Version inversion. new:" << globalStatsVersion << " old:" << version);
						version = globalStatsVersion;
					}

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_total;
					int64_t total = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);

					checkColumnReset(total, colName, statResetList, version, globalStatsVersion, collatorConfig);

					total += 1;  // Increment total matches

					mUpdateStatsHelper.setValueInt(key, colName, periodType, total);

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_mismatch;
					int64_t mismatch = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);

					checkColumnReset(mismatch, colName, statResetList, version, globalStatsVersion, collatorConfig);

					if (enhancedReport->getCollision() == true)
					{
						++mismatch;

						if (Stats::STAT_PERIOD_ALL_TIME == periodType)
						{
							Blaze::GameReporting::UpdateMetricRequest metricRequest;
							metricRequest.setMetricName("FUT_mismatch");
							metricRequest.setValue(mReportGameTypeId);

							getComponent().processUpdateMetric(metricRequest, NULL);
						}
					}

					mUpdateStatsHelper.setValueInt(key, colName, periodType, mismatch);

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_singleReport;
					int64_t singleReport = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);

					checkColumnReset(singleReport, colName, statResetList, version, globalStatsVersion, collatorConfig);

					if (enhancedReport->getReportCount() < 2)
					{
						++singleReport;
					}

					mUpdateStatsHelper.setValueInt(key, colName, periodType, singleReport);

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_dnfMatchResult;
					int64_t dnfMatchResult = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);

					checkColumnReset(dnfMatchResult, colName, statResetList, version, globalStatsVersion, collatorConfig);

					if (NULL != futPlayerReportPtr)
					{
						switch (futPlayerReportPtr->getMatchResult())
						{
						case FUT::DNF_WIN:
						case FUT::DNF_DRAW:
						case FUT::DNF_LOSS:
							++dnfMatchResult;
							break;

						default:
							// Don't count it
							break;
						}
					}
					
					mUpdateStatsHelper.setValueInt(key, colName, periodType, dnfMatchResult);

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_critMissMatch;
					int64_t critMissMatch = mUpdateStatsHelper.getValueInt(key, colName, periodType, true);

					checkColumnReset(critMissMatch, colName, statResetList, version, globalStatsVersion, collatorConfig);

					if (enhancedReport->getCritMissMatch() == true)
					{
						++critMissMatch;
					}

					mUpdateStatsHelper.setValueInt(key, colName, periodType, critMissMatch);

					//----------------------------------------------------------------------------------
					// Update the version after all the stats have been updated
					mUpdateStatsHelper.setValueInt(key, FUT::MATCHREPORTING_version, periodType, globalStatsVersion);

				} // for periodTypeIt in FUTMatchReportStats
			}

			key = mBuilder.getUpdateRowKey(FUT::MATCHREPORTING_FUTReputationStats, playerId, NULL);
			
			if (NULL != key)
			{
				const FUT::StatResetList& statResetList = collatorConfig->getRatingResetList();
				int64_t globalStatsVersion = statResetList.size();

				for (int32_t periodTypeIt = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); NULL != key && periodTypeIt < Stats::STAT_NUM_PERIODS; ++periodTypeIt)
				{
					Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(periodTypeIt);
					const char* colName = NULL;

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_version;
					int64_t version = mUpdateStatsHelper.getValueInt(key, FUT::MATCHREPORTING_version, periodType, true);

					// Fix test version edits
					if (version > globalStatsVersion)
					{
						WARN_LOG("[FutH2HReportSlave::" << this << "].updateCustomTransformStats(): Version inversion. new:" << globalStatsVersion << " old:" << version);
						version = globalStatsVersion;
					}

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_stdRating;
					int64_t stdRating = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, colName, periodType, true));

					checkColumnReset(stdRating, colName, statResetList, version, globalStatsVersion, collatorConfig);

					if( NULL != futPlayerReportPtr)
					{
						stdRating = ClampToRange(stdRating, futPlayerReportPtr->getStandardRating(), stdRange);
					}

					mUpdateStatsHelper.setValueInt(key, colName, periodType, stdRating);

					//----------------------------------------------------------------------------------
					colName = FUT::MATCHREPORTING_cmpRating;
					int64_t cmpRating = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, colName, periodType, true));

					checkColumnReset(cmpRating, colName, statResetList, version, globalStatsVersion, collatorConfig);

					if (NULL != futPlayerReportPtr)
					{
						cmpRating = ClampToRange(cmpRating, futPlayerReportPtr->getCompetitiveRating(), cmpRange);
					}

					mUpdateStatsHelper.setValueInt(key, colName, periodType, cmpRating);

					//----------------------------------------------------------------------------------
					// Update the version after all the stats have been updated
					mUpdateStatsHelper.setValueInt(key, FUT::MATCHREPORTING_version, periodType, globalStatsVersion);

				} // for periodTypeIt in FUTReputationStats
			}
		} // for playerIter
	} // if enhancedReport is valid
}

/*************************************************************************************************/
/*! \brief commitMatchReportingStats
	Commit match reporting stats to the db on early exits.
*/
/*************************************************************************************************/
BlazeRpcError FutH2HReportSlave::commitMatchReportingStats()
{
	bool strict = getComponent().getConfig().getBasicConfig().getStrictStatsUpdates();
	BlazeRpcError processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

	if (Blaze::ERR_OK == processErr)
	{
		// Force the fetch stats here since it isn't done in the transformStats
		if( DB_ERR_OK == mUpdateStatsHelper.fetchStats())
		{
			updateMatchReportingStats();

			TRACE_LOG("[FutH2HReportSlave:" << this << "].commitMatchReportingStats() - commit stats");
			processErr = mUpdateStatsHelper.commitStats();
		}
	}

	return processErr;
}

/*************************************************************************************************/
/*! \brief updateGameHistory
	Prepare the game history data for commit.
*/
/*************************************************************************************************/
void FutH2HReportSlave::updateGameHistory(ProcessedGameReport& processedReport, GameReport& report)
{	
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*processedReport.getGameReport().getReport());
	FUT::CollatorReport* collatorReport = static_cast<FUT::CollatorReport*>(osdkReport.getEnhancedReport());

	if (NULL != collatorReport)
	{
		FUT::PlayerReportMap& playerReportMap = collatorReport->getPlayerReportMap();
		for (FUT::PlayerReportMap::iterator playerReportIt = playerReportMap.begin(); playerReportIt != playerReportMap.end(); ++playerReportIt)
		{
			// Fix invalid entries in the collator report.
			// This was causing copies to fail in subsequent processing.
			if (playerReportIt->second == NULL)
			{
				WARN_LOG("[FutH2HReportSlave:" << this << "].updateGameHistory(): Player(" << playerReportIt->first << ") does not belong to the collator report for game(" << processedReport.getGameId() << ").");
				playerReportIt = playerReportMap.erase(playerReportIt);
			}
		}
	}

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

/*************************************************************************************************/
/*! \brief setCustomEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FutH2HReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
    HttpParam param;
    param.name = "gameName";
    param.value = "BlazeGame";
    param.encodeValue = true;

    mailParamList->push_back(param);
	    
	blaze_snzprintf(mailTemplateName, MAX_MAIL_TEMPLATE_NAME_LEN, "%s", "sendgamecomplete");

    return true;
}
*/
/*************************************************************************************************/
/*! \brief skipProcess
    Indicate if the rest of report processing should continue
	FUT override so we can proccess unranked games.

    \return - true if the rest of report processing is not needed

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
bool FutH2HReportSlave::skipProcess()
{   
	// Bring back the logic to count the match only when match time MaxTime for deducing HMAC/UTAS to Blaze
	// load.
	if (false == mIsOfflineReport)
	{
		// stage 1 disconnect
		// no stats will be tracked
		if (mGameTime < 1)
		{
			TRACE_LOG("[GameCommonReportSlave:" << this << "].skipProcess(): End Phase 1 - Nothing counts");
			mProcessedReport->enableHistorySave(false);

			// NOTE We are not recording any failure in this case because UTAS does not read from the game history table
			// If it is necessary, enable the history save.

			return true;
		}
		// stage 2 disconnect
		// all stats will be tracked
		else
		{
			TRACE_LOG("[GameCommonReportSlave:" << this << "].skipProcess(): End Phase 2 -  Everything counts");
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
bool FutH2HReportSlave::skipProcessConclusionType()
{
	// We current do not have a reason to skip the game history for exceptional cases (quit, disc, desync, etc.)
	return false;
}

/*************************************************************************************************/
/*! \brief initGameParams

	\Reference: GameH2HReportSlave::initGameParams()
*/
/*************************************************************************************************/
void FutH2HReportSlave::initGameParams()
{
	// Copied from void GameH2HReportSlave::initGameParams()
	// could be a tie game if there is more than one player
	mTieGame = (mPlayerReportMapSize > 1);

	const char8_t* serviceEnv = gController->getServiceEnvironment();
	mEnableValidation = (blaze_stricmp(serviceEnv, "test") != 0 || blaze_stricmp(serviceEnv, "dev"));
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
	\Reference: GameCommonReportSlave::updatePlayerKeyscopes
*/
/*************************************************************************************************/
void FutH2HReportSlave::updatePlayerKeyscopes()
{
	// Do nothing
}

/*************************************************************************************************/
/*! \brief IsFUTMode
	Parse the game type name to determine whether the current mode is FUT and set the member to determine whether the opponent 
	should be logged for match making.

	\return - true if the gametype is FUT
*/
/*************************************************************************************************/
bool FutH2HReportSlave::isFUTMode(const char8_t* gameTypeName)
{
	mReportGameTypeId = FUT::GetGameTypeId(gameTypeName);

	mShouldLogLastOpponent = true;

	switch(mReportGameTypeId)
	{
	case 88:	//FUT_ONLINE_FRIENDLIES
	case 75:	//FUT_ONLINE_COMPETITIVE
	case 72:	//FUT_ONLINE_FRIENDLY_HOUSERULES
		mShouldLogLastOpponent = false;
	case 80:	//FUT_ONLINE_DRAFT
	case 81:	//FUT_ONLINE_H2H
	case 82:	//FUT_ONLINE_TOUR
	case 79:	//FUT_ONLINE_CHAMPIONS
	case 76:	//FUT_ONLINE_RIVALS
	case 73:	//FUT_ONLINE_HOUSERULES
		return true;
	}

	return false;
}

/*************************************************************************************************/
/*! \brief GenerateOpponentReason

	Take the reason reported by the user to generate the reason for the opponent.

	\return - GameEndedReason for the opponent
*/
/*************************************************************************************************/
Fifa::GameEndedReason GenerateOpponentReason(Fifa::GameEndedReason reason)
{
	Fifa::GameEndedReason oppoReason = Fifa::GAMEENDED_LOCALDISCONNECT;
	
	switch (reason)
	{
	case Fifa::GAMEENDED_COMPLETE:				oppoReason = Fifa::GAMEENDED_COMPLETE;
	case Fifa::GAMEENDED_REMOTEQUIT:			oppoReason = Fifa::GAMEENDED_LOCALQUIT;
	case Fifa::GAMEENDED_LOCALQUIT:				oppoReason = Fifa::GAMEENDED_REMOTEQUIT;
	case Fifa::GAMEENDED_MUTUALQUIT:			oppoReason = Fifa::GAMEENDED_MUTUALQUIT;
	case Fifa::GAMEENDED_REMOTEDISCONNECT:		oppoReason = Fifa::GAMEENDED_LOCALDISCONNECT;	// TODO: Review with designers
	case Fifa::GAMEENDED_LOCALDISCONNECT:		oppoReason = Fifa::GAMEENDED_REMOTEDISCONNECT;
	case Fifa::GAMEENDED_DESYNC:				oppoReason = Fifa::GAMEENDED_DESYNC;
	case Fifa::GAMEENDED_OPPONENT_NOTENOUGHPLAYERS:	oppoReason = Fifa::GAMEENDED_NOTENOUGHPLAYERS;
	case Fifa::GAMEENDED_NOTENOUGHPLAYERS:		oppoReason = Fifa::GAMEENDED_OPPONENT_NOTENOUGHPLAYERS;
	case Fifa::GAMEENDED_SQUADMISMATCH:			oppoReason = Fifa::GAMEENDED_SQUADMISMATCH;
	case Fifa::GAMEENDED_MISMATCHCHANGELIST:	oppoReason = Fifa::GAMEENDED_MISMATCHCHANGELIST;	// In FIFA 19, it is debug only.
	case Fifa::GAMEENDED_OVRATTR_MISMATCH:		oppoReason = Fifa::GAMEENDED_OVRATTR_MISMATCH;	// The local user has identified the opponent squad as incorrect. 
	case Fifa::GAMEENDED_LOCALIDLE:				oppoReason = Fifa::GAMEENDED_REMOTEQUIT;
	case Fifa::GAMEENDED_LOCALIDLE_H2H:			oppoReason = Fifa::GAMEENDED_REMOTEQUIT;
	case Fifa::GAMEENDED_ALTTAB:				oppoReason = Fifa::GAMEENDED_REMOTEQUIT;
	case Fifa::GAMEENDED_OWNGOALS:				oppoReason = Fifa::GAMEENDED_REMOTEQUIT;
	case Fifa::GAMEENDED_OWNGOALS_H2H:			oppoReason = Fifa::GAMEENDED_REMOTEQUIT;
	case Fifa::GAMEENDED_CONSTRAINED:			oppoReason = Fifa::GAMEENDED_REMOTEQUIT;
	default:									oppoReason = Fifa::GAMEENDED_LOCALDISCONNECT;
	}

	return oppoReason;
}

/*************************************************************************************************/
/*! \brief determineFUTGameResult

	Generate the match results for UTAS.

*/
/*************************************************************************************************/
void FutH2HReportSlave::determineFUTGameResult()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	mMatchResultMap.clear();
	GameManager::PlayerId homePlayerId = 0;
	GameManager::PlayerId awayPlayerId = 0;

	for (OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::value_type playerIter : OsdkPlayerReportsMap)
	{
		GameManager::PlayerId playerId = playerIter.first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = *(playerIter.second);

		FUT::MatchResult matchResult = FUT::NO_CONTEST;

		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
		Fifa::CommonPlayerReport& fifaCommonPlayerReport = h2hPlayerReport.getCommonPlayerReport();

		if (mUnadjustedHighScore == mUnadjustedLowScore)
		{
			matchResult = FUT::DRAW;
		}
		else if( fifaCommonPlayerReport.getUnadjustedScore() == mUnadjustedHighScore )
		{
			matchResult = FUT::WIN;
		}
		else
		{
			EA_ASSERT(fifaCommonPlayerReport.getUnadjustedScore() == mUnadjustedLowScore );
			matchResult = FUT::LOSS;
		}

		mMatchResultMap[playerId] = matchResult;

		if (playerReport.getHome() == true)
		{
			homePlayerId = playerId;
		}
		else
		{
			awayPlayerId = playerId;
		}
	}

	updateFUTMatchResult(homePlayerId, awayPlayerId, OsdkReport, mMatchResultMap);
}

/*************************************************************************************************/
/*! \brief updatePlayerFUTMatchResult

	Generate the FUT match result for the player.

*/
/*************************************************************************************************/
void FutH2HReportSlave::updatePlayerFUTMatchResult(GameManager::PlayerId playerId, GameManager::PlayerId opponentId, OSDKGameReportBase::OSDKReport& osdkReport, FUTMatchResultMap& matchResultMap, FUT::CollatorReport& collatorReport)
{
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
			WARN_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): (" << playerId << ") was not found in GameInfo::PlayerInfoMap.");
			
			// Remove the following for FIFA 19 final QSPR.  This was added to investigate missing data in the Load Test Script.
			//INFO_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): osdkReport: " << osdkReport);

			collatorReport.setInvalidPlayerMap(true);

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
	}
	else
	{
		WARN_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): (" << playerId << ") was not found in OSDKPlayerReportsMap.");
		INFO_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): osdkReport: " << osdkReport);
	}

	if (reasonFound == false)
	{
		// Find the opponent report
		OSDKReport::OSDKPlayerReportsMap::const_iterator oppoReportIt = playerReportMap.find(opponentId);
		if (oppoReportIt != playerReportMap.end())
		{
			OSDKPlayerReport& oppoReport = *(oppoReportIt->second);
			IntegerAttributeMap& oppoAttributeMap = oppoReport.getPrivatePlayerReport().getPrivateIntAttributeMap();
			IntegerAttributeMap::iterator oppoGameEndReasonIt = oppoAttributeMap.find(STR_GAMEENDREASON);
			if (oppoGameEndReasonIt != oppoAttributeMap.end())
			{
				reason = GenerateOpponentReason(static_cast<Fifa::GameEndedReason>(oppoGameEndReasonIt->second));
			}
			else
			{
				WARN_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): Player and opponent end reason not found in private attribute map.");
				
				reason = Fifa::GAMEENDED_LOCALDISCONNECT;
				collatorReport.setInvalidPlayerMap(true);
			}
		}
		else
		{
			WARN_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): Opp (" << opponentId << ") was not found in OSDKPlayerReportsMap.");
			INFO_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): osdkReport: " << osdkReport);
			
			reason = Fifa::GAMEENDED_LOCALDISCONNECT;
			collatorReport.setInvalidPlayerMap(true);
		}
	}
	
	FUT::MatchResult matchResult = FUT::DNF;

	switch (reason) 
	{
	
	// Game Completed Normally
	case Fifa::GAMEENDED_COMPLETE:
	{
		matchResult = matchResultMap[playerId];
	}
	break;

	// User Quit
	case Fifa::GAMEENDED_LOCALQUIT:
	case Fifa::GAMEENDED_NOTENOUGHPLAYERS:
	{
		matchResult = FUT::QUIT;
	}
	break;

	// Opponent Left Mid Match
	case Fifa::GAMEENDED_REMOTEDISCONNECT:
	case Fifa::GAMEENDED_REMOTEQUIT:
	case Fifa::GAMEENDED_OPPONENT_NOTENOUGHPLAYERS:
	{
		matchResult = convertToDnf(matchResultMap[playerId]);
	}
	break;

	// Local Disconnect
	case Fifa::GAMEENDED_LOCALDISCONNECT:
	{
		matchResult = convertToDnf(matchResultMap[playerId]);
	}
	break;

	// Something went wrong, we don't know who's fault it is
	case Fifa::GAMEENDED_DESYNC:
	{
		matchResult = convertToDnf(matchResultMap[playerId]);
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
		WARN_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTOutcome(): Unknown Outcome Reported.");
		
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

	FUT::IndividualPlayerReportPtr& futPlayerReportPtr = collatorReport.getPlayerReportMap()[playerId];
	if( futPlayerReportPtr == NULL )
	{
		WARN_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): Adding missing player id:" << playerId);
		INFO_LOG("[FutH2HReportSlave::" << this << "].updatePlayerFUTMatchResult(): osdkReport: " << osdkReport);

		FUT::IndividualPlayerReport playerInfo;
		playerInfo.setId(playerId);
		playerInfo.setIsRecovered(true);

		futPlayerReportPtr = playerInfo.clone();
	}
	
	futPlayerReportPtr->setMatchResult( matchResult );

	updateFUTUserRating(reason, *futPlayerReportPtr, collatorConfig);
}

/*************************************************************************************************/
/*! \brief updateFUTUserRating

	Generate the FUT match report rating for the user.

*/
/*************************************************************************************************/
void FutH2HReportSlave::updateFUTUserRating(Fifa::GameEndedReason reason, FUT::IndividualPlayerReport& futPlayerReport, const FUT::CollatorConfig* collatorConfig)
{
	bool isCompMode = false;

	if (EA_LIKELY(NULL != collatorConfig))
	{
		for (int32_t compModeId : collatorConfig->getCompetitiveModes())
		{
			if (compModeId == mReportGameTypeId)
			{
				isCompMode = true;
			}
		}

		const char8_t * endedReasonString = Fifa::GameEndedReasonToString(reason);

		if (isCompMode)
		{
			const FUT::EventAdjustmentMap& adjustmentMap = collatorConfig->getCompetitiveEventAdjustment();
			FUT::EventAdjustmentMap::const_iterator adjustmentIt = adjustmentMap.find(endedReasonString);
			if (adjustmentIt != adjustmentMap.end())
			{
				futPlayerReport.setCompetitiveRating(static_cast<FUT::UserRating>(adjustmentIt->second));
			}
		}
		else
		{
			const FUT::EventAdjustmentMap& adjustmentMap = collatorConfig->getStandardEventAdjustment();
			FUT::EventAdjustmentMap::const_iterator adjustmentIt = adjustmentMap.find(endedReasonString);
			if (adjustmentIt != adjustmentMap.end())
			{
				futPlayerReport.setStandardRating(static_cast<FUT::UserRating>(adjustmentIt->second));
			}
		}
	}
}

/*************************************************************************************************/
/*! \brief updatePreMatchUserRating

	Generate the FUT match report rating for the user before the match start.

*/
/*************************************************************************************************/
void FutH2HReportSlave::updatePreMatchUserRating()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FUT::CollatorReport* collatorReport = static_cast<FUT::CollatorReport*>(osdkReport.getEnhancedReport());
	const FUT::CollatorConfig* collatorConfig = GetCollatorConfig();

	EA_ASSERT(NULL != collatorConfig);

	if (collatorReport == NULL)
	{
		INFO_LOG("[FutH2HReportSlave::" << this << "].updatePreMatchUserRating(): Skip due to enhanced report missing.");
		return;
	}

	for (OSDKReport::OSDKPlayerReportsMap::value_type playerPair : osdkReport.getPlayerReports())
	{
		OSDKPlayerReport& playerReport = *(playerPair.second);
		IntegerAttributeMap& playerAttributeMap = playerReport.getPrivatePlayerReport().getPrivateIntAttributeMap();
		IntegerAttributeMap::iterator playerGameEndReasonIt = playerAttributeMap.find(STR_GAMEENDREASON);
		if (playerGameEndReasonIt != playerAttributeMap.end())
		{
			Fifa::GameEndedReason reason = static_cast<Fifa::GameEndedReason>(playerGameEndReasonIt->second);

			FUT::IndividualPlayerReportPtr futPlayerReportPtr = GetIndividualPlayerReportPtr(collatorReport, playerPair.first);
			if (NULL != futPlayerReportPtr)
			{
				futPlayerReportPtr->setMatchResult(FUT::NO_CONTEST);
				updateFUTUserRating(reason, *futPlayerReportPtr, collatorConfig);
			}
		}
	}
}

/*************************************************************************************************/
/*! \brief updateFUTMatchResult

	Flip the variables so the match result is updated for the appropriate player

*/
/*************************************************************************************************/
void FutH2HReportSlave::updateFUTMatchResult(GameManager::PlayerId homePlayerId, GameManager::PlayerId awayPlayerId, OSDKGameReportBase::OSDKReport& osdkReport, FUTMatchResultMap& matchResultMap)
{
	FUT::CollatorReport* collatorReport = static_cast<FUT::CollatorReport*>(osdkReport.getEnhancedReport());

	// Only process the FUT outcome if we are using the FUT Collator
	if (collatorReport != NULL)
	{
		TRACE_LOG("[FutH2HReportSlave:" << this << "].updateFUTOutcome()");

		// Update the home player
		updatePlayerFUTMatchResult(homePlayerId, awayPlayerId, osdkReport, matchResultMap, *collatorReport);

		// Update the away player
		updatePlayerFUTMatchResult(awayPlayerId, homePlayerId, osdkReport, matchResultMap, *collatorReport);
	}
}

void FutH2HReportSlave::updatePlayerStats()
{
	TRACE_LOG("[FutH2HReportSlave:" << this << "].updatePlayerStats()");

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
			playerReport.setWins(0);
			playerReport.setLosses(0);
			playerReport.setTies(1);
		}
		else if (mWinnerSet.find(playerId) != mWinnerSet.end())
		{
			playerReport.setWins(1);
			playerReport.setLosses(0);
			playerReport.setTies(0);
		}
		else if (mLoserSet.find(playerId) != mLoserSet.end())
		{
			playerReport.setWins(0);
			playerReport.setLosses(1);
			playerReport.setTies(0);
		}
	}
}

/*************************************************************************************************/
/*! \brief IsValidReport

	When the fut collator is used, we need to know if either user reported a complete report.  
	If neither do, then skip over the general processing.
*/
/*************************************************************************************************/
bool FutH2HReportSlave::isValidReport()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FUT::CollatorReport* enhancedReport = static_cast<FUT::CollatorReport*>(OsdkReport.getEnhancedReport());

	if (NULL != enhancedReport)
	{
		return enhancedReport->getValidReport();
	}

	// This is to mimic original behavior.  Ignore this check.
	return true;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError FutH2HReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
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
            WARN_LOG("[FutH2HReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

		// Add the FUT match reporting specific request to the mBuilder
		initMatchReportingStatsSelect();

        // common processing
        processCommon();

        // initialize game parameters
        initGameParams();

        if (true == skipProcess())
        {
			updatePreMatchUserRating();

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
			WARN_LOG("[FutH2HReportSlave::" << this << "].process(): Error parsing keyscopes");

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
			WARN_LOG("[FutH2HReportSlave::" << this << "].process(): Error parsing stats");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

        bool strict = getComponent().getConfig().getBasicConfig().getStrictStatsUpdates();
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
			// Fetch the stats
			if (Blaze::ERR_OK == transformStats(getCustomH2HStatsCategoryName()))
			{
				statsUpdater.transformStats();
			}

            TRACE_LOG("[FutH2HReportSlave:" << this << "].process() - commit stats");
            processErr = mUpdateStatsHelper.commitStats();
        }

		updateGameHistory(processedReport, report);

        // post game reporting processing()
        processErr = (true == processUpdatedStats()) ? Blaze::ERR_OK : Blaze::ERR_SYSTEM;
    }

    return processErr;
}

}   // namespace GameReporting

}   // namespace Blaze

