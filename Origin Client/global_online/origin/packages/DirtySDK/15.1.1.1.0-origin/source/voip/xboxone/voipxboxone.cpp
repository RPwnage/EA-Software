/*H********************************************************************************/
/*!
    \File voipxboxone.cpp

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2013. ALL RIGHTS RESERVED.

    \Version 05/24/2013 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

// voip includes
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "voipserializationxboxone.h"

// for PartyChat
#using "Microsoft.Xbox.Services.winmd"
using namespace Windows::Xbox::Multiplayer;

/*** Defines **********************************************************************/

/*
On Xbox One, we need to guarantee that voice frames are being submitted (SubmitMix) at
a rate matching the capture interval specified when creating the game chat session, i.e. 20 ms.
Failing to do so results in static effects in voice playback. Setting VOIP_THREAD_SLEEP_DURATION
to 20 proved to not be good enough: it is too tight with the system expectation and the 
rate requirement. A value of 18 proved to eliminate the static effects. A value
smaller than 18 proved to create another unwanted effect: we sometime attempt to feed
too many voice frames back-to-back and SubmitMix() starts throwing the 
ERROR_INSUFFICIENT_BUFFER exception, which results in the _VoipThread warning that
its execution cycle takes a lot longer than 20 ms (~ 200 ms).
*/
#define VOIP_THREAD_SLEEP_DURATION        (18)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    VoipCommonRefT              Common;             //!< cross-platform voip data (must come first!)

    DWORD                       dwThreadId;         //!< thread ID
    volatile int32_t            iThreadActive;      //!< control variable used during thread creation and exit
    volatile HANDLE             hThread;            //!< thread handle used to signal shutdown
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private Variables


// Public Variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function    _VoipMicDataCb

    \Description
        Callback to handle incoming mic data.

    \Input *pVoiceData      - pointer to mic data
    \Input iDataSize        - size of mic data
    \Input *pMetaData       - pointer to platform-specific metadata (can be NULL)
    \Input iMetaDataSize    - size of platform-specifici metadata
    \Input uLocalIdx        - local user index
    \Input uSendSeqn        - send sequence
    \Input *pUserData       - pointer to callback user data

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipMicDataCb(const void *pVoiceData, int32_t iDataSize, const void *pMetaData, int32_t iMetaDataSize, uint32_t uLocalIdx, uint8_t uSendSeqn, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;
    VoipConnectionSend(&pVoip->Common.Connectionlist, (uint8_t *)pVoiceData, iDataSize, (uint8_t *)pMetaData, iMetaDataSize, uLocalIdx, uSendSeqn);
}

