/*H**************************************************************************/
/*!

    \File dirtycert.c

    \Description
        This module defines the CA fallback mechanism which is used by ProtoSSL.

    \Notes
        This module is designed to be thread-safe except the creating/destroying APIs,
        however because some other code (such as CA list) are not thread-safe yet,
        so it's not totally thread-safe right now.
        If two protossl instances are requesting the same CA cert at the same time,
        ref-counting is used and only one CA fetch request will be made to the server.

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Version 01/23/2012 (szhu)
*/
/***************************************************************************H*/

/*** Include files ***********************************************************/

#include <string.h>

#include "dirtyvers.h"
#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"
#include "protossl.h"
#include "protohttp.h"
#include "xmlparse.h"
#include "lobbybase64.h"
#include "dirtycert.h"

/*** Defines ***************************************************************************/

#define DIRTYCERT_MAXREQUESTS       (16)                            //! max concurrent CA requests
#define DIRTYCERT_TIMEOUT           (30*1000)                       //! default dirtycert timeout
#define DIRTYCERT_MAXURL            (2048)                          //! max url length
#define DIRTYCERT_MAXCERTIFICATE    (8*1024)                        //! max certificate size, in bytes
#define DIRTYCERT_MAXRESPONSE   ((DIRTYCERT_MAXCERTIFICATE)*3)      //! max response size, in bytes
#define DIRTYCERT_MAXCERTDECODED \
    ((LobbyBase64DecodedSize(DIRTYCERT_MAXCERTIFICATE)+3)&0x7ffc)   //! max certificate size (decoded), in bytes
#define DIRTYCERT_REQUESTID_NONE    (-1)                            //! request id none

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! request type
typedef enum RequestTypeE
{
    RT_ONDEMAND = 0,
    RT_PREFETCH
} RequestTypeE;

//! request status
typedef enum RequestStatusE
{
    RS_NONE = 0,
    RS_NOT_STARTED,
    RS_IN_PROGRESS,
    RS_DONE,
    RS_FAILED,
} RequestStatusE;

//! CA fetch request
typedef struct DirtyCertCARequestT
{
    ProtoSSLCertInfoT CertInfo;     //!< certificate info (will be sent for ondemand request)
    char strServiceName[32];        //!< service name (used for prefetch request)
    RequestTypeE eType;             //!< request type
    RequestStatusE eStatus;         //!< request status
    int32_t iRefCount;              //!< ref count to this request
}DirtyCertCARequestT;

//! module state
struct DirtyCertRefT
{
    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    NetCritT crit;                  //!< critical section used to ensure thread safety
    ProtoHttpRefT *pHttp;           //!< http ref

    char strUrl[DIRTYCERT_MAXURL];  //!< url buffer

    // buffer for response processing
    uint8_t aResponseBuf[DIRTYCERT_MAXRESPONSE]; //!< buffer for http response
    uint8_t aCertBuf[DIRTYCERT_MAXCERTIFICATE]; //!< buffer for a single certificate
    uint8_t aCertDecodedBuf[DIRTYCERT_MAXCERTDECODED]; //!< buffer for a single certificate (decoded)

    uint32_t uTimeout;              //!< request timeout
    int32_t iRequestId;             //!< current on-going request (==index in request list)
    int32_t iCount;                 //!< count of valid requests
    DirtyCertCARequestT requests[DIRTYCERT_MAXREQUESTS]; //!< request list
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! module state ref
static DirtyCertRefT *_DirtyCert_pState = NULL;

//! hard-coded server url
static const char* _DirtyCert_strRedirectorUrl = "https://gosca.ea.com:44125/redirector";

// Public variables

/*** Private functions ******************************************************/


#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function _DirtyCertCAGetStrIdent

    \Description
        Debug-only function to get a string identifier of the request, for debug printing

    \Input *pRequest        - request to get string identifier for

    \Output
        const char *        - pointer to string ident

