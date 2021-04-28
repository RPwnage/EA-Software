/*************************************************************************************************/
/*!
    \file   gamecustomclubintrah2hreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMCLUBINTRAH2HREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMCLUBINTRAH2HREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameclubintrah2hreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomClubIntraH2HReportSlave
*/
class GameCustomClubIntraH2HReportSlave: public GameClubIntraH2HReportSlave
{
public:
    GameCustomClubIntraH2HReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomClubIntraH2HReportSlave();

protected:
    virtual const char8_t* getCustomClubIntraH2HGameTypeName() const;
    virtual void initCustomGameParams();
    virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
    virtual void updateCustomNotificationReport() {};
    virtual bool processCustomUpdatedStats() { return true; };
    virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMH2HREPORT_SLAVE_H

