/*************************************************************************************************/
/*!
    \file   gameclubplayoffreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECLUBPLAYOFFREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECLUBPLAYOFFREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameclubreportslave.h"
#include "gamereporting/osdk/osdktournamentsutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameClubPlayoffReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class GameClubPlayoffReportSlave: public GameClubReportSlave
{
public:
    GameClubPlayoffReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameClubPlayoffReportSlave();

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
        virtual void getGameModeGameHistoryQuery(char8_t *query, int32_t querySize) {};
        virtual void determineGameResult();
        virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updatePlayerStats();
        virtual void updatePlayerStatsConclusionType() {};
        virtual bool processUpdatedStats();
        virtual void updateNotificationReport() {};

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomClubGameTypeName() const { return getCustomClubPlayoffGameTypeName(); };
        virtual void updateCustomClubRivalChallengeWinnerPlayerStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport) {};
        virtual void updateCustomClubRivalChallengeWinnerClubStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport) {};
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomClubEndGameMailParam(Mail::HttpParamList* mailParamList, const char8_t* mailTemplateName) {return false;};

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubPlayoffReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomClubPlayoffGameTypeName() const = 0;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameClubPlayoffReportSlave class.
    ********************************************************************************/
        virtual void updateCustomClubSeasonalStats() {};
        virtual void updateCustomPlayoffStats(OSDKTournaments::TournamentId tournamentId, Clubs::ClubId clubId0, Clubs::ClubId clubId1, uint32_t team0, uint32_t team1, 
            uint32_t& score0, uint32_t& score1, char8_t* metadata0, char8_t* metadata1) {};

    OSDKTournamentsUtil mTournamentsUtil;
    OSDKTournaments::TournamentId mTournamentId[MAX_CLUB_NUMBER];
    OSDKSeasonalPlay::SeasonState mClubSeasonState[MAX_CLUB_NUMBER];

private:

    /*! ****************************************************************************/
    /*! \Private helper functions.
    ********************************************************************************/
    void updateClubKeyscope();
    void updateClubStats();
    void updateClubSeasonalStats();
    void updatePlayoffStats(OSDKTournaments::TournamentId tournamentId);
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECLUBPLAYOFFREPORT_SLAVE_H
