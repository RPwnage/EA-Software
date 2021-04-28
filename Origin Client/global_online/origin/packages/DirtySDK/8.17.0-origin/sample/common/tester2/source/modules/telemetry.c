/*H********************************************************************************/
/*!
    \File telemetry.c

    \Description
        Tester module for Telemetry, adapted from logger.c

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "platform.h"
#include "dirtysock.h"
#include "lobbytagfield.h"
#include "lobbylang.h"

#include "zlib.h"
#include "zmem.h"
#include "zfile.h"

#include "telemetryapi.h"
#include "testersubcmd.h"
#include "testerregistry.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

#define _TELEMETRY_MAX_SENDFILE_BUFFERS    (2)
//! news file telemetry tags
#define TELEMETRY_NEWSFILETAG_SENDPERCENTAGE  ("TELE_SEND_PCT")   //!< percentage full the buffer should be before attempting a send
#define TELEMETRY_NEWSFILETAG_SENDDELAY       ("TELE_SEND_DLY")   //!< the delay, in milliseconds, to wait after an entry before a send
#define TELEMETRY_NEWSFILETAG_EVENTFILTER     ("TELE_FILTER")     //!< the filter to apply to events before sending them to the server
#define TELEMETRY_NEWSFILETAG_DISABLE         ("TELE_DISABLE")    //!< a list of countries in which we do not collect telemetry

#define TELEMETRY_PERSRESPTAG_AUTHSTRING      ("EX-Telemetry")    //!< the authentication string from the Lobby 'pers' response

/*** Type Definitions *************************************************************/

typedef struct TelemetryApiSendFileDataT
{
    enum { TRANS_NOT_INUSE, TRANS_BEG, TRANS_SEND } iTransactionState;  //!< the state of this transaction
    ZFileT     iFileId;        //!< file id provided by ZFileOpen
    int32_t     iTransactionId; //!< telemetry api transaction id from TelemetryApiBeginTransaction
} TelemetryApiSendFileDataT; 

typedef struct TelemetryAppT
{
    TelemetryApiRefT *pTelemetryApiRef;
    int32_t           iTelemetryUpdateCallback;
    TelemetryApiSendFileDataT SendFileData[_TELEMETRY_MAX_SENDFILE_BUFFERS];
    TelemetryApiSendFileDataT TransactionData;
    void *pSnapshotBuffer;
    uint32_t uSnapshotBufferSize;
    enum
    {
        ST_IDLE,
        ST_FILL,
    } eState;
} TelemetryAppT;

typedef struct TelemetryEventSampleT
{
    uint32_t uModuleID;
    uint32_t uGroupID;
    char strEvent[TELEMETRY_EVENTSTRINGSIZE_DEFAULT];
} TelemetryEventSampleT;


/*** Function Prototypes ***************************************************************/

