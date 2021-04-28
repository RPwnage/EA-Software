/*H**********************************************************
 * aries.c
 *
 * Description:
 *
 * Test aries support
 *
 * Copyright (c) Tiburon Entertainment, Inc. 2003.
 *               All rights reserved.
 *
 * Ver.		Description
 * 1.0      07/12/2003 (GWS) [Greg Schaefer] Initial tester
 *
 *H*/

// Includes

#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "dirtysock.h"
#include "protoaries.h"
#include "protoname.h"

#include "zlib.h"

#include "testermodules.h"

// Defines

#define ARIES_CALLBACK_FREQ (100)

//! not a real state, used to display anything invalid
#define PROTOARIES_STATE_INVALID (PROTOARIES_STATE_DISC+1)

// Types

typedef struct AriesRef   // finger daemon state
{
    ProtoAriesRefT *pAries;
    int32_t             iState;
    int32_t             iSndCnt;
} AriesRef;


// Globals

static AriesRef _Aries_State = { 0, 0, 0 };

static char *_Aries_strStates[] =
{
    "PROTOARIES_STATE_IDLE",
    "PROTOARIES_STATE_LIST",
    "PROTOARIES_STATE_CONN",
    "PROTOARIES_STATE_ONLN",
    "PROTOARIES_STATE_DISC",
    "PROTOARIES_STATE_INVALID" // not a real state, used to display invalid state
};

// Prototypes

int32_t CmdAriesCB(ZContext *argz, int32_t argc, char **argv);


static void _AriesDisc(const char *pCmdName)
{
    if (_Aries_State.iState == PROTOARIES_STATE_IDLE)
    {
        return;
    }

    if (_Aries_State.iState == PROTOARIES_STATE_LIST)
    {
        ZPrintf("%s: unlistening\n", pCmdName);
        ProtoAriesUnlisten(_Aries_State.pAries);
    }
    else
    {
        ZPrintf("%s: disconnecting\n", pCmdName);
        ProtoAriesUnconnect(_Aries_State.pAries);
    }

    _Aries_State.iState = PROTOARIES_STATE_IDLE;
}

static void _AriesDestroy(const char *pCmdName)
{
    if (_Aries_State.pAries != NULL)
    {
        _AriesDisc(pCmdName);

        ProtoAriesDestroy(_Aries_State.pAries);
        _Aries_State.pAries = NULL;
    }
}

static void _AriesStat(const char *pCmdName)
{
    uint32_t uStat;

    ZPrintf("%s: Status\n", pCmdName);
    ZPrintf("   addr=%a\n", ProtoAriesStatus(_Aries_State.pAries, 'addr', NULL, 0));
    ZPrintf("   port=%d\n", ProtoAriesStatus(_Aries_State.pAries, 'port', NULL, 0));
    ZPrintf("   ladr=%a\n", ProtoAriesStatus(_Aries_State.pAries, 'ladr', NULL, 0));
    ZPrintf("   lprt=%d\n", ProtoAriesStatus(_Aries_State.pAries, 'lprt', NULL, 0));
    ZPrintf("   ibuf=%d\n", ProtoAriesStatus(_Aries_State.pAries, 'ibuf', NULL, 0));
    ZPrintf("   obuf=%d\n", ProtoAriesStatus(_Aries_State.pAries, 'obuf', NULL, 0));
    uStat = ProtoAriesStatus(_Aries_State.pAries, 'stat', NULL, 0);
    if (uStat > PROTOARIES_STATE_DISC) uStat = PROTOARIES_STATE_INVALID;
    ZPrintf("   stat=%s\n", _Aries_strStates[uStat]);
}

/*F******************************************************
 * CmdAries
 *
 * Description:
 *   Initiate the finger server.
 *
 * Inputs:
 *   Context, arg count, arg strings (zlib standard)
 *
 * Outputs:
 *   Exit code
 * 
 * Ver. Description
 * 1.0  10/4/99 (GWS) Initial version
 *
 *F*/
