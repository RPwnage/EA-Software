/*H*************************************************************************************/
/*!
    \File dirtysessionmanagerxenon.c

    \Description
        This module encapsulates the Xenon functions required to resolve an XUID into a
        registered secure address, to enable a secure connection to be established between
        two Xenons.  Like the Xbox implementations of DirtySessionManager, use of this module
        (or something performing the same functionality) is required to be able to connect
        two or more Xenons in secure mode.

        In addition, DirtySessionManager encapsulates use of the XSession API for setting up
        Xenon sessions with peers.

    \Copyright
        Copyright (c) / Electronic Arts 2005-2007.  ALL RIGHTS RESERVED.

    \Version 1.0 05/03/2005 (jbrookes)  First Xenon Version
    \Version 2.0 11/04/2007 (jbrookes)  Removed from ProtoMangle namespace, cleanup
    \Version 2.1 07/06/2009 (mclouatre) Corrected implementation of cancellation of overlapped operations
    \Version 2.2 29/09/2009 (mclouatre) Refactored to have all XSession calls perform from a dedicated thread
    \Version 3.0 29/01/2011 (jrainy) Rewritten
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <xtl.h>
#include <xonline.h>
#include <stdio.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/binary7.h"
#include "DirtySDK/util/utf8.h"
#include "DirtySDK/dirtysock/xenon/dirtysessionmanagerxenon.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/
#define DIRTYSESSIONMANAGER_SESSIONFLAGS_DEFAULT            \
   (XSESSION_CREATE_USES_PRESENCE |                 \
    XSESSION_CREATE_USES_STATS |                    \
    XSESSION_CREATE_USES_MATCHMAKING |              \
    XSESSION_CREATE_USES_PEER_NETWORK |             \
    XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED)

#define DIRTYSESSIONMANAGER_MAXPUBLIC_DEFAULT   (2)
#define DIRTYSESSIONMANAGER_MAXPRIVATE_DEFAULT  (0)

#define DIRTYSESSIONMANAGER_MAX_GAMER_COUNT     (4)
#define DIRTYSESSIONMANAGER_ARBITRATIONTIMER    (1000)  // once a second
#define DIRTYSESSIONMANAGER_MAX_OPERATIONS      (64)

/*** Type Definitions ******************************************************************/

typedef enum
{
    ST_SESSMGR_CREA,            //!< waiting for session creation completion
    ST_SESSMGR_MODI,            //!< modify session flags
    ST_SESSMGR_JOIN_REM,        //!< waiting for session join remote completion
    ST_SESSMGR_LEAV_REM,        //!< waiting for session leave remote completion
    ST_SESSMGR_INVT,            //!< inviting someone
    ST_SESSMGR_ARBI_SIZE,       //!< waiting for completion of arbitration register
    ST_SESSMGR_ARBI,            //!< waiting for completion of arbitration register
    ST_SESSMGR_STRT,            //!< starting session
    ST_SESSMGR_STAT,            //!< writing stats
    ST_SESSMGR_MIGR_HOSTING,    //!< migrate the local user to become the new xsession host
    ST_SESSMGR_MIGR_NOTHOSTING, //!< migrate to the new xsession host (not hosting locally)
    ST_SESSMGR_STOP,            //!< ending session
    ST_SESSMGR_DELE,            //!< deleting session
    ST_SESSMGR_MAX
} SessMgrStateE;

#if DIRTYCODE_LOGGING
static const char *_DsmStateNames[ST_SESSMGR_MAX] =
{
    "ST_SESSMGR_CREA",
    "ST_SESSMGR_MODI",
    "ST_SESSMGR_JOIN_REM",
    "ST_SESSMGR_LEAV_REM",
    "ST_SESSMGR_INVT",
    "ST_SESSMGR_ARBI_SIZE",
    "ST_SESSMGR_ARBI",
    "ST_SESSMGR_STRT",
    "ST_SESSMGR_STAT",
    "ST_SESSMGR_MIGR_HOSTING",
    "ST_SESSMGR_MIGR_NOTHOSTING",
    "ST_SESSMGR_STOP",
    "ST_SESSMGR_DELE",
};
#endif

// this structure is shallow-copied using C builtin assignment operator. Don't put any pointer in.
typedef struct DirtySessionManagerOperation
{
    SessMgrStateE   eOp;
    int32_t         iFlags[DIRTYSESSIONMANAGER_MAX_GAMER_COUNT];
    int32_t         iCount;

    //todo: deprecate, could be a single entry, as it is now used only for migrate and invites, not joinlocal
    DWORD           dwIndex[DIRTYSESSIONMANAGER_MAX_GAMER_COUNT];

    int32_t         iMaxPublicSlots;
    int32_t         iMaxPrivateSlots;
    char            strMachineAddr[64];
    XUID            PeerXuid[DIRTYSESSIONMANAGER_MAX_GAMER_COUNT];
    int32_t         iNumArbitrationClients;
    int32_t         iNumArbitrationAttempts;
    DWORD           dwQueriedSize;
    uint32_t        iTimer;
    uint32_t        uGameType;
    uint32_t        uGameMode;
    uint32_t        uQueueTime;
    uint32_t        uStartTime;

    DWORD           dwResult;
} DirtySessionManagerOperation;

//! internal module state
struct DirtySessionManagerRefT
{
    int32_t iMemGroup;                          //!< module mem group id
    void    *pMemGroupUserData;                 //!< user data associated with mem group

    int32_t iNumQueuedOperations;               //!< number of XSession calls queued, including the one in progress, if any
    uint8_t bOpInProgress;                      //!< whether there's an operation currently in progress
    uint8_t bTerminating;                       //!< whether we're terminating. Set as termination starts
    uint8_t bTerminated;                        //!< whether we're done (failure or graceful). Set once idle
    DirtySessionManagerOperation QueuedOperations[DIRTYSESSIONMANAGER_MAX_OPERATIONS];   //!< queued operations
    uint8_t bDsmPresenceMutexOwner;             //!< whether we have the presence mutex taken

    uint8_t bSessionCreationQueued;             //!< whether we queued an XSessionCreate
    uint8_t bSessionDeletionQueued;             //!< whether we queued an XSessionDelete
    uint8_t bSessionCreationDone;               //!< whether we called an XSessionCreate. The two above have precedence, if the interest is the state after applying all queued ops.
    char    strSession[256];                    //!< the session string

    int32_t iSessionStartQueued;
    int32_t iSessionStopQueued;
    uint8_t bSessionStartDone;

    int32_t iSessionCreationFlags;              //!< flags to use for session creation
    int32_t iSessionQueuedFlags;                //!< flags once all queued operations will be done

    XUID    LocalXuid[NETCONN_MAXLOCALUSERS];   //!< XUIDs of local player, from netconn
    uint8_t bAllowPresenceJoins;                //!< whether this session allows Join-Via-Presence
    uint8_t bAllowPresenceInvites;              //!< whether this session allows Inviting through the Guide
    int32_t iMaxPublicSlots;                    //!< the max number of public slots for this session
    int32_t iMaxPrivateSlots;                   //!< the max number of private slots for this session
    int32_t iRankWeight;                        //!< weight to use for ranking. Exponential decayed on rematches
    uint8_t bRanked;                            //!< whether the session is ranked or not
    uint8_t uJoinInProgress;                    //!< whether the session allows JIP
    uint8_t bLocalSlotTypePrivate[NETCONN_MAXLOCALUSERS];   //!< whether the local slot is private or public
    uint8_t bHosting;                           //!< whether we're hosting. While a migration is queued this shows the final, queued, state
    int32_t iGameMode;                          //!< game mode. Set in the module ref, affects queued and future ops
    int32_t iUserIndex;                         //!< local user index used for session creation

    WCHAR strInviteText[1024];                  //!< buffer for the invite text

    DWORD dwRegistrationSize;                   //!< Size of arbitration registration result buffer
    PXSESSION_REGISTRATION_RESULTS pRegistrationResults;    //!< pointer to internally allocated buffer to be filled with arbitration registration result (XSessionArbitrationRegister())
    XSESSION_INFO SessionInfo;                  //!< information about the session
    ULONGLONG qwSessionNonce;                   //!< nonce information, needed for arbitration
    HANDLE hSession;                            //!< handle to the local session object
    XOVERLAPPED XOverlapped;                    //!< overlapped operation structure for all async ops
    XUID StatXuid;                              //!< XUID to use to write stats
    XSESSION_VIEW_PROPERTIES ViewProperties[1];
    XUSER_PROPERTY ViewSkills[3];
    DWORD dwNumViews;                           //!< number of views; size of buffer pointed to by pViews = dwNumViews * sizeof(XSESSION_VIEW_PROPERTIES)
    XSESSION_VIEW_PROPERTIES *pViews;           //!< pointer to internally allocated buffer to be filled with statistics received from user (input for XSessionWriteStats())

};

static uint8_t _bDsmPresenceMutexTaken = FALSE;


