/*************************************************************************************************/
/*!
    \file   gameh2hreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMEH2HREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMEH2HREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gamecommonreportslave.h"
#include "gamereporting/osdk/gamecommonlobbyskill.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameH2HReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class GameH2HReportSlave: public GameCommonReportSlave
{
public:
    GameH2HReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameH2HReportSlave();

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
        virtual void initGameParams();
        virtual void getGameModeGameHistoryQuery(char8_t *query, int32_t querySize);
        virtual void determineGameResult();
        virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updatePlayerKeyscopes();
        virtual void updatePlayerStats();
        virtual void updatePlayerStatsConclusionType();
        virtual void updateGameStats();
        virtual bool processUpdatedStats();
        virtual void updateNotificationReport();
        virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds);

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportSlave, which has to be implemented by custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const = 0;
        virtual const char8_t* getCustomH2HStatsCategoryName() const = 0;
		virtual const uint16_t getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const { return 0; };

    /*! ****************************************************************************/
    /*! \Virtual functions provided by GameH2HReportSlave that can be overwritten with custom functionalities
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport) {};
        virtual void updateCustomNotificationReport(const char8_t* statsCategory) {};
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true; };
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
    //    virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName) { return false; };
    /* A custom hook for Game team to perform custom transform stats */
        virtual void updateCustomTransformStats(const char8_t* statsCategory) {};
        virtual BlazeRpcError transformStats(const char8_t* statsCategory);
        virtual void updateH2HSeasonalStats();


    /*! ****************************************************************************/
    /*! \Protected functions
    ********************************************************************************/        
        void aggregatePlayerStats();

    static const char8_t *AGGREGATE_SCORE;
    AggregatePlayerStatsMap mAggregatedPlayerStatsMap;

    Utilities::ReportParser *mReportParser;

private:

    /*! ****************************************************************************/
    /*! \Private helper functions.
    ********************************************************************************/

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMEH2HREPORT_SLAVE_H