    \Version 04/18/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_DirtyCertCAGetStrIdent(DirtyCertCARequestT *pRequest)
{
    static char _strIdent[512];
    if (pRequest->eType == RT_ONDEMAND)
    {
        ds_snzprintf(_strIdent, sizeof(_strIdent), "[%s:%s:%d]", pRequest->CertInfo.Ident.strCommon, pRequest->CertInfo.Ident.strUnit, pRequest->CertInfo.iKeyModSize);
    }
    else
    {
        ds_snzprintf(_strIdent, sizeof(_strIdent), "[%s]", pRequest->strServiceName);
    }
    return(_strIdent);
}
#endif

/*F********************************************************************************/
/*!
    \Function _DirtyCertUrlEncodeStrParm

    \Description
        Wrapper for ProtoHttpUrlEncodeStrParm() that won't encode a null string.

    \Input *pBuffer         - output buffer to encode into
    \Input iLength          - length of output buffer
    \Input *pParm           - parameter name (not encoded)
    \Input *pData           - parameter data

    \Version 04/18/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyCertUrlEncodeStrParm(char *pBuffer, int32_t iLength, const char *pParm, const char *pData)
{
    if (*pData != '\0')
    {
        ProtoHttpUrlEncodeStrParm(pBuffer, iLength, pParm, pData);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertCompareCertInfo

    \Description
        Compare two cert infos.

    \Input *pCertInfo1    - the cert info to be compared
    \Input *pCertInfo2    - the cert info to be compared

    \Output int32_t       - zero=match, non-zero=no-match

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _DirtyCertCompareCertInfo(const ProtoSSLCertInfoT *pCertInfo1, const ProtoSSLCertInfoT *pCertInfo2)
{
    if ((pCertInfo1->iKeyModSize != pCertInfo2->iKeyModSize)
        || strcmp(pCertInfo1->Ident.strCountry, pCertInfo2->Ident.strCountry)
        || strcmp(pCertInfo1->Ident.strState, pCertInfo2->Ident.strState)
        || strcmp(pCertInfo1->Ident.strCity, pCertInfo2->Ident.strCity)
        || strcmp(pCertInfo1->Ident.strOrg, pCertInfo2->Ident.strOrg)
        || strcmp(pCertInfo1->Ident.strCommon, pCertInfo2->Ident.strCommon)
        || strcmp(pCertInfo1->Ident.strUnit, pCertInfo2->Ident.strUnit))
    {
        return(1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertFormatRequestUrl

    \Description
        Format a request into provided buffer.

    \Input *pState      - module ref
    \Input *pRequest    - CA request
    \Input *pBuf        - user-supplied url buffer
    \Input iBufLen      - size of buffer pointed to by pBuf

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static void _DirtyCertFormatRequestUrl(DirtyCertRefT *pState, DirtyCertCARequestT *pRequest, char *pBuf, int32_t iBufLen)
{
    // for params name and format, refer to: blazeserver\dev3\component\redirector\gen\redirectortypes.tdf
    if (pRequest->eType == RT_ONDEMAND)
    {
        // set url and DS version (the latter is currently ignored by the redirector)
        ds_snzprintf(pBuf, iBufLen, "%s/findCACertificates?v=%08x", _DirtyCert_strRedirectorUrl, DIRTYVERS);
        // append KeyModSize (in bits)
        ProtoHttpUrlEncodeIntParm(pBuf, iBufLen, "&bits=", pRequest->CertInfo.iKeyModSize * 8);
        // append Common Name
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|CN=", pRequest->CertInfo.Ident.strCommon);
        // append Country info
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|C=", pRequest->CertInfo.Ident.strCountry);
        // append Organization info
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|O=", pRequest->CertInfo.Ident.strOrg);
        // append Organization Unit info
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|OU=", pRequest->CertInfo.Ident.strUnit);
        // append City info
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|L=", pRequest->CertInfo.Ident.strCity);
        // append State info
        _DirtyCertUrlEncodeStrParm(pBuf, iBufLen, "&entr|S=", pRequest->CertInfo.Ident.strState);
    }
    else // PREFETCH type
    {
        // set url and DS version (the latter is currently ignored by the redirector)
        ds_snzprintf(pBuf, iBufLen, "%s/getCACertificates?v=%08x", _DirtyCert_strRedirectorUrl, DIRTYVERS);
        // append service name
        ProtoHttpUrlEncodeStrParm(pBuf, iBufLen, "&name=", pRequest->strServiceName);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertCreateRequest

    \Description
        Issue a CA fetch request.

    \Input *pState      - module ref
    \Input iRequestId   - CA request id
    \Input *pRequest    - CA request

    \Output int32_t     - 0 for success

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _DirtyCertCreateRequest(DirtyCertRefT *pState, int32_t iRequestId, DirtyCertCARequestT *pRequest)
{
    // if we're not busy
    if (pState->iRequestId == DIRTYCERT_REQUESTID_NONE)
    {
        memset(pState->strUrl, 0, sizeof(pState->strUrl));
        // format request url
        _DirtyCertFormatRequestUrl(pState, pRequest, pState->strUrl, sizeof(pState->strUrl));
        // set timeout
        ProtoHttpControl(pState->pHttp, 'time', pState->uTimeout, 0, NULL);
        // http GET
        NetPrintf(("dirtycert: making CA request: %s\n", pState->strUrl));
        if (ProtoHttpGet(pState->pHttp, pState->strUrl, 0) >= 0)
        {
            pState->iRequestId = iRequestId;
            pRequest->eStatus = RS_IN_PROGRESS;
        }
        else
        {
            // failed?
            pRequest->eStatus = RS_FAILED;
            NetPrintf(("dirtycert: ProtoHttpGet(%x, %s) failed.\n", pState->pHttp, pState->strUrl));
        }
    }

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _DirtyCertCARequestFree

    \Description
        Release resources used by a CA fetch request.

    \Input *pState      - module state
    \Input *pRequest    - request ptr
    \Input iRequestId   - request id

    \Output
        int32_t         - negative=error, else success

    \Version 04/18/2012 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _DirtyCertCARequestFree(DirtyCertRefT *pState, DirtyCertCARequestT *pRequest, int32_t iRequestId)
{
    int32_t iResult = 0;

    if (pRequest->iRefCount <= 0)
    {
        // error request id?
        iResult = -3;
    }
    else if (--pRequest->iRefCount == 0)
    {
        // abort current operation
        if (pState->iRequestId == iRequestId)
        {
            ProtoHttpAbort(pState->pHttp);
            pState->iRequestId = DIRTYCERT_REQUESTID_NONE;
        }
        NetPrintf(("dirtycert: freeing CA fetch request for %s\n", _DirtyCertCAGetStrIdent(pRequest)));
        memset(pRequest, 0, sizeof(*pRequest));
        pRequest->eStatus = RS_NONE;
        pState->iCount--;
        iResult = 1;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertProcessResponse

    \Description
        Process response.

    \Input *pState      - module ref
    \Input *pResponse   - http response
    \Input iLen         - length of response
    \Input *pFailed     - [out] storage for number of CA certificate loads that failed

    \Output int32_t     - positive=installed certs, 0 or negative=error

    \Version 01/24/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _DirtyCertProcessResponse(DirtyCertRefT *pState, const uint8_t *pResponse, int32_t iLen, int32_t *pFailed)
{
    int32_t iCount = 0, iCertLen, iResult;
    const uint8_t *pCert;

    *pFailed = 0;

    // for response format, refer to: blazeserver\dev3\component\redirector\gen\redirectortypes.tdf
    pCert = XmlFind((const uint8_t*)pResponse, (const uint8_t*)"cacertificate.certificatelist.certificatelist");
    while (pCert)
    {
        memset(pState->aCertBuf, 0, sizeof(pState->aCertBuf));
        if ((iCertLen = XmlContentGetString(pCert, pState->aCertBuf, sizeof(pState->aCertBuf), (const uint8_t*)"")) > 0)
        {
            // the cert response might be base64 encoded
            char sEncoding[32];
            memset(sEncoding, 0, sizeof(sEncoding));
            XmlAttribGetString(pCert, (const uint8_t*)"enc", (uint8_t*)sEncoding, sizeof(sEncoding), (const uint8_t*)"");
            // base64 encoded, decode and install the cert
            if (ds_stricmp(sEncoding, "base64") == 0)
            {
                memset(pState->aCertDecodedBuf, 0, sizeof(pState->aCertDecodedBuf));
                if (LobbyBase64Decode(iCertLen, (const char*)pState->aCertBuf, (char*)pState->aCertDecodedBuf))
                {
                    if ((iResult = ProtoSSLSetCACert(pState->aCertDecodedBuf, (int32_t)strlen((const char*)pState->aCertDecodedBuf))) > 0)
                    {
                        iCount += iResult;
                    }
                    else
                    {
                        *pFailed += 1;
                    }
                }
            }
            else
            {
                // plain, install the cert
                if ((iResult = ProtoSSLSetCACert(pState->aCertBuf, iCertLen)) > 0)
                {
                    iCount += iResult;
                }
                else
                {
                    *pFailed += 1;
                }
            }
        }
        pCert = XmlNext(pCert);
    }
    return(iCount);
}

/*F********************************************************************************/
/*!
    \Function _DirtyCertUpdate

    \Description
        Update status of DirtyCert module.

    \Input *pData   - pointer to DirtyCert module ref
    \Input uData    - unused

    \Notes
        This is be the only private 'static' function that needs to acquire crit
        because it's actually called from NetConnIdle.

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
static void _DirtyCertUpdate(void *pData, uint32_t uData)
{
    DirtyCertRefT *pState = (DirtyCertRefT*)pData;
    DirtyCertCARequestT *pRequest;

    // acquire critical section
    NetCritEnter(&pState->crit);
    if (pState->iRequestId != DIRTYCERT_REQUESTID_NONE)
    {
        pRequest = &pState->requests[pState->iRequestId];

        if (pRequest->eStatus == RS_IN_PROGRESS)
        {
            int32_t iHttpStatus;
            // update http
            ProtoHttpUpdate(pState->pHttp);
            // if we've done with current http request
            if ((iHttpStatus = ProtoHttpStatus(pState->pHttp, 'done', NULL, 0)) > 0)
            {
                // check http response code, we only deal with "20X OK"
                if ((ProtoHttpStatus(pState->pHttp, 'code', NULL, 0) / 100) == 2)
                {
                    int32_t iRecvLen, iFailed;
                    // zero buf
                    memset(pState->aResponseBuf, 0, sizeof(pState->aResponseBuf));
                    if ((iRecvLen = ProtoHttpRecvAll(pState->pHttp, (char*)pState->aResponseBuf, sizeof(pState->aResponseBuf))) > 0)
                    {
                        // install received certs
                        iRecvLen = _DirtyCertProcessResponse(pState, pState->aResponseBuf, iRecvLen, &iFailed);
                        pRequest->eStatus = RS_DONE;
                        pState->iRequestId = DIRTYCERT_REQUESTID_NONE;
                        NetPrintf(("dirtycert: %d cert(s) installed for %s (%d failed)\n", iRecvLen,
                            _DirtyCertCAGetStrIdent(pRequest), iFailed));
                    }
                }

                // unknown error? (ex. http 500 error)
                if (pRequest->eStatus != RS_DONE)
                {
                    iHttpStatus = -1;
                    NetPrintf(("dirtycert: CA fetch request failed (httpcode=%d)\n", ProtoHttpStatus(pState->pHttp, 'code', NULL, 0)));
                }
            }
            else if (iHttpStatus == 0)
            {
                // if we're missing CA cert for our http ref, we fail.
                if (ProtoHttpStatus(pState->pHttp, 'cfip', NULL, 0) > 0)
                {
                    // mark request as failed but don't reset pState->iRequestId otherwise a new request may be issued
                    pRequest->eStatus = RS_FAILED;
                    NetPrintf(("dirtycert: CA fetch request failed because of no CA available for redirector: %s\n", _DirtyCert_strRedirectorUrl));
                }
            }

            // if we failed (or timed-out)
            if (iHttpStatus < 0)
            {
                pRequest->eStatus = RS_FAILED;
                pState->iRequestId = DIRTYCERT_REQUESTID_NONE;
                NetPrintf(("dirtycert: CA fetch request for %s %s\n", _DirtyCertCAGetStrIdent(pRequest),
                    ProtoHttpStatus(pState->pHttp, 'time', NULL, 0) ? "timeout" : "failed"));
            }

            // prefetch requests automatically clean up after themselves upon completion
            if ((pRequest->eType == RT_PREFETCH) && (iHttpStatus != 0))
            {
                _DirtyCertCARequestFree(pState, pRequest, pState->iRequestId);
            }
        }
    }

    // if we need to issue a new request?
    if ((pState->iRequestId == DIRTYCERT_REQUESTID_NONE) && (pState->iCount > 0))
    {
        int32_t iRequestId;
        for (iRequestId = 0; iRequestId < DIRTYCERT_MAXREQUESTS; iRequestId++)
        {
            pRequest = &pState->requests[iRequestId];
            if ((pRequest->iRefCount > 0) && (pRequest->eStatus == RS_NOT_STARTED))
            {
                if (_DirtyCertCreateRequest(pState, iRequestId, pRequest) == 0)
                {
                    break;
                }
            }
        }
    }

    // release critical section
    NetCritLeave(&pState->crit);
}


/*** Public functions ********************************************************/


/*F*************************************************************************************************/
/*!
    \Function DirtyCertCreate

    \Description
        Startup DirtyCert module.

    \Output int32_t     - 0 for success, negative for error

    \Version 01/23/2012 (szhu)
*/
/*************************************************************************************************F*/
int32_t DirtyCertCreate(void)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    if (_DirtyCert_pState != NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCreate() called while module is already active\n"));
        return(-1);
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), DIRTYCERT_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtycert: could not allocate module state\n"));
        return(-2);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // create http ref
    if ((pState->pHttp = ProtoHttpCreate(DIRTYCERT_MAXRESPONSE)) == NULL)
    {
        DirtyMemFree(pState, DIRTYCERT_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(-3);
    }
    // no request
    pState->iRequestId = DIRTYCERT_REQUESTID_NONE;

    // set request timeout
    pState->uTimeout = DIRTYCERT_TIMEOUT;

    // init crit
    NetCritInit(&pState->crit, "DirtyCert");

    // add update function to netconn idle handler
    NetConnIdleAdd(_DirtyCertUpdate, pState);

    // save module ref
    _DirtyCert_pState = pState;

    NetPrintf(("dirtycert: created dirtycert 0x%x (pHttp=0x%08x)\n", pState, pState->pHttp));
    // return module state
    return(0);
}

/*F************************************************************************/
/*!
    \Function DirtyCertDestroy

    \Description
        Destroy the module and release its state.

    \Output int32_t     - 0 for success, negative for error

    \Version 01/23/2012 (szhu)
*/
/*************************************************************************F*/
int32_t DirtyCertDestroy(void)
{
    DirtyCertRefT *pState = _DirtyCert_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertDestroy() called while module is not active\n"));
        return(-1);
    }