/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function    _DirtySessionManagerTerminate

    \Description
        Triggers termination of the session. Session will be stopped/deleted as appropriate

    \Input *pRef    - pointer to module state

    \Version 01/20/2011 (jrainy)
*/
/*************************************************************************************F*/
static void _DirtySessionManagerTerminate(DirtySessionManagerRefT *pRef)
{
    if (pRef->bTerminating)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] already terminating, ignored\n", pRef));
        return;
    }

    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] terminating\n", pRef));

    // flush all operations except the current running one, if any
    if (pRef->bOpInProgress)
    {
        pRef->iNumQueuedOperations = 1;
    }
    else
    {
        pRef->iNumQueuedOperations = 0;
    }

    pRef->iSessionStartQueued = (pRef->bOpInProgress && pRef->QueuedOperations[0].eOp == ST_SESSMGR_STRT) ? 1 : 0;
    pRef->iSessionStopQueued = (pRef->bOpInProgress && pRef->QueuedOperations[0].eOp == ST_SESSMGR_STOP) ? 1 : 0;
    pRef->bSessionCreationQueued = (pRef->bOpInProgress && pRef->QueuedOperations[0].eOp == ST_SESSMGR_CREA) ? 1 : 0;
    pRef->bSessionDeletionQueued = (pRef->bOpInProgress && pRef->QueuedOperations[0].eOp == ST_SESSMGR_DELE) ? 1 : 0;

    if ((pRef->bSessionCreationDone) || (pRef->bSessionCreationQueued))
    {
        DirtySessionManagerControl(pRef, 'dele', 1, 0, NULL);
    }

    pRef->bTerminating = TRUE;
}

/*F*************************************************************************************/
/*!
    \Function    _DirtySessionManagerQueueOpAtFront

    \Description
        Queues an XSession operation at the beginning of the operation queue. Useful
        when a dequeued Operation must be re-attempted, for example.

    \Input *pRef    - pointer to module state
    \Input *pOp     - pointer to the Operation to Queue (will be copied)

    \Version 01/20/2011 (jrainy)
*/
/*************************************************************************************F*/
static void _DirtySessionManagerQueueOpAtFront(DirtySessionManagerRefT *pRef, DirtySessionManagerOperation *pOp)
{
    if (pRef->iNumQueuedOperations < DIRTYSESSIONMANAGER_MAX_OPERATIONS)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] Queueing %s at front (new queue count=%d)\n", pRef, _DsmStateNames[pOp->eOp], (pRef->iNumQueuedOperations+1)));

        memmove(&pRef->QueuedOperations[1], &pRef->QueuedOperations[0], pRef->iNumQueuedOperations * sizeof(pRef->QueuedOperations[0]));
        pRef->QueuedOperations[0] = *pOp;
        pRef->QueuedOperations[0].uQueueTime = NetTick();
        pRef->iNumQueuedOperations++;
    }
    else
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] Unable to queue %s at front. Queue full.\n", pRef, _DsmStateNames[pOp->eOp]));

        // We can't call _DirtySessionManagerTerminate here, as it would possibly queue operations.
        // So, let's go for the next best thing:
        pRef->bTerminating = TRUE;
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DirtySessionManagerQueueOp

    \Description
        Queues an XSession operation at the end of the operation queue. This is the normal
        way to add XSession operations to the list of pending operations.

    \Input *pRef    - pointer to module state
    \Input *pOp     - pointer to the Operation to Queue (will be copied)

    \Version 01/20/2011 (jrainy)
*/
/*************************************************************************************F*/
static void _DirtySessionManagerQueueOp(DirtySessionManagerRefT *pRef, DirtySessionManagerOperation *pOp)
{
    if (pRef->iNumQueuedOperations < DIRTYSESSIONMANAGER_MAX_OPERATIONS)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] Queueing %s at %d (new queue count=%d)\n", pRef, _DsmStateNames[pOp->eOp], NetTick(), (pRef->iNumQueuedOperations+1)));

        pRef->QueuedOperations[pRef->iNumQueuedOperations] = *pOp;
        pRef->QueuedOperations[pRef->iNumQueuedOperations].uQueueTime = NetTick();
        pRef->iNumQueuedOperations++;
    }
    else
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] Unable to queue %s. Queue full.\n", pRef, _DsmStateNames[pOp->eOp]));

        // We can't call _DirtySessionManagerTerminate here, as it would possibly queue operations.
        // So, let's go for the next best thing:
        pRef->bTerminating = TRUE;
    }
}

/*F*************************************************************************************/
/*!
    \Function    _HandleSessionFlags

    \Description
        Using current module state and base flags provided by the user calculate a new 
        set of session flags.

    \Input *pRef    - module state
    \Input dwFlags  - base flags

    \Output
        int32_t         - a new set of dwFlags

    \Version 01/20/2004 (cvienneau)
*/
/*************************************************************************************F*/
static DWORD _HandleSessionFlags(DirtySessionManagerRefT *pRef, DWORD dwFlags)
{
    if (dwFlags & XSESSION_CREATE_USES_PRESENCE)
    {
        // ranked session?
        if (pRef->bRanked)
        {
            dwFlags |= XSESSION_CREATE_INVITES_DISABLED;
            dwFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
        }
        else
        {
            if (pRef->bAllowPresenceJoins)
            {
                dwFlags &= (~XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED);
            }
            else 
            {
                dwFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
            }

            if (pRef->bAllowPresenceInvites)
            {
                dwFlags &= (~XSESSION_CREATE_INVITES_DISABLED);
            }
            else 
            {
                dwFlags |= XSESSION_CREATE_INVITES_DISABLED;
            }
        }
    }
    else
    {
        dwFlags &= ~XSESSION_CREATE_INVITES_DISABLED;
        dwFlags &= ~XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
    }

    if (pRef->uJoinInProgress)
    {
        dwFlags &= (~XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED);
    }
    else
    {
        dwFlags |= XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;
    }
    
    // Make sure the XESSION_CREATE_HOST flag is not modified explicitly. After session creation, that flag can
    // only be changed through a platform host migration procedure.
    if (pRef->bHosting)
    {
        dwFlags |= XSESSION_CREATE_HOST;
    }
    else
    {
        dwFlags &= ~XSESSION_CREATE_HOST;
    }

    return(dwFlags);
}

