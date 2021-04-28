/*H*************************************************************************************/
/*!
    \File    tick.c

    \Description
        Reference application for 'ticker' functions.

    \Copyright
        Copyright (c) Electronic Arts 2006.    ALL RIGHTS RESERVED.

    \Version    1.0        05/04/2009 (cvienneau) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "dirtysock.h"
#include "netconn.h"
#include "lobbytagfield.h"
#include "testerregistry.h"
#include "tickerapi.h"

#include "zlib.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

static TickerApiT *_Ticker_pRef = NULL;

/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdTicker
    
    \Description
        Ticker command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 05/04/2009 (cvienneau) First Version
*/
/**************************************************************************************F*/
int32_t CmdTicker(ZContext *argz, int32_t argc, char *argv[])
{
    // handle basic help
    if ((argc != 2) || (strcmp(argv[1], "create") && strcmp(argv[1], "destroy")))
    {
        ZPrintf("   usage: %s [create|destroy]\n", argv[0]);
        return(0);
    }

    if (!strcmp(argv[1], "create"))
    {
        // only allow one ref
        if (_Ticker_pRef != NULL)
        {
            ZPrintf("   ticker already active\n");
            return(0);
        }

        // create ticker
        if ((_Ticker_pRef = TickerApiCreate(32)) == NULL)
        {
            ZPrintf("   unable to create ticker\n");
        }
        else
        {
            TesterRegistrySetPointer("TICKER", _Ticker_pRef);
        }
    }
    else if (!strcmp(argv[1], "destroy"))
    {
        if(_Ticker_pRef != NULL)
        {
            TickerApiDestroy(_Ticker_pRef);
        }
        else
        {
            ZPrintf("   unable to destroy tick, its not active.\n");
        }
    }
    return(0);
}
