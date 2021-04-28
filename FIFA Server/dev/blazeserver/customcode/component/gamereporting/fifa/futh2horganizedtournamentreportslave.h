/*************************************************************************************************/
/*!
    \file   futh2horganizedtournamentreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FUTH2HORGANIZEDTOURNAMENT_REPORT_SLAVE_H
#define BLAZE_CUSTOM_FUTH2HORGANIZEDTOURNAMENT_REPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifacustom.h"
#include "gamereporting/fifa/fifah2hbasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FutH2HOrganizedTournamentReportSlave
*/
class FutH2HOrganizedTournamentReportSlave: public FifaH2HBaseReportSlave
{
public:
	FutH2HOrganizedTournamentReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FutH2HOrganizedTournamentReportSlave();

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
		virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
		virtual void updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
		virtual void updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport) {};
        virtual void updateCustomNotificationReport(const char8_t* statsCategory);
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true;};
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
	/* A custom hook  to set win/draw/loose in gamereport */	
		virtual void updatePlayerStats();  
	/* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
		//virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName) { return false; };
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};

		virtual void selectCustomStats() {};
		virtual void updateCustomTransformStats(const char8_t* statsCategory) {};
		virtual bool skipProcess() { return false; }; //always handle the report
		virtual bool wasDNFPlayerWinning() { return false; } //always handle the report

private:
	virtual bool							sendXmsDataForPlayer(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter) { return false; };
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FUTH2HORGANIZEDTOURNAMENT_REPORT_SLAVE_H

