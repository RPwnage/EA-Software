/*************************************************************************************************/
/*!
    \file   gamecustomclubchampreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMCLUBCHAMPREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMCLUBCHAMPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameclubchampreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomClubChampReportSlave
*/
class GameCustomClubChampReportSlave: public GameClubChampReportSlave
{
public:
    GameCustomClubChampReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomClubChampReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Virtual functions from GameClubChampReportSlave, which can be implemented by custom class.
    ********************************************************************************/
    virtual const char8_t* getCustomClubChampGameTypeName() const;
    virtual void initCustomGameParams() {};
    virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
    virtual void updateCustomNotificationReport() {};
    virtual bool processCustomUpdatedStats() { return true; };
    virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};    
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMCLUBCHAMPREPORT_SLAVE_H

