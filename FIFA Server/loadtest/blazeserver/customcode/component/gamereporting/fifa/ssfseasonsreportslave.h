/*************************************************************************************************/
/*!
    \file   ssfeasonsreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_SSFSEASONSREPORT_SLAVE_H
#define BLAZE_CUSTOM_SSFSEASONSREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/ssfbasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
namespace FIFA
{

/*!
    class SSFSeasonsReportSlave
*/
class SSFSeasonsReportSlave: public SSFBaseReportSlave
{
public:
    SSFSeasonsReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~SSFSeasonsReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from SSFBaseReportSlave class.
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
		virtual void updateNotificationReport();

		void updateCleanSheets(uint32_t pos, int64_t entityId, SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport);
		void updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport);
		void updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport);
		
		void updateGameStats();

private:

		int32_t GetTeamGoals(SSFSeasonsReportBase::GoalEventVector& goalEvents, int64_t teamId) const;
		bool isManOfMatchOnTheTeam(SSFSeasonsReportBase::SSFTeamReport& teamReport) const;

		FifaLastOpponent		mFifaLastOpponent;
		HeadToHeadLastOpponent	mH2HLastOpponentExtension;
};

}   // namespace FIFA 
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_SSFSEASONSREPORT_SLAVE_H

