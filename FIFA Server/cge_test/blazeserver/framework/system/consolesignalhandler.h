/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONSOLESIGNALHANDLER_H
#define BLAZE_CONSOLESIGNALHANDLER_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

typedef void (*ShutdownHandler)(void* context);
typedef void (*ReloadHandler)(void* context);
typedef void (*SetServiceStateHandler)(bool inService);

bool InstallConsoleSignalHandler(ShutdownHandler shutdownCb, void* shutdownContext,
        ReloadHandler reloadCb, void* reloadContext, SetServiceStateHandler setServiceStateCb);

void ResetDefaultConsoleSignalHandler();

} // Blaze

#endif // BLAZE_CONSOLESIGNALHANDLER_H