/*F*************************************************************************************/
/*!
    \Function    _DirtySessionManagerDestroy

    \Description
        Free Dirty Session Manager

    \Input *pRef    - pointer to module state

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************F*/
static void _DirtySessionManagerDestroy(DirtySessionManagerRefT *pRef)
{
    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] final destruction\n", pRef));

    if (pRef->bDsmPresenceMutexOwner)
    {
        _bDsmPresenceMutexTaken = FALSE;
        pRef->bDsmPresenceMutexOwner = FALSE;
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] Releasing Presence mutex\n", pRef));
    }

    // release ref
    DirtyMemFree(pRef, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    _DirtySessionManagerDestroyInternal

    \Description
        Destroy the module and release its state. To be used by NetConnXenon only. Game code should
        use DirtySessionManagerDestroy().

    \Input *pDestroyData    - reference to DSM state

    \Output
       int32_t              - 0 = success; -1 = try again; other negative=error

    \Notes
        This function returns -1 if internal state is not appropriate for proceeding with
        freeing resources. In that context, user needs to call DirtySessionManagerDestroyInternal()
        again later in time.

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerDestroyInternal(void *pDestroyData)
{
    DirtySessionManagerRefT *pRef = (DirtySessionManagerRefT *)pDestroyData;

    if (pRef->iNumQueuedOperations)
    {
        // At this point the idler is no more registered, so we cannot rely on it to call DirtySessionManagerUpdate() anymore.
        // We rather have to call it here and rely on NetConn to call DirtySessionManagerDestroy() until it succeeds.
        DirtySessionManagerUpdate(pRef);
        return(-1);
    }

    // proceed with destruction of dirty session manager reference
    _DirtySessionManagerDestroy(pRef);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerConnect

    \Description
        Connect to peers

    \Input *pRef        - pointer to module state
    \Input **pSessID    - array of pointers to session identifier string
    \Input *uSlot       - array of slots to use, i.e. DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT
    \Input uCount       - number of peers to connect to

    \Output
        int32_t     - zero=success, negative=failure

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerConnect(DirtySessionManagerRefT *pRef, const char **pSessID, uint32_t *uSlot, uint32_t uCount)
{
    BOOL bPrivate;
    char strTempBuffer[256];
    uint32_t uIndex;

    // session created?
    if ((pRef->bSessionCreationDone && !pRef->bSessionDeletionQueued) || pRef->bSessionCreationQueued)
    {
        DirtySessionManagerOperation Op;

        for(uIndex = 0; uIndex < uCount; uIndex++)
        {
            bPrivate = (uSlot[uIndex] & DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT) ? TRUE : FALSE;

            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] adding remote player %s in %s slot\n", pRef, pSessID[uIndex], bPrivate?"private":"public"));
            DirtySessionManagerEncodeSession(strTempBuffer, sizeof(strTempBuffer), &pRef->SessionInfo);
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session=%s (len=%d)\n", pRef, strTempBuffer, strlen(strTempBuffer)));

            Op.iFlags[uIndex] = bPrivate;
            DirtyAddrToHostAddr(&Op.PeerXuid[uIndex], sizeof(Op.PeerXuid[uIndex]), (DirtyAddrT *)pSessID[uIndex]);
        }

        Op.eOp = ST_SESSMGR_JOIN_REM;
        Op.iCount = uCount;

        _DirtySessionManagerQueueOp(pRef, &Op);

        return(0);
    }
    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] cannot connect with no session\n", pRef));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerDelete

    \Description
        Destroy a Xenon Live game session.

    \Input *pRef        - pointer to module state
    \Input bTerminal    - whether this should trigger the transition to a DSM terminal state

    \Output
        int32_t         - zero=success, negative=failure

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerDelete(DirtySessionManagerRefT *pRef, uint8_t bTerminal)
{
    DirtySessionManagerOperation Op;

    if (pRef->bTerminating || pRef->bTerminated)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] already terminating or terminated\n", pRef));
        return(-1);
    }

    if ((!pRef->bSessionCreationDone || pRef->bSessionDeletionQueued) && !pRef->bSessionCreationQueued)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] can't delete non-existent session\n", pRef));
        return(-1);
    }

    // stop the session first if we need to
    if (pRef->bSessionStartDone || (pRef->iSessionStartQueued > pRef->iSessionStopQueued))
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] stopping session before deleting it\n", pRef));

        DirtySessionManagerControl(pRef, 'stop', 0, 0, NULL);
    }

    Op.eOp = ST_SESSMGR_DELE;
    Op.iFlags[0] = (int32_t)bTerminal;
    _DirtySessionManagerQueueOp(pRef, &Op);

    pRef->bSessionCreationQueued = FALSE;
    pRef->bSessionDeletionQueued = TRUE;

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerStart

    \Description
        Start session

    \Input *pRef                  - pointer to module state
    \Input iNumArbitrationClients - number of arbitration clients

    \Output
        int32_t                   - zero=success, negative=failure

    \Version 01/21/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerStart(DirtySessionManagerRefT *pRef, int32_t iNumArbitrationClients)
{
    DirtySessionManagerOperation Op;
    int32_t iCurrent = pRef->bSessionStartDone ? 1 : 0;

    if (iCurrent + pRef->iSessionStartQueued - pRef->iSessionStopQueued  > 0)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session already started or session start already queued\n", pRef));
        return(-1);
    }

    // ranked?
    if (pRef->bRanked)
    {
        Op.iNumArbitrationClients = iNumArbitrationClients;
        Op.iNumArbitrationAttempts = (pRef->bHosting) ? 5 : 1;
        Op.dwQueriedSize = 0;

        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] max number of arbitration attempts set to %d\n", pRef, Op.iNumArbitrationAttempts));

        // set to immediately issue arbitration
        Op.iTimer = NetTick();

        Op.eOp = ST_SESSMGR_ARBI_SIZE;
        _DirtySessionManagerQueueOp(pRef, &Op);

        Op.eOp = ST_SESSMGR_ARBI;
        _DirtySessionManagerQueueOp(pRef, &Op);
    }

    Op.eOp = ST_SESSMGR_STRT;
    _DirtySessionManagerQueueOp(pRef, &Op);
    pRef->iSessionStartQueued += 1;

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerStop

    \Description
        Stop session

    \Input *pRef    - pointer to module state

    \Output
        int32_t     - zero=success, negative=failure

    \Version 01/21/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerStop(DirtySessionManagerRefT *pRef)
{
    DirtySessionManagerOperation Op;
    int32_t iCurrent = pRef->bSessionStartDone ? 1 : 0;

    if (iCurrent + pRef->iSessionStartQueued - pRef->iSessionStopQueued  < 1)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session not already started or session stop already queued\n", pRef));
        return(-1);
    }

    Op.eOp = ST_SESSMGR_STOP;
    _DirtySessionManagerQueueOp(pRef, &Op);
    pRef->iSessionStopQueued += 1;

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerWriteStats

    \Description
        Write statistics

    \Input *pRef        - pointer to module state
    \Input dwNumViews   - number of view pointed to by pViews
    \Input pViews       - pointer to array of views

    \Output
        int32_t         - zero=success, negative=failure

    \Version 01/25/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerWriteStats(DirtySessionManagerRefT *pRef, DWORD dwNumViews, XSESSION_VIEW_PROPERTIES *pViews)
{
    uint32_t uView;
    DirtySessionManagerOperation Op;

    // verify current local statistic buffer is big enough
    if (pRef->dwNumViews < dwNumViews)
    {
        // current buffer is too small, free and re-alloc

        if (pRef->pViews != NULL)
        {
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] freeing old statistic buffer (size=%d)\n", pRef, pRef->dwNumViews * sizeof(XSESSION_VIEW_PROPERTIES)));
            DirtyMemFree(pRef->pViews, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] allocating new statistic buffer (size=%d)\n", pRef, dwNumViews * sizeof(XSESSION_VIEW_PROPERTIES)));
        pRef->pViews = DirtyMemAlloc(dwNumViews * sizeof(XSESSION_VIEW_PROPERTIES), DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        pRef->dwNumViews = dwNumViews;
    }

    // copy statistics from user buffer to our buffer
    for (uView=0; uView < dwNumViews; uView++)
    {
        memcpy(&pRef->pViews[uView], &pViews[uView], sizeof(XSESSION_VIEW_PROPERTIES));
    }

    //todo: where to queue stats if more than one write stat is queued ??

    Op.eOp = ST_SESSMGR_STAT;
    _DirtySessionManagerQueueOp(pRef, &Op);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerMigrateHost

    \Description
        Migrate the host of a Xenon XSession.

    \Input *pRef            - pointer to module state
    \Input bLocalUserIsHost - TRUE if the local user is the new host
    \Input *pSession        - pointer to session string (client only)

    \Output
        int32_t             - zero=success, negative=failure

    \Version 01/26/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerMigrateHost(DirtySessionManagerRefT *pRef, int32_t bLocalUserIsHost, const char *pSession)
{
    DirtySessionManagerOperation Op;

    if (bLocalUserIsHost)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] migrating host - setting parameters to null\n", pRef));

        pRef->iSessionQueuedFlags = (pRef->iSessionQueuedFlags & (~XSESSION_CREATE_HOST)) | (bLocalUserIsHost ? XSESSION_CREATE_HOST : 0);

        Op.dwIndex[0] = pRef->iUserIndex;
        Op.eOp = ST_SESSMGR_MIGR_HOSTING;
        Op.iFlags[0] = pRef->iSessionQueuedFlags;

        _DirtySessionManagerQueueOp(pRef, &Op);

        pRef->bHosting = TRUE;
    }
    else
    {
        ds_strnzcpy(pRef->strSession, pSession, sizeof(pRef->strSession));
        DirtySessionManagerDecodeSession(&pRef->SessionInfo, pRef->strSession);

        pRef->iSessionQueuedFlags = (pRef->iSessionQueuedFlags & (~XSESSION_CREATE_HOST)) | (bLocalUserIsHost ? XSESSION_CREATE_HOST : 0);

        Op.eOp = ST_SESSMGR_MIGR_NOTHOSTING;
        Op.iFlags[0] = pRef->iSessionQueuedFlags;

        _DirtySessionManagerQueueOp(pRef, &Op);

        pRef->bHosting = FALSE;
    }

    // clearing or setting the HOST bit in the session flags after migration.

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerRemove

    \Description
        Remove peers

    \Input *pRef            - pointer to module state
    \Input **pDirtyAddr     - array of pointers to XUID to remove
    \Input uCount           - number of peers to remove

    \Output
        int32_t             - zero=success, negative=failure

    \Version 01/27/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerRemove(DirtySessionManagerRefT *pRef, const DirtyAddrT **pDirtyAddr, uint32_t uCount)
{
    uint32_t uIndex;
    DirtySessionManagerOperation Op;

    for(uIndex = 0; uIndex < uCount; uIndex++)
    {
        // get XUID to remove
        DirtyAddrToHostAddr(&Op.PeerXuid[uIndex], sizeof(Op.PeerXuid[uIndex]), pDirtyAddr[uIndex]);
    }

    // note - we can use LeaveRemote() for local and remote users - Remote() just means "use XUID instead of index" according to XDS
    Op.eOp = ST_SESSMGR_LEAV_REM;
    Op.iCount = uCount;

    _DirtySessionManagerQueueOp(pRef, &Op);

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerModify

    \Description
        Modify session attributes.

    \Input *pRef            - pointer to module state
    \Input iFlags           - flags
    \Input iMaxPublicSlots  - max public slots
    \Input iMaxPrivateSlots - max private slots

    \Output
        int32_t             - zero=success, negative=failure

    \Version 02/01/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerModify(DirtySessionManagerRefT *pRef, int32_t iFlags, int32_t iMaxPublicSlots, int32_t iMaxPrivateSlots)
{
    DirtySessionManagerOperation Op;
    int32_t iTmpFlags = iFlags;

    // make sure the passed in flags contain only modifiable flags
    iTmpFlags &= XSESSION_CREATE_MODIFIERS_MASK;

    /* make sure the flags we pass to XSessionModify contain the same flags we used to create
       the session, except for the flags we can change - it will fail to work otherwise */
    iTmpFlags = iTmpFlags | (pRef->iSessionCreationFlags & ~XSESSION_CREATE_MODIFIERS_MASK);

    iTmpFlags = _HandleSessionFlags(pRef, iTmpFlags);

    // in XSessionCreate() we always include PEER_NETWORK, so we need to do the same here
    iTmpFlags |= XSESSION_CREATE_USES_PEER_NETWORK;

    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session modify (flags=0x%08x, pub=%d, prv=%d)\n",
        pRef, iTmpFlags, iMaxPublicSlots, iMaxPrivateSlots));

    Op.eOp = ST_SESSMGR_MODI;
    Op.iFlags[0] = iTmpFlags;
    Op.iMaxPublicSlots = iMaxPublicSlots;
    Op.iMaxPrivateSlots = iMaxPrivateSlots;

    _DirtySessionManagerQueueOp(pRef, &Op);

    pRef->iSessionCreationFlags = iTmpFlags;
    pRef->iMaxPublicSlots = iMaxPublicSlots;
    pRef->iMaxPrivateSlots = iMaxPrivateSlots;

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerSetFlags

    \Description
        Modify session flags

    \Input *pRef    - pointer to module state
    \Input iFlags   - flags

    \Output
        int32_t     - zero=success, negative=failure

    \Version 10/05/2009 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerSetFlags(DirtySessionManagerRefT *pRef, int32_t iFlags)
{
    return(_DirtySessionManagerModify(pRef, iFlags, pRef->iMaxPublicSlots, pRef->iMaxPrivateSlots));
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerSetSlots

    \Description
        Modify session slots

    \Input *pRef            - pointer to module state
    \Input iMaxPublicSlots  - max public slots
    \Input iMaxPrivateSlots - max private slots

    \Output
        int32_t             - zero=success, negative=failure

    \Version 10/05/2009 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerSetSlots(DirtySessionManagerRefT *pRef, int32_t iMaxPublicSlots, int32_t iMaxPrivateSlots)
{
    return(_DirtySessionManagerModify(pRef, pRef->iSessionQueuedFlags, iMaxPublicSlots, iMaxPrivateSlots));
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerInvite

    \Description
        Invite peer

    \Input *pRef        - pointer to module state
    \Input *pDirtyAddr  - pointer to XUID to invite

    \Output
        int32_t         - zero=success, negative=failure

    \Version 02/01/2011 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerInvite(DirtySessionManagerRefT *pRef, const DirtyAddrT *pDirtyAddr)
{
    DirtySessionManagerOperation Op;

    if (pRef->bHosting)
    {
        static XUID InviteXuid;

        // convert input string-form to xuid
        DirtyAddrToHostAddr(&InviteXuid, sizeof(InviteXuid), pDirtyAddr);

        Op.eOp = ST_SESSMGR_INVT;
        Op.iCount = 1;

        //todo: MLU
        Op.PeerXuid[0] = InviteXuid;
        Op.dwIndex[0] = pRef->iUserIndex;

        _DirtySessionManagerQueueOp(pRef, &Op);
    }
    else
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] can't invite when not hosting\n", pRef));
    }

    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtySessionManagerIsSessionCleanable

    \Description
        This function shall be used to find out whether it is safe to call XSessionDelete().
        When unsafe, calling XSessionDelete() can lead to some system RIPs.

    \Input *pRef        - pointer to module state

    \Output
        int8_t          - TRUE = sessoin is cleanable; FALSE=session is not cleanable

    \Notes
        This function was added according to guidelines received directly from Microsoft Game Developer support.
        The guidelines summarize to:

            Note1: When a session host signs out explicitly from XBL, or implicitly as a result of
                   an upstream disconnection, the associated session is deleted "behind the scene" by
                   the system code and the game code shall not try to delete it again.

            Note2: In the specific case of an ethernet cable pull (link down event), the session is not
                   deleted "behind the scene" by the system and the game code is expected to call
                   XSessionDelete() explicitly.

    \Version 14/01/2013 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _DirtySessionManagerIsSessionCleanable(DirtySessionManagerRefT *pRef)
{
    uint8_t bCleanable = TRUE;  // default to TRUE

    // consider skipping session deletion only if we are the platform host (aka session owner or session host)
    if (pRef->bHosting)
    {
        // consider skipping session deletion only if ethernet cable is connected (link up)
        if(NetConnStatus('plug', 0, NULL, 0))
        {
            // skip session deletion only if user is no longer signed in with XBL
            if (XUserGetSigninState(pRef->iUserIndex) != eXUserSigninState_SignedInToLive)
            {
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session not cleanable because session owner no longer signed in with XBL -- signin state != SignedInToLive\n", pRef));
                bCleanable = FALSE;
            }
            else
            {
                /*
                skip session deletion if XUID changed for user index associated with this session (which
                would mean that former user is no longer signed with XBL)
                note: XUserGetXUID() returns different value depending on the gamer being signed in or not
                      with XBL. Because we have just checked that user is signed in, it is safe to compare XUIDs.
                */
                XUID Xuid;
                XUserGetXUID(pRef->iUserIndex, &Xuid);
                if(!IsEqualXUID(Xuid, pRef->LocalXuid[pRef->iUserIndex]))
                {
                    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session not cleanable because session owner no longer signed in with XBL -- xuid changed at user index %d\n", pRef, pRef->iUserIndex));
                    bCleanable = FALSE;
                }
            }
        }
    }

    return(bCleanable);
}

/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function DirtySessionManagerCreateSess

    \Description
        Create a Xenon Live game session.

    \Input *pRef        - pointer to module state
    \Input bRanked      - TRUE if ranked, else FALSE
    \Input uUserFlags   - User flags such as slot type, DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT | DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT
    \Input *pSession    - NULL if creating new session, pointer to session string if joining
    \Input *pLocalAddrs - Array of local address to join-in. Controller-index based. 0 if player is not joining

    \Output
        int32_t         - zero=success, negative=failure

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
int32_t DirtySessionManagerCreateSess(DirtySessionManagerRefT *pRef, uint32_t bRanked, uint32_t *uUserFlags, const char *pSession, DirtyAddrT *pLocalAddrs)
{
    int32_t iFlagsUsed;

    // if we already have a session, return an error
    if ((pRef->bSessionCreationDone && !pRef->bSessionDeletionQueued) || pRef->bSessionCreationQueued)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] attempt to create session when session already exists\n", pRef));
        return(-1);
    }

    pRef->bRanked = bRanked;

    //todo:review
//    for(iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
//    {
//        pRef->bLocalSlotTypePrivate[iUserIndex] = (uUserFlags[iUserIndex] & DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT) ? TRUE : FALSE;
//    }
    

    if (pRef->bRanked)
    {
        pRef->iSessionCreationFlags |= XSESSION_CREATE_USES_ARBITRATION;
    }

    // determine if we are hosting the 'session' or not
    if (pSession == NULL)
    {
        pRef->iSessionCreationFlags |= XSESSION_CREATE_HOST;
        pRef->bHosting = TRUE;
    }
    else 
    {
        pRef->bHosting = FALSE;
        ds_strnzcpy(pRef->strSession, pSession, sizeof(pRef->strSession));
        DirtySessionManagerDecodeSession((void*)&pRef->SessionInfo, pRef->strSession);
    }

    pRef->iSessionCreationFlags = _HandleSessionFlags(pRef, pRef->iSessionCreationFlags);

    // set up flags
    pRef->iSessionQueuedFlags = pRef->iSessionCreationFlags;

    // if no user is selected just pick one
    if (pRef->iUserIndex == -1)
    {
        pRef->iUserIndex = NetConnQuery(NULL, NULL, 0);
    }

    if (pRef->iUserIndex != -1)
    {
        DirtySessionManagerOperation Op;
        int32_t iUserIndex;
        // we lie to other layers, in DS, as they expect absence of XSESSION_CREATE_USES_PEER_NETWORK for some sessions
        iFlagsUsed = pRef->iSessionQueuedFlags|XSESSION_CREATE_USES_PEER_NETWORK;

        // create the session
        NetPrintf(("dirtysessionmanagerxenon: creating session with flags 0x%08x\n", iFlagsUsed));

        Op.eOp = ST_SESSMGR_CREA;
        for (iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
        {
            if (pLocalAddrs[iUserIndex].strMachineAddr[0])
            {
                DirtyAddrToHostAddr(&Op.PeerXuid[iUserIndex], sizeof(Op.PeerXuid[iUserIndex]), &pLocalAddrs[iUserIndex]);
            }
            else
            {
                Op.PeerXuid[iUserIndex] = 0;
            }
        }

        Op.iFlags[0] = iFlagsUsed;
        Op.dwIndex[0] = pRef->iUserIndex;
        Op.iMaxPublicSlots = pRef->iMaxPublicSlots;
        Op.iMaxPrivateSlots = pRef->iMaxPrivateSlots;
        Op.uGameMode = pRef->iGameMode;
        Op.uGameType = bRanked ? X_CONTEXT_GAME_TYPE_RANKED : X_CONTEXT_GAME_TYPE_STANDARD;

        _DirtySessionManagerQueueOp(pRef, &Op);

        pRef->bSessionCreationQueued = TRUE;
        pRef->bSessionDeletionQueued = FALSE;
    }
    else
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] no valid user index set for session creation\n", pRef));
        _DirtySessionManagerTerminate(pRef);
    }

    return((pRef->bTerminating || pRef->bTerminated) ? -1 : 0);
}