static void _TelemetryCreate       (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryConnect      (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryDestroy      (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryDisconnect   (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryDump         (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryEvent        (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryEventBDID    (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryEvents       (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryFill         (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryFilter       (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryPopBack      (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryPopFront     (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryInfoInt      (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryControl      (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetrySendFile     (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetrySnapLoad     (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetrySnapSave     (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetrySnapSize     (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryTransStart   (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryTransSend    (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryTransCancel  (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);
static void _TelemetryTransEnd     (void *_pApp, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables *************************************************************************/

static T2SubCmdT _Telemetry_Commands[] =
{
    { "create",         _TelemetryCreate       },
    { "connect",        _TelemetryConnect      },
    { "destroy",        _TelemetryDestroy      },
    { "disconnect",     _TelemetryDisconnect   },
    { "dump",           _TelemetryDump         },
    { "event",          _TelemetryEvent        },
    { "bdid",           _TelemetryEventBDID    },
    { "events",         _TelemetryEvents       },
    { "fill",           _TelemetryFill         },
    { "filter",         _TelemetryFilter       },
    { "popback",        _TelemetryPopBack      },
    { "popfront",       _TelemetryPopFront     },
    { "infoint",        _TelemetryInfoInt      },
    { "control",        _TelemetryControl      },
    { "sendfile",       _TelemetrySendFile     },
    { "snapload",       _TelemetrySnapLoad     },
    { "snapsave",       _TelemetrySnapSave     },
    { "snapsize",       _TelemetrySnapSize     },
    { "tstart",         _TelemetryTransStart   },
    { "tsend",          _TelemetryTransSend    },
    { "tcancel",        _TelemetryTransCancel  },
    { "tend",           _TelemetryTransEnd     },
    { "",               NULL                }
};

static TelemetryAppT _TelemetryApp = { NULL };

#if 0
    static const char *_strTestFilters[] = 
    {
        /* wildcard good  */    "+****/****",
        /* wildcard good  */    "-****/AAAA",
        /* wildcard bad   */    "-*   /AAAA",
        /* wildcard bad   */    "-*  */****",
        /* wildcard good  */    "+AAAA/****",
        /* wildcard bad   */    "+*asd/fghj",
        /* wildcard bad   */    "-sad*/fhjk",
        /* wildcard bad   */    "+asdf/*bad",
        /* wildcard bad   */    "-asdf/ba*d",
        /* empty string   */    "",
        /* leading space  */    "  +ABCD/EFGH, -IJKL/MNOP ",
        /* normal         */    "+1234/5678, +7893/ABCD, -jksd/ffff",
        /* trailing comma */    "+1111/2222,",
        /* illegal length */    "+111/222,",
        /* illegal length */    "+11/22,",
        /* illegal length */    "+1/2,",
        /* no tokens      */    "+/,",
        /* no tokens      */    " + / , ",
        /* bad then good  */    " + / , +1234/5678,",
        /* illegal length */    "111/222,",
        /* no tokens      */    "+1111/2222, -",
        /* no token       */    "+1111/2222, -ABCD",
        /*                */    "+1111/2222, -ABC",
        /*                */    "+1111/2222, -A",
        /*                */    "+1111/2222, -A/",
        /*                */    "+1111/2222, -A/Z",
        /*                */    "+1111/2222, -A/ZXY",
        /*                */    "+1111/2222, -ABCD/",
        /*                */    "+1111/2222, -ABCD/DEF",
        /* spaces         */    " + 1111 / 2222 , +1111/2222,",
        /* backslash      */    "-3333\\4444",
        /*                */    "1111/2222",
        /* backslash      */    "1111/2222, +abcd/EFGH, -7896\\1234",
        /* space, tab char*/    "+5555                                     /              7777, \t3333/9999",
        /*                */    "+5555                                     /              7777, \t3333/9999, +ABCD/EFGH",
        /* should ovflow  */    "+1111/1111,+1111/2222,+1111/3333,+1111/4444,+1111/5555,+1111/6666,+1111/7777,+1111/8888,+1111/9999,+1111/1010,+1111/1111,+1111/1212,+1111/1313,+1111/1414,+1111/1515,+1111/1616,+1111/1717,+1111/1818,+1111/1919,+1111/2020,+1111/2121,+1111/2222,+1111/2323,+1111/2424,+1111/2525,+1111/2626,+1111/2727,+1111/2828,+1111/2929,+1111/3030,+1111/3131,+1111/3232,+1111/3333",
        /* last is bad    */    "+1111/1111,+1111/2222,+1111/3333,+1111/4444,+1111/5555,+1111/6666,+1111/7777,+1111/8888,+1111/9999,+1111/1010,+1111/1111,+1111/1212,+1111/1313,+1111/1414,+1111/1515,+1111/1616,+1111/1717,+1111/1818,+1111/1919,+1111/2020,+1111/2121,+1111/2222,+1111/2323,+1111/2424,+1111/2525,+1111/2626,+1111/2727,+1111/2828,+1111/2929,+1111/3030,+1111/3131,+1111/3232,1111/3333",
        /* last is bad    */    "+1111/1111,+1111/2222,+1111/3333,+1111/4444,+1111/5555,+1111/6666,+1111/7777,+1111/8888,+1111/9999,+1111/1010,+1111/1111,+1111/1212,+1111/1313,+1111/1414,+1111/1515,+1111/1616,+1111/1717,+1111/1818,+1111/1919,+1111/2020,+1111/2121,+1111/2222,+1111/2323,+1111/2424,+1111/2525,+1111/2626,+1111/2727,+1111/2828,+1111/2929,+1111/3030,+1111/3131,+1111/3232,         1111/3333",
        /* big, spaces    */    "+1111/1111,  +1111/2222,  +1111/3333,  +1111/4444,  +1111/5555,  +1111/6666,  +1111/7777,  +1111/8888,  +1111/9999,  +1111/1010,  +1111/1111,  +1111/1212,  +1111/1313,  +1111/1414,  +1111/1515,  +1111/1616,  +1111/1717,  +1111/1818,  +1111/1919,  +1111/2020,  +1111/2121,  +1111/2222,  +1111/2323,  +1111/2424,  +1111/2525,  +1111/2626,  +1111/2727,  +1111/2828,  +1111/2929,  +1111/3030,  +1111/3131,  +1111/3232,  +1111/3333  "
    };
    static const uint32_t _uNumTestFilters = sizeof(_strTestFilters) / sizeof(_strTestFilters[0]);
#else
    static const char *_strTestFilters[] = 
    {
        "+****/****",
        "-****/****",
        "-abcd/****",
        "-ABCD/****",
        "-abcd/defg",
    };
    static const uint32_t _uNumTestFilters = sizeof(_strTestFilters) / sizeof(_strTestFilters[0]);
#endif

// I used these for testing, and I might want these later, but for now they should be disabled
//static const TelemetryEventSampleT _TestEvents[] = 
//{
//    { 'abcd', 'defg', "SampleString1" },
//    { 'ABCD', 'defg', "SampleString2" },
//    { 'abcd', 'DEFG', "SampleString3" },
//    { '1234', 'defg', "SampleString4" },
//    { 'abcd', '5678', "SampleString5" }
//};
//static const uint32_t _uNumTestEvents = sizeof(_TestEvents) / sizeof(_TestEvents[0]);

/*** Private Functions ************************************************************/

/*F*************************************************************************************/
/*!
    \Function    _TelemetryBufferFullCallback
    
    \Description
        Telemetry buffer full callback
    
    \Input  *pTelemetryApiRef - Telemetry Instance
    
    \Output None
            
    \Version 03/07/2007 (danielcarter)
*/
/**************************************************************************************F*/
static void _TelemetryBufferFullCallback(TelemetryApiRefT *pTelemetryApiRef, void *pUserData)
{
    ZPrintf("Telemetry: Telemetry Buffer FULL callback activated for instance [0x%08X].\n", pTelemetryApiRef);
    if (_TelemetryApp.eState == ST_FILL)
    {
        _TelemetryApp.eState = ST_IDLE;
    }
}


/*F*************************************************************************************/
/*!
    \Function    _TelemetryBufferEmptyCallback
    
    \Description
        Telemetry buffer empty callback
    
    \Input  *pTelemetryApiRef - Telemetry Instance
    
    \Output None
            
    \Version 03/07/2007 (danielcarter)
*/
/**************************************************************************************F*/
static void _TelemetryBufferEmptyCallback(TelemetryApiRefT *pTelemetryApiRef, void *pUserData)
{
    ZPrintf("Telemetry: Telemetry Buffer EMPTY callback activated for instance [0x%08X].\n", pTelemetryApiRef);
}


/*F*************************************************************************************/
/*!
    \Function    _TelemetryBufferSendComplete
    
    \Description
        Telemetry buffer seend complete callback
    
    \Input  *pTelemetryApiRef - Telemetry Instance
    
    \Output None
            
    \Version 03/07/2007 (danielcarter)
*/
/**************************************************************************************F*/
static void _TelemetryBufferSendComplete(TelemetryApiRefT *pTelemetryApiRef, void *pUserData)
{
    ZPrintf("Telemetry: Telemetry buffer send complete callback activated for instance [0x%08X].\n", pTelemetryApiRef);
}

/*F*************************************************************************************/
/*!
    \Function    _CmdTelemetryCB
    
    \Description
        Telemetry command callback
    
    \Input  *argz   - unused
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    
    \Output
        int32_t         - zero
            
    \Version 03/07/2007 (danielcarter)
*/
/**************************************************************************************F*/
static int32_t _CmdTelemetryCB(ZContext *argz, int32_t argc, char *argv[])
{
    #define BLOCK_SIZE (8192)
    int32_t iTransaction, iBytesRead;
    char    strBuff[BLOCK_SIZE];

    // handle the shutdown case
    if (argc == 0)
    {
        // check to make sure we haven't already quit
        if (_TelemetryApp.pTelemetryApiRef) 
        {
            TelemetryApiDestroy(_TelemetryApp.pTelemetryApiRef);
            _TelemetryApp.pTelemetryApiRef = 0;
        }
        return(0);
    }

    // iterate over the transaction and continue their progress as necessary
    for (iTransaction = 0; iTransaction < _TELEMETRY_MAX_SENDFILE_BUFFERS; iTransaction++)
    {
        // see if we are in-progress
        if (_TelemetryApp.SendFileData[iTransaction].iTransactionState == TRANS_SEND)
        {
            // read the next block of data from the file
            iBytesRead = ZFileRead(_TelemetryApp.SendFileData[iTransaction].iFileId, strBuff, BLOCK_SIZE);
            // send the data to the telemetry server
            if ((iBytesRead > 0) && (iBytesRead <= BLOCK_SIZE))
            {
                int32_t iBytesSent = 0;

                while (iBytesSent < iBytesRead)
                {
                    int32_t iNewBytesSent = TelemetryApiSendTransactionData(_TelemetryApp.pTelemetryApiRef, 
                                                                           _TelemetryApp.SendFileData[iTransaction].iTransactionId, 
                                                                           &(strBuff[iBytesSent]), iBytesRead-iBytesSent);

                    iBytesSent = iBytesSent + iNewBytesSent;

                    ZPrintf("TelemetrySendFile: transaction #%d; sent %d bytes to telemetryserver\n",
                        _TelemetryApp.SendFileData[iTransaction].iTransactionId, iNewBytesSent);
                }
            }
            // if that is the last of the data then end the transaction
            if (iBytesRead != BLOCK_SIZE)
            {
                TelemetryApiEndTransaction(_TelemetryApp.pTelemetryApiRef, _TelemetryApp.SendFileData[iTransaction].iTransactionId);
                ZFileClose(_TelemetryApp.SendFileData[iTransaction].iFileId);
                _TelemetryApp.SendFileData[iTransaction].iTransactionState = TRANS_NOT_INUSE;
            }
        }
    }

    if (_TelemetryApp.eState == ST_FILL)
    {
        TelemetryApiEvent(_TelemetryApp.pTelemetryApiRef, 'TSTR', 'YOYO', "");
    }
    
    // re-call me in 50ms, just like the ticker sample
    return(ZCallback(_CmdTelemetryCB,50));
}

/*F********************************************************************************/
/*!
    \Function _TelemetryCreate

    \Description
        Create a Telemetry Instance

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryCreate(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    uint32_t uNumEvents = 0;
    //TelemetryApiBufferTypeE eBufferType = TELEMETRY_BUFFERTYPE_FILLANDSTOP;
    TelemetryApiBufferTypeE eBufferType = TELEMETRY_BUFFERTYPE_CIRCULAROVERWRITE;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s create <timeout, ms> <num events> <buffer type, 0 or 1>\n", argv[0]);
        return;
    }

    if (pApp->pTelemetryApiRef)
    {
        ZPrintf("Telemetry: TelemetryApiRef instance already created.\n");
        return;
    }

    if (argc > 3)   uNumEvents  = atoi(argv[3]);
    if (argc > 4)   eBufferType = atoi(argv[4]);

    // create telemetry
    pApp->pTelemetryApiRef = TelemetryApiCreate(uNumEvents, eBufferType);
    TelemetryApiSetCallback(pApp->pTelemetryApiRef, TELEMETRY_CBTYPE_BUFFERFULL,  _TelemetryBufferFullCallback,  NULL);
    TelemetryApiSetCallback(pApp->pTelemetryApiRef, TELEMETRY_CBTYPE_BUFFEREMPTY, _TelemetryBufferEmptyCallback, NULL);
    TelemetryApiSetCallback(pApp->pTelemetryApiRef, TELEMETRY_CBTYPE_FULLSENDCOMPLETE, _TelemetryBufferSendComplete, NULL);

    // start the callback
    ZCallback(_CmdTelemetryCB,50);

    ZPrintf("Telemetry: Create returned pointer [0x%X]\n", pApp->pTelemetryApiRef);
}

/*F********************************************************************************/
/*!
    \Function _LobbyTelemetryUpdate

    \Description
        _LobbyTelemetryUpdate checks the size of the storage buffer and the time 
        since it was last flushed. If the buffer is more than (threshold) full
        or the timeout has expired, the current buffer is sent to the server and 
        cleared locally. 

    \Input *pLobbyRef  - module state

    \Output None

    \Version 1.0 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
/*
CHANGEME: find a proper hook for a regular update, not through lobby
static void _LobbyTelemetryUpdate(void *unused1, void *unused2, void *pUserData)
{
    TelemetryApiRefT *pTelemetryApiRef = pUserData;

    // give the TelemetryApi some CPU time
    TelemetryApiUpdate(pTelemetryApiRef);
}
*/

/*F********************************************************************************/
/*!
    \Function _LobbyTelemetryConnect

    \Description
        This function “connects” the API to the lobby system so data can be 
        uploaded.  After authenticating to the lobby server, call this function
        with the lobby server reference.

    \Input *pTelemetryApp  - module state
    \Input *pLobby   - Lobby reference used to send the buffer to the server

    \Output int32_t - TRUE if we are now connected; FALSE if not

    \Version 1.0 03/07/2007 (danielcarter) First Version

*/
/********************************************************************************F*/
static int32_t _LobbyTelemetryConnect(TelemetryAppT *pTelemetryApp, void *unused)
{
    const char *pTelemetryAuthPtr, *pSettingPtr;
    int32_t iTimeout, iThresholdPCT;

    // check for error conditions
    // check to see if we're already connected

    // get the 'perd' data for the EX-Telemetry to set the ip/port/auth string
    pTelemetryAuthPtr = "CHANGEME";
    if (pTelemetryAuthPtr)
    {
        char strAuthString[512];
        if (TagFieldGetString(pTelemetryAuthPtr, strAuthString, sizeof(strAuthString), "") > 0)
        {
            TelemetryApiAuthent(pTelemetryApp->pTelemetryApiRef, strAuthString);
        }
    }
    else
    {
        NetPrintf(("Telemetry: Cannot start telemetry API; EX-Telemetry tag is missing from 'perd' data.\n"));
        return(FALSE);
    }

    // keep a reference to the lobby around for use in _LobbyTelemetryUpdate

    // register the callback
    pTelemetryApp->iTelemetryUpdateCallback = 0;// CHANGEME 

    // check for configuration items
    iTimeout     = 15; // CHANGEME
    iThresholdPCT = 15; // CHANGEME

    // send config parameters to the TelemetryAPI
    TelemetryApiControl(pTelemetryApp->pTelemetryApiRef, 'time', iTimeout, NULL);
    TelemetryApiControl(pTelemetryApp->pTelemetryApiRef, 'thrs', iThresholdPCT, NULL);

    // get the list of disabled countries from the lobby config
    pSettingPtr = "CHANGEME"; // CHANGEME
    if (pSettingPtr)
    {
        char strDisabledCountries[128];
        TagFieldGetString(pSettingPtr, strDisabledCountries, 128, "");
        // send the list of disabled countries to TelemetryApi
        TelemetryApiControl(pTelemetryApp->pTelemetryApiRef, 'cdbl', 0, strDisabledCountries);
    }

    // apply any event filter rules the server might have
    pSettingPtr = TELEMETRY_NEWSFILETAG_EVENTFILTER;
    if (pSettingPtr)
    {
        char strFilterRules[4096]; // CHANGEME
        if (TagFieldGetString(pSettingPtr, strFilterRules, sizeof(strFilterRules), "") > 0)
        {
            TelemetryApiFilter(pTelemetryApp->pTelemetryApiRef, strFilterRules);
        }
    }

    // call the Telemetry function
    return(TelemetryApiConnect(pTelemetryApp->pTelemetryApiRef));
}

/*F********************************************************************************/
/*!
    \Function _TelemetryConnect

    \Description
        Connect to a running lobby server

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryConnect(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;

    // check for create
    if (pApp->pTelemetryApiRef == NULL)
    {
        ZPrintf("Telemetry: No TelemetryApiRef available.  Try calling create.\n");
        return;
    }

    _LobbyTelemetryConnect(pApp, NULL);
    ZPrintf("Telemetry: Connect\n");
}

/*F********************************************************************************/
/*!
    \Function _TelemetryEvent

    \Description
        Create an event to report

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryEvent(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    uint32_t uModule = 'TSTR';
    uint32_t uGroup = 'YOYO';
    const char *pKind;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s event <module> <group> <text> : create telemetry message\n", argv[0]);
        return;
    }

    // check for create
    if (pApp->pTelemetryApiRef == NULL)
    {
        ZPrintf("Telemetry: No TelemetryApiRef available.  Try calling create.\n");
        return;
    }
    
    if (argc > 3)
    {
        // convert the first parameter to a module id.
        pKind = argv[2];
        uModule = (pKind[0] << 24) + (pKind[1] << 16) + (pKind[2] << 8) + pKind[3];
    }

    if (argc > 4)
    {
        // convert the second parameter to a group id.
        pKind = argv[3];
        uGroup = (pKind[0] << 24) + (pKind[1] << 16) + (pKind[2] << 8) + pKind[3];
    }

    if (argc == 2)
    {
        // enter a default event
        TelemetryApiEvent(pApp->pTelemetryApiRef, 'TSTR', 'YOYO', "");
    }
    else
    {
        // enter a better specified event
        TelemetryApiEvent(pApp->pTelemetryApiRef, uModule, uGroup, argv[argc-1]);
    }
}

/*F********************************************************************************/
/*!
    \Function _TelemetryEventBDID

    \Description
        Report a BDID

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryEventBDID(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    static uint8_t _TestData1[] =
    {
        0xDD, 0x29, 0x7C, 0x80,
        0x9B, 0xCA, 0xC3, 0x4E,
        0x23, 0xD3, 0xC1, 0xE6,
        0xAC, 0xA2, 0x23, 0x17
    };
    static uint8_t _TestData2[] =
    {
        0xDD, 0x29, 0x7C, 0x80,
        0x9B, 0xCA, 0xC3, 0x4E,
        0x23, 0xD3, 0xC1, 0xE6,
        0xAC, 0xA2, 0x23, 0x17,
        0x23, 0xD3, 0xC1, 0xE6,
    };    
    static uint8_t _TestData3[] =
    {
        0xDD, 0x29, 0x7C, 0x80,
        0x23, 0xD3, 0xC1, 0xE6,
    };    
    static uint8_t _TestData4[] =
    {
        0xDD, 0x29, 0x7C, 0x80,
        0x23, 0xD3, 0xC1, 
    };    

    if (bHelp == TRUE)
    {
        ZPrintf("    %s bdid: report bdid data\n", argv[0]);
        return;
    }

    // check for create
    if (pApp->pTelemetryApiRef == NULL)
    {
        ZPrintf("Telemetry: No TelemetryApiRef available.  Try calling create.\n");
        return;
    }

    NetPrintf(("telemetry test 1\n"));
    TelemetryApiEventBDID(pApp->pTelemetryApiRef, _TestData1, sizeof(_TestData1));
    NetPrintf(("telemetry test 2\n"));
    TelemetryApiEventBDID(pApp->pTelemetryApiRef, _TestData2, sizeof(_TestData2));
    NetPrintf(("telemetry test 3\n"));
    TelemetryApiEventBDID(pApp->pTelemetryApiRef, _TestData3, sizeof(_TestData3));
    NetPrintf(("telemetry test 4\n"));
    TelemetryApiEventBDID(pApp->pTelemetryApiRef, _TestData4, sizeof(_TestData4));
}

/*F********************************************************************************/
/*!
    \Function _TelemetryEvents

    \Description
        Push a predefined set of events into the buffer

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryEvents(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    char strEvent[TELEMETRY_EVENTSTRINGSIZE_DEFAULT];
    uint32_t uLoop, uMaxEvents;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s %s : write predefined telemetry messages\n", argv[0], argv[1]);
        return;
    }

    // check for create
    if (pApp->pTelemetryApiRef == NULL)
    {
        ZPrintf("Telemetry: No TelemetryApiRef available.  Try calling create.\n");
        return;
    }

    uMaxEvents = TelemetryApiStatus(pApp->pTelemetryApiRef, 'maxe', NULL, 0);
    NetPrintf(("Telemetry: Filling Telemetry buffer with [%u] events for testing.\n", uMaxEvents));
    for(uLoop = 0; uLoop < uMaxEvents; uLoop++)
    {
        snzprintf(strEvent, sizeof(strEvent), "Tester2[%05u]", uLoop);
        TelemetryApiEvent(pApp->pTelemetryApiRef, 'DRTY', 'TEST', strEvent);
    }

    //for(uLoop = 0; uLoop < _uNumTestEvents; uLoop++)
    //{
    //    const TelemetryEventSampleT *pEvent;
    //    pEvent = &(_TestEvents[uLoop]);
    //    TelemetryApiEvent(pApp->pTelemetryApiRef, 
    //        pEvent->uModuleID, pEvent->uGroupID, pEvent->strEvent);
    //}
}

/*F********************************************************************************/
/*!
    \Function _TelemetryFill

    \Description
        Fill the buffer.

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryFill(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s %s : fill the event buffer\n", argv[0], argv[1]);
        return;
    }

    // check for create
    if (pApp->pTelemetryApiRef == NULL)
    {
        ZPrintf("Telemetry: No TelemetryApiRef available.  Try calling create.\n");
        return;
    }

    pApp->eState = ST_FILL;
}


/*F********************************************************************************/
/*!
    \Function _TelemetryFilter

    \Description
        Set the filters

    \Input  *pApp   - pointer to telemetry app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryFilter(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    uint32_t uLoop, uSelected;


    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("    %s %s <filter number> : set the filter rules\n", argv[0], argv[1]);
        ZPrintf("\tAvailable filter rules:\n");
        for(uLoop = 0; uLoop < _uNumTestFilters; uLoop++)
            ZPrintf("\t\t[%u][%s]\n", uLoop+1, _strTestFilters[uLoop]);
        ZPrintf("\n");
        return;
    }

    // check for create
    if (pApp->pTelemetryApiRef == NULL)
    {
        ZPrintf("Telemetry: No TelemetryApiRef available.  Try calling create.\n");
        return;
    }

    // try to pick the requested string - user has entered a 1-based filter number
    uSelected = (uint32_t)(atoi(argv[2]))-1;
    if(uSelected < _uNumTestFilters)
    {
        ZPrintf("Telemetry: Applying filter [%s]\n", _strTestFilters[uSelected]);
        TelemetryApiFilter(pApp->pTelemetryApiRef, _strTestFilters[uSelected]);
        //ZPrintf("Telemetry: dumping all filters [ %d in use, %d total ]\n", 
        //    LobbyTelemetryInfoInt(pApp->pTelemetryApiRef, 'nflt'), LobbyTelemetryInfoInt(pApp->pTelemetryApiRef, 'mflt'));
        //TelemetryFiltersPrint(pApp->pTelemetryApiRef);
        //ZPrintf("Telemetry: dump complete.\n");
    }
    else
    {
        ZPrintf("Illegal filter number [ %u ]\n", uSelected+1);
    }
}


/*F********************************************************************************/
/*!
    \Function _TelemetryDump

    \Description
        Dump all events in the telemetry buffer.  This is a hack function to check the
        operation of the telemetry and this sort of operation should not be used
        by the game.

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryDump(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    int32_t iMaxEvents, iNumEvents, iFull, iEmpty, iNumFilters, iMaxFilters, iHead, iTail;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s %s - dump the contents of the telemetry buffers\n", argv[0], argv[1]);
        return;
    }

    iMaxEvents  = TelemetryApiStatus(pApp->pTelemetryApiRef, 'maxe', NULL, 0);
    iNumEvents  = TelemetryApiStatus(pApp->pTelemetryApiRef, 'nume', NULL, 0);
    iFull       = TelemetryApiStatus(pApp->pTelemetryApiRef, 'full', NULL, 0);
    iEmpty      = TelemetryApiStatus(pApp->pTelemetryApiRef, 'mpty', NULL, 0);
    iNumFilters = TelemetryApiStatus(pApp->pTelemetryApiRef, 'nflt', NULL, 0);
    iMaxFilters = TelemetryApiStatus(pApp->pTelemetryApiRef, 'mflt', NULL, 0);
    iHead       = TelemetryApiStatus(pApp->pTelemetryApiRef, 'evth', NULL, 0);
    iTail       = TelemetryApiStatus(pApp->pTelemetryApiRef, 'evtt', NULL, 0);

    #if DIRTYCODE_LOGGING
    ZPrintf("Telemetry: dumping all events  [ %d in use, %d total, full(%d) empty(%d) head(%d) tail(%d) ]\n", 
        iNumEvents, iMaxEvents, iFull, iEmpty, iHead, iTail);
    TelemetryApiEventsPrint(pApp->pTelemetryApiRef);
    
    ZPrintf("\nTelemetry: dumping all filters [ %d in use, %d total ]\n", iNumFilters, iMaxFilters);
    TelemetryApiFiltersPrint(pApp->pTelemetryApiRef);
    #endif
    
    ZPrintf("Telemetry: dump complete.\n\n");
}


/*F********************************************************************************/
/*!
    \Function _TelemetryPopFront

    \Description
        Test removal of an entry from the front of the buffer

    \Input  *pApp   - pointer to telemetry app
    \Input   argc   - argument count
    \Input  *argv[] - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryPopFront(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    TelemetryApiEventT *pTelemetryEvent;
    uint32_t uNumEvents, uLoop;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s %s <# of entries to pop from the front of the buffer, default is 1>\n", argv[0], argv[1]);
        return;
    }

    if(argc>2)
    {
        uNumEvents = atoi(argv[2]);
        pTelemetryEvent = ZMemAlloc(sizeof(TelemetryApiEventT));
    }
    else
    {
        uNumEvents = 1;
        pTelemetryEvent = NULL;
    }

    ZPrintf("Telemetry: popping front [%u] events\n", uNumEvents);
    for(uLoop = 0; uLoop < uNumEvents; uLoop++)
    {
        if(TelemetryApiPopFront(pApp->pTelemetryApiRef, pTelemetryEvent))
        {
            ZPrintf("Telemetry: Popped event:");
            #if DIRTYCODE_LOGGING
            TelemetryApiEventPrint(pTelemetryEvent, uLoop);
            #endif
        }
        else
        {
            ZPrintf("Telemetry: Could not pop event [%04u]\n", uLoop);
        }
    }

    // free if necessary
    if(pTelemetryEvent)
        ZMemFree(pTelemetryEvent);
}


/*F********************************************************************************/
/*!
    \Function _TelemetryPopBack

    \Description
        Test removal of an entry from the back of the buffer

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input  *argv[] - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryPopBack(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    TelemetryApiEventT *pTelemetryEvent;
    uint32_t uNumEvents, uLoop;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s %s <# of entries to pop from the back of the buffer, default is 1>\n", argv[0], argv[1]);
        return;
    }

    if(argc>2)
    {
        uNumEvents = atoi(argv[2]);
        pTelemetryEvent = ZMemAlloc(sizeof(TelemetryApiEventT));
    }
    else
    {
        uNumEvents = 1;
        pTelemetryEvent = NULL;
    }

    ZPrintf("Telemetry: popping back [%u] events\n", uNumEvents);
    for(uLoop = 0; uLoop < uNumEvents; uLoop++)
    {
        if(TelemetryApiPopBack(pApp->pTelemetryApiRef, pTelemetryEvent))
        {
            ZPrintf("Telemetry: Popped event:");
            #if DIRTYCODE_LOGGING
            TelemetryApiEventPrint(pTelemetryEvent, uLoop);
            #endif
        }
        else
        {
            ZPrintf("Telemetry: Could not pop event [%u]\n", uLoop);
        }
    }

    // free if necessary
    if(pTelemetryEvent)
        ZMemFree(pTelemetryEvent);
}


/*F********************************************************************************/
/*!
    \Function _TelemetryDisconnect

    \Description
        Disconnect telemetry from a lobby

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryDisconnect(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s disconnect : disconnect from the lobby\n", argv[0]);
        return;
    }

    // check for create
    if (pApp->pTelemetryApiRef == NULL)
    {
        ZPrintf("Telemetry: No TelemetryApiRef available.  Try calling create.\n");
        return;
    }

    TelemetryApiDisconnect(pApp->pTelemetryApiRef);

    ZPrintf("Telemetry: Disconnect\n");
}

/*F********************************************************************************/
/*!
    \Function _TelemetryDestroy

    \Description
        Destroy a telemetry instance

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryDestroy(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;

    if (bHelp == TRUE)
    {
        ZPrintf("    %s destroy : destroy a telemetry instance\n", argv[0]);
        return;
    }

    if(pApp->pTelemetryApiRef)
    {
        TelemetryApiDestroy(pApp->pTelemetryApiRef);
        pApp->pTelemetryApiRef = NULL;
    }

    ZPrintf("Telemetry: Destroy\n");
}


/*F********************************************************************************/
/*!
    \Function _TelemetryControl

    \Description
        Send a control message to telemetry

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryControl(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    int32_t iKind = 0, iNewVal = 0, iResult = 0;
    const char *pKind;

    if ((bHelp == TRUE) || (argc < 4))
    {
        ZPrintf("    %s %s <4-char iKind token> <new integer value>: send a control message (integer) to the telemetry module\n", argv[0], argv[1]);
        return;
    }

    if((pApp == NULL) || (pApp->pTelemetryApiRef == NULL))
    {
        ZPrintf("Please create telemetry module first.\n");
        return;
    }

    pKind   = argv[2];
    iKind   = (pKind[0] << 24) + (pKind[1] << 16) + (pKind[2] << 8) + pKind[3];
    iNewVal = atoi(argv[3]);
    iResult = TelemetryApiControl(pApp->pTelemetryApiRef, iKind, iNewVal, NULL);

    ZPrintf("TelemetryApiControl: [%c%c%c%c][%d] = [%d]\n",
        LOBBYAPI_LocalizerTokenPrintCharArray(iKind), iNewVal, iResult);

}


/*F********************************************************************************/
/*!
    \Function _TelemetryInfoInt

    \Description
        Check a status token for the telemetry module

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetryInfoInt(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    int32_t iKind = 0, iResult = 0;
    const char *pKind;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("    %s %s <4-char iKind token> : check a status token for the telemetry module\n", argv[0], argv[1]);
        return;
    }

    if((pApp == NULL) || (pApp->pTelemetryApiRef == NULL))
    {
        ZPrintf("Please create telemetry module first.\n");
        return;
    }

    pKind   = argv[2];
    iKind   = (pKind[0] << 24) + (pKind[1] << 16) + (pKind[2] << 8) + pKind[3];
    iResult = TelemetryApiStatus(pApp->pTelemetryApiRef, iKind, NULL, 0);

    ZPrintf("TelemetryInfoInt: [%c%c%c%c] = [0x%08X = %d = %u]\n",
        LOBBYAPI_LocalizerTokenPrintCharArray(iKind), iResult, iResult, iResult);

}

/*F********************************************************************************/
/*!
    \Function _TelemetrySendFileCallback

    \Description
        Sends the contents of a text file on the target system to the telemetry server.

    \Input  *pTelemetryApiRef - reference to the telemetry module
    \Input  *pUserData  - referece to our TelemetryAppT object
    \Input  iTransactionId - the transaction id returned by the server; -1 if error.
    
    \Output None

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
static void _TelemetrySendFileCallback(TelemetryApiRefT *pTelemetryApiRef, void* pUserData, int32_t iTransactionId)
{
    TelemetryApiSendFileDataT *pSendFileData = (TelemetryApiSendFileDataT*)pUserData;
    
    pSendFileData->iTransactionId = iTransactionId;
    if (pSendFileData->iTransactionId < 0)
    {
        ZPrintf("TelemetrySendFile: failed to start telemetry transaction\n");
        ZFileClose(pSendFileData->iFileId);
        pSendFileData->iTransactionState = TRANS_NOT_INUSE;
        return;
    }
    pSendFileData->iTransactionState = TRANS_SEND;
}


/*F********************************************************************************/
/*!
    \Function _TelemetrySendFile

    \Description
        Sends the contents of a text file on the target system to the telemetry server.

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
static void _TelemetrySendFile(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    int32_t iBufferNum;
    
    // display help if necessary
    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("    %s %s <filename>: send a file to the telemetry server.\n", argv[0], argv[1]);
        return;
    }

    // check module reference
    if (pApp == NULL)
    {
        ZPrintf("TelemetrySendFile: pApp reference is invalid\n");
        return;
    }

    for (iBufferNum=0; iBufferNum < _TELEMETRY_MAX_SENDFILE_BUFFERS; iBufferNum++)
    {
        // look for an open buffer to use.
        if (pApp->SendFileData[iBufferNum].iTransactionState == TRANS_NOT_INUSE)
        {
            // make sure we don't clobber ourselves.
            pApp->SendFileData[iBufferNum].iTransactionState = TRANS_BEG;

            // open the file we're sending
            pApp->SendFileData[iBufferNum].iFileId = ZFileOpen(argv[2], ZFILE_OPENFLAG_RDONLY);
            if (pApp->SendFileData[iBufferNum].iFileId < 0)
            {
                ZPrintf("TelemetrySendFile: failed to open file\n");
                pApp->SendFileData[iBufferNum].iTransactionState = TRANS_NOT_INUSE;
                return;
            }

            // start the transaction
            TelemetryApiBeginTransaction(pApp->pTelemetryApiRef, _TelemetrySendFileCallback, &pApp->SendFileData[iBufferNum]);

            // everything is working - the rest of the transmission will occur in _CmdTelemetryCB
            return;
        }
    }

    ZPrintf("TelemetrySendFile: not enough buffers to send file; try again when other transactions are completed.\n");
}

/*F********************************************************************************/
/*!
    \Function _TelemetryTransStartCallback

    \Description
        Callback from TelemetryTransStart.

    \Input  *pTelemetryApiRef - reference to the telemetry module
    \Input  *pUserData  - referece to our TelemetryAppT object
    \Input  iTransactionId - the transaction id returned by the server; -1 if error.
    
    \Output None

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
static void _TelemetryTransStartCallback(TelemetryApiRefT *pTelemetryApiRef, void* pUserData, int32_t iTransactionId)
{
    TelemetryApiSendFileDataT *pSendFileData = (TelemetryApiSendFileDataT*)pUserData;
    
    pSendFileData->iTransactionId = iTransactionId;
    if (pSendFileData->iTransactionId < 0)
    {
        ZPrintf("TelemetryTransStart: failed to start telemetry transaction\n");
        pSendFileData->iTransactionState = TRANS_NOT_INUSE;
        return;
    }
    ZPrintf("TelemetryTransStart - transaction %d started\n", pSendFileData->iTransactionId);
    pSendFileData->iTransactionState = TRANS_SEND;
}

/*F********************************************************************************/
/*!
    \Function _TelemetryTransStart

    \Description
        Start a telemetry transaction

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
static void _TelemetryTransStart(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    
    // display help if necessary
    if ((bHelp == TRUE) || (argc < 2))
    {
        ZPrintf("    %s %s: begin a transaction with the telemetry server.\n", argv[0], argv[1]);
        return;
    }

    // check module reference
    if (pApp == NULL)
    {
        ZPrintf("TelemetryTransStart: pApp reference is invalid\n");
        return;
    }

    if (pApp->TransactionData.iTransactionState != TRANS_NOT_INUSE)
    {
        ZPrintf("TelemetryTransStart: buffer in use; end the current transaction and try again.\n");
        return;
    }

    // make sure we don't clobber ourselves.
    pApp->TransactionData.iTransactionState = TRANS_BEG;

    // start the transaction
    TelemetryApiBeginTransaction(pApp->pTelemetryApiRef, _TelemetryTransStartCallback, &pApp->TransactionData);
}

/*F********************************************************************************/
/*!
    \Function _TelemetryTransSend

    \Description
        Send a telemetry transaction data

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
static void _TelemetryTransSend(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    
    // display help if necessary
    /* DTCC */
    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("    %s %s <data>: send transaction data to the telemetry server.\n", argv[0], argv[1]);
        return;
    }

    // check module reference
    if (pApp == NULL)
    {
        ZPrintf("TelemetryTransSend: pApp reference is invalid\n");
        return;
    }

    if (pApp->TransactionData.iTransactionState != TRANS_SEND)
    {
        ZPrintf("TelemetryTransSend: buffer in wrong state; did you start the transaction?\n");
        return;
    }

    // send the transaction data
    TelemetryApiSendTransactionData(pApp->pTelemetryApiRef, pApp->TransactionData.iTransactionId, argv[2], (int32_t)strlen(argv[2]));
}

/*F********************************************************************************/
/*!
    \Function _TelemetryTransCancel

    \Description
        Cancel a telemetry transaction

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
static void _TelemetryTransCancel(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    
    // display help if necessary
    if ((bHelp == TRUE) || (argc < 2))
    {
        ZPrintf("    %s %s: cancel transaction in-progress.\n", argv[0], argv[1]);
        return;
    }

    // check module reference
    if (pApp == NULL)
    {
        ZPrintf("TelemetryTransCancel: pApp reference is invalid\n");
        return;
    }

    if (pApp->TransactionData.iTransactionState != TRANS_SEND)
    {
        ZPrintf("TelemetryTransCancel: buffer in wrong state; did you start the transaction?\n");
        return;
    }

    // sned the transaction data
    TelemetryApiCancelTransaction(pApp->pTelemetryApiRef, pApp->TransactionData.iTransactionId);

    pApp->TransactionData.iTransactionState = TRANS_NOT_INUSE;
}


/*F********************************************************************************/
/*!
    \Function _TelemetryTransEnd

    \Description
        End a telemetry transaction

    \Input  *pApp   - pointer to telemetry app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter) First Version
*/
/********************************************************************************F*/
static void _TelemetryTransEnd(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    
    // display help if necessary
    if ((bHelp == TRUE) || (argc < 2))
    {
        ZPrintf("    %s %s: end transaction in-progress.\n", argv[0], argv[1]);
        return;
    }

    // check module reference
    if (pApp == NULL)
    {
        ZPrintf("TelemetryTransEnd: pApp reference is invalid\n");
        return;
    }

    if (pApp->TransactionData.iTransactionState != TRANS_SEND)
    {
        ZPrintf("TelemetryTransEnd: buffer in wrong state; did you start the transaction?\n");
        return;
    }

    // sned the transaction data
    TelemetryApiEndTransaction(pApp->pTelemetryApiRef, pApp->TransactionData.iTransactionId);

    pApp->TransactionData.iTransactionState = TRANS_NOT_INUSE;
}

/*F********************************************************************************/
/*!
    \Function _TelemetrySnapLoad

    \Description
        Load data from a file (in this case an internal buffer to simulate a file)

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetrySnapLoad(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    uint32_t uLoop, uBuffer;

    typedef struct TelemetrySnapLoadBufferT
    {
        uint32_t    uBufferSize;        //!< size of the entire buffer
        uint32_t    uEventsChecksum;    //!< checksum of the stored events
        uint32_t    uNumEvents;         //!< number of events
        TelemetryApiEventT Events[10];   // event buffer
    } TelemetrySnapLoadBufferT;
    
    typedef struct TelemetrySnapLoadBufferDescriptorT
    {
        char strDescriptor[32];           //!< descriptor for the buffer
        uint32_t uBufferSize;             //!< buffer size to send the load command
        TelemetrySnapLoadBufferT Buffer;     //!< the data for loading
    } TelemetrySnapLoadBufferDescriptorT;


    // some sample data for loading
    const TelemetrySnapLoadBufferDescriptorT pSnapBuffers[] =
    {
        {  "<Internal Save>",  0, { 0,0,0, { { 'NADA', 'NADA', 0, "" } } } },
        { "Normal buffer",  12 + 1*sizeof(TelemetryApiEventT), { 1, 0x64cf36cf, 1, { { 'TEST', 'ME11', 0, "Normal event"      } } } },
        { "Another buffer", 12 + 1*sizeof(TelemetryApiEventT), { 1, 0xda965019, 1, { { 'TEST', 'ME22', 0, "Another event"     } } } }
    };
    const uint32_t uNumSnapBuffers = sizeof(pSnapBuffers) / sizeof(pSnapBuffers[0]);


    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("    %s %s <buffer # to load from> : check a status token for the telemetry module\n", argv[0], argv[1]);
        ZPrintf("\t\tAvailable buffers\n");
        for(uLoop = 0; uLoop < uNumSnapBuffers; uLoop++)
        {
            ZPrintf("\t\t\t%02u : [%32s]\n", uLoop, pSnapBuffers[uLoop].strDescriptor);
        }
        return;
    }

    uBuffer = (uint32_t)atoi(argv[2]);
    if(uBuffer >= uNumSnapBuffers)
    {
        ZPrintf("Invalid buffer number.\n");
    }

    if(uBuffer == 0)
    {
        TelemetryApiSnapshotLoad(pApp->pTelemetryApiRef, pApp->pSnapshotBuffer, pApp->uSnapshotBufferSize);
    }
    else
    {
        TelemetryApiSnapshotLoad(pApp->pTelemetryApiRef, &(pSnapBuffers[uBuffer].Buffer), pSnapBuffers->uBufferSize);
    }

    if((pApp == NULL) || (pApp->pTelemetryApiRef == NULL))
    {
        ZPrintf("Please create telemetry module first.\n");
        return;
    }
}


/*F********************************************************************************/
/*!
    \Function _TelemetrySnapSave

    \Description
        Load data from a file (in this case an internal buffer to simulate a file)

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetrySnapSave(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    uint32_t uResult;

    if ((bHelp == TRUE) || (argc < 2))
    {
        ZPrintf("    %s %s : save the telemetry buffer\n", argv[0], argv[1]);
        return;
    }

    if((pApp == NULL) || (pApp->pTelemetryApiRef == NULL))
    {
        ZPrintf("Please create telemetry module first.\n");
        return;
    }

    if(pApp->pSnapshotBuffer)
    {
        ZMemFree(pApp->pSnapshotBuffer);
        pApp->pSnapshotBuffer = NULL;
        pApp->uSnapshotBufferSize = 0;
    }


    // first get a size
    pApp->uSnapshotBufferSize = TelemetryApiSnapshotSave(pApp->pTelemetryApiRef, NULL, 0);
    if(pApp->uSnapshotBufferSize == 0)
    {
        ZPrintf("Telemetry has no data to save.\n");
    }
    else
    {
        ZPrintf("Telemetry requests buffer of size [%u] bytes.\n", pApp->uSnapshotBufferSize);

        // now allocate a buffer
        pApp->pSnapshotBuffer = ZMemAlloc(pApp->uSnapshotBufferSize);

        // and get a snapshot
        uResult = TelemetryApiSnapshotSave(pApp->pTelemetryApiRef, pApp->pSnapshotBuffer, pApp->uSnapshotBufferSize);
        ZPrintf("Snapshot saved with result code [%u]\n", uResult);
    }
}

/*F********************************************************************************/
/*!
    \Function _TelemetrySnapSize

    \Description
        Determine the number of evetns in the internal snapshot buffer

    \Input  *pApp   - pointer to lobby app
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    \Input   bHelp  - 1=show help
    
    \Output None

    \Version 03/07/2007 (danielcarter)
*/
/********************************************************************************F*/
static void _TelemetrySnapSize(void *_pApp, int32_t argc, char *argv[], unsigned char bHelp)
{
    TelemetryAppT *pApp = (TelemetryAppT *)_pApp;
    uint32_t uLoop, uBuffer, uBuffSize;

    typedef struct TelemetrySnapLoadBufferT
    {
        uint32_t    uBufferSize;        //!< size of the entire buffer
        uint32_t    uEventsChecksum;    //!< checksum of the stored events
        uint32_t    uNumEvents;         //!< number of events
        TelemetryApiEventT Events[10];   // event buffer
    } TelemetrySnapLoadBufferT;
    
    typedef struct TelemetrySnapLoadBufferDescriptorT
    {
        char strDescriptor[32];           //!< descriptor for the buffer
        uint32_t uBufferSize;             //!< buffer size to send the load command
        TelemetrySnapLoadBufferT Buffer;     //!< the data for loading
    } TelemetrySnapLoadBufferDescriptorT;

    // some sample data for loading
    const TelemetrySnapLoadBufferDescriptorT pSnapBuffers[] =
    {
        {  "<Internal Save>",  0, { 0,0,0, { { 'NADA', 'NADA', 0, "" } } } },
        { "Normal buffer",  12 + 1*sizeof(TelemetryApiEventT), { 1, 0x64cf36cf, 1, { { 'TEST', 'ME11', 0, "Normal event"      } } } },
        { "Another buffer", 12 + 1*sizeof(TelemetryApiEventT), { 1, 0xda965019, 1, { { 'TEST', 'ME22', 0, "Another event"     } } } }
    };
    const uint32_t uNumSnapBuffers = sizeof(pSnapBuffers) / sizeof(pSnapBuffers[0]);

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("    %s %s <buffer # to determine size> : check a status token for the telemetry module\n", argv[0], argv[1]);
        ZPrintf("\t\tAvailable buffers\n");
        for(uLoop = 0; uLoop < uNumSnapBuffers; uLoop++)
        {
            ZPrintf("\t\t\t%02u : [%32s]\n", uLoop, pSnapBuffers[uLoop].strDescriptor);
        }
        return;
    }

    if((pApp == NULL) || (pApp->pTelemetryApiRef == NULL))
    {
        ZPrintf("Please create telemetry module first.\n");
        return;
    }

    uBuffer = (uint32_t)atoi(argv[2]);
    if(uBuffer >= uNumSnapBuffers)
    {
        ZPrintf("Invalid buffer number.\n");
    }

    if(uBuffer == 0)
    {
        uBuffSize = TelemetryApiSnapshotSize(pApp->pSnapshotBuffer, pApp->uSnapshotBufferSize);
    }
    else
    {
        uBuffSize = TelemetryApiSnapshotSize(&(pSnapBuffers[uBuffer].Buffer), pSnapBuffers->uBufferSize);
    }

    ZPrintf("telemetry: buff size = %d\n", uBuffSize);

}

/*** Public functions *************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdTelemetry
    
    \Description
        Telemetry command
    
    \Input  *argz   - unused
    \Input   argc   - argument count
    \Input *argv[]   - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 03/07/2007 (danielcarter) First Version
*/
/**************************************************************************************F*/
int32_t CmdTelemetry(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    TelemetryAppT *pApp = &_TelemetryApp;
    unsigned char bHelp;

    // handle basic help
    if ((argc < 2) || (((pCmd = T2SubCmdParse(_Telemetry_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the telemetry module\n");
        T2SubCmdUsage(argv[0], _Telemetry_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);
    return(0);
}

