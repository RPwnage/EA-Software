/*H********************************************************************************/
/*!
    \File voice.c

    \Description
        Test Voice over IP

    \Copyright
        Copyright (c) Electronic Arts 2004-2005.  ALL RIGHTS RESERVED.

    \Version 07/22/2004 (jbrookes)  First Version
    \Version 07/09/2012 (akirchner) Added functionality to play pre-recorded file
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#ifdef _WIN32
 #pragma warning(push,0)
 #include <windows.h>
 #pragma warning(pop)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"

#if defined(DIRTYCODE_XBOXONE)
#include "DirtySDK/DirtySock/dirtyaddr.h"
#endif

#include "mp3play.h"

#include "zlib.h"
#include "zfile.h"
#include "zmem.h"
#include "testersubcmd.h"
#include "testermodules.h"

#if !defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_XBOXONE)
 #include "DirtySDK/voip/voipcodec.h"
#endif

#if defined(DIRTYCODE_PC)
 #include "t2hostresource.h"
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
    ZFileT iFile;
    int32_t iSamples;
    uint8_t bRecording;
    uint8_t bPlaying;
    int16_t *pBuffer;
} VoiceAppT;

/*** Function Prototypes **********************************************************/

static void _VoiceCreate(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceDestroy(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceConnect(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#if !defined(DIRTYCODE_PS4)
static void _VoiceCodecControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
#endif
static void _VoiceControl(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceRecord(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoicePlay(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceResetChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceSelectChannel(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceShowChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceVolume(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

static void _VoiceConn(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceUserLocal(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceUserRemote(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceSetLocalUser(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _VoiceActivateLocalUser(void *CmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _Voice_Commands[] =
{
    { "create",     _VoiceCreate            },
    { "destroy",    _VoiceDestroy           },
    { "connect",    _VoiceConnect           },
#if !defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_XBOXONE)
    { "cdec",       _VoiceCodecControl      },
#endif
    { "ctrl",       _VoiceControl           },
    { "record",     _VoiceRecord            },
    { "play",       _VoicePlay              },
    { "resetchans", _VoiceResetChannels     },
    { "selectchan", _VoiceSelectChannel     },
    { "showchans",  _VoiceShowChannels      },
    { "volume",     _VoiceVolume            },
    { "conn",       _VoiceConn              },
    { "userlocal",  _VoiceUserLocal         },
    { "userremote", _VoiceUserRemote        },
    { "set",        _VoiceSetLocalUser      },
    { "activate",   _VoiceActivateLocalUser },

    { "",           NULL                    },
};

static VoiceAppT _Voice_App = { NULL, NULL, 0, 0, 0, (ZFileT)NULL, 0, 0, 0, NULL };

/*** Private Functions ************************************************************/

#if (!defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_XBOXONE))
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
#endif

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
    if (pApp->iFile <= 0)
    {
        return;
    }

    // write audio to output file
    iDataSize = iNumSamples * sizeof(*pFrameData);
    if ((iResult = ZFileWrite(pApp->iFile, pFrameData, iDataSize)) != iDataSize)
    {
        ZPrintf("voice: error %d writing %d samples to file\n", iResult, iNumSamples);
    }
    else
    {
        pApp->iSamples += iNumSamples;
        ZPrintf("voice: wrote %d samples to output file (%d total)\n", iNumSamples, pApp->iSamples);
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
#if defined(DIRTYCODE_PC)
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
        VoipShutdown(pApp->pVoip, 0);
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
    //char strUniqueId[32];
    uint32_t uLocalAddr;
    int32_t iMaxPeers;
    const char *pOpt;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create <maxpeers>\n", argv[0]);
        return;
    }

    // allow setting of max number of remote clients, defaulting to one
    iMaxPeers = (argc == 3) ? strtol(argv[2], NULL, 10) : 16;

    // start up voice module if it isn't already started
    if ((pApp->pVoip = VoipGetRef()) == NULL)
    {
        pApp->pVoip = VoipStartup(iMaxPeers, 0);
    }

    // register and select speex (for PC)
    #if defined(DIRTYCODE_PC)
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

    // set our clientId using local address
    uLocalAddr = SocketInfo(NULL, 'addr', 0, NULL, 0);
    VoipControl(pApp->pVoip, 'clid', uLocalAddr, NULL);

    // select output device (PC only)
    #if defined(DIRTYCODE_PC)
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

    if (pApp->pBuffer)
    {
        ZMemFree((void *) pApp->pBuffer);
        pApp->pBuffer = NULL;
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

    if ((bHelp == TRUE) || (argc > 4))
    {
        ZPrintf("   usage: %s connect <address> [rclientid]\n", argv[0]);
        return;
    }

    if (argc >= 3)
    {
        // get remote address and clientid (clientid==address if not specified)
        uint32_t uRemoteAddr = SocketInTextGetAddr(argv[2]);
        uint32_t uRemoteClientId = (argc == 4) ? (uint32_t)strtol(argv[3], NULL, 16) : uRemoteAddr;

        // disable loop-back mode, in case it was previously set
        VoipControl(pApp->pVoip, 'loop', FALSE, NULL);

        // log connection attempt
        ZPrintf("%s: connecting to %a (clientId=0x%08x)\n", argv[0], uRemoteAddr, uRemoteClientId);

        // start connect to remote user
        pApp->iConnID = VoipConnect(pApp->pVoip, VOIP_CONNID_NONE, uRemoteAddr, VOIP_PORT, VOIP_PORT, uRemoteClientId, 0);
    }
    else
    {
        // set loopback
        ZPrintf("%s: enabling loopback\n", argv[0]);
        VoipControl(pApp->pVoip, 'loop', TRUE, NULL);
    }
}

#if !defined(DIRTYCODE_PS4) && !defined(DIRTYCODE_XBOXONE)
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
#endif


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
        if (pApp->bPlaying)
        {
            ZPrintf("%s: cannot start, currently playing\n", argv[0]);
            return;
        }

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
        if ((pApp->iFile = ZFileOpen(argv[3], ZFILE_OPENFLAG_CREATE|ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY)) < 0)
        {
            ZPrintf("%s: error %d opening file '%s' for recording\n", pApp->iFile, argv[0], argv[3]);
        }
        ZPrintf("%s: opened file '%s' for recording\n", argv[0], argv[3]);
        pApp->bRecording = TRUE;
        pApp->iSamples = 0;
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
        iFileId = pApp->iFile;
        pApp->iFile = 0;
        pApp->bRecording = FALSE;
        ZPrintf("%s: recording stopped, %d samples (%d bytes) written\n", argv[0], pApp->iSamples, pApp->iSamples * sizeof(int16_t));

        // close the file
        if ((iResult = ZFileClose(iFileId)) < 0)
        {
            ZPrintf("%s: error %d closing recording file\n", argv[0], iResult);
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoicePlay

    \Description
        Voice subcommand - start/stop playing

    \Input *pCmdRef - unused
    \Input  argc    - argument count
    \Input *argv[]  - argument list

    \Version 7/9/2012 (akirchner)
*/
/**************************************************************************************F*/
static void _VoicePlay(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT * pApp = & _Voice_App;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s play [start|stop] <filename>\n", argv[0]);
        return;
    }

    if ((argc >= 3) && (!strcmp(argv[2], "start")))
    {
        int32_t iResult;

        if (pApp->bRecording)
        {
            ZPrintf("%s: cannot start, currently recording\n", argv[0]);
            return;
        }

        if (pApp->bPlaying)
        {
            ZPrintf("%s: cannot start, already playing\n", argv[0]);
            return;
        }

        if (argc != 4)
        {
            ZPrintf("   usage: %s play start <filename>\n", argv[0]);
            return;
        }

        // open file
        if ((pApp->iFile = ZFileOpen(argv[3], ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) < 0)
        {
            ZPrintf("%s: error %d opening file '%s' for playing\n", argv[0], pApp->iFile, argv[3]);
            return;
        }

        // get size
        if ((pApp->iSamples = ZFileSize(pApp->iFile)) < 0)
        {
            ZPrintf("%s: error %d invalid size of file '%s'\n", argv[0], pApp->iFile, argv[3]);
            return;
        }

        // read file
        if ((pApp->pBuffer = (int16_t *) ZMemAlloc(pApp->iSamples)) == NULL)
        {
            ZPrintf("%s: error allocating %d bytes of memory\n", argv[0], pApp->iSamples);
            return;
        }

        if ((iResult = ZFileSeek(pApp->iFile, 0, ZFILE_SEEKFLAG_SET)) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: error %d seeking begging of file '%s'\n", argv[0], iResult, argv[3]);
            return;
        }

        if (ZFileRead(pApp->iFile, (void *) pApp->pBuffer, pApp->iSamples) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: error %d reading file '%s'\n", argv[0], pApp->iFile, argv[3]);
            return;
        }

        // close file
        if ((iResult = ZFileClose(pApp->iFile)) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: error %d closing file '%s' for playing\n", argv[0], iResult, argv[3]);
            return;
        }

        if (VoipControl(pApp->pVoip, 'play', pApp->iSamples, (void *) pApp->pBuffer) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;

            ZPrintf("%s: failed to activate playing mode\n", argv[0]);
            return;
        }

        ZPrintf("%s: opened file '%s' of %d bytes for playing\n", argv[0], argv[3], pApp->iSamples);

        pApp->iFile    = 0;
        pApp->bPlaying = TRUE;
    }
    else if ((argc >= 3) && (!strcmp(argv[2], "stop")))
    {
        if (argc != 3)
        {
            ZPrintf("   usage: %s play stop\n", argv[0]);
            return;
        }

        if (!pApp->bPlaying)
        {
            ZPrintf("%s: cannot stop, not playing\n", argv[0]);
            return;
        }

        if (VoipControl(pApp->pVoip, 'play', 0, NULL) < 0)
        {
            ZMemFree((void *) pApp->pBuffer);

            ZPrintf("%s: failed to deactivate playing mode\n", argv[0]);
            return;
        }

        pApp->bPlaying = FALSE;

        if (pApp->pBuffer)
        {
            ZMemFree((void *) pApp->pBuffer);
            pApp->pBuffer = NULL;
        }

        ZPrintf("%s: playing stopped\n", argv[0]);
    }
    else
    {
        ZPrintf("%s: invalid argument. Neither start nor stop\n", argv[0]);
        return;
    }
}

/*F*************************************************************************************/
/*!
    \Function _VoiceResetChannels

    \Description
        Voice subcommand - reset channel configuration

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 02/16/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _VoiceResetChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s resetchans\n", argv[0]);
        return;
    }

    ZPrintf("%s: resetting channel config\n", argv[0]);
    VoipResetChannels(VoipGetRef(), 0);
}

/*F*************************************************************************************/
/*!
    \Function _VoiceSelectChannel

    \Description
        Voice subcommand - subscribe/unsubcribes to/from a voip channel

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 02/16/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _VoiceSelectChannel(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iChannel;
    VoipChanModeE eMode;

    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s selectchan <channel id [0,63]> <talk|listen|both|none>\n", argv[0]);
        return;
    }

    sscanf(argv[2], "%d", &iChannel);
    if ((iChannel < 0) && (iChannel > 63))
    {
        ZPrintf("%s: invalid channel id argument: %s\n", argv[0], argv[2]);
        return;
    }

    if (!strcmp(argv[3], "talk"))
    {
        eMode = VOIP_CHANSEND;
    }
    else if (!strcmp(argv[3], "listen"))
    {
        eMode = VOIP_CHANRECV;
    }
    else if (!strcmp(argv[3], "both"))
    {
        eMode = VOIP_CHANSENDRECV;
    }
    else if (!strcmp(argv[3], "none"))
    {
        eMode = VOIP_CHANNONE;
    }
    else
    {
        ZPrintf("%s: invalid channel mode argument: %s\n", argv[0], argv[3]);
        return;
    }

    ZPrintf("%s: setting channel %d:%s\n", argv[0], iChannel, argv[3]);
    VoipSelectChannel(VoipGetRef(), 0, iChannel, eMode);
}

/*F*************************************************************************************/
/*!
    \Function _VoiceShowChannels

    \Description
        Voice subcommand - show local voip channel config

    \Input *pCmdRef - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Version 02/16/2012 (mclouatre)
*/
/**************************************************************************************F*/
static void _VoiceShowChannels(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    int32_t iChannelSlot, iChannelId;
    char *pChannelModeStr;
    VoipChanModeE eChannelMode;
    char strChannelId[8];
    char strChannelSlot[32];
    char strChannelCfg[32];

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s showchans\n", argv[0]);
        return;
    }

    for (iChannelSlot = 0; iChannelSlot < VoipGroupStatus(NULL, 'chnc', 0, NULL, 0); iChannelSlot++)
    {
        // get channel id and channel mode
        iChannelId = VoipGroupStatus(NULL, 'chnl', iChannelSlot, &eChannelMode, sizeof(eChannelMode));
        // init to default
        pChannelModeStr = "UNKNOWN";

        // initialize channel mode string and 
        switch (eChannelMode)
        {
            case VOIP_CHANNONE:
                pChannelModeStr = "UNSUBSCRIBED";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "n/a");
                break;
            case VOIP_CHANSEND:
                pChannelModeStr = "TALK-ONLY";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "%03d", iChannelId);
                break;
            case VOIP_CHANRECV:
                pChannelModeStr = "LISTEN-ONLY";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "%03d", iChannelId);
                break;
            case VOIP_CHANSENDRECV:
                pChannelModeStr = "TALK+LISTEN";
                ds_snzprintf(strChannelId, sizeof(strChannelId), "%03d", iChannelId);
                break;
        }

        // build channel slot string
        ds_snzprintf(strChannelSlot, sizeof(strChannelSlot), "VOIP channel slot %d", iChannelSlot);

        // build channel config string
        ds_snzprintf(strChannelCfg, sizeof(strChannelCfg), "id=%s  mode=%s", strChannelId, pChannelModeStr);

        ZPrintf("     %s ----> %s\n", strChannelSlot, strChannelCfg);
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

    \Version 04/22/2009 (cadam)
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

static void _VoiceSetLocalUser(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    int32_t iLocalUserIndex;
    uint32_t bRegister;
    int32_t iOnOffIndex;

#if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE)
    if (bHelp == TRUE || argc != 4)
    {
        ZPrintf("   usage: %s set <local user index> <on|off>\n", argv[0]);
        return;
    }

    iLocalUserIndex = strtol(argv[2], NULL, 10);
    iOnOffIndex = 3;
#else
    if (bHelp == TRUE || argc != 3)
    {
        ZPrintf("   usage: %s set <on|off>\n", argv[0]);
        return;
    }

    iLocalUserIndex = 0;
    iOnOffIndex = 2;
#endif

    if (strcmp(argv[iOnOffIndex], "on") == 0)
    {
        bRegister = TRUE;
        ZPrintf("%s: registering local user at index %d with voip sub-system\n", argv[0], iLocalUserIndex);
    }
    else
    {
        bRegister = FALSE;
        ZPrintf("%s: unregistering local user at index %d from voip sub-system\n", argv[0], iLocalUserIndex);
    }

    VoipSetLocalUser(pApp->pVoip, iLocalUserIndex, bRegister);
}

static void _VoiceActivateLocalUser(void *CmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint32_t iLocalUserIndex;
    uint32_t bActivate;
    int32_t iOnOffIndex;

#if defined(DIRTYCODE_PS4) || defined(DIRTYCODE_XBOXONE)
    if (bHelp == TRUE || argc != 4)
    {
        ZPrintf("   usage: %s participate <local user index> <on|off> \n", argv[0]);
        return;
    }

    iLocalUserIndex = strtol(argv[2], NULL, 10);
    iOnOffIndex = 3;
#else
    if (bHelp == TRUE || argc != 3)
    {
        ZPrintf("   usage: %s participate <on|off> \n", argv[0]);
        return;
    }

    iLocalUserIndex = 0;
    iOnOffIndex = 2;
#endif

    if (strcmp(argv[iOnOffIndex], "on") == 0)
    {
        bActivate = TRUE;
        ZPrintf("%s: promoting local user %d to participating state\n", argv[0], iLocalUserIndex);
    }
    else
    {
        bActivate = FALSE;
        ZPrintf("%s: forcing local user %d out of participating state\n", argv[0], iLocalUserIndex);
    }

    VoipActivateLocalUser(pApp->pVoip, iLocalUserIndex, bActivate);
}

static void _VoiceConn(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint32_t uOutput;
    int32_t  iConnId = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s connremote <ConnId>\n", argv[0]);
        return;
    }

    if (argc == 3)
    {
        iConnId = strtod(argv[2], NULL);
        uOutput = VoipConnStatus(pApp->pVoip, iConnId);
        ZPrintf("Status = %08x\n", uOutput);
        if (uOutput == 0)
        {
            ZPrintf("No VoipConnRemote flags set\n");
        }
        else
        {
            if (uOutput & VOIP_CONN_CONNECTED)
                ZPrintf("VOIP_CONN_CONNECTED\n");
            if (uOutput & VOIP_CONN_BROADCONN)
                ZPrintf("VOIP_CONN_BROADCONN\n");
            if (uOutput & VOIP_CONN_ACTIVE)
                ZPrintf("VOIP_CONN_ACTIVE\n");
            if (uOutput & VOIP_CONN_STOPPED)
                ZPrintf("VOIP_CONN_STOPPED\n");
        }
    }
}

static void _VoiceUserLocal(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{    
    VoiceAppT *pApp = &_Voice_App;
    uint32_t uOutput;
    int32_t  iUserIndex = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s userlocal <userindex>\n", argv[0]);
        return;
    }

    if (argc == 3)
    {
        iUserIndex = strtod(argv[2], NULL);
        uOutput = VoipLocalUserStatus(pApp->pVoip, iUserIndex);
        ZPrintf("Status = %08x\n", uOutput);
        if (uOutput == 0)
        {
            ZPrintf("No VoipUserLocal flags set\n");
        }
        else
        {
            if (uOutput & VOIP_LOCAL_USER_MICON)
                ZPrintf("VOIP_LOCAL_USER_MICON\n");
            if (uOutput & VOIP_LOCAL_USER_HEADSETOK)
                ZPrintf("VOIP_LOCAL_USER_HEADSETOK\n");
            if (uOutput & VOIP_LOCAL_USER_TALKING)
                ZPrintf("VOIP_LOCAL_USER_TALKING\n");
            if (uOutput & VOIP_LOCAL_USER_SENDVOICE)
                ZPrintf("VOIP_LOCAL_USER_SENDVOICE\n");
#if defined(DIRTYCODE_PC)
            if (uOutput & VOIP_LOCAL_USER_INPUTDEVICEOK)
                ZPrintf("VOIP_LOCAL_USER_INPUTDEVICEOK\n");
            if (uOutput & VOIP_LOCAL_USER_OUTPUTDEVICEOK)
                ZPrintf("VOIP_LOCAL_USER_OUTPUTDEVICEOK\n");
#endif
        }
    }
}

static void _VoiceUserRemote(void *pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    VoiceAppT *pApp = &_Voice_App;
    uint32_t uOutput;
    int32_t  iUserIndex = 0;
    int32_t  iConnId = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s userremote <ConnId> <userindex>\n", argv[0]);
        return;
    }

    if (argc == 4)
    {
        iConnId = strtod(argv[2], NULL);
        iUserIndex = strtod(argv[3], NULL);
        uOutput = VoipRemoteUserStatus(pApp->pVoip, iConnId, iUserIndex);
        ZPrintf("Status = %08x\n", uOutput);
        if (uOutput == 0)
        {
            ZPrintf("No VoipUserRemote flags set\n");
        }
        else
        {
            if (uOutput & VOIP_REMOTE_USER_HEADSETOK)
                ZPrintf("VOIP_REMOTE_USER_HEADSETOK\n");
            if (uOutput & VOIP_REMOTE_USER_RECVVOICE)
                ZPrintf("VOIP_REMOTE_CONN_RECVVOICE\n");
        }
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
        // only create the new ref if we are not exercising VoiceControl('dcde')\VoiceControl('drat')
        // to modify the default sample rate and codec BEFORE the module creation
        if ( !((strcmp(argv[1], "ctrl") == 0) && (strcmp(argv[2], "dcde") == 0)) &&
             !((strcmp(argv[1], "ctrl") == 0) && (strcmp(argv[2], "drat") == 0)) )
        {
            char *pCreate = "create";
            ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
            _VoiceCreate(pApp, 1, &pCreate, bHelp);
        }
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
