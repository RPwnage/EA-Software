/*************************************************************************************************/
/*!
    \file   futh2horganizedtournamentreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "futh2horganizedtournamentreportslave.h"

#include "fifa/tdf/fifah2hreport.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FutH2HReportSlave
    Constructor
*/
/*************************************************************************************************/
FutH2HOrganizedTournamentReportSlave::FutH2HOrganizedTournamentReportSlave(GameReportingSlaveImpl& component) :
FifaH2HBaseReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief FutH2HReportSlave
    Destructor
*/
/*************************************************************************************************/
FutH2HOrganizedTournamentReportSlave::~FutH2HOrganizedTournamentReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FutH2HReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FutH2HOrganizedTournamentReportSlave::create(GameReportingSlaveImpl& component)
{
	TRACE_LOG("[FutH2HOrganizedTournamentReportSlave::Create]");
    return BLAZE_NEW_NAMED("FutH2HOrganizedTournamentReportSlave") FutH2HOrganizedTournamentReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head game used in gamereporting.cfg

    \return - the H2H game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FutH2HOrganizedTournamentReportSlave::getCustomH2HGameTypeName() const
{
    return "gameType70";
}

/*************************************************************************************************/
/*! \brief getCustomH2HStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for H2H game
*/
/*************************************************************************************************/
const char8_t* FutH2HOrganizedTournamentReportSlave::getCustomH2HStatsCategoryName() const
{
    return "NormalGameStats";
}

const uint16_t FutH2HOrganizedTournamentReportSlave::getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const
{
	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
	return h2hPlayerReport.getH2HCustomPlayerData().getControls();
}

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FutH2HOrganizedTournamentReportSlave::updateCustomNotificationReport(const char8_t* statsCategory)
{	
	TRACE_LOG("[FutH2HOrganizedTournamentReportSlave:" << this << "].updateCustomNotificationReport()");

	Fifa::H2HNotificationCustomGameData *gameCustomData = BLAZE_NEW Fifa::H2HNotificationCustomGameData();
	gameCustomData->setSpecial("TournamentMatch");
	gameCustomData->setGameReportingId(mProcessedReport->getGameReport().getGameReportingId());

    // Obtain the custom data for report notification
    OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

void FutH2HOrganizedTournamentReportSlave::updatePlayerStats()
{
	TRACE_LOG("[FutH2HOrganizedTournamentReportSlave:" << this << "].updatePlayerStats()");

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
				TRACE_LOG("[FutH2HOrganizedTournamentReportSlave:" << this << "].updateGameModeWinPlayerStats() - all players finished set WinnerByDnf to 0");
				playerReport.setWinnerByDnf(0);
			}
			else
			{
				TRACE_LOG("[FutH2HOrganizedTournamentReportSlave:" << this << "].updateGameModeWinPlayerStats() - all players did NOT finish set WinnerByDnf to 1");
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


}   // namespace GameReporting

}   // namespace Blaze

