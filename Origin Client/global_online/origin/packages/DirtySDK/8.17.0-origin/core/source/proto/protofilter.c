/*H********************************************************************************/
/*!
    \File protofilter.c

    \Description
        Encapsulate the profanity filter logic into one place. Especially for the
        Xenon profanity filter.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/10/2006 (tdills) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "protofilter.h"

/*** Defines **********************************************************************/

/*!  The number of times ProtoFilterResult will return PROTOFILTER_ERROR_PENDING
     before returning the result. This is for simulating latency. Set to 0 to get
     the result on the first call to ProtoFilterResult. */
#define PROTOFILTER_NUM_ASYNCWAITS  1 

#define PROFANE_STRING          "@/&!"

#define PROTOFILTER_PROFANE_STR_LEN 32

/*** Type Definitions *************************************************************/

//! filter operation
typedef struct ProtoFilterOpT
{
    uint8_t uRemainingResultDelay;
    uint8_t bInProgress;
    uint8_t _pad[2];
    int32_t iFilterId;
} ProtoFilterOpT;

// module state
struct ProtoFilterRefT
{
    //! module memory group
    int32_t         iMemGroup;
    void            *pMemGroupUserData;
    
    //! user-specified callback used to optionally override default filter functionality
    ProtoFilterCallbackT *pFilterCb;
    //! user callback data
    void            *pUserData;
    
    //! known profane string
    char            strKnownProfane[PROTOFILTER_PROFANE_STR_LEN];
    
    //! filter op array
    ProtoFilterOpT  FilterOps[PROTOFILTER_MAX_FILTEROPS];
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoFilterRequestCancel

    \Description
        Cancels an outstanding profanity filter request.

    \Input *pFilterRef  - pointer to module state
    \Input iFilterReqId - id returned from ProtoFilterRequest

    \Output None.

