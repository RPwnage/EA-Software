/*************************************************************************************************/
/*!
    \file   fifah2hcupsreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifah2hcupsreportslave.h"

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
FifaH2HCupsReportSlave::FifaH2HCupsReportSlave(GameReportingSlaveImpl& component) :
FifaH2HBaseReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaH2HCupsReportSlave::~FifaH2HCupsReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaH2HCupsReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaH2HCupsReportSlave") FifaH2HCupsReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head game used in gamereporting.cfg

    \return - the H2H game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FifaH2HCupsReportSlave::getCustomH2HGameTypeName() const
{
    return "gameType1";
}

/*************************************************************************************************/
/*! \brief getCustomH2HStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for H2H game
*/
/*************************************************************************************************/
const char8_t* FifaH2HCupsReportSlave::getCustomH2HStatsCategoryName() const
{
    return "NormalGameStats";
}

const uint16_t FifaH2HCupsReportSlave::getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const
{
	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
	return h2hPlayerReport.getH2HCustomPlayerData().getControls();
}

/*************************************************************************************************/
/*! \brief updateCustomPlayerStats
    Update custom stats that are regardless of the game result
*/
/*************************************************************************************************/
void FifaH2HCupsReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
//	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
}


void FifaH2HCupsReportSlave::updatePlayerStats()
{
	TRACE_LOG("[FifaH2HReportSlave:" << this << "].updatePlayerStats()");

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
/*! \brief updateCustomH2HSeasonalPlayerStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void FifaH2HCupsReportSlave::updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateCustomPlayerStats(/*playerId*/0, playerReport);
}

/*************************************************************************************************/
/*! \brief updateCustomH2HSeasonalPlayerStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void FifaH2HCupsReportSlave::updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport)
{
	HeadtoHeadExtension::ExtensionData data;
	data.mProcessedReport = mProcessedReport;
	data.mTieSet = &mTieSet;
	data.mWinnerSet = &mWinnerSet;
	data.mLoserSet = &mLoserSet;
	data.mTieGame = mTieGame;

	mSeasonalPlayExtension.setExtensionData(&data);
	mSeasonalPlay.setExtension(&mSeasonalPlayExtension);
	mSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mSeasonalPlay.updateCupStats();
}

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FifaH2HCupsReportSlave::updateCustomNotificationReport(const char8_t* statsCategory)
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

/*************************************************************************************************/
/*! \brief setCustomEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FifaH2HCupsReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
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
}   // namespace GameReporting

}   // namespace Blaze

