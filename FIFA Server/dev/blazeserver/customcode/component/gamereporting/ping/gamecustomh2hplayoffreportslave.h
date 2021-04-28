/*************************************************************************************************/
/*!
    \file   gamecustomh2hplayoffreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMH2HPLAYOFFREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMH2HPLAYOFFREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameh2hplayoffreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomH2HPlayoffReportSlave
*/
class GameCustomH2HPlayoffReportSlave: public GameH2HPlayoffSlave
{
public:
    GameCustomH2HPlayoffReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomH2HPlayoffReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HPlayoffSlave, which has to be implemented by custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HPlayoffGameTypeName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions provided by GameH2HPlayoffSlave that can be overwritten with custom functionalities
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomNotificationReport() {};
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true; };
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMH2HREPORT_SLAVE_H

