/*H********************************************************************************/
/*!
    \File voipxenon.c

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 05/13/2005 (jbrookes) First Version.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xonline.h>

// dirtysock includes
#include "platform.h"
#include "dirtysock.h"
#include "dirtyerr.h"
#include "netconn.h"

// voip includes
#include "voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"

#include "voip.h"

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    VoipCommonRefT          Common;             //!< cross-platform voip data (must come first!)
    HANDLE                  hListener;          //!< listener for xenon events

    DWORD                   dwThreadId;         //!< thread ID
    volatile uint32_t       iThreadActive;      //!< control variable used during thread creation and exit
    volatile HANDLE         hThread;            //!< thread handle used to signal shutdown

    int32_t                 iFriendVersion;     //!< version of friends list. Used to detect friends change
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

    \Input pVoiceData   - pointer to mic data
    \Input iDataSize    - size of mic data
    \Input uLocalIdx    - local index
    \Input uSendSeqn    - send sequence
    \Input pUserData    - pointer to callback user data

    \Version 03/31/04 (JLB)
*/
/********************************************************************************F*/
static void _VoipMicDataCb(const void *pVoiceData, int32_t iDataSize, uint32_t uLocalIdx, uint8_t uSendSeqn, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;
    VoipConnectionSend(&pVoip->Common.Connectionlist, (const uint8_t *)pVoiceData, iDataSize, uLocalIdx, uSendSeqn);
}

