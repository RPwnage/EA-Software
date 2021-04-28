/*************************************************************************************************/
/*!
    \file   gametournamentreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMETOURNAMENTREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMETOURNAMENTREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameh2hreportslave.h"
#include "gamereporting/osdk/osdktournamentsutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameTournamentReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class GameTournamentReportSlave: public GameH2HReportSlave
{
public:
    GameTournamentReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameTournamentReportSlave();

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


    OSDKTournamentsUtil mTournamentsUtil;

protected:
    /*! ****************************************************************************/
    /*! \Protected helpers. Game teams may find these useful if they have overridden
        a virtual method like process that previously called into them. 
    ********************************************************************************/
    /*! updateTournamentStats() is called by process already (best used if overriding
        that function)
    ********************************************************************************/
    void updateTournamentStats();

private:

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
        virtual bool processUpdatedStats();
        virtual void updateNotificationReport();

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportSlave, which has to be implemented by custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const {return getCustomTournamentGameTypeName();};
        virtual const char8_t* getCustomH2HStatsCategoryName() const {return getCustomTournamentStatsCategoryName();};

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameTournamentReportSlave, which has to be implemented by custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomTournamentGameTypeName() const = 0;
        virtual const char8_t* getCustomTournamentStatsCategoryName() const = 0;
    /* Fill in the tournament Id parameter and return true if it's a valid tournament Id */
        virtual bool getCustomTournamentId(OSDKGameReportBase::OSDKGameReport& osdkGameReport, OSDKTournaments::TournamentId& tournamentId) = 0;
    /* Update the tournament custom team stats that depends on the game custom game report */
        virtual void updateCustomTournamentTeamStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap) = 0;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameTournamentReportSlave, which is optional to be overwritten by custom class.
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomTournamentStats(OSDKTournaments::TournamentId& tournamentId, Blaze::BlazeId& userId0, Blaze::BlazeId& userId1, 
            uint32_t& team0, uint32_t& team1, uint32_t& score0, uint32_t& score1, char8_t* metadata0, char8_t* metadata1) {};
    /* A custom hook to allow game team to process custom stats update and return false if process failed and should stop further game report processing */
        virtual bool processCustomUpdatedStats() { return true; };

    /*! ****************************************************************************/
    /*! \Private helper functions.
    ********************************************************************************/
        void updateTournamentTeamStats();
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMETOURNAMENTREPORT_SLAVE_H
