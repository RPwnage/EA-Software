/*************************************************************************************************/
/*!
    \file   futsoloreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FUTSOLOREPORT_SLAVE_H
#define BLAZE_CUSTOM_FUTSOLOREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifasoloreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
class FifaHMAC;

/*!
    class FifaSoloReportSlave
*/
class FutSoloReportSlave: public FifaSoloReportSlave
{
public:
    FutSoloReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FutSoloReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameSoloReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomSoloGameTypeName() const;
        virtual const char8_t* getCustomSoloStatsCategoryName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
		virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

		virtual void initCustomGameParams() {};
		virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
		virtual void updateCustomNotificationReport();
		/* A custom hook for Game team to perform post stats update processing */
		virtual bool processCustomUpdatedStats() { return true; };
		/* A custom hook for Game team to add additional player keyscopes needed for stats update */
		virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
		/* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
		//virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);

	private:
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FUTSOLOREPORT_SLAVE_H

