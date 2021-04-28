/*************************************************************************************************/
/*!
    \file   gamecustomtournamentreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMTOURNAMENTREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMTOURNAMENTREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gametournamentreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomTournamentReportSlave
*/
class GameCustomTournamentReportSlave: public GameTournamentReportSlave
{
public:
    GameCustomTournamentReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomTournamentReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomTournamentGameTypeName() const;
        virtual const char8_t* getCustomTournamentStatsCategoryName() const;
    /* Fill in the tournament Id parameter and return true if it's a valid tournament Id */
        virtual bool getCustomTournamentId(OSDKGameReportBase::OSDKGameReport& osdkGameReport, OSDKTournaments::TournamentId& tournamentId);
    /* Update the tournament custom team stats that depends on the game custom game report */
        virtual void updateCustomTournamentTeamStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap);

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomTournamentStats(OSDKTournaments::TournamentId& tournamentId, Blaze::BlazeId& userId0, Blaze::BlazeId& userId1, 
            uint32_t& team0, uint32_t& team1, uint32_t& score0, uint32_t& score1, char8_t* metadata0, char8_t* metadata1) {};
        virtual void updateCustomNotificationReport(const char8_t* statsCategory) {};
    /* A custom hook to allow game team to process custom stats update and return false if process failed and should stop further game report processing */
        virtual bool processCustomUpdatedStats();
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap);
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
     //   virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};

    static const int32_t MAX_TOURNAMENT_TEAM = 2;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMTOURNAMENTREPORT_SLAVE_H

