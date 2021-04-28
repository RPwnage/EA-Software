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

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"
#include "voipcommon.h"
#include "DirtySDK/voip/voip.h"

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
        Set voip event notification handler.

    \Input *pVoip       - module state from VoipStartup
    \Input *pCallback   - event notification handler
    \Input *pUserData   - user data for handler

    \Version 02/10/2006 (jbrookes)
*/
/*************************************************************************************************F*/
void VoipSetEventCallback(VoipRefT *pVoip, VoipCallbackT *pCallback, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;

    // acquire critical section
    NetCritEnter(&pVoipCommon->ThreadCrit);

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

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
}

/*F*************************************************************************************************/
/*!
    \Function VoipSetTranscribedTextReceivedCallback

    \Description
        Set transcribed text received notification handler.

    \Input *pVoip       - module state from VoipStartup
    \Input *pCallback   - notification handler
    \Input *pUserData   - user data for handler

    \Version 05/03/2017 (mclouatre)
*/
/*************************************************************************************************F*/
void VoipSetTranscribedTextReceivedCallback(VoipRefT *pVoip, VoipTranscribedTextReceivedCallbackT *pCallback, void *pUserData)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;

    // acquire critical section
    NetCritEnter(&pVoipCommon->ThreadCrit);

    if (pCallback == NULL)
    {
        pVoipCommon->pTranscribedTextRecvCb = pCallback;
        pVoipCommon->pTranscribedTextUserData = NULL;
    }
    else
    {
        pVoipCommon->pTranscribedTextRecvCb = pCallback;
        pVoipCommon->pTranscribedTextUserData = pUserData;
    }

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function   VoipConnect

    \Description
        Connect to a peer.

    \Input *pVoip       - module state from VoipStartup
    \Input iConnID      - [zero, iMaxPeers-1] for an explicit slot or VOIP_CONNID_NONE to auto-allocate
    \Input uAddress     - remote peer address
    \Input uManglePort  - port from demangler
    \Input uGamePort    - port to connect on
    \Input uClientId    - remote clientId to connect to (cannot be 0)
    \Input uSessionId   - session identifier (optional)
    
    \Output
        int32_t         - connection identifier (negative=error)

    \Version 1.0 03/02/2004 (jbrookes) first version
    \Version 1.1 10/26/2009 (mclouatre) uClientId is no longer optional
*/
/********************************************************************************F*/
int32_t VoipConnect(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;

    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    // initiate connection
    iConnID = VoipConnectionStart(&pVoipCommon->Connectionlist, iConnID, uAddress, uManglePort, uGamePort, uClientId, uSessionId);

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);

    // return connection ID to caller
    return(iConnID);
}

/*F********************************************************************************/
/*!
    \Function   VoipDisconnect2

    \Description
        Disconnect from peer.

    \Input *pVoip       - module state from VoipStartup
    \Input iConnID      - which connection to disconnect (VOIP_CONNID_ALL for all)
    \Input uAddress     - remote peer address
    \Input bSendDiscMsg - TRUE if a voip disc pkt needs to be sent, FALSE otherwise

    \Todo
        Multiple connection support.
        Deprecate unused uAddress parameter
        Rename VoipDisconnect2() to VoipDisconnect() when current version of VoipDisconnect() is deprecated

    \Version 15/01/2014 (mclouatre)
*/
/********************************************************************************F*/
void VoipDisconnect2(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress, int32_t bSendDiscMsg)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    
    // acquire critical section to modify ConnectionList
    NetCritEnter(&pVoipCommon->ThreadCrit);

    // shut down connection
    VoipConnectionStop(&pVoipCommon->Connectionlist, iConnID, bSendDiscMsg);

    // release critical section
    NetCritLeave(&pVoipCommon->ThreadCrit);
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
        Remove VoipDisconnect() in favor of VoipDisconnect2()

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipDisconnect(VoipRefT *pVoip, int32_t iConnID, uint32_t uAddress)
{
    VoipDisconnect2(pVoip, iConnID, uAddress, TRUE);
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
    \Function   VoipRemoteUserStatus

    \Description
        Return information about remote peer.

    \Input *pVoip            - module state from VoipStartup
    \Input  iConnID          - which connection to get remote info for, or VOIP_CONNID_ALL
    \Input  iRemoteUserIndex - user index at the connection iConnID

    \Output
        int32_t         - VOIP_REMOTE* flags, or VOIP_FLAG_INVALID if iConnID is invalid

    \Version 05/06/2014 (amakoukji)
*/
/********************************************************************************F*/
int32_t VoipRemoteUserStatus(VoipRefT *pVoip, int32_t iConnID, int32_t iRemoteUserIndex)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    int32_t iRemoteStatus = VOIP_FLAG_INVALID;

    if (pVoipCommon != NULL && pVoipCommon->Connectionlist.pConnections != NULL)
    {
        if (iConnID == VOIP_CONNID_ALL)
        {
            for (iConnID = 0, iRemoteStatus = 0; iConnID < pVoipCommon->Connectionlist.iMaxConnections; iConnID++)
            {
                iRemoteStatus |= (int32_t)pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteUserStatus[iRemoteUserIndex];
            }
        }
        else if ((iConnID >= 0) && (iConnID < pVoipCommon->Connectionlist.iMaxConnections))
        {
            iRemoteStatus = pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteUserStatus[iRemoteUserIndex];
        }
    }

    return(iRemoteStatus);
}

