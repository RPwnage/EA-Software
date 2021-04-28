/*************************************************************************************************/
/*!
    \file   gamecustomh2hreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomh2hreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomH2HReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomH2HReportSlave::GameCustomH2HReportSlave(GameReportingSlaveImpl& component) :
GameH2HReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomH2HReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomH2HReportSlave::~GameCustomH2HReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomH2HReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomH2HReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomH2HReportSlave") GameCustomH2HReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head game used in gamereporting.cfg

    \return - the H2H game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* GameCustomH2HReportSlave::getCustomH2HGameTypeName() const
{
    return "gameType0";
}

/*************************************************************************************************/
/*! \brief getCustomH2HStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for H2H game
*/
/*************************************************************************************************/
const char8_t* GameCustomH2HReportSlave::getCustomH2HStatsCategoryName() const
{
    return "NormalGameStats";
}

/*************************************************************************************************/
/*! \brief updateCustomPlayerStats
    Update custom stats that are regardless of the game result
*/
/*************************************************************************************************/
void GameCustomH2HReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    // If the game is a regular H2H game, stats are already updated with the report parser.
}

/*************************************************************************************************/
/*! \brief updateCustomH2HSeasonalPlayerStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void GameCustomH2HReportSlave::updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateCustomPlayerStats(/*playerId*/0, playerReport);
}

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void GameCustomH2HReportSlave::updateCustomNotificationReport(const char8_t* statsCategory)
{
    // Obtain the custom data for report notification
    OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
    GameH2HReportBase::GameCustomData *gameCustomData = BLAZE_NEW GameH2HReportBase::GameCustomData();
    GameH2HReportBase::GameCustomData::PlayerCustomDataMap &playerCustomDataMap = gameCustomData->getPlayerCustomDataReports();

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
            int32_t overallSkillPoints = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, Stats::STAT_PERIOD_ALL_TIME, false));

            // Update the report notification custom data with the overall SkillPoints
            GameH2HReportBase::PlayerCustomData* playerCustomData = BLAZE_NEW GameH2HReportBase::PlayerCustomData();
            playerCustomData->setSkillPoints(overallSkillPoints);

            // Insert the player custom data into the player custom data map
            playerCustomDataMap.insert(eastl::make_pair(playerId, playerCustomData));
        }
    }

    // Set the gameCustomData to the OsdkReportNotification
    OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

}   // namespace GameReporting

}   // namespace Blaze