/*F*************************************************************************************************/
/*!
    \Function DirtySessionManagerEncodeSession

    \Description
        Encode session variables (host address, session key, and session ID) into a binary7 string.

    \Input *pBuffer         - [out] pointer to buffer for session string
    \Input iBufSize         - size of output buffer
    \Input *pSessionInfo0   - pointer to session info to encode

    \Version 05/12/2005 (jbrookes)
*/
/*************************************************************************************************F*/
void DirtySessionManagerEncodeSession(char *pBuffer, int32_t iBufSize, const void *pSessionInfo0)
{
    const XSESSION_INFO *pSessionInfo = pSessionInfo0;

    char *pBufEnd = pBuffer + iBufSize;
    #if DIRTYCODE_LOGGING
    char *pBufSta = pBuffer;
    #endif

    // echo info being encoded
    NetPrintMem(&pSessionInfo->hostAddress, sizeof(pSessionInfo->hostAddress), "hostAddress");
    NetPrintMem(&pSessionInfo->keyExchangeKey, sizeof(pSessionInfo->keyExchangeKey), "keyExchangeKey");
    NetPrintMem(&pSessionInfo->sessionID, sizeof(pSessionInfo->sessionID), "sessionID");

    // encode session information
    *pBuffer++ = '$';
    pBuffer = Binary7Encode((uint8_t *)pBuffer, pBufEnd-pBuffer, (const uint8_t *)&pSessionInfo->hostAddress, sizeof(pSessionInfo->hostAddress), FALSE);
    pBuffer = Binary7Encode((uint8_t *)pBuffer, pBufEnd-pBuffer, (const uint8_t *)&pSessionInfo->keyExchangeKey, sizeof(pSessionInfo->keyExchangeKey), FALSE);
    pBuffer = Binary7Encode((uint8_t *)pBuffer, pBufEnd-pBuffer, (const uint8_t *)&pSessionInfo->sessionID, sizeof(pSessionInfo->sessionID), TRUE);

    // echo encoded string
    NetPrintMem(pBufSta, strlen(pBufSta), "strSess");
}

