/*************************************************************************************************/
/*!
    \file   gamecustomotpreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMOTPREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMOTPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameotpreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomOTPReportSlave
*/
class GameCustomOTPReportSlave: public GameOTPReportSlave
{
public:
    GameCustomOTPReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomOTPReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomOTPGameTypeName() const;
        virtual const char8_t* getCustomOTPStatsCategoryName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
        virtual void initCustomGameParams();
        virtual void updateCustomPlayerKeyscopes();
        virtual void updateCustomClubFreeAgentPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    /* A custom hook for game team to preform custom process for post stats update, return true if process success */
        virtual bool processCustomUpdatedStats() {return true;};
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMOTPREPORT_SLAVE_H
