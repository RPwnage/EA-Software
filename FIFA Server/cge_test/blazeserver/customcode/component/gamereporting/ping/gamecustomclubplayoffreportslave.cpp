/*************************************************************************************************/
/*!
    \file   gamecustomclubplayoffreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomclubplayoffreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomClubPlayoffReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomClubPlayoffReportSlave::GameCustomClubPlayoffReportSlave(GameReportingSlaveImpl& component) :
GameClubPlayoffReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomClubPlayoffReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomClubPlayoffReportSlave::~GameCustomClubPlayoffReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomClubPlayoffReportSlave::Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomClubPlayoffReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomClubPlayoffReportSlave") GameCustomClubPlayoffReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomClubPlayoffGameTypeName
    Return the game type name for Club Playoff game used in gamereporting.cfg

    \return - the club playoff game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* GameCustomClubPlayoffReportSlave::getCustomClubPlayoffGameTypeName() const
{
    return "gameType13";
}

/*************************************************************************************************/
/*! \brief processCustomUpdatedStats
    A custom hook to allow game team to process custom stats update and return false if process 
        failed and should stop further game report processing

    \return - true if the post-game custom processing is performed successfully
*/
/*************************************************************************************************/
bool GameCustomClubPlayoffReportSlave::processCustomUpdatedStats() 
{
    return true;
}

}   // namespace GameReporting

}   // namespace Blaze

