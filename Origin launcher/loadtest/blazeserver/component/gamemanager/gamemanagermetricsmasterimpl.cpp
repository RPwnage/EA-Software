/*************************************************************************************************/
/*!
    \file   gamemanagermetricsmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GameManagerMetricsMasterImpl

    Implements the master portion of the GM metrics component.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamemanager/gamemanagermetricsmasterimpl.h"

namespace Blaze
{
namespace GameManager
{

/*** Public Methods ******************************************************************************/
// static
GameManagerMetricsMaster* GameManagerMetricsMaster::createImpl()
{
    return BLAZE_NEW_NAMED("GameManagerMetricsMasterImpl") GameManagerMetricsMasterImpl();
}

}
}
