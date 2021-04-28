/*************************************************************************************************/
/*!
    \file   fifacoopseasonsreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFACOOPSEASONSREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFACOOPSEASONSREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifacoopbasereportslave.h"
#include "gamereporting/fifa/fifacustomclub.h"

#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplayextensions.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskill.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskillextensions.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
namespace FIFA
{

/*!
    class FifaCoopSeasonsReportSlave
*/
class FifaCoopSeasonsReportSlave: public FifaCoopBaseReportSlave
{
public:
    FifaCoopSeasonsReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaCoopSeasonsReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomSquadGameTypeName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);

		virtual void selectCustomStats();
		virtual void updateCustomTransformStats();

		virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
		virtual void updateCustomSquadStats(OSDKGameReportBase::OSDKReport& OsdkReport);

		void updateSoloStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
		void writeSoloStats(const char* statsCategory, GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport, bool isLowerBlazeId);

		void updateCleanSheets(uint32_t pos, CoopSeason::CoopId coopId, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport);
		void updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport);
		void updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport);
		
		uint32_t determineResult(CoopSeason::CoopId id, FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadDetailsReport);

private:

	FifaEloRpgHybridSkill		mEloRpgHybridSkill;
	CoopEloExtension			mEloRpgHybridSkillExtension;

	FifaSeasonalPlay			mFifaSeasonalPlay;
	CoopSeasonsExtension		mFifaSeasonalPlayExtension;
};

}   // namespace FIFA 
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFACOOPSEASONSREPORT_SLAVE_H

