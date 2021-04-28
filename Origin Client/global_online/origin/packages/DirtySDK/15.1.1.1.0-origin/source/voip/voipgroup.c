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
    VoipGroupConnStateE eConnState; //<! state of this high-level connection
    int32_t  iMappedLowLevelConnId; //<! id of low-level voip connection to which this high-level connection is mapped
    uint32_t uClientId;             //<! uClientId associated with this high-level connection
    uint32_t uSessionId;            //<! uSessionId associated with this high-level connection
    #if defined(DIRTYCODE_XENON)
    uint32_t uAddress;              //<! ip address to be used to send data for this connection
    #endif
} VoipGroupConnT;

//! VoIP module state
struct VoipGroupRefT
{
    VoipGroupConnT  connections[MAX_CLIENTS_PER_GROUP];

    uint8_t         bUsed;           //<! whether a group is used

    //! low-level voip connection sharing
    uint8_t         bServer;         //<! TRUE when voipgroup needs voip connections via voipserver, FALSE otherwise
    uint8_t         bTunnel;         //<! TRUE when voipgroup needs voip connections over dirtysock tunnel, FALSE otherwise

    uint8_t         aParticipatingUser[VOIP_MAXLOCALUSERS]; //<! TRUE when this VoipGroup has activated a local user

    //! connection sharing
    ConnSharingCallbackT* pConnSharingCallback;
    void*           pConnSharingCallbackData;

    VoipCallbackT*  pGroupCallback;
    void*           pGroupCallbackData;
};

