/*************************************************************************************************/
/*!
    \file   gameOTPreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMEOTPREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMEOTPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/util/updateclubsutil.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/osdk/gamecommonreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameOTPReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class GameOTPReportSlave: public GameCommonReportSlave
{
public:
    GameOTPReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameOTPReportSlave();

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
        virtual void getGameModeGameHistoryQuery(char8_t *query, int32_t querySize) {};
        virtual void determineGameResult();
        virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updatePlayerKeyscopes();
        virtual void updatePlayerStats();
        virtual void updatePlayerStatsConclusionType();
        virtual void updateGameStats() {};
        virtual bool processUpdatedStats();
		virtual bool processPostStatsCommit() { return true; };
        virtual void updateNotificationReport() {};
        virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds);
		virtual void CustomizeGameReportForEventReporting(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds) {};

    /*! ****************************************************************************/
    /*! \Pure virtual functions to be implemented by each game team's game mode custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomOTPGameTypeName() const = 0;
        virtual const char8_t* getCustomOTPStatsCategoryName() const = 0;

    /*! ****************************************************************************/
    /*! \Virtual functions game team can overwrite with custom functionalities
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerKeyscopes() {};
        virtual void updateCustomClubFreeAgentPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
    /* A custom hook for game team to preform custom process for post stats update, return true if process success */
        virtual bool processCustomUpdatedStats() {return true;};
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
		virtual void updateClubFreeAgentPlayerStats(){};
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
       // virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName) { return false;};
    /* A custom hook for Game team to perform custom transform stats */
        virtual void updateCustomTransformStats(const char8_t* statsCategory) {};

	/* Custom hoom similar to club processing for game team to perform custom transform stats */
		virtual void updateCustomTransformStats() {};

        virtual BlazeRpcError transformStats(const char8_t* statsCategory);




    /*! ****************************************************************************/
    /*! \Protected functions
    ********************************************************************************/
        void aggregatePlayerStats();

    static const char8_t *AGGREGATE_SCORE;
    AggregatePlayerStatsMap mAggregatedPlayerStatsMap;

    Utilities::ReportParser *mReportParser;

    UpdateClubsUtil mUpdateClubsUtil;
    PlayerScopeIndexMap mFreeAgentPlayerKeyscopes;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMEOTPREPORT_SLAVE_H
