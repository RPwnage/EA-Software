/*H********************************************************************************/
/*!
    \File voipps4.c

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2013. ALL RIGHTS RESERVED.

    \Version 01/16/2013 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <kernel.h>
#include <string.h>
#include <np.h>

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

// voip includes
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

/*** Defines **********************************************************************/
#define VOIP_THREAD_SLEEP_DURATION        (20)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    VoipCommonRefT              Common;             //!< cross-platform voip data (must come first!)

    volatile int32_t            iThreadActive;
    volatile ScePthread         iThreadId;
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private Variables

// Public Variables

//! Global array used to track which local user(s) shall be included in the most restritive voip privileges calculation.
//! This is not tracked in the VoipRef because we want to empower the game code to pass this info (via '+pri' and '-pri' 
//! control selectors) even if voip is not yet started. The info tracked in this array is only used with users that are
//! NOT yet registered with VOIP. When a user is already registered with voip, he is by default included in the most
//! restritive voip privileges calculation
static uint8_t _VoipPs4_MostRestrictiveVoipContributors[NETCONN_MAXLOCALUSERS] = { FALSE, FALSE, FALSE, FALSE };

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

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipMicDataCb(const void *pVoiceData, int32_t iDataSize, const void *pMetaData, int32_t iMetaDataSize, uint32_t uLocalIdx, uint8_t uSendSeqn, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;
    VoipConnectionSend(&pVoip->Common.Connectionlist, pVoiceData, iDataSize, pMetaData, iMetaDataSize, uLocalIdx, uSendSeqn);
}