typedef struct VoipGroupManagerT
{
    VoipGroupRefT   groups[MAX_GROUPS];
    uint8_t         uGroupsAvailable;
    int32_t         aActiveRefCount[VOIP_MAXLOCALUSERS];

    uint32_t        uUserRecvMask;  //<! the recv mask associated with the user mute requests
    uint32_t        uUserSendMask;  //<! the send mask associated with the user mute requests

    //! tracking of different voipgroups using the same low-level voip connection
    VoipGroupRefT* lowLevelConnReferences[VOIP_MAX_LOW_LEVEL_CONNS][MAX_REF_PER_LOW_LEVEL_CONNS];

    //! module memory group
    int32_t         iMemGroup;
    void            *pMemGroupUserData;
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
    VoipGroupManagerT *pManager = pUserData;

    int32_t iConnId, iGroup;
    int32_t iNewValue, iMappedConnId;

    for(iGroup = 0; iGroup < MAX_GROUPS; iGroup++)
    {
        if ((pManager->groups[iGroup].bUsed) && (pManager->groups[iGroup].pGroupCallback))
        {
            if (eCbType == VOIP_CBTYPE_FROMEVENT)
            {
                iNewValue = 0;
                for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
                {
                    iMappedConnId = _VoipGroupMatch(&pManager->groups[iGroup], iConnId);
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

            pManager->groups[iGroup].pGroupCallback(pVoip, eCbType, iNewValue, pManager->groups[iGroup].pGroupCallbackData);
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
        memset(_VoipGroupManager_pRef, 0, sizeof(*_VoipGroupManager_pRef));
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
        if (pVoipGroup->connections[iConnId].uClientId == 0)
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
        if (pVoipGroup->connections[iConnId].uClientId == uClientId)
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
    return(_VoipGroupManagerGet()->lowLevelConnReferences[iConnId][0] != NULL);
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
            if (!pVoipGroupManager->groups[iIndex].bUsed)
            {
                return(&pVoipGroupManager->groups[iIndex]);
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
            if (pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex] != NULL)
            {
                NetPrintf(("0x%08x  ", pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex]));
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
        DirtyMemFree(_VoipGroupManager_pRef, VOIP_MEMID, _VoipGroupManager_pRef->iMemGroup, _VoipGroupManager_pRef->pMemGroupUserData);
        _VoipGroupManager_pRef = NULL;

        // remove callback registered with low-level VOIP module
        if (VoipGetRef() != NULL)
        {
            VoipSetEventCallback(VoipGetRef(), NULL, NULL);
        }
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
        if (pVoipGroup->connections[iSlot].uClientId != 0)
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
    memset(pVoipGroup, 0, sizeof(VoipGroupRefT));

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
    if ((iConnId < 0) || (iConnId >= MAX_CLIENTS_PER_GROUP))
    {
        return(VOIP_CONNID_NONE);
    }

    if (pVoipGroup->connections[iConnId].uClientId != 0)
    {
        return(pVoipGroup->connections[iConnId].iMappedLowLevelConnId);
    }

    return(VOIP_CONNID_NONE);
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
            if ( (pManager->groups[iIndex].connections[iConnId].uClientId == uClientId) &&
                 (pManager->groups[iIndex].connections[iConnId].eConnState == eConnState) )
            {
                // do not allow a voipgroup to collide with itself
                if (&pManager->groups[iIndex] == pVoipGroup)
                {
                    continue;
                }

                NetPrintf(("voipgroup: [0x%08x] collision found with voipgroup 0x%08x for client id 0x%08x\n", pVoipGroup, &pManager->groups[iIndex], uClientId));
                *pCollidingConnId = iConnId;
                return(&pManager->groups[iIndex]);
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
        if (pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex] == pVoipGroup)
        {
            iRetCode = -1;
            NetPrintf(("voipgroup: [0x%08x] error - current voipgroup already exists as a reference for low-level voip conn %d\n", pVoipGroup, iLowLevelConnId));
            break;
        }

        // look for a free spot to find our ref
        if (pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex] == NULL)
        {
            pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex] = pVoipGroup;
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
        if (pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex] == pVoipGroup)
        {
            int32_t iRefIndex2;

            // move all following references one cell backward in the array
            for(iRefIndex2 = iRefIndex; iRefIndex2 < MAX_REF_PER_LOW_LEVEL_CONNS; iRefIndex2++)
            {
                if (iRefIndex2 == MAX_REF_PER_LOW_LEVEL_CONNS-1)
                {
                    // last entry, reset to NULL
                    pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex2] = NULL;
                }
                else
                {
                    pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex2] = pManager->lowLevelConnReferences[iLowLevelConnId][iRefIndex2+1];
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
    int32_t iMappedLowLevelConnId = VOIP_CONNID_NONE;
    int32_t iHighLevelConnIdInUse = VOIP_CONNID_NONE;
    int32_t iCollidingConnId;
    VoipGroupRefT *pCollidingVoipGroup;

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
            NetPrintf(("voipgroup: [0x%08x] current voipgroup already knows about client id 0x%08x with high-level conn id %d\n", 
                pVoipGroup, uClientId, iConnId));

            // if connection is "suspended", early exit faking a success
            if (pVoipGroup->connections[iConnId].eConnState == VOIPGROUP_CONNSTATE_SUSPENDED)
            {
                NetPrintf(("voipgroup: [0x%08x] high-level conn % (sess id 0x%08x) is suspended, assume low-level connectivity is up\n", pVoipGroup, iConnId, pVoipGroup->connections[iConnId].uSessionId));
                return(iConnId);
            }
        }
        else
        {
            NetPrintf(("voipgroup: [0x%08x] critical error - current voipgroup already knows about client id x%08x but conn ids do not match (%d != %d)\n", 
                pVoipGroup, uClientId, iHighLevelConnIdInUse, iConnId));
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
        // if both voipgroups do not share same bServer / bTunnel attributes, then connection re-establishment kicks-in
        if ( (pCollidingVoipGroup->bServer != pVoipGroup->bServer) ||
             (pCollidingVoipGroup->bTunnel != pVoipGroup->bTunnel) )
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
            if (pCollidingVoipGroup->bServer)
            {
                // no need to proceed with current connection establishment because the colliding voipgroup
                // already has voip connectivity via the voipserver, so it has precedence. just mark current
                // connection as "SUSPENDED" such that it can be resumed later if needed.
                pVoipGroup->connections[iConnId].eConnState = VOIPGROUP_CONNSTATE_SUSPENDED;
                pVoipGroup->connections[iConnId].iMappedLowLevelConnId = VOIP_CONNID_NONE;
                pVoipGroup->connections[iConnId].uClientId = uClientId;
                pVoipGroup->connections[iConnId].uSessionId = uSessionId;
                #if defined(DIRTYCODE_XENON)
                pVoipGroup->connections[iConnId].uAddress = uAddress;
                #endif
                NetPrintf(("voipgroup: [0x%08x] early suspension of high-level conn %d (sess id 0x%08x) - there already exists a low-level voip conn through the voipserver for client id 0x%08x.\n",
                    pVoipGroup, iConnId, uSessionId, uClientId));
                return(iConnId); // fake success
            }
            else
            {
                // let the colliding voipgroup entity know that it has to suspend its connection right away
                // because the current voipgroup will be creating a new one with new connection parameters
                pCollidingVoipGroup->pConnSharingCallback(pCollidingVoipGroup, VOIPGROUP_CBTYPE_CONNSUSPEND,
                                                              iCollidingConnId, pCollidingVoipGroup->pConnSharingCallbackData);
            }
        }
        else
        {
            // both voipgroups can share the underlying low-level voip connection
            iMappedLowLevelConnId = pCollidingVoipGroup->connections[iCollidingConnId].iMappedLowLevelConnId;

            // add session ID to the set of sessions sharing the voip connection
            VoipControl(VoipGetRef(), 'ases', iMappedLowLevelConnId, &uSessionId);

            #if defined(DIRTYCODE_XENON)
            // add uAddress to the send address fallback list of the existing low-level voip connection
            VoipControl(VoipGetRef(), 'afbk', iMappedLowLevelConnId, &uAddress);
            #endif
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
            iMappedLowLevelConnId = VoipConnect(VoipGetRef(), iConnId, uAddress, uManglePort, uGamePort, uClientId, uSessionId);
        }
        else
        {
            // else, let the voip module select a low-level voip conn id for us.
            iMappedLowLevelConnId = VoipConnect(VoipGetRef(), VOIP_CONNID_NONE, uAddress, uManglePort, uGamePort, uClientId, uSessionId);
        }
    }

    if (iMappedLowLevelConnId < 0)
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupConnect() failed obtaining a valid low-level conn id\n", pVoipGroup));
        return(-2);
    }

    NetPrintf(("voipgroup: [0x%08x] high-level conn id %d (sess id 0x%08x) mapped to low-level conn id %d --> address %a, clientId=0x%08x.\n", 
        pVoipGroup, iConnId, uSessionId, iMappedLowLevelConnId, uAddress, uClientId));

    // save the mapping.
    pVoipGroup->connections[iConnId].iMappedLowLevelConnId = iMappedLowLevelConnId;
    if (pVoipGroup->bServer == TRUE)
    {
        // let the voip module know about the local-to-voipserver conn id mapping
        VoipControl(VoipGetRef(), 'conm', pVoipGroup->connections[iConnId].iMappedLowLevelConnId, &iConnId);
    }

    // save other connection-specific data
    pVoipGroup->connections[iConnId].uClientId = uClientId;
    pVoipGroup->connections[iConnId].uSessionId = uSessionId;
    #if defined(DIRTYCODE_XENON)
    pVoipGroup->connections[iConnId].uAddress = uAddress;
    #endif

    // if this is the first attempt to connect, let's just add ourself as an additional reference for this connection
    if ((iHighLevelConnIdInUse == VOIP_CONNID_NONE) && (_VoipGroupAddRefToLowLevelConn(iMappedLowLevelConnId, pVoipGroup) < 0))
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupConnect() failed to add a new reference to the low-level conn %d\n", pVoipGroup, iMappedLowLevelConnId));
        VoipGroupDisconnect(pVoipGroup, iConnId);
        return(-3);
    }

    _VoipGroupManagerGet()->uUserRecvMask |= (1 << iMappedLowLevelConnId);
    VoipSpeaker(VoipGetRef(), _VoipGroupManagerGet()->uUserRecvMask);
    _VoipGroupManagerGet()->uUserSendMask |= (1 << iMappedLowLevelConnId);
    VoipMicrophone(VoipGetRef(), _VoipGroupManagerGet()->uUserSendMask);

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

    \Version 11/11/2009 (mclouatre)
