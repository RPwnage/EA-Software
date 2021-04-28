/*************************************************************************************************/
/*!
    \file   fifaclubclubreplortslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFACLUBCUPREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFACLUBCUPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifaclubbasereportslave.h"
#include "gamereporting/fifa/fifacustomclub.h"

#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplayextensions.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskill.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskillextensions.h"
#include "gamereporting/fifa/fifa_vpro/fifa_vpro.h"
#include "gamereporting/fifa/fifa_vpro/fifa_vproobjectivestatsupdater.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
namespace FIFA
{

/*!
    class FifaClubCupReportSlave
*/
class FifaClubCupReportSlave: public FifaClubBaseReportSlave
{
public:
    FifaClubCupReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaClubCupReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomClubGameTypeName() const;
    /* A custom hook for Club Rival feature that game team needs to update winner club player's stats based on challenge type */ 
        virtual void updateCustomClubRivalChallengeWinnerPlayerStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport);
    /* A custom hook for Club Rival feature that game team needs to update winner club's stats based on challenge type */ 
        virtual void updateCustomClubRivalChallengeWinnerClubStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport);


    /*! ****************************************************************************/
    /*! \Virtual functions game team can overwrite with custom functionalities
    ********************************************************************************/
    /* A custom hook for game team to process for club records based on game report and cache the result in mNewRecordArray, called during updateClubStats() */
        virtual bool updateCustomClubRecords(int32_t iClubIndex, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap);
    /* A custom hook for game team to give club awards based on game report, called during updateClubStats() */
        virtual bool updateCustomClubAwards(Clubs::ClubId clubId,  OSDKClubGameReportBase::OSDKClubClubReport& clubReport, OSDKGameReportBase::OSDKReport& OsdkReport);
    /* A custom hook for game team to preform custom process for post stats update, ie. awards based on win streak*/
        virtual bool processCustomUpdatedStats();

    /*! ****************************************************************************/
    /*! \Virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);

		virtual void selectCustomStats();
		virtual void updateCustomTransformStats();
		virtual bool processPostStatsCommit();

		virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);

		void updateCleanSheets(uint32_t pos, Clubs::ClubId clubId, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport);
		void updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport);
		void updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport);
		
		uint32_t determineResult(Clubs::ClubId id, OSDKClubGameReportBase::OSDKClubClubReport& clubReport);

private:

	FifaVpro					mFifaVpro;
	ClubsVproExtension			mFifaVproExtension;

	FifaSeasonalPlay			mFifaSeasonalPlay;
	ClubSeasonsExtension		mFifaSeasonalPlayExtension;

	FifaVProObjectiveStatsUpdater mFifaVproObjectiveStatsUpdater;
};

}   // namespace FIFA 
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFACLUBCUPREPORT_SLAVE_H

