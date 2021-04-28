/*************************************************************************************************/
/*!
\file   futgamereportcollator.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamereporting/fifa/futgamereportcollator.h"
#include "gamereporting/fifa/futgamereportcompare.h"

#include "gamereporting/fifa/fut_utility/fut_utility.h"

#include "util/collatorutil.h"
#include "gamereportcompare.h"

namespace Blaze
{
namespace GameReporting
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

uint16_t GetPlayerRedCard(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	uint16_t redCardCount = 0;

	Fifa::H2HPlayerReport* h2hPlayerReport = static_cast<Fifa::H2HPlayerReport*>(playerReport.getCustomPlayerReport());
	if (h2hPlayerReport != NULL)
	{
		redCardCount = h2hPlayerReport->getH2HCollationPlayerData().getRedCard();
	}
	
	return redCardCount;
}


FUTGameReportCollator::FUTGameReportCollator(ReportCollateData& gameReport, GameReportingSlaveImpl& component)
	: BasicGameReportCollator(gameReport, component)
{}

GameReportCollator* FUTGameReportCollator::create(ReportCollateData& gameReport, GameReportingSlaveImpl& component)
{
	return BLAZE_NEW_NAMED("FUTGameReportCollator") FUTGameReportCollator(gameReport, component);
}

FUTGameReportCollator::~FUTGameReportCollator()
{
}

GameReportCollator::ReportResult FUTGameReportCollator::reportSubmitted(BlazeId playerId, const GameReport& report, const EA::TDF::Tdf* privateReport, GameReportPlayerFinishedStatus finishedStatus, ReportType reportType)
{
	Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
	if (NULL != statsComponent)
	{
		Blaze::BlazeRpcError error;

		// Get the match reporting stats to determine the trusted report
		Stats::GetStatsRequest getStatsRequest;
		Stats::GetStatsResponse getStatsResponse;
		
		getStatsRequest.setPeriodType(Stats::STAT_PERIOD_ALL_TIME);
		getStatsRequest.getEntityIds().push_back(playerId);
		getStatsRequest.setCategory(FUT::MATCHREPORTING_FUTMatchReportStats);

		Stats::ScopeNameValueMap& playerKeyScopeMap = getStatsRequest.getKeyScopeNameValueMap();
		playerKeyScopeMap[FUT::MATCHREPORTING_gameType] = Stats::KEY_SCOPE_VALUE_AGGREGATE;

		error = statsComponent->getStats(getStatsRequest, getStatsResponse);

		if (Blaze::ERR_OK == error)
		{
			Stats::KeyScopeStatsValueMap& statsMap = getStatsResponse.getKeyScopeStatsValueMap();
			Stats::KeyScopeStatsValueMap::const_iterator it = statsMap.begin();
			Stats::KeyScopeStatsValueMap::const_iterator itEnd = statsMap.end();

			for (; it != itEnd; it++)
			{
				EA::TDF::tdf_ptr<Stats::StatValues> statValues = it->second;
				if (statValues != NULL)
				{
					const Stats::StatValues::EntityStatsList &entityList = statValues->getEntityStatsList();
					
					Stats::StatValues::EntityStatsList::const_iterator entityItr = entityList.begin();
					Stats::StatValues::EntityStatsList::const_iterator entityItrEnd = entityList.end();

					for (; (entityItr != entityItrEnd); ++entityItr)
					{
						if ((*entityItr) != NULL && (*entityItr)->getEntityId() == playerId)
						{
							const Stats::EntityStats::StringStatValueList &statList = (*entityItr)->getStatValues();

							mReportRating[playerId].mismatch = EA::StdC::AtoI64(statList[FUT::MATCHREPORTSTATS_IDX_MISMATCH].c_str());

							INFO_LOG("[FUTGameReportCollator:" << this << "].reportSubmitted() : User(" <<  playerId << ") with mismatch=" << mReportRating[playerId].mismatch << ".");
						}
					}
				}
			}
		}
	}

	return BasicGameReportCollator::reportSubmitted(playerId, report, privateReport, finishedStatus, reportType);
}

CollatedGameReport& FUTGameReportCollator::finalizeCollatedGameReport(ReportType /*collatedReportType*/)
{
	//  define a collator utility that merges maps - meaning that players in source that are not in the target are merged in.
	CollatedGameReport::DnfStatusMap& dnfStatus = mCollatedReport->getDnfStatus();
	Utilities::Collator collator;

	///////////////////////////////////////////////////////////////////////////
	//  CREATE COLLATED STAGE GAME REPORTS (midgame, finished and DNF)
	collator.setMergeMode(Utilities::Collator::MERGE_MAPS);

	//  generate collated mid-game report
	GameReport collatedFinalReport;
	bool baselineReport = false;

	if (!usesTrustedGameReporting())
	{
		//  update the dnf status map based on player info map
		GameInfo::PlayerInfoMap::const_iterator playerIt, playerEnd;
		playerIt = mFinishedGameInfo.getPlayerInfoMap().begin();
		playerEnd = mFinishedGameInfo.getPlayerInfoMap().end();
		for (; playerIt != playerEnd; ++playerIt)
		{
			GameManager::PlayerId playerId = playerIt->first;
			dnfStatus[playerId] = !playerIt->second->getFinished();
		}
	}

	// Setup the collator report
	FUT::CollatorReport collatorReport;
	FUT::PlayerReportMap& playerReportMap = collatorReport.getPlayerReportMap();
	for (auto playerIt : mFinishedGameInfo.getPlayerInfoMap())
	{
		FUT::IndividualPlayerReport playerInfo;
		playerInfo.setId(playerIt.first);

		playerReportMap.insert(
			FUT::PlayerReportMap::value_type(playerIt.first, 
				FUT::IndividualPlayerReportPtr(playerInfo.clone())
			));
	}

	//  go through all finished reports and merge them into a single post-game report.    
	Utilities::Collector::CategorizedGameReports midGameReports;
	mCollectorUtil.getGameReportsByCategory(midGameReports, Utilities::Collector::REPORT_TYPE_GAME_MIDGAME);
	if (!midGameReports.empty())
	{
		Utilities::Collector::CategorizedGameReports::const_iterator reportIt = midGameReports.begin();
		Utilities::Collector::CategorizedGameReports::const_iterator reportItEnd = midGameReports.end();
		for (; reportIt != reportItEnd; ++reportIt)
		{
			GameReport *report = reportIt->second;

			if (!baselineReport)
			{
				//  construct the baseline report.
				report->copyInto(collatedFinalReport);
				baselineReport = true;
			}
			else
			{
				//  compare baseline with this report - if different then report error.
				GameReportCompare compare;
				const GameType* gameType = getComponent().getGameTypeCollection().getGameType(report->getGameReportName());
				if (!compare.compare(*gameType, *report->getReport(), *collatedFinalReport.getReport()))
				{
					mCollatedReport->setError(Blaze::GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT);
					return *mCollatedReport;
				}
				else if (getComponent().getConfig().getBasicConfig().getMergeReports())
				{
					collator.merge(collatedFinalReport, *report);
				}
			}
		}

		//  clear collected trusted mid game reports after collation so that the
		//  processed mid game reports will not be used again in the next collation.
		if (usesTrustedGameReporting())
		{
			mCollectorUtil.clearGameReportsByCategory(Utilities::Collector::REPORT_TYPE_GAME_MIDGAME);
		}
	}

	// generate finished player report.
	GameReport collatedFinishedReport;
	bool baselineFinishedReport = false;

	//  go through all finished reports and merge them into a single post-game report.    
	Utilities::Collector::CategorizedGameReports finishedGameReports;
	mCollectorUtil.getGameReportsByCategory(finishedGameReports, Utilities::Collector::REPORT_TYPE_GAME_FINISHED);
	//BlazeRpcError totalCompareResult = Blaze::ERR_OK;
	uint32_t reportCount = 0;

	if (!finishedGameReports.empty())
	{
		// Find the most trusted report
		Utilities::Collector::CategorizedGameReports::const_iterator reportIt = finishedGameReports.begin();
		Utilities::Collector::CategorizedGameReports::const_iterator reportItEnd = finishedGameReports.end();

		// Use the first report by default
		Utilities::Collector::CategorizedGameReports::const_iterator bestReportIt = reportItEnd;
		int64_t trustRating = 0;

		for (; reportIt != reportItEnd; ++reportIt)
		{
			GameReport *report = reportIt->second;
			GameManager::PlayerId playerId = reportIt->first;
			bool isReportValid = true;

			FUT::PlayerReportMap::iterator iter = playerReportMap.find(reportIt->first);
			if (iter != playerReportMap.end())
			{
				iter->second->setReportReceived(true);
			}
			else
			{
				char8_t buf[4096];
				mFinishedGameInfo.print(buf, sizeof(buf));
				WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : did not find key (reportIt) " << reportIt->first << " in playerReportMap, mFinishedGameInfo.getPlayerInfoMap:" << buf);
				collatorReport.setInvalidPlayerMap(true);
				isReportValid = false;
			}
			
			OSDKGameReportBase::OSDKReport* osdkReport = static_cast<OSDKGameReportBase::OSDKReport*>(report->getReport());
			if (NULL == osdkReport || osdkReport->getPlayerReports().mapSize() != 2)
			{
				char8_t buf[4096];
				osdkReport->print(buf, sizeof(buf));
				WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : Invalid number of OSDKPlayerReport in OSDKPlayerReportsMap for " << playerId << " in OSDKReport:" << buf);

				isReportValid = false;
			}

			// Try to find the best user for the report
			if (isReportValid && (bestReportIt == reportItEnd || trustRating > mReportRating[playerId].mismatch))
			{
				bestReportIt = reportIt;
				trustRating = mReportRating[playerId].mismatch;
			}
		}

		// No valid report was found.
		if (bestReportIt == reportItEnd)
		{
			WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : a valid report was not found.");
			mCollatedReport->setError(Blaze::GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT);
			return *mCollatedReport;
		}
		else if (!baselineFinishedReport)
		{
			//  construct the baseline report using the trusted report.
			GameReport *report = bestReportIt->second;
			report->copyInto(collatedFinishedReport);
			baselineFinishedReport = true;

			FUT::PlayerReportMap::iterator iter = playerReportMap.find(bestReportIt->first);
			if (iter != playerReportMap.end())
			{
				iter->second->setIsBaseReport(true);
			}
			else
			{
				char8_t buf[4096];
				mFinishedGameInfo.print(buf, sizeof(buf));
				WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : did not find key (bestReportIt) " << bestReportIt->first << " in playerReportMap, mFinishedGameInfo.getPlayerInfoMap:" << buf);
				collatorReport.setInvalidPlayerMap(true);
			}

			INFO_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : Using game report (" << report->getGameReportingId() << ") from " << bestReportIt->first << " as the base with rating = " << trustRating << "." );
		}

		const EA::TDF::Tdf *customTdf = getComponent().getConfig().getCustomGlobalConfig();
		const FUT::CollatorConfig* collatorConfig = NULL;

		if (NULL != customTdf)
		{
			const OSDKGameReportBase::OSDKCustomGlobalConfig& customConfig = static_cast<const OSDKGameReportBase::OSDKCustomGlobalConfig&>(*customTdf);
			customTdf = customConfig.getGameCustomConfig();

			if (NULL != customTdf)
			{
				collatorConfig = static_cast<const FUT::CollatorConfig*>(customTdf);
			}
		}
		
		reportIt = finishedGameReports.begin();
		for (; reportIt != reportItEnd; ++reportIt)
		{
			GameReport *report = reportIt->second;
			GameManager::PlayerId playerId = reportIt->first;

			if (dnfStatus.find(playerId) == dnfStatus.end())
			{
				dnfStatus[playerId] = false;
			}

			reportCount++;

			//  compare baseline with this report - if different then report error.
			FUTGameReportCompare compare;

			if (NULL != collatorConfig)
			{
				compare.setConfig(*collatorConfig);
			}

			const GameType* gameType = getComponent().getGameTypeCollection().getGameType(report->getGameReportName());
			if (!compare.compare(*gameType, *collatedFinishedReport.getReport(), *report->getReport()))
			{
				WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : A difference was found in the game report id = " << report->getGameReportingId() << ".");
				
				// Enhance the collatedFinishedReport with the collision report
				collatorReport.setCollision(true);

				compare.getCollisionList().copyInto(collatorReport.getCollisionList());

				OSDKGameReportBase::OSDKReport* osdkBaseReport = static_cast<OSDKGameReportBase::OSDKReport*>(collatedFinishedReport.getReport());
				OSDKGameReportBase::OSDKReport* osdkReport = static_cast<OSDKGameReportBase::OSDKReport*>(report->getReport());
				
				if (NULL != osdkBaseReport && NULL != osdkReport)
				{
					const OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkBasePlayerMap = osdkBaseReport->getPlayerReports();
					const OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerMap = osdkReport->getPlayerReports();

					for (const OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::value_type& playerPair : osdkBasePlayerMap)
					{
						OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator osdkPlayerIt = osdkPlayerMap.find(playerPair.first);
						if (osdkPlayerIt != osdkPlayerMap.end())
						{
							EA::TDF::tdf_ptr<OSDKGameReportBase::OSDKPlayerReport> basePlayerReport = osdkPlayerIt->second;
							EA::TDF::tdf_ptr<OSDKGameReportBase::OSDKPlayerReport> playerReport = playerPair.second;

							// Determine whether the parameters defined by security are matching.
							if (NULL != basePlayerReport && NULL != playerReport)
							{
								if (basePlayerReport->getScore() != playerReport->getScore() ||
									GetPlayerRedCard(*basePlayerReport) != GetPlayerRedCard(*playerReport))
								{
									collatorReport.setCritMissMatch(true);
								}
							}
							else
							{
								WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : Users submitting report missing OSDKPlayerReport.");

								// When both players submit a report, the opponent data should be in it.
								mCollatedReport->setError(Blaze::GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT);
								return *mCollatedReport;
							}
						}
					}
				}
				else
				{
					WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : Users submitting report missing OSDKReport.");

					// The game should have a osdk report.
					mCollatedReport->setError(Blaze::GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT);
					return *mCollatedReport;
				}
			}
			
			// We still need to merge the maps when collision fails
			// Allows use to further analyze the data in the report slave
			// Final judgment will be made in the FUT Report Slave.
			if (getComponent().getConfig().getBasicConfig().getMergeReports())
			{
				collator.merge(collatedFinishedReport, *report);
			}
		}
	}

	//  collation requires at least one mid-game/finished report
	if (!baselineFinishedReport && !baselineReport)
	{
		INFO_LOG("[BasicGameReportCollator:" << this << "].finalizeCollatedGameReport() : collation requires at least one mid-game/finished report, continuing anyways ...");
		collatorReport.setValidReport(false);
	}
	else
	{
		// NOTE: In a rebroadcaster disconnect scenario, the user submitting the finished game report may not be flagged as (dnfStatus[playerId] = false)
		// This flag has been introduced to ensure that the full game report is processed under that scenario

		collatorReport.setValidReport(true);
	}

	// Confirm players in OSDK report are valid.
	if (true == baselineFinishedReport)
	{
		GameInfo::PlayerInfoMap& playerInfoMap = mFinishedGameInfo.getPlayerInfoMap();

		OSDKGameReportBase::OSDKReport* osdkBaseReport = static_cast<OSDKGameReportBase::OSDKReport*>(collatedFinishedReport.getReport());
		if (NULL != osdkBaseReport)
		{
			const OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkBasePlayerMap = osdkBaseReport->getPlayerReports();
			for (const OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::value_type& playerPair : osdkBasePlayerMap)
			{
				if (playerInfoMap.find(playerPair.first) == playerInfoMap.end())
				{
					WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : The player:" << playerPair.first << " is not part of the session.");

					mCollatedReport->setError(Blaze::GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT);
					return *mCollatedReport;
				}
			}
		}
	}

	//  generate collated dnf player report.
	GameReport collatedDnfReport;
	bool baselineDnfReport = false;

	Utilities::Collector::CategorizedGameReports dnfGameReports;
	mCollectorUtil.getGameReportsByCategory(dnfGameReports, Utilities::Collector::REPORT_TYPE_GAME_DNF);
	if (!dnfGameReports.empty())
	{
		Utilities::Collector::CategorizedGameReports::const_iterator reportIt = dnfGameReports.begin();
		Utilities::Collector::CategorizedGameReports::const_iterator reportItEnd = dnfGameReports.end();
		for (; reportIt != reportItEnd; ++reportIt)
		{
			GameReport *report = reportIt->second;
			GameManager::PlayerId playerId = reportIt->first;

			if (dnfStatus.find(playerId) == dnfStatus.end())
			{
				dnfStatus[playerId] = true;
			}

			// Users reporting Quit are uploading a report, so count it.
			FUT::PlayerReportMap::iterator iter = playerReportMap.find(reportIt->first);
			if (iter != playerReportMap.end())
			{
				iter->second->setIsDNFReport(true);
			}
			else
			{
				char8_t buf[4096];
				mFinishedGameInfo.print(buf, sizeof(buf));
				WARN_LOG("[FUTGameReportCollator:" << this << "].finalizeCollatedGameReport() : did not find key (reportIt - setIsDNFReport) " << reportIt->first << " in playerReportMap, mFinishedGameInfo.getPlayerInfoMap:" << buf);
				collatorReport.setInvalidPlayerMap(true);
			}

			reportCount++;

			if (!baselineDnfReport)
			{
				//  construct the baseline report.
				report->copyInto(collatedDnfReport);
				baselineDnfReport = true;
			}
			else
			{
				collator.merge(collatedDnfReport, *report);
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////
	//  MERGE COLLATED STAGE REPORTS INTO FINAL REPORT
	collator.setMergeMode(Utilities::Collator::MERGE_MAPS_COPY);

	//  merge dnf report, overwriting overlapped entries in the current final (mid-game) report.
	if (baselineDnfReport)
	{
		if (baselineReport)
		{
			collator.merge(collatedFinalReport, collatedDnfReport);
		}
		else
		{
			collatedDnfReport.copyInto(collatedFinalReport);
			baselineReport = true;
		}
	}

	// Now merge the post-game finished reports into the DNF report (aka current final report.)
	if (baselineFinishedReport)
	{
		if (baselineReport)
		{
			collator.merge(collatedFinalReport, collatedFinishedReport);
		}
		else
		{
			collatedFinishedReport.copyInto(collatedFinalReport);
			baselineReport = true;
		}
	}

	// Add the FUT collator data in the the final report
	collatorReport.setReportCount(reportCount);

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*collatedFinalReport.getReport());
	osdkReport.setEnhancedReport(collatorReport);

	//  copy final report to class copy and return the class copy.
	collatedFinalReport.copyInto(mCollatedReport->getGameReport());

	if (!baselineReport)
	{
		WARN_LOG("[BasicGameReportCollator:" << this << "].finalizeCollatedGameReport() : No baseline report to return for game, reporting id = " << mFinishedGameInfo.getGameReportingId() << ".");
		mCollatedReport->setError(GAMEREPORTING_COLLATION_ERR_NO_REPORTS);
	}
	else
	{
		mCollatedReport->setError(ERR_OK);
	}

	return *mCollatedReport;
}

}   // namespace GameReporting
}   // namespace Blaze