/*F********************************************************************************/
/*!
    \Function   VoipConnStatus

    \Description
        Return information about peer connection.

    \Input *pVoip       - module state from VoipStartup
    \Input  iConnID     - which connection to get remote info for, or VOIP_CONNID_ALL

    \Output
        int32_t         - VOIP_CONN* flags, or VOIP_FLAG_INVALID if iConnID is invalid

    \Version 05/06/2014 (amakoukji)
*/
/********************************************************************************F*/
int32_t VoipConnStatus(VoipRefT *pVoip, int32_t iConnID)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    int32_t iRemoteStatus = VOIP_FLAG_INVALID;

    if (pVoipCommon != NULL && pVoipCommon->Connectionlist.pConnections != NULL)
    {
        if (iConnID == VOIP_CONNID_ALL)
        {
            for (iConnID = 0, iRemoteStatus = 0; iConnID < pVoipCommon->Connectionlist.iMaxConnections; iConnID++)
            {
                iRemoteStatus |= (int32_t)pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteConnStatus;
            }
        }
        else if ((iConnID >= 0) && (iConnID < pVoipCommon->Connectionlist.iMaxConnections))
        {
            iRemoteStatus = pVoipCommon->Connectionlist.pConnections[iConnID].uRemoteConnStatus;
        }
    }

    return(iRemoteStatus);
}

/*F********************************************************************************/
/*!
    \Function   VoipLocalUserStatus

    \Description
        Return information about local hardware state

    \Input *pVoip           - module state from VoipStartup
    \Input  iLocalUserIndex - local index of the user
    
    \Output
        int32_t         - VOIP_LOCAL* flags
        
    \Version 1.0 03/02/2004 (jbrookes) First Version
    \Version 2.0 04/25/2014 (amakoukji) Refactored for MLU
*/
/********************************************************************************F*/
int32_t VoipLocalUserStatus(VoipRefT *pVoip, int32_t iLocalUserIndex)
{
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    uint32_t uLocalStatus = pVoipCommon->Connectionlist.uLocalUserStatus[iLocalUserIndex];
    
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
    #if defined(DIRTYCODE_PC)
    VoipCommonRefT *pVoipCommon = (VoipCommonRefT *)pVoip;
    VoipHeadsetSpkrCallback(pVoipCommon->pHeadset, pCallback, pUserData);
    #endif
}

/*F********************************************************************************/
/*!
    \Function   VoipSelectChannel

    \Description
        Select the mode(send/recv) of a given channel.

    \Input *pVoip       - voip module state
    \Input iUserIndex   - local user index
    \Input iChannel     - Channel ID (valid range: [0,63])
    \Input eMode        - The mode, combination of VOIP_CHANSEND, VOIP_CHANRECV

    \Output
        int32_t         - number of channels remaining that this console could join

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipSelectChannel(VoipRefT *pVoip, int32_t iUserIndex, int32_t iChannel, VoipChanModeE eMode)
{
    return(VoipCommonSelectChannel((VoipCommonRefT*)pVoip, iUserIndex, iChannel, eMode));
}

/*F********************************************************************************/
/*!
    \Function   VoipResetChannels

    \Description
        Resets the channels selection to defaults. Sends and receives to all

    \Input *pVoip           - module ref
    \Input iUserIndex       - local user index

    \Version 12/07/2009 (jrainy)
*/
/********************************************************************************F*/
void VoipResetChannels(VoipRefT *pVoip, int32_t iUserIndex)
{
    VoipCommonResetChannels((VoipCommonRefT*)pVoip, iUserIndex);
}
