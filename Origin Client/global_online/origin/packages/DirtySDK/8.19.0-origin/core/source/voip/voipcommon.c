/*H********************************************************************************/
/*!
    \File voipcommon.c

    \Description
        Cross-platform voip data types and functions.

    \Copyright
        Copyright (c) 2009 Electronic Arts Inc.

    \Version 12/02/2009 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef _XBOX
#include <xtl.h>
#include <xonline.h>
#endif

#include <string.h>

#include "platform.h"
#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"

#include "voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "voip.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

//! pointer to module state
static VoipRefT             *_Voip_pRef = NULL;


/*** Private Functions ************************************************************/


/*F*************************************************************************************************/
/*!
    \Function _VoipIdle

    \Description
        NetConn idle function to update the Voip module.

        This function is designed to handle issuing user callbacks based on events such as
        headset insertion and removal.  It is implemented as a NetConn idle function instead
        of as a part of the Voip thread so that callbacks will be generated from the same
        thread that DirtySock operates in.

    \Input *pData   - pointer to module state
    \Input uTick    - current tick count

    \Output
        None.

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called.

    \Version 02/10/2006 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipIdle(void *pData, uint32_t uTick)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pData;
    int32_t iIndex;

    // check for state changes and trigger callback
    if (pVoipCommon->uLastHsetStatus != pVoipCommon->uPortHeadsetStatus)
    {
        pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_HSETEVENT, pVoipCommon->uPortHeadsetStatus, pVoipCommon->pUserData);
        pVoipCommon->uLastHsetStatus = pVoipCommon->uPortHeadsetStatus;
    }

    if (pVoipCommon->uLastFrom != pVoipCommon->Connectionlist.uRecvVoice)
    {
        pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_FROMEVENT, pVoipCommon->Connectionlist.uRecvVoice, pVoipCommon->pUserData);
        pVoipCommon->uLastFrom = pVoipCommon->Connectionlist.uRecvVoice;
    }

    for(iIndex = 0; iIndex < pVoipCommon->Connectionlist.iMaxConnections; iIndex++)
    {
        VoipConnectionT* pVoipConnection = &pVoipCommon->Connectionlist.pConnections[iIndex];

        //read ahead. Needed because another thread writes to this.
        //we want to be sure we save in uLastRecvChannelId what we actually callback'ed with
        uint32_t remoteChannel = pVoipConnection->uRecvChannelId;

        if ((pVoipConnection->eState == ST_ACTV) &&
            remoteChannel != pVoipConnection->uLastRecvChannelId)
        {
            pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_CHANEVENT, iIndex, pVoipCommon->pUserData);
            pVoipConnection->uLastRecvChannelId = remoteChannel;
        }
    }

    if (pVoipCommon->uLastLocalStatus != pVoipCommon->Connectionlist.uLocalStatus)
    {
        if ((pVoipCommon->uLastLocalStatus ^ pVoipCommon->Connectionlist.uLocalStatus) & VOIP_LOCAL_SENDVOICE)
        {
            pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_SENDEVENT, pVoipCommon->Connectionlist.uLocalStatus & VOIP_LOCAL_SENDVOICE, pVoipCommon->pUserData);
        }
        pVoipCommon->uLastLocalStatus = pVoipCommon->Connectionlist.uLocalStatus;
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipCommonGetRef

    \Description
        Return current module reference.

    \Output
        VoipRefT *      - reference pointer, or NULL if the module is not active

    \Version 1.0 03/08/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipCommonGetRef(void)
{
    // return pointer to module state
    return(_Voip_pRef);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonStartup

    \Description
        Start up common functionality

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iMaxLocal    - maximum number of local users supported
    \Input iVoipRefSize - size of voip ref to allocate
    \Input *pMicDataCb  - mic data ready callback
    \Input *pStatusCb   - headset status callback

    \Output
        VoipRefT *      - voip ref if successful; else NULL

    \Version 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
VoipRefT *VoipCommonStartup(int32_t iMaxPeers, int32_t iMaxLocal, int32_t iVoipRefSize, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetStatusCbT *pStatusCb)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    VoipCommonRefT *pVoipCommon;

    // make sure we're not already started
    if (_Voip_pRef != NULL)
    {
        NetPrintf(("voipcommon: module startup called when not in a shutdown state\n"));
        return(NULL);
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // create and initialize module state
    if ((pVoipCommon = (VoipCommonRefT *)DirtyMemAlloc(iVoipRefSize, VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipcommon: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pVoipCommon, 0, iVoipRefSize);

    // set voip memgroup
    VoipMemGroupSet(iMemGroup, pMemGroupUserData);

    // create thread critical section
    NetCritInit(&pVoipCommon->ThreadCrit, "voip");

    // create connection list
    if (VoipConnectionStartup(&pVoipCommon->Connectionlist, iMaxPeers) < 0)
    {
        NetPrintf(("voipcommon: unable to allocate connectionlist\n"));
        NetCritKill(&pVoipCommon->ThreadCrit);
        DirtyMemFree(pVoipCommon, VOIP_MEMID, iMemGroup, pMemGroupUserData);
        return(NULL);
    }

    // create headset module
    pVoipCommon->pHeadset = VoipHeadsetCreate(iMaxPeers, iMaxLocal, pMicDataCb, pStatusCb, pVoipCommon);

    // set up connectionlist callback interface
    VoipConnectionSetCallbacks(&pVoipCommon->Connectionlist, &VoipHeadsetReceiveVoiceDataCb, &VoipHeadsetRegisterUserCb, pVoipCommon->pHeadset);

    // add callback idle function
    VoipSetEventCallback((VoipRefT *)pVoipCommon, NULL, NULL);
    pVoipCommon->uLastFrom = pVoipCommon->Connectionlist.uRecvVoice;
    pVoipCommon->uLastLocalStatus = pVoipCommon->Connectionlist.uLocalStatus;
    NetConnIdleAdd(_VoipIdle, pVoipCommon);

    // set initial defaults for local status and broadband
    pVoipCommon->Connectionlist.uLocalStatus = VOIP_LOCAL_BROADBAND;

    // save ref
    _Voip_pRef = (VoipRefT *)pVoipCommon;
    return(_Voip_pRef);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonStartup

    \Description
        Shutdown common functionality

    \Input *pVoipCommon - common module state

    \Output None.

    \Version 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonShutdown(VoipCommonRefT *pVoipCommon)
{
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // destroy critical section
    NetCritKill(&pVoipCommon->ThreadCrit);

    // destroy headset module
    VoipHeadsetDestroy(pVoipCommon->pHeadset);

    // shut down connectionlist
    VoipConnectionShutdown(&pVoipCommon->Connectionlist);

    // del callback idle function
    NetConnIdleDel(_VoipIdle, pVoipCommon);

    // free module memory
    VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemFree(pVoipCommon, VOIP_MEMID, iMemGroup, pMemGroupUserData);

    // clear pointer to module state
    _Voip_pRef = NULL;
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdateRemoteStatus

    \Description
        Process mute list, and set appropriate flags/priority for each remote user.

    \Input *pVoipCommon - pointer to module state

    \Output
        None.

    \Version 1.0 08/23/2005 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipCommonUpdateRemoteStatus(VoipCommonRefT *pVoipCommon)
{
    uint32_t uSendMask, uRecvMask, uChannelMask, bEnabled;
    uint8_t bPlaybackEnabled;
    int32_t iConnection;

    // process all active channels
    for (iConnection = 0; iConnection < pVoipCommon->Connectionlist.iMaxConnections; iConnection++)
    {
        VoipConnectionT *pConnection = &pVoipCommon->Connectionlist.pConnections[iConnection];

        // if not active, don't process
        if (pConnection->eState != ST_ACTV)
        {
            continue;
        }

        // calculate channel mask
        uChannelMask = 1 << iConnection;

        // decide whether this channel should be enabled or not
        #if defined(DIRTYCODE_XENON)
        bEnabled = !pConnection->bMuted && (pVoipCommon->bPrivileged || (pVoipCommon->bFriendsPrivileged && (pVoipCommon->Connectionlist.uFriendsMask & uChannelMask)));
        #else
        bEnabled = pVoipCommon->bPrivileged;
        #endif

        // set send/recv masks based on enable + user send override
        if (bEnabled && ((pVoipCommon->Connectionlist.uUserSendMask & uChannelMask) != 0))
        {
            uSendMask = pVoipCommon->Connectionlist.uSendMask | uChannelMask;
        }
        else
        {
            uSendMask = pVoipCommon->Connectionlist.uSendMask & ~uChannelMask;
        }

        // set send/recv masks based on enable + user recv override
        if (bEnabled && ((pVoipCommon->Connectionlist.uUserRecvMask & uChannelMask) != 0))
        {
            uRecvMask = pVoipCommon->Connectionlist.uRecvMask | uChannelMask;
            bPlaybackEnabled = TRUE;
        }
        else
        {
            uRecvMask = pVoipCommon->Connectionlist.uRecvMask & ~uChannelMask;
            bPlaybackEnabled = FALSE;
        }

        // set send/recv masks and priority
        if (uSendMask != pVoipCommon->Connectionlist.uSendMask)
        {
            VoipConnectionSetSendMask(&pVoipCommon->Connectionlist, uSendMask);
        }
        if (uRecvMask != pVoipCommon->Connectionlist.uRecvMask)
        {
            VoipConnectionSetRecvMask(&pVoipCommon->Connectionlist, uRecvMask);

            #if defined(DIRTYCODE_XENON) && DIRTYCODE_LOGGING
            {
                NetPrintf(("voipcommon: %s playback for remote user %s on port %d\n", bPlaybackEnabled ? "enabling" : "disabling",
                    pConnection->RemoteUsers[pConnection->iPrimaryRemoteUser].strUserId,
                    pVoipCommon->iActivePort));
            }
            #endif
        }
        #if defined(DIRTYCODE_XENON)
        // $$ todo - need to set playback priority for all remote users for all active local users
        VoipHeadsetControl(pVoipCommon->pHeadset, 'prio', pVoipCommon->iActivePort, bPlaybackEnabled, &pConnection->RemoteUsers[pConnection->iPrimaryRemoteUser]);
        #endif
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonStatus

    \Description
        Return status.

    \Input *pVoipCommon - voip common state
    \Input iSelect      - status selector
    \Input iValue       - selector-specific
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of output buffer

    \Output
        int32_t         - selector-specific data

    \Notes
        Allowed selectors:

        \verbatim
            'avlb' - whether a given ConnID is available
            'chan' - retrieve the active voip channel (coded value of channels and modes)
            'from' - bitfield indicating which peers are talking
            'hset' - bitfield indicating which ports have headsets
            'lprt' - return local socket port
            'mgrp' - voip memgroup user id
            'mgud' - voip memgroup user data
            'micr' - who we're sending voice to
            'rchn' - retrieve the active voip channel of a remote peer (coded value of channels and modes)
            'sock' - voip socket ref
            'spkr' - who we're accepting voice from
            'umic' - user-specified microphone send list
            'uspk' - user-specified microphone recv list
        \endverbatim

    \Version 1.0 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCommonStatus(VoipCommonRefT *pVoipCommon, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'avlb')
    {
        return(VoipConnectionCanAllocate(&pVoipCommon->Connectionlist, iValue));
    }
    if (iSelect == 'chan')
    {
        return(pVoipCommon->Connectionlist.uChannelId);
    }
    if (iSelect == 'from')
    {
        return(pVoipCommon->Connectionlist.uRecvVoice);
    }
    if (iSelect == 'hset')
    {
        uint32_t uPortHeadsetStatus = pVoipCommon->uPortHeadsetStatus;
        #if defined(DIRTYCODE_XENON)
        if (pVoipCommon->iLockedPort >= 0)
        {
            uPortHeadsetStatus &= (1 << pVoipCommon->iLockedPort);
        }
        #endif
        return(uPortHeadsetStatus);
    }
    if (iSelect == 'lprt')
    {
        return(pVoipCommon->Connectionlist.uBindPort);
    }
    if (iSelect == 'mgrp')
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;

        // Query mem group data
        VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

        // return mem group id
        return(iMemGroup);
    }
    if (iSelect == 'mgud')
    {
        // Make user-provide buffer is large enough to receive a pointer
        if ((pBuf != NULL) && (iBufSize >= (signed)sizeof(void *)))
        {
            int32_t iMemGroup;
            void *pMemGroupUserData;

            // Query mem group data
            VoipMemGroupQuery(&iMemGroup, &pMemGroupUserData);

            // fill [out] parameter with pointer to mem group user data
            *((void **)pBuf) = pMemGroupUserData;
            return(0);
        }
        else
        {
            // unhandled
            return(-1);
        }
    }
    if (iSelect == 'micr')
    {
        return(pVoipCommon->Connectionlist.uSendMask);
    }
    if (iSelect == 'rchn')
    {
        return(pVoipCommon->Connectionlist.pConnections[iValue].uRecvChannelId);
    }
    if (iSelect == 'sock')
    {
        if (pBuf != NULL)
        {
            if (iBufSize >= (signed)sizeof(pVoipCommon->Connectionlist.pSocket))
            {
                memcpy(pBuf, &pVoipCommon->Connectionlist.pSocket, sizeof(pVoipCommon->Connectionlist.pSocket));
            }
            else
            {
                NetPrintf(("voipcommon: socket reference cannot be copied because user buffer is not large enough\n"));
                return(-1);
            }
        }
        else
        {
            NetPrintf(("voipcommon: socket reference cannot be copied because user buffer pointer is NULL\n"));
            return(-1);
        }
        return(0);
    }
    if (iSelect == 'spkr')
    {
        return(pVoipCommon->Connectionlist.uRecvMask);
    }
    if (iSelect == 'umic')
    {
        return(pVoipCommon->Connectionlist.uUserSendMask);
    }
    if (iSelect == 'uspk')
    {
        return(pVoipCommon->Connectionlist.uUserRecvMask);
    }
    // unrecognized selector, so pass through to voipheadset
    return(VoipHeadsetStatus(pVoipCommon->pHeadset, iSelect, iValue, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonControl

    \Description
        Set control options.

    \Input *pVoipCommon - voip common state
    \Input iControl     - control selector
    \Input iValue       - selector-specific input
    \Input *pValue      - selector-specific input

    \Output
        int32_t         - selector-specific output

    \Notes
        iControl can be one of the following:

        \verbatim
            'ases' - add session id to the set of sessions sharing this connnection
            'chan' - set the active voip channel (coded value of channels and modes)
            'clid' - set local client id
            'conm' - for voip via voip server, maps a local conn id to a voip server conn id
            'dses' - delete session id from the set of sessions sharing this connnection
            'flsh' - send currently queued voice data immediately (iValue=connection id)
            'time' - set data timeout in milliseconds
            'umic' - select which peers to send voice to
            'uspk' - Select which peers to accept voice from
        \endverbatim

        Unhandled selectors are passed through to VoipHeadsetControl()

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipCommonControl(VoipCommonRefT *pVoipCommon, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'ases')
    {
        int32_t iRetCode;

        // acquire critical section to modify ConnectionList
        NetCritEnter(&pVoipCommon->ThreadCrit);

        iRetCode = VoipConnectionAddSessionId(&pVoipCommon->Connectionlist, iValue, *(uint32_t*)pValue);

        // release critical section to modify ConnectionList
        NetCritLeave(&pVoipCommon->ThreadCrit);

        return(iRetCode);
    }
    if (iControl == 'chan')
    {
        NetPrintf(("voipcommon: setting voip channel = 0x%08x\n", iValue));
        pVoipCommon->Connectionlist.uChannelId = iValue;
        return(0);
    }
    if (iControl == 'clid')
    {
        if ((pVoipCommon->Connectionlist.uClientId != 0) && (pVoipCommon->Connectionlist.uClientId != (unsigned)iValue))
        {
            NetPrintf(("voipcommon: warning - local client id is being changed from %d to %d\n", pVoipCommon->Connectionlist.uClientId, iValue));
        }
        pVoipCommon->Connectionlist.uClientId = (unsigned)iValue;
        return(0);
    }
    if (iControl == 'conm')
    {
        if (pVoipCommon->Connectionlist.pConnections[iValue].eState != ST_DISC)
        {
            NetPrintf(("voipcommon: mapping local conn id %d to voipserver conn id %d\n", iValue, *(int32_t*)pValue));
            pVoipCommon->Connectionlist.pConnections[iValue].iVoipServerConnId = *(int32_t*)pValue;
            return(0);
        }
        else
        {
            NetPrintf(("voipcommon: warning - 'conm' selector ignored because connection state is ST_DISC\n"));
            return(-1);
        }
    }
    if (iControl == 'dses')
    {
        int32_t iRetCode;

        // acquire critical section to modify ConnectionList
        NetCritEnter(&pVoipCommon->ThreadCrit);

        iRetCode = VoipConnectionDeleteSessionId(&pVoipCommon->Connectionlist, iValue, *(uint32_t*)pValue);

        // release critical section to modify ConnectionList
        NetCritLeave(&pVoipCommon->ThreadCrit);

        return(iRetCode);
    }
    if (iControl == 'flsh')
    {
        return(VoipConnectionFlush(&pVoipCommon->Connectionlist, iValue));
    }
    if (iControl == 'time')
    {
        pVoipCommon->Connectionlist.iDataTimeout = iValue;
        return(0);
    }
    if (iControl == 'umic')
    {
        #if !defined(DIRTYCODE_PC)
        VoipCommonSetMask(&pVoipCommon->Connectionlist.uUserSendMask, iValue, "usendmask");
        #else
        VoipConnectionSetSendMask(&pVoipCommon->Connectionlist, iValue);
        #endif
        return(0);
    }
    if (iControl == 'uspk')
    {
        #if !defined(DIRTYCODE_PC)
        VoipCommonSetMask(&pVoipCommon->Connectionlist.uUserRecvMask, iValue, "urecvmask");
        #else
        VoipConnectionSetRecvMask(&pVoipCommon->Connectionlist, iValue);
        #endif
        return(0);
    }
    // unhandled selectors are passed through to voipheadset
    return(VoipHeadsetControl(pVoipCommon->pHeadset, iControl, iValue, 0, pValue));
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonAddMask

    \Description
        Add (OR) uAddMask into *pMask

    \Input *pMask       - mask to add into
    \Input uAddMask     - mask to add (OR)
    \Input *pMaskName   - name of mask (for debug logging)

    \Output None.

    \Version 12/03/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonAddMask(uint32_t *pMask, uint32_t uAddMask, const char *pMaskName)
{
    VoipCommonSetMask(pMask, *pMask | uAddMask, pMaskName);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonDelMask

    \Description
        Del (&~) uDelMask from *pMask

    \Input *pMask       - mask to del from
    \Input uDelMask     - mask to del (&~)
    \Input *pMaskName   - name of mask (for debug logging)

    \Output None.

    \Version 12/03/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonDelMask(uint32_t *pMask, uint32_t uDelMask, const char *pMaskName)
{
    VoipCommonSetMask(pMask, *pMask & ~uDelMask, pMaskName);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonSetMask

    \Description
        Set value of mask (with logging).

    \Input *pMask       - mask to write to
    \Input uNewMask     - new mask value
    \Input *pMaskName   - name of mask (for debug logging)

    \Output None.

    \Version 12/03/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonSetMask(uint32_t *pMask, uint32_t uNewMask, const char *pMaskName)
{
    NetPrintf(("voipcommon: %s update: 0x%08x->0x%08x\n", pMaskName, *pMask, uNewMask));
    *pMask = uNewMask;
}

