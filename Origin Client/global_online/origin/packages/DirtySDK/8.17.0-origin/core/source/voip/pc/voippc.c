/*H********************************************************************************/
/*!
    \File voippc.c

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 07/27/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <windows.h>

// dirtysock includes
#include "platform.h"
#include "dirtysock.h"
#include "netconn.h"

// voip includes
#include "voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "voipcodec.h"
#include "voip.h"

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define VOIP_THREAD_SLEEP_DURATION        (20)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    VoipCommonRefT          Common;             //!< cross-platform voip data (must come first!)

    DWORD                   dwThreadId;         //!< thread ID
    volatile int32_t        iThreadState;       //!< control variable used during thread creation and exit (0=waiting for start, 1=running, 2=shutdown)
    volatile HANDLE         hThread;            //!< thread handle used to signal shutdown

    uint32_t                bSpeakerOutput;     //!< whether we are outputting through speakers or not
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private Variables

// Public Variables

/*** Private Functions ************************************************************/


/*F*************************************************************************************************/
/*!
    \Function _VoipMicDataCb

    \Description
        Callback to handle incoming mic data.

    \Input *pVoiceData  - pointer to mic data
    \Input iDataSize    - size of mic data
    \Input uLocalIdx    - local index of user
    \Input uSendSeqn    - send sequence
    \Input *pUserData   - pointer to callback user data

    \Version 03/31/04 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipMicDataCb(const void *pVoiceData, int32_t iDataSize, uint32_t uLocalIdx, uint8_t uSendSeqn, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;
    VoipConnectionSend(&pVoip->Common.Connectionlist, pVoiceData, iDataSize, uLocalIdx, uSendSeqn);
}

/*F*************************************************************************************************/
/*!
    \Function    _VoipStatusCb

    \Description
        Callback to handle change of headset status.

    \Input iHeadset     - headset that has changed (currently ignored)
    \Input uStatus      - event identifer (4 possible values:
    \Input pUserData    - pointer to callback user data

    \Version    1.0       04/01/04 (JLB) First Version
*/
/*************************************************************************************************F*/
static void _VoipStatusCb(int32_t iHeadset, uint32_t uStatus, void *pUserData)
{
    VoipRefT *pRef = (VoipRefT *)pUserData;

    switch (uStatus)
    {
        case VOIP_PC_OUTPUT_DEVICE_ACTIVE:
            pRef->Common.Connectionlist.uLocalStatus |= VOIP_LOCAL_OUTPUTDEVICEOK;
            break;
        case VOIP_PC_OUTPUT_DEVICE_INACTIVE:
            pRef->Common.Connectionlist.uLocalStatus &= ~VOIP_LOCAL_OUTPUTDEVICEOK;
            break;
        case VOIP_PC_INPUT_DEVICE_ACTIVE:
            pRef->Common.Connectionlist.uLocalStatus |= VOIP_LOCAL_INPUTDEVICEOK;
            break;
        case VOIP_PC_INPUT_DEVICE_INACTIVE:
            pRef->Common.Connectionlist.uLocalStatus &= ~VOIP_LOCAL_INPUTDEVICEOK;
            break;
        default:
            break;
    }

    if ( (pRef->Common.Connectionlist.uLocalStatus & VOIP_LOCAL_INPUTDEVICEOK) &&
         (pRef->Common.Connectionlist.uLocalStatus & VOIP_LOCAL_OUTPUTDEVICEOK) )
    {
        NetPrintf(("voippc: valid voice input/output devices combination found\n"));
        pRef->Common.Connectionlist.uLocalStatus |= VOIP_LOCAL_HEADSETOK;
    }
    else
    {
        NetPrintf(("voippc: currently no valid voice input/output devices combination\n"));
        pRef->Common.Connectionlist.uLocalStatus &= ~VOIP_LOCAL_HEADSETOK;
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipThread

    \Description
        Main Voip thread that updates all active connection and calls the
        XHV Engine's DoWork() function.

    \Input *pParam      - pointer to voip module state

    \Version 1.0 03/19/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _VoipThread(void *pParam)
{
    VoipRefT *pVoip = (VoipRefT *)pParam;
    uint32_t uProcessCt = 0;
    uint32_t uTime0, uTime1;
    int32_t iSleep;

    // wait until we're started up
    while(pVoip->iThreadState == 0)
    {
        NetConnSleep(VOIP_THREAD_SLEEP_DURATION);
    }

    // execute until we're killed
    while(pVoip->iThreadState == 1)
    {
        uTime0 = NetTick();

        NetCritEnter(&pVoip->Common.ThreadCrit);

        VoipConnectionUpdate(&pVoip->Common.Connectionlist);
        VoipHeadsetProcess(pVoip->Common.pHeadset, uProcessCt++);

        NetCritLeave(&pVoip->Common.ThreadCrit);

        uTime1 = NetTick();

        iSleep = VOIP_THREAD_SLEEP_DURATION - NetTickDiff(uTime1, uTime0);

        // wait for next tick
        if (iSleep >= 0)
        {
            NetConnSleep(iSleep);
        }
        else
        {
            NetPrintf(("voippc: [WARNING] Overall voip thread update took %d ms\n", NetTickDiff(uTime1, uTime0)));
        }
    }

    // indicate we're exiting
    pVoip->hThread = 0;
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iMaxLocal    - maximum number of local users supported

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iMaxLocal)
{
    VoipRefT *pVoip;

    // common startup
    if ((pVoip = VoipCommonStartup(iMaxPeers, iMaxLocal, sizeof(*pVoip), _VoipMicDataCb, _VoipStatusCb)) == NULL)
    {
        return(NULL);
    }

    // create worker thread
    pVoip->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_VoipThread, pVoip, 0, &pVoip->dwThreadId);

    // crank priority for best responsiveness
    SetThreadPriority(pVoip->hThread, THREAD_PRIORITY_HIGHEST);

    // tell thread to start executing
    pVoip->iThreadState = 1;

    // return to caller
    return(pVoip);
}

/*F********************************************************************************/
/*!
    \Function   VoipShutdown

    \Description
        Release all VoIP resources -- DEPRECATE in V9

    \Input  *pVoip      - module state from VoipStartup

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipShutdown(VoipRefT *pVoip)
{
    VoipShutdown2(pVoip, 0);
}

/*F********************************************************************************/
/*!
    \Function   VoipShutdown2

    \Description
        Release all VoIP resources.

    \Input *pVoip           - module state from VoipStartup
    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipShutdown2(VoipRefT *pVoip, uint32_t uShutdownFlags)
{
    // make sure we're really started up
    if (VoipGetRef() == NULL)
    {
        NetPrintf(("voippc: module shutdown called when not in a started state\n"));
        return;
    }

    // tell thread to shut down and wait for shutdown confirmation
    for (pVoip->iThreadState = 2; pVoip->hThread != 0; )
    {
        NetPrintf(("voippc: waiting for shutdown\n"));
        NetConnSleep(VOIP_THREAD_SLEEP_DURATION);
    }

    // unregister local talker

    // common shutdown (must come last as this frees the memory)
    VoipCommonShutdown(&pVoip->Common);
}

/*F********************************************************************************/
/*!
    \Function   VoipSetLocalUser

    \Description
        Set local user ID.

    \Input *pVoip      - module state from VoipStartup
    \Input *pUserID    - local user ID
    \Input bRegister   - ignored

    \Notes
        The local user ID is typically a DirtyAddrT when VoIP is used in
        conjunction with DirtySock, but may be any uniquely identifying
        null-terminated string less than or equal to 20 characters in
        length.

    \Version 1.0 04/15/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, const char *pUserID, uint32_t bRegister)
{
    NetPrintf(("voippc: setting local user to '%s'\n", pUserID));
    VOIP_CopyUser(&pVoip->Common.Connectionlist.LocalUsers[0], (VoipUserT *)pUserID);
}

/*F********************************************************************************/
/*!
    \Function   VoipStatus

    \Description
        Return status.

    \Input *pVoip   - module state from VoipStartup
    \Input iSelect  - status selector
    \Input iValue   - selector-specific
    \Input *pBuf    - [out] storage for selector-specific output
    \Input iBufSize - size of output buffer

    \Output
        int32_t     - status-specific data

    \Notes
        iSelect can be one of the following:

        \verbatim
            No PC-specific VoipStatus() selectors
        \endverbatim

        Unhandled selectors are passed through to VoipCommonStatus(), and
        further to VoipHeadsetStatus() if unhandled there.

    \Version 1.0 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    return(VoipCommonStatus(&pVoip->Common, iSelect, iValue, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function   VoipControl

    \Description
        Set control options.

    \Input *pVoip   - module state from VoipStartup
    \Input iControl - status selector
    \Input iValue   - selector-specific
    \Input *pValue  - register value

    \Output
        int32_t     - selector-specific data

    \Notes
        iControl can be one of the following:

        \verbatim
            'creg' - register codec
            'plvl' - set the output power level
            'qual' - set codec quality
        \endverbatim

        Unhandled selectors are passed through to VoipCommonControl(), and
        further to VoipHeadsetControl() if unhandled there.

    \Version 1.0 04/08/2007 (cadam)
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'cdec')
    {
        int32_t iResult;
        // if switching codecs we require sole access to thread critical section
        NetCritEnter(&pVoip->Common.ThreadCrit);
        iResult = VoipHeadsetControl(pVoip->Common.pHeadset, iControl, iValue, 0, pValue);
        NetCritLeave(&pVoip->Common.ThreadCrit);
        return(iResult);
    }
    if (iControl == 'creg')
    {
        VoipCodecRegister(iValue, pValue);
        return(0);
    }
    if (iControl == 'plvl')
    {
        int32_t iFractal;
        float fValue = *(float *)pValue;
        if ((fValue >= 16.0f) || (fValue < 0.0f))
        {
            NetPrintf(("voippc: setting power level failed; value must be less than 16 and greater than or equal to 0."));
            return(-1);
        }
        iFractal = (int32_t)(fValue * (1 << VOIP_CODEC_OUTPUT_FRACTIONAL));
        VoipCodecControl(-1, iControl, iFractal, 0, NULL);
        return(0);
    }
    if (iControl == 'qual')
    {
        VoipCodecControl(VOIP_CODEC_ACTIVE, iControl, iValue, 0, NULL);
        return(0);
    }

    // let VoipCommonControl take a shot at it
    return(VoipCommonControl(&pVoip->Common, iControl, iValue, pValue));
}
