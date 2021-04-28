/*************************************************************************************************/
/*!
    \file   gamecustomh2hreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMH2HREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMH2HREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/ping/gamecustom.h"
#include "gamereporting/osdk/gameh2hreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomH2HReportSlave
*/
class GameCustomH2HReportSlave: public GameH2HReportSlave
{
public:
    GameCustomH2HReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomH2HReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const;
        virtual const char8_t* getCustomH2HStatsCategoryName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport) {};
        virtual void updateCustomNotificationReport(const char8_t* statsCategory);
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true;};
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMH2HREPORT_SLAVE_H