*/
/********************************************************************************F*/
void VoipGroupResume(VoipGroupRefT *pVoipGroup, int32_t iConnId, uint32_t uAddress, uint32_t uManglePort, uint32_t uGamePort, uint32_t uClientId, uint32_t uSessionId)
{
    int32_t iMappedLowLevelConnId = -1;

    NetPrintf(("voipgroup: [0x%08x] resuming high-level conn %d (sess id 0x%08x)\n", pVoipGroup, iConnId, uSessionId));

    #if DIRTYCODE_LOGGING
    if (pVoipGroup->connections[iConnId].uClientId != uClientId)
    {
        NetPrintf(("voipgroup: [0x%08x] warning connection being resumed has inconsistent client id: 0x%08x should be 0x%08x\n",
            pVoipGroup, uClientId, pVoipGroup->connections[iConnId].uClientId));
    }
    if (pVoipGroup->connections[iConnId].uSessionId != uSessionId)
    {
        NetPrintf(("voipgroup: [0x%08x] warning connection being resumed has inconsistent session id: 0x%08x should be 0x%08x\n",
            pVoipGroup, uSessionId, pVoipGroup->connections[iConnId].uSessionId));
    }
    #if defined(DIRTYCODE_XENON)
    if (pVoipGroup->connections[iConnId].uAddress != uAddress)
    {
        NetPrintf(("voipgroup: [0x%08x] warning connection being resumed has inconsistent ip address: %a should be %a\n",
            pVoipGroup, uAddress, pVoipGroup->connections[iConnId].uAddress));
    }
    #endif
    #endif

    // force connection back to active state
    pVoipGroup->connections[iConnId].eConnState = VOIPGROUP_CONNSTATE_ACTIVE;

    // check if the high-level conn id is directly available in voip and if there is currently no reference to it
    // (there is indeed a small transition time during which a connection may go to DISC state and a voip goup still refers to it)
    if (VoipStatus(VoipGetRef(), 'avlb', iConnId, NULL, 0) && !_VoipGroupLowLevelConnInUse(iConnId))
    {
        // conn id is available, use it!
        iMappedLowLevelConnId = VoipConnect(VoipGetRef(), iConnId, uAddress, uManglePort, uGamePort, uClientId, uSessionId);
    }
    else
    {
        // else, let Voip allocate one.
        iMappedLowLevelConnId = VoipConnect(VoipGetRef(), VOIP_CONNID_NONE, uAddress, uManglePort, uGamePort, uClientId, uSessionId);
    }

    if (iMappedLowLevelConnId < 0)
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupResume() failed obtaining a valid low-level conn id\n", pVoipGroup));
        return;
    }

    NetPrintf(("voipgroup: [0x%08x] high-level conn id %d (sess id 0x%08x) mapped to low-level conn id %d --> address %a, clientId=0x%08x.\n", 
        pVoipGroup, iConnId, uSessionId, iMappedLowLevelConnId, uAddress, uClientId));

    // save the mapping.
    pVoipGroup->connections[iConnId].iMappedLowLevelConnId = iMappedLowLevelConnId;
    if (pVoipGroup->bServer == TRUE)
    {
        // let the voip module know about the local-to-voipserver conn id mapping
        VoipControl(VoipGetRef(), 'conm', pVoipGroup->connections[iConnId].iMappedLowLevelConnId, &iConnId);
    }

    // let's just add ourself as an additional reference for this connection
    if ((_VoipGroupAddRefToLowLevelConn(iMappedLowLevelConnId, pVoipGroup)) < 0)
    {
        NetPrintf(("voipgroup: [0x%08x] VoipGroupResume() failed adding a new reference\n", pVoipGroup));
        return;
    }

    _VoipGroupManagerGet()->uUserRecvMask |= (1 << iMappedLowLevelConnId);
    VoipSpeaker(VoipGetRef(), _VoipGroupManagerGet()->uUserRecvMask);
    _VoipGroupManagerGet()->uUserSendMask |= (1 << iMappedLowLevelConnId);
    VoipMicrophone(VoipGetRef(), _VoipGroupManagerGet()->uUserSendMask);

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

    NetPrintf(("voipgroup: [0x%08x] disconnecting high-level conn %d (sess id 0x%08x)\n", pVoipGroup, iConnId, pVoipGroup->connections[iConnId].uSessionId));

    // if high-level connection is already suspended, skip disconnection and just clean up corresponding data in the group
    if (pVoipGroup->connections[iConnId].eConnState != VOIPGROUP_CONNSTATE_SUSPENDED)
    {
        iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

        if (iMappedLowLevelConnId != VOIP_CONNID_NONE)
        {
            // remove voipgroup from low-level connection's reference set
            _VoipGroupRemoveRefToLowLevelConn(iMappedLowLevelConnId, pVoipGroup);

            #if defined(DIRTYCODE_XENON)
            // remove peer address from the send address fallback list of the existing low-level voip connection
            VoipControl(VoipGetRef(), 'dfbk', iMappedLowLevelConnId, &pVoipGroup->connections[iConnId].uAddress);
            #endif 

            // only proceed if the ref count for this low-level conn is 0
            if (!_VoipGroupLowLevelConnInUse(iMappedLowLevelConnId))
            {
                VoipDisconnect(VoipGetRef(), iMappedLowLevelConnId, 0);

                _VoipGroupManagerGet()->uUserRecvMask &= ~(1 << iMappedLowLevelConnId);
                VoipSpeaker(VoipGetRef(), _VoipGroupManagerGet()->uUserRecvMask);
                _VoipGroupManagerGet()->uUserSendMask &= ~(1 << iMappedLowLevelConnId);
                VoipMicrophone(VoipGetRef(), _VoipGroupManagerGet()->uUserSendMask);

                // scan other voipgroups and resume any suspended connection to the same client id
                if ( (pCollidingVoipGroup = _VoipGroupFindCollision(pVoipGroup, pVoipGroup->connections[iConnId].uClientId, VOIPGROUP_CONNSTATE_SUSPENDED, &iCollidingConnId)) != NULL )
                {
                    // let colliding voipgroup entity that it can resume its connection right away as the current
                    // voipgroup has just disconnected a connection with different connection parameters
                    pCollidingVoipGroup->pConnSharingCallback(pCollidingVoipGroup, VOIPGROUP_CBTYPE_CONNRESUME,
                                                                   iCollidingConnId, pCollidingVoipGroup->pConnSharingCallbackData);
                }
            }
            else
            {
                // remove session ID from the set of sessions sharing the low-level voip connection
                // note: don't call this function earlier in time (before VoipDisconnect) because the session id is needed for building the disconnect message
                VoipControl(VoipGetRef(), 'dses', iMappedLowLevelConnId, &pVoipGroup->connections[iConnId].uSessionId);

                NetPrintf(("voipgroup: [0x%08x] skipped call to VoipDisconnect() because low-level conn ref count has not reached 0\n", pVoipGroup, iConnId));
            }

            pVoipGroup->connections[iConnId].iMappedLowLevelConnId = VOIP_CONNID_NONE;
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
    pVoipGroup->connections[iConnId].eConnState = VOIPGROUP_CONNSTATE_ACTIVE;
    pVoipGroup->connections[iConnId].uClientId = 0;
    pVoipGroup->connections[iConnId].uSessionId = 0;
    #if defined(DIRTYCODE_XENON)
    pVoipGroup->connections[iConnId].uAddress = 0;
    #endif
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

    NetPrintf(("voipgroup: [0x%08x] suspending high-level conn %d (sess id 0x%08x)\n", pVoipGroup, iConnId, pVoipGroup->connections[iConnId].uSessionId));

    // force connection state back to suspended
    pVoipGroup->connections[iConnId].eConnState = VOIPGROUP_CONNSTATE_SUSPENDED;

    iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

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

        pVoipGroup->connections[iConnId].iMappedLowLevelConnId = VOIP_CONNID_NONE;
    }

    return;
}

