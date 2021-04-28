/*************************************************************************************************/
/*!
    \file   gamereportingcustom.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_CUSTOM_H
#define BLAZE_GAMEREPORTING_CUSTOM_H
#ifdef TARGET_gamereporting

/*** Include files *******************************************************************************/
#include "gamereporting/tdf/gamereporting.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

    void registerCustomReportTdfs();
    void deregisterCustomReportTdfs();

}   // namespace GameReporting
}   // namespace Blaze

#endif  // TARGET_gamereporting
#endif  // BLAZE_GAMEREPORT_SLAVE_H
