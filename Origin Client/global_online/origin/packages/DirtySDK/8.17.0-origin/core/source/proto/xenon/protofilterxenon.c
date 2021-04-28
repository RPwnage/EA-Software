/*H********************************************************************************/
/*!
    \File protofilterxenon.c

    \Description
        Encapsulate the profanity filter logic into one place. Especially for the
        Xenon profanity filter.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/10/2006 (tdills) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <memory.h>
#include <xtl.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "protofilter.h"

/*** Defines **********************************************************************/

#define PROFANE_STRING          "@/&!"

#define PROTOFILTER_PROFANE_STR_LEN 32

/*** Type Definitions *************************************************************/

typedef struct ProtoFilterXenonVerifyResponseT
{
    STRING_VERIFY_RESPONSE VerifyResponse;
    HRESULT                hResult;
} ProtoFilterXenonVerifyResponseT;

typedef struct ProtoFilterXenonOpT
{
    STRING_DATA StringData;
    XOVERLAPPED XOverlapped;
    DWORD       cbResults;
    ProtoFilterXenonVerifyResponseT LobbyResponse;
    uint8_t     bInProgress;
    uint8_t     _pad[3];
    WCHAR       wstrText[1024];
} ProtoFilterXenonOpT;

struct ProtoFilterRefT
{
    // module memory group
    int32_t             iMemGroup;          //!< module mem group id
    void                *pMemGroupUserData; //!< user data associated with mem group
    
    char                strKnownProfane[32];
    ProtoFilterXenonOpT FilterOps[PROTOFILTER_MAX_FILTEROPS];
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _strcpyw

    \Description
        Copy a string to a wide string

    \Input pStrW    - [out] pointer to wide string
    \Input pSrc     - pointer to input string
    
    \Output
        None.

    \Version 09/16/2005 (jbrookes)
    \Version 03/10/2006 (tdills) moved from lobbyapixenon.c to protofilterxenon.c
*/
/********************************************************************************F*/
static void _strcpyw(WCHAR *pStrW, const char *pSrc)
{
    while(*pSrc != '\0')
    {
        *pStrW++ = (WCHAR)*pSrc++;
    }
    *pStrW++ = (WCHAR)*pSrc++;
}

/*F********************************************************************************/
/*!
    \Function _ProtoFilterRequestCancel

    \Description
        Cancels a single outstanding profanity filter request.

    \Input *pFilterRef  - pointer to module state
    \Input iFilterReqId - id returned from ProtoFilterRequest

    \Output None.

    \Version 04/05/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoFilterRequestCancel(ProtoFilterRefT *pFilterRef, int32_t iFilterReqId)
{
    ProtoFilterXenonOpT *pFilterOp;
    
    // validate the uFilterReqId parameter
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

    // cancel the op
    if (XCancelOverlapped(&pFilterOp->XOverlapped) != ERROR_SUCCESS)
    {
        NetPrintf(("protofilter: error cancelling overlapped operation\n"));
    }
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

    \Input *pFilterRef  - pointer to module state
    
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

    // find an empty filter op
    for (iFilterOp = 0; iFilterOp < PROTOFILTER_MAX_FILTEROPS; iFilterOp++)
    {
        ProtoFilterXenonOpT *pFilterOp = &pFilterRef->FilterOps[iFilterOp];
        if (!pFilterOp->bInProgress)
        {
            // make a wide character version of message text to feed to XStringVerify()
            _strcpyw(pFilterOp->wstrText, pData);

            // set up stringdata structure
            pFilterOp->StringData.pszString = pFilterOp->wstrText;
            pFilterOp->StringData.wStringSize = wcslen(pFilterOp->StringData.pszString); 

            // clear filter response            
            memset(&pFilterOp->LobbyResponse, 0, sizeof(pFilterOp->LobbyResponse));
            pFilterOp->LobbyResponse.VerifyResponse.pStringResult = &pFilterOp->LobbyResponse.hResult;
            pFilterOp->cbResults = sizeof(pFilterOp->LobbyResponse);

            // send the request to the XBox SDK.
            if (XStringVerify(0, "en-us", 1, &pFilterOp->StringData, pFilterOp->cbResults, 
                &pFilterOp->LobbyResponse.VerifyResponse, &pFilterOp->XOverlapped) == ERROR_IO_PENDING)
            {
                // set the FilterOp to in progress.
                pFilterOp->bInProgress = TRUE;

                // set the callback id to the index of the FilterOp
                *pFilterReqId = iFilterOp;

                // tell the caller that the request is pending
                return(PROTOFILTER_ERROR_PENDING);
            }
        }
    }

    return(PROTOFILTER_ERROR_FULL);
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
    ProtoFilterXenonOpT *pFilterOp = NULL;
    DWORD dwResult;

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

    // determine if it is time to send a response
    if ((dwResult = XGetOverlappedExtendedError(&pFilterOp->XOverlapped)) != ERROR_IO_INCOMPLETE)
    {
        // this request is done.
        pFilterOp->bInProgress = FALSE;

        // return the result
        if (S_OK == pFilterOp->LobbyResponse.hResult)
        {
            return(PROTOFILTER_VIRTUOUS);
        }
        else
        {
            return(PROTOFILTER_PROFANE);
        }
    }

    // tell the caller the request is still pending.
    return(PROTOFILTER_ERROR_PENDING);
}

/*F********************************************************************************/
/*!
    \Function ProtoFilterApplyFilter

    \Description
        apply a profanity filter to a string. Modifies the string in place.

    \Input *pFilterRef - pointer to module state
    \Input uFilterType   - the filter type SQUELCH - masks the string; PROFANE returns a dummy string
    \Input *pData        - the string to filter

    \Output
        const char*      - the input string is modified in place.

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
        Unsupported on Xenon

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
}
