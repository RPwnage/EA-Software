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

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "DirtySDK/voip/voip.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

//! pointer to module state
static VoipRefT             *_Voip_pRef = NULL;


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function   _VoipCommonEncode

    \Description
        Packs the channel selection for sending to a remote machine

    \Input iChannels[]      - the array of channels to encode
    \Input uModes[]         - the array of modes to encode

    \Output
        uint32_t            - the coded channels

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static uint32_t _VoipCommonEncode(int32_t iChannels[], uint32_t uModes[])
{
    uint32_t codedChannel = 0;
    int32_t iIndex;

    for(iIndex = 0; iIndex < VOIP_MAX_CONCURRENT_CHANNEL; iIndex++)
    {
        codedChannel |= (iChannels[iIndex] & 0x3f) << (8 * iIndex);
        codedChannel |= ((uModes[iIndex] & 0x3) << 6) << (8 * iIndex);
    }

    return(codedChannel);
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonDecode

    \Description
        Unpacks the channel selection received by a remote machine

    \Input uCodedChannel    - the coded channels
    \Input iChannels[]      - the array of channels to fill up
    \Input uModes[]         - the array of modes to fill up

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipCommonDecode(uint32_t uCodedChannel, int32_t iChannels[], uint32_t uModes[])
{
    int32_t iIndex;
    for(iIndex = 0; iIndex < VOIP_MAX_CONCURRENT_CHANNEL; iIndex++)
    {
        iChannels[iIndex] = (uCodedChannel >> (8 * iIndex)) & 0x3f;
        uModes[iIndex] = ((uCodedChannel >> (8 * iIndex)) >> 6) & 0x3;
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonProcessChannelChange

    \Description
        Unpacks the channel selection received by a remote machine

    \Input pVoipCommon   - voip common state
    \Input iConnId       - connection id

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipCommonProcessChannelChange(VoipCommonRefT *pVoipCommon, int32_t iConnId)
{
    int32_t iUserIndex;

    for (iUserIndex = 0; iUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iUserIndex++)
    {
        uint32_t uInputOuputVar;
        uint32_t uNewChannelConfig, uOldChannelConfig;

        uInputOuputVar = (uint32_t)iUserIndex;
        VoipStatus(VoipGetRef(), 'rchn', iConnId, &uInputOuputVar, sizeof(uInputOuputVar));

        uNewChannelConfig = uInputOuputVar;
        uOldChannelConfig = pVoipCommon->uRemoteChannelSelection[iConnId][iUserIndex];

        NetPrintf(("voipcommon: got remote channels 0x%08x (old config: 0x%08x)for user index %d on low-level conn id %d\n",
            uNewChannelConfig, uOldChannelConfig, iUserIndex, iConnId));

        pVoipCommon->uRemoteChannelSelection[iConnId][iUserIndex] = uInputOuputVar;
    }

    VoipCommonApplyChannelConfig(pVoipCommon);
}

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
        uint32_t aRemoteChannels[VOIP_MAXLOCALUSERS_EXTENDED];

        if (pVoipConnection->eState == ST_ACTV)
        {
            //read ahead. Needed because another thread writes to this.
            //we want to be sure we save in aLastRecvChannels what we actually callback'ed with
            ds_memcpy_s(aRemoteChannels, sizeof(aRemoteChannels), &pVoipConnection->aRecvChannels, sizeof(pVoipConnection->aRecvChannels));

            if (memcmp(aRemoteChannels, &pVoipConnection->aLastRecvChannels, sizeof(aRemoteChannels)))
            {
                _VoipCommonProcessChannelChange((VoipCommonRefT *)pData, iIndex);

                ds_memcpy_s(&pVoipConnection->aLastRecvChannels, sizeof(pVoipConnection->aLastRecvChannels), aRemoteChannels, sizeof(aRemoteChannels));
            }
        }
    }

    for (iIndex = 0; iIndex < VOIP_MAXLOCALUSERS; ++iIndex)
    {
        if (pVoipCommon->uLastLocalStatus[iIndex] != pVoipCommon->Connectionlist.uLocalUserStatus[iIndex])
        {
            if ((pVoipCommon->uLastLocalStatus[iIndex] ^ pVoipCommon->Connectionlist.uLocalUserStatus[iIndex]) & VOIP_LOCAL_USER_SENDVOICE)
            {
                pVoipCommon->pCallback((VoipRefT *)pData, VOIP_CBTYPE_SENDEVENT, pVoipCommon->Connectionlist.uLocalUserStatus[iIndex] & VOIP_LOCAL_USER_SENDVOICE, pVoipCommon->pUserData);
            }
            pVoipCommon->uLastLocalStatus[iIndex] = pVoipCommon->Connectionlist.uLocalUserStatus[iIndex];
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonChannelMatch

    \Description
        Returns whether a given channel selection is to be received and/or sent to

    \Input *pVoipCommon         - voip common state
    \Input iUserIndex           - local user index
    \Input uCodedChannel        - coded channels of remote user

    \Output
        uint32_t                - the coded channels

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static VoipChanModeE _VoipCommonChannelMatch(VoipCommonRefT *pVoipCommon, int32_t iUserIndex, uint32_t uCodedChannel)
{
    int32_t iIndexRemote;
    int32_t iIndexLocal;
    VoipChanModeE eMode = 0;
    int32_t iChannels[VOIP_MAX_CONCURRENT_CHANNEL];
    uint32_t uModes[VOIP_MAX_CONCURRENT_CHANNEL];

    // Voip traffic sent on channels {(0,none),(0,none),(0,none),(0,none)} 
    // is assumed to be meant for everybody.
    // enables default behaviour for teams not using channels.
    if ((uCodedChannel == 0) || (pVoipCommon->uLocalChannelSelection[iUserIndex] == 0))
    {
        return(VOIP_CHANSEND|VOIP_CHANRECV);
    }

    _VoipCommonDecode(uCodedChannel, iChannels, uModes);

    // for all our channels, and all the channels of the remote party
    for(iIndexRemote = 0; iIndexRemote < VOIP_MAX_CONCURRENT_CHANNEL; iIndexRemote++)
    {
        for(iIndexLocal = 0; iIndexLocal < VOIP_MAX_CONCURRENT_CHANNEL; iIndexLocal++)
        {
            // if their channel numbers match 
            if (pVoipCommon->iLocalChannels[iUserIndex][iIndexLocal] == iChannels[iIndexRemote])
            {
                // if we're sending on it and they're receiving on it
                if ((pVoipCommon->uLocalModes[iUserIndex][iIndexLocal] & VOIP_CHANSEND) && (uModes[iIndexRemote] & VOIP_CHANRECV))
                {
                    // let's send
                    eMode |= VOIP_CHANSEND;
                }
                // if we're receiving on it and they're send on it
                if ((pVoipCommon->uLocalModes[iUserIndex][iIndexLocal] & VOIP_CHANRECV) && (uModes[iIndexRemote] & VOIP_CHANSEND))
                {
                    // let's receive
                    eMode |= VOIP_CHANRECV;
                }
            }
        }
    }

    return(eMode);
}


/*F********************************************************************************/
/*!
    \Function   _VoipCommonComputeEffectiveUserSendMask

    \Description
        Compute effective user send mask from two other masks: user-selected
        send mask and mask obtained from voip channel config

    \Input *pVoipCommon - voip common state

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipCommonComputeEffectiveUserSendMask(VoipCommonRefT *pVoipCommon)
{
    VoipCommonSetMask(&pVoipCommon->Connectionlist.uUserSendMask, pVoipCommon->uUserMicrValue & pVoipCommon->uChanSendMask, "usendmask");
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonComputeDefaultSharedChannelConfig

    \Description
        Computes the default shared channel config. This is the logical "and" of all 
        local user's channel modes

    \Input *pVoipCommon - voip common state

    \Notes
        Loops through all the channels and "and" the channel mode of all the valid users on the
        that particular channel together. Only if everyone is subcribed to the channel do we set
        the mode for the shared user.

    \Version 12/11/2014 (tcho)
*/
/********************************************************************************F*/
static void _VoipCommonComputeDefaultSharedChannelConfig(VoipCommonRefT *pVoipCommon)
{
    int32_t iLocalUserIndex;
    int32_t iChannel;
    int32_t iChannelSlot;
    int32_t iEmptySlot = 0;

    // loop through all the channel from 1 to 63
    for (iChannel = 1; iChannel < VOIP_MAXLOCALCHANNEL; ++iChannel)
    {
        int32_t iValidUser = 0;
        int32_t iChannelSubscriber = 0;
        uint32_t uSharedChannelMode = VOIP_CHANSENDRECV;

        // only compute the shared channel config if we still have a free slot
        if (iEmptySlot != VOIP_MAX_CONCURRENT_CHANNEL)
        {
            // loop through all the local users excluding the shared user
            for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS; ++iLocalUserIndex)
            {
                if (!VOIP_NullUser((VoipUserT *)&pVoipCommon->Connectionlist.LocalUsers[iLocalUserIndex]))
                {
                    ++iValidUser;

                    // the user is valid loop through his subscribed channels
                    for (iChannelSlot = 0; iChannelSlot < VOIP_MAX_CONCURRENT_CHANNEL; ++iChannelSlot)
                    {
                        if (pVoipCommon->iLocalChannels[iLocalUserIndex][iChannelSlot] == iChannel)
                        {
                            ++iChannelSubscriber;
                            uSharedChannelMode &= pVoipCommon->uLocalModes[iLocalUserIndex][iChannelSlot];
                            break;
                        }
                    }
                }
            }

            // if everyone is subscribed to the channel then the uSharedChannelMode is valid
            if ((iValidUser == iChannelSubscriber) && (iChannelSubscriber != 0))
            {
                pVoipCommon->iDefaultSharedChannels[iEmptySlot] = iChannel;
                pVoipCommon->uDefaultSharedModes[iEmptySlot] = uSharedChannelMode;
                ++iEmptySlot;
            }
        }
        else
        {
            NetPrintf(("voipcommon: local shared channel config ran out of free channel slots\n"));
            break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonComputeEffectiveUserRecvMask

    \Description
        Compute effective user recv mask from two other masks: user-selected
        recv mask and mask obtained from voip channel config

    \Input *pVoipCommon - voip common state

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
static void _VoipCommonComputeEffectiveUserRecvMask(VoipCommonRefT *pVoipCommon)
{
    VoipCommonSetMask(&pVoipCommon->Connectionlist.uUserRecvMask, pVoipCommon->uUserSpkrValue & pVoipCommon->uChanRecvMask, "urecvmask");
}

/*F********************************************************************************/
/*!
    \Function   _VoipCommonSetSharedUserChannelConfig

    \Description
        Set the shared user channel config to custom or default based on the 
        bUseDefaultSharedChannelConfig flag.

    \Input *pVoipCommon - voip common state

    \Version 12/16/2014 (tcho)
*/
/********************************************************************************F*/
static void _VoipCommonSetSharedUserChannelConfig(VoipCommonRefT *pVoipCommon)
{
    int32_t iMaxConcurrentChannel;
    int32_t iSharedUserIndex = VOIP_SHARED_USER_INDEX;

    // exit early if shared user channel config not supported
    if (iSharedUserIndex == VOIP_INVALID_SHARED_USER_INDEX)
    {
        return;
    }

    for (iMaxConcurrentChannel = 0; iMaxConcurrentChannel < VOIP_MAX_CONCURRENT_CHANNEL; ++iMaxConcurrentChannel)
    {
        if (pVoipCommon->bUseDefaultSharedChannelConfig)
        {
            pVoipCommon->iLocalChannels[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->iDefaultSharedChannels[iMaxConcurrentChannel];
            pVoipCommon->uLocalModes[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->uDefaultSharedModes[iMaxConcurrentChannel];
        }
        else
        {
            pVoipCommon->iLocalChannels[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->iCustomSharedChannels[iMaxConcurrentChannel];
            pVoipCommon->uLocalModes[iSharedUserIndex][iMaxConcurrentChannel] = pVoipCommon->uCustomSharedModes[iMaxConcurrentChannel];
        }
    }

    // encdoe the channel selection and set the active voip channels
    pVoipCommon->uLocalChannelSelection[iSharedUserIndex] = _VoipCommonEncode(pVoipCommon->iLocalChannels[iSharedUserIndex], pVoipCommon->uLocalModes[iSharedUserIndex]);
    VoipControl(VoipGetRef(), 'chan', pVoipCommon->uLocalChannelSelection[iSharedUserIndex], &iSharedUserIndex);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipCommonGetRef

    \Description
        Return current module reference.

    \Output
        VoipRefT *      - reference pointer, or NULL if the module is not active

    \Version 03/08/2004 (jbrookes)
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
    \Input iData        - platform-specific

    \Output
        VoipRefT *      - voip ref if successful; else NULL

    \Version 12/02/2009 (jbrookes)
*/
/********************************************************************************F*/
VoipRefT *VoipCommonStartup(int32_t iMaxPeers, int32_t iMaxLocal, int32_t iVoipRefSize, VoipHeadsetMicDataCbT *pMicDataCb, VoipHeadsetStatusCbT *pStatusCb, int32_t iData)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    VoipCommonRefT *pVoipCommon;
    int32_t i;

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

    // by default, user-selected micr and spkr flag always default to ON
    pVoipCommon->uUserMicrValue = 0xFFFFFFFF;
    pVoipCommon->uUserSpkrValue = 0xFFFFFFFF;

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
    pVoipCommon->pHeadset = VoipHeadsetCreate(iMaxPeers, iMaxLocal, pMicDataCb, pStatusCb, pVoipCommon, iData);
    
    // turn on default shared channel config by default
    pVoipCommon->bUseDefaultSharedChannelConfig = TRUE;
    
    // set up connectionlist callback interface
    VoipConnectionSetCallbacks(&pVoipCommon->Connectionlist, &VoipHeadsetReceiveVoiceDataCb, &VoipHeadsetRegisterUserCb, pVoipCommon->pHeadset);

    // add callback idle function
    VoipSetEventCallback((VoipRefT *)pVoipCommon, NULL, NULL);
    pVoipCommon->uLastFrom = pVoipCommon->Connectionlist.uRecvVoice;
    for (i = 0; i < VOIP_MAXLOCALUSERS; ++i)
    {
        pVoipCommon->uLastLocalStatus[i] = pVoipCommon->Connectionlist.uLocalUserStatus[i];
    }
    NetConnIdleAdd(_VoipIdle, pVoipCommon);

    // save ref
    _Voip_pRef = (VoipRefT *)pVoipCommon;
    return(_Voip_pRef);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonShutdown

    \Description
        Shutdown common functionality

    \Input *pVoipCommon - common module state

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
    \Function   VoipCommonUpdateRemoteStatus

    \Description
        Process mute list, and set appropriate flags/priority for each remote user.

    \Input *pVoipCommon - pointer to module state

    \Version 08/23/2005 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonUpdateRemoteStatus(VoipCommonRefT *pVoipCommon)
{
    uint32_t uSendMask, uRecvMask, uChannelMask, bEnabled;
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
        }
        else
        {
            uRecvMask = pVoipCommon->Connectionlist.uRecvMask & ~uChannelMask;
        }

        // set send/recv masks and priority
        if (uSendMask != pVoipCommon->Connectionlist.uSendMask)
        {
            VoipConnectionSetSendMask(&pVoipCommon->Connectionlist, uSendMask);
        }
        if (uRecvMask != pVoipCommon->Connectionlist.uRecvMask)
        {
            VoipConnectionSetRecvMask(&pVoipCommon->Connectionlist, uRecvMask);
        }
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
            'chnc' - return the count of channel slots used by the voip group manager
            'chnl' - return the channel id and channel mode associated with specified channel slot
            'from' - bitfield indicating which peers are talking
            'hset' - bitfield indicating which ports have headsets
            'lprt' - return local socket port
            'luid' - return user id of specified local user
            'maxc' - return max connections
            'mgrp' - voip memgroup user id
            'mgud' - voip memgroup user data
            'micr' - who we're sending voice to
            'rchn' - retrieve the active voip channel of a remote peer (coded value of channels and modes)
            'ruid' - return user id of specified remote user index for specified connection
            'shch' - return bUseDefaultSharedChannelConfig which indicates if the default shared channel config is used
            'sock' - voip socket ref
            'spkr' - who we're accepting voice from
            'umic' - user-specified microphone send list
            'uspk' - user-specified microphone recv list
        \endverbatim

    \Version 12/02/2009 (jbrookes)
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
        return(pVoipCommon->Connectionlist.aChannels[iValue]);
    }
    if (iSelect == 'chnc')
    {
       return(VOIP_MAX_CONCURRENT_CHANNEL);
    }
    if (iSelect == 'chnl')
    {
        int32_t iUserIndex = iValue & 0xFFFF;
        int32_t iChannelSlot = iValue >> 16;

        if (!pBuf)
        {
            NetPrintf(("voipcommon: pBuf must be valid with the 'chnl' selector\n"));
            return(-1);
        }

        if ((unsigned)iBufSize < sizeof(VoipChanModeE))
        {
            NetPrintf(("voipcommon: user buffer used with the 'chnl' selector is too small (expected size = %d; current size =%d)\n",
                sizeof(VoipChanModeE), iBufSize));
            return(-2);
        }

        if ((iChannelSlot < 0) || (iChannelSlot >= VOIP_MAX_CONCURRENT_CHANNEL))
        {
            NetPrintf(("voipcommon: invalid slot id (%d) used with 'chnl' selector; valid range is [0,%d]\n", iChannelSlot, VOIP_MAX_CONCURRENT_CHANNEL-1));
            return(-3);
        }

        *(VoipChanModeE *)pBuf = pVoipCommon->uLocalModes[iUserIndex][iChannelSlot];
        return(pVoipCommon->iLocalChannels[iUserIndex][iChannelSlot]);
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
    if (iSelect == 'luid')
    {
        if (pBuf)
        {
            if (iBufSize >= (signed)sizeof(VoipUserT))
            {
                if (!VOIP_NullUser((VoipUserT *)(&pVoipCommon->Connectionlist.LocalUsers[iValue])))             
                {
                    // copy user id in caller-provided buffer
                    ds_memcpy(pBuf, &pVoipCommon->Connectionlist.LocalUsers[iValue], sizeof(VoipUserT));
                    return(0);
                }
            }
            else
            {
                NetPrintf(("voipcommon: VoipCommonStatus('luid') error -->  iBufSize (%d) not big enough (needs to be at least %d bytes)\n",
                    iBufSize, sizeof(VoipUserT)));
            }
        }
        else
        {
            NetPrintf(("voipcommon: VoipCommonStatus('luid') error --> pBuf cannot be NULL\n"));
        }

        return(-1);
    }
    if (iSelect == 'maxc')
    {
        return(pVoipCommon->Connectionlist.iMaxConnections);
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
        // make sure user-provided buffer is large enough to receive a pointer
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
        if (pBuf)
        {
            if (iBufSize == sizeof(uint32_t))
            {
                // pBut is used as an input and an output parameter
                // input -> user index
                // output -> returned channels
                int32_t iUserIndex = *(uint32_t *)pBuf;
                *(uint32_t *)pBuf = pVoipCommon->Connectionlist.pConnections[iValue].aRecvChannels[iUserIndex];
                return(0);
            }
            else
            {
                NetPrintf(("voipcommon: VoipCommonStatus('rchn') error for conn id %d -->  iBufSize (%d) does not match expected size (%d)\n",
                    iValue, iBufSize, sizeof(int32_t)));
            }
        }
        else
        {
            NetPrintf(("voipcommon: VoipCommonStatus('rchn') error for conn id %d --> pBuf cannot be NULL\n", iValue));
        }

        return(-1);
    }
    if (iSelect == 'ruid')
    {
        int32_t iConnIndex = iValue & 0xFFFF;
        int32_t iRemoteUserIndex = iValue >> 16;

        if (pBuf)
        {
            if (iBufSize >= (signed)sizeof(VoipUserT))
            {
                if (pVoipCommon->Connectionlist.pConnections[iConnIndex].eState != ST_DISC)
                {
                    if (!VOIP_NullUser((VoipUserT *)(&pVoipCommon->Connectionlist.pConnections[iConnIndex].RemoteUsers[iRemoteUserIndex])))
                    {
                        // copy user id in caller-provided buffer
                        ds_memcpy(pBuf, &pVoipCommon->Connectionlist.pConnections[iConnIndex].RemoteUsers[iRemoteUserIndex], sizeof(VoipUserT));
                        return(0);
                    }
                }
                else
                {
                    NetPrintf(("voipcommon: VoipCommonStatus('ruid') error for conn id %d and remote user index %d --> connection is not active\n",
                            iConnIndex, iRemoteUserIndex));
                }
            }
            else
            {
                NetPrintf(("voipcommon: VoipCommonStatus('ruid') error for conn id %d and remote user index %d --> iBufSize (%d) not big enough (needs to be at least %d bytes)\n",
                    iConnIndex, iRemoteUserIndex, iBufSize, sizeof(VoipUserT)));
            }
        }
        else
        {
            NetPrintf(("voipcommon: VoipCommonStatus('ruid') error for conn id %d and remote user index %d --> pBuf cannot be NULL\n", iConnIndex, iRemoteUserIndex));
        }

        return(-1);
    }
    if (iSelect == 'shch')
    {
        return(pVoipCommon->bUseDefaultSharedChannelConfig);
    }
    if (iSelect == 'sock')
    {
        if (pBuf != NULL)
        {
            if (iBufSize >= (signed)sizeof(pVoipCommon->Connectionlist.pSocket))
            {
                ds_memcpy(pBuf, &pVoipCommon->Connectionlist.pSocket, sizeof(pVoipCommon->Connectionlist.pSocket));
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
        return(pVoipCommon->uUserMicrValue);
    }
    if (iSelect == 'uspk')
    {
        return(pVoipCommon->uUserSpkrValue);
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
            'shch' - turn on or the of the default shared channel config. iValue = TRUE to turn, FALSE to turn off
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
        int32_t iUserIndex = *(int32_t *)pValue;
        NetPrintf(("voipcommon: setting voip channels (0x%08x) for local user index %d\n", iValue, iUserIndex));
        pVoipCommon->Connectionlist.aChannels[iUserIndex] = iValue;
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
    if (iControl == 'shch')
    {
        if (VOIP_SHARED_USER_INDEX < 0)
        {
            NetPrintf(("voipcommon: VoipCommonStatus('shch') shared channel config is not supported on this platform\n"));
        }
        else
        {
            pVoipCommon->bUseDefaultSharedChannelConfig = iValue;
            _VoipCommonComputeDefaultSharedChannelConfig(pVoipCommon);
            _VoipCommonSetSharedUserChannelConfig(pVoipCommon);
            VoipCommonApplyChannelConfig(pVoipCommon);
            NetPrintf(("voipcommon: VoipCommonStatus('shch') default shared channel config is %s\n", pVoipCommon->bUseDefaultSharedChannelConfig ? "on" : "off"));
        }
        return(0);
    }
    if (iControl == 'time')
    {
        pVoipCommon->Connectionlist.iDataTimeout = iValue;
        return(0);
    }
    if (iControl == 'umic')
    {
        pVoipCommon->uUserMicrValue = iValue;
        _VoipCommonComputeEffectiveUserSendMask(pVoipCommon);
        return(0);
    }
    if (iControl == 'uspk')
    {
        pVoipCommon->uUserSpkrValue = iValue;
        _VoipCommonComputeEffectiveUserRecvMask(pVoipCommon);
        return(0);
    }

    // unhandled selectors are passed through to voipheadset
    if (pVoipCommon)
    {
        return(VoipHeadsetControl(pVoipCommon->pHeadset, iControl, iValue, 0, pValue));
    }
    else
    {
        // some voipheadset control selectors can be used before a call to VoipStartup()
        return(VoipHeadsetControl(NULL, iControl, iValue, 0, pValue));
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonAddMask

    \Description
        Add (OR) uAddMask into *pMask

    \Input *pMask       - mask to add into
    \Input uAddMask     - mask to add (OR)
    \Input *pMaskName   - name of mask (for debug logging)

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

    \Version 12/03/2009 (jbrookes)
*/
/********************************************************************************F*/
void VoipCommonSetMask(uint32_t *pMask, uint32_t uNewMask, const char *pMaskName)
{
    NetPrintf(("voipcommon: %s update: 0x%08x->0x%08x\n", pMaskName, *pMask, uNewMask));
    *pMask = uNewMask;
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonSelectChannel

    \Description
        Select the mode(send/recv) of a given channel.

    \Input *pVoipCommon - common module state
    \Input iUserIndex   - local user index
    \Input iChannel     - Channel ID (valid range: [0,63])
    \Input eMode        - The mode, combination of VOIP_CHANSEND, VOIP_CHANRECV

    \Output
        int32_t         - number of channels remaining that this console could join

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipCommonSelectChannel(VoipCommonRefT *pVoipCommon, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode)
{
    int32_t iIndex;
    int32_t iSlot = VOIP_MAX_CONCURRENT_CHANNEL;
    int32_t iCount = 0;
 
    // if the shared user index is -1, the current platform does not support shared channel config feature
    if (iUserIndex < 0)
    {
        NetPrintf(("voipcommon: VoipCommonSelectChannel() invalid user index passed in. Shared channel config is not supported on this platform\n"));
        return(-2);
    }

    // enforcing the valid ranges
    eMode &= VOIP_CHANSENDRECV;
    iChannel &= 0x3f;

    // find the slot to store the specified channel and count the slots remaining.
    for(iIndex = 0; iIndex < VOIP_MAX_CONCURRENT_CHANNEL; iIndex++)
    {
        // remember either the slot with the channel we want to set, or if we have found nothing, an empty slot
        if ( ((pVoipCommon->uLocalModes[iUserIndex][iIndex] != VOIP_CHANNONE) && (pVoipCommon->iLocalChannels[iUserIndex][iIndex] == iChannel)) ||
             ((pVoipCommon->uLocalModes[iUserIndex][iIndex] == VOIP_CHANNONE) && (iSlot == VOIP_MAX_CONCURRENT_CHANNEL)) )
        {
            iSlot = iIndex;
        }
        // and take the opportunity to count the free slots
        if (pVoipCommon->uLocalModes[iUserIndex][iIndex] == VOIP_CHANNONE)
        {
            iCount++;
        }
    }

    // no more slots to store the channel selection or
    // the given channel doesn't exist
    if (iSlot == VOIP_MAX_CONCURRENT_CHANNEL)
    {
        return(-1);
    }

    //count if we're taking a spot.
    if ((pVoipCommon->uLocalModes[iUserIndex][iSlot] == VOIP_CHANNONE) && (eMode != VOIP_CHANNONE))
    {
        iCount--;
    }
    //count if we're freeing a spot.
    if ((pVoipCommon->uLocalModes[iUserIndex][iSlot] != VOIP_CHANNONE) && (eMode == VOIP_CHANNONE))
    {
        iCount++;
    }

    // if we are trying to set the shared channel 
    if (iUserIndex == VOIP_SHARED_USER_INDEX)
    {
        pVoipCommon->iCustomSharedChannels[iSlot] = iChannel;
        pVoipCommon->uCustomSharedModes[iSlot] = eMode;
    }
    // set the channel and mode in selected slot
    else
    {
        pVoipCommon->iLocalChannels[iUserIndex][iSlot] = iChannel;
        pVoipCommon->uLocalModes[iUserIndex][iSlot] = eMode;

        NetPrintf(("voipcommon: [channel] set local channel at slot %d: channelId=%d, channelMode=%u for local user index %d\n",
        iSlot, pVoipCommon->iLocalChannels[iUserIndex][iSlot], pVoipCommon->uLocalModes[iUserIndex][iSlot], iUserIndex));
        pVoipCommon->uLocalChannelSelection[iUserIndex] = _VoipCommonEncode(pVoipCommon->iLocalChannels[iUserIndex], pVoipCommon->uLocalModes[iUserIndex]);
        NetPrintf(("voipcommon: [channel] set local channels 0x%08x for local user index %d\n", pVoipCommon->uLocalChannelSelection[iUserIndex], iUserIndex));
    }

    // calulate the shared user config
    _VoipCommonComputeDefaultSharedChannelConfig(pVoipCommon);

    // set the shared user config to custom or default
    _VoipCommonSetSharedUserChannelConfig(pVoipCommon);

    VoipCommonApplyChannelConfig(pVoipCommon);
    VoipControl(VoipGetRef(), 'chan', pVoipCommon->uLocalChannelSelection[iUserIndex], &iUserIndex);

    return(iCount);

}

/*F********************************************************************************/
/*!
    \Function   VoipCommonApplyChannelConfig

    \Description
        Setup user muting flags based on channel config

    \Input *pVoipCommon - voip module state

    \Version 12/02/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipCommonApplyChannelConfig(VoipCommonRefT *pVoipCommon)
{
    static uint32_t uLastChanSendMask = 0;
    static uint32_t uLastChanRecvMask = 0;
    int32_t iConnIndex, iLocalUserIndex, iRemoteUserIndex;
    uint32_t uConnUserPair;
    VoipChanModeE eMatch;
    uint8_t aLocalUserId[32];   // known to be big enough to receive a user id on any platform  (see definition of VoipUserT)
    uint8_t aRemoteUserId[32];  // known to be big enough to receive a user id on any platform  (see definition of VoipUserT)

    pVoipCommon->uChanRecvMask = pVoipCommon->uChanSendMask = 0;

    for (iLocalUserIndex = 0; iLocalUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iLocalUserIndex++)
    {
        // find out if there is a local user for that local user index
        if (VoipStatus(VoipGetRef(), 'luid', iLocalUserIndex, &aLocalUserId, sizeof(aLocalUserId)) == 0)
        {
            for(iConnIndex = 0; iConnIndex < VoipStatus(VoipGetRef(), 'maxc', 0, NULL, 0); iConnIndex++)
            {
                if (pVoipCommon->Connectionlist.pConnections[iConnIndex].eState != ST_DISC)
                {
                    for (iRemoteUserIndex = 0; iRemoteUserIndex < VOIP_MAXLOCALUSERS_EXTENDED; iRemoteUserIndex++)
                    {
                        // initialize variable used with VoipStatus('ruid'). most-significant 16 bits = remote user index, least-significant 16 bits = conn index
                        uConnUserPair = iConnIndex;
                        uConnUserPair |= (iRemoteUserIndex << 16);

                        // find out if there is a valid remote user for that connId/userId pair
                        if (VoipStatus(VoipGetRef(), 'ruid', (int32_t)uConnUserPair, &aRemoteUserId, sizeof(aRemoteUserId)) == 0)
                        {
                            eMatch = _VoipCommonChannelMatch(pVoipCommon, iLocalUserIndex, pVoipCommon->uRemoteChannelSelection[iConnIndex][iRemoteUserIndex]);

                            // update send mask
                            if (eMatch & VOIP_CHANSEND)
                            {
                                pVoipCommon->uChanSendMask |= (1 << iConnIndex);
                            }

                            // update receive mask and user-specific playback config
                            if (eMatch & VOIP_CHANRECV)
                            {
                                pVoipCommon->uChanRecvMask |= (1 << iConnIndex);

                                // remote users exist in the connection list before they are registered with voipheadset later in the voip thread
                                // therefore we must make sure the remote user is registered with voip headset before applying voice playback
                                if (VoipHeadsetStatus(pVoipCommon->pHeadset, 'ruid', 0, &aRemoteUserId, sizeof(aRemoteUserId)))
                                {
                                    // enable playback of the remote user's voice for the specified local user
                                    VoipControl(VoipGetRef(), '+pbk', iLocalUserIndex, &aRemoteUserId);
                                }
                            }
                            else
                            {
                                // remote users exist in the connection list before they are registered with voipheadset later in the voip thread
                                // therefore we must make sure the remote user is registered with voip headset before applying voice playback
                                if (VoipHeadsetStatus(pVoipCommon->pHeadset, 'ruid', 0, &aRemoteUserId, sizeof(aRemoteUserId)))
                                {
                                    // disable playback of the remote user's voice for the specified local user
                                    VoipControl(VoipGetRef(), '-pbk', iLocalUserIndex, &aRemoteUserId);
                                }
                            }
                        } // if
                    } // for
                } // if
            } // for
        } // if
    } // for

    if (uLastChanSendMask != pVoipCommon->uChanSendMask)
    {
        NetPrintf(("voipcommon: uChanSendMask is now 0x%08x\n", pVoipCommon->uChanSendMask));
        uLastChanSendMask = pVoipCommon->uChanSendMask;
    }

    if (uLastChanRecvMask != pVoipCommon->uChanRecvMask)
    {
        NetPrintf(("voipcommon: uChanRecvMask is now 0x%08x\n", pVoipCommon->uChanRecvMask));
        uLastChanRecvMask = pVoipCommon->uChanRecvMask;
    }

    _VoipCommonComputeEffectiveUserSendMask(pVoipCommon);
    _VoipCommonComputeEffectiveUserRecvMask(pVoipCommon);
}

/*F********************************************************************************/
/*!
    \Function   VoipCommonResetChannels

    \Description
        Resets the channels selection to defaults. Sends and receives to all

    \Input *pVoipCommon     - voip common state
    \Input iUserIndex       - local user index

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipCommonResetChannels(VoipCommonRefT *pVoipCommon, int32_t iUserIndex)
{
    int32_t iChannelSlot;

    // if the default config is on ignore the channel reset on the shared user
    if ((pVoipCommon->bUseDefaultSharedChannelConfig) && (iUserIndex == VOIP_SHARED_USER_INDEX))
    {
        return;
    }

    for (iChannelSlot = 0; iChannelSlot < VOIP_MAX_CONCURRENT_CHANNEL; iChannelSlot++)
    {
        pVoipCommon->iLocalChannels[iUserIndex][iChannelSlot] = 0;
        pVoipCommon->uLocalModes[iUserIndex][iChannelSlot] = 0;
    }

    pVoipCommon->uLocalChannelSelection[iUserIndex] = _VoipCommonEncode(pVoipCommon->iLocalChannels[iUserIndex], pVoipCommon->uLocalModes[iUserIndex]);
    NetPrintf(("voipcommon: [channel] set local channels 0x%08x\n", pVoipCommon->uLocalChannelSelection[iUserIndex]));

    if (VoipGetRef() != NULL)
    {
        VoipCommonApplyChannelConfig(pVoipCommon);
        VoipControl(VoipGetRef(), 'chan', pVoipCommon->uLocalChannelSelection[iUserIndex], &iUserIndex);
    }
}