/*F********************************************************************************/
/*!
    \Function    _VoipStatusCb

    \Description
        Callback to handle change of headset status.

    \Input iHeadset     - headset that has changed (currently ignored)
    \Input uStatus      - headset status
    \Input pUserData    - pointer to callback user data

    \Version 04/01/04 (JLB)
*/
/********************************************************************************F*/
static void _VoipStatusCb(int32_t iHeadset, uint32_t uStatus, void *pUserData)
{
    VoipRefT *pVoip = (VoipRefT *)pUserData;

    // update local flags
    if ((1 << pVoip->Common.iActivePort) & uStatus)
    {
        pVoip->Common.Connectionlist.uLocalStatus |= VOIP_LOCAL_HEADSETOK;
    }
    else
    {
        pVoip->Common.Connectionlist.uLocalStatus &= ~VOIP_LOCAL_HEADSETOK;
    }

    // save updated status
    pVoip->Common.uPortHeadsetStatus = uStatus;
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdateFriendsStatus

    \Description
        Update friend status bitmask.

    \Input *pVoip       - voip module reference
    \Input iConnId      - connection id

    \Version 04/18/2007 (jrainy)
*/
/********************************************************************************F*/
static void _VoipUpdateFriendsStatus(VoipRefT *pVoip, int32_t iConnId)
{
    int32_t iChannel;
    uint32_t uChannelMask;
    BOOL uFriendOnly;
    int32_t index;

    if ((pVoip->iFriendVersion != NetConnStatus('frnv', 0, NULL, 0)) || (iConnId != VOIP_CONNID_NONE))
    {
        if ((iConnId == VOIP_CONNID_NONE) || (iConnId == VOIP_CONNID_ALL))
        {
            pVoip->Common.Connectionlist.uFriendsMask = ((unsigned)(-1)) & ~(((unsigned)(-1)) << pVoip->Common.Connectionlist.iMaxConnections);
        }
        else
        {
            pVoip->Common.Connectionlist.uFriendsMask |= (1 << iConnId);
        }

        // for each remote user R (channels) if one of the signed-in user U has friends-only comms and U and R are not-friend,
        // we prevent sending to R from this console.
        for (iChannel = 0; iChannel < pVoip->Common.Connectionlist.iMaxConnections; iChannel++)
        {
            if ((iConnId == VOIP_CONNID_NONE) || (iConnId == VOIP_CONNID_ALL) || (iChannel == iConnId))
            {
                VoipConnectionT *pConnection = &pVoip->Common.Connectionlist.pConnections[iChannel];
                uChannelMask = 1 << iChannel;

                for(index = 0; index < 4; index++)
                {
                    DWORD dwResult;
                    dwResult = XUserCheckPrivilege(index, XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY, &uFriendOnly);

                    // $$ todo - need to check all active user's friendness against all remote active user
                    if ((dwResult==ERROR_SUCCESS) && uFriendOnly)
                    {
                        XUID Xuid;
                        VoipXuidFromUser(&Xuid, &pConnection->RemoteUsers[pConnection->iPrimaryRemoteUser]);

                        if (!NetConnStatus('isfr', index, &Xuid, 0))
                        {
                            pVoip->Common.Connectionlist.uFriendsMask &= ~uChannelMask;
                        }
                    }
                }
            }
        }
        pVoip->iFriendVersion = NetConnStatus('frnv', 0, NULL, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdateMuteStatus

    \Description
        Process mute list, and cache results for each user.

    \Input *pVoip       - pointer to module state

    \Version 08/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipUpdateMuteStatus(VoipRefT *pVoip)
{
    int32_t iChannel;
    BOOL bMuted;
    XUID Xuid;

    // do we need an update?
    if (!pVoip->Common.Connectionlist.bUpdateMute)
    {
        return;
    }

    // process all active channels
    for (iChannel = 0; iChannel < pVoip->Common.Connectionlist.iMaxConnections; iChannel++)
    {
        VoipConnectionT *pConnection = &pVoip->Common.Connectionlist.pConnections[iChannel];

        // if not active yet, don't process
        if (pConnection->eState != ST_ACTV)
        {
            continue;
        }

        // refresh mute setting
        // $$ todo - need to check all active user's mute settings against all remote active user's mute settings
        VoipXuidFromUser(&Xuid, &pConnection->RemoteUsers[pConnection->iPrimaryRemoteUser]);
        XUserMuteListQuery(pVoip->Common.iActivePort, Xuid, &bMuted);
        if (pConnection->bMuted != (uint8_t)bMuted)
        {
            NetPrintf(("voipxenon: %d) muting is %s\n", iChannel, bMuted ? "enabled" : "disabled"));
            pConnection->bMuted = bMuted;
        }
    }

    // clear update flag
    pVoip->Common.Connectionlist.bUpdateMute = FALSE;
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdatePrivileges

    \Description
        Update privileges for current local user.

    \Input *pVoip       - pointer to module state

    \Version 08/23/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipUpdatePrivileges(VoipRefT *pVoip)
{
    DWORD dwResult;

    // check global communications privilege

    dwResult = XUserCheckPrivilege(XUSER_INDEX_ANY, XPRIVILEGE_COMMUNICATIONS, &pVoip->Common.bPrivileged);
    if (dwResult != ERROR_SUCCESS)
    {
        NetPrintf(("voipxenon: error 0x%08x trying to get XPRIVILEGE_COMMUNICATIONS\n", dwResult));
    }

    dwResult = XUserCheckPrivilege(XUSER_INDEX_ANY, XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY, &pVoip->Common.bFriendsPrivileged);
    if (dwResult != ERROR_SUCCESS)
    {
        NetPrintf(("voipxenon: error 0x%08x trying to get XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY\n", dwResult));
    }

    // echo privilege status
    NetPrintf(("voipxenon: voice is %s\n", pVoip->Common.bPrivileged ? "enabled" : (pVoip->Common.bFriendsPrivileged ? "friends only" : "disabled")));
}

/*F********************************************************************************/
/*!
    \Function   _VoipRegisterRemoteTalkers

    \Description
        Register or unregister all remote talkers on all connections.

    \Input *pVoip       - module state
    \Input bRegister    - TRUE to register, FALSE to unregister.

    \Version 03/26/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipRegisterRemoteTalkers(VoipRefT *pVoip, uint32_t bRegister)
{
    int32_t iConnID;

    for (iConnID = 0; iConnID < pVoip->Common.Connectionlist.iMaxConnections; iConnID++)
    {
        VoipConnectionT *pConnection = &pVoip->Common.Connectionlist.pConnections[iConnID];
        if (pConnection->eState == ST_ACTV)
        {
            VoipConnectionRegisterRemoteTalkers(&pVoip->Common.Connectionlist, iConnID, bRegister);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipRegisterLocalTalker

    \Description
        Register the local talker on the given port.

    \Input *pVoip       - voip module reference
    \Input iPort        - port to register talker on
    \Input bRegister    - TRUE to register user, FALSE to unregister
    \Input *pVoipUser   - VoipUserT to register

    \Output
        int32_t         - TRUE if the local talker was successfully registered, else FALSE

    \Version 03/26/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipRegisterLocalTalker(VoipRefT *pVoip, int32_t iPort, uint32_t bRegister, VoipUserT *pVoipUser)
{
    int32_t iResult;
    if ((iResult = VoipHeadsetControl(pVoip->Common.pHeadset, 'rloc', iPort, bRegister, pVoipUser)) != 0)
    {
        if (bRegister)
        {
            pVoip->Common.iActivePort = iPort;

            // tell connectionlist who is "active" user
            pVoip->Common.Connectionlist.iPrimaryLocalUser = pVoip->Common.iActivePort;

            // add user to local list
            if (pVoipUser != NULL)
            {
                memcpy(&pVoip->Common.Connectionlist.LocalUsers[iPort], pVoipUser, sizeof(*pVoipUser));
            }
        }
        else
        {
            pVoip->Common.Connectionlist.uLocalStatus &= ~(VOIP_LOCAL_MICON|VOIP_LOCAL_HEADSETOK);
            memset(&pVoip->Common.Connectionlist.LocalUsers[iPort], 0, sizeof(pVoip->Common.Connectionlist.LocalUsers[iPort]));
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function   _VoipRegisterLocalTalkerSingle

    \Description
        Register the local talker on the given port, unregistering the previously
        registered user (if any).

    \Input *pVoip       - voip module reference
    \Input iPortID      - port to register talker on
    \Input bRegister    - TRUE to register user, FALSE to unregister

    \Output
        int32_t         - TRUE if the local talker was successfully registered, else FALSE

    \Version 03/26/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipRegisterLocalTalkerSingle(VoipRefT *pVoip, int32_t iPortID, uint32_t bRegister)
{
    // if a previous local user was registered, unregister them
    if ((bRegister == TRUE) && (pVoip->Common.iActivePort != -1))
    {
        _VoipRegisterLocalTalker(pVoip, iPortID, FALSE, NULL);
        pVoip->Common.iActivePort = -1;
    }

    return(_VoipRegisterLocalTalker(pVoip, iPortID, bRegister, NULL));
}

/*F********************************************************************************/
/*!
    \Function   _VoipSetLocalPort

    \Description
        Switch to a new local port.

    \Input *pVoip   - voip module reference
    \Input iPort    - port to register talker on

    \Output
        int32_t     - TRUE if the local talker was successfully registered, else FALSE

    \Notes
        Only one local talker is currently supported.

    \Version 03/26/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipSetLocalPort(VoipRefT *pVoip, int32_t iPort)
{
    int32_t iSuccess;

    // make sure we're not already set
    if (pVoip->Common.iActivePort == iPort)
    {
        return(TRUE);
    }

    // unregister remote talkers from current port
    _VoipRegisterRemoteTalkers(pVoip, FALSE);

    // register the local talker
    iSuccess = _VoipRegisterLocalTalkerSingle(pVoip, iPort, TRUE);

    // register remote talkers to new port
    _VoipRegisterRemoteTalkers(pVoip, TRUE);

    // return if local talker registeration succeeded or not
    return(iSuccess);
}

/*F********************************************************************************/
/*!
    \Function   _VoipUpdate

    \Description
        Update Voip state.

    \Input *pVoip       - voip module state

    \Version 03/19/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipUpdate(VoipRefT *pVoip)
{
    DWORD dwNotify;
    ULONG ulParam;
    uint32_t uProcessCt = 0;

    // check for mutelist changes
    if (XNotifyGetNext(pVoip->hListener, XN_SYS_MUTELISTCHANGED, &dwNotify, &ulParam) != 0)
    {
        NetPrintf(("voipxenon: mutelist change->0x%02x\n", ulParam));
        pVoip->Common.Connectionlist.bUpdateMute = TRUE;
    }

    // update voice privileges on sign-in notification
    if (XNotifyGetNext(pVoip->hListener, XN_SYS_SIGNINCHANGED, &dwNotify, &ulParam) != 0)
    {
        _VoipUpdatePrivileges(pVoip);
    }

    // update mute status, if warranted
    _VoipUpdateMuteStatus(pVoip);

    // update friend status to remote users of local user
    _VoipUpdateFriendsStatus(pVoip, pVoip->Common.Connectionlist.iFriendConnId);
    pVoip->Common.Connectionlist.iFriendConnId = VOIP_CONNID_NONE;

    // update status of remote users
    VoipCommonUpdateRemoteStatus(&pVoip->Common);

    // update connection module
    VoipConnectionUpdate(&pVoip->Common.Connectionlist);

    // update headset module
    VoipHeadsetProcess(pVoip->Common.pHeadset, uProcessCt++);
}

/*F********************************************************************************/
/*!
    \Function   _VoipThread

    \Description
        Main Voip thread that updates all active connection and calls the
        XHV Engine's DoWork() function.

    \Input *pParam      - pointer to voip module state

    \Version 03/19/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipThread(void *pParam)
{
    VoipRefT *pVoip = (VoipRefT *)pParam;

    // execute until we're killed
    for (pVoip->iThreadActive = 1; pVoip->iThreadActive != 0; )
    {
        NetCritEnter(&pVoip->Common.ThreadCrit);
        _VoipUpdate(pVoip);
        NetCritLeave(&pVoip->Common.ThreadCrit);

        Sleep(20);
    }

    // indicate we're exiting
    pVoip->hThread = NULL;
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to VOIP_MAXCONNECT)
    \Input iMaxLocal    - maximum number of local users (up to four)

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 03/02/2004 (jbrookes)
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

    // register default user
    pVoip->Common.iActivePort = pVoip->Common.iLockedPort = -1;

    // create listener for live events
    if ((pVoip->hListener = XNotifyCreateListener(XNOTIFY_ALL)) == 0)
    {
        NetPrintf(("netconnxenon: unable to create live event listener\n"));
    }

    // create our own worker thread
    if ((pVoip->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_VoipThread, pVoip, 0, &pVoip->dwThreadId)) == NULL)
    {
        NetPrintf(("voipxenon: error creating voip thread (err=%s)\n", DirtyErrGetName(GetLastError())));
        VoipShutdown(pVoip);
        return(NULL);
    }

    // crank priority for best responsiveness
    SetThreadPriority(pVoip->hThread, THREAD_PRIORITY_HIGHEST);

    // wait for thread startup
    while (pVoip->iThreadActive == 0)
    {
        Sleep(1);
    }

    // return to caller
    return(pVoip);
}

/*F********************************************************************************/
/*!
    \Function   VoipShutdown

    \Description
        Release all VoIP resources -- DEPRECATE in V9

    \Input  *pVoip      - module state from VoipStartup

    \Version 03/02/2004 (jbrookes)
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

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipShutdown2(VoipRefT *pVoip, uint32_t uShutdownFlags)
{
    // make sure we're really started up
    if (VoipGetRef() == NULL)
    {
        NetPrintf(("voipxenon: module shutdown called when not in a started state\n"));
        return;
    }

    // tell thread to shut down and wait for shutdown confirmation
    for (pVoip->iThreadActive = 0; pVoip->hThread != NULL; )
    {
        NetPrintf(("voipxenon: waiting for shutdown\n"));
        Sleep(1);
    }

    // destroy listener
    if (pVoip->hListener != 0)
    {
        CloseHandle(pVoip->hListener);
    }

    // common shutdown (must come last as this frees the memory)
    VoipCommonShutdown(&pVoip->Common);
}

/*F********************************************************************************/
/*!
    \Function VoipSetLocalUser

    \Description
        Set local user ID.

    \Input *pVoip       - module state from VoipStartup
    \Input *pUserID     - local user ID
    \Input bRegister    - TRUE to register, FALSE to unregister.

    \Version 04/15/2004 (jbrookes)
*/
/********************************************************************************F*/
void VoipSetLocalUser(VoipRefT *pVoip, const char *pUserID, uint32_t bRegister)
{
    XUID Xuid, LocalXuid;
    int32_t iLocalIdx;

    // convert textual form to binary form
    VoipXuidFromUser(&LocalXuid, (VoipUserT *)pUserID);

    if (bRegister)
    {
        // find index of user
        for (iLocalIdx = 0; iLocalIdx < 4; iLocalIdx++)
        {
            XUserGetXUID(iLocalIdx, &Xuid);
            if (!memcmp(&Xuid, &LocalXuid, sizeof(Xuid)))
            {
                break;
            }
        }
        if (iLocalIdx < 4)
        {
            NetPrintf(("voipxenon: assigning local user %s to port %d\n", pUserID, iLocalIdx));

            // is there another user registered in this slot?
            if (!VOIP_NullUser(&pVoip->Common.Connectionlist.LocalUsers[iLocalIdx]))
            {
                NetPrintf(("voipxenon: warning -- ignoring attempt to register user to a port that is already registered to user %s\n",
                    pVoip->Common.Connectionlist.LocalUsers[iLocalIdx].strUserId));
                return;
            }

            // unregister remote talkers
            _VoipRegisterRemoteTalkers(pVoip, FALSE);

            // register the local talker
            _VoipRegisterLocalTalker(pVoip, iLocalIdx, TRUE, (VoipUserT *)pUserID);

            // register remote talkers
            _VoipRegisterRemoteTalkers(pVoip, TRUE);
        }
        else
        {
            NetPrintf(("voipxenon: could not find user to register with XUID=%s\n", pUserID));
        }
    }
    else
    {
        // find index of user
        for (iLocalIdx = 0; iLocalIdx < VOIP_MAXLOCALUSERS; iLocalIdx += 1)
        {
            if (VOIP_SameUser(&pVoip->Common.Connectionlist.LocalUsers[iLocalIdx], (VoipUserT *)pUserID))
            {
                _VoipRegisterLocalTalker(pVoip, iLocalIdx, FALSE, NULL);
                break;
            }
        }
        if (iLocalIdx == VOIP_MAXLOCALUSERS)
        {
            NetPrintf(("voipxenon: unable to find user to unregister with XUID=%s\n", pUserID));
        }
    }

    // get initial privilege status
    _VoipUpdatePrivileges(pVoip);
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
        Allowed selectors:

        \verbatim
            'lock' - return locked port identifier (negative=no locked port)
            'port' - returns current active port
        \endverbatim

        Unhandled selectors are passed through to VoipCommonStatus(), and
        further to VoipHeadsetStatus() if unhandled there.

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'lock')
    {
        return(pVoip->Common.iLockedPort);
    }
    if (iSelect == 'port')
    {
        return(pVoip->Common.iActivePort);
    }
    // let VoipCommon handle it
    return(VoipCommonStatus(&pVoip->Common, iSelect, iValue, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function   VoipControl

    \Description
        Set control options.

    \Input *pVoip       - module state from VoipStartup
    \Input iControl     - control selector
    \Input iValue       - selector-specific
    \Input *pValue      - selector-specific

    \Output
        int32_t         - selector-specific data

    \Notes
        Allowed selectors:

        \verbatim
            'afbk' - add address to the list of send address fallback for this connection
            'dfbk' - delete address from the list of send address fallback for this connection
            'lock' - lock local port to given port (negative=no lock)
            'port' - set active local port (returns TRUE on success, FALSE on failure)
        \endverbatim

        Unhandled selectors are passed through to VoipCommonControl(), and
        further to VoipHeadsetControl() if unhandled there.

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iControl, int32_t iValue, void *pValue)
{
    int32_t iRetCode;

    if (iControl == 'afbk')
    {
        // acquire critical section to modify ConnectionList
        NetCritEnter(&pVoip->Common.ThreadCrit);

        iRetCode = VoipConnectionAddFallbackAddr(&pVoip->Common.Connectionlist, iValue, *(uint32_t*)pValue);

        // release critical section to modify ConnectionList
        NetCritLeave(&pVoip->Common.ThreadCrit);

        return(iRetCode);
    }
    if (iControl == 'dfbk')
    {
        // acquire critical section to modify ConnectionList
        NetCritEnter(&pVoip->Common.ThreadCrit);

        iRetCode = VoipConnectionDeleteFallbackAddr(&pVoip->Common.Connectionlist, iValue, *(uint32_t*)pValue);

        // release critical section to modify ConnectionList
        NetCritLeave(&pVoip->Common.ThreadCrit);

        return(iRetCode);
    }
    if (iControl == 'lock')
    {
        pVoip->Common.iLockedPort = iValue;
        return(0);
    }
    if (iControl == 'port')
    {
        return(_VoipSetLocalPort(pVoip, iValue));
    }
    return(VoipCommonControl(&pVoip->Common, iControl, iValue, pValue));
}