    // remove idle handler
    NetConnIdleDel(_DirtyCertUpdate, pState);

    // clear global module ref
    _DirtyCert_pState = NULL;

    // destroy http ref
    ProtoHttpDestroy(pState->pHttp);

    // destroy crit
    NetCritKill(&pState->crit);

    // free the memory
    DirtyMemFree(pState, DIRTYCERT_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    NetPrintf(("dirtycert: freed dirtycert 0x%x\n", pState));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function DirtyCertCARequestCert

    \Description
        Initialize a CA fetch request.

    \Input *pCertInfo   - CA Cert Info

    \Output int32_t     - positive=id of request, negative=error

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************F*/
int32_t DirtyCertCARequestCert(const ProtoSSLCertInfoT *pCertInfo)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t i, iSlot = -1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestCert() called while module is not active\n"));
        return(-1);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // check if it matches any in-progress request
    for (i = 0; i < DIRTYCERT_MAXREQUESTS; i++)
    {
        // if this is an empty slot
        if (pState->requests[i].iRefCount <= 0)
        {
            // mark it
            if (iSlot < 0)
            {
                iSlot = i;
            }
            continue;
        }

        // we found an in-progress request with the same cert info
        if (_DirtyCertCompareCertInfo(pCertInfo, &pState->requests[i].CertInfo) == 0)
        {
            iSlot = i;
            break;
        }
    }

    if (iSlot >= 0)
    {
        DirtyCertCARequestT *pRequest = &pState->requests[iSlot];
        // a CA request for this cert is already queued
        if (pRequest->iRefCount > 0)
        {
            // inc refcount
            pRequest->iRefCount++;
            NetPrintf(("dirtycert: CA fetch request %s already queued (status=%d), updated refcount to %d.\n",
                _DirtyCertCAGetStrIdent(pRequest), (int32_t)pRequest->eStatus, pRequest->iRefCount));
        }
        // no matching request, create a new one
        else
        {
            memset(pRequest, 0, sizeof(*pRequest));
            // save cert info
            memcpy(&pRequest->CertInfo, pCertInfo, sizeof(pRequest->CertInfo));
            pRequest->eType = RT_ONDEMAND;
            pRequest->eStatus = RS_NOT_STARTED;
            // inc refcount
            pRequest->iRefCount = 1;
            pState->iCount++;
            NetPrintf(("dirtycert: queued CA fetch request for %s\n", _DirtyCertCAGetStrIdent(pRequest)));
            _DirtyCertCreateRequest(pState, iSlot, pRequest);
            // public slot reference is +1 to reserve zero
            iSlot += 1;
        }
    }
    else
    {
        NetPrintf(("dirtycert: too many CA fetch requests.\n"));
    }

    // release critical section
    NetCritLeave(&pState->crit);

    // return slot ref to caller
    return(iSlot);
}

/*F********************************************************************************/
/*!
    \Function DirtyCertCAPreloadCerts

    \Description
        Initialize a CA preload request

    \Input *pServiceName    - servicename to identify CA preload set ("gamename-gameyear-platform")

    \Notes
        Unlike the explicit CA load, a preload request is fire and forget, and
        will clean up after itself when complete.

    \Version 04/18/2012 (jbrookes)
*/
/********************************************************************************F*/
void DirtyCertCAPreloadCerts(const char *pServiceName)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t i, iSlot = -1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestCert() called while module is not active\n"));
        return;
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // find an empty slot
    for (i = 0; i < DIRTYCERT_MAXREQUESTS; i++)
    {
        // if this is an empty slot
        if (pState->requests[i].iRefCount <= 0)
        {
            // mark it
            if (iSlot < 0)
            {
                iSlot = i;
                break;
            }
        }
    }