/*F*************************************************************************************************/
/*!
    \Function DirtySessionManagerDecodeSession

    \Description
        Decode session variables (host address, session key, and session ID) from a binary7 string.

    \Input *pSessionInfo0   - [out] pointer to buffer to store decoded session info
    \Input *pBuffer         - pointer to session string

    \Version 05/12/2005 (jbrookes)
*/
/*************************************************************************************************F*/
void DirtySessionManagerDecodeSession(void *pSessionInfo0, const char *pBuffer)
{
    XSESSION_INFO *pSessionInfo = pSessionInfo0;

    pBuffer += 1;
    pBuffer = (const char *)Binary7Decode((uint8_t *)&pSessionInfo->hostAddress, sizeof(pSessionInfo->hostAddress), (uint8_t *)pBuffer);
    pBuffer = (const char *)Binary7Decode((uint8_t *)&pSessionInfo->keyExchangeKey, sizeof(pSessionInfo->keyExchangeKey), (uint8_t *)pBuffer);
    pBuffer = (const char *)Binary7Decode((uint8_t *)&pSessionInfo->sessionID, sizeof(pSessionInfo->sessionID), (uint8_t *)pBuffer);

    // echo info after decode
    NetPrintMem(&pSessionInfo->hostAddress, sizeof(pSessionInfo->hostAddress), "hostAddress");
    NetPrintMem(&pSessionInfo->keyExchangeKey, sizeof(pSessionInfo->keyExchangeKey), "keyExchangeKey");
    NetPrintMem(&pSessionInfo->sessionID, sizeof(pSessionInfo->sessionID), "sessionID");
}

