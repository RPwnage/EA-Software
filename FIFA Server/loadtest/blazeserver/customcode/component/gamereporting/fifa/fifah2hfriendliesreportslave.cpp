/*************************************************************************************************/
/*!
    \file   fifah2hfriendliesreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifah2hfriendliesreportslave.h"

#include "fifa/tdf/fifah2hreport.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaH2HFriendliesReportSlave::FifaH2HFriendliesReportSlave(GameReportingSlaveImpl& component) :
FifaH2HBaseReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaH2HFriendliesReportSlave::~FifaH2HFriendliesReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaH2HFriendliesReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaH2HFriendliesReportSlave") FifaH2HFriendliesReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head game used in gamereporting.cfg

    \return - the H2H game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FifaH2HFriendliesReportSlave::getCustomH2HGameTypeName() const
{
    return "gameType20";
}

/*************************************************************************************************/
/*! \brief getCustomH2HStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for H2H game
*/
/*************************************************************************************************/
const char8_t* FifaH2HFriendliesReportSlave::getCustomH2HStatsCategoryName() const
{
    return "NormalGameStats";
}

const uint16_t FifaH2HFriendliesReportSlave::getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const
{
	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
	return h2hPlayerReport.getH2HCustomPlayerData().getControls();
}

/*************************************************************************************************/
/*! \brief updateCustomPlayerStats
    Update custom stats that are regardless of the game result
*/
/*************************************************************************************************/
void FifaH2HFriendliesReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
//	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
}

/*************************************************************************************************/
/*! \brief updateCustomH2HSeasonalPlayerStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void FifaH2HFriendliesReportSlave::updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateCustomPlayerStats(/*playerId*/0, playerReport);
}

/*************************************************************************************************/
/*! \brief updateCustomH2HSeasonalPlayerStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void FifaH2HFriendliesReportSlave::updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport)
{
}

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FifaH2HFriendliesReportSlave::updateCustomNotificationReport(const char8_t* statsCategory)
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

	//Stats were already committed and can be evaluated during that process. Fetching stats again
	//to ensure mUpdateStatsHelper cache is valid (and avoid crashing with assert)
	BlazeRpcError fetchError = mUpdateStatsHelper.fetchStats();
    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;

        // Obtain the Skill Points
        Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(statsCategory, playerId);
        if (DB_ERR_OK == fetchError && NULL != key)
        {
			int64_t overallSkillPoints = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, Stats::STAT_PERIOD_ALL_TIME, false));

            // Update the report notification custom data with the overall SkillPoints
			Fifa::H2HNotificationPlayerCustomData* playerCustomData = BLAZE_NEW Fifa::H2HNotificationPlayerCustomData();
            playerCustomData->setSkillPoints(static_cast<uint32_t>(overallSkillPoints));

            // Insert the player custom data into the player custom data map
            playerCustomDataMap.insert(eastl::make_pair(playerId, playerCustomData));
        }
    }

    // Set the gameCustomData to the OsdkReportNotification
    OsdkReportNotification->setCustomDataReport(*gameCustomData);
}


void FifaH2HFriendliesReportSlave::updatePlayerStats()
{
	TRACE_LOG("[FifaH2HFriendliesReportSlave:" << this << "].updatePlayerStats()");

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
/*! \brief setCustomEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FifaH2HFriendliesReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
    HttpParam param;
    param.name = "gameName";
    param.value = "BlazeGame";
    param.encodeValue = true;

    mailParamList->push_back(param);
	blaze_snzprintf(mailTemplateName, MAX_MAIL_TEMPLATE_NAME_LEN, "%s", "h2h_gamereport");
    
    return true;
}
*/
void FifaH2HFriendliesReportSlave::selectCustomStats()
{
	HeadtoHeadFriendliesExtension::ExtensionData data;
	data.mProcessedReport = mProcessedReport;
	data.mTieSet = &mTieSet;
	data.mWinnerSet = &mWinnerSet;
	data.mLoserSet = &mLoserSet;

	mFriendliesExtension.setExtensionData(&data);
	mFriendlies.setExtension(&mFriendliesExtension);
	mFriendlies.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);

	mFriendlies.selectFriendliesStats();
}

void FifaH2HFriendliesReportSlave::updateCustomTransformStats(const char8_t* statsCategory)
{
	mFriendlies.transformFriendliesStats();
}

