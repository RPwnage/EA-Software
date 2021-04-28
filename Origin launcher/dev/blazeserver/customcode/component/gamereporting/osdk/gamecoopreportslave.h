/*************************************************************************************************/
/*!
    \file   gamecoopreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECOOPREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECOOPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameh2hreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCoopReportSlave
*/
class GameCoopReportSlave: public GameH2HReportSlave
{
public:
    GameCoopReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCoopReportSlave();

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

private:

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameCommonReportSlave class.
    ********************************************************************************/
    virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void updatePlayerKeyscopes() {};
    virtual void updatePlayerStats();
    virtual void updatePlayerStatsConclusionType();
    virtual void updateNotificationReport() {};
    virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds) {};

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportSlave, which has to be implemented by custom class.
    TODO: Coop Game Reporting needs to rework to support the custom game reporting class like H2H
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const {return "gameType3";};
        virtual const char8_t* getCustomH2HStatsCategoryName() const {return "CoopGameStats";};

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECOOPREPORT_SLAVE_H