/*F********************************************************************************/
/*!
    \Function _VoipStatusCb

    \Description
        Callback to handle change of headset status.

    \Input iLocalUserIndex - headset that has changed
    \Input bStatus         - if TRUE the headset was inserted, else if FALSE it was removed
    \Input eUpdate         - functionality of the headset (input, output or both)
    \Input *pUserData      - pointer to callback user data

    \Notes
        It is assumed that eUpdate will always be VOIP_HEADSET_STATUS_INOUT for 
        this platform.

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipStatusCb(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData)
{
    VoipRefT *pRef = (VoipRefT *)pUserData;

    if (eUpdate != VOIP_HEADSET_STATUS_INOUT)
    {
        NetPrintf(("voipxboxone: warning, unexpected headset event for this platform!\n"));
    }

    if (bStatus)
    {
        NetPrintf(("voipxboxone: [%d] headset inserted\n", iLocalUserIndex));
        pRef->Common.uPortHeadsetStatus |= 1 << iLocalUserIndex;
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= (VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
    }
    else
    {
        NetPrintf(("voipxboxone: [%d] headset removed\n", iLocalUserIndex));
        pRef->Common.uPortHeadsetStatus &= ~(1 << iLocalUserIndex);
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~(VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipRegisterLocalTalker

    \Description
        Register the local talker on the given port.

    \Input *pVoip           - voip module reference
    \Input iLocalUserIndex  - local user index associated with the user
    \Input bRegister        - TRUE to register user, FALSE to unregister
    \Input *pVoipUser       - VoipUserT to register

    \Output
        int32_t             - 1 if the local talker was successfully registered, else 0

    \Version 04/25/2014 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipRegisterLocalTalker(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister, VoipUserT *pVoipUser)
{
    int32_t iResult;
    if ((iResult = VoipHeadsetControl(pVoip->Common.pHeadset, 'rloc', iLocalUserIndex, bRegister, pVoipUser)) == 0)
    {
        if (bRegister)
        {
            // add user to local list
            if (pVoipUser != NULL)
            {
                ds_memcpy_s(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex], sizeof(pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]), pVoipUser, sizeof(*pVoipUser));
            }
        }
        else
        {
            pVoip->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~(VOIP_LOCAL_USER_MICON|VOIP_LOCAL_USER_HEADSETOK);
            memset(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex], 0, sizeof(pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]));

        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _VoipRegisterLocalSharedTalker

    \Description
        Helper function to register/unregister a shared user

    \Input *pVoip           - voip module reference
    \Input bRegister        - TRUE to register user, FALSE to unregister

    \Version 12/10/2014 (tcho)
*/
/********************************************************************************F*/
static void _VoipRegisterLocalSharedTalker(VoipRefT *pVoip, uint32_t bRegister)
{
    if (bRegister)
    {
        // do we have a shared user? If not add one
        if (VOIP_NullUser((VoipUserT *)(&pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX])))
        {
            VoipUserT voipSharedUser;
            memset(&voipSharedUser, 0, sizeof(voipSharedUser));
            ds_snzprintf(voipSharedUser.strUserId, (int32_t)sizeof(voipSharedUser.strUserId), "shared-%d", pVoip->Common.Connectionlist.uClientId);
            ds_memcpy_s(&pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX], sizeof(pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX]), &voipSharedUser, sizeof(voipSharedUser));
            pVoip->Common.Connectionlist.aIsParticipating[VOIP_SHARED_USER_INDEX] = TRUE; // the shared user is always participating
            
            NetPrintf(("voipxboxone: local shared user added at index %d\n", VOIP_SHARED_USER_INDEX));
        }
    }
    else
    {
        // remove the shared user if there are no more local users left
        if (!VOIP_NullUser((VoipUserT *)(&pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX])))
        {
            memset(&pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX], 0, sizeof(pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX]));
            pVoip->Common.Connectionlist.aIsParticipating[VOIP_SHARED_USER_INDEX] = FALSE;
            NetPrintf(("voipxboxone: shared user removed at index %d\n", VOIP_SHARED_USER_INDEX));
        }

    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipThread

    \Description
        Main Voip thread that updates all active connections

    \Input *pParam      - pointer to voip module state

    \Output
        DWORD           - return value

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
static DWORD _VoipThread(void *pParam)
{
    VoipRefT *pVoip = (VoipRefT *)pParam;
    uint32_t uProcessCt = 0;
    uint32_t uTime0, uTime1;
    int32_t iSleep;

    // execute until we're killed AND chat session state is not in progress
    for (pVoip->iThreadActive = 1; pVoip->iThreadActive != 0 || (VoipHeadsetStatus(pVoip->Common.pHeadset, 'supd', 0, NULL, 0) && !NetConnStatus('susp', 0, NULL, 0)); )
    {
        uTime0 = NetTick();

        NetCritEnter(&pVoip->Common.ThreadCrit);

        // update status of remote users
        VoipCommonUpdateRemoteStatus(&pVoip->Common);

        VoipConnectionUpdate(&pVoip->Common.Connectionlist);

        /*
        For Xbox One, the ConnectionList's critical section (NetCrit) is entered before
        calling VoipHeadsetProcess() to guarantee thread-safety between _VoipHeadsetProcessInboundQueue()
        or _VoipHeadsetUpdateEncodersDecodersForLocalCaptureSources() being executed from the voip thread,
        and _VoipHeadsetEnqueueInboundSubPacket() being executed from the socket recv thread.
        */
        NetCritEnter(&pVoip->Common.Connectionlist.NetCrit);
        VoipHeadsetProcess(pVoip->Common.pHeadset, uProcessCt++);
        NetCritLeave(&pVoip->Common.Connectionlist.NetCrit);

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
            NetPrintf(("voipxboxone: [WARNING] Overall voip thread update took %d ms\n", NetTickDiff(uTime1, uTime0)));
        }
    }

    // indicate we're exiting
    pVoip->hThread = NULL;

    return(0);
}



