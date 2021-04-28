/*************************************************************************************************/
/*!
    \file   registercustomgamereportcollators.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "gamereportcollator.h"
#include "gamereportingslaveimpl.h"

// FIFA SPECIFIC CODE START
#include "osdk/osdkgamereportmaster.h"

#include "fifa/futgamereportcollator.h"
// FIFA SPECIFIC CODE END

namespace Blaze
{
namespace GameReporting
{

void GameReportingSlaveImpl::registerCustomGameReportCollators()
{
    // game team should register their own GameReportCollator here
    // e.g. GameReportingSlaveImpl::registerGameReportCollator("MyOwnGameReportCollator", &MyOwnGameReportCollator::create);
    // FIFA SPECIFIC CODE START
    GameReportingSlaveImpl::registerGameReportCollator("osdkgamereportmaster", &OsdkGameReportMaster::create);

    GameReportingSlaveImpl::registerGameReportCollator("futReportCollator", &FUTGameReportCollator::create);
    // FIFA SPECIFIC CODE END
}

}   // namespace GameReporting
}   // namespace Blaze
