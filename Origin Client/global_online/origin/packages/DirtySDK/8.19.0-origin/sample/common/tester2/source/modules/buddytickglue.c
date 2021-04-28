/*H*************************************************************************************/
/*!
    \File    buddytickglue.c

    \Description
        Reference application for the BuddyTickerGlue module

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
#include "hlbudapi.h"
#include "tickerapi.h"
#include "buddytickerglue.h"

#include "zlib.h"

#include "testerregistry.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

static BuddyTickerGlueRefT *_BuddyTickerGlue_pRef = NULL;

/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    CmdBuddyTickerGlue
    
    \Description
        Ticker command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - '0' OK, '-1' error
            
    \Version 1.0 05/04/2009 (cvienneau) First Version
*/
/**************************************************************************************F*/
int32_t CmdBuddyTickerGlue(ZContext *argz, int32_t argc, char *argv[])
{
    // handle basic help
    if ((argc < 2) || (strcmp(argv[1], "create") && strcmp(argv[1], "destroy") && strcmp(argv[1], "score")))
    {
        ZPrintf("   usage: %s create\n", argv[0]);
        ZPrintf("   usage: %s destroy\n", argv[0]);
        ZPrintf("   usage: %s score HomeUser iHomeScore AwayUser iAwayScore pStatus\n", argv[0]);
        return(0);
    }

    if (!strcmp(argv[1], "create"))
    {
        HLBApiRefT *pHLBudApi;
        TickerApiT *pTickerApi;
        // only allow one ref
        if (_BuddyTickerGlue_pRef != NULL)
        {
            ZPrintf("   ticker already active\n");
            return(-1);
        }

        // get hlbudapi, tickerapi
        if ((pHLBudApi = TesterRegistryGetPointer("HLBUDDY")) == NULL)
        {
            ZPrintf("   creating buddy ticker glue without hlbuddy ref\n");
            return(-1);
        }
        if ((pTickerApi = TesterRegistryGetPointer("TICKER")) == NULL)
        {
            ZPrintf("   creating buddy ticker glue without ticker ref\n");
            return(-1);
        }
        
        // create buddy ticker glue
        if ((_BuddyTickerGlue_pRef = BuddyTickerGlueCreate(pHLBudApi, pTickerApi)) == NULL)
        {
            ZPrintf("   unable to create buddy ticker glue\n");
        }
    }
    else if (!strcmp(argv[1], "destroy"))
    {
        if(_BuddyTickerGlue_pRef != NULL)
        {
            BuddyTickerGlueDestroy(_BuddyTickerGlue_pRef);
        }
        else
        {
            ZPrintf("   unable to buddytickerglue, its not active.\n");
        }
    }
    else if (!strcmp(argv[1], "score"))
    {
        HLBApiRefT *pHLBudApi;
        if ((pHLBudApi = TesterRegistryGetPointer("HLBUDDY")) == NULL)
        {
            ZPrintf("   unable to run score command without hlbuddy ref\n");
            return(-1);
        }

        BuddyTickerGlueSetScore(pHLBudApi, argv[2], atoi(argv[3]), argv[4], atoi(argv[5]), argv[6], argv[7]);
        //BuddyTickerGlueSetScore(pHLBudApi, "L Buddy", 3, "R Buddy", 2, "Q2", NULL);
    }
    return(0);
}