/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iMaxLocal    - maximum number of local users supported
    \Input iData        - platform-specific - unused for xboxone

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iMaxLocal, int32_t iData)
{
    VoipRefT *pVoip;
    VoipCommonRefT *pVoipCommon;

    // common startup
    if ((pVoip = VoipCommonStartup(iMaxPeers, iMaxLocal, sizeof(*pVoip), _VoipMicDataCb, _VoipStatusCb, iData)) == NULL)
    {
        return(NULL);
    }
    pVoipCommon = (VoipCommonRefT *)pVoip;

    VoipSerializeInit();

    //permission is handled by the Game Chat API so bPrivileged is always TRUE
    pVoipCommon->bPrivileged = TRUE; 

    // create our own worker thread
    if ((pVoip->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_VoipThread, pVoip, 0, &pVoip->dwThreadId)) == NULL)
    {
        NetPrintf(("voipxboxone: error creating voip thread (err=%s)\n", DirtyErrGetName(GetLastError())));
        VoipShutdown(pVoip, 0);
        return(NULL);
    }

    // crank priority for best responsiveness
    SetThreadPriority(pVoip->hThread, THREAD_PRIORITY_HIGHEST);

    // wait for thread startup
    while (pVoip->iThreadActive == 0)
    {
        Sleep(1);
    }

    // tell thread to start executing
    pVoip->iThreadActive = 1;

    // add a local shared user
    _VoipRegisterLocalSharedTalker(pVoip, TRUE);

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

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags)
{
    // make sure we're really started up
    if (VoipGetRef() == NULL)
    {
        NetPrintf(("voipxboxone: module shutdown called when not in a started state\n"));
        return;
    }

    // tell thread to shut down and wait for shutdown confirmation
    for (pVoip->iThreadActive = 0; pVoip->hThread != NULL; )
    {
        NetPrintf(("voipxboxone: waiting for shutdown\n"));
        Sleep(1);
    }

    VoipSerializeTerm();

    // remove the shared user
    _VoipRegisterLocalSharedTalker(pVoip, FALSE);

    // common shutdown (must come last as this frees the memory)
    VoipCommonShutdown(&pVoip->Common);
}