/*F*************************************************************************************************/
/*!
    \Function    DirtySessionManagerUpdate

    \Description
        Update module state

    \Input *pRef    - module state

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
void DirtySessionManagerUpdate(DirtySessionManagerRefT *pRef)
{
    uint8_t bOperationCompletedImmediately = FALSE;
    int32_t iIndex;
    uint32_t uNow = NetTick();

    if (!pRef->bOpInProgress && (pRef->iNumQueuedOperations > 0))
    {
        DirtySessionManagerOperation* pOp = &pRef->QueuedOperations[0];

        // this Operation can wait to be actually started
        if ((pOp->eOp != ST_SESSMGR_ARBI) &&
            (pOp->eOp != ST_SESSMGR_CREA))
        {
            pOp->uStartTime = uNow;
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] starting operation %s\n", pRef, _DsmStateNames[pOp->eOp]));
        }

        memset(&pRef->XOverlapped, 0, sizeof(pRef->XOverlapped));

        switch(pOp->eOp)
        {
        case ST_SESSMGR_CREA:

            //take presence if needed (break out otherwise)
            if (pOp->iFlags[0] & XSESSION_CREATE_USES_PRESENCE)
            {
                // if someone else has presence and we're not terminating, let's wait
                if (_bDsmPresenceMutexTaken && !pRef->bTerminating)
                {
                    return;
                }

                _bDsmPresenceMutexTaken = TRUE;
                pRef->bDsmPresenceMutexOwner = TRUE;
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] Taking Presence mutex\n", pRef));
            }

            pOp->uStartTime = uNow;
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] starting operation %s\n", pRef, _DsmStateNames[pOp->eOp]));

            // set game type and mode for all users 
            for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
            {
                if (pOp->PeerXuid[iIndex])
                {
                    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] setting gametype=%d and gamemode=%d for local user %d\n", pRef, pOp->uGameType, pOp->uGameMode, iIndex));
                    XUserSetContext(iIndex, X_CONTEXT_GAME_TYPE, pOp->uGameType);
                    XUserSetContext(iIndex, X_CONTEXT_GAME_MODE, pOp->uGameMode);
                }
            }

            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] calling XSessionCreate(0x%08x, %d, %d, %d, [...]) ... \n",
                pRef, pOp->iFlags[0], pOp->dwIndex[0], pOp->iMaxPublicSlots, pOp->iMaxPrivateSlots));

            pOp->dwResult = XSessionCreate(pOp->iFlags[0], pOp->dwIndex[0], pOp->iMaxPublicSlots, pOp->iMaxPrivateSlots,
                &pRef->qwSessionNonce, &pRef->SessionInfo, &pRef->XOverlapped, &pRef->hSession);
            break;

        case ST_SESSMGR_JOIN_REM:
            if (pOp->iCount > DIRTYSESSIONMANAGER_MAX_GAMER_COUNT)
            {
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] warning... incorrect gamer count (%d) used with XSessionJoinRemote()!\n", pRef, pOp->iCount));
                pOp->iCount = DIRTYSESSIONMANAGER_MAX_GAMER_COUNT;
            }
            pOp->dwResult = XSessionJoinRemote(pRef->hSession, pOp->iCount, pOp->PeerXuid, pOp->iFlags, &pRef->XOverlapped);

            for(iIndex = 0; iIndex < pOp->iCount; iIndex++)
            {
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] XSessionJoinRemote %I64d\n", pRef, pOp->PeerXuid[iIndex]));
            }

            break;

        case ST_SESSMGR_ARBI_SIZE:
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] XSessionArbitrationRegister(getsize) using nonce (low 32 bits) %d\n", 
                pRef, (uint32_t)pRef->qwSessionNonce));

            // query result size
            pOp->dwResult = XSessionArbitrationRegister(pRef->hSession, 0, pRef->qwSessionNonce, &pOp->dwQueriedSize, NULL, &pRef->XOverlapped);
            break;

        case ST_SESSMGR_ARBI:
            if (NetTickDiff(uNow, pOp->iTimer) > 0)
            {
                pOp->uStartTime = uNow;
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] starting operation %s\n", pRef, _DsmStateNames[pOp->eOp]));
                pOp->dwResult = XSessionArbitrationRegister(pRef->hSession, 0, pRef->qwSessionNonce, &pRef->dwRegistrationSize, pRef->pRegistrationResults, &pRef->XOverlapped);
            }
            else
            {
                // effectively wait until the right time to submit the arbitration request
                return;
            }
            break;

        case ST_SESSMGR_DELE:
            // first check whether we can safely call XSessionDelete()
            if (_DirtySessionManagerIsSessionCleanable(pRef))
            {
                pOp->dwResult = XSessionDelete(pRef->hSession, &pRef->XOverlapped);
            }
            else
            {
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] skipping XSessionDelete() as instructed by MS (faking a successful XSessionDelete())\n", pRef));
                pOp->dwResult = ERROR_SUCCESS;
            }
            break;

        case ST_SESSMGR_LEAV_REM:
            if (pOp->iCount > DIRTYSESSIONMANAGER_MAX_GAMER_COUNT)
            {
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] warning... incorrect gamer count (%d) used with XSessionLeaveRemote()!\n",
                    pRef, pOp->iCount));
                pOp->iCount = DIRTYSESSIONMANAGER_MAX_GAMER_COUNT;
            }
            pOp->dwResult = XSessionLeaveRemote(pRef->hSession, pOp->iCount, pOp->PeerXuid, &pRef->XOverlapped);

            for(iIndex = 0; iIndex < pOp->iCount; iIndex++)
            {
                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] XSessionLeaveRemote %I64d\n", pRef, pOp->PeerXuid[iIndex]));
            }

            break;

        case ST_SESSMGR_MIGR_HOSTING:
            // clear session buffer
            memset(&pRef->SessionInfo, 0, sizeof(pRef->SessionInfo));
            pOp->dwResult = XSessionMigrateHost(pRef->hSession, pOp->dwIndex[0], &pRef->SessionInfo, &pRef->XOverlapped);
            break;

        case ST_SESSMGR_MIGR_NOTHOSTING:
            pOp->dwResult = XSessionMigrateHost(pRef->hSession, XUSER_INDEX_NONE, &pRef->SessionInfo, &pRef->XOverlapped);
            break;

        case ST_SESSMGR_STAT:
            pOp->dwResult = XSessionWriteStats(pRef->hSession, pRef->StatXuid, pRef->dwNumViews, pRef->pViews, &pRef->XOverlapped);
            break;

        case ST_SESSMGR_STRT:
            pOp->dwResult = XSessionStart(pRef->hSession, 0, &pRef->XOverlapped);
            break;

        case ST_SESSMGR_STOP:
            pOp->dwResult = XSessionEnd(pRef->hSession, &pRef->XOverlapped);
            break;

        case ST_SESSMGR_MODI:       
            pOp->dwResult = XSessionModify(pRef->hSession, pOp->iFlags[0], pOp->iMaxPublicSlots, pOp->iMaxPrivateSlots, &pRef->XOverlapped);
            pRef->iMaxPublicSlots = pOp->iMaxPublicSlots;
            pRef->iMaxPrivateSlots = pOp->iMaxPrivateSlots;
            //todo: check how the two above are queued and used.

            break;

        case ST_SESSMGR_INVT:
            pOp->dwResult = XInviteSend(pOp->dwIndex[0], 1, &(pOp->PeerXuid[0]), (const WCHAR *)pRef->strInviteText, &pRef->XOverlapped);
            break;

        default:
            NetPrintf(("dirtysessionmanagerxenon: Terminating, unknown operation %d\n", pOp->eOp));
            _DirtySessionManagerTerminate(pRef);

            // skip the normal handling below.
            return;
        }

        if (((pOp->dwResult == ERROR_INSUFFICIENT_BUFFER) && (pOp->eOp == ST_SESSMGR_ARBI_SIZE)) ||
            ((pOp->dwResult == ERROR_SUCCESS) && (pOp->eOp == ST_SESSMGR_DELE)))
        {
            bOperationCompletedImmediately = TRUE;
        }
        else if (pOp->dwResult != ERROR_IO_PENDING)
        {
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] operation %s error returned 0x%x (%s)\n", pRef, _DsmStateNames[pOp->eOp], pOp->dwResult, DirtyErrGetName(pOp->dwResult)));
            _DirtySessionManagerTerminate(pRef);
        }
        else
        {
            pRef->bOpInProgress = TRUE;
        }
    }

    // if we have something in progress
    if ((pRef->bOpInProgress && (pRef->iNumQueuedOperations > 0)) || bOperationCompletedImmediately)
    {
        DirtySessionManagerOperation Op = pRef->QueuedOperations[0];

        // that just completed
        if (bOperationCompletedImmediately || XHasOverlappedIoCompleted(&pRef->XOverlapped))
        {
            if (!bOperationCompletedImmediately)
            {
                XGetOverlappedResult(&pRef->XOverlapped, &Op.dwResult, FALSE);
            }

            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] operation %s ended with code 0x%x (%s)\n", pRef, _DsmStateNames[Op.eOp], Op.dwResult, DirtyErrGetName(Op.dwResult)));
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] operation was queued %d ms and executed in %d ms (new queue count=%d)\n", pRef, NetTickDiff(Op.uStartTime, Op.uQueueTime), NetTickDiff(uNow, Op.uStartTime), (pRef->iNumQueuedOperations-1)));

            memmove(&pRef->QueuedOperations[0], &pRef->QueuedOperations[1], (pRef->iNumQueuedOperations - 1) * sizeof(pRef->QueuedOperations[0]));
            pRef->iNumQueuedOperations -= 1;

            pRef->bOpInProgress = FALSE;

            // succesfully
            if ((Op.dwResult == ERROR_SUCCESS) || 
                (Op.dwResult == ERROR_INSUFFICIENT_BUFFER) && (Op.eOp == ST_SESSMGR_ARBI_SIZE))
            {
                // deal with it, if we're not in termination state
                if ((Op.eOp == ST_SESSMGR_CREA) && (!pRef->bTerminating))
                {

                    pRef->bSessionCreationDone = TRUE;
                    pRef->bSessionCreationQueued = FALSE;

                    if (pRef->bHosting)
                    {
                        DirtySessionManagerEncodeSession(pRef->strSession, sizeof(pRef->strSession), &pRef->SessionInfo);
                        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session=%s (len=%d)\n", pRef, pRef->strSession, strlen(pRef->strSession)));
                    }
                }
                else if ((Op.eOp == ST_SESSMGR_ARBI_SIZE) && (!pRef->bTerminating))
                {
                    if (Op.dwQueriedSize > pRef->dwRegistrationSize)
                    {
                        if (pRef->pRegistrationResults != NULL)
                        {
                            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] freeing old arbitration result buffer (size=%d)\n", pRef, pRef->dwRegistrationSize));
                            DirtyMemFree(pRef->pRegistrationResults, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
                        }
                        
                        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] allocating new arbitration result buffer (size=%d)\n", pRef, Op.dwQueriedSize));
                        pRef->pRegistrationResults = DirtyMemAlloc(Op.dwQueriedSize, DIRTYSESSMGR_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
                        pRef->dwRegistrationSize = Op.dwQueriedSize;
                    }
                }
                else if ((Op.eOp == ST_SESSMGR_ARBI) && (!pRef->bTerminating))
                {
                    #if DIRTYCODE_LOGGING
                    XSESSION_REGISTRANT *pClient;
                    DirtyAddrT DirtyAddr;
                    int32_t iClient, iUser;

                    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] wNumRegistrants=%d\n", pRef, pRef->pRegistrationResults->wNumRegistrants));
                    for (iClient = 0; iClient < (signed) pRef->pRegistrationResults->wNumRegistrants; iClient++)
                    {
                        pClient = &pRef->pRegistrationResults->rgRegistrants[iClient];

                        DirtyAddrFromHostAddr(&DirtyAddr, &pClient->qwMachineID);
                        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] client %d: nusers=%d, trust=%d, machineid=%s\n", pRef, iClient,
                            pClient->bNumUsers, pClient->bTrustworthiness, DirtyAddr.strMachineAddr));

                        for (iUser = 0; iUser < (signed)pClient->bNumUsers; iUser++)
                        {
                            DirtyAddrFromHostAddr(&DirtyAddr, &pClient->rgUsers[iUser]);
                            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] user %d) %s\n", pRef, iUser, DirtyAddr.strMachineAddr));
                        }
                    }
                    #endif

                    if (pRef->bHosting)
                    {
                        // if everyone isn't registered, try again
                        if (( pRef->pRegistrationResults->wNumRegistrants < (unsigned)Op.iNumArbitrationClients) && (Op.iNumArbitrationAttempts > 0))
                        {
                            DirtySessionManagerOperation OpNext = Op;

                            OpNext.iTimer = uNow + DIRTYSESSIONMANAGER_ARBITRATIONTIMER;
                            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] host attempting another arbitration (expected # of reg = %d; current # of reg = %d)\n",
                                pRef, OpNext.iNumArbitrationClients, pRef->pRegistrationResults->wNumRegistrants));

                            _DirtySessionManagerQueueOpAtFront(pRef, &OpNext);
                        }
                    }
                }
                else if ((Op.eOp == ST_SESSMGR_MIGR_HOSTING) && (!pRef->bTerminating))
                {
                    pRef->bHosting = TRUE;
                    DirtySessionManagerEncodeSession(pRef->strSession, sizeof(pRef->strSession), &pRef->SessionInfo);
                    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] session=%s (len=%d)\n", pRef, pRef->strSession, strlen(pRef->strSession)));
                }
                else if ((Op.eOp == ST_SESSMGR_MIGR_NOTHOSTING) && (!pRef->bTerminating))
                {
                    pRef->bHosting = FALSE;
                }
            }

            // the 'strt' operation success code is done even if start failed, to properly keep track of number of start/stop
            if (Op.eOp == ST_SESSMGR_STRT)
            {
                pRef->iSessionStartQueued -= 1;
                pRef->bSessionStartDone = TRUE;
            }
            else if (Op.eOp == ST_SESSMGR_STOP)
            {
                pRef->iSessionStopQueued -= 1;
                pRef->bSessionStartDone = FALSE;
            }
            // the 'dele' operation success code is done even if deletion failed, as there really isn't anyway around
            // a deletion failure.
            else if (Op.eOp == ST_SESSMGR_DELE)
            {
                pRef->bSessionCreationDone = FALSE;
                pRef->bSessionDeletionQueued = FALSE;

                NetPrintf(("dirtysessionmanagerxenon: [0x%08x] closing session handle\n", pRef));

                XCloseHandle(pRef->hSession);
                pRef->hSession = 0;

                if (Op.iFlags[0])
                {
                    pRef->bTerminated = TRUE;
                }

                if (pRef->bDsmPresenceMutexOwner)
                {
                    _bDsmPresenceMutexTaken = FALSE;
                    pRef->bDsmPresenceMutexOwner = FALSE;
                    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] Releasing Presence mutex\n", pRef));
                }
            }
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    DirtySessionManagerControl

    \Description
        DirtySessionManager control function.

    \Input *pRef    - module ref
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - 0=success; negative=error

    \Notes
        iControl can be one of the following:

        \verbatim
            'dele' - iValue of 1 means terminal.
            'pres' - add/remove presence to/from session creation flags
        \endverbatim

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
int32_t DirtySessionManagerControl(DirtySessionManagerRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] DirtySessionManagerControl('%c%c%c%c'), (%d, %d)\n", (uintptr_t)pRef,
            (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)(iControl), iValue, iValue2));

    //removes ambiguitiy from the status 'sess' call. Allows a caller to say: "when I ask about the 'sess', I mean the next new one".
    if (iControl == 'clrs')
    {
        pRef->strSession[0] = 0;
        return(0);
    }

    if (iControl == 'dele')
    {
        return(_DirtySessionManagerDelete(pRef, iValue));
    }

    if (iControl == 'jinp')
    {
        pRef->uJoinInProgress = iValue;
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] uJoinInProgress is now %d\n", pRef, pRef->uJoinInProgress));
        return(0);
    }

    // set invite text
    if (iControl == 'itxt')
    {
        Utf8DecodeToUCS2(pRef->strInviteText, sizeof(pRef->strInviteText)/sizeof(WCHAR), (const char *)pValue);
        return(0);
    }

    // migrate the host
    if (iControl == 'migr')
    {
        return(_DirtySessionManagerMigrateHost(pRef, iValue, (const char *)pValue));
    }

    if (iControl == 'nonc')
    {
        Binary7Decode((uint8_t *)&pRef->qwSessionNonce, sizeof(pRef->qwSessionNonce), (uint8_t *)pValue);
        NetPrintMem(&pRef->qwSessionNonce, sizeof(pRef->qwSessionNonce), "sessionNonce");
        return(0);
    }

    // invite a user into session
    if (iControl == 'invt')
    {
        return(_DirtySessionManagerInvite(pRef, (const DirtyAddrT *)pValue));
    }

    if (iControl == 'pinv')
    {
        pRef->bAllowPresenceInvites = (iValue ? TRUE : FALSE);
        return(0);
    }

    if (iControl == 'pjoi')
    {
        pRef->bAllowPresenceJoins = (iValue ? TRUE : FALSE);
        return(0);
    }

    // add/remove presence to/from session creation flags
    if (iControl == 'pres')
    {
        if (iValue)
        {
            pRef->iSessionCreationFlags |= XSESSION_CREATE_USES_PRESENCE;
        }
        else
        {
            pRef->iSessionCreationFlags &= ~XSESSION_CREATE_USES_PRESENCE;
        }

        pRef->iSessionCreationFlags = _HandleSessionFlags(pRef, pRef->iSessionCreationFlags);

        return(0);
    }

    if (iControl == 'remv')
    {
        const DirtyAddrT *pAddresses[4];
        int32_t iIndex;

        for(iIndex = 0; iIndex < iValue; iIndex++)
        {
            pAddresses[iIndex] = ((DirtyAddrT**)pValue)[iIndex];
        }

        //todo : review the passing of number of removal
        return(_DirtySessionManagerRemove(pRef, pValue, iValue));
    }

    // set local xuid
    if (iControl == 'self')
    {
        int32_t iUserIndex;
        XUID LocalXuid;
        DirtyAddrToHostAddr(&LocalXuid, sizeof(LocalXuid), (DirtyAddrT *)pValue);
        for (iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
        {
            if (pRef->LocalXuid[iUserIndex] == LocalXuid)
            {
                pRef->iUserIndex = iUserIndex;
                break;
            }
        }
        
        return(0);
    }

    if (iControl == 'sflg')
    {
        if ((!pRef->bSessionCreationDone || pRef->bSessionDeletionQueued) && !pRef->bSessionCreationQueued)
        {
            // dealing with MS
            // RIP: XSessionCreate(): Invalid flags: Modifiers can only be used with presence or matchmaking Sessions
            if ((iValue & (XSESSION_CREATE_USES_PRESENCE | XSESSION_CREATE_USES_MATCHMAKING)) == 0)
            {
                iValue &= (~XSESSION_CREATE_MODIFIERS_MASK);
            }

            if (pRef->uJoinInProgress)
            {
                iValue &= (~XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED);
            }
            else
            {
                iValue |= XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;
            }

            iValue = _HandleSessionFlags(pRef, iValue);

            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] setting session base flags to 0x%08x\n", pRef, iValue));
            pRef->iSessionCreationFlags = iValue; 
            return(0);
        }
        else
        {
            return(_DirtySessionManagerSetFlags(pRef, iValue));
        }
    }

    if (iControl == 'slot')
    {
        if ((!pRef->bSessionCreationDone || pRef->bSessionDeletionQueued) && !pRef->bSessionCreationQueued)
        {
            NetPrintf(("dirtysessionmanagerxenon: [0x%08x] setting slot count (pub=%d, prv=%d)\n", pRef, iValue, iValue2));
            pRef->iMaxPublicSlots = iValue;
            pRef->iMaxPrivateSlots = iValue2;
            return(0);
        }
        else
        {
            return(_DirtySessionManagerSetSlots(pRef, iValue, iValue2));
        }
    }

    // write to skill leaderboard
    if (iControl == 'skil')
    {
#if DIRTYCODE_LOGGING
        char strTemp[64];
#endif

        // format stats
        pRef->ViewSkills[0].dwPropertyId = X_PROPERTY_RELATIVE_SCORE;
        pRef->ViewSkills[0].value.nData  = iValue2;
        pRef->ViewSkills[0].value.type   = XUSER_DATA_TYPE_INT32;

        pRef->ViewSkills[1].dwPropertyId = X_PROPERTY_SESSION_TEAM;
        pRef->ViewSkills[1].value.nData  = iValue;
        pRef->ViewSkills[1].value.type   = XUSER_DATA_TYPE_INT32;

        pRef->ViewSkills[2].dwPropertyId = X_PROPERTY_PLAYER_SKILL_UPDATE_WEIGHTING_FACTOR;
        pRef->ViewSkills[2].value.nData  = pRef->iRankWeight;
        pRef->ViewSkills[2].value.type   = XUSER_DATA_TYPE_INT32;

        pRef->ViewProperties[0].dwNumProperties = 3;
        pRef->ViewProperties[0].dwViewId        = X_STATS_VIEW_SKILL;
        pRef->ViewProperties[0].pProperties     = pRef->ViewSkills;

        // fall through to 'stat' to write stats
#if DIRTYCODE_LOGGING
        ds_snzprintf(strTemp, sizeof(strTemp), "%I64d", pRef->StatXuid);
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] skill leaderboard: user %s, teamid %d, score %d, weight %d\n", pRef, strTemp, iValue, iValue2, pRef->iRankWeight));
#endif
        pValue = (void *)&pRef->ViewProperties;
        iValue = sizeof(pRef->ViewProperties) / sizeof(pRef->ViewProperties[0]);
        iControl = 'stat';
    }
    if (iControl == 'stat')
    {
        return(_DirtySessionManagerWriteStats(pRef, iValue, (XSESSION_VIEW_PROPERTIES *)pValue));
    }

    // start session
    if (iControl == 'strt')
    {
        return(_DirtySessionManagerStart(pRef, iValue));
    }

    // stop session
    if (iControl == 'stop')
    {
        return(_DirtySessionManagerStop(pRef));
    }

    if (iControl == 'suid')
    {
        DirtyAddrToHostAddr(&pRef->StatXuid, sizeof(pRef->StatXuid), pValue);
        return(0);
    }

    // control selectors that are deprecated or don't apply to this platform
    if ((iControl == 'upnp') || (iControl == 'mngl') || (iControl == 'dsrv') || (iControl == 'host'))
    {
        return(0);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtySessionManagerStatus

    \Description
        Return module status based on input selector.

    \Input *pRef    - module ref
    \Input iSelect  - status selector
    \Input *pBuf    - user-provided buffer
    \Input iBufSize - size of user-provided buffer

    \Notes
        iSelect can be one of the following:

        \verbatim
            'sess' - copies session information into output buffer
            'pres' - returns whether sessions creation flags have the presence flag set
        \endverbatim

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
int32_t DirtySessionManagerStatus(DirtySessionManagerRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    // are we in a terminal state?
    if (iSelect == 'fail')
    {
        return(pRef->bTerminated);
    }

    if (iSelect == 'host')
    {
        //todo: check how this behaves when a migration is queued.
        return(pRef->bHosting);
    }

    if (iSelect == 'idle')
    {
        return(!pRef->bOpInProgress);
    }

    // is the presence flag set in the session creation flag
    if (iSelect == 'pres')
    {
        return( ((pRef->iSessionCreationFlags & XSESSION_CREATE_USES_PRESENCE) ? TRUE : FALSE) );
    }

    if ((iSelect == 'sess') || (iSelect == 'nonc'))
    {
        if (!pRef->bSessionCreationDone || (pRef->strSession[0] == 0))
        {
            return(0);
        }
        else if (pRef->strSession[0] == '$')
        {
            if (iSelect == 'sess')
            {
                ds_strnzcpy((char *)pBuf, pRef->strSession, iBufSize);
            }
            else
            {
                NetPrintMem(&pRef->qwSessionNonce, sizeof(pRef->qwSessionNonce), "sessionNonce");
                Binary7Encode((uint8_t *)pBuf, iBufSize, (uint8_t *)&pRef->qwSessionNonce, sizeof(pRef->qwSessionNonce), TRUE);
            }
            return(1);
        }
        else
        {
            return(-1);
        }
    }

    // get session flags
    if (iSelect == 'sflg')
    {
        return(_HandleSessionFlags(pRef, pRef->iSessionCreationFlags));
    }

    // status selectors that are deprecated or don't apply to this platform
    if (iSelect == 'skil')
    {
        return(0);
    }

    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] DirtySessionManagerStatus('%c%c%c%c'), (%d)\n", (uintptr_t)pRef,
            (uint8_t)(iSelect>>24), (uint8_t)(iSelect>>16), (uint8_t)(iSelect>>8), (uint8_t)(iSelect), iBufSize));

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtySessionManagerComplete

    \Description
        Returns TRUE and fills in secure address address if session manager has finished operation
        successfully.

    \Input *pRef            - module state
    \Input *pAddr           - storage for secure address (if result code is positive)
    \Input *pMachineAddr    - machine address

    \Output
        int32_t             - positive=success, zero=in progress, negative=failure

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
int32_t DirtySessionManagerComplete(DirtySessionManagerRefT *pRef, int32_t *pAddr, const char* pMachineAddr)
{
    XNADDR XnAddr;
    IN_ADDR InAddr;
    HRESULT hResult;
    
    if ((pRef->iSessionCreationFlags & XSESSION_CREATE_USES_PEER_NETWORK) == 0)
    {
        return(1);
    }
    
    // sanity check -- make sure machine addr contains xnaddr
    if (strchr(pMachineAddr, '^') == NULL)
    {
        NetPrintf(("dirtysessionmanagerxenon: [0x%08x] no xnaddr specified with peer address - cannot connect\n", pRef));
        return(-1);
    }
    
    // extract xnaddr from strMachineAddr
    DirtyAddrGetInfoXenon((DirtyAddrT *)pMachineAddr, NULL, &XnAddr);
    
    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] XnAddr.ina=%a\n", pRef, XnAddr.ina.s_addr));
    NetPrintf(("dirtysessionmanagerxenon: XnAddr.inaOnline=%a\n", XnAddr.inaOnline.s_addr));
    NetPrintf(("dirtysessionmanagerxenon: XnAddr.wPortOnline=%d\n", XnAddr.wPortOnline));

    // convert xnaddr to secure address
    if ((hResult = XNetXnAddrToInAddr(&XnAddr, &pRef->SessionInfo.sessionID, &InAddr)) != S_OK)
    {
        NetPrintf(("dirtysessionmanagerxenon: XNetXnAddrToInAddr() = 0x%08x\n", hResult));
        return(-1);
    }
    NetPrintf(("dirtysessionmanagerxenon: secure addr=%a\n", InAddr.s_addr));

    // save address for caller
    *pAddr = SocketHtonl(InAddr.s_addr);
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtySessionManagerConnect

    \Description
        Connects one or many player(s) to a given game

    \Input *pRef    - module state
    \Input **pSessID- for Xbox, this is an array of pointers to the peers DirtyAddrT
    \Input uSlot    - slot type to use, i.e. DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT, array: one per player
    \Input uCount   - number of players to join

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
void DirtySessionManagerConnect(DirtySessionManagerRefT *pRef, const char **pSessID, uint32_t *uSlot, uint32_t uCount)
{
    _DirtySessionManagerConnect(pRef, pSessID, uSlot, uCount);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtySessionManagerCreate

    \Description
        Allocate module state and prepare for use.

    \Input *pHostAddr               - host dirtyaddr, Not used
    \Input *pLocalAddrUnused        - DEPRECATED: local dirtyaddr, Not used

    \Output
        DirtySessionManagerRefT *   - reference pointer (must be passed to all other functions)

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
DirtySessionManagerRefT *DirtySessionManagerCreate(DirtyAddrT *pHostAddr, DirtyAddrT *pLocalAddrUnused)
{
    DirtySessionManagerRefT *pRef;
    int32_t iMemGroup;
    int32_t iIndex;
    void *pMemGroupUserData;

    // dirtysessionmanager uses netconn's memgroup and memgroup pointer as the memory can exceed the lifespan
    // of the module. see usage of  NetConnControl('recu') in DirtySessionManagerDestroy
    iMemGroup = NetConnStatus('mgrp', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), DIRTYSESSMGR_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->bAllowPresenceJoins = TRUE;
    pRef->bAllowPresenceInvites = TRUE;
    pRef->iUserIndex = -1;  //'self' selector is required to tell us who the local user index is

    for(iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; iIndex++)
    {
        NetConnStatus('xuid', iIndex, &pRef->LocalXuid[iIndex], sizeof(pRef->LocalXuid[iIndex]));
    }

    // set defaults
    pRef->iSessionCreationFlags = DIRTYSESSIONMANAGER_SESSIONFLAGS_DEFAULT;
    pRef->iMaxPublicSlots = DIRTYSESSIONMANAGER_MAXPUBLIC_DEFAULT;
    pRef->iMaxPrivateSlots = DIRTYSESSIONMANAGER_MAXPRIVATE_DEFAULT;
    pRef->iRankWeight = 100;

    NetPrintf(("dirtysessionmanagerxenon: [0x%08x] initial creation\n", pRef));

    // return ref to caller
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    DirtySessionManagerDestroy

    \Description
        Destroy the module and release its state

    \Input *pRef    - reference pointer

    \Version 01/17/2011 (jrainy)
*/
/*************************************************************************************************F*/
void DirtySessionManagerDestroy(DirtySessionManagerRefT *pRef)
{
    _DirtySessionManagerTerminate(pRef);

    if (!pRef->iNumQueuedOperations)
    {
        // proceed with destruction of DirtySessionManager reference
        _DirtySessionManagerDestroy(pRef);
    }
    else
    {
        // transfer DirtySessionManager destruction ownership to NetConn
        NetConnControl('recu', 0, 0, (void *)_DirtySessionManagerDestroyInternal, pRef);
    }
}