/*F********************************************************************************/
/*!
    \Function    _VoipTextDataCb

    \Description
        Callback to handle incoming mic data.

    \Input *pText           - pointer to text data
    \Input uLocalIdx        - local user index
    \Input *pUserData       - pointer to callback user data

    \Version 05/02/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipTextDataCb(const wchar_t *pText, uint32_t uLocalIdx, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;

    VoipConnectionReliableTranscribedTextMessage(&pVoip->Common.Connectionlist, uLocalIdx, pText);
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

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipStatusCb(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData)
{
    VoipRefT *pRef = (VoipRefT *)pUserData;

    if (eUpdate != VOIP_HEADSET_STATUS_INOUT)
    {
        NetPrintf(("voipps4: warning, unexpected headset event for this platform!\n"));
    }

    if (bStatus)
    {
        NetPrintf(("voipps4: [%d] headset inserted\n", iLocalUserIndex));
        pRef->Common.uPortHeadsetStatus |= 1 << iLocalUserIndex;
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= (VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
    }
    else
    {
        NetPrintf(("voipps4: [%d] headset removed\n", iLocalUserIndex));
        pRef->Common.uPortHeadsetStatus &= ~(1 << iLocalUserIndex);
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~(VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipRegisterLocalSharedTalker

    \Description
        Helper function to register/unregister a shared user

    \Input *pVoip           - voip module reference
    \Input bRegister        - TRUE to register user, FALSE to unregister

    \Version 01/12/2014 (tcho)
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
            ds_memclr(&voipSharedUser, sizeof(voipSharedUser));
            ds_snzprintf(voipSharedUser.strUserId, (int32_t)sizeof(voipSharedUser.strUserId), "shared-%d", pVoip->Common.Connectionlist.uClientId);
            ds_memcpy_s(&pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX], sizeof(pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX]), &voipSharedUser, sizeof(voipSharedUser));
            pVoip->Common.Connectionlist.aIsParticipating[VOIP_SHARED_USER_INDEX] = TRUE; // the shared user is always participating
            
            NetPrintf(("voipps4: local shared user added at index %d\n", VOIP_SHARED_USER_INDEX));
        }
    }
    else
    {
        // remove the shared user if there are no more local users left
        if (!VOIP_NullUser((VoipUserT *)(&pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX])))
        {
            ds_memclr(&pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX], sizeof(pVoip->Common.Connectionlist.LocalUsers[VOIP_SHARED_USER_INDEX]));
            pVoip->Common.Connectionlist.aIsParticipating[VOIP_SHARED_USER_INDEX] = FALSE;
            NetPrintf(("voipps4: shared user removed at index %d\n", VOIP_SHARED_USER_INDEX));
        }

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
        int32_t             - TRUE if the local talker was successfully registered, else FALSE

    \Version 02/05/2013 (mclouatre)
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
            pVoip->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~(VOIP_LOCAL_USER_MICON|VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
            ds_memclr(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex], sizeof(pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]));
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _VoipActivateLocalTalker

    \Description
        Register the participating talker on the given port.

    \Input *pVoip           - voip module reference
    \Input iLocalUserIndex  - local user index associated with the user
    \Input *pVoipUser       - VoipUserT to register
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Output
        int32_t         - TRUE if the local talker was successfully registered, else FALSE

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
static int32_t _VoipActivateLocalTalker(VoipRefT *pVoip, int32_t iLocalUserIndex, VoipUserT *pVoipUser, uint8_t bActivate)
{
    int32_t iResult = -1;

    if (pVoipUser != NULL)
    {
        iResult = VoipHeadsetControl(pVoip->Common.pHeadset, 'aloc', iLocalUserIndex, bActivate, pVoipUser);
        if (iResult >= 0)
        {
            if (bActivate == TRUE)
            {
                // mark local user as participating
                pVoip->Common.Connectionlist.aIsParticipating[iLocalUserIndex] = TRUE;
            }
            else
            {
                // mark local user as not participating
                pVoip->Common.Connectionlist.aIsParticipating[iLocalUserIndex] = FALSE;
            }
        }
        else
        {
            NetPrintf(("voipps4: cannot %s a null user %s participating state (local user index %d)\n",
                (bActivate?"promote":"demote"), (bActivate?"to":"from"), iLocalUserIndex));
        }
    }
    
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _VoipThread

    \Description
        Main Voip thread that updates all active connections

    \Input *pArg    - argument

    \Output
        void *      - NULL

    \Version 01/18/2013 (mclouatre)
*/
/********************************************************************************F*/
static void *_VoipThread(void *pArg)
{
    VoipRefT *pVoip = (VoipRefT *)pArg;
    uint32_t uProcessCt = 0;
    uint32_t uTime0, uTime1;
    int32_t iSleep;

    // wait until we're started up
    while(pVoip->iThreadActive == 0)
    {
        NetConnSleep(VOIP_THREAD_SLEEP_DURATION);
    }

    // execute until we're killed
    while(pVoip->iThreadActive > 0)
    {
        uTime0 = NetTick();

        NetCritEnter(&pVoip->Common.ThreadCrit);
        NetCritEnter(&pVoip->Common.Connectionlist.NetCrit);
        
        if (pVoip->Common.bApplyChannelConfig)
        {
            pVoip->Common.bApplyChannelConfig = FALSE;
            VoipCommonApplyChannelConfig((VoipCommonRefT *)pVoip);
        }     
        
        // update status of remote users
        VoipCommonUpdateRemoteStatus(&pVoip->Common);      
        NetCritLeave(&pVoip->Common.Connectionlist.NetCrit);

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
            NetPrintf(("voipps4: [WARNING] Overall voip thread update took %d ms\n", NetTickDiff(uTime1, uTime0)));
        }
    }

    // notify main thread we're done
    pVoip->iThreadId = 0;

    return(NULL);
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iData        - platform-specific - unused for ps4

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iData)
{
    VoipRefT *pVoip;
    ScePthreadAttr attr;
    int32_t iResult, iThreadCpuAffinity;

    // common startup
    if ((pVoip = VoipCommonStartup(iMaxPeers, sizeof(*pVoip), _VoipMicDataCb, _VoipTextDataCb, _VoipStatusCb, iData)) == NULL)
    {
        return(NULL);
    }

    VoipHeadsetControl(pVoip->Common.pHeadset, 'vpri', 0, 0, (void *)_VoipPs4_MostRestrictiveVoipContributors);

    // create worker thread
    if ((iResult = scePthreadAttrInit(&attr)) == SCE_OK)
    {
        SceKernelSchedParam schedParam;

        if ((iResult = scePthreadAttrSetdetachstate(&attr, SCE_PTHREAD_CREATE_DETACHED)) != SCE_OK)
        {
            NetPrintf(("voipps4: scePthreadAttrSetdetachstate failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        if ((iThreadCpuAffinity = NetConnStatus('affn', 0, NULL, 0)) > 0)
        {
            if ((iResult = scePthreadAttrSetaffinity(&attr, iThreadCpuAffinity)) != SCE_OK)
            {
                NetPrintf(("voipps4: scePthreadAttrSetaffinity %x failed (err=%s)\n", iThreadCpuAffinity, DirtyErrGetName(iResult)));
            }
        }
        else
        {
            if ((iResult = scePthreadAttrSetaffinity(&attr, SCE_KERNEL_CPUMASK_USER_ALL)) != SCE_OK)
            {
                NetPrintf(("voipps4: scePthreadAttrSetaffinity SCE_KERNEL_CPUMASK_USER_ALL failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
        }

        // set scheduling policy to round-robin (for consistency with EAthread)
        if ((iResult = scePthreadAttrSetschedpolicy(&attr, SCHED_RR)) != SCE_OK)
        {
            NetPrintf(("voipps4: scePthreadAttrSetschedpolicy SCHED_RR failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // set thread priority to highest
        schedParam.sched_priority = SCE_KERNEL_PRIO_FIFO_HIGHEST;
        if ((iResult = scePthreadAttrSetschedparam(&attr, &schedParam)) != SCE_OK)
        {
            NetPrintf(("voipps4: scePthreadAttrSetschedparam SCE_KERNEL_PRIO_FIFO_HIGHEST failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // set inheritance flag to EXPLICIT to avoid inheriting thread settings for parent thread
        if ((iResult = scePthreadAttrSetinheritsched(&attr, SCE_PTHREAD_EXPLICIT_SCHED)) != SCE_OK)
        {
            NetPrintf(("voipps4: scePthreadAttrSetinheritsched SCE_PTHREAD_EXPLICIT_SCHED failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        if ((iResult = scePthreadCreate((ScePthread *)&pVoip->iThreadId, &attr, _VoipThread, pVoip, "VoipThread")) != SCE_OK)
        {
            NetPrintf(("voipps4: scePthreadCreate failed (err=%s)\n", DirtyErrGetName(iResult)));
            scePthreadAttrDestroy(&attr);
            VoipShutdown(pVoip, 0);
            return(NULL);
        }

        if ((iResult = scePthreadAttrDestroy(&attr)) != SCE_OK)
        {
            NetPrintf(("voipps4: scePthreadAttrDestroy failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }
    else
    {
        NetPrintf(("voipps4: scePthreadAttrInit failed (err=%s)\n", DirtyErrGetName(iResult)));
        VoipShutdown(pVoip, 0);
        return(NULL);
    }

    // tell thread to start executing
    pVoip->iThreadActive = 1;

    //permission is handled by the voipheadsetps4 so bPrivileged is always TRUE
    pVoip->Common.bPrivileged = TRUE; 

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

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags)
{
    // make sure we're really started up
    if (VoipGetRef() == NULL)
    {
        NetPrintf(("voipps4: module shutdown called when not in a started state\n"));
        return;
    }

    // tell thread to shut down and wait for shutdown confirmation
    for (pVoip->iThreadActive = -1; pVoip->iThreadId != 0; )
    {
        NetPrintf(("voipps4: waiting for shutdown\n"));
        NetConnSleep(VOIP_THREAD_SLEEP_DURATION);
    }

    // remove the shared user
    _VoipRegisterLocalSharedTalker(pVoip, FALSE);

    // common shutdown (must come last as this frees the memory)
    VoipCommonShutdown(&pVoip->Common);
}

/*F********************************************************************************/
/*!
    \Function   VoipSetLocalUser

    \Description
        Register/unregister specified local user with the voip sub-system.

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index
    \Input bRegister        - true to register user, false to unregister user

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister)
{
    if ((iLocalUserIndex < VOIP_MAXLOCALUSERS) && (iLocalUserIndex >= 0))
    {
        NetCritEnter(&pVoip->Common.ThreadCrit);

        if (bRegister)
        {
            if (VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]))
            {
                int32_t iResult;
                VoipUserT voipUser;
                SceNpAccountId accountId;
                ds_memclr(&voipUser, sizeof(voipUser));

                if ((iResult = NetConnStatus('acct', iLocalUserIndex, &accountId, sizeof(accountId))) >= 0)
                {
                    ds_snzprintf(voipUser.strUserId, (int32_t)sizeof(voipUser.strUserId), "%llX", accountId);
                    NetPrintf(("voipps4: registering local user %s at index %d\n", voipUser.strUserId, iLocalUserIndex));

                    // register the local talker
                    _VoipRegisterLocalTalker(pVoip, iLocalUserIndex, TRUE, &voipUser);

                }
                else
                {
                    NetPrintf(("voipps4: error getting user info for index %d\n", iLocalUserIndex));
                }
            }
            else
            {
                NetPrintf(("voipps4: warning -- ignoring attempt to register user index %d because user %s is already registered at that index\n",
                    iLocalUserIndex, pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex].strUserId));
            }
        }
        else
        {
            if (!VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex]))
            {
                // if a participating user demote him first 
                if (pVoip->Common.Connectionlist.aIsParticipating[iLocalUserIndex] == TRUE)
                {
                    VoipActivateLocalUser(pVoip, iLocalUserIndex, FALSE);
                }

                NetPrintf(("voipps4: unregistering local user %s at index %d\n", pVoip->Common.Connectionlist.LocalUsers[iLocalUserIndex].strUserId, iLocalUserIndex));
                _VoipRegisterLocalTalker(pVoip, iLocalUserIndex, FALSE, NULL);
            }
            else
            {
                NetPrintf(("voipps4: warning -- ignoring attempt to unregister local user at index %d because it is no yet registered\n", iLocalUserIndex));
            }
        }

        NetCritLeave(&pVoip->Common.ThreadCrit);
    }
    else
    {
        NetPrintf(("voipps4: VoipSetLocalUser() invoked with an invalid user index %d\n", iLocalUserIndex));
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipActivateLocalUser

    \Description
        Promote/demote specified local user to/from "participating" state

    \Input *pVoip           - module state from VoipStartup
    \Input iLocalUserIndex  - local user index
    \Input bActivate        - TRUE to Activate False to unActivate

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
void VoipActivateLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint8_t bActivate)
{
    VoipConnectionlistT *pConnectionlist = &pVoip->Common.Connectionlist;

    if ((iLocalUserIndex < NETCONN_MAXLOCALUSERS) && (iLocalUserIndex >= 0))
    {
        NetCritEnter(&pVoip->Common.ThreadCrit);

        if (!VOIP_NullUser(&pConnectionlist->LocalUsers[iLocalUserIndex]))
        {
            if (bActivate != FALSE)
            {
                NetPrintf(("voipps4: promoting local user %s to participating state\n", &pConnectionlist->LocalUsers[iLocalUserIndex].strUserId));
                _VoipActivateLocalTalker(pVoip, iLocalUserIndex, (VoipUserT *) &pConnectionlist->LocalUsers[iLocalUserIndex].strUserId, TRUE);

                // reapply playback muting config based on the channel configuration only if this is a participating user
                pVoip->Common.bApplyChannelConfig = TRUE;
            }
            else
            {
                NetPrintf(("voipps4: demoting local user %s from participating state\n", &pConnectionlist->LocalUsers[iLocalUserIndex].strUserId));
                _VoipActivateLocalTalker(pVoip, iLocalUserIndex, (VoipUserT *) &pConnectionlist->LocalUsers[iLocalUserIndex].strUserId, FALSE);
            }

            // announce jip add or remove event on connection thater are already active
            VoipConnectionReliableBroadcastUser(pConnectionlist, iLocalUserIndex, bActivate);
        }
        else
        {
            NetPrintf(("voipps4: VoipActivateLocalUser() failed because no local user registered at index %d\n", iLocalUserIndex));
        }

        NetCritLeave(&pVoip->Common.ThreadCrit);
    }
    else
    {
        NetPrintf(("voipps4: VoipActivateLocalUser() invoked with an invalid user index %d\n", iLocalUserIndex));
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
            'vpri' - returns whether the specified local user (iValue) is a contributor to most restrictive voip privilege calculation
        \endverbatim

        Unhandled selectors are passed through to VoipCommonStatus(), and
        further to VoipHeadsetStatus() if unhandled there.

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    // returns whether the specified local user (iValue) is a contributor to most restrictive voip privilege calculation
    if (iSelect == 'vpri')
    {
        return(_VoipPs4_MostRestrictiveVoipContributors[iValue]);
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
             '+pri' - signal that local user shall be included when internally calculating most restrictive voip privileges
             '-pri' - signal that local user shall not be included when internally calculating most restrictive voip privileges
        \endverbatim

        Unhandled selectors are passed through to VoipCommonControl(), and
        further to VoipHeadsetControl() if unhandled there.

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == '+pbk')
    {
        return(VoipHeadsetControl(pVoip->Common.pHeadset, '+pbk', iValue, 0, (VoipUserT *)pValue));
    }
    if (iControl == '-pbk')
    {
        return(VoipHeadsetControl(pVoip->Common.pHeadset, '-pbk', iValue, 0, (VoipUserT *)pValue));
    }
    if (iControl == '+pri')
    {
        if (_VoipPs4_MostRestrictiveVoipContributors[iValue] == FALSE)
        {
            NetPrintf(("voipps4: local user at index %d is now an explicit contributor to most restrictive voip privilege determination\n", iValue));
            _VoipPs4_MostRestrictiveVoipContributors[iValue] = TRUE;
        }
        #if DIRTYCODE_LOGGING
        else
        {
            NetPrintf(("voipps4: VoipControl('+pri', %d) ignored because user is already an explicit contributor to most restrictive voip privilege determination\n", iValue));
        }
        #endif
    }
    if (iControl == '-pri')
    {
        if (_VoipPs4_MostRestrictiveVoipContributors[iValue] == TRUE)
        {
            NetPrintf(("voipps4: local user at index %d is no longer an explicit contributor to most restrictive voip privilege determination\n", iValue));
            _VoipPs4_MostRestrictiveVoipContributors[iValue] = FALSE;
        }
        #if DIRTYCODE_LOGGING
        else
        {
            NetPrintf(("voipps4: VoipControl('-pri', %d) ignored because user is already not explicitly contributing to most restrictive voip privilege determination\n", iValue));
        }
        #endif
    }

    return(VoipCommonControl(&pVoip->Common, iControl, iValue, pValue));
}
