/*H********************************************************************************/
/*!
    \File modem.c

    \Description
        A simple TAPI sample application

    \Copyright
        Copyright (c) Electronic Arts 2003-2005.  ALL RIGHTS RESERVED.

    \Version 03/10/2003 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/comm/commall.h"
#include "DirtySDK/comm/pc/commtapi.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamepkt.h"

#include "zlib.h"
#include "..\..\pc\resource\t2hostresource.h"

/*** Defines **********************************************************************/

#define MODEM_PACKETSIZE (NETGAME_DATAPKT_MAXSIZE)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct ModemT
{
    // connection state
    enum
    {
        INIT, LISTEN, CONN, SHUTDOWN
    } state;

    NetGameUtilRefT *pUtilRef;
    NetGameLinkRefT *pLinkRef;

    char strDevice[128];
    int32_t iSendCount;
    int32_t iRecvCount;
    int32_t iCallCnt;

    int32_t bInitialized;

} ModemT;

/*** Function Prototypes **********************************************************/


/*** Variables ********************************************************************/


// Private variables

static ModemT *_Modem_pRef = NULL;

// Public variables


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _CmdModemDestroy
    
    \Description
        Destroy modem connection.
    
    \Input *pRef        - module state
    
    \Output
        None.
            
    \Version 03/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdModemDestroy(ModemT *pRef)
{
    if (pRef->pLinkRef)
    {
        NetGameLinkDestroy(pRef->pLinkRef);
    }

    if (pRef->pUtilRef)
    {
        NetGameUtilDestroy(pRef->pUtilRef);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdModemInitiate
    
    \Description
        Start connection.
    
    \Input *pRef        - module state
    \Input *pDevice     - device string
    \Input *pDevName    - device name
    \Input *pNumber     - number to dial
    
    \Output
        None.
            
    \Version 03/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdModemInitiate(ModemT *pRef, char *pDevice, char *pDevName, char *pNumber)
{
    char strAddr[64];

    // resolve modem device index
    sprintf(strAddr, "%s:", pDevice);

    // create util ref
    pRef->pUtilRef = NetGameUtilCreate();
    strcpy(pRef->strDevice, strAddr);

    // now connect or listen
    if (pNumber[0] != '\0')
    {
        strcat(strAddr, pNumber);
        NetGameUtilConnect(pRef->pUtilRef, NETGAME_CONN_CONNECT, strAddr, (CommAllConstructT *)CommTAPIConstruct);
        ZPrintf("connecting to %s on %s\n", strAddr, pDevName);
    }
    else
    {
        NetGameUtilConnect(pRef->pUtilRef, NETGAME_CONN_LISTEN, strAddr, (CommAllConstructT *)CommTAPIConstruct);
        ZPrintf("listening for connect on %s\n", pDevName);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdModemSelectProc
    
    \Description
        Process modem selection dialog
    
    \Input win      - windows handle
    \Input msg      - windows message
    \Input wparm    - windows parameter
    \Input lparm    - windows parameter
    
    \Output
        LRESULT     - FALSE
            
    \Version 03/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static LRESULT CALLBACK _CmdModemSelectProc(HWND win, UINT msg, WPARAM wparm, LPARAM lparm)
{
    static CommTAPIRef *_pComm = NULL;

    // handle init special (create the class)
    if (msg == WM_INITDIALOG)
    {
        char strAddrList[512], strTemp[64];
        int32_t  iField;

        _pComm = CommTAPIConstruct(300,32,32);
        CommTAPIResolve(_pComm, "localhost", strAddrList, sizeof(strAddrList), ':');

        // parse list and add to combo control
        for (iField = 0; ; iField++)
        {
            if (TagFieldGetDelim(strAddrList, strTemp, sizeof(strTemp), "", iField, ':') < 1)
            {
                break;
            }

            // add new item
            SendDlgItemMessage(win, IDC_MODEMS, CB_ADDSTRING, 0, (LPARAM)strTemp);
        }

        // select first item
        SendDlgItemMessage(win, IDC_MODEMS, CB_SETCURSEL, 0, 0);
    }

    // handle cancel
    if ((msg == WM_COMMAND) && (LOWORD(wparm) == IDCANCEL))
    {
        CommTAPIDestroy(_pComm);
        _pComm = NULL;
        DestroyWindow(win);
    }

    // handle ok
    if ((msg == WM_COMMAND) && (LOWORD(wparm) == IDOK))
    {
        char strBuf[1024], strAddr[64], strNumber[64];
        int32_t iSel;

        iSel = SendDlgItemMessage(win, IDC_MODEMS, CB_GETCURSEL, 0, 0);
        if (iSel >= 0)
        {
            SendDlgItemMessage(win, IDC_MODEMS, CB_GETLBTEXT, iSel, (LPARAM)strBuf);

            // resolve modem device index
            if (CommTAPIResolve(_pComm, strBuf, strAddr, sizeof(strAddr), ':') > 0)
            {
                GetDlgItemText(win, IDC_NUMBER, strNumber, sizeof(strNumber)-1);

                _CmdModemInitiate(_Modem_pRef, strAddr, strBuf, strNumber);
                _Modem_pRef->bInitialized = TRUE;
            }
            else
            {
                ZPrintf("error: invalid device\n");
            }
        }

        CommTAPIDestroy(_pComm);
        _pComm = NULL;

        DestroyWindow(win);
    }

    // let windows handle
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _CmdModemCB
    
    \Description
        Update module state
    
    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output
        int32_t     -
            
    \Version 03/03/2003 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdModemCB(ZContext *argz, int32_t argc, char **argv)
{
    ModemT  *pRef = (ModemT *)argz;

    if (_Modem_pRef->bInitialized == FALSE)
    {
        return(ZCallback(&_CmdModemCB, 100));
    }

    // check for kill
    if (argc == 0)
    {
        _CmdModemDestroy(pRef);
        return(0);
    }

    if ((pRef->state == INIT) || (pRef->state == LISTEN))
    {
        void *pCommRef;

        // check for connect
        pCommRef = NetGameUtilComplete(pRef->pUtilRef);
        if (pCommRef == NULL)
        {
            return(ZCallback(&_CmdModemCB, 100));
        }

        // got a connect -- startup link
        ZPrintf("%s: comm established\n", argv[0]);
        pRef->pLinkRef = NetGameLinkCreate(pCommRef, FALSE, 8192);
        ZPrintf("%s: link running\n", argv[0]);
        pRef->state = CONN;
    }
    else // connected
    {
        NetGamePacketT TestPacket;
        NetGameLinkStatT Stats;

        // send a packet
        TestPacket.head.kind = GAME_PACKET_USER;
        TestPacket.head.len = MODEM_PACKETSIZE;
        sprintf((char *)TestPacket.body.data, "GameServ test packet %d (size %d)", pRef->iCallCnt++, MODEM_PACKETSIZE);
        NetGameLinkSend(pRef->pLinkRef,&TestPacket,1);

        // check for receive
        while (NetGameLinkRecv(pRef->pLinkRef, &TestPacket, 1, FALSE) > 0)
        {
            ZPrintf("%s\n", TestPacket.body.data);

            assert(TestPacket.head.len == MODEM_PACKETSIZE);
            assert(TestPacket.head.kind == GAME_PACKET_USER);
        }

        // check connection status
        NetGameLinkStatus(pRef->pLinkRef, 'stat', 0, &Stats, sizeof(NetGameLinkStatT));
        if (Stats.isopen == FALSE)
        {
            _CmdModemDestroy(pRef);
            ZPrintf("%s: client disconnected\n", argv[0]);
            return(0);
        }
    }

    return(ZCallback(&_CmdModemCB, 100));
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdModem
    
    \Description
        Implementation of simple CommTAPI module.
    
    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output
        int32_t     -
            
    \Version 03/03/2003 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdModem(ZContext *argz, int32_t argc, char **argv)
{
    HWND win;

    win = CreateDialogParam(GetModuleHandle(NULL), "MODEMSELECT", HWND_DESKTOP, (DLGPROC)_CmdModemSelectProc, 0);
    ShowWindow(win, TRUE);

    // allocate context
    _Modem_pRef = (ModemT *) ZContextCreate(sizeof(*_Modem_pRef));
    memset(_Modem_pRef, 0, sizeof(*_Modem_pRef));

    return(ZCallback(&_CmdModemCB, 100));
}
