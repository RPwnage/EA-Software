/*H********************************************************************************/
/*!
    \File voipgroup.c

    \Description
        A module that handles logical grouping of voip connections. Each module that wants to deal
        with a block of voip connections uses a voipgroup. This reduces the singleton nature
        of voip.h

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 12/18/07 (jrainy)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

// dirtysock includes
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

// voip includes
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"

/*** Defines **********************************************************************/

#define MAX_GROUPS                   (8)
#define MAX_CLIENTS_PER_GROUP       (32)

#define MAX_REF_PER_LOW_LEVEL_CONNS  (8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! high-level connection state
typedef enum VoipGroupConnStateE
{
    VOIPGROUP_CONNSTATE_ACTIVE,     //<! connection is active
    VOIPGROUP_CONNSTATE_SUSPENDED   //<! connection was suspended with VoipGroupSuspend(), can be resumed with VoipGroupResume()
} VoipGroupConnStateE;

//! high-level connection maintained by the VoipGroup module
typedef struct VoipGroupConnT
{
    VoipGroupConnStateE eConnState;     //<! state of this high-level connection
    int32_t  iMappedLowLevelConnId;     //<! id of low-level voip connection to which this high-level connection is mapped
    uint32_t uClientId;                 //<! client id associated with this high-level connection
    uint32_t uLowLevelConnectivityId;   //<! low client id associated with this high-level connection
    uint32_t uSessionId;                //<! uSessionId associated with this high-level connection
    uint8_t  bIsConnectivityHosted;     //<! connectivity hosted flag
    uint8_t  pad[3];                    //<! padding
} VoipGroupConnT;

//! VoIP module state
struct VoipGroupRefT
{
    VoipGroupConnT  aConnections[MAX_CLIENTS_PER_GROUP];
    uint32_t        aLowLevelConnectivityId[MAX_CLIENTS_PER_GROUP]; //<! low level  local connectivity id
    int32_t         iCcMode;                                //! Connection Concierge mode (VOIPGROUP_CCMODE_*)
    uint8_t         aParticipatingUser[VOIP_MAXLOCALUSERS]; //<! TRUE when this VoipGroup has activated a local user

    //! connection sharing
    ConnSharingCallbackT* pConnSharingCallback;
    void*           pConnSharingCallbackData;

    //! event callback
    VoipCallbackT*  pGroupCallback;
    void*           pGroupCallbackData;

    //! received transcribed text callback
    VoipTranscribedTextReceivedCallbackT *pTranscribedTextRecvCb; //!< inbound transcribed text event callback (optional)
    void                *pTranscribedTextUserData; //!< inbound transcribed text event callback (optional)

    //! low-level voip connection sharing
    uint8_t         bServer;        //<! TRUE when voipgroup needs voip connections via voipserver, FALSE otherwise
    uint8_t         bTunnel;        //<! TRUE when voipgroup needs voip connections over dirtysock tunnel, FALSE otherwise

    uint8_t         bUsed;          //<! whether a group is used
    uint8_t         _pad;           //<! padding
};

typedef struct VoipGroupManagerT
{
    VoipGroupRefT   aGroups[MAX_GROUPS];
    uint8_t         uGroupsAvailable;
    int32_t         aActiveRefCount[VOIP_MAXLOCALUSERS];

    uint32_t        uUserRecvMask;  //<! the recv mask associated with the user mute requests
    uint32_t        uUserSendMask;  //<! the send mask associated with the user mute requests

    //! tracking of different voipgroups using the same low-level voip connection
    VoipGroupRefT* aLowLevelConnReferences[VOIP_MAX_LOW_LEVEL_CONNS][MAX_REF_PER_LOW_LEVEL_CONNS];

    //! module memory group
    int32_t         iMemGroup;
    void            *pMemGroupUserData;

    //! TTS and STT 
    int32_t         iSTTRecvCbCount;

    //! crit section to protect the mute flags
    NetCritT        crit;
} VoipGroupManagerT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static VoipGroupManagerT *_VoipGroupManager_pRef = NULL;

/*** Private Functions ************************************************************/

//declared before since it is used by VoipGroupManager
static int32_t _VoipGroupMatch(VoipGroupRefT *pVoipGroup, int32_t iConnId);


