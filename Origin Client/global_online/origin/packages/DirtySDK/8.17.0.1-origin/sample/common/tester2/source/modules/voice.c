/*H********************************************************************************/
/*!
    \File voice.c

    \Description
        Test Voice over IP

    \Copyright
        Copyright (c) Electronic Arts 2004-2005.  ALL RIGHTS RESERVED.

    \Version 07/22/2004 (jbrookes) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#ifdef _WIN32
 #include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "platform.h"
#include "dirtysock.h"
#include "dirtymem.h"
#include "voip.h"

#include "mp3play.h"

#include "zlib.h"
#include "zfile.h"
#include "testersubcmd.h"
#include "testermodules.h"

#include "voipcodec.h"
#if DIRTYCODE_PLATFORM == DIRTYCODE_PC
 #include "..\..\pc\resource\t2hostresource.h"
 #include "voipspeex.h"
#endif

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct VoiceAppT     // gamer daemon state
{
    VoipRefT *pVoip;
    MP3PlayStateT *pPlayer;
    uint32_t bZCallback;
    int32_t iAddress;
    int32_t iConnID;
    ZFileT iOutFile;
    int32_t iOutSamples;
    uint8_t bRecording;
} VoiceAppT;

/*** Function Prototypes **********************************************************/

static void _VoiceCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceCodecControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceRecord(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceVolume(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _Voice_Commands[] =
{
    { "create",     _VoiceCreate        },
    { "destroy",    _VoiceDestroy       },
    { "connect",    _VoiceConnect       },
    { "cdec",       _VoiceCodecControl  },
    { "ctrl",       _VoiceControl       },
    { "record",     _VoiceRecord        },
    { "volume",     _VoiceVolume        },
    { "",           NULL                },
};

static VoiceAppT _Voice_App = { NULL, NULL, 0, 0, 0, (ZFileT)NULL, 0, 0 };

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipCodecGetIntArg

    \Description
        Get fourcc/integer from command-line argument

    \Input *pArg        - pointer to argument

    \Version 10/20/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipCodecGetIntArg(const char *pArg)
{
    int32_t iValue;

    // check for possible fourcc value
    if ((strlen(pArg) == 4) && (isalpha(pArg[0]) || isalpha(pArg[1]) || isalpha(pArg[2]) || isalpha(pArg[3])))
    {
        iValue  = pArg[0] << 24;
        iValue |= pArg[1] << 16;
        iValue |= pArg[2] << 8;
        iValue |= pArg[3];
    }
    else
    {
        iValue = strtol(pArg, NULL, 10);
    }
    return(iValue);
}

/*F********************************************************************************/
/*!
    \Function _VoiceSpkrCallback

    \Description
        Speaker callback - optional callback to receive voice data ready for
        output; this module uses this callback to capture voice data for writing
        to a file.

    \Input *pFrameData  - pointer to output data
    \Input iNumSamples  - number of samples in output data
    \Input *pUserData   - app ref

    \Version 10/31/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _VoiceSpkrCallback(int16_t *pFrameData, int32_t iNumSamples, void *pUserData)
{
    VoiceAppT *pApp = (VoiceAppT *)pUserData;
    int32_t iDataSize, iResult;

    // no file to write to?
    if (pApp->iOutFile <= 0)
    {
        return;
    }

    // write audio to output file
    iDataSize = iNumSamples * sizeof(*pFrameData);
    if ((iResult = ZFileWrite(pApp->iOutFile, pFrameData, iDataSize)) != iDataSize)
    {
        ZPrintf("voice: error %d writing %d samples to file\n", iResult, iNumSamples);
    }
    else
    {
        pApp->iOutSamples += iNumSamples;
        ZPrintf("voice: wrote %d samples to output file (%d total)\n", iNumSamples, pApp->iOutSamples);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdVoiceDevSelectProc

    \Description
        Voice device select for PC,

    \Input win      - window handle
    \Input msg      - window message
    \Input wparm    - message parameter
    \Input lparm    - message parameter

    \Output
        LRESULT     - FALSE

    \Version 07/22/2004 (jbrookes)
*/
/********************************************************************************F*/
#if DIRTYCODE_PLATFORM == DIRTYCODE_PC
static LRESULT CALLBACK _CmdVoiceDevSelectProc(HWND win, UINT msg, WPARAM wparm, LPARAM lparm)
{
    // handle init special (create the class)
    if (msg == WM_INITDIALOG)
    {
        char pDeviceName[64];
        int32_t iDevice, iNumDevices;
        VoipRefT *pVoip;

        pVoip = VoipGetRef();

        // add input devices to combo box and select the first item
        iNumDevices = VoipStatus(pVoip, 'idev', -1, NULL, 0);
        for (iDevice = 0; iDevice < iNumDevices; iDevice++)
        {
            VoipStatus(pVoip, 'idev', iDevice, pDeviceName, 64);
            SendDlgItemMessage(win, IDC_VOICEINP, CB_ADDSTRING, 0, (LPARAM)pDeviceName);
        }
        // select default input device, if available; otherwise just pick the first
        if ((iDevice = VoipStatus(pVoip, 'idft', 0, NULL, 0)) == -1)
        {
            iDevice = 0;
        }
        SendDlgItemMessage(win, IDC_VOICEINP, CB_SETCURSEL, iDevice, 0);

        // add output devices to combo box and select the first item
        iNumDevices = VoipStatus(pVoip, 'odev', -1, NULL, 0);
        for (iDevice = 0; iDevice < iNumDevices; iDevice++)
        {
            VoipStatus(pVoip, 'odev', iDevice, pDeviceName, 64);
            SendDlgItemMessage(win, IDC_VOICEOUT, CB_ADDSTRING, 0, (LPARAM)pDeviceName);
        }
        // select default output device, if available; otherwise just pick the first
        if ((iDevice = VoipStatus(pVoip, 'odft', 0, NULL, 0)) == -1)
        {
            iDevice = 0;
        }
        SendDlgItemMessage(win, IDC_VOICEOUT, CB_SETCURSEL, iDevice, 0);
    }

    // handle ok
    if ((msg == WM_COMMAND) && (LOWORD(wparm) == IDOK))
    {
        VoipRefT *pVoip;
        int32_t iInpDev, iOutDev;

        pVoip = VoipGetRef();

        iInpDev = SendDlgItemMessage(win, IDC_VOICEINP, CB_GETCURSEL, 0, 0);
        iOutDev = SendDlgItemMessage(win, IDC_VOICEOUT, CB_GETCURSEL, 0, 0);

        VoipControl(pVoip, 'idev', iInpDev, NULL);
        VoipControl(pVoip, 'odev', iOutDev, NULL);

        DestroyWindow(win);
    }

    // let windows handle
    return(FALSE);
}
#endif

/*F********************************************************************************/
/*!
    \Function _VoiceDestroyApp

    \Description
        Destroy app, freeing modules.

    \Input *pApp    - app state

    \Output
        None.

    \Version 12/12/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _VoiceDestroyApp(VoiceAppT *pApp)
{
    if (pApp->pVoip)
    {
        VoipShutdown(pApp->pVoip);
    }
    memset(pApp, 0, sizeof(*pApp));
}

/*

    Voice Commands

*/

/*F*************************************************************************************/
/*!
    \Function _VoiceCreate

    \Description
        Voice subcommand - create voice module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 12/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    char strUniqueId[32];
    uint32_t uLocalAddr;
    int32_t iMaxPeers;
    const char *pOpt;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create <maxpeers>\n", argv[0]);
        return;
    }

    // allow setting of max number of remote clients, defaulting to one
    iMaxPeers = (argc == 3) ? strtol(argv[2], NULL, 10) : 1;

    // start up voice module if it isn't already started
    if ((pApp->pVoip = VoipGetRef()) == NULL)
    {
        pApp->pVoip = VoipStartup(iMaxPeers, 1);
    }

    // register and select speex (for PC)
    #if DIRTYCODE_PLATFORM == DIRTYCODE_PC
    VoipControl(pApp->pVoip, 'creg', 'spex', (void *)&VoipSpeex_CodecDef);
    VoipControl(pApp->pVoip, 'cdec', 'spex', NULL);
    #endif

    // set speaker callback
    VoipSpkrCallback(pApp->pVoip, _VoiceSpkrCallback, pApp);

    // set quality?
    if ((argc > 2) && ((pOpt = strstr(argv[2], "-qual")) != NULL))
    {
        int32_t iQuality;
        pOpt += strlen("-qual") + 1;
        iQuality = strtol(pOpt, NULL, 10);
        VoipControl(pApp->pVoip, 'qual', iQuality, NULL);
    }

    // set our clientId and uniqueId, using local address for both
    uLocalAddr = SocketInfo(NULL, 'addr', 0, NULL, 0);
    VoipControl(pApp->pVoip, 'clid', uLocalAddr, NULL);
    snzprintf(strUniqueId, sizeof(strUniqueId), "$%08x", uLocalAddr);
    VoipSetLocalUser(pApp->pVoip, strUniqueId, TRUE);

    // select output device (PC only)
    #if DIRTYCODE_PLATFORM == DIRTYCODE_PC
    ShowWindow(CreateDialogParam(GetModuleHandle(NULL), "VOICEDEVSELECT", HWND_DESKTOP, (DLGPROC)_CmdVoiceDevSelectProc,0), TRUE);
    #endif
}

/*F*************************************************************************************/
/*!
    \Function _VoiceDestroy

    \Description
        Voice subcommand - destroy voice module

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 12/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    _VoiceDestroyApp(pApp);
}

/*F*************************************************************************************/
/*!
    \Function _VoiceConnect

    \Description
        Voice subcommand - connect to peer

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 12/12/2005 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;

    if ((bHelp == TRUE) || (argc > 3))
    {
        ZPrintf("   usage: %s connect <address>\n", argv[0]);
        return;
    }

    if (argc == 3)
    {
        uint32_t uRemoteAddr = SocketInTextGetAddr(argv[2]);
        char strUniqueId[32];

        // disable loop-back mode, in case it was previously set
        VoipControl(pApp->pVoip, 'loop', FALSE, NULL);

        // create unique identifier for remote user, using their IP address
        snzprintf(strUniqueId, sizeof(strUniqueId), "$%08x", uRemoteAddr);

        // log connection attempt
        ZPrintf("%s: connecting to %a (uniqueId=%s, clientId=0x%08x)\n", argv[0], uRemoteAddr, strUniqueId, uRemoteAddr);

        // start connect to remote user
        pApp->iConnID = VoipConnect(pApp->pVoip, VOIP_CONNID_NONE, strUniqueId, uRemoteAddr, VOIP_PORT, VOIP_PORT, uRemoteAddr, 0);
    }
    else
    {
        // set loopback
        ZPrintf("%s: enabling loopback\n", argv[0]);
        VoipControl(pApp->pVoip, 'loop', TRUE, NULL);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceCodecControl

    \Description
        Voice control subcommand - set control options

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 05/06/2011 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceCodecControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s cdec <ident> <cmd> <arg1> [arg2]\n", argv[0]);
        return;
    }

    if ((argc > 4) && (argc < 7))
    {
        int32_t iIdent, iCmd, iValue = 0, iValue2 = 0;

        iIdent = _VoipCodecGetIntArg(argv[2]);
        iCmd = _VoipCodecGetIntArg(argv[3]);

        if (argc > 4)
        {
            iValue = _VoipCodecGetIntArg(argv[4]);
        }
        if (argc > 5)
        {
            iValue2 = _VoipCodecGetIntArg(argv[5]);
        }

        VoipCodecControl(iIdent, iCmd, iValue, iValue2, NULL);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceControl

    \Description
        Voice control subcommand - set control options

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 05/05/2011 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s ctrl <cmd> <arg>\n", argv[0]);
        return;
    }

    if ((argc > 2) && (argc < 5))
    {
        int32_t iCmd, iValue = 0;

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        if (argc > 3)
        {
            // fourcc?
            if ((strlen(argv[3]) == 4) && (isalpha(argv[3][0]) || isalpha(argv[3][1]) ||
                 isalpha(argv[3][2]) || isalpha(argv[3][3])))
            {
                iValue  = argv[3][0] << 24;
                iValue |= argv[3][1] << 16;
                iValue |= argv[3][2] << 8;
                iValue |= argv[3][3];
            }
            else
            {
                iValue = strtol(argv[3], NULL, 10);
            }
        }

        VoipControl(pApp->pVoip, iCmd, iValue, NULL);
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceRecord

    \Description
        Voice subcommand - start/stop recording

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 10/28/2011 (jbrookes)
*/
/**************************************************************************************F*/
static void _VoiceRecord(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint8_t bStart;

    if ((bHelp == TRUE) || (argc < 3) || (argc > 4))
    {
        ZPrintf("   usage: %s record [start|stop] <filename>\n", argv[0]);
        return;
    }

    // see if we are stopping or starting
    if (!strcmp(argv[2], "start"))
    {
        bStart = TRUE;
    }
    else if (!strcmp(argv[2], "stop"))
    {
        bStart = FALSE;
    }
    else
    {
        ZPrintf("%s: invalid start/stop argument %s\n", argv[0], argv[2]);
        return;
    }

    if (bStart)
    {
        if (pApp->bRecording)
        {
            ZPrintf("%s: cannot start, already recording\n", argv[0]);
            return;
        }
        if (argc != 4)
        {
            ZPrintf("%s: cannot record without a filename to write to\n", argv[0]);
            return;
        }
        if ((pApp->iOutFile = ZFileOpen(argv[3], ZFILE_OPENFLAG_CREATE|ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY)) < 0)
        {
            ZPrintf("%s: error %d opening file '%s' for recording\n", pApp->iOutFile, argv[0], argv[3]);
        }
        ZPrintf("%s: opened file '%s' for recording\n", argv[0], argv[2]);
        pApp->bRecording = TRUE;
        pApp->iOutSamples = 0;
    }
    else
    {
        int32_t iResult;
        ZFileT iFileId;

        if (!pApp->bRecording)
        {
            ZPrintf("%s: cannot stop, not recording\n", argv[0]);
            return;
        }

        // save fileid, clear it from ref (stop the callback from writing to it)
        iFileId = pApp->iOutFile;
        pApp->iOutFile = 0;
        pApp->bRecording = FALSE;
        ZPrintf("%s: recording stopped, %d samples (%d bytes) written\n", argv[0], pApp->iOutSamples, pApp->iOutSamples*sizeof(int16_t));

        // close the file
        if ((iResult = ZFileClose(iFileId)) < 0)
        {
            ZPrintf("%s: error %d closing recording file\n", argv[0], iResult);
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceVolume

    \Description
        Voice subcommand - set the output volume level

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output None.

    \Version 04/22/2009 (christianadam)
*/
/**************************************************************************************F*/
static void _VoiceVolume(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    float fOutputLevel;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s volume <level>\n", argv[0]);
        return;
    }

    if (argc == 3)
    {
        fOutputLevel = strtod(argv[2], NULL);
        ZPrintf("%s: setting volume to %f\n", argv[0], fOutputLevel);
        VoipControl(pApp->pVoip, 'plvl', 0, &fOutputLevel);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdVoiceCb

    \Description
        Update voice command

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output standard return value

    \Version 07/22/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdVoiceCb(ZContext *argz, int32_t argc, char *argv[])
{
    VoiceAppT *pApp = &_Voice_App;

    // check for kill
    if (argc == 0)
    {
        _VoiceDestroyApp(pApp);
        ZPrintf("%s: killed\n", argv[0]);
        return(0);
    }

    // keep running
    return(ZCallback(&_CmdVoiceCb, 16));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdVoice

    \Description
        Initiate Voice connection.

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 07/22/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdVoice(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    VoiceAppT *pApp = &_Voice_App;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Voice_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the voip module\n");
        T2SubCmdUsage(argv[0], _Voice_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmd->pFunc != _VoiceCreate) && (pApp->pVoip == NULL))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _VoiceCreate(pApp, 1, &pCreate, bHelp);
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // one-time install of periodic callback
    if (pApp->bZCallback == FALSE)
    {
        pApp->bZCallback = TRUE;
        return(ZCallback(_CmdVoiceCb, 16));
    }
    else
    {
        return(0);
    }
}
