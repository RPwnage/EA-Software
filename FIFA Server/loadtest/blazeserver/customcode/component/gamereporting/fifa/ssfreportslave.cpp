/*************************************************************************************************/
/*!
    \file   ssfreportslave.cpp


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
#include "ssfreportslave.h"

#include "osdk/tdf/gameosdkreport.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief SSFReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* SSFReportSlave::create(GameReportingSlaveImpl& component)
{
    // SSFReportSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief SSFReportSlave
    Constructor
*/
/*************************************************************************************************/
SSFReportSlave::SSFReportSlave(GameReportingSlaveImpl& component) :
    GameOTPReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief SSFReportSlave
    Destructor
*/
/*************************************************************************************************/
SSFReportSlave::~SSFReportSlave()
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
BlazeRpcError SSFReportSlave::validate(GameReport& report) const
{
    if (NULL == report.getReport())
    {
        WARN_LOG("[SSFReportSlave:" << this << "].validate() : NULL report.");
        return Blaze::ERR_SYSTEM; // EARLY RETURN
    }

    OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*report.getReport());   
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

    if (2 != ssfTeamReportMap.size())
    {
        WARN_LOG("[SSFReportSlave:" << this << "].validate() : There must have been exactly two squads in this game!");
        return Blaze::ERR_SYSTEM; // EARLY RETURN
    }

    return Blaze::ERR_OK;
}

/*******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void SSFReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief initGameParams
    Initialize game parameters that are needed for later processing

    \Customizable - Virtual function to override default behavior.
    \Customizable - initCustomGameParams() to provide additional custom behavior.
*/
/*************************************************************************************************/
void SSFReportSlave::initGameParams()
{
    OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

    mCategoryId = osdkReport.getGameReport().getCategoryId();
    mRoomId = osdkReport.getGameReport().getRoomId();

    // could be a tie game if there is more than one squad
    mTieGame = (ssfTeamReportMap.size() > 1);

    initCustomGameParams();
}

OSDKGameReportBase::OSDKReport& SSFReportSlave::getOsdkReport()
{
	return static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
}

/*************************************************************************************************/
/*! \brief determineGameResult
    Determine the game results

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void SSFReportSlave::determineGameResult()
{
	SSFGameResultHelper<SSFReportSlave>::DetermineGameResult();
}

/*************************************************************************************************/
/*! \brief updateSquadResults
	Update squad's game results

*/
/*************************************************************************************************/
void SSFReportSlave::updateSquadResults()
{
	SSFGameResultHelper<SSFReportSlave>::updateSquadResults();
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Update Player Keyscopes

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void SSFReportSlave::updatePlayerKeyscopes() 
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
				TRACE_LOG("[SSFReportSlave:" << this << "].updatePlayerKeyscopes() AccountCountry " << accountLocale << " for Player " << playerId);
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
void SSFReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
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
void SSFReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateWinPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeLossPlayerStats
    Update loser's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void SSFReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateLossPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeTiePlayerStats
    Update tied player's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void SSFReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void SSFReportSlave::updatePlayerStats()
{
    TRACE_LOG("[SSFReportSlave:" << this << "].updatePlayerStats()");

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
bool SSFReportSlave::processUpdatedStats()
{
    bool success = true;

    success = success ? processCustomUpdatedStats() : success;
	updateNotificationReport();

    return success;
}

/*************************************************************************************************/
/*! \brief updateSquadKeyscopes
    Set the value of squad keyscopes defined in config and needed for config based stat update

    \Customizable - updateCustomSquadKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void SSFReportSlave::updateSquadKeyscopes()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

	SSFSeasonsReportBase::SSFTeamReportMap::const_iterator squadIter, squadIterEnd;
    squadIter = ssfTeamReportMap.begin();
    squadIterEnd = ssfTeamReportMap.end();

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
bool SSFReportSlave::updateSquadStats()
{
    bool success = true;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

	SSFSeasonsReportBase::SSFTeamReportMap::const_iterator squadIter, squadIterEnd;
    squadIter = ssfTeamReportMap.begin();
    squadIterEnd = ssfTeamReportMap.end();

    for(int32_t i = 0; squadIter != squadIterEnd; ++squadIter, i++)
    {
		int64_t teamEntityId = squadIter->first;
		SSFSeasonsReportBase::SSFTeamReport& squadReportDetails = *squadIter->second;
        updateCustomSquadStatsPerSquad(teamEntityId, squadReportDetails);
    }

    updateCustomSquadStats(osdkReport);

    TRACE_LOG("[SSFReportSlave].updateSquadStats() Processing complete");

    return success;
}

/*************************************************************************************************/
/*! \brief transformStats
    calculate lobby skill and lobby skill points

    \Customizable - updateCustomTransformStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
BlazeRpcError SSFReportSlave::transformStats()
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
const char8_t* SSFReportSlave::getCustomOTPGameTypeName() const 
{ 
    return getCustomSquadGameTypeName(); 
};

/*************************************************************************************************/
/*! \brief getCustomOTPStatsCategoryName
    \Customizable - Virtual function.
*/
/*************************************************************************************************/
const char8_t* SSFReportSlave::getCustomOTPStatsCategoryName() const 
{ 
    // squad has it's own stats to update, doesn't need to update the OTP stats category
    return ""; 
};

}   // namespace GameReporting

}   // namespace Blaze
