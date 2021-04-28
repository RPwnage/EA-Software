/*H********************************************************************************/
/*!
    \File zlibps4.c

    \Description
        Platform-specific zlib implementations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/17/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "zlib.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F*************************************************************************************/
/*!
    \Function    ZTick

    \Description
        Return some kind of increasing tick count with millisecond scale (does
        not need to have millisecond precision, but higher precision is better).

    \Input None.

    \Output
        uint32_t    - millisecond tick count

    \Version 1.0 05/06/2005 (jfrank) First Version
*/
/*************************************************************************************F*/
uint32_t ZTick(void)
{
    return(NetTick());
}


/*F********************************************************************************/
/*!
    \Function ZSleep

    \Description
        Put process to sleep for some period of time

    \Input uMSecs - Number of milliseconds to sleep for.
    
    \Output None

    \Version 05/06/2005 (jfrank)
*/
/********************************************************************************F*/
void ZSleep(uint32_t uMSecs)
{
    NetConnSleep(uMSecs);
}