/*F********************************************************************************/
/*!
    \Function VoipSetLocalUser

    \Description
        Register/unregister specified local user with the voip sub-system.

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - controller index of user
    \Input bRegister        - TRUE to register, FALSE to unregister.

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister)
{
    if ((iLocalUserIndex < VOIP_MAXLOCALUSERS) && (iLocalUserIndex >= 0))
    {
        NetCritEnter(&pVoip->Common.ThreadCrit);

        if (bRegister)
        {
            // is this slot empty?
            if (VOIP_NullUser((VoipUserT *)(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex])))
            {
                uint64_t uXuid = 0;
                VoipUserT voipUser;

                NetPrintf(("voipxboxone: enabling voice for user at index %d\n", iLocalUserIndex));

                // register the local talker
                NetConnStatus('xuid', iLocalUserIndex, &uXuid, sizeof(uXuid));
                memset(&voipUser, 0, sizeof(voipUser));
                ds_snzprintf(voipUser.strUserId, (int32_t)sizeof(voipUser.strUserId), "%llX", uXuid); 
                _VoipRegisterLocalTalker(pVoip, iLocalUserIndex, TRUE, &voipUser);
            }
            else
            {
                NetPrintf(("voipxboxone: warning -- ignoring attempt to register user at index %d because user %s is already registered at that index\n",
                    iLocalUserIndex, ((VoipUserT *)(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]))->strUserId));
            }
        }
        else
        {
            // is there a user at this slot?
            if (!VOIP_NullUser((VoipUserT *)(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex])))
            {
                // if a participating user demote him first 
                if (pVoip->Common.Connectionlist.aIsParticipating[iLocalUserIndex] == TRUE)
                {
                    VoipActivateLocalUser(pVoip, iLocalUserIndex, FALSE);
                }

                NetPrintf(("voipxboxone: disabling voice for user at index %d\n", iLocalUserIndex));
                _VoipRegisterLocalTalker(pVoip, iLocalUserIndex, FALSE, NULL);
            }
            else
            {
                NetPrintf(("voipxboxone: warning -- ignoring attempt to un-register user at index %d because it is empty\n", iLocalUserIndex));
            }
        }

        NetCritLeave(&pVoip->Common.ThreadCrit);
    }
    else
    {
        NetPrintf(("voipxboxone: VoipSetLocalUser() invoked with an invalid user index %d\n", iLocalUserIndex));
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipActivateLocalUser

    \Description
        Promote/demote specified local user to/from "participating" state

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
void VoipActivateLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint8_t bActivate)
{
    int32_t iResult = 0;
    User ^refLocalUser = nullptr;

    if ((pVoip != NULL) && (iLocalUserIndex >= 0) && (iLocalUserIndex < NETCONN_MAXLOCALUSERS))
    {
        NetCritEnter(&pVoip->Common.ThreadCrit);

        if (!VOIP_NullUser((VoipUserT *)(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex])))
        {
            if (NetConnStatus('xusr', iLocalUserIndex, &refLocalUser, sizeof(refLocalUser)) == 0)
            {
                int32_t iSerializedSize = VoipSerializeUser(refLocalUser, &pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]);
                NetPrintf(("voipxboxone: serialized user object size = %d\n", iSerializedSize));

                if ((iResult = VoipHeadsetControl(pVoip->Common.pHeadset, 'aloc', iLocalUserIndex, bActivate, NULL)) < 0)
                {
                    NetPrintf(("voipxboxone: cannot %s a null user %s participating state (local user index %d)\n",
                        (bActivate?"promote":"demote"), (bActivate?"to":"from"), iLocalUserIndex));
                }

                pVoip->Common.Connectionlist.aIsParticipating[iLocalUserIndex] = bActivate;

                // reapply playback muting config based on the channel configuration
                VoipCommonApplyChannelConfig((VoipCommonRefT *)pVoip);

                // announce jip add or remove event on connection thater are already active
                VoipConnectionReliableBroadcastUser(&pVoip->Common.Connectionlist, iLocalUserIndex, bActivate);
            }
            else
            {
                NetPrintf(("voipxboxone: local talker registration aborted - no local user at index %d\n", iLocalUserIndex));
            }
        }
        else
        {
            NetPrintf(("voipxboxone: VoipActivateLocalUser() failed because no local user registered at index %d\n", iLocalUserIndex));
        }

        NetCritLeave(&pVoip->Common.ThreadCrit);
    }
    else
    {
        NetPrintf(("voipxboxone: VoipActivateLocalUser() invoked with an invalid user index %d\n", iLocalUserIndex));
    }
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
        int32_t     - selector-specific data

    \Notes
        iSelect can be one of the following:

        \verbatim
            'hset' - bitfield indicating which ports have headsets
            'pcac' - returns true if party chat is active
            'pcsu' - returns true if party chat is currently suspended
            'thid' - returns handle of _VoipThread
        \endverbatim

        Unhandled selectors are passed through to VoipCommonStatus(), and
        further to VoipHeadsetStatus() if unhandled there.

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'pcac')
    {
        //ref: https://forums.xboxlive.com/AnswerPage.aspx?qid=c0bdf120-e54c-49be-8141-8f67d31e8add&tgt=1
        try
        {
            return(PartyChat::IsPartyChatActive);
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipxboxone: error, exception thrown when calling get PartyChat::IsPartyChatActive (%S/0x%08x)\n", e->Message->Data(), e->HResult));
            return(-1);
        }
    }
    if (iSelect == 'pcsu')
    {
        //ref: https://forums.xboxlive.com/AnswerPage.aspx?qid=c0bdf120-e54c-49be-8141-8f67d31e8add&tgt=1
        try
        {
            return(PartyChat::IsPartyChatSuppressed);
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipxboxone: error, exception thrown when calling get PartyChat::IsPartyChatActive (%S/0x%08x)\n", e->Message->Data(), e->HResult));
            return(-1);
        }
    } 
    if (iSelect == 'thid')
    {
        *(HANDLE *)pBuf = pVoip->hThread;
        return(0);
    }
    return(VoipCommonStatus(&pVoip->Common, iSelect, iValue, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function   VoipControl

    \Description
        Set control options.

    \Input *pVoip   - module state from VoipStartup
    \Input iControl - control selector
    \Input iValue   - selector-specific input
    \Input *pValue  - selector-specific input

    \Output
        int32_t     - selector-specific output

    \Notes
        iControl can be one of the following:

        \verbatim
            '+pbk' - enable voice playback for a given remote user (pValue is the remote user VoipUserT).
            '-pbk' - disable voice playback for a given remote user (pValue is the remote user VoipUserT).
            'pcsu' - takes iValue as a bool, TRUE suspend party chat, FALSE resume it.
                In order for VoipControl 'pcsu' to work you must add to the appxmanifest.xml:
                <mx:Extension Category="xbox.multiplayer">
                    <mx:XboxMultiplayer CanSuppressPartyChat="true" />
                </mx:Extension>
        \endverbatim

        Unhandled selectors are passed through to VoipCommonControl(), and
        further to VoipHeadsetControl() if unhandled there.

    \Version 05/24/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    if ('+pbk' == iControl)
    {
        // make we have remote users registered before applying playback
        return(VoipHeadsetControl(pVoip->Common.pHeadset, '+pbk', iValue, 0, (VoipUserT *)pValue));
    }
    if ('-pbk' == iControl)
    {
        // make we have remote users registered before applying playback
        return(VoipHeadsetControl(pVoip->Common.pHeadset, '-pbk', iValue, 0, (VoipUserT *)pValue));
    }
    if ('pcsu' == iControl)
    {
        NetPrintf(("voipxboxone: IsPartyChatSuppressed=%d", iValue));

        //ref: https://forums.xboxlive.com/AnswerPage.aspx?qid=c0bdf120-e54c-49be-8141-8f67d31e8add&tgt=1
        try
        {
            PartyChat::IsPartyChatSuppressed = iValue;
        }
        catch (Exception ^ e)
        {
            NetPrintf(("voipxboxone: error, exception thrown when calling set PartyChat::IsPartyChatSuppressed (%S/0x%08x)\n", e->Message->Data(), e->HResult));
            return(-1);
        }
        return(iValue);
    }
    return(VoipCommonControl(&pVoip->Common, iControl, iValue, pValue));
}

