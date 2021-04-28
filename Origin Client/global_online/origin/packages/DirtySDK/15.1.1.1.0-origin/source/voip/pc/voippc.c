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

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"

// voip includes
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "DirtySDK/voip/voipcodec.h"
#include "DirtySDK/voip/voip.h"

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

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

    \Input *pVoiceData      - pointer to mic data
    \Input iDataSize        - size of mic data
    \Input *pMetaData       - pointer to platform-specific metadata (can be NULL)
    \Input iMetaDataSize    - size of platform-specifici metadata
    \Input uLocalIdx        - local index of user
    \Input uSendSeqn        - send sequence
    \Input *pUserData       - pointer to callback user data

    \Version 03/31/04 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipMicDataCb(const void *pVoiceData, int32_t iDataSize, const void *pMetaData, int32_t iMetaDataSize, uint32_t uLocalIdx, uint8_t uSendSeqn, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;
    VoipConnectionSend(&pVoip->Common.Connectionlist, pVoiceData, iDataSize, pMetaData, iMetaDataSize, uLocalIdx, uSendSeqn);
}

/*F*************************************************************************************************/
/*!
    \Function    _VoipStatusCb

    \Description
        Callback to handle change of headset status.

    \Input iLocalUserIndex - headset that has changed (currently ignored)
    \Input bStatus         - if TRUE the headset was inserted, else if FALSE it was removed
    \Input eUpdate         - functionality of the headset (input, output or both)
    \Input *pUserData      - pointer to callback user data

    \Version    1.0       04/01/04 (JLB) First Version
*/
/*************************************************************************************************F*/
static void _VoipStatusCb(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData)
{
    VoipRefT *pRef = (VoipRefT *)pUserData;

    if (bStatus > 0)
    {
        // Set the appropriate flags
        if (eUpdate == VOIP_HEADSET_STATUS_INPUT || eUpdate == VOIP_HEADSET_STATUS_INOUT)
        {
            pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= VOIP_LOCAL_USER_INPUTDEVICEOK;
        }

        if (eUpdate == VOIP_HEADSET_STATUS_OUTPUT || eUpdate == VOIP_HEADSET_STATUS_OUTPUT)
        {
            pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= VOIP_LOCAL_USER_OUTPUTDEVICEOK;
        }
    }
    else
    {
        // Reset the appropriate flags
        if (eUpdate == VOIP_HEADSET_STATUS_INPUT || eUpdate == VOIP_HEADSET_STATUS_INOUT)
        {
            pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~VOIP_LOCAL_USER_INPUTDEVICEOK;
        }

        if (eUpdate == VOIP_HEADSET_STATUS_OUTPUT || eUpdate == VOIP_HEADSET_STATUS_OUTPUT)
        {
            pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~VOIP_LOCAL_USER_OUTPUTDEVICEOK;
        }
    }

    if ( (pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] & VOIP_LOCAL_USER_INPUTDEVICEOK) &&
         (pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] & VOIP_LOCAL_USER_OUTPUTDEVICEOK) )
    {
        NetPrintf(("voippc: valid voice input/output devices combination found\n"));
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= VOIP_LOCAL_USER_HEADSETOK;
    }
    else
    {
        NetPrintf(("voippc: currently no valid voice input/output devices combination\n"));
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~VOIP_LOCAL_USER_HEADSETOK;
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

        // update status of remote users
        VoipCommonUpdateRemoteStatus(&pVoip->Common);

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
    \Input iMaxLocal    - maximum number of local users supported (ignored because MLU not supported on PC)
    \Input iData        - platform-specific - unused for PC

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iMaxLocal, int32_t iData)
{
    VoipRefT *pVoip;
    VoipCommonRefT *pVoipCommon;

    // common startup
    if ((pVoip = VoipCommonStartup(iMaxPeers, 1, sizeof(*pVoip), _VoipMicDataCb, _VoipStatusCb, iData)) == NULL)
    {
        return(NULL);
    }
    pVoipCommon = (VoipCommonRefT *)pVoip;

    // bPrivileged is always TRUE on PC
    pVoipCommon->bPrivileged = TRUE;

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
        Release all VoIP resources.

    \Input *pVoip           - module state from VoipStartup
    \Input uShutdownFlags   - NET_SHUTDOWN_* flags

    \Version 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags)
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
        Register/unregister specified local user with the voip sub-system.

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PC)
    \Input bRegister        - ignored

    \Version 04/15/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister)
{
    NetCritEnter(&pVoip->Common.ThreadCrit);

    if (bRegister)
    {
        if (VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[0]))
        {
            int32_t iAddr;

            if ((iAddr = NetConnStatus('addr', 0, NULL, 0)) >= 0 )
            {
                VoipUserT voipUser;

                memset(&voipUser, 0, sizeof(voipUser));
                ds_snzprintf(voipUser.strUserId, sizeof(voipUser.strUserId), "$%08x", (uint32_t)iAddr);

                NetPrintf(("voippc: registering a local user %s\n", voipUser.strUserId));
                VOIP_CopyUser(&pVoip->Common.Connectionlist.LocalUsers[0], &voipUser);
            }
            else
            {
                NetPrintf(("voippc: error getting the ip address of for local user\n"));
            }
        }
        else
        {
            NetPrintf(("voippc: warning -- ignoring attempt to register local user because user %s is already registered\n", pVoip->Common.Connectionlist.LocalUsers[0].strUserId));
        }
    }
    else
    {
        if (!VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[0]))
        {
            // if a participating user demote him first 
            if (pVoip->Common.Connectionlist.aIsParticipating[0] == TRUE)
            {
                VoipActivateLocalUser(pVoip, 0, FALSE);
            }

            NetPrintf(("voippc: unregistering local user %s\n", pVoip->Common.Connectionlist.LocalUsers[0].strUserId));
            memset(&pVoip->Common.Connectionlist.LocalUsers[0], 0, sizeof(pVoip->Common.Connectionlist.LocalUsers[0]));
        }
        else
        {
            NetPrintf(("voippc: warning -- ignoring attempt to unregister local user because it is not yet registered\n"));
        }
    }

    NetCritLeave(&pVoip->Common.ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function   VoipActivateLocalUser

    \Description
        Promote/demote specified local user to/from "participating" state

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PC)
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
void VoipActivateLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint8_t bActivate)
{
    NetCritEnter(&pVoip->Common.ThreadCrit);

    if (!VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[0]))
    {
        if (bActivate == TRUE)
        {
            NetPrintf(("voippc: promoting user %s to participating state\n", &pVoip->Common.Connectionlist.LocalUsers[0].strUserId));
            pVoip->Common.Connectionlist.aIsParticipating[0] = TRUE;
        }
        else
        {
            NetPrintf(("voippc: demoting user %s from participating state\n", &pVoip->Common.Connectionlist.LocalUsers[0].strUserId));
            pVoip->Common.Connectionlist.aIsParticipating[0] = FALSE;
        }
        VoipHeadsetControl(pVoip->Common.pHeadset, 'aloc', 0, bActivate, &pVoip->Common.Connectionlist.LocalUsers[0]);
    }
    else
    {
        NetPrintf(("voippc: VoipActivateLocalUser() failed because no local user registered\n"));
    }

    NetCritLeave(&pVoip->Common.ThreadCrit);
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
        if ((fValue >= 20.0f) || (fValue < 0.0f))
        {
            NetPrintf(("voippc: setting power level failed; value must be less than 20 and greater than or equal to 0."));
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
