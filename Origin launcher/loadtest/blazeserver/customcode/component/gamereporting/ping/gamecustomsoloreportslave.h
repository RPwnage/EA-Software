/*************************************************************************************************/
/*!
    \file   gamecustomsoloreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMSOLOREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMSOLOREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gamesoloreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomSoloReportSlave
*/
class GameCustomSoloReportSlave: public GameSoloReportSlave
{
public:
    GameCustomSoloReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomSoloReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Virtual functions provided by GameSoloReportSlave that can be overwritten with custom functionalities
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomNotificationReport() {};
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true; };
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};

		virtual const char8_t* getCustomSoloStatsCategoryName() const { return "NA";};
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMSOLOREPORT_SLAVE_H

