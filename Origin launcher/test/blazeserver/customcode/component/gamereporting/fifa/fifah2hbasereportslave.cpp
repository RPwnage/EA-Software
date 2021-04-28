/*************************************************************************************************/
/*!
    \file   fifah2hbasereportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifah2hbasereportslave.h"
#include "fifalivecompetitionsdefines.h"
#include "fifa/tdf/fifah2hreport.h"

#include "stats/scope.h"

#include "xmshd/tdf/xmshdtypes.h"
#include "xmshd/xmshdslaveimpl.h" 

#include "EAStdC/EAString.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FifaH2HBaseReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaH2HBaseReportSlave::FifaH2HBaseReportSlave(GameReportingSlaveImpl& component) :
	GameH2HReportSlave(component),
	mIsSponsoredEventReport(false),
	mIsValidSponsoredEventId(false),
	mSponsoredEventId(0)
{
}

/*************************************************************************************************/
/*! \brief FifaH2HBaseReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaH2HBaseReportSlave::~FifaH2HBaseReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaH2HBaseReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaH2HBaseReportSlave::create(GameReportingSlaveImpl& component)
{
	// FifaH2HBaseReportSlave should not be instantiated
	return NULL;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError FifaH2HBaseReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* H2H_GAME_TYPE_NAME = getCustomH2HGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), H2H_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[FifaH2HBaseReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // common processing
        processCommon();

        // initialize game parameters
        initGameParams();

		processErr = initH2HGameParams();
		if(processErr != Blaze::ERR_OK)
		{
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

        // check if this is a sponsored event game and if the sponsored event ID is valid
        if (mIsSponsoredEventReport && (false == mIsValidSponsoredEventId))
        {
            WARN_LOG("[FifaH2HBaseReportSlave::" << this << "].process(): Sponsored event game with invalid sponsored event ID");
			delete mReportParser;
            return Blaze::ERR_SYSTEM;  // EARLY RETURN
        }

        if (true == skipProcess())
        {
            delete mReportParser;
            return processErr;  // EARLY RETURN
        }

		//setup the game status
		if (!initGameStatus())
		{
			processErr = Blaze::ERR_SYSTEM;
			return processErr;
		}

        // determine win/loss/tie
        determineGameResult();

		if (true == wasDNFPlayerWinning())
		{
			delete mReportParser;
			return processErr;
		}

		processErr = initializeExtensions();

		if (processErr != ERR_OK)
		{
			WARN_LOG("[FifaH2HBaseReportSlave::" << this << "].process(): Error initializing extensions " << processErr);
			delete mReportParser;
			return processErr;
		}

        // update player keyscopes
        updatePlayerKeyscopes();

        // Do not update the stats in the gamereporting.cfg file if the game report is for sponsored event game.
        if (!mIsSponsoredEventReport)
        {
            // parse the keyscopes from the configuration
            if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
            {
                WARN_LOG("[FifaH2HBaseReportSlave::" << this << "].process(): Error parsing keyscopes");

                processErr = mReportParser->getErrorCode();
                delete mReportParser;
                return processErr; // EARLY RETURN
            }
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

        // update game stats
        updateGameStats();

		selectCustomStats(); 

		UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
		statsUpdater.updateGamesPlayed(playerIds);
		statsUpdater.selectStats();

		// extract and set stats
        if (!mIsSponsoredEventReport)
        {
            // extract and set stats
            if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
            {
                WARN_LOG("[FifaH2HBaseReportSlave::" << this << "].process(): Error parsing stats");

                processErr = mReportParser->getErrorCode();
                delete mReportParser;
                return processErr; // EARLY RETURN
            }
        }

		bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
		processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

		if (Blaze::ERR_OK == processErr)
		{
			if (mIsSponsoredEventReport)
			{
				char8_t strBuffer[OSDKGameReportBase::SE_CATEGORY_MAX_NAME_LENGTH];
				getLiveCompetitionsStatsCategoryName(strBuffer, sizeof(strBuffer));
				if (Blaze::ERR_OK == transformStats(strBuffer))
				{
					statsUpdater.transformStats();
				}
			}
			else
			{
				if (Blaze::ERR_OK == transformStats(getCustomH2HStatsCategoryName()))
				{
					statsUpdater.transformStats();
				}
			}

			processErr = mUpdateStatsHelper.commitStats();
			TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].process() - commit stats error code: " << ErrorHelp::getErrorName(processErr));				
		}

		if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
		{
			// extract game history attributes from report.
			GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
			mReportParser->setGameHistoryReport(&historyReport);
			mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
			delete mReportParser;
		}

		// post game reporting processing()
		processErr = (true == processUpdatedStats()) ? Blaze::ERR_OK : Blaze::ERR_SYSTEM;

		// send end game mail
		sendEndGameMail(playerIds);

		// send updated data to xms
		sendDataToXms();
	}

    return processErr;
}

void FifaH2HBaseReportSlave::fillNrmlXmlData(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter, eastl::string &seasonsDataXml)
{
	seasonsDataXml.sprintf("<%s>\r\n", getXmsDataRootElementName());
	// fill overall and divisional stats
	customFillNrmlXmlData(playerIter->first, seasonsDataXml);

	Stats::StatDescs getStatsDescsResponse; // This allocates structures which will be pointed to from "statUpdates" - so make sure it has the same lifetime as statUpdates
	StatUpdateList statUpdates;

	readNrmlXmlData(getCustomH2HStatsCategoryName(), playerIter, statUpdates, getStatsDescsResponse);
	
	// Normal Game Stats
	StatUpdateList::iterator iter = statUpdates.begin();
	StatUpdateList::iterator end = statUpdates.end();
	seasonsDataXml.append_sprintf("\t<nrmlstslist>\r\n");

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate* statUpdate = iter;

		Stats::StatPeriodType periodType = Stats::STAT_PERIOD_ALL_TIME;

		StatValueUtil::StatValueMap::iterator statValueIter = statUpdate->stats.begin();
		StatValueUtil::StatValueMap::iterator statValueEnd = statUpdate->stats.end();

		seasonsDataXml.append_sprintf("\t\t<nrmlsts>\r\n");
		for (; statValueIter != statValueEnd; statValueIter++)
		{
			StatValueUtil::StatValue* statValue = &statValueIter->second;
			seasonsDataXml.append_sprintf("\t\t\t<%s>%" PRId64 "</%s>\r\n",statValueIter->first, statValue->iAfter[periodType], statValueIter->first);
		}
		seasonsDataXml.append_sprintf("\t\t</nrmlsts>\r\n");
	}

	seasonsDataXml.append_sprintf("\t</nrmlstslist>\r\n");

	seasonsDataXml.append_sprintf("</%s>", getXmsDataRootElementName());
}

const eastl::string FifaH2HBaseReportSlave::getXmsAssetName(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter)
{
	return "fifa_online_season.bin";
}

const char8_t* FifaH2HBaseReportSlave::getXmsDataRootElementName()
{
	return "seasdata";
}

FifaH2HBaseReportSlave::XmsRequestMetadata FifaH2HBaseReportSlave::getXmsRequestMetadata(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter)
{
	XmsRequestMetadata metadataCollection;

	Blaze::XmsHd::XmsAttributes* metaData = BLAZE_NEW Blaze::XmsHd::XmsAttributes;
	metaData->setName("SeasonsData");
	metaData->setType("string");
	metaData->setValue("Test_Value");

	metadataCollection.push_back(metaData);
	return metadataCollection;
}

void FifaH2HBaseReportSlave::sendDataToXmsFiberCall(uint64_t nucleusId, eastl::string xmlPayload, eastl::string assetName, XmsRequestMetadata metadata)
{
	Blaze::XmsHd::XmsHdSlave *xmsHdComponent = static_cast<Blaze::XmsHd::XmsHdSlave*>(gController->getComponent(Blaze::XmsHd::XmsHdSlave::COMPONENT_ID, false));

	if (xmsHdComponent != NULL)
	{
		Blaze::XmsHd::PublishDataRequest request;

		request.setNucleusId(nucleusId);
		request.setXmsAssetName(assetName.c_str());

		XmsRequestMetadata::iterator start = metadata.begin();
		XmsRequestMetadata::iterator end = metadata.end();
		for(XmsRequestMetadata::iterator iter = start; iter != end; ++iter)
		{
			request.getAttributes().push_back(*iter);
		}

		Blaze::XmsHd::XmsBinary* binaryData = BLAZE_NEW Blaze::XmsHd::XmsBinary;
		binaryData->setName(assetName.c_str());

		binaryData->setData(xmlPayload.c_str());
		binaryData->setDataSize(strlen(xmlPayload.c_str()));

		request.getBinaries().push_back(binaryData);

		UserSession::pushSuperUserPrivilege();
		xmsHdComponent->publishData(request);
		UserSession::popSuperUserPrivilege();
	}
}

void FifaH2HBaseReportSlave::sendDataToXms()
{
	Blaze::XmsHd::XmsHdSlave *xmsHdComponent = static_cast<Blaze::XmsHd::XmsHdSlave*>(gController->getComponent(Blaze::XmsHd::XmsHdSlave::COMPONENT_ID, false));

	if (xmsHdComponent != NULL)
	{

		OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
		playerIter = OsdkPlayerReportsMap.begin();
		playerEnd = OsdkPlayerReportsMap.end();

		for(; playerIter != playerEnd; playerIter++)
		{
			if (!sendXmsDataForPlayer(playerIter))
			{
				continue;
			}
			eastl::string xmlPayload;

			fillNrmlXmlData(playerIter, xmlPayload);

			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			uint64_t nucleusId = playerReport.getNucleusId();

			if (nucleusId == 0)
			{
				ERR_LOG("[FifaH2HBaseReportSlave:"<<this<<"].sendDataToXms() nucleusId is 0, don't send data to XMS HD");
				return;
			}

			TRACE_LOG("[FifaH2HBaseReportSlave:"<<this<<"].sendDataToXms() xmlPayload for nucleusId: " << nucleusId << ": " << xmlPayload << ". Asset name: " << getXmsAssetName(playerIter).c_str());
			TRACE_LOG("[FifaH2HBaseReportSlave:"<<this<<"].sendDataToXms() xmlPayload size " << xmlPayload.size());			
			XmsRequestMetadata metadata = getXmsRequestMetadata(playerIter);

			{
				gSelector->scheduleFiberCall<FifaH2HBaseReportSlave, uint64_t, eastl::string, eastl::string, XmsRequestMetadata>(
					this,
					&FifaH2HBaseReportSlave::sendDataToXmsFiberCall,
					nucleusId,
					xmlPayload,
					getXmsAssetName(playerIter),
					metadata,
					"FifaH2HBaseReportSlave::sendDataToXms");
			}
		}
	}
}

void FifaH2HBaseReportSlave::readNrmlXmlData(const char8_t* statsCategory, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter, StatUpdateList& statUpdates, Stats::StatDescs& getStatsDescsResponse)
{
	Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
    if (NULL != statsComponent)
    {
 		Blaze::BlazeRpcError error = Blaze::ERR_OK;

        // Get description data
		Stats::GetStatDescsRequest getStatsDescRequest;
		getStatsDescRequest.setCategory(getCustomH2HStatsCategoryName());
		error = statsComponent->getStatDescs(getStatsDescRequest, getStatsDescsResponse);

        if (Blaze::ERR_OK == error)
        {
			Stats::ScopeNameValueMap playerKeyScopeMap;
			playerKeyScopeMap[SCOPENAME_ACCOUNTCOUNTRY]= 0; // use the aggregate row for country
			playerKeyScopeMap[SCOPENAME_CONTROLS]= Stats::KEY_SCOPE_VALUE_ALL; // grab all control rows
			
			// Get category data
			Stats::GetStatsRequest getStatsRequest;
			getStatsRequest.setPeriodType(Stats::STAT_PERIOD_ALL_TIME);
			
			getStatsRequest.getEntityIds().push_back(playerIter->first);
			getStatsRequest.setCategory(getCustomH2HStatsCategoryName());
			getStatsRequest.getKeyScopeNameValueMap().assign(playerKeyScopeMap.begin(), playerKeyScopeMap.end());
			Stats::GetStatsResponse getStatsResponse;
					
			UserSession::pushSuperUserPrivilege();
			error = statsComponent->getStats(getStatsRequest, getStatsResponse);
			UserSession::popSuperUserPrivilege();

			if (Blaze::ERR_OK == error)
			{
				// Get Key scope stats data
				Stats::KeyScopeStatsValueMap::const_iterator it = getStatsResponse.getKeyScopeStatsValueMap().begin();
				Stats::KeyScopeStatsValueMap::const_iterator itEnd = getStatsResponse.getKeyScopeStatsValueMap().end();
                    
				for (; it != itEnd; it++)
				{
					Stats::ScopeNameValueMap queryScopeMap;
					Stats::genScopeValueListFromKey(it->first.c_str(), queryScopeMap);
							
					OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
					const Stats::StatValues* statValues = it->second;

					if ((statValues != NULL) && (queryScopeMap[SCOPENAME_CONTROLS] != 0))
					{
						StatValueUtil::StatUpdate statUpdate;
						insertStat(statUpdate, "country", Stats::STAT_PERIOD_ALL_TIME, playerReport.getAccountCountry()); // Fudge the country to use current country
						insertStat(statUpdate, "controls", Stats::STAT_PERIOD_ALL_TIME, queryScopeMap[SCOPENAME_CONTROLS]);

						const Stats::StatValues::EntityStatsList &entityList = statValues->getEntityStatsList();
						if (entityList.size() > 0)
						{
							Stats::StatValues::EntityStatsList::const_iterator entityItr = entityList.begin();
							Stats::StatValues::EntityStatsList::const_iterator entityItrEnd = entityList.end();
                                
							const Stats::StatGroupResponse::StatDescSummaryList &statDescList = getStatsDescsResponse.getStatDescs();
							Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItr = statDescList.begin();
							Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItrEnd = statDescList.end();                                

							// Go through both stats data and group data lists
							for (; (entityItr != entityItrEnd); ++entityItr)
							{
								if ((*entityItr) != NULL && (*statDescItr) != NULL)
								{
									const Stats::EntityStats::StringStatValueList &statList = (*entityItr)->getStatValues();

									Stats::EntityStats::StringStatValueList::const_iterator statValuesItr = statList.begin();
									Stats::EntityStats::StringStatValueList::const_iterator statValuesItrEnd = statList.end();

									for(; (statValuesItr != statValuesItrEnd) && (statDescItr != statDescItrEnd); ++statValuesItr, ++statDescItr)
									{
										if (NULL != (*statValuesItr) && (*statDescItr) != NULL && !(*statDescItr)->getDerived())
										{
											char8_t statsValue[32];
											blaze_strnzcpy(statsValue,(*statValuesItr), sizeof(statsValue));
											insertStat(statUpdate, (*statDescItr)->getName(), Stats::STAT_PERIOD_ALL_TIME, atoi(statsValue));
										}
									}
								}
							}
						}
						else
						{
								WARN_LOG("[fifah2hbasereportslave] error - there's no NormalGameStats data in stats DB");
						}
						statUpdates.push_back(statUpdate);
					}
					else if (statValues == NULL)
					{
						 WARN_LOG("[fifah2hbasereportslave] error - load NormalGameStats category failed (" << error << ")");
					}
				}
			}
		}
		else
		{
			WARN_LOG("[fifah2hbasereportslave] error - load NormalGameStats StatDesc failed (" << error << ")");
		}
	}
}


void FifaH2HBaseReportSlave::determineGameResult()
{
	updateUnadjustedScore();

	adjustRedCardedPlayers();
	determineWinPlayer();
	adjustPlayerResult();
}

Fifa::CommonGameReport* FifaH2HBaseReportSlave::getGameReport()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReport& osdkGameReport = OsdkReport.getGameReport();

	Fifa::CommonGameReport* commonGameReport = nullptr;
	Fifa::H2HGameReport* h2hGameReport = static_cast<Fifa::H2HGameReport*>(osdkGameReport.getCustomGameReport());
	if (h2hGameReport != nullptr)
	{
		commonGameReport = &h2hGameReport->getCommonGameReport();
	}
	return commonGameReport;
}

OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap* FifaH2HBaseReportSlave::getEntityReportMap()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	return &OsdkPlayerReportsMap;
}

Fifa::CommonPlayerReport* FifaH2HBaseReportSlave::getCommonPlayerReport(OSDKGameReportBase::OSDKPlayerReport& entityReport)
{
	Fifa::CommonPlayerReport* commonPlayerReport = nullptr;

	Fifa::H2HPlayerReport* h2hPlayerReport = static_cast<Fifa::H2HPlayerReport*>(entityReport.getCustomPlayerReport());
	if (h2hPlayerReport != nullptr)
	{
		commonPlayerReport = &h2hPlayerReport->getCommonPlayerReport();
	}
	return commonPlayerReport;
}

bool FifaH2HBaseReportSlave::didEntityFinish(int64_t entityId, OSDKGameReportBase::OSDKPlayerReport& entityReport)
{
	return didPlayerFinish(entityId, entityReport);
}

OSDKGameReportBase::OSDKPlayerReport* FifaH2HBaseReportSlave::findEntityReport(int64_t entityId)
{
	OSDKPlayerReport* playerReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter;
	playerIter = osdkPlayerReportsMap.find(entityId);
	if (playerIter != osdkPlayerReportsMap.end())
	{
		playerReport = playerIter->second;
	}

	return playerReport;
}

int64_t FifaH2HBaseReportSlave::getWinningEntity()
{
	if(mWinnerSet.begin()!=mWinnerSet.end())
	{
		return *mWinnerSet.begin();
	}
	else
	{
		return 0;
	}
	
}

int64_t FifaH2HBaseReportSlave::getLosingEntity()
{
	if(mLoserSet.begin()!=mLoserSet.end())
	{
		return *mLoserSet.begin();
	}
	else
	{
		return 0;
	}
}

bool FifaH2HBaseReportSlave::isTieGame()
{
	return mTieGame;
}

bool FifaH2HBaseReportSlave::isEntityWinner(int64_t entityId)
{
	return (mWinnerSet.find(entityId) != mWinnerSet.end());
}

bool FifaH2HBaseReportSlave::isEntityLoser(int64_t entityId)
{
	return (mLoserSet.find(entityId) != mLoserSet.end());
}

int32_t FifaH2HBaseReportSlave::getHighestEntityScore()
{
	return mHighestPlayerScore;
}

int32_t FifaH2HBaseReportSlave::getLowestEntityScore()
{
	return mLowestPlayerScore;
}

void FifaH2HBaseReportSlave::setCustomEntityResult(OSDKGameReportBase::OSDKPlayerReport& entityReport, StatGameResultE gameResult)
{
	entityReport.setUserResult(gameResult);
}

void FifaH2HBaseReportSlave::setCustomEntityScore(OSDKGameReportBase::OSDKPlayerReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst)
{
	Fifa::H2HPlayerReport& fifaH2HPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*entityReport.getCustomPlayerReport());
	Fifa::H2HCustomPlayerData& fifaH2HCustomPlayerData = fifaH2HPlayerReport.getH2HCustomPlayerData();
	fifaH2HCustomPlayerData.setGoalAgainst(goalsAgainst);
}

void FifaH2HBaseReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	updateWinPlayerStats(playerId, playerReport);

	if (true == didAllPlayersFinish())
	{
		TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].updateGameModeWinPlayerStats() - all players finished set WinnerByDnf to 0");
		playerReport.setWinnerByDnf(0);
	}
	else
	{
		TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].updateGameModeWinPlayerStats() - all players did NOT finish set WinnerByDnf to 1");
		playerReport.setWinnerByDnf(1);
	}

	mBuilder.incrementStat(STATNAME_WSTREAK, 1);
	mBuilder.assignStat(STATNAME_LSTREAK, static_cast<int64_t>(0));

	// For non-sponsored event, the win player stats should already incremented according to the gamereporting.cfg setting
	if (mIsSponsoredEventReport)
	{
		mBuilder.incrementStat(STATNAME_WINS, 1);
	}
}

void FifaH2HBaseReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	updateLossPlayerStats(playerId, playerReport);
	playerReport.setWinnerByDnf(0);

	mBuilder.assignStat(STATNAME_WSTREAK, static_cast<int64_t>(0));
	mBuilder.incrementStat(STATNAME_LSTREAK, 1);

	// For non-sponsored event, the player loss stat should already incremented according to the gamereporting.cfg setting
	if (mIsSponsoredEventReport)
	{
		mBuilder.incrementStat(STATNAME_LOSSES, 1);
	}
}

void FifaH2HBaseReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	updateTiePlayerStats(playerId, playerReport);
	playerReport.setWinnerByDnf(0);

	mBuilder.assignStat(STATNAME_WSTREAK, static_cast<int64_t>(0));
	mBuilder.assignStat(STATNAME_LSTREAK, static_cast<int64_t>(0));

	// For non-sponsored event, the player loss stat should already incremented according to the gamereporting.cfg setting
	if (mIsSponsoredEventReport)
	{
		mBuilder.incrementStat(STATNAME_TIES, 1);
	}
}

bool FifaH2HBaseReportSlave::wasDNFPlayerWinning()
{
	bool wasDnfPlayerWinning = false;

	if (mGameDNF && !mRedCardRuleViolation && mUnadjustedLowScore != mUnadjustedHighScore)
	{
		OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerFirst, playerEnd;
		playerIter = playerFirst = OsdkPlayerReportsMap.begin();
		playerEnd = OsdkPlayerReportsMap.end();

		for (; playerIter != playerEnd; ++playerIter)
		{
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			if (playerReport.getUserResult() == GAME_RESULT_DISCONNECTANDCOUNT)
			{
				//check for user quit
				StatGameResultE gResult = GAME_RESULT_COMPLETE_REGULATION;
				OSDKGameReportBase::IntegerAttributeMap& intMap = playerReport.getPrivatePlayerReport().getPrivateIntAttributeMap();
				if (intMap.find("GRESULT") != intMap.end())
				{
					gResult = (static_cast<StatGameResultE>(intMap["GRESULT"]));
				}

				//grab the common stats report and check if the user was winning
				Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
				Fifa::CommonPlayerReport& commonPlayerReport = h2hPlayerReport.getCommonPlayerReport();

				if (commonPlayerReport.getUnadjustedScore() >= mUnadjustedHighScore && gResult != GAME_RESULT_QUITANDCOUNT)
				{
					TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].wasDNFPlayerWinning(): user was winning but got disconnected:" << playerIter->first << " getUnadjustedScore:" << commonPlayerReport.getUnadjustedScore() << " ");

					wasDnfPlayerWinning = true;
					break;
				}
			}
		}
	}

	return wasDnfPlayerWinning;
}

bool FifaH2HBaseReportSlave::didAllPlayersFinish()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator iter, end;
	iter = osdkPlayerReportMap.begin();
	end = osdkPlayerReportMap.end();

	bool result = true;
	for (; iter != end; ++iter)
	{
		OSDKGameReportBase::OSDKPlayerReport* playerReport = iter->second;
		if (!didPlayerFinish(iter->first, *playerReport))
		{
			result = false;
			break;
		}
	}

	return result;
}

void FifaH2HBaseReportSlave::sendEndGameMail(GameManager::PlayerIdList& playerIds) {};// feature silently removed from fifa14  but preserved it for future in the base class

void FifaH2HBaseReportSlave::getLiveCompetitionsStatsCategoryName(char8_t* pOutputBuffer, uint32_t uOutputBufferSize) const
{
	// parent implementation does nothing
}

BlazeRpcError FifaH2HBaseReportSlave::initH2HGameParams()
{
	// extract the sponsored event id
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReport& osdkGameReport = OsdkReport.getGameReport();
	Fifa::H2HGameReport* h2hGameReport = static_cast<Fifa::H2HGameReport*>(osdkGameReport.getCustomGameReport());
	if(NULL == h2hGameReport)
	{
		ERR_LOG("[FifaH2HBaseReportSlave:" << this << "].initH2HGameParams() this is not a h2h game report!");
		return Blaze::ERR_SYSTEM;
	}

	mSponsoredEventId = h2hGameReport->getSponsoredEventId();

	if (mSponsoredEventId > INVALID_SPONSORED_EVENT_ID)
	{
		TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].initH2HGameParams() SponsoredEventId:" << mSponsoredEventId);
		//mIsValidSponsoredEventId = mReportConfig->validateSponsoredEvent(mSponsoredEventId);
		mIsValidSponsoredEventId = true;
		mIsSponsoredEventReport = true;
	}
	else
	{
		// explicitly declaring that this event is NOT a sponsored event
		mIsSponsoredEventReport = false;
	}

	return Blaze::ERR_OK;
}

void FifaH2HBaseReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	if (mIsValidSponsoredEventId)
	{
		TRACE_LOG("[FifaH2HReportSlave:" << this << "].updateCustomPlayerStats() for SponsoredEventId:" << mSponsoredEventId);

		mBuilder.incrementStat(STATNAME_PTSFOR, playerReport.getScore());
		mBuilder.incrementStat(STATNAME_PTSAGAINST, playerReport.getPointsAgainst());

		//update common stats
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
		Fifa::CommonPlayerReport& commonPlayerReport = h2hPlayerReport.getCommonPlayerReport();
		mBuilder.incrementStat("goals",			 commonPlayerReport.getGoals());
		mBuilder.incrementStat("ownGoals",		 commonPlayerReport.getOwnGoals());
		mBuilder.incrementStat("possession",	 commonPlayerReport.getPossession());
		mBuilder.incrementStat("shotsForOnGoal", commonPlayerReport.getShotsOnGoal());
		mBuilder.incrementStat("shotsFor",		 commonPlayerReport.getShots());
		mBuilder.incrementStat("tacklesAttempted", commonPlayerReport.getTackleAttempts());
		mBuilder.incrementStat("tacklesMade",	commonPlayerReport.getTacklesMade());
		mBuilder.incrementStat("passAttempts",	commonPlayerReport.getPassAttempts());
		mBuilder.incrementStat("passesMade",	commonPlayerReport.getPassesMade());
		mBuilder.incrementStat("fouls",			commonPlayerReport.getFouls());
		mBuilder.incrementStat("yellowCards",	commonPlayerReport.getYellowCard());
		mBuilder.incrementStat("redCards",		commonPlayerReport.getRedCard());
		mBuilder.incrementStat("corners",		commonPlayerReport.getCorners());
		mBuilder.incrementStat("offsides",		commonPlayerReport.getOffsides());

		//update custom stats
		Fifa::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport.getH2HCustomPlayerData();

		mBuilder.incrementStat("shotsAgainst",		 h2hCustomPlayerData.getShotsAgainst());
		mBuilder.incrementStat("shotsAgainstOnGoal", h2hCustomPlayerData.getShotsAgainstOnGoal());
		mBuilder.incrementStat("goalsAgainst",		 h2hCustomPlayerData.getGoalAgainst());

		// assign team name
		mBuilder.assignStat("teamName",			 h2hCustomPlayerData.getTeamName());
		mBuilder.assignStat("team_id",			 h2hCustomPlayerData.getTeam());
	}
}

//void FifaH2HBaseReportSlave::getGameModeGameHistoryQuery(char8_t *query, int32_t querySize)
//{
//	if (NULL != query)
//	{
//		if (false == mIsSponsoredEventReport)
//		{
//			blaze_snzprintf(query, querySize, "%s%s", mProcessedReport->getGameType().getGameTypeName().c_str(), "_skill_damping_query");
//		}
//		else
//		{
//			const char8_t* eventQuery = mReportConfig->getOSDKCustomSponsoredEvents()->getQuery();
//			blaze_strnzcpy(query, eventQuery, querySize);
//		}
//
//		TRACE_LOG("[GameH2HReportSlave:" << this << "].getGameModeGameHistoryQuery() query:" << query << ", querySize:" << querySize);
//	}
//}

void FifaH2HBaseReportSlave::updateNotificationReport()
{
	if (true == mIsSponsoredEventReport)
	{
		char8_t strBuffer[OSDKGameReportBase::SE_CATEGORY_MAX_NAME_LENGTH];
		getLiveCompetitionsStatsCategoryName(strBuffer, sizeof(strBuffer));
		updateCustomNotificationReport(strBuffer);
	}
	else
	{
		updateCustomNotificationReport(getCustomH2HStatsCategoryName());
	}
}

void FifaH2HBaseReportSlave::updatePlayerKeyscopes()
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
				TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].updatePlayerKeyscopes() AccountCountry " << accountLocale << " for Player " << playerId);
			}
		}

		Stats::ScopeNameValueMap* indexMap = new Stats::ScopeNameValueMap();
		if (indexMap != NULL)
		{
			if (false == mIsSponsoredEventReport)
			{
				(*indexMap)[SCOPENAME_ACCOUNTCOUNTRY] = playerReport.getAccountCountry();
			}
			mPlayerKeyscopes[playerId] = indexMap;
		}
	}

	updateCustomPlayerKeyscopes(OsdkPlayerReportsMap);
}

void FifaH2HBaseReportSlave::updatePlayerStats()
{
	TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].updatePlayerStats()");

	// aggregate players' stats
	aggregatePlayerStats();

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	GameType::StatUpdate statUpdate;

	for(; playerIter != playerEnd; ++playerIter)
	{
		GameManager::PlayerId playerId = playerIter->first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
		Stats::ScopeNameValueMap indexMap;

		// Pull the player's keyscopes from the gamereporting configuration file.
		GameTypeReportConfig& playerConfig = *mProcessedReport->getGameType().getConfig().getSubreports().find("playerReports")->second;
		GameTypeReportConfig& customPlayerConfig = *playerConfig.getSubreports().find("customPlayerReport")->second;
		//Begin FIFA Specific
		GameTypeReportConfig& fifaCommonPlayerConfig = *customPlayerConfig.getSubreports().find("CommonPlayerReport")->second;
		bool getStatUpdateSuccess = mProcessedReport->getGameType().getStatUpdate(fifaCommonPlayerConfig, "PlayerStats", statUpdate);

		if (!getStatUpdateSuccess)
		{
			TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].updatePlayerStats() - no valid stats update");
			continue;
		}

		if (!mIsValidSponsoredEventId)
		{
			mReportParser->generateScopeIndexMap(indexMap, playerId, statUpdate);
		}

		// Pull the player's keyscopes from the mPlayerKeyscopes map.
		PlayerScopeIndexMap::const_iterator scopeIter = mPlayerKeyscopes.find(playerId);
		if(scopeIter != mPlayerKeyscopes.end())
		{
			Stats::ScopeNameValueMap *scopeMap = scopeIter->second;
			Stats::ScopeNameValueMap::iterator it = scopeMap->begin();

			for(; it != scopeMap->end(); it++)
			{
				// do NOT use insert here - generateScopeIndexMap inserts the value already (with 0
				// as the value), and calling insert a second time will not update this value. 
				indexMap[it->first] = it->second; 
			}
		}

		if (false == mIsSponsoredEventReport)
		{
			mBuilder.startStatRow(getCustomH2HStatsCategoryName(), playerId, &indexMap);
		}
		else
		{
			char8_t strBuffer[OSDKGameReportBase::SE_CATEGORY_MAX_NAME_LENGTH];
			getLiveCompetitionsStatsCategoryName(strBuffer, sizeof(strBuffer));
			mBuilder.startStatRow(strBuffer, playerId, &indexMap);
		}

		if (true == mTieGame)
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

		mBuilder.completeStatRow();

		// update game capping information for live competitions
		if (mIsSponsoredEventReport)
		{
			Stats::ScopeNameValueMap playerKeyScopeMap;
			playerKeyScopeMap[SCOPENAME_COMPETITIONID]= mSponsoredEventId;
			
			mBuilder.startStatRow("LiveCompGamesPlayedAllTime", playerId, &playerKeyScopeMap);
			mBuilder.incrementStat("gamesPlayedAllTime", "1");
			mBuilder.completeStatRow();

			mBuilder.startStatRow("LiveCompGamesPlayedMonthly", playerId, &playerKeyScopeMap);
			mBuilder.incrementStat("gamesPlayedThisMonth", "1");
			mBuilder.completeStatRow();

			mBuilder.startStatRow("LiveCompGamesPlayedWeekly", playerId, &playerKeyScopeMap);
			mBuilder.incrementStat("gamesPlayedThisWeek", "1");
			mBuilder.completeStatRow();

			mBuilder.startStatRow("LiveCompGamesPlayedDaily", playerId, &playerKeyScopeMap);
			mBuilder.incrementStat("gamesPlayedToday", "1");
			mBuilder.completeStatRow();
		}
	}
}

void FifaH2HBaseReportSlave::updatePlayerStatsConclusionType()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	mIsUpdatePlayerConclusionTypeStats = false;

	for(; playerIter != playerEnd; ++playerIter)
	{
		GameManager::PlayerId playerId = playerIter->first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
		Stats::ScopeNameValueMap indexMap;

		uint32_t userResult = playerReport.getUserResult(); // ATTRIBNAME_USERRESULT
		if (GAME_RESULT_DNF_QUIT == userResult || GAME_RESULT_DNF_DISCONNECT == userResult)
		{
			TRACE_LOG("[FifaH2HBaseReportSlave:" << this << "].updatePlayerStatsConclusionType() for playerId: " << playerId << " with UserResult:" << userResult);
			mIsUpdatePlayerConclusionTypeStats = true;

			// Pull the player's keyscopes from the gamereporting configuration file.
			GameType::StatUpdate statUpdate;
			GameTypeReportConfig& playerConfig = *mProcessedReport->getGameType().getConfig().getSubreports().find("playerReports")->second;
			GameTypeReportConfig& customPlayerConfig = *playerConfig.getSubreports().find("customPlayerReport")->second;
			mProcessedReport->getGameType().getStatUpdate(customPlayerConfig, "PlayerStats", statUpdate);
			
			if (!mIsValidSponsoredEventId)
			{
				mReportParser->generateScopeIndexMap(indexMap, playerId, statUpdate);
			}

			// Pull the player's keyscopes from the mPlayerKeyscopes map.
			PlayerScopeIndexMap::const_iterator scopeIter = mPlayerKeyscopes.find(playerId);
			if(scopeIter != mPlayerKeyscopes.end())
			{
				Stats::ScopeNameValueMap *scopeMap = scopeIter->second;
				Stats::ScopeNameValueMap::iterator it = scopeMap->begin();

				for(; it != scopeMap->end(); it++)
				{
					indexMap[it->first] = it->second; 
				}
			}

			if (false == mIsSponsoredEventReport)
			{
				mBuilder.startStatRow(getCustomH2HStatsCategoryName(), playerId, &indexMap);
				mBuilder.incrementStat(STATNAME_TOTAL_GAMES_PLAYED, 1);
				mBuilder.incrementStat(STATNAME_GAMES_NOT_FINISHED, 1);
			}
			else
			{
				char8_t strBuffer[OSDKGameReportBase::SE_CATEGORY_MAX_NAME_LENGTH];
				getLiveCompetitionsStatsCategoryName(strBuffer, sizeof(strBuffer));
				mBuilder.startStatRow(strBuffer, playerId, &indexMap);
			}

			mBuilder.incrementStat(STATNAME_LOSSES, 1);

			mBuilder.completeStatRow();
		}
	}
}

}   // namespace GameReporting

}   // namespace Blaze

