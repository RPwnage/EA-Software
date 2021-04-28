/*H********************************************************************************/
/*!
    \File dirtycontextmanagerps4.c

   \Description
        dirtycontextManagerPs4 handles thread synchornization for creating and freeing
        sceHttp, sceSsl and sceNetPool context creation and deletion. 
        This functionality is necessary since the Sony library does not offer a thread 
        safe implementation.

    \Copyright
        Copyright (c) Electronic Arts 2016

    \Version 1.0 07/22/2016 (amakoukji)  First Version
*/
/********************************************************************************H*/


/*** Include files *********************************************************************/
#include <net.h>
#include <libhttp.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/ps4/dirtycontextmanagerps4.h"

/*** Defines ***************************************************************************/

#define DIRTYCM_MAX_CONTEXTS        (100)
#define DIRTYCM_MAX_NET_CONTEXTS    (32)
#define DIRTYCM_EXTERNAL_NETPOOL    (-1)
#define DIRTYCM_MIN_NETHEAP_SIZE    (4096)
#define DIRTYCM_POOLSIZE_FACTOR     (16384)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct DirtyContextManagerRefT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    NetCritT crit;

    int32_t aHttpContextId[DIRTYCM_MAX_CONTEXTS];
    int32_t aSslContextId[DIRTYCM_MAX_CONTEXTS];
    int32_t aNetContexts[DIRTYCM_MAX_NET_CONTEXTS];

} DirtyContextManagerRefT;

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/
static DirtyContextManagerRefT *_DirtyContextManagerRef = NULL;


/*** Functions *************************************************************************/

// public functions
int32_t DirtyContextManagerInit()
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    if (_DirtyContextManagerRef != NULL)
    {
        // early out, already created
        return(DIRTYCM_ERROR_ALREADY_INITIALIZED);
    }

    _DirtyContextManagerRef = (DirtyContextManagerRefT*)DirtyMemAlloc((int32_t)sizeof(DirtyContextManagerRefT), DIRTYCM_MEMID, iMemGroup, pMemGroupUserData);
    if (_DirtyContextManagerRef == NULL)
    {
        // early out, no more memeory
        return(DIRTYCM_ERROR_OUT_OF_MEMORY);
    }

    // setup the context of the DirtyContextManagerRefT
    memset(_DirtyContextManagerRef, 0, sizeof(DirtyContextManagerRefT));
    _DirtyContextManagerRef->iMemGroup = iMemGroup;
    _DirtyContextManagerRef->pMemGroupUserData = pMemGroupUserData;
    NetCritInit(&_DirtyContextManagerRef->crit, "DirtyHttpCM");

    return(0);
}

void DirtyContextManagerShutdown()
{
    int i = 0;

    if (_DirtyContextManagerRef == NULL)
    {
        return;
    }

    NetCritEnter(&_DirtyContextManagerRef->crit);

    // loop through http contexts and free any unfreed context groups
    for (i = 0; i < DIRTYCM_MAX_CONTEXTS; ++i)
    {
        // free unfreed contexts and warn 
        if (_DirtyContextManagerRef->aHttpContextId[i] > 0)
        {
            NetPrintf(("dirtycontextmanagerps4: warning, http context %d was not freed\n", _DirtyContextManagerRef->aHttpContextId[i]));
            sceHttpTerm(_DirtyContextManagerRef->aHttpContextId[i]);
            _DirtyContextManagerRef->aHttpContextId[i] = 0;
        }
    }

    // loop through ssl contexts and free any unfreed context groups
    for (i = 0; i < DIRTYCM_MAX_CONTEXTS; ++i)
    {
        // free unfreed contexts and warn 
        if (_DirtyContextManagerRef->aSslContextId[i] > 0)
        {
            NetPrintf(("dirtycontextmanagerps4: warning, ssl context %d was not freed\n", _DirtyContextManagerRef->aSslContextId[i]));
            sceHttpTerm(_DirtyContextManagerRef->aSslContextId[i]);
            _DirtyContextManagerRef->aSslContextId[i] = 0;
        }
    }

    // loop through and free net context
    for (i = 0; i < DIRTYCM_MAX_NET_CONTEXTS; ++i)
    {
         // free unfreed contexts and warn 
        if (_DirtyContextManagerRef->aNetContexts[i] > 0)
        {
            NetPrintf(("dirtycontextmanagerps4: warning, net pool %d was not freed\n", _DirtyContextManagerRef->aNetContexts[i]));
            sceNetPoolDestroy(_DirtyContextManagerRef->aNetContexts[i]);
            _DirtyContextManagerRef->aNetContexts[i] = 0;
        }
    }

    NetCritLeave(&_DirtyContextManagerRef->crit);

    // kill the critical section
    NetCritKill(&_DirtyContextManagerRef->crit);

    // free the module memory
    DirtyMemFree(_DirtyContextManagerRef, DIRTYCM_MEMID, _DirtyContextManagerRef->iMemGroup, _DirtyContextManagerRef->pMemGroupUserData);
}

