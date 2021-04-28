/*************************************************************************************************/
/*!
    \file   gamecustomclubplayoffreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECUSTOMCLUBPLAYOFFREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECUSTOMCLUBPLAYOFFREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameclubplayoffreportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCustomClubPlayoffReportSlave
*/
class GameCustomClubPlayoffReportSlave: public GameClubPlayoffReportSlave
{
public:
    GameCustomClubPlayoffReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCustomClubPlayoffReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameClubPlayoffReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomClubPlayoffGameTypeName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual bool processCustomUpdatedStats();

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECUSTOMCLUBPLAYOFFREPORT_SLAVE_H