/*F********************************************************************************/
/*!
    \Function   _VoipGroupManagerEventCallback

    \Description
        callback registered with low-level Voip module

    \Input *pVoip           - pointer to the VoipRef
    \Input eCbType          - the callback type
    \Input iValue           - the callback value
    \Input *pUserData       - pointer to the VoipGroupManager

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
static void _VoipGroupManagerEventCallback(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData)
{
    VoipGroupManagerT *pManager = (VoipGroupManagerT *)pUserData;

    int32_t iConnId, iGroup;
    int32_t iNewValue, iMappedConnId;

    if (eCbType == VOIP_CBTYPE_AMBREVENT)
    {
        // we need to update the send and recv mask
        NetCritEnter(&pManager->crit);
        pManager->uUserRecvMask = VoipStatus(pVoip, 'uspk', 0, NULL, 0);
        pManager->uUserSendMask = VoipStatus(pVoip, 'umic', 0, NULL, 0);
        NetCritLeave(&pManager->crit);

        return;
    }

    for(iGroup = 0; iGroup < MAX_GROUPS; iGroup++)
    {
        if ((pManager->aGroups[iGroup].bUsed) && (pManager->aGroups[iGroup].pGroupCallback))
        {
            if (eCbType == VOIP_CBTYPE_FROMEVENT)
            {
                iNewValue = 0;
                for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
                {
                    iMappedConnId = _VoipGroupMatch(&pManager->aGroups[iGroup], iConnId);
                    if (iMappedConnId != VOIP_CONNID_NONE)
                    {
                        // lookup bit indexed by mapped value, and set the bit before mapping.
                        iNewValue |= (((iValue & (1 << iMappedConnId)) ? 1 : 0) << iConnId);
                    }
                }
            }
            else
            {
                iNewValue = iValue;
            }

            pManager->aGroups[iGroup].pGroupCallback(pVoip, eCbType, iNewValue, pManager->aGroups[iGroup].pGroupCallbackData);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupManagerTranscribedTextReceivedCallback

    \Description
        callback registered with low-level Voip module

    \Input iConnId          - connection index
    \Input iRemoteUserIndex - index of remote user from which the transcribed text is originated from
    \Input *pText           - received transcribed text
    \Input *pUserData       - pointer to the VoipGroupManager

    \Version 05/03/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipGroupManagerTranscribedTextReceivedCallback(int32_t iConnId, int32_t iRemoteUserIndex, const wchar_t *pText, void *pUserData)
{
    int32_t iGroup, iMappedConnId;
    VoipGroupManagerT *pManager = (VoipGroupManagerT *)pUserData;

    for (iGroup = 0; iGroup < MAX_GROUPS; iGroup++)
    {
        if ((pManager->aGroups[iGroup].bUsed) && (pManager->aGroups[iGroup].pTranscribedTextRecvCb))
        {
            iMappedConnId = _VoipGroupMatch(&pManager->aGroups[iGroup], iConnId);

            if (iMappedConnId != VOIP_CONNID_NONE)
            {
                pManager->aGroups[iGroup].pTranscribedTextRecvCb(iMappedConnId, iRemoteUserIndex, (const wchar_t *)pText, pManager->aGroups[iGroup].pTranscribedTextUserData);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupManagerGet

    \Description
        Returns the single VoipGroupManager.

    \Output
        VoipGroupManagerT* - The VoipGroup Manager

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
static VoipGroupManagerT *_VoipGroupManagerGet(void)
{
    if (_VoipGroupManager_pRef == NULL)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;

        // Query current mem group data
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

        _VoipGroupManager_pRef = DirtyMemAlloc(sizeof(*_VoipGroupManager_pRef), VOIP_MEMID, iMemGroup, pMemGroupUserData);
        if (_VoipGroupManager_pRef == NULL)
        {
            NetPrintf(("voipgroup: could not allocate module state\n"));
            return(NULL);
        }
        ds_memclr(_VoipGroupManager_pRef, sizeof(*_VoipGroupManager_pRef));
        
        NetCritInit(&_VoipGroupManager_pRef->crit, "voipgroup manager crit");
        _VoipGroupManager_pRef->uGroupsAvailable = MAX_GROUPS;
        _VoipGroupManager_pRef->iMemGroup = iMemGroup;
        _VoipGroupManager_pRef->pMemGroupUserData = pMemGroupUserData;

        // register callback with low-level VOIP module if it exists
        if (VoipGetRef() != NULL)
        {
            VoipSetEventCallback(VoipGetRef(), _VoipGroupManagerEventCallback, _VoipGroupManager_pRef);
        }
    }

    return(_VoipGroupManager_pRef);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupFindFreeHighLevelConn

    \Description
        Find a free high level connection spot in the given voipgroup.

    \Input *pVoipGroup  - the voipgroup to inspect for a free connection spot

    \Output
        int32_t         - failure: VOIP_CONNID_NONE, success: conn id of high-level conn found

    \Version 10/04/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipGroupFindFreeHighLevelConn(VoipGroupRefT *pVoipGroup)
{
    int32_t iConnId;

    for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
    {
        if (pVoipGroup->aConnections[iConnId].uClientId == 0)
        {
            break;
        }
    }

    if (iConnId == MAX_CLIENTS_PER_GROUP)
    {
        NetPrintf(("voipgroup: [0x%08x] warning - voipgroup is full\n", pVoipGroup));
        return(VOIP_CONNID_NONE);
    }

    return(iConnId);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupFindUsedHighLevelConn

    \Description
        In the given voipgroiup, find the high level connection associated with
        specified client id.

    \Input *pVoipGroup  - the voipgroup to inspect for a free connection spot
    \Input uClientId    - client id to look for

    \Output
        int32_t         - failure: VOIP_CONNID_NONE, success: conn id of high-level conn found

    \Version 10/04/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipGroupFindUsedHighLevelConn(VoipGroupRefT *pVoipGroup, uint32_t uClientId)
{
    int32_t iConnId;

    for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
    {
        if (pVoipGroup->aConnections[iConnId].uClientId == uClientId)
        {
            break;
        }
    }

    if (iConnId == MAX_CLIENTS_PER_GROUP)
    {
        return(VOIP_CONNID_NONE);
    }

    return(iConnId);
}


/*F********************************************************************************/
/*!
    \Function   _VoipGroupLowLevelConnInUse

    \Description
        Tests whether a voip-level ConnId is in use.

    \Input iConnId  - voip-level ConnId 

    \Output
        uint8_t     - TRUE or FALSE

    \Version 12/02/2009 (jrainy)
*/
/********************************************************************************F*/
static uint8_t _VoipGroupLowLevelConnInUse(int32_t iConnId)
{
    return(_VoipGroupManagerGet()->aLowLevelConnReferences[iConnId][0] != NULL);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupManagerGetGroup

    \Description
        Allocates a new VoipGroup for the given manager

    \Input *pVoipGroupManager  - The manager to allocate the group from

    \Output
        VoipGroupRefT*  - The allocated VoipGroup

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
static VoipGroupRefT* _VoipGroupManagerGetGroup(VoipGroupManagerT *pVoipGroupManager)
{
    int32_t iIndex;

    if (pVoipGroupManager->uGroupsAvailable)
    {
        for(iIndex = 0; iIndex < MAX_GROUPS; iIndex++)
        {
            if (!pVoipGroupManager->aGroups[iIndex].bUsed)
            {
                return(&pVoipGroupManager->aGroups[iIndex]);
            }
        }
    }

    NetPrintf(("voipgroup: no voipgroup available\n"));
    return(NULL);
}

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function   _VoipGroupPrintRefsToLowLevelConn

    \Description
        Prints all references to a given low-level voip connection

    \Input iLowLevelConnId  - id of low-level connection

    \Version 11/12/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipGroupPrintRefsToLowLevelConn(int32_t iLowLevelConnId)
{
    int32_t iRefIndex;
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

    if (_VoipGroupLowLevelConnInUse(iLowLevelConnId))
    {
        NetPrintf(("voipgroup: references to low-level voip conn %d are: ", iLowLevelConnId));
        for (iRefIndex = 0; iRefIndex < MAX_REF_PER_LOW_LEVEL_CONNS; iRefIndex++)
        {
            if (pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex] != NULL)
            {
                NetPrintf(("0x%08x  ", pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex]));
            }
            else
            {
                break;
            }
        }
        NetPrintf(("\n"));
    }
    else
    {
        NetPrintf(("voipgroup: there is no reference to low-level voip conn %d\n", iLowLevelConnId));
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function   _VoipGroupManagerShutdown

    \Description
        free the singleton VoipGroupManager.

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
static void _VoipGroupManagerShutdown(void)
{
    if (_VoipGroupManager_pRef != NULL)
    {
        // remove callback registered with low-level VOIP module
        if (VoipGetRef() != NULL)
        {
            VoipSetEventCallback(VoipGetRef(), NULL, NULL);
        }

        NetCritKill(&_VoipGroupManager_pRef->crit);
        DirtyMemFree(_VoipGroupManager_pRef, VOIP_MEMID, _VoipGroupManager_pRef->iMemGroup, _VoipGroupManager_pRef->pMemGroupUserData);
        _VoipGroupManager_pRef = NULL;

    }
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupManagerReleaseGroup

    \Description
        Frees a VoipGroup for the given manager

    \Input *pManager    - The manager to allocate the group from
    \Input *pVoipGroup  - The VoipGroup to free

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
static void _VoipGroupManagerReleaseGroup(VoipGroupManagerT *pManager, VoipGroupRefT* pVoipGroup)
{
    int32_t iSlot;
    int32_t iUser;

    NetPrintf(("voipgroup: [0x%08x] remove all connections and clear all mappings\n", pVoipGroup));

    // remove all connections
    for(iSlot = 0; iSlot < MAX_CLIENTS_PER_GROUP; iSlot++)
    {
        if (pVoipGroup->aConnections[iSlot].uClientId != 0)
        {
            VoipGroupDisconnect(pVoipGroup, iSlot);
        }
    }

    // deactivate active refs
    for(iUser = 0; iUser < VOIP_MAXLOCALUSERS; iUser++)
    {
        if (pVoipGroup->aParticipatingUser[iUser] == TRUE)
        {
            VoipGroupActivateLocalUser(pVoipGroup, iUser, FALSE);
        }
    }

    // clear voipgroup entry
    ds_memclr(pVoipGroup, sizeof(VoipGroupRefT));

    pManager->uGroupsAvailable++;
    if (pManager->uGroupsAvailable == MAX_GROUPS)
    {
        _VoipGroupManagerShutdown();
    }

    return;
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupMatch

    \Description
        Converts between VoipGroup ID and Voip ID.

    \Input *pVoipGroup      - The VoipGroup the iConnId indexes in
    \Input iConnId          - The VoipGroup ID

    \Output
        int32_t             - The corresponding Voip ID

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
static int32_t _VoipGroupMatch(VoipGroupRefT *pVoipGroup, int32_t iConnId)
{
    int32_t iIndex, iConn, iRetVal = VOIP_CONNID_NONE;
    uint32_t uClientId;
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

    if ((iConnId < 0) || (iConnId >= MAX_CLIENTS_PER_GROUP))
    {
        return(VOIP_CONNID_NONE);
    }

    uClientId = pVoipGroup->aConnections[iConnId].uClientId;
    if (uClientId == 0)
    {
        return(VOIP_CONNID_NONE);
    }

    // if this connection is suspended look for the active group 
    // for this iConnId to get the proper low level conn id
    if (pVoipGroup->aConnections[iConnId].eConnState == VOIPGROUP_CONNSTATE_SUSPENDED)
    {
        for(iIndex = 0; iIndex < MAX_GROUPS; iIndex++)
        {
            for(iConn = 0; iConn < MAX_CLIENTS_PER_GROUP; iConn++)
            {
                if ( (pManager->aGroups[iIndex].aConnections[iConn].uClientId == uClientId) &&
                     (pManager->aGroups[iIndex].aConnections[iConn].eConnState == VOIPGROUP_CONNSTATE_ACTIVE) )
                {
                    iRetVal = pManager->aGroups[iIndex].aConnections[iConn].iMappedLowLevelConnId;
                    break;
                }
            }
        }
    }
    else
    {
        iRetVal = pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId;
    }

    return(iRetVal);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupFindCollision

    \Description
        Looks for another group with a low-level connection to a given client id

    \Input *pVoipGroup          - pointer to voipgroup looking for a colliding voipgroup
    \Input uClientId            - the client id to match
    \Input eConnState           - connection state to match
    \Input *pCollidingConnId    - output parameter to be filled with id of colliding connection if applicable (invalid if return value is NULL)

    \Output
        VoipGroupRefT*          - group reference if found, NULL if no collision

    \Version 011/11/2009 (mclouatre)
*/
/********************************************************************************F*/
static VoipGroupRefT* _VoipGroupFindCollision(VoipGroupRefT *pVoipGroup, uint32_t uClientId, VoipGroupConnStateE eConnState, int32_t *pCollidingConnId)
{
    int32_t iIndex, iConnId;
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

    for(iIndex = 0; iIndex < MAX_GROUPS; iIndex++)
    {
        for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
        {
            if ( (pManager->aGroups[iIndex].aConnections[iConnId].uClientId == uClientId) &&
                 (pManager->aGroups[iIndex].aConnections[iConnId].eConnState == eConnState) )
            {
                // do not allow a voipgroup to collide with itself
                if (&pManager->aGroups[iIndex] == pVoipGroup)
                {
                    continue;
                }

                NetPrintf(("voipgroup: [0x%08x] collision found with voipgroup 0x%08x for client id 0x%08x\n", pVoipGroup, &pManager->aGroups[iIndex], uClientId));
                *pCollidingConnId = iConnId;
                return(&pManager->aGroups[iIndex]);
            }
        }
    }

    *pCollidingConnId = VOIP_CONNID_NONE;
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupAddRefToLowLevelConn

    \Description
        Add a reference to a low level connection

    \Input iLowLevelConnId  - low level conn id
    \Input *pVoipGroup      - voipgroup to be added as a reference to the low-level connection

    \Output
        int32_t             - 0 for success, -1 for failure

    \Version 11/12/2009 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipGroupAddRefToLowLevelConn(int32_t iLowLevelConnId, VoipGroupRefT * pVoipGroup)
{
    int32_t iRefIndex;
    int32_t iRetCode = 0; // default to succes
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

    for(iRefIndex = 0; iRefIndex < MAX_REF_PER_LOW_LEVEL_CONNS; iRefIndex++)
    {
        // if this ref already is in the table, fake a failure
        if (pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex] == pVoipGroup)
        {
            iRetCode = -1;
            NetPrintf(("voipgroup: [0x%08x] error - current voipgroup already exists as a reference for low-level voip conn %d\n", pVoipGroup, iLowLevelConnId));
            break;
        }

        // look for a free spot to find our ref
        if (pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex] == NULL)
        {
            pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex] = pVoipGroup;
            NetPrintf(("voipgroup: [0x%08x] added current voipgroup as a reference for low-level voip conn %d\n", pVoipGroup, iLowLevelConnId));
            break;
        }
    }

    if (iRefIndex == MAX_REF_PER_LOW_LEVEL_CONNS)
    {
        iRetCode = -1;

        NetPrintf(("voipgroup: [0x%08x] critical error - cannot add current voipgroup as a reference for low-level conn %d, max number of references exceeded\n",
            pVoipGroup, iLowLevelConnId));
    }

    #if DIRTYCODE_LOGGING
    _VoipGroupPrintRefsToLowLevelConn(iLowLevelConnId);
    #endif

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function   _VoipGroupRemoveRefToLowLevelConn

    \Description
        Remove a reference to a low level connection

    \Input iLowLevelConnId   - low level conn id
    \Input *pVoipGroup       - voipgroup to be added as a reference to the low-level connection

    \Version 11/12/2009 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipGroupRemoveRefToLowLevelConn(int32_t iLowLevelConnId, VoipGroupRefT * pVoipGroup)
{
    int32_t iRefIndex;
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

    for(iRefIndex = 0; iRefIndex < MAX_REF_PER_LOW_LEVEL_CONNS; iRefIndex++)
    {
        if (pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex] == pVoipGroup)
        {
            int32_t iRefIndex2;

            // move all following references one cell backward in the array
            for(iRefIndex2 = iRefIndex; iRefIndex2 < MAX_REF_PER_LOW_LEVEL_CONNS; iRefIndex2++)
            {
                if (iRefIndex2 == MAX_REF_PER_LOW_LEVEL_CONNS-1)
                {
                    // last entry, reset to NULL
                    pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex2] = NULL;
                }
                else
                {
                    pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex2] = pManager->aLowLevelConnReferences[iLowLevelConnId][iRefIndex2+1];
                 }
            }

            NetPrintf(("voipgroup: [0x%08x] current voipgroup no more referencing low-level voip conn %d\n", pVoipGroup, iLowLevelConnId));
            #if DIRTYCODE_LOGGING
            _VoipGroupPrintRefsToLowLevelConn(iLowLevelConnId);
            #endif

            break;
        }
    }

    if (iRefIndex == MAX_REF_PER_LOW_LEVEL_CONNS)
    {
        NetPrintf(("voipgroup: [0x%08x] warning - current voipgroup not found as a valid reference for low-level conn %d\n", pVoipGroup, iLowLevelConnId));
    }
}

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function   VoipGroupCreate

    \Description
        allocates a VoipGroup

    \Output
        VoipGroupRefT* - Allocated VoipGroup

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
VoipGroupRefT* VoipGroupCreate(void)
{
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();
    VoipGroupRefT *newlyCreated;

    if ((newlyCreated = _VoipGroupManagerGetGroup(pManager)) != NULL)
    {
        pManager->uGroupsAvailable--;
        newlyCreated->bUsed = TRUE;
    }

    return(newlyCreated);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupDestroy

    \Description
        Deallocates a VoipGroup

    \Input *pVoipGroup  - VoipGroup to Deallocate

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
void VoipGroupDestroy(VoipGroupRefT *pVoipGroup)
{
    _VoipGroupManagerReleaseGroup(_VoipGroupManagerGet(), pVoipGroup);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupSetEventCallback

    \Description
        Sets the callback and userdata for the specified group

    \Input *pVoipGroup  - VoipGroup to use
    \Input *pCallback   - pointer to the callback to use
    \Input *pUserData   - pointer to the user specified data

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
void VoipGroupSetEventCallback(VoipGroupRefT *pVoipGroup, VoipCallbackT *pCallback, void *pUserData)
{
    pVoipGroup->pGroupCallback = pCallback;
    pVoipGroup->pGroupCallbackData = pUserData;

    VoipSetEventCallback(VoipGetRef(), _VoipGroupManagerEventCallback, _VoipGroupManagerGet());
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupSetTranscribedTextReceivedCallback

    \Description
        Sets the callback and userdata for the specified group

    \Input *pVoipGroup  - VoipGroup to use
    \Input *pCallback   - pointer to the callback to use
    \Input *pUserData   - pointer to the user specified data

    \Version 05/03/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipGroupSetTranscribedTextReceivedCallback(VoipGroupRefT *pVoipGroup, VoipTranscribedTextReceivedCallbackT *pCallback, void *pUserData)
{
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

    NetCritEnter(&pManager->crit);

    // track the number of voipgroups using the Voip Transcribed Text Received Callback
    if ((pCallback != NULL) && (pVoipGroup->pTranscribedTextRecvCb == NULL))
    {
        ++(pManager->iSTTRecvCbCount);
    }
    else if ((pCallback == NULL) && (pVoipGroup->pTranscribedTextRecvCb != NULL) && (pManager->iSTTRecvCbCount > 0))
    {
        --(pManager->iSTTRecvCbCount);
    }

    pVoipGroup->pTranscribedTextRecvCb = pCallback;
    pVoipGroup->pTranscribedTextUserData = pUserData;

    // if at least one voipgroup is using the callback set it at the voip level as well, otherwise set it to NULL
    if (pManager->iSTTRecvCbCount <= 0)
    {
        VoipSetTranscribedTextReceivedCallback(VoipGetRef(), NULL, _VoipGroupManagerGet());
    }
    else if (pManager->iSTTRecvCbCount == 1)
    {
        VoipSetTranscribedTextReceivedCallback(VoipGetRef(), _VoipGroupManagerTranscribedTextReceivedCallback, _VoipGroupManagerGet());
    }
    NetCritLeave(&pManager->crit);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupSetConnSharingEventCallback

    \Description
        Sets the connection sharing callback and userdata for the specified group

    \Input *pVoipGroup     - voipgroup ref
    \Input *pCallback      - pointer to the callback to use
    \Input *pUserData      - pointer to the user specified data

    \Notes
        The callback registered with this function must invoke VoipGroupSuspend()
        when eCbType=VOIPGROUP_CBTYPE_CONNSUSPEND, and it must call VoipGroupResume()
        when eCbType=VOIPGROUP_CBTYPE_CONNRESUME.

    \Version 11/11/2009 (mclouatre)
*/
/********************************************************************************F*/
void VoipGroupSetConnSharingEventCallback(VoipGroupRefT *pVoipGroup, ConnSharingCallbackT *pCallback, void *pUserData)
{
    pVoipGroup->pConnSharingCallback = pCallback;
    pVoipGroup->pConnSharingCallbackData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupConnect

    \Description
        Set up a connection, using VoipGroup

    \Input *pVoipGroup - The VoipGroup the flags apply to
    \Input iConnId     - connection index
    \Input uAddress    - address to connect to
    \Input uManglePort - port to connect on
    \Input uGamePort   - port to connect on
    \Input uClientId   - client identifier
    \Input uSessionId  - session identifier (optional)

    \Output
        int32_t        - the VoipGroup connid the connection was made with

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipGroupConnect(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId)
{
    return(VoipGroupConnect2(pVoipGroup, iConnId, uAddress, uManglePort, uGamePort, uClientId, uSessionId, FALSE, uClientId));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupConnect2

    \Description
        Set up a connection, using VoipGroup

    \Input *pVoipGroup - The VoipGroup the flags apply to
    \Input iConnId     - connection index
    \Input uAddress    - address to connect to
    \Input uManglePort - port to connect on
    \Input uGamePort   - port to connect on
    \Input uClientId   - client identifier
    \Input uSessionId  - session identifier (optional)
    \Input bIsConnectivityHosted - is the connection through a relay server
    \Input uLowLevelConnectivityId - low level remote client id

    \Output
        int32_t        - the VoipGroup connid the connection was made with

    \Version 11/03/2015 (amakoukji)
*/
/********************************************************************************F*/
int32_t VoipGroupConnect2(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId, uint8_t bIsConnectivityHosted, uint32_t uLowLevelConnectivityId)
{
    int32_t iMappedLowLevelConnId = VOIP_CONNID_NONE;
    int32_t iHighLevelConnIdInUse = VOIP_CONNID_NONE;
    int32_t iCollidingConnId;
    VoipGroupRefT *pCollidingVoipGroup;
    uint8_t bMuted = FALSE; 

    NetPrintf(("voipgroup: [0x%08x] connecting high-level conn %d (sess id 0x%08x)\n", pVoipGroup, iConnId, uSessionId));

    // is the specified client id already known by the current voipgroup?
    // note: typically this occurs when the caller perform successive calls to VoipGroupConnect() while
    //       trying different port obtained from the demangler.
    if ((iHighLevelConnIdInUse = _VoipGroupFindUsedHighLevelConn(pVoipGroup, uClientId)) != VOIP_CONNID_NONE)
    {
        if (iConnId == VOIP_CONNID_NONE)
        {
            iConnId = iHighLevelConnIdInUse;
        }

        if (iHighLevelConnIdInUse == iConnId)
        {
            NetPrintf(("voipgroup: [0x%08x] current voipgroup already knows about client id 0x%08x (low level id 0x%08x) with high-level conn id %d\n", 
                pVoipGroup, uClientId, uLowLevelConnectivityId, iConnId));

            // if connection is "suspended", early exit faking a success
            if (pVoipGroup->aConnections[iConnId].eConnState == VOIPGROUP_CONNSTATE_SUSPENDED)
            {
                NetPrintf(("voipgroup: [0x%08x] high-level conn % (sess id 0x%08x) is suspended, assume low-level connectivity is up\n", pVoipGroup, iConnId, pVoipGroup->aConnections[iConnId].uSessionId));
                return(iConnId);
            }
        }
        else
        {
            NetPrintf(("voipgroup: [0x%08x] critical error - current voipgroup already knows about client id x%08x (low level id 0x%08x) but conn ids do not match (%d != %d)\n", 
                pVoipGroup, uClientId, uLowLevelConnectivityId, iHighLevelConnIdInUse, iConnId));
            return(-4);
        }
    }

    // if the user doesn't specify a conn id, use the first unmapped one.
    if (iConnId == VOIP_CONNID_NONE)
    {
        if ((iConnId = _VoipGroupFindFreeHighLevelConn(pVoipGroup)) == VOIP_CONNID_NONE)
        {
            NetPrintf(("voipgroup: [0x%08x] warning - voipgroup is full\n", pVoipGroup));
            return(-1);
        }
    }

    // check for collision: verify if another voipgroup deals with the same client
    if ( (pCollidingVoipGroup = _VoipGroupFindCollision(pVoipGroup, uClientId, VOIPGROUP_CONNSTATE_ACTIVE, &iCollidingConnId)) != NULL )
    {
        // if there is a collision check the mute status of the colliding voipgroup's on the connId so we can reset the status if we suspend
        bMuted = VoipGroupIsMutedByConnId(pCollidingVoipGroup, iCollidingConnId);

        /* If we are re-establishing voip from a hosted voip conn to a p2p voip conn, and if we know that the original
           hosted voip conn was resulting from a failing p2p voip conn, then we know this new targeted p2p voip conn will fail.
           Consequently, to avoid an unnecessary 10-sec p2p connection handshaking delay (which results in novoip between the users during that
           time because the original hosted voip connection gets suspended), we just return an error. The assumption being that blaze, upon 
           detecting the connection failure, will immediately resume connectivity but hosted this time. */
        if ((pCollidingVoipGroup->iCcMode == VOIPGROUP_CCMODE_HOSTEDFALLBACK) && pCollidingVoipGroup->aConnections[iCollidingConnId].bIsConnectivityHosted && !bIsConnectivityHosted)
        {
            NetPrintf(("voipgroup: [0x%08x] failing VoipGroupConnect() because we anticipate the target p2p conn to fail... we instead prefer waiting for hosted conn to hosted conn transition\n", pVoipGroup));
            return(-2);
        }

        // if both voipgroups do not share same bServer / bTunnel attributes
        // OR if the previous or current connection is connectivity hosted, 
        // then connection re-establishment kicks-in
        if ( (pCollidingVoipGroup->bServer != pVoipGroup->bServer) ||
             (pCollidingVoipGroup->bTunnel != pVoipGroup->bTunnel) ||
             bIsConnectivityHosted ||
             pCollidingVoipGroup->aConnections[iCollidingConnId].bIsConnectivityHosted)
        {
            /*
            Note:
            Voipgroups with bServer attribute set have precedence over voipgroups with bServer attribute not set,
            i.e. voip connectivity via voipserver is always preferred over p2p voip connectivity. Therefore, a newly
            created voip connection goes straight to "suspended state" if there already exists a voip connection
            via the voipserver for the same client. Or, if the newly created voip connection is to be established
            through the voipserver, then any existing p2p voip connection to the same client is first moved to 
            "suspended" state, and only then the voip connection through the voipserver is established.
            */

            // is the colliding voipgroup already using the voipserver?
            // note: bServer will never be the true for connectivity hosted connection
            if (pCollidingVoipGroup->bServer)
            {
                // no need to proceed with current connection establishment because the colliding voipgroup
                // already has voip connectivity via the voipserver, so it has precedence. just mark current
                // connection as "SUSPENDED" such that it can be resumed later if needed.
                pVoipGroup->aConnections[iConnId].eConnState = VOIPGROUP_CONNSTATE_SUSPENDED;
                pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId = VOIP_CONNID_NONE;
                pVoipGroup->aConnections[iConnId].uClientId = uClientId;
                pVoipGroup->aConnections[iConnId].uLowLevelConnectivityId = uLowLevelConnectivityId;
                pVoipGroup->aConnections[iConnId].uSessionId = uSessionId;
                pVoipGroup->aConnections[iConnId].bIsConnectivityHosted = bIsConnectivityHosted;

                NetPrintf(("voipgroup: [0x%08x] early suspension of high-level conn %d (sess id 0x%08x) - there already exists a low-level voip conn through the voipserver for client id 0x%08x (low level id 0x%08x).\n",
                    pVoipGroup, iConnId, uSessionId, uClientId, uLowLevelConnectivityId));
                return(iConnId); // fake success
            }
            else
            {

                // let the colliding voipgroup entity know that it has to suspend its connection right away
                // because the current voipgroup will be creating a new one with new connection parameters
                pCollidingVoipGroup->pConnSharingCallback(pCollidingVoipGroup, VOIPGROUP_CBTYPE_CONNSUSPEND,
                                                              iCollidingConnId, pCollidingVoipGroup->pConnSharingCallbackData, FALSE, FALSE);
            }
        }
        else
        {
            // both voipgroups can share the underlying low-level voip connection
            iMappedLowLevelConnId = pCollidingVoipGroup->aConnections[iCollidingConnId].iMappedLowLevelConnId;

            // add session ID to the set of sessions sharing the voip connection
            VoipControl(VoipGetRef(), 'ases', iMappedLowLevelConnId, &uSessionId);
        }
    }

    if (iMappedLowLevelConnId < 0)
    {
        // only call VoipConnect() with a valid conn id if any of the following two condidions is met:
        //   1- VoipConnect() was already called for that low-level connection (i.e iHighLevelConnIdInUse is valid)
        //   2- The high-level conn id is directly available in voip and there is currently no reference to it
        //      (there is indeed a small transition time during which a connection may go to DISC state and a voip goup still refers to it)
        if ( (iHighLevelConnIdInUse != VOIP_CONNID_NONE) ||
             (VoipStatus(VoipGetRef(), 'avlb', iConnId, NULL, 0) && !_VoipGroupLowLevelConnInUse(iConnId)) )
        {
            // conn id is available, use it!
            iMappedLowLevelConnId = VoipConnect(VoipGetRef(), iConnId, uAddress, uManglePort, uGamePort, uLowLevelConnectivityId, uSessionId);
        }
        else
        {
            // else, let the voip module select a low-level voip conn id for us.
            iMappedLowLevelConnId = VoipConnect(VoipGetRef(), VOIP_CONNID_NONE, uAddress, uManglePort, uGamePort, uLowLevelConnectivityId, uSessionId);
        }
    }

    if (iMappedLowLevelConnId < 0)
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupConnect() failed obtaining a valid low-level conn id\n", pVoipGroup));
        return(-3);
    }

    NetPrintf(("voipgroup: [0x%08x] high-level conn id %d (sess id 0x%08x) mapped to low-level conn id %d --> address %a, clientId=0x%08x (low level id 0x%08x).\n", 
        pVoipGroup, iConnId, uSessionId, iMappedLowLevelConnId, uAddress, uClientId, uLowLevelConnectivityId));

    // set the local client id for the mapping
    VoipControl(VoipGetRef(), 'vcid', iMappedLowLevelConnId, &pVoipGroup->aLowLevelConnectivityId[iConnId]);

    // save the mapping.
    pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId = iMappedLowLevelConnId;
    if (pVoipGroup->bServer == TRUE || bIsConnectivityHosted == TRUE)
    {
        // let the voip module know about the local-to-voipserver conn id mapping
        VoipControl(VoipGetRef(), 'conm', pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId, &iConnId);
    }

    // save other connection-specific data
    pVoipGroup->aConnections[iConnId].uClientId = uClientId;
    pVoipGroup->aConnections[iConnId].uLowLevelConnectivityId = uLowLevelConnectivityId;
    pVoipGroup->aConnections[iConnId].uSessionId = uSessionId;
    pVoipGroup->aConnections[iConnId].bIsConnectivityHosted = bIsConnectivityHosted;

    // if this is the first attempt to connect, let's just add ourself as an additional reference for this connection
    if ((iHighLevelConnIdInUse == VOIP_CONNID_NONE) && (_VoipGroupAddRefToLowLevelConn(iMappedLowLevelConnId, pVoipGroup) < 0))
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupConnect() failed to add a new reference to the low-level conn %d\n", pVoipGroup, iMappedLowLevelConnId));
        VoipGroupDisconnect(pVoipGroup, iConnId);
        return(-4);
    }

    // don't initialize the user masks if there was a collision since they'll already be set
    if (!bMuted)
    {
        _VoipGroupManagerGet()->uUserRecvMask |= (1 << iMappedLowLevelConnId);
        VoipSpeaker(VoipGetRef(), _VoipGroupManagerGet()->uUserRecvMask);
        _VoipGroupManagerGet()->uUserSendMask |= (1 << iMappedLowLevelConnId);
        VoipMicrophone(VoipGetRef(), _VoipGroupManagerGet()->uUserSendMask);
    }

    // return the high-level connection id used
    return(iConnId);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupResume

    \Description
        resume a connection

    \Input *pVoipGroup  - voipgroup ref
    \Input iConnId      - connection index
    \Input uAddress     - address to connect to
    \Input uManglePort  - port to connect on
    \Input uGamePort    - port to connect on
    \Input uClientId    - client identifier
    \Input uSessionId   - session identifier (optional)

    \Notes
        Muting logic is intentionally excluded from the resume. It is currently 
        handled by ConnApi. 

    \Version 11/11/2009 (mclouatre)
*/
/********************************************************************************F*/
void VoipGroupResume(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId)
{
    VoipGroupResume2(pVoipGroup, iConnId, uAddress, uManglePort, uGamePort, uClientId, uSessionId, FALSE);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupResume2

    \Description
        resume a connection

    \Input *pVoipGroup  - voipgroup ref
    \Input iConnId      - connection index
    \Input uAddress     - address to connect to
    \Input uManglePort  - port to connect on
    \Input uGamePort    - port to connect on
    \Input uClientId    - client identifier
    \Input uSessionId   - session identifier (optional)
    \Input bIsConnectivityHosted - TRUE is connection is connectivity hosted

    \Notes
        Muting logic is intentionally excluded from the resume. It is currently 
        handled by ConnApi. 

    \Version 01/26/2016 (amakoukji)
*/
/********************************************************************************F*/
void VoipGroupResume2(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId, uint8_t bIsConnectivityHosted)
{
    int32_t iMappedLowLevelConnId = -1;

    NetPrintf(("voipgroup: [0x%08x] resuming high-level conn %d (sess id 0x%08x)\n", pVoipGroup, iConnId, uSessionId));

    #if DIRTYCODE_LOGGING
    if (pVoipGroup->aConnections[iConnId].uClientId != uClientId)
    {
        NetPrintf(("voipgroup: [0x%08x] warning connection being resumed has inconsistent client id: 0x%08x should be 0x%08x\n",
            pVoipGroup, uClientId, pVoipGroup->aConnections[iConnId].uClientId));
    }
    if (pVoipGroup->aConnections[iConnId].uSessionId != uSessionId)
    {
        NetPrintf(("voipgroup: [0x%08x] warning connection being resumed has inconsistent session id: 0x%08x should be 0x%08x\n",
            pVoipGroup, uSessionId, pVoipGroup->aConnections[iConnId].uSessionId));
    }
    #endif

    // force connection back to active state
    pVoipGroup->aConnections[iConnId].eConnState = VOIPGROUP_CONNSTATE_ACTIVE;

    // check if the high-level conn id is directly available in voip and if there is currently no reference to it
    // (there is indeed a small transition time during which a connection may go to DISC state and a voip goup still refers to it)
    if (VoipStatus(VoipGetRef(), 'avlb', iConnId, NULL, 0) && !_VoipGroupLowLevelConnInUse(iConnId))
    {
        // conn id is available, use it!
        iMappedLowLevelConnId = VoipConnect(VoipGetRef(), iConnId, uAddress, uManglePort, uGamePort, pVoipGroup->aConnections[iConnId].uLowLevelConnectivityId, uSessionId);
    }
    else
    {
        // else, let Voip allocate one.
        iMappedLowLevelConnId = VoipConnect(VoipGetRef(), VOIP_CONNID_NONE, uAddress, uManglePort, uGamePort, pVoipGroup->aConnections[iConnId].uLowLevelConnectivityId, uSessionId);
    }

    if (iMappedLowLevelConnId < 0)
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupResume() failed obtaining a valid low-level conn id\n", pVoipGroup));
        return;
    }

    NetPrintf(("voipgroup: [0x%08x] high-level conn id %d (sess id 0x%08x) mapped to low-level conn id %d --> address %a, clientId=0x%08x (low level id 0x%08x).\n", 
        pVoipGroup, iConnId, uSessionId, iMappedLowLevelConnId, uAddress, uClientId, pVoipGroup->aConnections[iConnId].uLowLevelConnectivityId));

    // set the local client id for the mapping
    VoipControl(VoipGetRef(), 'vcid', iMappedLowLevelConnId, &pVoipGroup->aLowLevelConnectivityId[iConnId]);

    // save the mapping.
    pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId = iMappedLowLevelConnId;
    if (pVoipGroup->bServer == TRUE || bIsConnectivityHosted == TRUE)
    {
        // let the voip module know about the local-to-voipserver conn id mapping
        VoipControl(VoipGetRef(), 'conm', pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId, &iConnId);
    }

    // let's just add ourself as an additional reference for this connection
    if ((_VoipGroupAddRefToLowLevelConn(iMappedLowLevelConnId, pVoipGroup)) < 0)
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupResume() failed adding a new reference\n", pVoipGroup));
        return;
    }

    return;
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupDisconnect

    \Description
        Teardown a connection, using VoipGroups.

    \Input *pVoipGroup  - The VoipGroup the flags apply to
    \Input iConnId      - connection index

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
void VoipGroupDisconnect(VoipGroupRefT *pVoipGroup, int32_t iConnId)
{
    int32_t iCollidingConnId;
    int32_t iMappedLowLevelConnId;
    VoipGroupRefT *pCollidingVoipGroup;

    NetPrintf(("voipgroup: [0x%08x] disconnecting high-level conn %d (sess id 0x%08x)\n", pVoipGroup, iConnId, pVoipGroup->aConnections[iConnId].uSessionId));

    // if high-level connection is already suspended, skip disconnection and just clean up corresponding data in the group
    if (pVoipGroup->aConnections[iConnId].eConnState != VOIPGROUP_CONNSTATE_SUSPENDED)
    {
        iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

        if (iMappedLowLevelConnId != VOIP_CONNID_NONE)
        {
            // remove voipgroup from low-level connection's reference set
            _VoipGroupRemoveRefToLowLevelConn(iMappedLowLevelConnId, pVoipGroup);

            // only proceed if the ref count for this low-level conn is 0
            if (!_VoipGroupLowLevelConnInUse(iMappedLowLevelConnId))
            {
                uint8_t bMuted = VoipGroupIsMutedByConnId(pVoipGroup, iConnId);
                VoipDisconnect(VoipGetRef(), iMappedLowLevelConnId, 0);

                _VoipGroupManagerGet()->uUserRecvMask &= ~(1 << iMappedLowLevelConnId);
                VoipSpeaker(VoipGetRef(), _VoipGroupManagerGet()->uUserRecvMask);
                _VoipGroupManagerGet()->uUserSendMask &= ~(1 << iMappedLowLevelConnId);
                VoipMicrophone(VoipGetRef(), _VoipGroupManagerGet()->uUserSendMask);

                // scan other voipgroups and resume any suspended connection to the same client id
                if ( (pCollidingVoipGroup = _VoipGroupFindCollision(pVoipGroup, pVoipGroup->aConnections[iConnId].uClientId, VOIPGROUP_CONNSTATE_SUSPENDED, &iCollidingConnId)) != NULL )
                {
                    // let colliding voipgroup entity that it can resume its connection right away as the current
                    // voipgroup has just disconnected a connection with different connection parameters
                    pCollidingVoipGroup->pConnSharingCallback(pCollidingVoipGroup, VOIPGROUP_CBTYPE_CONNRESUME,
                                                                   iCollidingConnId, pCollidingVoipGroup->pConnSharingCallbackData, bMuted, bMuted);
                }
            }
            else
            {
                // remove session ID from the set of sessions sharing the low-level voip connection
                // note: don't call this function earlier in time (before VoipDisconnect) because the session id is needed for building the disconnect message
                VoipControl(VoipGetRef(), 'dses', iMappedLowLevelConnId, &pVoipGroup->aConnections[iConnId].uSessionId);

                NetPrintf(("voipgroup: [0x%08x] skipped call to VoipDisconnect() because low-level conn ref count has not reached 0\n", pVoipGroup, iConnId));
            }

            pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId = VOIP_CONNID_NONE;
        }
        else
        {
            NetPrintf(("voipgroup: [0x%08x] warning - VoipGroupDisconnect() dealing with a corrupted connection entry\n", pVoipGroup));
        }
    }
    else
    {
        NetPrintf(("voipgroup: [0x%08x] skipped call to VoipDisconnect() because high-level conn %d is suspended\n", pVoipGroup, iConnId));
    }

    // free the high-level connection entry
    pVoipGroup->aConnections[iConnId].eConnState = VOIPGROUP_CONNSTATE_ACTIVE;
    pVoipGroup->aConnections[iConnId].uClientId = 0;
    pVoipGroup->aConnections[iConnId].uLowLevelConnectivityId = 0;
    pVoipGroup->aConnections[iConnId].uSessionId = 0;
    pVoipGroup->aConnections[iConnId].bIsConnectivityHosted = FALSE;

    return;
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupSuspend

    \Description
        suspend a connection

    \Input *pVoipGroup  - voipgroup ref
    \Input iConnId      - connection index

    \Version 11/11/2009 (mclouatre)
*/
/********************************************************************************F*/
void VoipGroupSuspend(VoipGroupRefT *pVoipGroup, int32_t iConnId)
{
    int32_t iMappedLowLevelConnId;

    NetPrintf(("voipgroup: [0x%08x] suspending high-level conn %d (sess id 0x%08x)\n", pVoipGroup, iConnId, pVoipGroup->aConnections[iConnId].uSessionId));

    // force connection state back to suspended
    iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

    if (iMappedLowLevelConnId != VOIP_CONNID_NONE)
    {
        pVoipGroup->aConnections[iConnId].eConnState = VOIPGROUP_CONNSTATE_SUSPENDED;

        // remove voipgroup from low-level connection's reference set
        _VoipGroupRemoveRefToLowLevelConn(iMappedLowLevelConnId, pVoipGroup);

        if (_VoipGroupLowLevelConnInUse(iMappedLowLevelConnId))
        {
            /* 
            note:
            This scenario can occur if the low-level connection has already been disconnected by the other peer.
            In such a case, the voipgroup is not aware of the state of the low-level connection, and it still
            maintains the high-level connection entry as the corresponding connapi client is maintained in DISC
            state by the upper ConnApi layer. Now when asked to go to suspended state because of a conflict with another
            newly created voipgroup, it is very possible that the low-level connection has already been re-used and has
            other voipgroups referring to it.
            */
            NetPrintf(("voipgroup: [0x%08x] VoipGroupSuspend() skips call to VoipDisconnect() because low-level conn ref count has not reached 0\n", pVoipGroup));
        }
        else
        {
            /*
            note:
            bSendDiscMsg is FALSE here because we don't want the voip disc pkt to result in the
            other end of the connection having its connapi voip connection status moved to
            CONNAPI_STATUS_DISC state.
            */
            VoipDisconnect2(VoipGetRef(), iMappedLowLevelConnId, 0, FALSE);

            _VoipGroupManagerGet()->uUserRecvMask &= ~(1 << iMappedLowLevelConnId);
            VoipSpeaker(VoipGetRef(), _VoipGroupManagerGet()->uUserRecvMask);
            _VoipGroupManagerGet()->uUserSendMask &= ~(1 << iMappedLowLevelConnId);
            VoipMicrophone(VoipGetRef(), _VoipGroupManagerGet()->uUserSendMask);

            pVoipGroup->aConnections[iConnId].iMappedLowLevelConnId = VOIP_CONNID_NONE;
        }
    }
    else
    {
         NetPrintf(("voipgroup: [0x%08x] low level conn id not found\n", pVoipGroup));
    }

    return;
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupMuteByClientId2

    \Description
        mutes a connection by the client id and mute flag

    \Input *pVoipGroup  - voipgroup ref
    \Input uClientId    - client id
    \Input bMute        - TRUE to mute FALSE to umute
    \Input uMuteFlag    - mute flags (VOIPGROUP_INBOUND_MUTE, VOIPGROUP_OUTBOUND_MUTE, VOIPGROUP_TWOWAY_MUTE)

    \Output
        int32_t         - negative=error, zero=success

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
int32_t VoipGroupMuteByClientId2(VoipGroupRefT *pVoipGroup, uint32_t uClientId, uint8_t bMute, uint32_t uMuteFlag)
{
    int32_t iConnId;

    for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
    {
        if (pVoipGroup->aConnections[iConnId].uClientId == uClientId)
        {
            break;
        }
    }

    if (iConnId == MAX_CLIENTS_PER_GROUP)
    {
        return(-1);
    }

    return(VoipGroupMuteByConnId2(pVoipGroup, iConnId, bMute, uMuteFlag));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupMuteByClientId

    \Description
       2-way mutes a connection by the client id 

    \Input *pVoipGroup  - voipgroup ref
    \Input iClientId    - client id
    \Input uMute        - TRUE to mute FALSE to umute

    \Output
        int32_t         - negative=error, zero=success

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
int32_t VoipGroupMuteByClientId(VoipGroupRefT *pVoipGroup, uint32_t iClientId, uint8_t uMute)
{
    return(VoipGroupMuteByClientId2(pVoipGroup, iClientId, uMute, VOIPGROUP_TWOWAY_MUTE));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupMuteByConnId2

    \Description
        mutes a connection by the high level connection id and mute flag

    \Input *pVoipGroup  - voipgroup ref
    \Input iConnId      - connection index
    \Input bMute        - TRUE to mute FALSE to umute
    \Input uMuteFlag    - mute flags (VOIPGROUP_INBOUND_MUTE, VOIPGROUP_OUTBOUND_MUTE, VOIPGROUP_TWOWAY_MUTE)

    \Output
        int32_t         - negative=error, zero=success

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
int32_t VoipGroupMuteByConnId2(VoipGroupRefT *pVoipGroup, uint32_t iConnId, uint8_t bMute, uint32_t uMuteFlag)
{
    VoipGroupManagerT* pManager = _VoipGroupManagerGet();
    int32_t iMappedLowLevelConnId;

    iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

    if (iMappedLowLevelConnId == VOIP_CONNID_NONE)
    {
        return(-1);
    }

    NetCritEnter(&pManager->crit);

    if (bMute)
    {
        if (uMuteFlag & VOIPGROUP_INBOUND_MUTE)
        {
            pManager->uUserRecvMask &= ~(1 << iMappedLowLevelConnId);
        }

        if (uMuteFlag & VOIPGROUP_OUTBOUND_MUTE)
        {
            pManager->uUserSendMask &= ~(1 << iMappedLowLevelConnId);
        }
    }
    else
    {
        if (uMuteFlag & VOIPGROUP_INBOUND_MUTE)
        {
            pManager->uUserRecvMask |= (1 << iMappedLowLevelConnId);
        }

        if (uMuteFlag & VOIPGROUP_OUTBOUND_MUTE)
        {
            pManager->uUserSendMask |= (1 << iMappedLowLevelConnId);
        }
    }
    VoipSpeaker(VoipGetRef(), pManager->uUserRecvMask);
    VoipMicrophone(VoipGetRef(), pManager->uUserSendMask);

    NetCritLeave(&pManager->crit);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupMuteByConnId

    \Description
        2-way mutes a connection by the high level connection id

    \Input *pVoipGroup  - voipgroup ref
    \Input iConnId      - connection index
    \Input uMute        - TRUE to mute FALSE to umute

    \Output
        int32_t         - negative=error, zero=success

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
int32_t VoipGroupMuteByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId, uint8_t uMute)
{
    return (VoipGroupMuteByConnId2(pVoipGroup, iConnId, uMute, VOIPGROUP_TWOWAY_MUTE));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupIsMutedByClientId2

    \Description
       reutrns the mute flag for the connection specified by the client id

    \Input *pVoipGroup  - voipgroup ref
    \Input iClientId    - connection index

    \Output
        uint8_t         - mute flag

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
uint32_t VoipGroupIsMutedByClientId2(VoipGroupRefT *pVoipGroup, uint32_t iClientId)
{
    int32_t iConnId;

    for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
    {
        if (pVoipGroup->aConnections[iConnId].uClientId == iClientId)
        {
            break;
        }
    }

    if (iConnId == MAX_CLIENTS_PER_GROUP)
    {
        return(FALSE);
    }

    return(VoipGroupIsMutedByConnId2(pVoipGroup, iConnId));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupIsMutedByClientId

    \Description
       Is the connection specified by the client id muted or not (1-way or 2-way)

    \Input *pVoipGroup  - voipgroup ref
    \Input uClientId      - connection index

    \Output
        int32_t         - TRUE only if muted 2-ways

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
uint8_t VoipGroupIsMutedByClientId(VoipGroupRefT *pVoipGroup, uint32_t uClientId)
{
    int32_t iConnId;

    for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
    {
        if (pVoipGroup->aConnections[iConnId].uClientId == uClientId)
        {
            break;
        }
    }

    if (iConnId == MAX_CLIENTS_PER_GROUP)
    {
        return(FALSE);
    }

    return(VoipGroupIsMutedByConnId(pVoipGroup, iConnId));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupIsMutedByConnId2

    \Description
       reutrns the mute flag for the connection specified by the high level connection id

    \Input *pVoipGroup  - voipgroup ref
    \Input iConnId      - connection index

    \Output
        uint32_t         - mute flag 

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
uint32_t VoipGroupIsMutedByConnId2(VoipGroupRefT *pVoipGroup, uint32_t iConnId)
{
    uint8_t uMuteFlag = 0;
    int32_t iMappedLowLevelConnId;
    VoipGroupManagerT* pManager = _VoipGroupManagerGet();

    iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

    if (iMappedLowLevelConnId == VOIP_CONNID_NONE)
    {
        return(0);
    }

    if ((pManager->uUserRecvMask & (1 << iMappedLowLevelConnId)) == 0)
    {
        uMuteFlag |= VOIPGROUP_INBOUND_MUTE;
    }

    if ((pManager->uUserSendMask & (1 << iMappedLowLevelConnId)) == 0)
    {
        uMuteFlag |= VOIPGROUP_OUTBOUND_MUTE;
    }

    return(uMuteFlag);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupIsMutedByConnId

    \Description
       Is the connection specified by the conn id muted or not (1-way or 2-way)

    \Input *pVoipGroup  - voipgroup ref
    \Input iConnId      - connection index

    \Output
        int32_t         - TRUE only if it is 2-way muted

    \Version 11/02/2015 (tcho)
*/
/********************************************************************************F*/
uint8_t VoipGroupIsMutedByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId)
{
    uint32_t uMuteFlag = VoipGroupIsMutedByConnId2(pVoipGroup, iConnId);

    if (uMuteFlag == VOIPGROUP_TWOWAY_MUTE)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupConnStatus

    \Description
        Passes through to VoipConnRemote, afer adjusting connection ID

    \Input *pVoipGroup  - The VoipGroup 
    \Input iConnId      - The connection ID

    \Output
        int32_t          - VOIP_REMOTE_* flags, or VOIP_FLAG_INVALID if iConnId is invalid

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipGroupConnStatus(VoipGroupRefT *pVoipGroup, int32_t iConnId)
{
    int32_t iMappedConnId = _VoipGroupMatch(pVoipGroup, iConnId);

    // if the high-level voip connection is "suspended", in which case it has no valid low-level voip
    // connection associated with it, always fake that the low-level connection is "connected".
    if (pVoipGroup->aConnections[iConnId].eConnState == VOIPGROUP_CONNSTATE_SUSPENDED)
    {
        return(VOIP_CONN_CONNECTED);
    }

    if (iMappedConnId == VOIP_CONNID_NONE)
    {
        return(VOIP_FLAG_INVALID);
    }

    return(VoipConnStatus(VoipGetRef(), iMappedConnId));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupRemoteUserStatus

    \Description
        Passes through to VoipUserRemote, afer adjusting connection ID

    \Input *pVoipGroup      - The VoipGroup 
    \Input iConnId          - The connection ID
    \Input iRemoteUserIndex - User index

    \Output
        int32_t          - VOIP_REMOTE_* flags, or VOIP_FLAG_INVALID if iConnId is invalid

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipGroupRemoteUserStatus(VoipGroupRefT *pVoipGroup, int32_t iConnId, int32_t iRemoteUserIndex)
{
    int32_t iMappedConnId = _VoipGroupMatch(pVoipGroup, iConnId);

    // if the high-level voip connection is "suspended", in which case it has no valid low-level voip
    // connection associated with it, always fake that the low-level connection is "connected".
    if (pVoipGroup->aConnections[iConnId].eConnState == VOIPGROUP_CONNSTATE_SUSPENDED)
    {
        return(VOIP_CONN_CONNECTED);
    }

    if (iMappedConnId == VOIP_CONNID_NONE)
    {
        return(VOIP_FLAG_INVALID);
    }

    return(VoipRemoteUserStatus(VoipGetRef(), iMappedConnId, iRemoteUserIndex));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupLocalUserStatus

    \Description
        return information about local hardware state

    \Input *pVoipGroup      - The VoipGroup
    \Input  iLocalUserIndex - User index

    \Output
        int32_t         - VOIP_LOCAL_* flags

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipGroupLocalUserStatus(VoipGroupRefT *pVoipGroup, int32_t iLocalUserIndex)
{
    return(VoipLocalUserStatus(VoipGetRef(), iLocalUserIndex));
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupStatus

    \Description
        Passes through to VoipStatus, for historical reasons

    \Input *pVoipGroup - The VoipGroup
    \Input iSelect     - status selector
    \Input iValue      - selector-specific
    \Input *pBuf       - selector-specific
    \Input iBufSize    - size of the pBuf input

    \Output
        int32_t     - selector-specific data

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipGroupStatus(VoipGroupRefT *pVoipGroup, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    int32_t iRetCode = -1;   // default to error

    if (iSelect == 'rchn')
    {
        // convert high-level id to low-level id
        int32_t iMappedConnId = _VoipGroupMatch(pVoipGroup, iValue);

        if (iMappedConnId != VOIP_CONNID_NONE)
        {
            iRetCode = VoipStatus(VoipGetRef(), iSelect, iMappedConnId, pBuf, iBufSize);
        }
    }
    else if (iSelect == 'ruid')
    {
        int32_t iMappedConnId;
        int32_t iHighLevelConnId = iValue & 0xFFFF;
        int32_t iRemoteUserIndex = iValue >> 16;

        // convert high-level id to low-level id
        iMappedConnId = _VoipGroupMatch(pVoipGroup, iHighLevelConnId);

        if (iMappedConnId != VOIP_CONNID_NONE)
        {
            // rebuild input parameter with converted conn index
            iValue = iMappedConnId;
            iValue |= (iRemoteUserIndex << 16);

            iRetCode = VoipStatus(VoipGetRef(), iSelect, iValue, pBuf, iBufSize);
        }
    }
    else
    {
        iRetCode = VoipStatus(VoipGetRef(), iSelect, iValue, pBuf, iBufSize);
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupControl

    \Description
        Passes through to VoipControl, afer adjusting connection ID and flags.

    \Input *pVoipGroup - The VoipGroup
    \Input iControl    - status selector
    \Input iValue      - selector-specific
    \Input iValue2     - selector-specific
    \Input *pValue     - selector-specific

    \Output
        int32_t         - selector-specific data

    \verbatim
        ccmd: set the CC mode  (VOIPGROUP_CCMODE_*)
        umic: disable/enable sending of voip packets on a given a connection. Note the send mask is base on the low level conn index. (Deprecated)
        uspk: disable/enable receiving of voip packets on a given a connection. Note the receive mask is base on the low level conn index. (Deprecated)
        vcid: set the local id (pValue) for the voip connection with client (iValue)
    \endverbatim
        
    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipGroupControl(VoipGroupRefT *pVoipGroup, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

    if (iControl == 'ccmd')
    {
        #if DIRTYCODE_LOGGING
        static const char *_VoipGroupCcModeNames[] =
        {
            "VOIPGROUP_CCMODE_PEERONLY",
            "VOIPGROUP_CCMODE_HOSTEDONLY",
            "VOIPGROUP_CCMODE_HOSTEDFALLBACK"
        };
        #endif

        NetPrintf(("voipgroup: [0x%08x] CC mode = %d (%s)\n", pVoipGroup, iValue, _VoipGroupCcModeNames[iValue]));
        pVoipGroup->iCcMode = iValue;
        return(0);
    }
    if (iControl == 'getr')
    {
        ds_memcpy(pValue, &pVoipGroup, sizeof(pVoipGroup));
        return(0);
    }
    if (iControl == 'lusr')
    {
        if (VoipGetRef() == NULL)
        {
            return(-1);
        }

        // only proceed with performing the operation for the first group created or
        // for the last group being destroyed as groups cannot have different voip
        // local users.
        if (pManager->uGroupsAvailable != MAX_GROUPS-1)
        {
            NetPrintf(("voipgroup: [0x%08x] skipping %s of voip local user and %d, because multiple voipgroups exist\n",
                pVoipGroup, (iValue2 ? "registration" : "unregistration"), iValue));
            return(-1);
        }

        VoipSetLocalUser(VoipGetRef(), iValue, iValue2);
        return(0);
    }
    if (iControl == 'serv')
    {
        pVoipGroup->bServer = iValue;
        return(0);
    }
    if (iControl == 'tunl')
    {
        pVoipGroup->bTunnel = iValue;
        return(0);
    }
    if (iControl == 'vcid')
    {
        NetPrintf(("voipgroup: [0x%08x] set local client id 0x%08x\n", pVoipGroup, *(int32_t*)pValue));
        pVoipGroup->aLowLevelConnectivityId[iValue] = *(uint32_t*)pValue;
        return(0);
    }
    if (VoipGetRef() != NULL)
    {
        // these two selector are deprecated
        if (iControl == 'umic')
        {
            pManager->uUserSendMask = (uint32_t)iValue;
        }
        if (iControl == 'uspk')
        {
            pManager->uUserRecvMask = (uint32_t)iValue;
        }
        return(VoipControl(VoipGetRef(), iControl, iValue, pValue));
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function   VoipGroupActivateLocalUser

    \Description
        Refcounted access to VoipActivateLocalUser()

    \Input *pVoipGroup      - The VoipGroup (if voipgroup is null and bActivate is false we will deactivate the user from all the voipgroup)
    \Input iLocalUserIndex  - local user index
    \Input bActivate        - TRUE to activate, FALSE to deactivate

    \Version 05/12/2014 (mclouatre)
*/
/********************************************************************************F*/
void VoipGroupActivateLocalUser(VoipGroupRefT *pVoipGroup, int32_t iLocalUserIndex, uint8_t bActivate)
{
    if (iLocalUserIndex >= 0 && iLocalUserIndex < VOIP_MAXLOCALUSERS)
    {
        if (bActivate && (pVoipGroup != NULL))
        {
            // sanity check
            if (_VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex] < 0)
            {
                NetPrintf(("voipgroup: [0x%08x] warning, VoipGroupActivateLocalUser invalid ref count for user %d, %d\n", pVoipGroup, iLocalUserIndex, _VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex]));
                _VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex] = 0;
            }

            // increment ref count
            ++(_VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex]);

            pVoipGroup->aParticipatingUser[iLocalUserIndex] = TRUE;

            NetPrintf(("voipgroup: [0x%08x] VoipGroupActivateLocalUser incrementing ref count for user %d to %d\n", pVoipGroup, iLocalUserIndex, _VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex]));

            // activate underlying voip ref if required
            if (_VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex] <= 1)
            {
                VoipActivateLocalUser(VoipGetRef(), iLocalUserIndex, bActivate);
            }
        }
        else
        {
            // sanity check
            if (_VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex] <= 0)
            {
                NetPrintf(("voipgroup: [0x%08x] warning, deactivating local user %d with ref count %d\n", pVoipGroup, iLocalUserIndex, _VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex]));
                _VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex] = 0;
            }
            else
            {
                if (pVoipGroup == NULL)
                {
                   int32_t iGroupIndex = 0;
                   VoipGroupManagerT *pVoipGroupManager = _VoipGroupManagerGet();

                   for(iGroupIndex = 0; iGroupIndex < MAX_GROUPS; ++iGroupIndex)
                   {
                       if (pVoipGroupManager->aGroups[iGroupIndex].aParticipatingUser[iLocalUserIndex] == TRUE)
                       {
                           // decrement ref count
                           --(pVoipGroupManager->aActiveRefCount[iLocalUserIndex]);
                           pVoipGroupManager->aGroups[iGroupIndex].aParticipatingUser[iLocalUserIndex] = FALSE;
                       }
                   }

                   NetPrintf(("voipgroup: VoipGroupActivateLocalUser deactivating user %d from all voipgroups\n",  iLocalUserIndex));
                }
                else
                {
                    if (pVoipGroup->aParticipatingUser[iLocalUserIndex] == TRUE)
                    {
                        // decrement ref count
                        --(_VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex]);
                        pVoipGroup->aParticipatingUser[iLocalUserIndex] = FALSE;
                        NetPrintf(("voipgroup: [0x%08x] VoipGroupActivateLocalUser decrementing ref count for user %d to %d\n", pVoipGroup, iLocalUserIndex, _VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex]));
                    }
                }
       
                // deactivate underlying voip ref if required
                if (_VoipGroupManagerGet()->aActiveRefCount[iLocalUserIndex] <= 0)
                {
                    VoipActivateLocalUser(VoipGetRef(), iLocalUserIndex, bActivate);
                }
            }
        }
    }
}
