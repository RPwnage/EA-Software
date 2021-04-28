/*************************************************************************************************/
/*!
    \file   gamecustomclubchampreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomclubchampreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomClubChampReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomClubChampReportSlave::GameCustomClubChampReportSlave(GameReportingSlaveImpl& component) :
GameClubChampReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomClubChampReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomClubChampReportSlave::~GameCustomClubChampReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomClubChampReportSlave::create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomClubChampReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomClubChampReportSlave") GameCustomClubChampReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomClubChampGameTypeName
    Return the game type name for Club Champ game used in gamereporting.cfg

    \return - the Club Champ game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* GameCustomClubChampReportSlave::getCustomClubChampGameTypeName() const
{
    return "gameType11";
}

}   // namespace GameReporting

}   // namespace Blaze