int32_t CmdAries(ZContext *argz, int32_t argc, char **argv)
{
    // see if we need to create
    if (_Aries_State.pAries == NULL)
    {
        _Aries_State.pAries = ProtoAriesCreate(8192);
        _Aries_State.iState = PROTOARIES_STATE_IDLE;
    }

    // see if we are in DISC state
    if (_Aries_State.iState == PROTOARIES_STATE_DISC)
    {
        _Aries_State.iState = PROTOARIES_STATE_IDLE;
    }

    // handle listen
    if ((argc == 2) && (strcmp(argv[1], "list") == 0))
    {
        // allocate context
        if (_Aries_State.iState == PROTOARIES_STATE_IDLE)
        {
            ZPrintf("%s: listening\n", argv[0]);
            ProtoAriesListen(_Aries_State.pAries, 0, 8888);
            _Aries_State.iState = PROTOARIES_STATE_LIST;
        }
        else
        {
            ZPrintf("%s: not disconnected\n", argv[0]);
            return(0);
        }
        return(ZCallback(&CmdAriesCB, ARIES_CALLBACK_FREQ));
    }

    // handle connect
    else if ((argc == 3) && (strcmp(argv[1], "conn") == 0))
    {
        if (_Aries_State.iState == PROTOARIES_STATE_IDLE)
        {
            uint32_t uAddr = ProtoNameSync(argv[2], 5000);
            ProtoAriesConnect(_Aries_State.pAries, NULL, uAddr, 8888);
            _Aries_State.iState = PROTOARIES_STATE_CONN;
        }
        else
        {
            ZPrintf("%s: not disconnected\n", argv[0]);
            return(0);
        }
        return(ZCallback(&CmdAriesCB, ARIES_CALLBACK_FREQ));
    }

    // handle disconnect
    else if ((argc == 2) && (strcmp(argv[1], "disc") == 0))
    {
        if (_Aries_State.iState != PROTOARIES_STATE_IDLE)
        {
            _AriesDisc(argv[0]);
        }
        return(0);
    }

    // handle status request
    else if ((argc == 2) && (strcmp(argv[1], "stat") == 0))
    {
        _AriesStat(argv[0]);
        return(0);
    }

    // handle help
    else
    {
        ZPrintf("   test protoaries module\n");
        ZPrintf("   usage: %s list|conn [addr]|disc|stat\n", argv[0]);
        return(0);
    }
}


/*F******************************************************
 * CmdAriesdCB
 *
 * Description:
 *   Actually look for finger connections and respond to them.
 *
 * Inputs:
 *   Context, arg count, arg strings (zlib standard)
 *
 * Outputs:
 *   Exit code
 * 
 * Ver. Description
 * 1.0  10/4/99 (GWS) Initial version
 *
 *F*/
int32_t CmdAriesCB(ZContext *argz, int32_t argc, char **argv)
{
    int32_t len;
    int32_t kind;
    int32_t code;
    char data[256];

    // check for kill
    if (argc == 0)
    {
        _AriesDestroy(argv[0]);
        return(0);
    }

    // provide update time
    ProtoAriesUpdate(_Aries_State.pAries);

    // check for incoming packet
    len = ProtoAriesRecv(_Aries_State.pAries, &kind, &code, data, sizeof(data));
    if (len >= 0)
    {
        if ((kind == (signed)0xffffffff) && (code == (signed)0xffffffff))
        {
            ZPrintf("%s: got connect packet\n", argv[0]);
            ZPrintf("%08x/%08x: got %d bytes\n", kind, code, len);

            // update status
            _Aries_State.iState = ProtoAriesStatus(_Aries_State.pAries, 'stat', NULL, 0);
        }
        else if ((kind == (signed)0xffffffff) && (code == (signed)0xfefefefe))
        {
            ZPrintf("%s: got disconnect packet\n", argv[0]);
            ZPrintf("%08x/%08x: got %d bytes\n", kind, code, len);
            _AriesDisc(argv[0]);

            // update status
            _Aries_State.iState = ProtoAriesStatus(_Aries_State.pAries, 'stat', NULL, 0);
        }
        else
        {
            ZPrintf("%c%c%c%c/%08x: got %d bytes\n",
                (unsigned char)(kind >> 24),
                (unsigned char)(kind >> 16),
                (unsigned char)(kind >>  8),
                (unsigned char)(kind),
                code, len);
        }
    }

    // send occasionally
    if (_Aries_State.iState == PROTOARIES_STATE_ONLN && ((_Aries_State.iSndCnt++ % 10) == 0))
    {
        ProtoAriesSend(_Aries_State.pAries, 'blah', 0, "sdflsdfljk", -1);
    }

    // keep running
    return(ZCallback(&CmdAriesCB, ARIES_CALLBACK_FREQ));
}

