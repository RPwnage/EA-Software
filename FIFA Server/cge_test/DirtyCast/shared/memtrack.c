/*H********************************************************************************/
/*!
    \File memtrack.c

    \Description
        Routines for tracking memory allocations.

    \Copyright
        Copyright (c) 2005-2017 Electronic Arts Inc.

    \Version 02/15/2005 (jbrookes) First Version, based on jfrank's implementation.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "libsample/zmemtrack.h"

#include "dirtycast.h"
#include "memtrack.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _MemtrackLogPrintf

    \Description
        Logs the messages for memtrack

    \Input *pText       - text to log
    \Input *pUserData   - user data passed along

    \Version 09/18/2017 (eesponda)
*/
/********************************************************************************F*/
static void _MemtrackLogPrintf(const char *pText, void *pUserData)
{
    DirtyCastLogPrintf("memtrack: %s", pText);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function MemtrackStartup

    \Description
        Start up the Memtracking module.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void MemtrackStartup(void)
{
    ZMemtrackStartup();
    ZMemtrackCallback(_MemtrackLogPrintf, NULL);
}

/*F********************************************************************************/
/*!
    \Function MemtrackShutdown

    \Description
        Shut down the Memtracking module.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void MemtrackShutdown(void)
{
    ZMemtrackShutdown();
}

/*F********************************************************************************/
/*!
    \Function MemtrackAlloc

    \Description
        Track an allocation.

    \Input *pMem    - pointer to allocated memory block
    \Input uSize    - size of allocated memory block
    \Input uTag     - module tag

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void MemtrackAlloc(void *pMem, uint32_t uSize, uint32_t uTag)
{
    ZMemtrackAlloc(pMem, uSize, uTag);
}

/*F********************************************************************************/
/*!
    \Function MemtrackFree

    \Description
        Track a free operation.

    \Input *pMem    - pointer to allocated memory block
    \Input *pSize   - [out] storage for memory block size

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void MemtrackFree(void *pMem, uint32_t *pSize)
{
    ZMemtrackFree(pMem, pSize);
}

/*F********************************************************************************/
/*!
    \Function MemtrackPrint

    \Description
        Print overall memory info.

    \Input uFlags       - Memtrack_PRINTFLAG_*
    \Input uTag         - [optional] if non-zero, only display memory leaks stamped with this tag
    \Input *pModuleName - [optional] pointer to module name

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void MemtrackPrint(uint32_t uFlags, uint32_t uTag, const char *pModuleName)
{
    ZMemtrackPrint(uFlags, uTag, pModuleName);
}
