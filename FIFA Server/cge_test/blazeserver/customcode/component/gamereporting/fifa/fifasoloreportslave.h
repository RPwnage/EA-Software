/*************************************************************************************************/
/*!
    \file   fifasoloreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFASOLOREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFASOLOREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gamesoloreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaSoloReportSlave
*/
class FifaSoloReportSlave: public GameSoloReportSlave
{
public:
    FifaSoloReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaSoloReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameSoloReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomSoloGameTypeName() const;
        virtual const char8_t* getCustomSoloStatsCategoryName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
		virtual void initCustomGameParams() {};
		virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
		virtual void updateCustomNotificationReport();
		/* A custom hook for Game team to perform post stats update processing */
		virtual bool processCustomUpdatedStats() { return true; };
		/* A custom hook for Game team to add additional player keyscopes needed for stats update */
		virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
		/* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
		//virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFAH2HREPORT_SLAVE_H

