/*************************************************************************************************/
/*!
    \file   fifacoopseaosnscupreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFACOOPSEASONSCUPREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFACOOPSEASONSCUPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplayextensions.h"

#include "fifa/fifacoopbasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
namespace FIFA
{

/*!
    class FifaCoopSeasonsCupReportSlave
*/
class FifaCoopSeasonsCupReportSlave: public FifaCoopBaseReportSlave
{
public:
    FifaCoopSeasonsCupReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaCoopSeasonsCupReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomSquadGameTypeName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions game team can overwrite with custom functionalities
    ********************************************************************************/
    /* A custom hook for game team to preform custom process for post stats update, ie. awards based on win streak*/
        virtual bool processCustomUpdatedStats();

    /*! ****************************************************************************/
    /*! \Virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);

		virtual void selectCustomStats();
		virtual void updateCustomTransformStats();

		virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);

		void updateCleanSheets(uint32_t pos, CoopSeason::CoopId coopId, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport);
		void updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport);
		void updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport);
		
		uint32_t determineResult(CoopSeason::CoopId id, FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadDetailsReport);

private:

	FifaSeasonalPlay			mFifaSeasonalPlay;
	CoopSeasonsExtension		mFifaSeasonalPlayExtension;
};

}   // namespace FIFA 
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFACOOPSEASONSCUPREPORT_SLAVE_H

