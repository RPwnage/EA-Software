/*************************************************************************************************/
/*!
    \file   fifah2hcupsreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFAH2HCUPSREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFAH2HCUPSREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifacustom.h"
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"

#include "gamereporting/fifa/fifah2hbasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaH2HCupsReportSlave
*/
class FifaH2HCupsReportSlave: public FifaH2HBaseReportSlave
{
public:
    FifaH2HCupsReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaH2HCupsReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const;
        virtual const char8_t* getCustomH2HStatsCategoryName() const;
		virtual const uint16_t getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
		/* A custom hook  to set win/draw/loose in gamereport */	
		virtual void updatePlayerStats(); 
        virtual void updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport);
        virtual void updateCustomNotificationReport(const char8_t* statsCategory);
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true;};
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};

private:

	FifaSeasonalPlay		mSeasonalPlay;
	HeadtoHeadExtension		mSeasonalPlayExtension;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFAH2HCUPSREPORT_SLAVE_H