    \Version 04/05/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoFilterRequestCancel(ProtoFilterRefT *pFilterRef, int32_t iFilterReqId)
{
    ProtoFilterOpT *pFilterOp = NULL;
    
    // validate the iFilterReqId parameter
    if (iFilterReqId >= PROTOFILTER_MAX_FILTEROPS)
    {
        return;
    }

    // make sure this request is still pending.
    pFilterOp = &pFilterRef->FilterOps[iFilterReqId];
    if (!pFilterOp->bInProgress)
    {
        return;
    }

    // if we have a filter callback, forward cancel
    if (pFilterRef->pFilterCb != NULL)
    {
        ProtoFilterCtlT FilterCtl;
        
        // init filter control block
        memset(&FilterCtl, 0, sizeof(FilterCtl));
        FilterCtl.uFilterOp = PROTOFILTER_CALLBACKOP_CANCEL;
        FilterCtl.iFilterId = pFilterOp->iFilterId;
        
        // dispatch filter operation
        pFilterRef->pFilterCb(pFilterRef, &FilterCtl, pFilterRef->pUserData);
    }

    // cancel the op
    pFilterOp->bInProgress = FALSE;
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoFilterCreate

    \Description
        Create the ProtoFilter module.

    \Output ProtoFilterRefT* - pointer to module state

    \Version 03/10/2006 (tdills)
*/
/********************************************************************************F*/
ProtoFilterRefT *ProtoFilterCreate(void)
{
    ProtoFilterRefT *pFilterRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pFilterRef = DirtyMemAlloc(sizeof(*pFilterRef), PROTOFILTER_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("ProtoFilter: could not allocate module state\n"));
        return(NULL);
    }
    memset(pFilterRef, 0, sizeof(*pFilterRef));
    pFilterRef->iMemGroup = iMemGroup;
    pFilterRef->pMemGroupUserData = pMemGroupUserData;
    pFilterRef->strKnownProfane[0] = '\0';
    return(pFilterRef);
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterDestroy

    \Description
        Destroy the ProtoFilter module.

    \Input *pFilterRef - pointer to module state

    \Output None

    \Version 03/10/2006 (tdills)
*/
/********************************************************************************F*/
void ProtoFilterDestroy(ProtoFilterRefT *pFilterRef)
{
    // cancel any outstanding filter operations
    ProtoFilterRequestCancel(pFilterRef, -1);
    
    // dispose of module memory
    DirtyMemFree(pFilterRef, PROTOFILTER_MEMID, pFilterRef->iMemGroup, pFilterRef->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterRequest

    \Description
        Initiates a profanity filter request.

    \Input *pFilterRef   - pointer to module state
    \Input *pData        - text to be filtered
    \Input *pFilterReqId - id to use for polling response from ProtoFilterResult

    \Output int32_t - PROTOFILTER_ERROR_PENDING - if request is pending
                      PROTOFILTER_ERROR_FULL - if there are insufficient Ops buffers 
                                               to perform the request.

    \Version 03/10/2006 (tdills)
*/
/********************************************************************************F*/
int32_t ProtoFilterRequest(ProtoFilterRefT *pFilterRef, const char *pData, uint32_t *pFilterReqId)
{
    int32_t iFilterOp;

    // find an empty
    for (iFilterOp = 0; iFilterOp < PROTOFILTER_MAX_FILTEROPS; iFilterOp++)
    {
        ProtoFilterOpT *pFilterOp = &pFilterRef->FilterOps[iFilterOp];
        if (!pFilterOp->bInProgress)
        {
            // dispatch to callback?
            if (pFilterRef->pFilterCb != NULL)
            {
                ProtoFilterCtlT FilterCtl;
                
                // init filter control block
                memset(&FilterCtl, 0, sizeof(FilterCtl));
                FilterCtl.uFilterOp = PROTOFILTER_CALLBACKOP_FILTER;
                FilterCtl.pFilterText = pData;
                
                // dispatch filter operation
                if ((pFilterOp->iFilterId = pFilterRef->pFilterCb(pFilterRef, &FilterCtl, pFilterRef->pUserData)) < 0)
                {
                    NetPrintf(("protofilter: user callback filter request failed (err=%d)\n", pFilterOp->iFilterId));
                    return(pFilterOp->iFilterId);
                }
            }
            else
            {
                // set the number of attempts to get the response to 0
                pFilterOp->uRemainingResultDelay = PROTOFILTER_NUM_ASYNCWAITS;
            }
            
            // set the FilterOp to in progress.
            pFilterOp->bInProgress = TRUE;

            // set the callback id to the index of the FilterOp
            *pFilterReqId = iFilterOp;

            // tell the caller that the request is pending
            return(PROTOFILTER_ERROR_PENDING);
        }
    }

    return(PROTOFILTER_ERROR_FULL);
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterResult

    \Description
        Retrieves profanity filter responses.

    \Input *pFilterRef  - pointer to module state
    \Input uFilterReqId - id returned from ProtoFilterRequest

    \Output int32_t - PROTOFILTER_VIRTUOUS - if the text does not contain profanity.
                      PROTOFILTER_PROFANE - if the text is profane and should be filtered.
                      PROTOFILTER_ERROR_PENDING - if the request is still pending.
                      PROTOFILTER_INVALID_ID - if the uFilterReqId parameter is invalid.

    \Version 03/10/2006 (tdills)
*/
/********************************************************************************F*/
int32_t ProtoFilterResult(ProtoFilterRefT *pFilterRef, uint32_t uFilterReqId)
{
    ProtoFilterOpT *pFilterOp = NULL;

    // validate the uFilterReqId parameter
    if (uFilterReqId >= PROTOFILTER_MAX_FILTEROPS)
    {
        return(PROTOFILTER_INVALID_ID);
    }

    // make sure this request is still pending.
    pFilterOp = &pFilterRef->FilterOps[uFilterReqId];
    if (!pFilterOp->bInProgress)
    {
        return(PROTOFILTER_INVALID_ID);
    }

    // dispatch to callback?
    if (pFilterRef->pFilterCb != NULL)
    {
        ProtoFilterCtlT FilterCtl;
        int32_t iResult;
        
        // init filter control block
        memset(&FilterCtl, 0, sizeof(FilterCtl));
        FilterCtl.uFilterOp = PROTOFILTER_CALLBACKOP_RESULT;
        FilterCtl.iFilterId = pFilterOp->iFilterId;
        
        // dispatch filter operation
        if ((iResult = pFilterRef->pFilterCb(pFilterRef, &FilterCtl, pFilterRef->pUserData)) < 0)
        {
            if (iResult != PROTOFILTER_ERROR_PENDING)
            {
                NetPrintf(("protofilter: user callback filter request failed (err=%d)\n", iResult));
            }
            return(iResult);
        }
        else
        {
            // operation complete; return to caller
            pFilterOp->bInProgress = FALSE;
            return(iResult);
        }
    }
    else if (pFilterOp->uRemainingResultDelay == 0) // determine if it is time to send a response
    {
        // set this operation buffer to not-in-use
        pFilterOp->bInProgress = FALSE;

        // all text checked by the default implementation is "good"
        return(PROTOFILTER_VIRTUOUS);
    }

    // decrement the asynchronous reponse counter.
    pFilterOp->uRemainingResultDelay -= 1;
    
    // tell the caller the request is still pending.
    return(PROTOFILTER_ERROR_PENDING);
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterRequestCancel

    \Description
        Cancels an outstanding profanity filter request.

    \Input *pFilterRef  - pointer to module state
    \Input iFilterReqId - id returned from ProtoFilterRequest, or -1 to cancel all requests

    \Output None.

    \Version 04/05/2006 (jbrookes)
*/
/********************************************************************************F*/
void ProtoFilterRequestCancel(ProtoFilterRefT *pFilterRef, int32_t iFilterReqId)
{
    // cancel all?
    if (iFilterReqId == -1)
    {
        for (iFilterReqId = 0; iFilterReqId < PROTOFILTER_MAX_FILTEROPS; iFilterReqId++)
        {
            _ProtoFilterRequestCancel(pFilterRef, iFilterReqId);
        }
    }
    else if (iFilterReqId < PROTOFILTER_MAX_FILTEROPS)
    {
        _ProtoFilterRequestCancel(pFilterRef, iFilterReqId);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterApplyFilter

    \Description
        apply a profanity filter to a string. Modifies the string in place.

    \Input *pFilterRef  - pointer to module state
    \Input uFilterType  - the filter type SQUELCH - masks the string; PROFANE returns a dummy string
    \Input *pData       - the string to filter

    \Output
        const char*     - the input string is modified in place.

    \Version 03/10/2006 (tdills)
*/
/********************************************************************************F*/
void ProtoFilterApplyFilter(ProtoFilterRefT *pFilterRef, uint32_t uFilterType, char *pData)
{
    char *pTemp;

    if (uFilterType == PROTOFILTER_TYPE_SQUELCH)
    {
        // squelch the message text
        for (pTemp = pData; *pTemp != '\0'; pTemp++)
        {
            *pTemp = (*pTemp == ' ') ? ' ' : '*';
        }
    }
    else if (uFilterType == PROTOFILTER_TYPE_PROFANE)
    {
        if (pFilterRef->strKnownProfane[0] == '\0')
        {
            strnzcpy(pData, PROFANE_STRING, sizeof(PROFANE_STRING));
        }
        else
        {
            strnzcpy(pData, pFilterRef->strKnownProfane, PROTOFILTER_PROFANE_STR_LEN);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterControl

    \Description
        Control ProtoFilter behavior

    \Input *pFilterRef    - pointer to module state
    \Input iControl       - control selector
    \Input iValue         - selector specifc
    \Input iValue2        - selector specific
    \Input *pValue        - selector specific
    
    \Output
        int32_t           - selector specific

    \Notes
        iControl can be one of the following:
    
        \verbatim
            'pstr' - the known profane string to use in ProtoFilterApplyFilter
        \endverbatim

    \Version 03/13/2006 (tdills)
*/
/********************************************************************************F*/
int32_t ProtoFilterControl(ProtoFilterRefT *pFilterRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue)
{
    if (iControl == 'pstr')
    {
        strnzcpy(pFilterRef->strKnownProfane, (const char*)pValue, PROTOFILTER_PROFANE_STR_LEN);
        return(0);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterCallback

    \Description
        Set a ProtoFilter callback.  If set, the provided callback function will be
        called to perform filter operations and get filter results.

    \Input *pFilterRef  - pointer to module state
    \Input *pFilterCb   - pointer to filter callback function
    \Input *pUserData   - user data pointer to be passed through to user callback
    
    \Output
        None.

    \Version 03/21/2008 (jbrookes)
*/
/********************************************************************************F*/
void ProtoFilterCallback(ProtoFilterRefT *pFilterRef, ProtoFilterCallbackT *pFilterCb, void *pUserData)
{
    pFilterRef->pFilterCb = pFilterCb;
    pFilterRef->pUserData = pUserData;
}