int32_t VoipGroupMuteByClientId(VoipGroupRefT *pVoipGroup, uint32_t iClientId, uint8_t uMute)
{
    int32_t iConnId;

    for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
    {
        if (pVoipGroup->connections[iConnId].uClientId == iClientId)
        {
            break;
        }
    }

    if (iConnId == MAX_CLIENTS_PER_GROUP)
    {
        return(-1);
    }

    return(VoipGroupMuteByConnId(pVoipGroup, iConnId, uMute));
}

int32_t VoipGroupMuteByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId, uint8_t uMute)
{
    VoipGroupManagerT* pManager = _VoipGroupManagerGet();
    int32_t iMappedLowLevelConnId;

    iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

    if (iMappedLowLevelConnId == VOIP_CONNID_NONE)
    {
        return(-1);
    }

    if (uMute)
    {
        pManager->uUserRecvMask &= ~(1 << iMappedLowLevelConnId);
        pManager->uUserSendMask &= ~(1 << iMappedLowLevelConnId);
    }
    else
    {
        pManager->uUserRecvMask |= (1 << iMappedLowLevelConnId);
        pManager->uUserSendMask |= (1 << iMappedLowLevelConnId);
    }
    VoipSpeaker(VoipGetRef(), pManager->uUserRecvMask);
    VoipMicrophone(VoipGetRef(), pManager->uUserSendMask);

    return(0);
}

