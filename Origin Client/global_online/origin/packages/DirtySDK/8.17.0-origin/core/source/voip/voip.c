/*H********************************************************************************/
/*!
    \File voip.c

    \Description
        Cross-platform public voip functions.

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
#include "netconn.h"

#include "voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "voip.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F*************************************************************************************************/
/*!
    \Function _VoipCallback
    
    \Description
        Default (empty) user callback function.
        
    \Input *pVoip       - voip module state
    \Input eCbType      - type of event
    \Input iValue       - event-specific information
    \Input *pUserData   - callback user data
    
    \Output
        None.

    \Version 02/10/2006 (jbrookes)
*/
/*************************************************************************************************F*/
static void _VoipCallback(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData)
{
}


/*** Public functions *************************************************************/


/*F*************************************************************************************************/
/*!
    \Function VoipSetEventCallback

    \Description
        Set Voip event callback notification handler.

    \Input *pVoip       - module state from VoipStartup
    \Input *pCallback   - event callback notification handler
    \Input *pUserData   - user data for callback handler

    \Output
        None.

    \Version 02/10/2006 (jbrookes) first version
    \Version 11/05/2009 (mclouatre) if pCallback is NULL, revert to default callback
*/
/*************************************************************************************************F*/
void VoipSetEventCallback(VoipRefT *pVoip, VoipCallbackT *pCallback, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    if (pCallback == NULL)
    {
        pVoipCommon->pCallback = _VoipCallback;
        pVoipCommon->pUserData = NULL;
    }
    else
    {
        pVoipCommon->pCallback = pCallback;
        pVoipCommon->pUserData = pUserData;
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipConnect

    \Description
        Connect to a peer.

    \Input *pVoip       - module state from VoipStartup
    \Input iConnID      - [zero, iMaxPeers-1] for an explicit slot or VOIP_CONNID_NONE to auto-allocate
    \Input *pUserID     - user ID
    \Input uAddress     - remote peer address
    \Input uManglePort  - port from demangler (should be identical to game port on Xenon)
    \Input uGamePort    - port to connect on
    \Input uClientId    - remote clientId to connect to (cannot be 0)
    \Input uSessionId   - session identifier (optional)
    
    \Output
        int32_t         - connection identifier (negative=error)

    \Version 1.0 03/02/2004 (jbrookes) first version
    \Version 1.1 10/26/2009 (mclouatre) uClientId is no longer optional
*/
/********************************************************************************F*/
int32_t VoipConnect(VoipRefT *pVoip, int32_t iConnID, const char *pUserID, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    
    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    // initiate connection
    iConnID = VoipConnectionStart(&pVoipCommon->Connectionlist, iConnID, (VoipUserT *)pUserID, uAddress, uManglePort, uGamePort, uClientId, uSessionId);

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);

    // return connection ID to caller
    return(iConnID);
}

/*F********************************************************************************/
/*!
    \Function   VoipDisconnect

    \Description
        Disconnect from peer.

    \Input *pVoip   - module state from VoipStartup
    \Input iConnID  - which connection to disconnect (VOIP_CONNID_ALL for all)
    \Input uAddress - remote peer address

    \Todo
        Multiple connection support.
        Deprecate unused uAddress parameter

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipDisconnect(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    
    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    // shut down connection
    VoipConnectionStop(&pVoipCommon->Connectionlist, iConnID, TRUE);

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function   VoipRemove

    \Description
        Remove connection from connectionlist (do not contract list).

    \Input  *pVoip      - module state from VoipStartup
    \Input  iConnID     - which connection to remove from list
    
    \Version 1.0 06/14/2008 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipRemove(VoipRefT *pVoip, int32_t iConnID)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    
    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    // shut down connection
    VoipConnectionRemove(&pVoipCommon->Connectionlist, iConnID);

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function   VoipRemote

    \Description
        Return information about remote peer.

    \Input  *pVoip      - module state from VoipStartup
    \Input  iConnID     - which connection to get remote info for, or VOIP_CONNID_ALL

    \Output
        int32_t         - VOIP_REMOTE_* flags, or VOIP_FLAG_INVALID if iConnID is invalid

    \Version 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipRemote(VoipRefT *pVoip, int32_t iConnID)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    int32_t iRemoteStatus = VOIP_FLAG_INVALID;

    if (pVoipCommon != NULL && pVoipCommon->Connectionlist.pConnections != NULL)
    {
        if (iConnID == VOIP_CONNID_ALL)
        {
            for (iConnID = 0, iRemoteStatus = 0; iConnID < pVoipCommon->Connectionlist.iMaxConnections; iConnID++)
            {
                iRemoteStatus |= (int32_t)pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteStatus;
            }
        }
        else if ((iConnID >= 0) && (iConnID < pVoipCommon->Connectionlist.iMaxConnections))
        {
            iRemoteStatus = pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteStatus;
        }
    }

    return(iRemoteStatus);
}

/*F********************************************************************************/
/*!
    \Function   VoipLocal

    \Description
        Return information about local hardware state

    \Input *pVoip       - module state from VoipStartup
    
    \Output
        int32_t         - VOIP_LOCAL_* flags
        
    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t VoipLocal(VoipRefT *pVoip)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    uint32_t uLocalStatus = pVoipCommon->Connectionlist.uLocalStatus;
    
    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    // do we need to ignore the headset flag?
    if ((pVoipCommon->iLockedPort >= 0) && (pVoipCommon->iActivePort != pVoipCommon->iLockedPort))
    {
        uLocalStatus &= ~VOIP_LOCAL_HEADSETOK;
    }
    #endif
    
    return(uLocalStatus);
}

/*F********************************************************************************/
/*!
    \Function   VoipMicrophone

    \Description
        Select which peers to send voice to.

    \Input  *pVoip      - module state from VoipStartup
    \Input  uConnMask   - bitmask indicating which connections to send to (or VOIP_CONNID_*)
    
    \Output
        None.

    \Notes
        On Xenon, user-specified recv masks take lower priority than muting and
        permissions managed by the HUD.

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipMicrophone(VoipRefT *pVoip, uint32_t uConnMask)
{
    VoipControl(pVoip, 'umic', uConnMask, NULL);
}

/*F********************************************************************************/
/*!
    \Function   VoipSpeaker

    \Description
        Select which peers to accept voice from.

    \Input  *pVoip      - module state from VoipStartup
    \Input  uConnMask   - bitmask indicating which connections to receive from (or VOIP_CONNID_*)

    \Output
        None.

    \Notes
        On Xenon, user-specified recv masks take lower priority than muting and
        permissions managed by the HUD.

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipSpeaker(VoipRefT *pVoip, uint32_t uConnMask)
{
    VoipControl(pVoip, 'uspk', uConnMask, NULL);
}

/*F*************************************************************************************************/
/*!
    \Function   VoipGetProfileStat
    
    \Description
        Returns current profile data.

    \Input  *pVoip      - module state from VoipStartup
    \Input  eProfStat   - type of profile data to get

    \Output
        int32_t         - profile data

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/*************************************************************************************************F*/
int32_t VoipGetProfileStat(VoipRefT *pVoip, VoipProfStatE eProfStat)
{
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    VoipGetProfileTime

    \Description
        Returns current profile time from IOP

    \Input  *pVoip  - module state from VoipStartup

    \Output
        int32_t     - amount of time that elapsed during previous profiling cycle

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/*************************************************************************************************F*/
int32_t VoipGetProfileTime(VoipRefT *pVoip)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipGetRef

    \Description
        Return current module reference.

    \Output
        VoipRefT *      - reference pointer, or NULL if the module is not active

    \Version 1.0 03/08/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipGetRef(void)
{
    return(VoipCommonGetRef());
}

/*F*************************************************************************************************/
/*!
    \Function VoipSpkrCallback
    
    \Description
        Set speaker data callback (not supported on all platforms).
            
    \Input *pVoip       - voip module state
    \Input *pCallback   - data callback
    \Input *pUserData   - user data
    
    \Version 12/12/2005 (jbrookes)
*/
/*************************************************************************************************F*/
void VoipSpkrCallback(VoipRefT *pVoip, VoipSpkrCallbackT *pCallback, void *pUserData)
{
    #if DIRTYCODE_PLATFORM == DIRTYCODE_PC || DIRTYCODE_PLATFORM == DIRTYCODE_PS3
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    VoipHeadsetSpkrCallback(pVoipCommon->pHeadset, pCallback, pUserData);
    #endif
}