int32_t DirtyContextManagerCreateNetPoolContext(const char *strName, int32_t iNetPoolSize)
{
    int32_t iReturn = 0;
    int32_t i = 0;
    int32_t iSlotId = 0;
    int32_t iFinalSize;
    char netPoolName[64];

    NetCritEnter(&_DirtyContextManagerRef->crit);

    // the net heap size has a 4KiB minimum
    iFinalSize = iNetPoolSize;
    if (iFinalSize < DIRTYCM_MIN_NETHEAP_SIZE)
    {
        iFinalSize = DIRTYCM_MIN_NETHEAP_SIZE;
    }

    // find an empty slot
    for (i = 0; i < DIRTYCM_MAX_NET_CONTEXTS; ++i)
    {
        if (_DirtyContextManagerRef->aNetContexts[i] == 0)
        {
            // found a slot
            iSlotId = i;
            break;
        }
    }

    if (i == DIRTYCM_MAX_NET_CONTEXTS)
    {
        return(DIRTYCM_ERROR_MAX_CONTEXTS_REACHED);
    }

    // auto-generate a pool name is necessary
    if (strName == NULL || strName[0] == '\0' || ds_strnlen(strName, (int32_t)sizeof(netPoolName)) >= (int32_t)sizeof(netPoolName))
    {
        ds_snzprintf(netPoolName, (int32_t)sizeof(netPoolName), "DirtyCM_netpool");
    }

    iReturn = sceNetPoolCreate(strName, iFinalSize, 0);
    if (iReturn < 0)
    {
        NetPrintf(("dirtycontextmanagerps4: failed to create netPool, error 0x%08x\n", iReturn));
    }
    else
    {
        _DirtyContextManagerRef->aNetContexts[iSlotId] = iReturn;
    }

    NetCritLeave(&_DirtyContextManagerRef->crit);
    return(iReturn);
}

int32_t DirtyContextManagerCreateSslContext(int32_t iSslBufferSize)
{
    int32_t iReturn = 0;
    int32_t i = 0;
    int32_t iSlotId = 0;

    NetCritEnter(&_DirtyContextManagerRef->crit);

    // there are no special size considerations for the ssl buffer

    // find an empty slot
    for (i = 0; i < DIRTYCM_MAX_CONTEXTS; ++i)
    {
        if (_DirtyContextManagerRef->aSslContextId[i] == 0)
        {
            // found a slot
            iSlotId = i;
            break;
        }
    }

    if (i == DIRTYCM_MAX_CONTEXTS)
    {
        return(DIRTYCM_ERROR_MAX_CONTEXTS_REACHED);
    }

    iReturn = sceSslInit(iSslBufferSize);
    if (iReturn < 0)
    {
        NetPrintf(("dirtycontextmanagerps4: failed to create ssl context, error 0x%08x\n", iReturn));
    }
    else
    {
        _DirtyContextManagerRef->aSslContextId[iSlotId] = iReturn;
    }

    NetCritLeave(&_DirtyContextManagerRef->crit);
    return(iReturn);
}

