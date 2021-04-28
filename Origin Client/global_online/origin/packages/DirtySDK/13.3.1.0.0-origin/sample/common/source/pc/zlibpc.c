/*H********************************************************************************/
/*!
    \File zlibpc.c

    \Description
        Platform-specific zlib implementations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/17/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "zlib.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function    ZTick

    \Description
        Return some kind of increasing tick count with millisecond scale (does
        not need to have millisecond precision, but higher precision is better).

    \Input None.

    \Output
        uint32_t    - millisecond tick count

    \Version    1.0        09/15/99 (GWS) First Version
*/
/********************************************************************************F*/
uint32_t ZTick(void)
{
    return(GetTickCount());
}

/*F********************************************************************************/
/*!
    \Function ZSleep

    \Description
        Put process to sleep for some period of time

    \Input iMSecs - Number of milliseconds to sleep for.
    
    \Output None

    \Version 04/07/2005 (jfrank)
*/
/********************************************************************************F*/
void ZSleep(uint32_t uMSecs)
{
    uint32_t uWake;

    // calculate the wake time
    uWake = ZTick() + uMSecs;

    // sleep for a while, but still call NetConnIdle while doing so
    while(ZTick() < uWake)
    {
        // call the idle function
        NetConnIdle();
        // sleep for a while
        Sleep(10);
    }
}