//! whether a client is muted, by ClientId
uint8_t VoipGroupIsMutedByClientId(VoipGroupRefT *pVoipGroup, uint32_t iClientId)
{
    int32_t iConnId;

    for(iConnId = 0; iConnId < MAX_CLIENTS_PER_GROUP; iConnId++)
    {
        if (pVoipGroup->connections[iConnId].uClientId == iClientId)
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


//! whether a client is muted, by ConnId
uint8_t VoipGroupIsMutedByConnId(VoipGroupRefT *pVoipGroup, uint32_t iConnId)
{
    int32_t iMappedLowLevelConnId;
    VoipGroupManagerT* pManager = _VoipGroupManagerGet();

    iMappedLowLevelConnId = _VoipGroupMatch(pVoipGroup, iConnId);

    if (iMappedLowLevelConnId == VOIP_CONNID_NONE)
    {
        return(FALSE);
    }

    return(!(pManager->uUserSendMask & (1 << iMappedLowLevelConnId)));
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
    if (pVoipGroup->connections[iConnId].eConnState == VOIPGROUP_CONNSTATE_SUSPENDED)
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
    if (pVoipGroup->connections[iConnId].eConnState == VOIPGROUP_CONNSTATE_SUSPENDED)
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

    \Version 01/31/2007 (jrainy)
*/
/********************************************************************************F*/
int32_t VoipGroupControl(VoipGroupRefT *pVoipGroup, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    VoipGroupManagerT *pManager = _VoipGroupManagerGet();

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
    if (VoipGetRef() != NULL)
    {
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
                       if (pVoipGroupManager->groups[iGroupIndex].aParticipatingUser[iLocalUserIndex] == TRUE)
                       {
                           // decrement ref count
                           --(pVoipGroupManager->aActiveRefCount[iLocalUserIndex]);
                           pVoipGroupManager->groups[iGroupIndex].aParticipatingUser[iLocalUserIndex] = FALSE;
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
