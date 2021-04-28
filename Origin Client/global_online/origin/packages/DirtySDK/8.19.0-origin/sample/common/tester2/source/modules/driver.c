/*H*************************************************************************************/
/*!
    \File    driver.c

    \Description
        Reference application for 'driver' lobby functions and ConnApi.

    \Copyright
        Copyright (c) Electronic Arts 2007.    ALL RIGHTS RESERVED.

    \Version    1.0        18/10/2007 (jbrookes) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <xtl.h>


#include "platform.h"
#include "dirtysock.h"
#include "netconn.h"
#include "lobbytagfield.h"
#include "connapi.h"
#include "netgamepkt.h"
#include "netgamelink.h"
#include "netgamedist.h"
#include "voip.h"

#include "zlib.h"

#include "testerregistry.h"
#include "testersubcmd.h"
#include "testermodules.h"

#define BEGIN_TAG "<T2CODE>"

typedef struct DriverAppT
{
    uint8_t                 bZCallback;
    uint8_t                 bBool;
    /*
    ConnApiRefT             *pConnApi;
    GameManagerRefT         *pGameManager;
    VoipRefT                *pVoip;
    int32_t                 iMaxPacket;     //!< max packet size
    GameLinkStatT           LinkStat;
    uint8_t                 aScore[32];
    uint8_t                 bHosting;
    uint8_t                 bVoipStarted;
    NetGameDistRefT         *pNetGameDist;
    NetGameDistStreamT      *pNetGameDistStream;
    uint8_t                 bGameStarted;
    uint8_t                 bTrafficPaused;
    uint8_t                 bMustSendPause;
    uint32_t                iLastSendTime;
    uint32_t                iLastStreamSend;
    uint32_t                iSendPacketIndex;
    uint32_t                iRecvPacketIndex;
    int32_t                 iVerbosity;
    uint8_t                 bUseDist;
    uint8_t                 bRandomSend;
    NetGameLinkRefT         *pNetGameLinkRef;
    uint16_t                iNumberPlayers;
    int32_t                 iFramesCRC[NUMBER_CRC_FRAMES];
    int32_t                 iOurClientId;
    int32_t                 iCurrentCRC;
    uint32_t                uWhen;
    char                    strAuth[LOBBYAPI_STR_AUTH];
    char                    strRankBuffer[2048];
*/
} DriverAppT;

/*** Function Prototypes ***************************************************************/

static void _DriverCommand(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
void DriverReceived(char* buffer, int32_t length);


/*** Variables *************************************************************************/

static T2SubCmdT _Driver_Commands[] =
{
    { "command",        _DriverCommand     },
    { "",               NULL            }
};

static DriverAppT _Driver_App = { 0, 0 };

/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _DriverCmdFormat
    
    \Description
        Take an argument list and format it into a command
    
    \Input *pApp    - pointer to game app
    \Input iKind    - type of game command to execute
    \Input argc     - argument count of arguments to send with command
    \Input *argv[]  - argument list
    \Input *pExtra  - extra command param(s) to append to command
    
    \Output
        None.
            
    \Version 1.0 05/10/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _DriverCmdFormat(char *pBuf, int32_t iBufSize, int32_t argc, char *argv[], char *pExtra)
{
    int32_t iArg;

    // start with empty string
    pBuf[0] = '\0';

    // concatentate arguments to make command
    for (iArg = 0; iArg < argc; iArg++)
    {
        strcat(pBuf, argv[iArg]);
        strcat(pBuf, " ");
    }

    // add extra parameter?
    if (pExtra != NULL)
    {
        strcat(pBuf, pExtra);
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DriverCommand
    
    \Description
        Implements the "Command" command used by the driver module to extract commands
        received from the T2Driver.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
    
    \Version 1.0 10/22/2007 (jrainy) First Version
*/
/**************************************************************************************F*/
static void _DriverCommand(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    char strParams[2048];
    char buffer[2048];
    int32_t length;

    _DriverCmdFormat(strParams, sizeof(strParams), argc-2, argv+2, "");

    if (TagFieldFind(strParams, "DATA"))
    {
        length = TagFieldGetBinary(TagFieldFind(strParams, "DATA"), buffer, sizeof(buffer));

        if (length >= 0)
        {
            buffer[length] = 0;
            DriverReceived(buffer, length);
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function    _CmdDriverCb
    
    \Description
        Driver idle callback.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
    
    \Version 1.0 10/22/2007 (jrainy) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdDriverCb(ZContext *argz, int32_t argc, char *argv[])
{
    // shutdown?
    if (argc == 0)
    {
//        _DriverDestroy(NULL, 0, NULL, FALSE);
        return(0);
    }

    // update at fastest rate
    return(ZCallback(_CmdDriverCb, 1));
}

/*F*************************************************************************************/
/*!
    \Function    _DriverSendMessage
    
    \Description
        Used to sen a message back to T2Driver.
    
    \Input *buffer  - message to send
    \Input length   - length of the message
    
    \Version 1.0 10/22/2007 (jrainy) First Version
*/
/**************************************************************************************F*/
static void _DriverSendMessage(char* message, int32_t length)
{
    char buffer[2048];

    TagFieldClear(buffer, sizeof(buffer));
    TagFieldSetBinary(buffer, sizeof(buffer), "DATA", message, length);

    ZPrintf("%s%s\n", BEGIN_TAG, buffer);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    DriverReceived
    
    \Description
        Called when a message is received from T2Driver.
    
    \Input *buffer  - message received
    \Input length   - length of the message
    
    \Version 1.0 10/22/2007 (jrainy) First Version
*/
/**************************************************************************************F*/
void DriverReceived(char* buffer, int32_t length)
{
    ZPrintf("Driver: Received %s\n", buffer);

    _DriverSendMessage("Thanks", 6);
}


/*F*************************************************************************************/
/*!
    \Function    CmdDriver
    
    \Description
        Game command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 05/10/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
int32_t CmdDriver(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    DriverAppT *pApp = &_Driver_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Driver_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   %s: manage game sessions using gamemanager, connapi, and lobby server game commands\n", argv[0]);
        T2SubCmdUsage(argv[0], _Driver_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdDriverCb, 17));
    }
    else
    {
        return(0);
    }
}

