/*************************************************************************************************/
/*!
    \file   gameclubreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECLUBREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECLUBREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameotpreportslave.h"
#include "osdk/tdf/gameosdkclubreport.h"

#include "clubs/tdf/clubs_base.h"
#include "osdkseasonalplay/osdkseasonalplayslaveimpl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameClubReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class GameClubReportSlave: public GameOTPReportSlave
{
public:
    GameClubReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameClubReportSlave();

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
    typedef eastl::hash_map<Clubs::ClubId, OSDKSeasonalPlay::SeasonState> ClubSeasonStateMap;
    typedef eastl::hash_map<Clubs::ClubId, Stats::ScopeNameValueMap*> ClubScopeIndexMap;

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameCommonReportSlave class.
    ********************************************************************************/
        virtual void initGameParams();
        virtual void determineGameResult();
        virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updatePlayerKeyscopes();
        virtual void updatePlayerStats();
        virtual void updatePlayerStatsConclusionType() {};
        virtual bool processUpdatedStats();
        virtual void updateNotificationReport() {};
        virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds) {};

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomOTPGameTypeName() const;
        virtual const char8_t* getCustomOTPStatsCategoryName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
		virtual void updateClubFreeAgentPlayerStats(){};

    /*! ****************************************************************************/
    /*! \Protected functions from GameClubReportSlave class.
    ********************************************************************************/
        bool initClubUtil();    
        void determineWinClub();

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubReportSalve, which needs to be implemented by custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomClubGameTypeName() const = 0;
    /* A custom hook for Club Rival feature that game team needs to update winner club player's stats based on challenge type */ 
        virtual void updateCustomClubRivalChallengeWinnerPlayerStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport) = 0;
    /* A custom hook for Club Rival feature that game team needs to update winner club's stats based on challenge type */ 
        virtual void updateCustomClubRivalChallengeWinnerClubStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport) = 0;

    /*! ****************************************************************************/
    /*! \Virtual functions provided by GameClubReportSlave that can be overwritten with custom functionalities
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerKeyscopes() {};
        virtual void updateCustomClubKeyscopes() {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomClubFreeAgentPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
    /* A custom hook for game team to process for club records based on game report and cache the result in mNewRecordArray, called during updateClubStats() */
        virtual bool updateCustomClubRecords(int32_t iClubIndex, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap) {return true;};
    /* A custom hook for game team to give club awards based on game report, called during updateClubStats() */
        virtual bool updateCustomClubAwards(Clubs::ClubId clubId,  OSDKClubGameReportBase::OSDKClubClubReport& clubReport, OSDKGameReportBase::OSDKReport& OsdkReport) {return true;};
    /* A custom hook for game team to preform custom process for post stats update, ie. awards based on win streak*/
        virtual bool processCustomUpdatedStats() {return true;};
    /* A custom hook for game team to update custom club stats per club, called during updateClubStats() */
        virtual void updateCustomClubStatsPerClub(Clubs::ClubId clubId,  OSDKClubGameReportBase::OSDKClubClubReport& clubReport) {};
    /* A 'catch-all' custom hook for game team to update custom stats, after each club has been triggered with updateCustomClubStatsPerClub during updateClubStats() */
        virtual void updateCustomClubStats(OSDKGameReportBase::OSDKReport& OsdkReport) {};
        virtual void setCustomClubLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
        virtual void setCustomSeasonalClubLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
        virtual void updateCustomTransformStats() {};
        virtual void updateCustomTransformSeasonalClubStats() {};


    Clubs::ClubId mWinClubId;
    Clubs::ClubId mLossClubId;
    bool          mClubGameFinish;
    int32_t       mLowestClubScore;
    int32_t       mHighestClubScore;

    // Club post stats update processing helper functions
    bool updateClubRecords(GameManager::PlayerId playerId, int32_t iMaxStatPlayer, int32_t iRecordID);

    uint32_t calcClubDampingPercent(Clubs::ClubId clubId);
    void checkPlayerClubMembership();

    ClubSeasonStateMap mSeasonStateMap;
    ClubScopeIndexMap mClubRankingKeyscopes;
    ClubScopeIndexMap mClubSeasonalKeyscopes;

    // Useful when overriding process(). Game teams may wish to call into these directly. 
    void updateClubKeyscopes();
    bool updateClubStats();

	using GameOTPReportSlave::transformStats;
	BlazeRpcError transformStats();


private:

    /*! ****************************************************************************/
    /*! \Private helper functions.
    ********************************************************************************/
    void adjustClubScore();
    Clubs::ClubId getClubFromPlayer(GameManager::PlayerId playerID);

    // Club transform stats
    void transformSeasonalClubStats();

    // Club Rival helper functions
    void updateClubRivalChallengeClubStats(uint32_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport);
    void updateClubRivalChallengePlayerStats(uint32_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport);
    void updateClubRivalStats();

    // Club Seasonal helper functions
    void determineClubSeasonState();
    void updateClubSeasonalStats();
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECLUBREPORT_SLAVE_H
