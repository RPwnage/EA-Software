/*H********************************************************************************/
/*!
    \File voipstub.c

    \Description
        Voip library interface.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 0.5 03/02/2004 (jbrookes)  Implemented stubbed interface.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "platform.h"
#include "dirtylib.h"
#include "dirtymem.h"
#include "voip.h"
#include "voipcodec.h"

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP module state
struct VoipRefT
{
    //! module memory group
    int32_t         iMemGroup;
    void            *pMemGroupUserData;
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private Variables

//! pointer to module state
static VoipRefT             *_Voip_pRef = NULL;

// Public Variables


/*** Private Functions ************************************************************/


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   VoipStartup

    \Description
        Prepare VoIP for use.

    \Input iMaxPeers    - maximum number of peers supported (up to 32)
    \Input iMaxLocal    - maximum number of local users supported

    \Output
        VoipRefT        - state reference that is passed to all other functions

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipStartup(int32_t iMaxPeers, int32_t iMaxLocal)
{
    VoipRefT *pVoip;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure we're not already started
    if (_Voip_pRef != NULL)
    {
        NetPrintf(("voip: module startup called when not in a shutdown state\n"));
        return(NULL);
    }

    // create and initialize module state
    if ((pVoip = (VoipRefT *)DirtyMemAlloc(sizeof(*pVoip), VOIP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voip: unable to allocate module state\n"));
        return(NULL);
    }
    pVoip->iMemGroup = iMemGroup;
    pVoip->pMemGroupUserData = pMemGroupUserData;

    // save ref and return to caller
    _Voip_pRef = pVoip;
    return(pVoip);
}

/*F********************************************************************************/
/*!
    \Function   VoipGetRef

    \Description
        Return current module reference.

    \Input None

    \Output
        VoipRefT *      - reference pointer, or NULL if the module is not active

    \Version 1.0 03/08/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
VoipRefT *VoipGetRef(void)
{
    // return pointer to module state
    return(_Voip_pRef);
}

/*F********************************************************************************/
/*!
    \Function   VoipShutdown

    \Description
        Release all VoIP resources -- DEPRECATE in V9

    \Input  *pVoip      - module state from VoipStartup

    \Output
        None.

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
    if (_Voip_pRef == NULL)
    {
        NetPrintf(("voip: module shutdown called when not in a started state\n"));
        return;
    }

    // free module memory
    DirtyMemFree(pVoip, VOIP_MEMID, pVoip->iMemGroup, pVoip->pMemGroupUserData);

    // clear pointer to module state
    _Voip_pRef = NULL;
}


/*F********************************************************************************/
/*!
    \Function   VoipSetLocalUser

    \Description
        Set local user ID.

    \Input *pVoip       - module state from VoipStartup
    \Input *pUserID     - local user ID
    \Input bRegister    - TRUE to register user, FALSE to unregister them

    \Output
        None.

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
}

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

    \Version 02/10/2006 (jbrookes)
*/
/*************************************************************************************************F*/
void VoipSetEventCallback(VoipRefT *pVoip, VoipCallbackT *pCallback, void *pUserData)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipConnect

    \Description
        Connect to a peer.

    \Input *pVoip       - module state from VoipStartup
    \Input iConnID      - [zero, iMaxPeers-1] for an explicit slot or VOIP_CONNID_NONE to auto-allocate
    \Input uAddress     - remote peer address
    \Input uManglePort  - port from demangler (should be identical to game port on Xbox)
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
    // return connection ID to caller
    return(0);
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
}

/*F********************************************************************************/
/*!
    \Function   VoipRemove

    \Description
        Remove connection from connectionlist (do not contract connectionlist).

    \Input  *pVoip      - module state from VoipStartup
    \Input  iConnID     - which connection to remove from list

    \Version 1.0 06/14/2008 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipRemove(VoipRefT *pVoip, int32_t iConnID)
{
}

/*F********************************************************************************/
/*!
    \Function   VoipRemote

    \Description
        Return information about remote peer.

    \Input  *pVoip      - module state from VoipStartup
    \Input  iConnID     - which connection to get remote info for

    \Output
        int32_t             - VOIP_REMOTE_* flags, or VOIP_FLAG_INVALID if iConnID is invalid

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t VoipRemote(VoipRefT *pVoip, int32_t iConnID)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipLocal

    \Description
        Return information about local hardware state

    \Input  *pVoip      - module state from VoipStartup

    \Output
        int32_t             - VOIP_LOCAL_* flags

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t VoipLocal(VoipRefT *pVoip)
{
    return(0);
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

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipMicrophone(VoipRefT *pVoip, uint32_t uConnMask)
{
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

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipSpeaker(VoipRefT *pVoip, uint32_t uConnMask)
{
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

    \Version 1.0 03/02/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipStatus(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'avlb')
    {
        return(TRUE);
    }
    if (iSelect == 'from')
    {
        return(0);
    }
    if (iSelect == 'micr')
    {
        return(0);
    }
    if (iSelect == 'sock')
    {
        return(0);
    }
    if (iSelect == 'spkr')
    {
        return(0);
    }
    // unsupported selector
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function   VoipControl

    \Description
        Set control options.

    \Input *pVoip       - module state from VoipStartup
    \Input iSelect      - status selector
    \Input iValue       - selector-specific
    \Input *pValue      - selector-specific

    \Output
        int32_t         - selector-specific data

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t VoipControl(VoipRefT *pVoip, int32_t iSelect, int32_t iValue, void *pValue)
{
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function   VoipGetProfileData

    \Description
        Returns current profile data.

    \Input  eProfStat   - type of profile data to get

    \Output
        int32_t             - profile data

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

    \Input  None.

    \Output
        int32_t             - amount of time that elapsed during previous profiling cycle

    \Version 1.0 03/02/2004 (jbrookes) First Version
*/
/*************************************************************************************************F*/
int32_t VoipGetProfileTime(VoipRefT *pVoip)
{
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function VoipSpkrCallback

    \Description
        Set speaker data callback.

    \Input *pVoip       - voip module state
    \Input *pCallback   - data callback
    \Input *pUserData   - user data

    \Output
        None.

    \Version 1.0 12/12/2005 (jbrookes) First Version
*/
/*************************************************************************************************F*/
void VoipSpkrCallback(VoipRefT *pVoip, VoipSpkrCallbackT *pCallback, void *pUserData)
{
}

/*F*************************************************************************************************/
/*!
    \Function VoipCodecControl

    \Description
        Stub for codec control

    \Input iCodecIdent - codec identifier, or VOIP_CODEC_ACTIVE
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Version 1.0 10/11/2011 (jbrookes) First Version
*/
/*************************************************************************************************F*/
int32_t VoipCodecControl(int32_t iCodecIdent, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    return(0);
}


