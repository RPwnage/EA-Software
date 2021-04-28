/*************************************************************************************************/
/*!
    \file   fifacoopreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFACOOPREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFACOOPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameotpreportslave.h"
//#include "osdk/tdf/gameosdkclubreport.h"
#include "fifa/tdf/fifacoopreport.h"

//#include "clubs/tdf/clubs_base.h"
#include "coopseason/tdf/coopseasontypes_base.h"
#include "osdkseasonalplay/osdkseasonalplayslaveimpl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define CATEGORYNAME_COOP_RANKING	"CoopRankStats"
#define QUERY_COOPGAMESTATS			"coop_skill_damping_query"

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaCoopReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class FifaCoopReportSlave: public GameOTPReportSlave
{
public:
    FifaCoopReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaCoopReportSlave();

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
    //typedef eastl::hash_map<Clubs::ClubId, OSDKSeasonalPlay::SeasonState> ClubSeasonStateMap;
    //typedef eastl::hash_map<Clubs::ClubId, Stats::ScopeNameValueMap*> ClubScopeIndexMap;

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

    ///*! ****************************************************************************/
    ///*! \Virtual functions from GameOTPReportSlave class.
    //********************************************************************************/
    //    virtual void updateClubFreeAgentPlayerStats();

    /*! ****************************************************************************/
    /*! \Protected functions from FifaCoopReportSlave class.
    ********************************************************************************/
        //bool initClubUtil();    
        void determineWinSquad();

    /*! ****************************************************************************/
    /*! \Pure virtual functions from FifaCoopReportSalve, which needs to be implemented by custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomSquadGameTypeName() const = 0;
 
    /*! ****************************************************************************/
    /*! \Virtual functions provided by FifaCoopReportSlave that can be overwritten with custom functionalities
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerKeyscopes() {};
        virtual void updateCustomSquadKeyscopes() {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
    /* A custom hook for game team to preform custom process for post stats update, ie. awards based on win streak*/
        virtual bool processCustomUpdatedStats() {return true;};
    /* A custom hook for game team to update custom club stats per club, called during updateClubStats() */
		virtual void updateCustomSquadStatsPerSquad(CoopSeason::CoopId clubId,  FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadReport) {};
    /* A 'catch-all' custom hook for game team to update custom stats, after each club has been triggered with updateCustomClubStatsPerClub during updateClubStats() */
        virtual void updateCustomSquadStats(OSDKGameReportBase::OSDKReport& OsdkReport) {};
        virtual void updateCustomTransformStats() {};


	CoopSeason::CoopId	mWinCoopId;
    CoopSeason::CoopId	mLossCoopId;
    bool				mCoopGameFinish;
    int32_t				mLowestCoopScore;
    int32_t				mHighestCoopScore;

	bool				mIsHomeCoopSolo;
	Blaze::BlazeId		mHomeUpperBlazeId;
	Blaze::BlazeId				mHomeLowerBlazeId;

	bool				mIsAwayCoopSolo;
	Blaze::BlazeId		mAwayUpperBlazeId;
	Blaze::BlazeId		mAwayLowerBlazeId;

	uint32_t calcSquadDampingPercent(CoopSeason::CoopId coopId);

    // Useful when overriding process(). Game teams may wish to call into these directly. 
    void updateSquadKeyscopes();
    bool updateSquadStats();

	using GameOTPReportSlave::transformStats;
    BlazeRpcError transformStats();

private:

    /*! ****************************************************************************/
    /*! \Private helper functions.
    ********************************************************************************/
    void adjustSquadScore();
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFACOOPREPORT_SLAVE_H
