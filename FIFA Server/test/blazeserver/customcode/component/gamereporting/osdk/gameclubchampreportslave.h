/*************************************************************************************************/
/*!
    \file   gameclubchampreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECLUBCHAMPREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECLUBCHAMPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/util/updateclubsutil.h"
#include "gamereporting/osdk/gameclub.h"
#include "gamereporting/osdk/gameh2hreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameClubChampReportSlave
*/
class GameClubChampReportSlave: public GameH2HReportSlave
{
public:
    GameClubChampReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameClubChampReportSlave();

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
    virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updatePlayerKeyscopes();
    virtual void updatePlayerStats();
    virtual void updatePlayerStatsConclusionType() {};
    virtual void updateNotificationReport() { updateCustomNotificationReport(); };
    virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds);

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportSlave, which has to be implemented by sub class.
    ********************************************************************************/
    virtual const char8_t* getCustomH2HGameTypeName() const { return "gameType11"; };
    virtual const char8_t* getCustomH2HStatsCategoryName() const {return "";};

    /*! ****************************************************************************/
    /*! \Virtual functions to be implemented by game custom code.
    ********************************************************************************/
    virtual const char8_t* getCustomClubChampGameTypeName() const {return "gameType11";};
    virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
	using GameH2HReportSlave::updateCustomNotificationReport;
    virtual void updateCustomNotificationReport() {};
    //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName) { return false; };

    /*! ****************************************************************************/
    /*! \Helper functions.
    ********************************************************************************/
    void updateClubChampStats();

    UpdateClubsUtil mUpdateClubsUtil;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECLUBCHAMPREPORT_SLAVE_H
