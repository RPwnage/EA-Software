/*************************************************************************************************/
/*!
    \file   gamecustomclubintrah2hreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomclubintrah2hreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomClubIntraH2HReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomClubIntraH2HReportSlave::GameCustomClubIntraH2HReportSlave(GameReportingSlaveImpl& component) :
GameClubIntraH2HReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomClubIntraH2HReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomClubIntraH2HReportSlave::~GameCustomClubIntraH2HReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomClubIntraH2HReportSlave::create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomClubIntraH2HReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomClubIntraH2HReportSlave") GameCustomClubIntraH2HReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head playoff game used in gamereporting.cfg

    \return - the H2H Playoff game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* GameCustomClubIntraH2HReportSlave::getCustomClubIntraH2HGameTypeName() const
{
    return "gameType14";
}

/*! ****************************************************************************/
/*! \brief initCustomGameParams
    Initialize game parameters that are needed for later processing
********************************************************************************/
void GameCustomClubIntraH2HReportSlave::initCustomGameParams()
{
    // could be a tie game if there is more than one player
    mTieGame = (mPlayerReportMapSize > 1);
}

}   // namespace GameReporting

}   // namespace Blaze