int32_t DirtyContextManagerCreateHttpContext(int32_t iSceNetContextId, int32_t iSslContextId, int32_t iHttpBufferSize)
{
    int32_t iReturn = 0;
    int32_t i = 0;
    int32_t iSlotId = 0;
    int32_t iFinalSize;

    NetCritEnter(&_DirtyContextManagerRef->crit);

    // http buffer size needs to be a multiple of 16KiB, round up
    iFinalSize = iHttpBufferSize;
    if ((iFinalSize % DIRTYCM_POOLSIZE_FACTOR) != 0)
    {
        int32_t iNewBufSize = ((iFinalSize / DIRTYCM_POOLSIZE_FACTOR) + 1) * DIRTYCM_POOLSIZE_FACTOR;
        NetPrintf(("dirtycontextmanagerps4: the memory pool %d needs to be a multiple of 16k, using next largest multiple %d\n", iFinalSize, iNewBufSize));
        iFinalSize = iNewBufSize;
    }

    // find an empty slot
    for (i = 0; i < DIRTYCM_MAX_CONTEXTS; ++i)
    {
        if (_DirtyContextManagerRef->aHttpContextId[i] == 0)
        {
            // found a slot
            iSlotId = i;
            break;
        }
    }

    if (i == DIRTYCM_MAX_CONTEXTS)
    {
        return(DIRTYCM_ERROR_MAX_CONTEXTS_REACHED);
    }

    iReturn = sceHttpInit(iSceNetContextId, iSslContextId, iFinalSize);
    if (iReturn < 0)
    {
        NetPrintf(("dirtycontextmanagerps4: failed to create http context, error 0x%08x\n", iReturn));
    }
    else
    {
        _DirtyContextManagerRef->aHttpContextId[iSlotId] = iReturn;
    }

    NetCritLeave(&_DirtyContextManagerRef->crit);
    return(iReturn);
}


int32_t DirtyContextManagerFreeNetPoolContext(int32_t iSceNetContextId)
{
    int32_t i = 0;
    int32_t iReturn = DIRTYCM_ERROR_UNKNOWN_CONTEXT;

    // loop through and find the context
    for (i = 0; i < DIRTYCM_MAX_NET_CONTEXTS; ++i)
    {
        if (_DirtyContextManagerRef->aNetContexts[i] ==  iSceNetContextId)
        {
            NetCritEnter(&_DirtyContextManagerRef->crit);
            sceNetPoolDestroy(_DirtyContextManagerRef->aNetContexts[i]);
            _DirtyContextManagerRef->aNetContexts[i] = 0;
            iReturn = 0;
            NetCritLeave(&_DirtyContextManagerRef->crit);
            break;
        }
    }

    return(iReturn);
}

int32_t DirtyContextManagerFreeSslContext(int32_t iSslContextId)
{
    int32_t i = 0;
    int32_t iReturn = DIRTYCM_ERROR_UNKNOWN_CONTEXT;

    // loop through and find the context
    for (i = 0; i < DIRTYCM_MAX_CONTEXTS; ++i)
    {
        if (_DirtyContextManagerRef->aSslContextId[i] ==  iSslContextId)
        {
            NetCritEnter(&_DirtyContextManagerRef->crit);
            sceSslTerm(_DirtyContextManagerRef->aSslContextId[i]);
            _DirtyContextManagerRef->aSslContextId[i] = 0;
            iReturn = 0;
            NetCritLeave(&_DirtyContextManagerRef->crit);
            break;
        }
    }

    return(iReturn);
}

int32_t DirtyContextManagerFreeHttpContext(int32_t iHttpContextId)
{
    int32_t i = 0;
    int32_t iReturn = DIRTYCM_ERROR_UNKNOWN_CONTEXT;

    // loop through and find the context
    for (i = 0; i < DIRTYCM_MAX_CONTEXTS; ++i)
    {
        if (_DirtyContextManagerRef->aHttpContextId[i] ==  iHttpContextId)
        {
            NetCritEnter(&_DirtyContextManagerRef->crit);
            sceHttpTerm(_DirtyContextManagerRef->aHttpContextId[i]);
            _DirtyContextManagerRef->aHttpContextId[i] = 0;
            iReturn = 0;
            NetCritLeave(&_DirtyContextManagerRef->crit);
            break;
        }
    }

    return(iReturn);
}