    if (iSlot >= 0)
    {
        DirtyCertCARequestT *pRequest = &pState->requests[iSlot];

        memset(pRequest, 0, sizeof(*pRequest));
        // save service id info
        ds_strnzcpy(pRequest->strServiceName, pServiceName, sizeof(pRequest->strServiceName));
        pRequest->eType = RT_PREFETCH;
        pRequest->eStatus = RS_NOT_STARTED;
        // inc refcount
        pRequest->iRefCount = 1;
        pState->iCount++;

        NetPrintf(("dirtycert: queued CA prefetch request for %s\n", _DirtyCertCAGetStrIdent(pRequest)));
        _DirtyCertCreateRequest(pState, iSlot, pRequest);
    }
    else
    {
        NetPrintf(("dirtycert: too many CA fetch requests.\n"));
    }

    // release critical section
    NetCritLeave(&pState->crit);
}

/*F*************************************************************************************/
/*!
    \Function DirtyCertCARequestDone

    \Description
        Determine if a CA fetch request is complete.

    \Input iRequestId   - id of CA request

    \Output
        int32_t         - zero=in progess, neg=done w/error, pos=done w/success

    \Version 01/23/2012 (szhu)
*/
/************************************************************************************F*/
int32_t DirtyCertCARequestDone(int32_t iRequestId)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    DirtyCertCARequestT *pRequest;
    int32_t iResult = 0;

    // internal request id is public id - 1
    iRequestId -= 1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestDone() called while module is not active\n"));
        return(-1);
    }
    // if it's a valid request id
    if ((iRequestId < 0) || (iRequestId >= DIRTYCERT_MAXREQUESTS))
    {
        NetPrintf(("dirtycert: invalid request id (%d)\n", iRequestId));
        return(-2);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    pRequest = &pState->requests[iRequestId];
    if (pRequest->iRefCount <= 0)
    {
        // error request id?
        iResult = -3;
    }
    else if (pRequest->eStatus == RS_FAILED)
    {
        iResult = -4;
    }
    else if (pRequest->eStatus == RS_DONE)
    {
        iResult = 1;
    }

    // release critical section
    NetCritLeave(&pState->crit);

    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function DirtyCertCARequestFree

    \Description
        Release resources used by a CA fetch request.

    \Input iRequestId    - id of CA request

    \Output int32_t      - negative=error, else success

    \Version 01/23/2012 (szhu)
*/
/************************************************************************************F*/
int32_t DirtyCertCARequestFree(int32_t iRequestId)
{
    DirtyCertRefT *pState = _DirtyCert_pState;
    int32_t iResult;

    // internal request id is public id - 1
    iRequestId -= 1;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertCARequestFree() called while module is not active\n"));
        return(-1);
    }
    // if it's a valid request id
    if ((iRequestId < 0) || (iRequestId >= DIRTYCERT_MAXREQUESTS))
    {
        NetPrintf(("dirtycert: invalid request id (%d)\n", iRequestId));
        return(-2);
    }

    // acquire critical section
    NetCritEnter(&pState->crit);

    // free the request
    iResult = _DirtyCertCARequestFree(pState, &pState->requests[iRequestId], iRequestId);

    // release critical section
    NetCritLeave(&pState->crit);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyCertControl

    \Description
        Set module behavior based on input selector.

    \Input  iControl    - input selector
    \Input  iValue      - selector input
    \Input  iValue2     - selector input
    \Input *pValue      - selector input

    \Output
        int32_t         - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            SELECTOR    DESCRIPTION
            'time'      Sets CA request timeout in milliseconds (default=30s)
        \endverbatim

    \Version 01/24/2012 (szhu)
*/
/********************************************************************************F*/
int32_t DirtyCertControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    DirtyCertRefT *pState = _DirtyCert_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtycert: DirtyCertControl() called while module is not active\n"));
        return(-1);
    }

    // set timeout
    if (iControl == 'time')
    {
        // acquire critical section
        NetCritEnter(&pState->crit);
        NetPrintf(("dirtycert: setting timeout to %d ms\n", iValue));
        pState->uTimeout = (unsigned)iValue;
        // release critical section
        NetCritLeave(&pState->crit);
        return(0);
    }

    // unhandled
    NetPrintf(("dirtycert: unhandled control option '%c%c%c%c'\n",
        (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)iControl));
    return(-1);
}
