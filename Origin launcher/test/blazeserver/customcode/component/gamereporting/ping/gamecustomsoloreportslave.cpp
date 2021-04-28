/*************************************************************************************************/
/*!
    \file   gamecustomsoloreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomsoloreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomSoloReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomSoloReportSlave::GameCustomSoloReportSlave(GameReportingSlaveImpl& component) :
GameSoloReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomSoloReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomSoloReportSlave::~GameCustomSoloReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomSoloReportSlave::create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomSoloReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomSoloReportSlave") GameCustomSoloReportSlave(component);
}

}   // namespace GameReporting

}   // namespace Blaze