//We won't be using the statsCategory parameter; we're looking for another stats category
void FifaH2HFriendliesReportSlave::readNrmlXmlData(const char8_t* statsCategory, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator & playerIter, StatUpdateList& statUpdates, Stats::StatDescs& getStatsDescsResponse)
{
	mNucleausIdMap.clear();
	statsCategory = "FriendlyGameStats";
	Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
	if (NULL != statsComponent)
	{
		Blaze::BlazeRpcError error = Blaze::ERR_OK;

		// Get description data
		Stats::GetStatDescsRequest getStatsDescRequest;
		getStatsDescRequest.setCategory(statsCategory);
		error = statsComponent->getStatDescs(getStatsDescRequest, getStatsDescsResponse);

		if (Blaze::ERR_OK == error)
		{
			int64_t friendBlazeId = 0;
			int64_t friendNucleusId = 0;
			OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator pIter, pEnd;
			pIter = OsdkPlayerReportsMap.begin();
			pEnd = OsdkPlayerReportsMap.end();
			for (; pIter != pEnd; ++pIter)
			{
				if (pIter != playerIter)
				{
					friendBlazeId = pIter->first;
					break;
				}
			}
			if (friendBlazeId != 0)
			{
				UserInfoPtr userInfo;
				BlazeRpcError blazeRpcError = gUserSessionManager->lookupUserInfoByBlazeId(friendBlazeId, userInfo);
				if (blazeRpcError != ERR_OK)
				{
					ERR_LOG("[FifaH2HFriendliesReportSlave::readNrmlXmlData] couldn't lookup nucleusId from blazeId: " << friendBlazeId << "; error: '" << ErrorHelp::getErrorName(blazeRpcError) <<"'.");
				}
				else
				{
					friendNucleusId = userInfo->getPlatformInfo().getEaIds().getNucleusAccountId();
				}
			}
			else
			{
				ERR_LOG("[FifaH2HFriendliesReportSlave::readNrmlXmlData] couldn't find blazeId for friend of blazeid: " << playerIter->first);
			}
			mNucleausIdMap.insert(eastl::make_pair(playerIter->first, friendNucleusId));

			Stats::ScopeNameValueMap playerKeyScopeMap;
			playerKeyScopeMap[SCOPENAME_FRIEND]= friendBlazeId;

			// Get category data
			Stats::GetStatsRequest getStatsRequest;
			getStatsRequest.setPeriodType(Stats::STAT_PERIOD_ALL_TIME);

			getStatsRequest.getEntityIds().push_back(playerIter->first);
			getStatsRequest.setCategory(statsCategory);
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

					const Stats::StatValues* statValues = it->second;

					if (statValues != NULL)
					{
						StatValueUtil::StatUpdate statUpdate;
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
							WARN_LOG("[FifaH2HFriendliesReportSlave] error - there's no FriendlyGameStats data in stats DB");
						}
						statUpdates.push_back(statUpdate);
					}
					else
					{
						WARN_LOG("[FifaH2HFriendliesReportSlave] error - load FriendlyGameStats category failed (" << error << ")");
					}
				}
			}
		}
		else
		{
			WARN_LOG("[FifaH2HFriendliesReportSlave] error - load FriendlyGameStats StatDesc failed (" << error << ")");
		}
	}
}

const eastl::string FifaH2HFriendliesReportSlave::getXmsAssetName(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator pIter, pEnd;
	pIter = OsdkPlayerReportsMap.begin();
	pEnd = OsdkPlayerReportsMap.end();

	GameManager::PlayerId friendBlazeId = 0;

	for(; pIter != pEnd; pIter++)
	{
		if (pIter != playerIter)
		{
			friendBlazeId = pIter->first;
		}
	}
	eastl::string assetName;

	//TODO: maybe a better way to find the client platform is available (i.e. like PLATFORM in config files)
	const char* platform = "undefined";
	const char8_t* serviceName = gController->getDefaultServiceName();
	if (NULL != serviceName)
	{
		const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(serviceName);
		if (NULL != serviceNameInfo)
		{
			platform = ClientPlatformTypeToString(serviceNameInfo->getPlatform());
		}
	}

	assetName.append_sprintf("fifa_online_friendlies_%s_%" PRId64 ".bin", platform, friendBlazeId);
	return assetName;	
}

FifaH2HFriendliesReportSlave::XmsRequestMetadata FifaH2HFriendliesReportSlave::getXmsRequestMetadata(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter)
{
	XmsRequestMetadata metadataCollection;

	AccountId playerOneNucleusId = playerIter->second->getNucleusId();
	AccountId friendNucleusId = 0;
	FriendNucleusIdMap::iterator it = mNucleausIdMap.find(playerIter->first);
	if (it != mNucleausIdMap.end())
	{
		friendNucleusId = it->second;
	}

	Blaze::XmsHd::XmsAttributes* metaData = BLAZE_NEW Blaze::XmsHd::XmsAttributes;
	metaData->setName("Friendlies");
	metaData->setType("string");
	metaData->setValue("Fifa16");
	metaData->setSearchable(true);
	metadataCollection.push_back(metaData);

	eastl::string metadataValue;
	metadataValue.append_sprintf("%" PRId64 , playerOneNucleusId);

	metaData = BLAZE_NEW Blaze::XmsHd::XmsAttributes;
	metaData->setName("Player1");
	metaData->setType("long");
	metaData->setValue(metadataValue.c_str());
	metaData->setSearchable(true);
	metadataCollection.push_back(metaData);

	metadataValue.clear();
	metadataValue.append_sprintf("%" PRId64 , friendNucleusId);
	metaData = BLAZE_NEW Blaze::XmsHd::XmsAttributes;
	metaData->setName("Player2");
	metaData->setType("long");
	metaData->setValue(metadataValue.c_str());
	metaData->setSearchable(true);
	metadataCollection.push_back(metaData);

	return metadataCollection;
}

const char8_t* FifaH2HFriendliesReportSlave::getXmsDataRootElementName()
{
	return "friendliesdata";
}

bool FifaH2HFriendliesReportSlave::sendXmsDataForPlayer(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator pIter, pEnd;
	pIter = OsdkPlayerReportsMap.begin();
	pEnd = OsdkPlayerReportsMap.end();

	GameManager::PlayerId friendBlazeId = 0;

	for(; pIter != pEnd; pIter++)
	{
		if (pIter != playerIter)
		{
			friendBlazeId = pIter->first;
		}
	}

	return playerIter->first < friendBlazeId;
}

}   // namespace GameReporting

}   // namespace Blaze

