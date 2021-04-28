/*************************************************************************************************/
/*!
    \file   gameclubintrah2hreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECLUBINTRAH2HREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECLUBINTRAH2HREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gamecommonreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameClubIntraH2HReportSlave
*/
class GameClubIntraH2HReportSlave: public GameCommonReportSlave
{
public:
    GameClubIntraH2HReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameClubIntraH2HReportSlave();

    /*! ****************************************************************************/
    /*! \brief Implementation performs simple validation and if necessary modifies 
            the game report.
 
        On success report may be submitted to master for collation or direct to 
        processing for offline or trusted reports. Behavior depends on the calling RPC.

        \param report Incoming game report from submit request
        \return ERR_OK on success. GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError validate(GameReport& report) const;
 
    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.
 
        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);
 
    /*! ****************************************************************************/
    /*! \brief Triggered on server reconfiguration.
    ********************************************************************************/
    virtual void reconfigure() const;

protected:

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameCommonReportSlave class.
    ********************************************************************************/
    virtual bool skipProcessConclusionType();
    virtual void initGameParams() { initCustomGameParams(); };
    virtual void getGameModeGameHistoryQuery(char8_t *query, int32_t querySize);
    virtual void determineGameResult();
    virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updatePlayerKeyscopes();
    virtual void updatePlayerStats();
    virtual void updatePlayerStatsConclusionType() {};
    virtual void updateGameStats() {};
    virtual bool processUpdatedStats();
    virtual void updateNotificationReport() { updateCustomNotificationReport(); };
    virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds);
    virtual void transformStats(const char8_t* statsCategory);


    /*! ****************************************************************************/
    /*! \Virtual functions to be implemented by game custom code.
    ********************************************************************************/
    virtual const char8_t* getCustomClubIntraH2HGameTypeName() const { return "gameType14"; };
    virtual void initCustomGameParams() {};
    virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
    virtual void updateCustomNotificationReport() {};
    virtual bool processCustomUpdatedStats() { return true; };
    virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName) { return false; };

    /*! ****************************************************************************/
    /*! \Helper functions.
    ********************************************************************************/
    void aggregatePlayerStats();

    static const char8_t *AGGREGATE_SCORE;
    AggregatePlayerStatsMap mAggregatedPlayerStatsMap;

    Utilities::ReportParser *mReportParser;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECLUBINTRAH2HREPORT_SLAVE_H
