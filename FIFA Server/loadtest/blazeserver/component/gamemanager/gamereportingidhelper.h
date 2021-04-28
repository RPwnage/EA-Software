/*! ************************************************************************************************/
/*!
    \file   gamereportingidhelper.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMEREPORTINGIDHELPER_H
#define BLAZE_GAMEMANAGER_GAMEREPORTINGIDHELPER_H


#include "gamemanager/tdf/gamemanager.h"

namespace Blaze
{
namespace GameManager
{

class GameReportingIdHelper
{
    NON_COPYABLE(GameReportingIdHelper);

public:
    static bool init();
    static BlazeRpcError getNextId(GameReportingId& id);
};

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_GAMEREPORTINGIDHELPER_H
