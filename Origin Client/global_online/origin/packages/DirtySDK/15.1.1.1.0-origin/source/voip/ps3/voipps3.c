/*H********************************************************************************/
/*!
    \File voipps3.c

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 08/10/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ppu_thread.h>

#include <sdk_version.h>
#include <netex/net.h>
#include <np.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "DirtySDK/voip/voip.h"

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/
#define VOIP_THREAD_SLEEP_DURATION        (20)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    VoipCommonRefT              Common;             //!< cross-platform voip data (must come first!)

    volatile int8_t             bThreadStarve;      //!< Added to prevent voip thread from executing anything when VOIP_SHUTDOWN_THREADSTARVE flag is set
    volatile int32_t            iThreadActive;
    volatile sys_ppu_thread_t   iThreadId;
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

    \Version 03/31/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipMicDataCb(const void *pVoiceData, int32_t iDataSize, const void *pMetaData, int32_t iMetaDataSize, uint32_t uLocalIdx, uint8_t uSendSeqn, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;
    VoipConnectionSend(&pVoip->Common.Connectionlist, pVoiceData, iDataSize, pMetaData, iMetaDataSize, uLocalIdx, uSendSeqn);
}

/*F********************************************************************************/
/*!
    \Function _VoipStatusCb

    \Description
        Callback to handle change of headset status.

    \Input iLocalUserIndex - headset that has changed (currently ignored)
    \Input bStatus         - if TRUE the headset was inserted, else if FALSE it was removed
    \Input eUpdate         - functionality of the headset (input, output or both)
    \Input *pUserData      - pointer to callback user data

    \Notes
        It is assumed that eUpdate will always be VOIP_HEADSET_STATUS_INOUT for 
        this platform.

    \Version 04/01/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipStatusCb(int32_t iLocalUserIndex, uint32_t bStatus, VoipHeadsetStatusUpdateE eUpdate, void *pUserData)
{
    VoipRefT *pRef = (VoipRefT *)pUserData;

    if (eUpdate != VOIP_HEADSET_STATUS_INOUT)
    {
        NetPrintf(("voipps3: warning, unexpected headset event for this platform!\n"));
    }

    if (bStatus)
    {
        NetPrintf(("voipps3: [%d] headset inserted\n", iLocalUserIndex));
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] |= (VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
        pRef->Common.uPortHeadsetStatus |= 1 << iLocalUserIndex;
    }
    else
    {
        NetPrintf(("voipps3: [%d] headset removed\n", iLocalUserIndex));
        pRef->Common.Connectionlist.uLocalUserStatus[iLocalUserIndex] &= ~(VOIP_LOCAL_USER_HEADSETOK|VOIP_LOCAL_USER_INPUTDEVICEOK|VOIP_LOCAL_USER_OUTPUTDEVICEOK);
        pRef->Common.uPortHeadsetStatus &= ~(1 << iLocalUserIndex);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdatePrivileges

    \Description
        Update privileges for current local user.

    \Input *pVoip       - pointer to module state

    \Version 09/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipUpdatePrivileges(VoipRefT *pVoip)
{
    // get privilege status
    pVoip->Common.bPrivileged = !NetConnStatus('chat', 0, NULL, 0);
    // display if voice is enabled
    NetPrintf(("voipps3: voice is %s\n", pVoip->Common.bPrivileged ? "enabled" : "disabled"));
}

/*F********************************************************************************/
/*!
    \Function   _VoipThread

    \Description
        Main Voip thread that updates all active connection and calls the
        XHV Engine's DoWork() function.

    \Input pArg - arguments

    \Version 03/19/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipThread(uint64_t pArg)
{
    VoipRefT *pVoip = (VoipRefT *)(uintptr_t)pArg;
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

        //if thread is starve dont run voip code
        if (!pVoip->bThreadStarve)
        {
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
                NetPrintf(("voipps3: [WARNING] Overall voip thread update took %d ms\n", NetTickDiff(uTime1, uTime0)));
            }
        }
    }

    // notify main thread we're done
    pVoip->iThreadId = 0;

    // terminate ourselves
    sys_net_free_thread_context(0, SYS_NET_THREAD_SELF);
    sys_ppu_thread_exit(0);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iMaxLocal    - maximum number of local users supported (ignored because only primary user can do voip on PS3)
    \Input iData        - platform-specific - unused for PS3

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iMaxLocal, int32_t iData)
{
    VoipRefT *pVoip;
    int32_t iResult;

    // common startup
    if ((pVoip = VoipCommonStartup(iMaxPeers, 1, sizeof(*pVoip), _VoipMicDataCb, _VoipStatusCb, iData)) == NULL)
    {
        return(NULL);
    }

    // get initial privilege status
    _VoipUpdatePrivileges(pVoip);

    // create worker thread
    if (((iResult = sys_ppu_thread_create((sys_ppu_thread_t *)&pVoip->iThreadId, _VoipThread, (uintptr_t)pVoip, 10, 0x4000, 0, "VoIP")) != CELL_OK) || (pVoip->iThreadId <= 0))
    {
        NetPrintf(("voipps3: unable to create worker thread (err=%s, thid=%d)\n", DirtyErrGetName(iResult), pVoip->iThreadId));
        VoipShutdown(pVoip, 0);
        return(NULL);
    }

    // tell thread to start executing
    pVoip->iThreadActive = 1;

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

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipShutdown(VoipRefT *pVoip, uint32_t uShutdownFlags)
{
    // make sure we're really started up
    if (VoipGetRef() == NULL)
    {
        NetPrintf(("voipps3: module shutdown called when not in a started state\n"));
        return;
    }

    /* If VOIP_SHUTDOWN_THREADSTARVE is passed in, instead of shutting down we simply
       lower the priority of the VoIP thread and stop the thread from executing voip related functions.
       This functionality is expected to be used in an XMB exit situation
       where the main code can be interrupted at any time, in any state, and forced
       to exit.  In this situation, simply preventing the threads from executing
       further allows the game to safely exit. */
    if (uShutdownFlags & VOIP_SHUTDOWN_THREADSTARVE)
    {
        pVoip->bThreadStarve = TRUE;
        sys_ppu_thread_set_priority(pVoip->iThreadId, 3071);
        return;
    }

    // tell thread to shut down and wait for shutdown confirmation
    for (pVoip->iThreadActive = -1; pVoip->iThreadId != 0; )
    {
        NetPrintf(("voipps3: waiting for shutdown\n"));
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
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PS3)
    \Input bRegister        - true to register user, false to unregister user

    \Version 01/16/2013 (mclouatre)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint32_t bRegister)
{
    NetCritEnter(&pVoip->Common.ThreadCrit);

    if (bRegister)
    {
        if (VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[0]))
        {
            int32_t iResult;
            SceNpId npId;

            if ((iResult = NetConnStatus('npid', 0, &npId, sizeof(npId))) >= 0)
            {
                NetPrintf(("voipps3: registering local user %s\n", npId.handle.data));

                VOIP_CopyUser(&pVoip->Common.Connectionlist.LocalUsers[0], (VoipUserT *)npId.handle.data);

                // update privilege status
                _VoipUpdatePrivileges(pVoip);
            }
            else
            {
                NetPrintf(("voipps3: error getting NP id for local user\n"));
            }
        }
        else
        {
            NetPrintf(("voipps3: warning -- ignoring attempt to register local user because user %s is already registered\n",
                pVoip->Common.Connectionlist.LocalUsers[0].strUserId));
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

            NetPrintf(("voipps3: unregistering local user %s\n", pVoip->Common.Connectionlist.LocalUsers[0].strUserId));

            memset(&pVoip->Common.Connectionlist.LocalUsers[0], 0, sizeof(pVoip->Common.Connectionlist.LocalUsers[0]));

            // update privilege status
            _VoipUpdatePrivileges(pVoip);
        }
        else
        {
            NetPrintf(("voipps3: warning -- ignoring attempt to unregister local user because it is not yet registered\n"));
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
    \Input iLocalUserIndex  - local user index (ignored because MLU not supported on PS3)
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Version 04/25/2013 (tcho)
*/
/********************************************************************************F*/
void VoipActivateLocalUser(VoipRefT *pVoip, int32_t iLocalUserIndex, uint8_t bActivate)
{
    NetCritEnter(&pVoip->Common.ThreadCrit);

    if (!VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[0]))
    {
        if (bActivate != FALSE)
        {
            NetPrintf(("voipps3: promoting local user %s to participating state", &pVoip->Common.Connectionlist.LocalUsers[0].strUserId));
            pVoip->Common.Connectionlist.aIsParticipating[0] = TRUE;
        }
        else
        {
            NetPrintf(("voipps3: demoting local user %s from participating state", &pVoip->Common.Connectionlist.LocalUsers[0].strUserId));
            pVoip->Common.Connectionlist.aIsParticipating[0] = FALSE;
        }
        VoipHeadsetControl(pVoip->Common.pHeadset, 'aloc', 0, bActivate, &pVoip->Common.Connectionlist.LocalUsers[0]);
    }
    else
    {
        NetPrintf(("voipps3: VoipActivateLocalUser() failed because no local user registered\n"));
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
        int32_t     - selector-specific data

    \Notes
        iSelect can be one of the following:

        \verbatim
            'hset' - bitfield indicating which ports have headsets
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
    \Input iControl - control selector
    \Input iValue   - selector-specific input
    \Input *pValue  - selector-specific input

    \Output
        int32_t     - selector-specific output

    \Notes
        iControl can be one of the following:

        \verbatim
            No PS3-specific control selectors.
        \endverbatim

        Unhandled selectors are passed through to VoipCommonControl(), and
        further to VoipHeadsetControl() if unhandled there.

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    // if switching codecs or sample rates we require sole access to thread critical section
    if ((iControl == 'cdec') || (iControl == 'mrat'))
    {
        int32_t iResult;
        NetCritEnter(&pVoip->Common.ThreadCrit);
        iResult = VoipHeadsetControl(pVoip->Common.pHeadset, iControl, iValue, 0, pValue);
        NetCritLeave(&pVoip->Common.ThreadCrit);
        return(iResult);
    }
    return(VoipCommonControl(&pVoip->Common, iControl, iValue, pValue));
}
