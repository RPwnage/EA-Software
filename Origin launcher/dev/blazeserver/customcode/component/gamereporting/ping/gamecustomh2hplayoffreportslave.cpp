/*************************************************************************************************/
/*!
    \file   gamecustomh2hplayoffreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomh2hplayoffreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomH2HPlayoffReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomH2HPlayoffReportSlave::GameCustomH2HPlayoffReportSlave(GameReportingSlaveImpl& component) :
GameH2HPlayoffSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomH2HPlayoffReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomH2HPlayoffReportSlave::~GameCustomH2HPlayoffReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomH2HPlayoffReportSlave::create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomH2HPlayoffReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomH2HPlayoffReportSlave") GameCustomH2HPlayoffReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head playoff game used in gamereporting.cfg

    \return - the H2H Playoff game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* GameCustomH2HPlayoffReportSlave::getCustomH2HPlayoffGameTypeName() const
{
    return "gameType12";
}

}   // namespace GameReporting

}   // namespace Blaze

