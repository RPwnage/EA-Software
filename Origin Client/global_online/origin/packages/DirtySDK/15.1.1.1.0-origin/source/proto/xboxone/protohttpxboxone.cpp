/*H********************************************************************************/
/*!
    \File protohttpxboxone.cpp

    \Description
        This module implements the ProtoHttp API using Microsoft's IXMLHTTPRequest2
        API.

    \Notes
        \verbatim
        There are some differences between the 'normal' version of ProtoHttp and this
        implementation:
            - Certificates are managed by the system, so this module does not 
              implement any of the certificate APIs, which are stubbed.
            - The receive buffer will grow in size when downloading so as not to
              limit the download speed.  The amount of memory utilized will depend
              on the internal update rate and the speed of the transaction, with
              a faster transaction utilizing more memory.
            - The send buffer will grow in size to be able to hold the entire amount
              of data being sent in the transaction.

        Memory Allocation
            Only some of the memory allocated by this module is tracked by the
            DirtyMem API.  The following long-term allocations are not currently
            tracked:
            - CoCreateInstance() used to create the IXMLHttpRequest2() object
            - MakeAndInitialize() used to instantiate the HttpCallback object
            - MakeAndInitialize() used to create the SequentialStream output object

            The following temporary allocations are not currently tracked:
            - Two std::wstring allocations in _ProtoHttpRequest()
            - CoTaskMemAlloc() from pXmlHttp->GetAllResponseHeaders() in
              _ProtoHttpGetResponseHeader()
        \endverbatim

    \Copyright
        Copyright (c) Electronic Arts 2013-2014.

    \Version 06/27/2013 (jbrookes) (initial implementation by akirchner)
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

#include <ctype.h>
#include <exception>
#include <iostream> // std::wstring unicodeUrl()
#if defined(DIRTYCODE_XBOXONE) && !defined(DIRTYCODE_XBOXONEADK_ONLY)
#include <ixmlhttprequest2.h>
#else
#include <Msxml6.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wrl.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/xboxone/dirtyauthxboxone.h"
#include "DirtySDK/dirtyvers.h"
#include "dirtynetpriv.h" // SocketRate*

#include "DirtySDK/proto/protohttp.h"

/*** Defines **********************************************************************/

//! default ProtoHttp timeout
#define PROTOHTTP_TIMEOUT       (30*1000)

//! default maximum allowed redirections
#define PROTOHTTP_MAXREDIRECT   (3)

//! size of "last-received" header cache
#define PROTOHTTP_HDRCACHESIZE  (1024)

//! protohttp revision number (maj.min)
#define PROTOHTTP_VERSION       (0x0103)          // update this for major bug fixes or protocol additions/changes

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// forward declaration of xmlhttp object type
typedef struct ProtoHttpXmlHttpT ProtoHttpXmlHttp;

class HttpCallback : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IXMLHTTPRequest2Callback>
{
    public:
        HttpCallback();
        virtual ~HttpCallback();

        STDMETHODIMP OnRedirect        (IXMLHTTPRequest2 *pXHR, const WCHAR *pwszRedirectUrl);
        STDMETHODIMP OnHeadersAvailable(IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const WCHAR *pwszStatus);
        STDMETHODIMP OnDataAvailable   (IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream);
        STDMETHODIMP OnResponseReceived(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream);
        STDMETHODIMP OnError           (IXMLHTTPRequest2 *pXHR, HRESULT hrError);

        ProtoHttpXmlHttpT *GetXmlHttp();
        void SetXmlHttp(ProtoHttpXmlHttpT *pXmlHttp);

    private:
        ProtoHttpXmlHttpT *m_pXmlHttp;
};

class SequentialStream : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, ISequentialStream>
{
    public:
        SequentialStream();
        virtual ~SequentialStream();

        HRESULT STDMETHODCALLTYPE Read (void *pBuffer, ULONG uMaxBytes, ULONG *uBytesRead);
        HRESULT STDMETHODCALLTYPE Write(const void *pBuffer, ULONG uMaxBytes, ULONG *uBytesWritten);

        ProtoHttpRefT * GetProtoHttpRef() { return(m_pState); }
        void SetProtoHttpRef(ProtoHttpRefT *pState) { m_pState = pState; }
        int32_t GetDataSize() { return(m_uBufferPositionWrite - m_uBufferPositionRead); }
        char *GetData() { return(m_pBuffer); }

    protected:
        NetCritT  m_StreamCrit;
        char     *m_pBuffer;
        ULONG     m_uBufferSize;
        ULONG     m_uBufferPositionWrite;
        ULONG     m_uBufferPositionRead;

    private:
        ProtoHttpRefT *m_pState; //$$ todo - get rid of this, only used for debug printing

};

typedef enum ProtoHttpStateE
{
    ST_IDLE,        //!< idle
    ST_AUTH,        //!< acquiring auth token
    ST_CONN,        //!< connecting state
    ST_RESP,        //!< sending, waiting for initial response
    ST_HEAD,        //!< processing header
    ST_BODY,        //!< receiving body
    ST_DONE,        //!< transaction success
    ST_FAIL,        //!< transaction failed

    ST_NUMSTATES    //!< number of states
}ProtoHttpStateE;

typedef struct ProtoHttpHeaderT
{
    int32_t iHdrCode;                                   //!< result code
    int32_t iHdrDate;                                   //!< last modified date
    int32_t iHeadSize;                                  //!< size of head data
    int64_t iBodySize;                                  //!< size of body data
    char strHdr[PROTOHTTP_HDRCACHESIZE];                //!< storage for most recently received HTTP header
}ProtoHttpHeaderT;

typedef struct ProtoHttpBufferT
{
    char *pInpBuf;                                      //!< pointer to input data
    int32_t iInpLen;                                    //!< total length of the URL in buffer
    int32_t iInpOvr;                                    //!< input overflow amount
    int32_t iInpMax;                                    //!< maximum buffer size
    int32_t iInpMaxIdeal;                               //!< requested (ideal) max size; (iInpMax may increase during receive operations)
    int32_t iInpTotal;                                  //!< total amount of data streamed through response buffer
}ProtoHttpBufferT;

// forward-declared earlier in the file
struct ProtoHttpXmlHttpT
{
    Microsoft::WRL::ComPtr<IXMLHTTPRequest2> cpXmlHttp;     //!< pointer to IXMLHTTPRequest2 object
    Microsoft::WRL::ComPtr<HttpCallback> cpXmlHttpCallback; //!< callback functor
    Microsoft::WRL::ComPtr<SequentialStream> cpOutStream;   //!< output stream (optional; used to send data)

    ProtoHttpRefT *pState;                                  //!< http state this is assocated with

    int32_t iMemGroup;                                      //!< module mem group id (required for delayed destroy)
    void *pMemGroupUserData;                                //!< user data associated with mem group (required for delayed destroy)

    uint8_t bStarted;                                       //!< transaction was started; must be marked complete before it can be cleaned up
    uint8_t bSent;
    uint8_t bComplete;                                      //!< transaction complete and can be cleaned up
    uint8_t bAborted;                                       //!< transaction has been aborted
    uint8_t bDestroyed;                                     //!< xmlhttp object was destroyed (used to signal to callbacks to skip any processing when fired)
    int8_t  iVerbose;                                       //!< copy of state's verbose setting
    uint8_t bSpammedShutdown;                               //!< debug counter to only spam shutdown spam once
};

typedef struct ProtoHttpCriticalT
{
    NetCritT Locker;                                        //!< mutex used to protect the following members
    int32_t iNumRedirect;                                   //!< number of redirections received
    int32_t iNumRedirProc;                                  //!< number of redirections processed
    ProtoHttpStateE eRequestState;                          //!< request state
    int32_t iRequestSize;                                   //!< total request size
    ProtoHttpBufferT responseBuffer;                        //!< response body received
    ProtoHttpXmlHttpT *pXmlHttp;                            //!< xmlhttp object used for transaction
    int8_t bBodyReceived;                                   //!< TRUE if the entire body was received. FALSE otherwise
    uint8_t bTimeout;                                       //!< boolean indicating whether a timeout occurred or not
    uint8_t bAbortingRedirection;                           //!< TRUE if we are aborting a redirection
    uint8_t bHeaderReceived;                                //!< TRUE if header has been received
    uint8_t bRedirBugWorkaround;                            //!< TRUE if redirection bug workaround is being enabled
}ProtoHttpCriticalT;

//! http module state
struct ProtoHttpRefT
{
    int32_t iMemGroup;                                  //!< module mem group id
    void *pMemGroupUserData;                            //!< user data associated with mem group

    ProtoHttpCustomHeaderCbT  *pCustomHeaderCb;         //!< callback for modifying request header
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;        //!< callback for viewing recv header on recepit
    void *pCallbackRef;                                 //!< user ref for callback

    ProtoHttpWriteCbT *pWriteCb;                        //!< optional data write callback
    void *pWriteCbUserData;                             //!< user data for write callback

    int32_t iPort;                                      //!< server port
    int32_t iBasePort;                                  //!< base port (used for partial urls)
    int32_t iSecure;                                    //!< secure connection
    int32_t iBaseSecure;                                //!< base security setting (used for partial urls)
    int32_t iRecvSize;                                  //!< protohttprecvall variable to track received size
    int32_t iPendingXmlHttpDestructions;                //!< number of xml http object destructions still queued with netconn external cleanup

    char *pAppendHdr;                                   //!< append header buffer pointer
    int32_t iAppendLen;                                 //!< size of append header buffer

    ProtoHttpRequestTypeE eRequestType;                 //!< request type of current request
    ProtoHttpHeaderT responseHeader;                    //!< response header received
    ProtoHttpCriticalT critical;                        //!< variables that should be thread safe

    int8_t bAuthenticate;                               //!< if enable authentication if TRUE, and disable it if FALSE
    int8_t iVerbose;                                    //!< debug output level
    uint8_t bGetAuthToken;                              //!< if TRUE get and embed auth token
    int8_t iAuthTokenIdx;                               //!< index of user acquiring the auth token
    uint8_t bRetriedOpen;                               //!< work-around for OR_INVALID_OXID bug
    uint8_t _pad[3];
    int32_t iOnDataThreshold;                           //!< ON_DATA_THRESHOLD setting

    SocketRateT SendRate;                               //!< send rate estimation data
    SocketRateT RecvRate;                               //!< recv rate estimation data

    int32_t iMaxRedirect;                               //!< maximum number of redirections allowed
    int32_t iKeepAlive;                                 //!< indicate if we should try to use keep-alive
    int32_t iKeepAliveDflt;                             //!< keep-alive default (keep-alive will be reset to this value; can be overridden by user)
    uint32_t uTimer;                                    //!< timeout timer
    uint32_t uTimeout;                                  //!< protocol timeout

    char strTokenUrl[256];                              //!< token uri, in case different from hostname
    char strHost[256];                                  //!< server name
    char strBaseHost[256];                              //!< base server name (used for partial urls)
    char strProxy[256];                                 //!< proxy server name/address (including port)

    char strRequestHdr[PROTOHTTP_HDRCACHESIZE];         //!< storage for most recent HTTP request header
    char strLocation[1024];                             //!< stores most recent redirection header; used to work around 302/303 POST redirection bug in IXHR2
};

/*** Function Prototypes **********************************************************/

static int32_t _ProtoHttpAbort(ProtoHttpRefT *pState);
static int32_t _ProtoHttpGetResponseHeader(ProtoHttpRefT *pState, int32_t iHdrCode, const wchar_t *pwStrCode);
static int32_t _ProtoHttpRead(ProtoHttpRefT *pState, ISequentialStream *pStream, char *pBuffer, int32_t iMaxRead);

/*** Variables ********************************************************************/

#if DIRTYCODE_LOGGING
//! names of request states for debug use
static const char *_ProtoHttp_strRequestState[ST_NUMSTATES] =
{
    "ST_IDLE",
    "ST_AUTH",
    "ST_CONN",
    "ST_RESP",
    "ST_HEAD",
    "ST_BODY",
    "ST_DONE",
    "ST_FAIL",
};
#endif

//! names of HTTP request types
static const char *_ProtoHttp_strRequestType[PROTOHTTP_NUMREQUESTTYPES] =
{
    "HEAD",
    "GET",
    "POST",
    "PUT",
    "DELETE",
    "OPTIONS"
};

/*** Private Functions ************************************************************/

//$$ tmp - we have to define this to allow building this module in WinRT
#if defined(DIRTYCODE_PC)
const char *DirtyErrGetName(uint32_t uError)
{
    static char _strError[32];
    ds_snzprintf(_strError, sizeof(_strError), "0x%08x");
    return(_strError);
}
#endif

/*
    HttpCallback: implementation of IXMLHTTPRequest2Callback interface required by
    IXMLHTTPGetRequest2().
*/

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function HttpCallback

    \Description
        HttpCallback constructor

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
HttpCallback::HttpCallback() : m_pXmlHttp(NULL)
{
}

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function HttpCallback

    \Description
        HttpCallback destructor

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
HttpCallback::~HttpCallback()
{
}

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function OnRedirect

    \Description
        The requested URI was redirected by the HTTP server to a new URI.

    \Input *pXHR         - The interface pointer of originating IXMLHTTPRequest2 object.
    \Input *pRedirectUrl - The new URL to for the request.

    \Output
        IFACEMETHODIMP   - S_OK = success

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
IFACEMETHODIMP HttpCallback::OnRedirect(IXMLHTTPRequest2* pXHR, const wchar_t *pRedirectUrl)
{
    ProtoHttpXmlHttpT *pXmlHttp = GetXmlHttp();
    ProtoHttpRefT *pState = pXmlHttp->pState;
    ProtoHttpBufferT *pBuffer;
    char *pTmp;

    NetPrintfVerbose((pXmlHttp->iVerbose, 1, "protohttpxboxone: [%p] OnRedirect() url='%S'\n", pState, pRedirectUrl));

    NetCritEnter(&pState->critical.Locker);

    if (!pXmlHttp->bDestroyed)
    {
        pBuffer = &pState->critical.responseBuffer;

        // format null-terminated redirection response into input buffer for processing in update
        pState->critical.responseBuffer.iInpLen += ds_snzprintf(pBuffer->pInpBuf+pBuffer->iInpLen, pBuffer->iInpMax-pBuffer->iInpLen,
            "HTTP/1.1 302 Found\r\n"
            "Location: %S\r\n"
            "\r\n",
            pRedirectUrl);
        pState->critical.responseBuffer.pInpBuf[pState->critical.responseBuffer.iInpLen++] = '\0';

        // copy to temp location; in case we get a subsequent OnError and we need to re-issue the redirect ourselves
        wcstombs(pState->strLocation, pRedirectUrl, sizeof(pState->strLocation));

        // if location URL includes a '#' character, trim it
        if ((pTmp = strchr(pState->strLocation, '#')) != NULL)
        {
            *pTmp = '\0';
        }

        // check max redirection limit
        if (pState->critical.iNumRedirect == pState->iMaxRedirect)
        {
            NetPrintf(("protohttpxboxone: [%p] maximum number of redirections (%d) exceeded\n", pState, pState->iMaxRedirect));
            pState->critical.bAbortingRedirection = TRUE;
            _ProtoHttpAbort(pState);
             // processing will complete in the OnError(E_ABORT) callback response
        }
        else // track redirection
        {
            pState->critical.iNumRedirect += 1;
        }
    }

    NetCritLeave(&pState->critical.Locker);
    return(S_OK);
};

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function OnHeadersAvailable

    \Description
        The HTTP Headers have been downloaded and are ready for parsing. The string that is
        returned is owned by this function and should be copied or deleted before exit.

    \Input *pXHR       - The interface pointer of originating IXMLHTTPRequest2 object.
    \Input dwStatus    - The value of HTTP status code, e.g. 200, 404
    \Input *pwszStatus - The description text of HTTP status code.

    \Output
        IFACEMETHODIMP - S_OK = success

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
IFACEMETHODIMP HttpCallback::OnHeadersAvailable(IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const wchar_t *pwszStatus)
{
    ProtoHttpXmlHttpT *pXmlHttp = GetXmlHttp();
    ProtoHttpRefT *pState = pXmlHttp->pState;

    NetPrintfVerbose((pXmlHttp->iVerbose, 1, "protohttpxboxone: [%p] OnHeadersAvailable(%d, %S)\n", pState, dwStatus, pwszStatus));

    NetCritEnter(&pState->critical.Locker);

    if (!pXmlHttp->bDestroyed)
    {
        // get response header
        if (_ProtoHttpGetResponseHeader(pState, dwStatus, pwszStatus) == 0)
        {
            // indicate we've received the header
            pState->critical.bHeaderReceived = TRUE;
        }
        else
        {
            pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
        }
    }

    NetCritLeave(&pState->critical.Locker);

    return(S_OK);
}

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function OnDataAvailable

    \Description
        Part of the HTTP Data payload is available, we can start processing it
        here or copy it off and wait for the whole request to finish loading.

    \Input *pXHR            - Pointer to the originating IXMLHTTPRequest2 object.
    \Input *pResponseStream - Pointer to the input stream, which may only be part of the whole stream.

    \Output
        IFACEMETHODIMP      - S_OK = success

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
IFACEMETHODIMP HttpCallback::OnDataAvailable(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream)
{
    ProtoHttpXmlHttpT *pXmlHttp = GetXmlHttp();
    ProtoHttpRefT *pState = pXmlHttp->pState;
    ProtoHttpBufferT *pRespBuf;
    int32_t iOnDataThresholdMargin;
    int32_t iBytesRead, iMaxRead, iBytesAvail;
    uint32_t uSleepCount = 0;

    NetPrintfVerbose((pXmlHttp->iVerbose, 1, "protohttpxboxone: [%p] OnDataAvailable(%p)\n", pState, pResponseStream));

    NetCritEnter(&pState->critical.Locker);

    if (!pXmlHttp->bDestroyed)
    {
        pRespBuf = &pState->critical.responseBuffer;

        /* MS suggests limiting the maximum read size to slightly over the ondatathreshold size; this improves
           performance by limiting the amount of buffer space that needs to be marshalled across VM boundaries,
           while still allowing us to pull the maximum amount of data available in one read */
        iOnDataThresholdMargin = (pState->iOnDataThreshold > 1024) ? pState->iOnDataThreshold + pState->iOnDataThreshold/8 : 1024;
        
        do
        {
            // calc max read and realloc buffer if no room to receive
            // $$TODO -- change to sleep here instead, like with rate throttling below?
            if ((iMaxRead = pRespBuf->iInpMax - pRespBuf->iInpLen) == 0)
            {
                int32_t iNewMax = pRespBuf->iInpMax * 2;
                NetPrintfVerbose((pXmlHttp->iVerbose, 1, "protohttpxboxone: [%p] reallocing input buffer (%d->%d)\n", pState, pRespBuf->iInpMax, iNewMax));
                char *pNewBuf = (char *)DirtyMemAlloc(iNewMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
                ds_memcpy(pNewBuf, pRespBuf->pInpBuf, pRespBuf->iInpLen);
                DirtyMemFree(pRespBuf->pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
                pRespBuf->pInpBuf = pNewBuf;
                pRespBuf->iInpMax = iNewMax;
                iMaxRead = pRespBuf->iInpMax - pRespBuf->iInpLen;
            }

            // limit max read size based on threshold margin
            if (iMaxRead > iOnDataThresholdMargin)
            {
                iMaxRead = iOnDataThresholdMargin;
            }

            // handle optional data rate throttling
            while ((iBytesAvail = SocketRateThrottle(&pState->RecvRate, SOCK_STREAM, iMaxRead, "recv")) == 0)
            {
                NetCritLeave(&pState->critical.Locker);
                Sleep(16);
                uSleepCount += 1;
                NetCritEnter(&pState->critical.Locker);
            }

            // do the read, update buffer info
            if ((iBytesRead = _ProtoHttpRead(pState, pResponseStream, pRespBuf->pInpBuf + pRespBuf->iInpLen, iBytesAvail)) > 0)
            {
                NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] recv %d bytes\n", pState, iBytesRead));
            }
            pRespBuf->iInpLen += iBytesRead;
            pRespBuf->iInpTotal += iBytesRead;

            // update data rate estimation
            SocketRateUpdate(&pState->RecvRate, iBytesRead, "recv");
        }
        /* as per MS, only continue to read if we received the maximum amount the
           last time around; otherwise we know there is no data to be read */
        while (iBytesRead == iBytesAvail);
    }

    NetCritLeave(&pState->critical.Locker);

    return(S_OK);
}

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function OnResponseReceived

    \Description
        Called when the entire body has been received.

    \Input *pXHR            - Pointer to the originating IXMLHTTPRequest2 object.
    \Input *pResponseStream - Pointer to the complete input stream.

    \Output
        IFACEMETHODIMP      - S_OK = success

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
IFACEMETHODIMP HttpCallback::OnResponseReceived(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream)
{
    ProtoHttpXmlHttpT *pXmlHttp = GetXmlHttp();
    ProtoHttpRefT *pState = pXmlHttp->pState;

    NetPrintfVerbose((pXmlHttp->iVerbose, 1, "protohttpxboxone: [%p] OnResponseReceived(%p)\n", pState, pResponseStream));

    // forward data on to data callback
    OnDataAvailable(pXHR, pResponseStream);

    // make sure we have a state reference
    if (pState != NULL)
    {
        NetCritEnter(&pState->critical.Locker);
        pState->critical.bBodyReceived = TRUE;
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] finished receiving data\n", pState));
        NetCritLeave(&pState->critical.Locker);
    }

    // mark as complete; so we can clean up the ref
    pXmlHttp->bComplete = TRUE;
    return(S_OK);
}

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function OnError

    \Description
        Handle errors that have occurred during the HTTP request

    \Input *pXHR       - The interface pointer of IXMLHTTPRequest2 object
    \Input hError       - The errorcode for the httprequest

    \Output
        IFACEMETHODIMP - S_OK = success

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
IFACEMETHODIMP HttpCallback::OnError(IXMLHTTPRequest2 *pXHR, HRESULT hError)
{
    ProtoHttpXmlHttpT *pXmlHttp = GetXmlHttp();
    ProtoHttpRefT *pState = pXmlHttp->pState;

    //NetPrintfVerbose((pXmlHttp->iVerbose, 1, "protohttpxboxone: [%p] OnError (%s)\n", pState, DirtyErrGetName(hError)));
    NetPrintf(("protohttpxboxone: [%p] OnError (%s)\n", pState, DirtyErrGetName(hError)));

    // make sure we have a state reference
    if (pState != NULL)
    {
        /* $$TODO - we can't acquire the critical section because pXmlHttp->Abort() seems to issue the OnError
            callback and then block until completion.  we have already acquired our critical section before calling
            Abort() and the OnError callback is delegated to a separate thread, so this results in a deadloock */
        //NetCritEnter(&pState->critical.Locker);

        if ((pState->iMaxRedirect == 0) && (hError == E_ABORT) && pState->critical.bAbortingRedirection)
        {
            NetPrintf(("protohttpxboxone: [%p] completed abort of redirection\n", pState));
            pState->critical.bHeaderReceived = TRUE;
            //pState->critical.eRequestState = ProtoHttpStateE::ST_HEAD;
            pState->critical.bBodyReceived = TRUE;
        }
        else if (((pState->eRequestType == PROTOHTTP_REQUESTTYPE_POST) || (pState->eRequestType == PROTOHTTP_REQUESTTYPE_PUT)) &&
                (hError == INET_E_REDIRECT_FAILED) && (pState->strLocation[0] != '\0'))
        {
            NetPrintf(("protohttpxboxone: [%p] setting up for re-issue of redirection after failure\n", pState));
            pState->critical.bRedirBugWorkaround = TRUE;
        }
        else
        {
            pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
        }

        //pState->critical.iNumRedirect = 0;
        //$$todo - remove this - we are doing this ourselves and don't need it
        //pState->critical.bTimeout = (hError == INET_E_CONNECTION_TIMEOUT) ? TRUE : FALSE;

        //NetCritLeave(&pState->critical.Locker);
    }

    // mark as complete, so we can clean up the ref
    pXmlHttp->bComplete = TRUE;

    return(S_OK);
}

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function GetXmlHttp

    \Description
        Get XmlHttp object reference

    \Output
        ProtoHttpXmlHttpT * - Pointer to reference

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
ProtoHttpXmlHttpT * HttpCallback::GetXmlHttp()
{
    return(m_pXmlHttp);
}

/*F********************************************************************************/
/*!
    \relates  HttpCallback
    \Function SetXmlHttp

    \Description
        Set XmlHttp object reference

    \Input
        *pXmlHttp       - xmlhttp object reference

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
void HttpCallback::SetXmlHttp(ProtoHttpXmlHttpT *pXmlHttp)
{
    m_pXmlHttp = pXmlHttp;
}

/*
    SequentialStream: implementation of ISequentialStream interface required by
    IXMLHTTPGetRequest2().
*/

/*F********************************************************************************/
/*!
    \relates  SequentialStream
    \Function SequentialStream

    \Description
        SequentialStream constructor

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
SequentialStream::SequentialStream():
    m_pBuffer(NULL),
    m_uBufferSize(0),
    m_uBufferPositionWrite(0),
    m_uBufferPositionRead(0)
{
    NetCritInit(&m_StreamCrit, "SequentialStream");
};

/*F********************************************************************************/
/*!
    \relates  SequentialStream
    \Function SequentialStream

    \Description
        SequentialStream destructor

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
SequentialStream::~SequentialStream()
{
    if (m_pBuffer)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;

        // query current mem group data
        // $$TODO this should use the protohttp memgroup and memgroupuserdata
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

        DirtyMemFree(m_pBuffer, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData);
    }

    NetCritKill(&m_StreamCrit);
};

/*F********************************************************************************/
/*!
    \relates  SequentialStream
    \Function Read

    \Description
        Reads from the uBufferPositionRead position

    \Input *pData     - [OUT] A pointer to the buffer which the stream data is read into
    \Input uMaxBytes  - The number of bytes of data to read from the stream object
    \Input uBytesRead - [OUT] A pointer to a ULONG variable that receives the actual number of bytes read from the stream object

    \Output
        HRESULT       - S_OK if succeed

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
HRESULT SequentialStream::Read(void *pData, ULONG uMaxBytes, ULONG *uBytesRead)
{
    ProtoHttpRefT *pState = GetProtoHttpRef();
    ULONG uBytesAvail, uBytesAvailMax;
    uint32_t uSleepCount = 0;

    if ((pData == NULL) || (uBytesRead == NULL))
    {
        return(STG_E_INVALIDPOINTER);
    }

    NetCritEnter(&m_StreamCrit);

    // clamp to max
    if ((uBytesAvailMax = (m_uBufferPositionWrite - m_uBufferPositionRead)) > uMaxBytes)
    {
        uBytesAvailMax = uMaxBytes;
    }

    // check for data to read
    if ((m_pBuffer == NULL) || (uBytesAvailMax == 0))
    {
        NetCritLeave(&m_StreamCrit);
        return(S_FALSE);
    }

    // handle optional data rate throttling
    while ((uBytesAvail = SocketRateThrottle(&pState->SendRate, SOCK_STREAM, uBytesAvailMax, "send")) == 0)
    {
        NetCritLeave(&m_StreamCrit);
        Sleep(16);
        uSleepCount += 1;
        NetCritEnter(&m_StreamCrit);
    }

    Sleep(250);

    // copy the data
    if (uBytesAvail > 0)
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] sequentialstream read %d bytes\n", pState, uBytesAvail));
        ds_memcpy(pData, m_pBuffer + m_uBufferPositionRead, uBytesAvail);
        m_uBufferPositionRead += uBytesAvail;
        *uBytesRead = uBytesAvail;

        // update data rate estimation
        SocketRateUpdate(&pState->SendRate, uBytesAvail, "send");

        // compact the buffer
        if ((uBytesAvail = m_uBufferPositionWrite - m_uBufferPositionRead) > 0)
        {
            memmove(m_pBuffer, m_pBuffer + m_uBufferPositionRead, m_uBufferPositionWrite - m_uBufferPositionRead);
        }
        m_uBufferPositionWrite -= m_uBufferPositionRead;
        m_uBufferPositionRead = 0;
    }

    NetCritLeave(&m_StreamCrit);

    // return if there is data available
    return((uBytesAvail > 0) ? S_OK : S_FALSE);
}

/*F********************************************************************************/
/*!
    \relates  SequentialStream
    \Function Write

    \Description
        Writes to the end of the buffer, and increases the buffer size if needed

    \Input *pData           - input data to be written into the stream
    \Input uMaxBytes        - size of data to be written into the stream
    \Input *pBytesWritten   - [OUT] storage for actual number of bytes that were written

    \Output
        HRESULT             - S_OK if succeed

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
HRESULT SequentialStream::Write(const void *pData, ULONG uMaxBytes, ULONG *pBytesWritten)
{
#if DIRTYCODE_LOGGING
    ProtoHttpRefT *pState = GetProtoHttpRef();
#endif

    NetCritEnter(&m_StreamCrit);

    if (pData == NULL)
    {
        NetCritLeave(&m_StreamCrit);
        return(STG_E_INVALIDPOINTER);
    }
    ULONG uBufferSizeNew = m_uBufferSize;

    // find out if the size of the buffer should be increased
    while (uBufferSizeNew < (m_uBufferPositionWrite + uMaxBytes))
    {
        if (uBufferSizeNew == 0)
        {
            uBufferSizeNew = 1024;
        }
        else
        {
            uBufferSizeNew *= 2;
        }
    }

    // increase the size of the buffer if needed
    if (m_uBufferSize < uBufferSizeNew)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;
        char *pBufferNew;

        // query current mem group data
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

        // allocate new buffer
        if ((pBufferNew = (char *)DirtyMemAlloc(uBufferSizeNew, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
        {
            NetPrintf(("protohttpxboxone: [%p] sequentialstream [%p] Failed to increase sequential stream buffer size\n", pState, this));
            NetCritLeave(&m_StreamCrit);
            return(STG_E_MEDIUMFULL);
        }

        // if we had an old buffer, copy information over and free it
        if (m_pBuffer != NULL)
        {
            memset(pBufferNew, 0, uBufferSizeNew);
            ds_memcpy(pBufferNew, m_pBuffer, m_uBufferSize);
            DirtyMemFree(m_pBuffer, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData);
        }

        NetPrintfVerbose((pState->iVerbose, 2, "protohttpxboxone: [%p] realloc sequentialstream [%p] %d->%d bytes\n", pState, this, m_uBufferSize, uBufferSizeNew));

        m_pBuffer = pBufferNew;
        m_uBufferSize = uBufferSizeNew;
    }

    // append new data
    NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] sequentialstream [%p] write %d bytes\n", pState, this, uMaxBytes));
    ds_memcpy(m_pBuffer + m_uBufferPositionWrite, pData, uMaxBytes);
    m_uBufferPositionWrite += uMaxBytes;
    NetCritLeave(&m_StreamCrit);

    if (pBytesWritten !=  NULL)
    {
        *pBytesWritten = uMaxBytes;
    }
    return(S_OK);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpXmlHttpIsActive

    \Description
        Returns whether XmlHttp is active or not

    \Input *pState      - http module state (may be null during delayed destroy)
    \Input *pXmlHttp    - xmlhttp object pointer

    \Output
        int32_t         - FALSE if not active, else TRUE

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpXmlHttpIsActive(ProtoHttpRefT *pState, ProtoHttpXmlHttpT *pXmlHttp)
{
#if DIRTYCODE_LOGGING
    int32_t iVerbose = (pState != NULL) ? pState->iVerbose : pXmlHttp->bSpammedShutdown ? 0 : 2;
#endif

    // is there a transaction allocated?
    if (pXmlHttp == NULL)
    {
        NetPrintfVerbose((iVerbose, 1, "protohttpxboxone: [%p] xmlhttp [%p] inactive (no transaction allocated)\n", pState, pXmlHttp));
        return(FALSE);
    }
    // is our transaction not yet started, or has been started and has finished?
    if (!pXmlHttp->bStarted)
    {
        NetPrintfVerbose((iVerbose, 1, "protohttpxboxone: [%p] xmlhttp [%p] inactive (transaction not started)\n", pState, pXmlHttp));
        return(FALSE);
    }
    // has our transaction completed?
    if (pXmlHttp->bComplete)
    {
        NetPrintfVerbose((iVerbose, 1, "protohttpxboxone: [%p] xmlhttp [%p] inactive (transaction complete)\n", pState, pXmlHttp));
        return(FALSE);
    }
    // transaction is active
    NetPrintfVerbose((iVerbose, 1, "protohttpxboxone: [%p] xmlhttp [%p] active\n", pState, pXmlHttp));
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpAbort

    \Description
        Abort current operation, if any.

    \Input *pState      - reference pointer

    \Output
        int32_t         - negative = failure, zero = success

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpAbort(ProtoHttpRefT *pState)
{
    HRESULT hResult;

    // if we're not active, or have already aborted, no need to abort
    if (!_ProtoHttpXmlHttpIsActive(pState, pState->critical.pXmlHttp) || pState->critical.pXmlHttp->bAborted)
    {
        return(0);
    }

    // mark that we've tried to abort
    pState->critical.pXmlHttp->bAborted = TRUE;

    // do the abort
    //NetPrintfVerbose((pState->iVerbose, 0, "protohttpxboxone: [%p] abort (state=%s)\n", pState, _ProtoHttp_strRequestState[pState->critical.eRequestState]));
    NetPrintf(("protohttpxboxone: [%p] abort (state=%s)\n", pState, _ProtoHttp_strRequestState[pState->critical.eRequestState]));
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->Abort()) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to abort ongoing request (err=%s)\n", pState, DirtyErrGetName(hResult)));
        pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
        return(-1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpApplyBaseUrl

    \Description
        Apply base url elements (if set) to any url elements not specified (relative
        url support).

    \Input *pState      - module state
    \Input *pKind       - parsed http kind ("http" or "https")
    \Input *pHost       - [in/out] parsed URL host
    \Input iHostSize    - size of pHost buffer
    \Input *pPort       - [in/out] parsed port
    \Input *pSecure     - [in/out] parsed security (0 or 1)
    \Input bPortSpecified - TRUE if a port is explicitly specified in the url, else FALSE

    \Output
        uint32_t        - non-zero if changed, else zero

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ProtoHttpApplyBaseUrl(ProtoHttpRefT *pState, const char *pKind, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure, uint8_t bPortSpecified)
{
    uint8_t bChanged = FALSE;
    if ((*pHost == '\0') && (pState->strBaseHost[0] != '\0'))
    {
        NetPrintf(("protohttpxboxone: [%p] host not present; setting to %s\n", pState, pState->strBaseHost));
        ds_strnzcpy(pHost, pState->strBaseHost, iHostSize);
        bChanged = TRUE;
    }
    if ((bPortSpecified == FALSE) && (pState->iBasePort != 0))
    {
        NetPrintf(("protohttpxboxone: [%p] port not present; setting to %d\n", pState, pState->iBasePort));
        *pPort = pState->iBasePort;
        bChanged = TRUE;
    }
    if (*pKind == '\0')
    {
        NetPrintf(("protohttpxboxone: [%p] kind (protocol) not present; setting to %d\n", pState, pState->iBaseSecure));
        *pSecure = pState->iBaseSecure;
        // if our port setting is default and incompatible with our security setting, override it
        if (((*pPort == 80) && (*pSecure == 1)) || ((*pPort == 443) && (*pSecure == 0)))
        {
            *pPort = *pSecure ? 443 : 80;
        }
        bChanged = TRUE;
    }
    return(bChanged);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSetAppendHeader

    \Description
        Set given string as append header, allocating memory as required.

    \Input *pState      - reference pointer
    \Input *pAppendHdr  - append header string

    \Output
        int32_t         - zero=success, else error

    \Version 11/11/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSetAppendHeader(ProtoHttpRefT *pState, const char *pAppendHdr)
{
    int32_t iAppendBufLen, iAppendStrLen;

    // check for empty append string, in which case we free the buffer
    if ((pAppendHdr == NULL) || (*pAppendHdr == '\0'))
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->pAppendHdr = NULL;
        }
        pState->iAppendLen = 0;
        return(0);
    }

    // check to see if append header is already set
    if ((pState->pAppendHdr != NULL) && (!strcmp(pAppendHdr, pState->pAppendHdr)))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] ignoring set of append header '%s' that is already set\n", pState, pAppendHdr));
        return(0);
    }

    // get append header length
    iAppendStrLen = (int32_t)strlen(pAppendHdr);
    // append buffer size includes null and space for \r\n if not included by submitter
    iAppendBufLen = iAppendStrLen + 3;

    // see if we need to allocate a new buffer
    if (iAppendBufLen > pState->iAppendLen)
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
        if ((pState->pAppendHdr = (char *)DirtyMemAlloc(iAppendBufLen, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) != NULL)
        {
            pState->iAppendLen = iAppendBufLen;
        }
        else
        {
            NetPrintf(("protohttpxboxone: [%p] could not allocate %d byte buffer for append header\n", pState, iAppendBufLen));
            pState->iAppendLen = 0;
            return(-1);
        }
    }

    // copy append header
    ds_strnzcpy(pState->pAppendHdr, pAppendHdr, iAppendStrLen+1);

    // if append header is not \r\n terminated, do it here
    if (pState->pAppendHdr[iAppendStrLen-2] != '\r' || pState->pAppendHdr[iAppendStrLen-1] != '\n')
    {
        ds_strnzcat(pState->pAppendHdr, "\r\n", pState->iAppendLen);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpResizeBuffer

    \Description
        Resize the buffer

    \Input *pState      - reference pointer
    \Input iBufMax      - new buffer size

    \Output
        int32_t         - zero=success, else failure

    \Version 02/21/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpResizeBuffer(ProtoHttpRefT *pState, int32_t iBufMax)
{
    int32_t iCopySize;
    char *pInpBuf;

    NetCritEnter(&pState->critical.Locker);

    NetPrintf(("protohttpxboxone: [%p] resizing input buffer from %d to %d bytes\n", pState, pState->critical.responseBuffer.iInpMax, iBufMax));
    if ((pInpBuf = (char *)DirtyMemAlloc(iBufMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpxboxone: [%p] could not resize input buffer\n", pState));
        NetCritLeave(&pState->critical.Locker);
        return(-1);
    }

    // calculate size of data to copy from old buffer to new
    if ((iCopySize = pState->critical.responseBuffer.iInpLen) > iBufMax)
    {
        NetPrintf(("protohttpxboxone: [%p] warning; resize of input buffer is losing %d bytes of data\n", pState, iCopySize - iBufMax));
        iCopySize = iBufMax;
    }
    // copy valid contents of input buffer, if any, to new buffer
    ds_memcpy(pInpBuf, pState->critical.responseBuffer.pInpBuf, iCopySize);

    // dispose of old buffer
    DirtyMemFree(pState->critical.responseBuffer.pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // update buffer variables
    pState->critical.responseBuffer.pInpBuf = pInpBuf;
    pState->critical.responseBuffer.iInpMax = iBufMax;

    // clear overflow count
    pState->critical.responseBuffer.iInpOvr = 0;

    NetCritLeave(&pState->critical.Locker);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpResizeBufferAuto

    \Description
        Resize the buffer, sizing based on increments of original buffer max.
        This function serves a similar function to what httpmanager does with
        normal protohttp.

    \Input *pState      - reference pointer
    \Input iBufMax      - requested buffer max

    \Output
        int32_t         - zero=success, else failure

    \Version 07/31/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpResizeBufferAuto(ProtoHttpRefT *pState, int32_t iBufMax)
{
    int32_t iNewSize; 

    // calc new buffer size in increments of original buffer size
    for (iNewSize = pState->critical.responseBuffer.iInpMax; iNewSize < iBufMax; iNewSize += pState->critical.responseBuffer.iInpMax)
        ;

    // realloc buffer to allow for larger size
    return(_ProtoHttpResizeBuffer(pState, iNewSize));
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatUrl

    \Description
        This function builds the URL based on on values aquired on values cached by
        previous function calls

    \Input *pState      - reference pointer
    \Input *pUrl        - pointer to user-supplied url

    \Output
        int32_t         - negative for error, and ZERO for success

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpFormatUrl(ProtoHttpRefT *pState, const char *pUrl)
{
    char strHost[sizeof(pState->strHost)], strKind[32];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // if no url specified use the minimum
    if (*pUrl == '\0')
    {
        pUrl = "/";
    }

    // parse the url for kind, host, and port
    if (pState->strProxy[0] == '\0')
    {
        pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
    }
    else
    {
        NetPrintf(("protohttpxboxone: [%p] using proxy server %s\n", pState, pState->strProxy));
        ProtoHttpUrlParse2(pState->strProxy, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
    }

    // determine if host, port, or security settings have changed since the previous request
    if ((iSecure != pState->iSecure) || (ds_stricmp(strHost, pState->strHost) != 0) || (iPort != pState->iPort))
    {
        pState->iKeepAlive = pState->iKeepAliveDflt;
    }

    // fill in any missing info (relative url) if available
    if (_ProtoHttpApplyBaseUrl(pState, strKind, strHost, sizeof(strHost), &iPort, &iSecure, bPortSpecified) != 0)
    {
        NetPrintf(("protohttpxboxone: [%p] %s://%s:%d%s\n", pState, iSecure ? "https" : "http", strHost, iPort, pUrl));
    }

    // determine if host, port, or security settings have changed since the previous request
    if ((iSecure != pState->iSecure) || (ds_stricmp(strHost, pState->strHost) != 0) || (iPort != pState->iPort))
    {
        // save new server/port/security state
        ds_strnzcpy(pState->strHost, strHost, sizeof(pState->strHost));
        pState->iPort = iPort;
        pState->iSecure = iSecure;
    }

    // save request type and url in input buffer
    pState->critical.responseBuffer.iInpLen = ds_snzprintf(pState->critical.responseBuffer.pInpBuf, pState->critical.responseBuffer.iInpMax,
        "%s %s", _ProtoHttp_strRequestType[pState->eRequestType], pUrl);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSetRequestHeader

    \Description
        Set a request header into IXMLHTTPRequest2()

    \Input  *pState     - reference pointer
    \Input  *pHeader    - unicode header name
    \Input  *pValue     - header value (not unicode)

    \Version 06/28/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpSetRequestHeader(ProtoHttpRefT *pState, const wchar_t *pHeader, const char *pValue)
{
    wchar_t *pWideStrBuf;
    int32_t iWideStrBufSize;
    HRESULT hResult;

    // allocate memory for converting the header value to unicode
    iWideStrBufSize = (signed) ((strlen(pValue) + 1) *  sizeof(wchar_t));
    if ((pWideStrBuf = (wchar_t *)DirtyMemAlloc(iWideStrBufSize, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpxboxone: [%p] request header failure - unable to allocate (%d bytes) for mbs to wcs conversion buffer\n", pState, iWideStrBufSize));
        return;
    }

    // convert to wc
    MultiByteToWideChar(CP_UTF8, 0, pValue, -1, pWideStrBuf, iWideStrBufSize);

    // set the request header
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->SetRequestHeader(pHeader, pWideStrBuf)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: could not set request header '%s' (err=%s)\n", pValue, DirtyErrGetName(hResult)));
    }

    DirtyMemFree(pWideStrBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSetRequestHeaders

    \Description
        Set all request headers into IXMLHTTPRequest2

    \Input *pState      - reference pointer
    \Input *pHdrBuf     - pointer to headers
    \Input *pStrTempBuf - temp work buffer
    \Input iBufLen      - size of temp work buffer

    \Version 07/31/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpSetRequestHeaders(ProtoHttpRefT *pState, const char *pHdrBuf, char *pStrTempBuf, int32_t iBufLen)
{
    wchar_t wstrHeaderName[256];
    char strHeaderName[256];

    for ( ; ProtoHttpGetNextHeader(pState, pHdrBuf, strHeaderName, sizeof(strHeaderName), pStrTempBuf, iBufLen, &pHdrBuf) == 0; )
    {
        MultiByteToWideChar(CP_UTF8, 0, strHeaderName, -1, wstrHeaderName, sizeof(wstrHeaderName));
        _ProtoHttpSetRequestHeader(pState, wstrHeaderName, pStrTempBuf);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRead

    \Description
        Read out of SequentialStream into provided buffer

    \Input  *pState     - reference pointer
    \Input  *pStream    - stream to read from
    \Input  *pBuffer    - [out] buffer to read into
    \Input  iMaxRead    - max buffer size

    \Output
        int32_t         - negative=error, else number of bytes read

    \Version 06/27/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRead(ProtoHttpRefT *pState, ISequentialStream *pStream, char *pBuffer, int32_t iMaxRead)
{
    ULONG uBytesRead = 0; 

    NetCritEnter(&pState->critical.Locker);

    // check for space to read into
    if ((pBuffer != NULL) && (iMaxRead > 0))
    {
        HRESULT hResult = pStream->Read(pBuffer, iMaxRead, &uBytesRead);
        if (SUCCEEDED(hResult) && (uBytesRead > 0))
        {
            #if DIRTYCODE_LOGGING
            if (pState->iVerbose > 2)
            {
                NetPrintf(("protohttpxenon: [%p] recv %d bytes\n", pState, uBytesRead));
            }
            if (pState->iVerbose > 3)
            {
                NetPrintMem(pBuffer, uBytesRead, "http-recv");
            }
            #endif

            // received data, so update timeout
            pState->uTimer = NetTick() + pState->uTimeout;
        }
        else if (FAILED(hResult))
        {
            NetPrintf(("protohttpxboxone: read failed (err=%s)\n", DirtyErrGetName(hResult)));
            uBytesRead = (unsigned)-1;
        }
    }

    NetCritLeave(&pState->critical.Locker);

    return(uBytesRead);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSend

    \Description
        Try and send some data.  If data is sent, the timout value is updated.

    \Input  *pState     - reference pointer
    \Input  *pData      - pointer to buffer to send from
    \Input  iDataSize   - amount of data to try and send

    \Output
        int32_t         - negative=error, else number of bytes sent

    \Version 07/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSend(ProtoHttpRefT *pState, const char *pData, int32_t iDataSize)
{
    ULONG uBytesWritten;
    int32_t iResult = 0;

    // if given data, try to buffer it in output stream
    if (iDataSize > 0)
    {
        if (pState->critical.pXmlHttp->cpOutStream.Get()->Write(pData, iDataSize, &uBytesWritten) == S_OK)
        {
            iResult = (signed)uBytesWritten;

            #if DIRTYCODE_LOGGING
            if (pState->iVerbose > 2)
            {
                NetPrintf(("protohttpxboxone: [%p] sent %d bytes\n", pState, iResult));
            }
            if (pState->iVerbose > 3)
            {
                NetPrintMem(pData, iResult, "http-send");
            }
            #endif

            // sent data, so update timeout
            pState->uTimer = NetTick() + pState->uTimeout;
        }
        else
        {
            NetPrintf(("protohttpxboxone: [%p] failed to write %d bytes to output stream\n", pState, iDataSize));
            return(-1);
        }
    }

    // if we've started the request, and if we've buffered all of the data, do the actual send
    if ((pState->critical.pXmlHttp->bStarted == TRUE) && (pState->critical.pXmlHttp->cpOutStream.Get()->GetDataSize() == pState->critical.iRequestSize))
    {
        HRESULT hResult;

        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] sending request:\n", pState));
        if (pState->iVerbose > 1)
        {
            NetPrintWrap(pState->critical.responseBuffer.pInpBuf, 132);
        }

        if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->Send(pState->critical.pXmlHttp->cpOutStream.Get(), pState->critical.pXmlHttp->cpOutStream.Get()->GetDataSize())) != S_OK)
        {
            NetPrintf(("protohttpxboxone: [%p] failed to send data (err=%s)\n", pState, DirtyErrGetName(hResult)));
            iResult = -2;
        }
        pState->critical.pXmlHttp->bSent = TRUE;
        // reset buffer for input
        pState->critical.responseBuffer.iInpLen = 0;
        // set connection timeout
        pState->uTimer = NetTick() + pState->uTimeout;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatRequestHeader

    \Description
        Format request header, execute custom header callback, and set headers
        into IXMLHTTPRequest2 object.

    \Input *pState      - reference pointer
    \Input *pStrTempBuf - temp work buffer
    \Input iBufLen      - size of temp work buffer

    \Output
        int32_t         - negative=error, else zero

    \Version 07/18/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpFormatRequestHeader(ProtoHttpRefT *pState, char *pStrTempBuf, int32_t iBufLen)
{
    int32_t iInpLen, iInpMax, iResult;
    char *pInpBuf;

    // set up for header formatting
    pInpBuf = pState->critical.responseBuffer.pInpBuf;
    iInpLen = pState->critical.responseBuffer.iInpLen;
    iInpMax = pState->critical.responseBuffer.iInpMax;

    // add HTTP/1.1 and crlf after url
    iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, " HTTP/1.1\r\n");

    // append host (note: MS will supply host, but always with :port, so we override that ourselves)
    if ((pState->iSecure && (pState->iPort == 443)) || (pState->iPort == 80))
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "Host: %s\r\n", pState->strHost);
    }
    else
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "Host: %s:%d\r\n", pState->strHost, pState->iPort);
    }

    // disable keep-alive?
    if (pState->iKeepAlive == 0)
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "Connection: Close\r\n");
    }

    // append user-agent, if not specified in append header
    if ((pState->pAppendHdr == NULL) || !ds_stristr(pState->pAppendHdr, "User-Agent:"))
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen,
            "User-Agent: ProtoHttp %d.%d/DS %d.%d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
            (PROTOHTTP_VERSION>>8)&0xff, PROTOHTTP_VERSION&0xff, DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON,
            DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    }

    // append auth token if enabled
    if (pState->bGetAuthToken)
    {
        int32_t iTokenLen;
        if ((iResult = DirtyAuthCheckToken(-1, pState->strTokenUrl, &iTokenLen, pStrTempBuf)) == DIRTYAUTH_SUCCESS)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] appending xsts token header (len=%d)\n", pState, iTokenLen));
            iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "Authorization: %s\r\n", pStrTempBuf);
        }
        else
        {
            NetPrintf(("protohttpxboxone: [%p] failed to set authorization header (err=%d)\n", pState, iResult));
        }
    }

    // add append headers, if specified
    if ((pState->pAppendHdr != NULL) && (pState->pAppendHdr[0] != '\0'))
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "%s", pState->pAppendHdr);
    }

    // call custom header format callback, if specified
    if (pState->pCustomHeaderCb != NULL)
    {
        const char *pData = (pState->critical.iRequestSize > 0) ? pState->critical.pXmlHttp->cpOutStream->GetData() : NULL;
        if ((iInpLen = pState->pCustomHeaderCb(pState, pInpBuf, iInpMax, pData, pState->critical.iRequestSize, pState->pCallbackRef)) < 0)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttpxboxone: [%p] custom header callback error %d\n", pState, iInpLen));
            return(iInpLen);
        }
        if (iInpLen == 0)
        {
            iInpLen = (int32_t)strlen(pInpBuf);
        }
    }

    // make sure we were able to complete the header
    if (iInpLen > iInpMax)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpxboxone: [%p] not enough buffer to format request header (have %d, need %d)\n", pState, iInpMax, iInpLen));
        pState->critical.responseBuffer.iInpOvr = iInpLen;
        return(PROTOHTTP_MINBUFF);
    }

    // save a copy of the header
    ds_strnzcpy(pState->strRequestHdr, pInpBuf, sizeof(pState->strRequestHdr));

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRequest

    \Description
        Make the request with XMLHttpRequest2

    \Input *pState      - reference pointer

    \Output
        int32_t         - negative=error, ZERO otherwise

    \Version 07/01/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRequest(ProtoHttpRefT *pState)
{
    char strTempBuf[8*1024], strUrlPrefix[300], *pUrlEnd, *pHdrStrt, *pUrl;
    wchar_t wstrRequestType[16], wstrUrlPrefix[300];

    HRESULT hResult;
    int32_t iInpLen, iResult;

    // save input length (url); we use this if we need to retry
    iInpLen = pState->critical.responseBuffer.iInpLen;

    // format request header
    if ((iResult = _ProtoHttpFormatRequestHeader(pState, strTempBuf, sizeof(strTempBuf))) < 0)
    {
        if (iResult == PROTOHTTP_MINBUFF)
        {
            // resize buffer
            _ProtoHttpResizeBufferAuto(pState, pState->critical.responseBuffer.iInpOvr);

            // reset and try again
            pState->critical.responseBuffer.iInpLen = iInpLen;
            iResult = _ProtoHttpFormatRequestHeader(pState, strTempBuf, sizeof(strTempBuf));
        }
        if (iResult < 0)
        {
            return(iResult);
        }
    }

    // find end of request url (note that url may have been modified by custom header callback)
    if ((pUrlEnd = ds_stristr(pState->critical.responseBuffer.pInpBuf, " HTTP/1.1\r\n")) == NULL)
    {
        NetPrintf(("protohttpxboxone: [%p] could not find url terminator in request; aborting\n", pState));
        return(-1);
    }

    #if DIRTYCODE_LOGGING
    if (pState->iVerbose > 0)
    {
        // temporarily null-terminate url for debug output
        char cUrlChar = *pUrlEnd;
        *pUrlEnd = '\0';
        NetPrintf(("protohttpxboxone: [%p] %s (state=%s)\n", pState, pState->critical.responseBuffer.pInpBuf, _ProtoHttp_strRequestState[pState->critical.eRequestState]));
        *pUrlEnd = cUrlChar;
    }
    #endif

    // skip request type to find url start
    for (pUrl = pState->critical.responseBuffer.pInpBuf; *pUrl != ' '; pUrl += 1)
        ;
    pUrl += 1;

    // convert request type to unicode
    MultiByteToWideChar(CP_UTF8, 0, _ProtoHttp_strRequestType[pState->eRequestType], -1, wstrRequestType, sizeof(wstrRequestType));

    // construct url prefix (protocol, hostname, port)
    ds_snzprintf(strUrlPrefix, sizeof(strUrlPrefix), "%s://%s:%d", pState->iSecure ? "https" : "http", pState->strHost, pState->iPort);
    MultiByteToWideChar(CP_UTF8, 0, strUrlPrefix, -1, wstrUrlPrefix, sizeof(wstrUrlPrefix));

    // construct url
    std::wstring unicodeUrl(wstrUrlPrefix);
    std::wstring unicodeUri(pUrl, pUrlEnd);
    unicodeUrl += unicodeUri;

    // after following open call, request must complete before deletion of the xmlhttp object can take place
    pState->critical.pXmlHttp->bStarted = TRUE;

    // open the request
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->Open(wstrRequestType, unicodeUrl.c_str(), pState->critical.pXmlHttp->cpXmlHttpCallback.Get(), NULL, NULL, NULL, NULL)) != S_OK)
    {
        // workaround for MS OR_INVALID_OXID bug
        if (((hResult&0xffff) == OR_INVALID_OXID) && (pState->bRetriedOpen == FALSE))
        {
            NetPrintf(("protohttpxboxone: [%p] failed to open request (err=%s); executing re-try workaround\n", pState, DirtyErrGetName(hResult)));
            pState->critical.responseBuffer.iInpLen = iInpLen;
            pState->critical.pXmlHttp->bStarted = FALSE;
            pState->bRetriedOpen = TRUE;
            return(1);
        }
        NetPrintf(("protohttpxboxone: [%p] failed to open request (err=%s)\n", pState, DirtyErrGetName(hResult)));
        pState->critical.pXmlHttp->bComplete = TRUE;
        return(-2);
    }

    // skip past end of url line
    pHdrStrt = ds_stristr(pUrlEnd, "\r\n") + 2;
    _ProtoHttpSetRequestHeaders(pState, pHdrStrt, strTempBuf, sizeof(strTempBuf));

    // set request properties
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->SetProperty(XHR_PROP_NO_AUTH, !pState->bAuthenticate)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to %s authentication\n", pState, pState->bAuthenticate ? "enable" : "disable"));
        return(-3);
    }
    // disable ms timeouts; only implements a connection timeout so we implement our own
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->SetProperty(XHR_PROP_TIMEOUT, /*pState->uTimeout*/0x7fffffff)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to set timeout (err=%s)\n", pState, DirtyErrGetName(hResult)));
        return(-4);
    }
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->SetProperty(XHR_PROP_NO_CRED_PROMPT, TRUE)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to disable credential prompt\n", pState));
        return(-5);
    }
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->SetProperty(XHR_PROP_ONDATA_THRESHOLD, pState->iOnDataThreshold)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to set ondata threshold\n", pState));
        return(-6);
    }

    // count the attempt
    pState->iKeepAlive += 1;

    // send the request
    return(_ProtoHttpSend(pState, NULL, 0));
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpProcessRedirect

    \Description
        Process pending redirect notifications

    \Input *pState      - reference pointer

    \Version 07/22/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpProcessRedirect(ProtoHttpRefT *pState)
{
    int32_t iHdrLen;

    // check for a redirection header
    if (pState->critical.iNumRedirect == pState->critical.iNumRedirProc)
    {
        return(0);
    }

    // copy header to header cache
    ds_strnzcpy(pState->responseHeader.strHdr, pState->critical.responseBuffer.pInpBuf, sizeof(pState->responseHeader.strHdr));

    // set response code to 302/FOUND
    pState->responseHeader.iHdrCode = PROTOHTTP_RESPONSE_FOUND;
    pState->responseHeader.iHeadSize = (int32_t)strlen(pState->critical.responseBuffer.pInpBuf);

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pState->iVerbose, 0, "protohttpxboxone: [%p] received %d response (%d bytes)\n", pState, pState->responseHeader.iHdrCode, pState->responseHeader.iHeadSize));
    if (pState->iVerbose > 1)
    {
        NetPrintWrap(pState->critical.responseBuffer.pInpBuf, 80);
    }
    #endif

    // trigger recv header user callback, if specified
    if (pState->pReceiveHeaderCb != NULL)
    {
        pState->pReceiveHeaderCb(pState, pState->critical.responseBuffer.pInpBuf, pState->responseHeader.iHeadSize, pState->pCallbackRef);
    }

    // remove header from buffer
    iHdrLen = pState->responseHeader.iHeadSize + 1;
    if (iHdrLen < pState->critical.responseBuffer.iInpLen)
    {
        memmove(pState->critical.responseBuffer.pInpBuf, pState->critical.responseBuffer.pInpBuf + iHdrLen, pState->critical.responseBuffer.iInpLen - iHdrLen);
    }
    pState->critical.responseBuffer.iInpLen -= iHdrLen;

    // track processing of redirection
    pState->critical.iNumRedirProc += 1;

    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpGetResponseHeader

    \Description
        Get response header from IXMLHttpRequest2 object

    \Input *pState      - reference pointer
    \Input iHdrCode     - header code
    \Input *pwStrCode   - ??

    \Output
        int32_t         - negative=error, else zero

    \Version 07/22/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpGetResponseHeader(ProtoHttpRefT *pState, int32_t iHdrCode, const wchar_t *pwStrCode)
{
    ProtoHttpBufferT *pBuffer;
    WCHAR *pwstrHeaders;
    int32_t iHdrLen;
    HRESULT hResult;
    
    NetCritEnter(&pState->critical.Locker);

    // reference response buffer
    pBuffer = &pState->critical.responseBuffer;

    // get all headers
    if ((hResult = pState->critical.pXmlHttp->cpXmlHttp->GetAllResponseHeaders(&pwstrHeaders)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: GetAllResponseHeaders() failed (err=%s)\n", pState, DirtyErrGetName(hResult)));
        pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
        NetCritLeave(&pState->critical.Locker);
        return(-1);
    }

    // calc header length; add a pad for HTTP status line we will add
    if ((iHdrLen = (int32_t)wcslen(pwstrHeaders) + 64) > pBuffer->iInpMax)
    {
        _ProtoHttpResizeBufferAuto(pState, iHdrLen);
    }

    // format response header protocol, status, and status string, add null for later easy parsing
    pBuffer->iInpLen += ds_snzprintf(pBuffer->pInpBuf+pBuffer->iInpLen, pBuffer->iInpMax-pBuffer->iInpLen, "HTTP/1.1 %d %S\r\n%S", iHdrCode, pwStrCode, pwstrHeaders);
    pBuffer->iInpLen += 1;

    // release tmp header buffer
    CoTaskMemFree(pwstrHeaders);

    // success
    NetCritLeave(&pState->critical.Locker);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpParseHeader

    \Description
        Parse incoming HTTP header.  The HTTP header is presumed to be at the
        beginning of the input buffer.

    \Input  *pState     - reference pointer

    \Output
        int32_t         - negative=not ready or error, else success

    \Version 11/12/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpParseHeader(ProtoHttpRefT *pState)
{
    char strTemp[128];
    int32_t iHdrLen;

    // save the header size (include final terminator)
    pState->responseHeader.iHeadSize = (int32_t)strlen(pState->critical.responseBuffer.pInpBuf);
    // parse header code
    pState->responseHeader.iHdrCode = ProtoHttpParseHeaderCode(pState->critical.responseBuffer.pInpBuf);

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pState->iVerbose, 0, "protohttpxboxone: [%p] received %d response (%d bytes)\n", pState, pState->responseHeader.iHdrCode, pState->responseHeader.iHeadSize));
    if (pState->iVerbose > 1)
    {
        NetPrintWrap(pState->critical.responseBuffer.pInpBuf, 80);
    }
    #endif

    // parse content-length field
    if ((ProtoHttpExtractHeaderValue(ProtoHttpFindHeaderValue(pState->critical.responseBuffer.pInpBuf, "content-length"), strTemp, sizeof(strTemp), NULL)) != -1)
    {
        pState->responseHeader.iBodySize = ds_strtoll(strTemp, NULL, 10);
    }
    else
    {
        pState->responseHeader.iBodySize = -1;
    }

    // parse last-modified header
    if ((ProtoHttpExtractHeaderValue(ProtoHttpFindHeaderValue(pState->critical.responseBuffer.pInpBuf, "last-modified"), strTemp, sizeof(strTemp), NULL)) != -1)
    {
        pState->responseHeader.iHdrDate = ds_strtotime(strTemp);
    }
    else
    {
        pState->responseHeader.iHdrDate = 0;
    }

    // copy header to header cache
    ds_strnzcpy(pState->responseHeader.strHdr, pState->critical.responseBuffer.pInpBuf, sizeof(pState->responseHeader.strHdr));

    // trigger recv header user callback, if specified
    if (pState->pReceiveHeaderCb != NULL)
    {
        pState->pReceiveHeaderCb(pState, pState->critical.responseBuffer.pInpBuf, pState->responseHeader.iHeadSize, pState->pCallbackRef);
    }

    // remove header (and null terminator) from input buffer
    iHdrLen = pState->responseHeader.iHeadSize + 1;
    if (iHdrLen < pState->critical.responseBuffer.iInpLen)
    {
        memmove(pState->critical.responseBuffer.pInpBuf, pState->critical.responseBuffer.pInpBuf + iHdrLen, pState->critical.responseBuffer.iInpLen - iHdrLen);
    }
    pState->critical.responseBuffer.iInpLen -= iHdrLen;
    
    // handle response code?
    if (PROTOHTTP_GetResponseClass(pState->responseHeader.iHdrCode) == PROTOHTTP_RESPONSE_INFORMATIONAL)
    {
        NetPrintf(("protohttpxboxone: [%p] ignoring %d response from server\n", pState, pState->responseHeader.iHdrCode));
    }
    else if (PROTOHTTP_GetResponseClass(pState->responseHeader.iHdrCode) == PROTOHTTP_RESPONSE_REDIRECTION)
    {
        NetPrintf(("protohttpxboxone: [%p] redirection %d response from server\n", pState, pState->responseHeader.iHdrCode));
    }

    // header successfully parsed
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpWriteCbProcess

    \Description
        User write callback processing, if write callback is set

    \Input *pState          - reference pointer

    \Version 05/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpWriteCbProcess(ProtoHttpRefT *pState)
{
    ProtoHttpWriteCbInfoT WriteCbInfo;
    int32_t iResult;

    memset(&WriteCbInfo, 0, sizeof(WriteCbInfo));
    WriteCbInfo.eRequestType = pState->eRequestType;
    WriteCbInfo.eRequestResponse = (ProtoHttpResponseE)pState->responseHeader.iHdrCode;

    if (pState->critical.eRequestState == ProtoHttpStateE::ST_BODY)
    {
        char strTempRecv[4096];
        while ((iResult = ProtoHttpRecv(pState, strTempRecv, 1, sizeof(strTempRecv))) > 0)
        {
            pState->pWriteCb(pState, &WriteCbInfo, strTempRecv, iResult, pState->pWriteCbUserData);
        }
    }
    else if (pState->critical.eRequestState > ST_BODY)
    {
        if (pState->critical.eRequestState == ST_DONE)
        {
            pState->pWriteCb(pState, &WriteCbInfo, "", (pState->eRequestType == PROTOHTTP_REQUESTTYPE_HEAD) ? PROTOHTTP_HEADONLY : PROTOHTTP_RECVDONE, pState->pWriteCbUserData);
        }
        if (pState->critical.eRequestState == ST_FAIL)
        {
            pState->pWriteCb(pState, &WriteCbInfo, "", PROTOHTTP_RECVFAIL, pState->pWriteCbUserData);
        }
        pState->pWriteCb = NULL;
        pState->pWriteCbUserData = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpDestroy

    \Description
        Destroy ProtoHttp ref

    \Input *pState  - module ref

    \Version 10/03/2013 (mclouatre)
*/
/********************************************************************************F*/
static void _ProtoHttpDestroy(ProtoHttpRefT *pState)
{
    NetPrintf(("protohttpxboxone: [%p] destructing...\n", pState));

    NetCritKill (&pState->critical.Locker);

    if (pState->pAppendHdr != NULL)
    {
        DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    if (pState->critical.responseBuffer.pInpBuf != NULL)
    {
        DirtyMemFree(pState->critical.responseBuffer.pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->critical.responseBuffer.pInpBuf = NULL;
    }

    DirtyMemFree(pState, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _ProtoHttpDestroyCallback

    \Description
        Callback registered with NetConn for deferred destruction occurring only when 
        no requests are left in flight.

    \Input *pRef        - reference pointer

    \Output
        int32_t         - zero = success; -1 = try again; other negative = error

    \Version 10/03/2013 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _ProtoHttpDestroyCallback(void *pRef)
{
    ProtoHttpRefT *pState = (ProtoHttpRefT *)pRef;

    if (pState->iPendingXmlHttpDestructions)
    {
        return(-1);
    }

    _ProtoHttpDestroy(pState);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpXmlHttpCreate

    \Description
        Create XmlHttp object

    \Input *pState          - reference pointer

    \Output
        ProtoHttpXmlHttpT * - XmlHttp object, or NULL on failure

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
static ProtoHttpXmlHttpT *_ProtoHttpXmlHttpCreate(ProtoHttpRefT *pState)
{
    ProtoHttpXmlHttpT *pXmlHttp;
    HRESULT hResult;

    // allocate memory for object
    if ((pXmlHttp = (ProtoHttpXmlHttpT *)DirtyMemAlloc(sizeof(*pXmlHttp), PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpxboxone: [%p] unable to allocate xmlhttp memory\n", pState));
        return(NULL);
    }
    memset(pXmlHttp, 0, sizeof(*pXmlHttp));

    // instantiate IXMLHTTPRequest2 object
    if ((hResult = CoCreateInstance(__uuidof(FreeThreadedXMLHTTP60), NULL, CLSCTX_SERVER, __uuidof(IXMLHTTPRequest2), &pXmlHttp->cpXmlHttp)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to instantiate IXMLHTTPRequest2 object (err=%s)\n", pState, DirtyErrGetName(hResult)));
        return(NULL);
    }
    // instantiate HttpCallback object and set state ref
    if ((hResult = Microsoft::WRL::Details::MakeAndInitialize<HttpCallback>(&pXmlHttp->cpXmlHttpCallback)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to instantiate HttpCallback object (err=%s)\n", pState, DirtyErrGetName(hResult)));
        return(NULL);
    }
    // instantiate output stream object and set state ref
    if ((hResult = Microsoft::WRL::Details::MakeAndInitialize<SequentialStream>(&pXmlHttp->cpOutStream)) != S_OK)
    {
        NetPrintf(("protohttpxboxone: [%p] failed to instantiate OutStream object (err=%s)\n", pState, DirtyErrGetName(hResult)));
        return(NULL);
    }
    pXmlHttp->cpOutStream->SetProtoHttpRef(pState);

    // save some state ref, and some state info for use when we don't have access to the state
    pXmlHttp->pState = pState;
    pXmlHttp->iMemGroup = pState->iMemGroup;
    pXmlHttp->pMemGroupUserData = pState->pMemGroupUserData;
    pXmlHttp->iVerbose = pState->iVerbose;

    // set callback to reference us
    pXmlHttp->cpXmlHttpCallback->SetXmlHttp(pXmlHttp);

    return(pXmlHttp);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpXmlHttpDestroyObject

    \Description
        Destroy XmlHttp object; this function assumes we are already in a valid
        state to destroy the object.

    \Input *pXmlHttp        - xmlhttp object to destroy

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpXmlHttpDestroyObject(ProtoHttpXmlHttpT *pXmlHttp)
{
    /* we need to explicitly release the ComPtr objects we created with
       CoCreateInstance/MakeInitialize as we are calling from C code, not
       a class with a destructor where this would happen automatically */
    pXmlHttp->cpXmlHttp.ReleaseAndGetAddressOf();
    pXmlHttp->cpXmlHttpCallback.ReleaseAndGetAddressOf();
    pXmlHttp->cpOutStream.ReleaseAndGetAddressOf();
    DirtyMemFree(pXmlHttp, PROTOHTTP_MEMID, pXmlHttp->iMemGroup, pXmlHttp->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _ProtoHttpXmlHttpDestroyCallback

    \Description
        Clean up XmlHttp object if asynchronous request is completed

    \Input *pRef        - reference pointer

    \Output
        int32_t         - zero = success; -1 = try again; other negative = error

    \Version 07/12/2013 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _ProtoHttpXmlHttpDestroyCallback(void *pRef)
{
    ProtoHttpXmlHttpT *pXmlHttp = (ProtoHttpXmlHttpT *)pRef;

    // delay while active
    if (_ProtoHttpXmlHttpIsActive(NULL, pXmlHttp))
    {
        NetPrintfVerbose((pXmlHttp->bSpammedShutdown ? 0 : 1, 0, "protohttpxboxone: [%p] deferring destroy of xmlhttp object [%p]\n", pXmlHttp->pState, pXmlHttp));
        pXmlHttp->bSpammedShutdown = TRUE;
        return(-1);
    }

    pXmlHttp->pState->iPendingXmlHttpDestructions--;
    NetPrintf(("protohttpxboxone: [%p] finished deferred destruction of xmlhttp object [%p] (pending count = %d)\n",
        pXmlHttp->pState, pXmlHttp, pXmlHttp->pState->iPendingXmlHttpDestructions));

    // destroy xmlhttp object
    _ProtoHttpXmlHttpDestroyObject(pXmlHttp);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpXmlHttpDestroy

    \Description
        Destroy XmlHttp object; if the object is still in an active state,
        destruction is handed off to netconn.

    \Input *pState          - reference pointer
    \Input *pXmlHttp        - xmlhttp object to destroy

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpXmlHttpDestroy(ProtoHttpRefT *pState, ProtoHttpXmlHttpT *pXmlHttp)
{
    // if not active, destroy it
    if (!_ProtoHttpXmlHttpIsActive(pState, pXmlHttp))
    {
        _ProtoHttpXmlHttpDestroyObject(pXmlHttp);
    }
    else
    {
        // abort ongoing transaction
        if (_ProtoHttpAbort(pState) < 0)
        {
            return;
        }

        // schedule for destruction with netconn
        if (!NetConnControl('recu', 0, 0, (void *)_ProtoHttpXmlHttpDestroyCallback, pXmlHttp))
        {
            pState->iPendingXmlHttpDestructions++;
            NetPrintf(("protohttpxboxone: [%p] destruction of xmlhttp object [%p] deferred to netconn due to active request  (pending count = %d)\n",
                pState, pState->critical.pXmlHttp, pState->iPendingXmlHttpDestructions));
        }
        else
        {
            NetPrintf(("protohttpxboxone: [%p] unable to schedule _ProtoHttpXmlHttpDestroyCallback\n", pState));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpReset

    \Description
        Reset module state for a new transaction.

    \Input *pState      - module state

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpReset(ProtoHttpRefT *pState)
{
    // reset module state data
    pState->iRecvSize = 0;

    // reset request and response header
    memset(&pState->strRequestHdr, 0, sizeof(pState->strRequestHdr));
    memset(&pState->responseHeader, 0, sizeof(pState->responseHeader));
    pState->responseHeader.iBodySize = -1;

    // reset critical data
    pState->critical.iNumRedirect = 0;
    pState->critical.iNumRedirProc = 0;
    pState->critical.eRequestState = ST_IDLE;
    pState->critical.iRequestSize = 0;
    pState->critical.bBodyReceived = FALSE;
    pState->critical.bTimeout = FALSE;
    pState->critical.bAbortingRedirection = FALSE;
    pState->critical.bHeaderReceived = FALSE;

    // reset response buffer
    pState->critical.responseBuffer.iInpLen = 0;
    pState->critical.responseBuffer.iInpOvr = 0;
    pState->critical.responseBuffer.iInpTotal = 0;

    // reset 302/303 redirection workaround bug info
    pState->critical.bRedirBugWorkaround = FALSE;
    pState->strLocation[0] = '\0';

    // OR_INVALID_OXID bug workaround
    pState->bRetriedOpen = FALSE;
};


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttpCreate

    \Description
        Allocate module state and prepare for use

    \Input iBufSize     - length of receive buffer

    \Output
        ProtoHttpRefT * - pointer to module state, or NULL

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
ProtoHttpRefT *ProtoHttpCreate(int32_t iBufSize)
{
    ProtoHttpRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp the buffer size
    if (iBufSize < 4*1024)
    {
        iBufSize = 4*1024;
    }

    // allocate the resources
    if ((pState = (ProtoHttpRefT *)DirtyMemAlloc(sizeof(*pState), PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpxboxone: [%p] unable to allocate module state\n", pState));
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));

    // initialize mutex
    NetCritInit(&pState->critical.Locker, "protohttpxboxone");

    // save memgroup (will be used in ProtoHttpDestroy)
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // initialize other settings
    pState->bAuthenticate = TRUE;
    pState->responseHeader.iHdrCode = -1;
    pState->iVerbose = 1;
    pState->iOnDataThreshold = 64*1024;

    // allocate input buffer
    if ((pState->critical.responseBuffer.pInpBuf = (char *)DirtyMemAlloc(iBufSize, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpxboxone: [%p] unable to allocate protohttp buffer\n", pState));
        ProtoHttpDestroy(pState);
        return(NULL);
    }

    // save parms & set defaults
    pState->critical.responseBuffer.iInpMax = iBufSize;
    pState->critical.responseBuffer.iInpMaxIdeal = iBufSize;
    pState->uTimeout = PROTOHTTP_TIMEOUT;
    pState->iMaxRedirect = PROTOHTTP_MAXREDIRECT;

    NetPrintf(("protohttpxboxone: [%p] creation successful\n", pState));

    // return the state
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpCallback

    \Description
        Set header callbacks.

    \Input *pState          - module state
    \Input *pCustomHeaderCb - pointer to custom send header callback (may be NULL)
    \Input *pReceiveHeaderCb- pointer to recv header callback (may be NULL)
    \Input *pCallbackRef    - user-supplied callback ref (may be NULL)

    \Notes
        The ProtHttpCustomHeaderCbt callback is used to allow customization of
        the HTTP header before sending.  It is more powerful than the append
        header functionality, allowing to make changes to any part of the header
        before it is sent.  The callback should return a negative code if an error
        occurred, zero can be returned if the application wants ProtoHttp to
        calculate the header size, or the size of the header can be returned if
        the application has already calculated it.  The header should *not* be
        terminated with the double \\r\\n that indicates the end of the entire
        header, as protohttp appends itself.

        The ProtoHttpReceiveHeaderCbT callback is used to view the header
        immediately on reception.  It can be used when the built-in header
        cache (retrieved with ProtoHttpStatus('htxt') is too small to hold
        the entire header received.  It is also possible with this method
        to view redirection response headers that cannot be retrieved
        normally.  This can be important if, for example, the application
        wishes to attach new cookies to a redirection response.  The
        custom response header and custom header callback can be used in
        conjunction to implement this type of functionality.

    \Version 1.0 02/16/2007 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpCallback(ProtoHttpRefT *pState, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pCallbackRef)
{
    pState->pCustomHeaderCb  = pCustomHeaderCb;
    pState->pReceiveHeaderCb = pReceiveHeaderCb;
    pState->pCallbackRef     = pCallbackRef;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpDestroy

    \Description
        Destroy the module and release its state

    \Input *pState      - reference pointer

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpDestroy(ProtoHttpRefT *pState)
{
    /* 
    acquire the critical section before destroying the pXmlHttp. By acquiring this critical section, we make sure that we are not in a callback.
    If a request is active, _ProtoHttpXmlHttpDestroy will decouple state from the callback and call Abort() on IXMLHTTPRequest2. After that, 
    we'll get either OnError or OnResponseReceived both of which check for pState not being null before doing anything meaningful.
    */
    NetCritEnter(&pState->critical.Locker); 
    if (pState->critical.pXmlHttp != NULL)
    {
        _ProtoHttpXmlHttpDestroy(pState, pState->critical.pXmlHttp);
    }
    NetCritLeave(&pState->critical.Locker);

    // attempt to destroy the module
    if (_ProtoHttpDestroyCallback(pState) == -1)
    {
        // schedule for destruction with netconn
        if (!NetConnControl('recu', 0, 0, (void *)_ProtoHttpDestroyCallback, pState))
        {
            NetPrintf(("protohttpxboxone: [%p] destruction deferred to netconn because not all xmlhttp objects have been destroyed yet\n", pState));
        }
        else
        {
            NetPrintf(("protohttpxboxone: [%p] unable to schedule _ProtoHttpDestroyCallback with netconn\n", pState));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGet

    \Description
        Initiate an HTTP transfer. Pass in a URL and the module starts a getting
        data from the appropriate server.

    \Input *pState      - reference pointer
    \Input *pUrl        - the url to retrieve
    \Input bHeadOnly    - if TRUE only get header

    \Output
        int32_t         - negative=failure, else success

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t ProtoHttpGet(ProtoHttpRefT *pState, const char *pUrl, uint32_t bHeadOnly)
{
    return(ProtoHttpRequest(pState, pUrl, NULL, 0, bHeadOnly ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_GET));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRecv

    \Description
        Return the actual url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufMin  - minimum number of bytes to return (returns zero if this number is not available)
    \Input iBufMax  - maximum number of bytes to return (buffer size)

    \Output
        int32_t     - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRecv(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    int32_t iResult = 0;

    if (!NetCritTry(&pState->critical.Locker))
    {
        return(0);
    }

    if (pState->critical.eRequestState == ProtoHttpStateE::ST_FAIL)
    {
        iResult = PROTOHTTP_RECVFAIL;
    }
    else if (pState->critical.eRequestState < ProtoHttpStateE::ST_BODY)
    {
        iResult = PROTOHTTP_RECVWAIT;
    }
    else if ((pState->critical.eRequestState == ProtoHttpStateE::ST_DONE) && (pState->critical.responseBuffer.iInpLen == 0))
    {
        iResult = PROTOHTTP_RECVDONE;
    }

    //$$TODO -- give a chance to get more data?

    if ((iResult == 0) && (iBufMax > 0) && (pState->critical.responseBuffer.iInpLen >= iBufMin))
    {
        // clamp the range
        if (iBufMin < 1)
            iBufMin = 1;
        if (iBufMax < iBufMin)
            iBufMax = iBufMin;
        if (iBufMin > pState->critical.responseBuffer.iInpMax)
            iBufMin = pState->critical.responseBuffer.iInpMax;
        if (iBufMax > pState->critical.responseBuffer.iInpMax)
            iBufMax = pState->critical.responseBuffer.iInpMax;

        // copy data to caller
        iResult = pState->critical.responseBuffer.iInpLen <= iBufMax ? pState->critical.responseBuffer.iInpLen : iBufMax;
        ds_memcpy(pBuffer, pState->critical.responseBuffer.pInpBuf, iResult);

        // compact data
        memmove(pState->critical.responseBuffer.pInpBuf, pState->critical.responseBuffer.pInpBuf + iResult, pState->critical.responseBuffer.iInpLen - iResult);
        pState->critical.responseBuffer.iInpLen -= iResult;

        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 3)
        {
            NetPrintf(("protohttpxboxone: [%p] read %d bytes\n", pState, iResult));
        }
        if (pState->iVerbose > 4)
        {
            NetPrintMem(pBuffer, iResult, "http-read");
        }
        #endif
    }

    NetCritLeave(&pState->critical.Locker);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRecvAll

    \Description
        Return all of the url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufSize - size of buffer

    \Output
        int32_t     - PROTOHTTP_RECV*, or positive=bytes in response

    \Version 12/14/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRecvAll(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufSize)
{
    int32_t iRecvMax, iRecvResult;

    // try to read as much data as possible (leave one byte for null termination)
    for (iRecvMax = iBufSize-1; (iRecvResult = ProtoHttpRecv(pState, pBuffer + pState->iRecvSize, 1, iRecvMax - pState->iRecvSize)) > 0; )
    {
        // add to amount received
        pState->iRecvSize += iRecvResult;
    }

    // check response code
    if (iRecvResult == PROTOHTTP_RECVDONE)
    {
        // null-terminate response & return completion success
        pBuffer[pState->iRecvSize] = '\0';
        iRecvResult = pState->iRecvSize;
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP_RECVWAIT))
    {
        // an error occurred
        NetPrintf(("protohttpxboxone: [%p] error %d receiving response\n", pState, iRecvResult));
    }
    else if (iRecvResult == 0)
    {
        iRecvResult = (pState->iRecvSize < iRecvMax) ? PROTOHTTP_RECVWAIT : PROTOHTTP_RECVBUFF;
    }

    // return result to caller
    return(iRecvResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpPost

    \Description
        Initiate transfer of data to to the server via a HTTP POST command.

    \Input *pState      - reference pointer
    \Input *pUrl        - the URL that identifies the POST action.
    \Input *pData       - pointer to URL data (optional, may be NULL)
    \Input iDataSize    - size of data being uploaded (see Notes below)
    \Input bDoPut       - meanless on xboxone

    \Output
        int32_t         - negative=failure, else number of data bytes sent

    \Notes
        Any data that is not sent as part of the Post transaction should be sent
        with ProtoHttpSend().  ProtoHttpSend() should be called at poll rate (i.e.
        similar to how often ProtoHttpUpdate() is called) until all of the data has
        been sent.  The amount of data bytes actually sent is returned by the
        function.

        If pData is not NULL and iDataSize is less than or equal to zero, iDataSize
        will be recalculated as the string length of pData, to allow for easy string
        sending.

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t ProtoHttpPost(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, uint32_t bDoPut)
{
    return(ProtoHttpRequest(pState, pUrl, pData, iDataSize, bDoPut ? PROTOHTTP_REQUESTTYPE_PUT : PROTOHTTP_REQUESTTYPE_POST));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSend

    \Description
        Send data during an ongoing Post transaction.

    \Input *pState      - reference pointer
    \Input *pData       - pointer to data to send
    \Input iDataSize    - size of data being sent

    \Output
        int32_t         - negative=failure, else number of data bytes sent

    \Version 07/15/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpSend(ProtoHttpRefT *pState, const char *pData, int32_t iDataSize)
{
    if (!NetCritTry(&pState->critical.Locker))
    {
        return(0);
    }

    if (pState->critical.eRequestState < ProtoHttpStateE::ST_RESP)
    {
        NetPrintf(("protohttpxboxone: [%p] calling send when not yet in send state (state=%s)\n", pState, _ProtoHttp_strRequestState[pState->critical.eRequestState]));
        NetCritLeave(&pState->critical.Locker);
        return(0);
    }
    else if (pState->critical.eRequestState != ProtoHttpStateE::ST_RESP)
    {
        NetPrintf(("protohttpxboxone: [%p] calling send when not in invalid state (state=%s)\n", pState, _ProtoHttp_strRequestState[pState->critical.eRequestState]));
        NetCritLeave(&pState->critical.Locker);
        return(-1);
    }

    // try to send data (note that it will not actually be sent until all of the data is buffered)
    iDataSize = _ProtoHttpSend(pState, pData, iDataSize);

    NetCritLeave(&pState->critical.Locker);
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpDelete

    \Description
        Request deletion of a server-based resource.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url describing reference to delete

    \Output
        int32_t         - negative=failure, zero=success

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t ProtoHttpDelete(ProtoHttpRefT *pState, const char *pUrl)
{
    return(ProtoHttpRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpOptions

    \Description
        Request options from a server.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url describing reference to get options on

    \Output
        int32_t         - negative=failure, zero=success

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t ProtoHttpOptions(ProtoHttpRefT *pState, const char *pUrl)
{
    return(ProtoHttpRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_OPTIONS));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRequestCb

    \Description
        Initiate an HTTP transfer. Pass in a URL and the module starts a transfer
        from the appropriate server.  Use ProtoHttpRequest() macro wrapper if
        write callback is not required.

    \Input *pState           - reference pointer
    \Input *pUrl             - the url to retrieve
    \Input *pData            - user data to send to server (PUT and POST only)
    \Input iDataSize         - size of user data to send to server (PUT and POST only)
    \Input eRequestType      - request type to make
    \Input *pWriteCb         - write callback (optional)
    \Input *pWriteCbUserData - write callback user data (optional)

    \Output
        int32_t              - negative=failure, zero=success, >0=number of data bytes sent (PUT and POST only)

    \Version 07/12/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRequestCb(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, void *pWriteCbUserData)
{
    int32_t iDataSent = 0, iResult;

    // validate URL
    if (pUrl == NULL)
    {
        NetPrintf(("protohttpxboxone: [%p] invalid url\n", pState));
        return(-2);
    }

    // validate request type
    if (eRequestType >= PROTOHTTP_NUMREQUESTTYPES)
    {
        NetPrintf(("protohttpxboxone: [%p] invalid request type %d\n", pState, eRequestType));
        return(-1);
    }

    // save write callback
    pState->pWriteCb = pWriteCb;
    pState->pWriteCbUserData = pWriteCbUserData;

    // make the request
    NetCritEnter(&pState->critical.Locker);

    // clean up previous XmlHttp object, if allocated, and reset module state
    if (pState->critical.pXmlHttp != NULL)
    {
        _ProtoHttpXmlHttpDestroy(pState, pState->critical.pXmlHttp);
    }
    _ProtoHttpReset(pState);

    // save request info
    pState->eRequestType = eRequestType;

    // format request url
    if (_ProtoHttpFormatUrl(pState, pUrl) < 0)
    {
        NetCritLeave(&pState->critical.Locker);
        return(-3);
    }

    // create new XmlHttp object for this request
    if ((pState->critical.pXmlHttp = _ProtoHttpXmlHttpCreate(pState)) == NULL)
    {
        NetCritLeave(&pState->critical.Locker);
        return(-4);
    }

    // special support for string sends
    if ((pData != NULL) && (iDataSize < 0))
    {
        iDataSize = strlen(pData);
    }
    // save total transaction size
    pState->critical.iRequestSize = iDataSize;

    /* limit max request size -- ProtoHttpPost() has no way of knowing how much request data is
       available, just the total request size.  if the request size is large, and the amount of
       data that is available to ProtoHttpPost() to read is small, ProtoHttpPost() will read
       as much data as it can fit in its internal buffer.  here we limit the maximum read size
       to the size of the ProtoHttpCreate() buffer, which is very similar to what the 'vanilla'
       version of protohttp does (the data size is slightly larger as we don't format the request
       header in the send buffer in this version).  this is an issue with the protohttp API and
       should be addressed properly, possibly by adding a new variable to indicate how much data
       is available, separately from the variable tracking the overall transaction size.  */
    if (iDataSize > pState->critical.responseBuffer.iInpMaxIdeal)
    {
        iDataSize = pState->critical.responseBuffer.iInpMaxIdeal;
    }

    // buffer data if available
    if ((pData != NULL) && (iDataSize > 0))
    {
        if ((iDataSent = _ProtoHttpSend(pState, pData, iDataSize)) < 0)
        {
            NetPrintf(("protohttpxboxone: [%p] failed to send %d bytes\n", pState, iDataSize));
            NetCritLeave(&pState->critical.Locker);
            return(-5);
        }
    }

    // get auth token if requested
    if (pState->bGetAuthToken)
    {
        // get token url
        if (pState->strTokenUrl[0] == '\0')
        {
            ds_strnzcpy(pState->strTokenUrl, pState->strHost, sizeof(pState->strTokenUrl));
        }
        // make token request
        if ((iResult = DirtyAuthGetToken(pState->iAuthTokenIdx, pState->strTokenUrl, FALSE)) == DIRTYAUTH_PENDING)
        {
            // transition to authenticating state, return success
            pState->critical.eRequestState = ProtoHttpStateE::ST_AUTH;
        }
        else if (iResult == DIRTYAUTH_SUCCESS)
        {
            // token is cached and already available, so we proceed directly to connection stage
            pState->critical.eRequestState = ProtoHttpStateE::ST_CONN;
        }
        else
        {
            NetPrintf(("protohttpxboxone: DirtyAuthGetToken() returned unexpected result %d\n", iResult));
            pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
            iDataSent = -6;
        }
    }
    else
    {
        pState->critical.eRequestState = ProtoHttpStateE::ST_CONN;
    }        

    // set request timer
    pState->uTimer = NetTick() + pState->uTimeout;

    NetCritLeave(&pState->critical.Locker);
    return(iDataSent);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpAbort

    \Description
        Abort current operation, if any.

    \Input *pState      - reference pointer

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
void ProtoHttpAbort(ProtoHttpRefT *pState)
{
    NetCritEnter(&pState->critical.Locker);

    _ProtoHttpAbort(pState);

    NetCritLeave(&pState->critical.Locker);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetBaseUrl

    \Description
        Set base url that will be used for any relative url references.

    \Input *pState      - module state
    \Input *pUrl        - base url

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpSetBaseUrl(ProtoHttpRefT *pState, const char *pUrl)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // parse the url for kind, host, and port
    ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // set base info
    NetPrintf(("protohttpxboxone: [%p] setting base url to %s://%s:%d\n", pState, iSecure ? "https" : "http", strHost, iPort));
    ds_strnzcpy(pState->strBaseHost, strHost, sizeof(pState->strBaseHost));
    pState->iBasePort = iPort;
    pState->iBaseSecure = iSecure;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetLocationHeader

    \Description
        Get location header from the input buffer.  The Location header requires
        some special processing.

    \Input *pState  - reference pointer
    \Input *pInpBuf - buffer holding header text
    \Input *pBuffer - [out] output buffer for parsed location header, null for size request
    \Input iBufSize - size of output buffer, zero for size request
    \Input **pHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=header not received, not found or not enough space, zero=success, positive=size query result

    \Version 03/24/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetLocationHeader(ProtoHttpRefT *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    const char *pLocHdr;
    int32_t iLocLen, iLocPreLen=0;

    // get a pointer to header
    if ((pLocHdr = ProtoHttpFindHeaderValue(pState->responseHeader.strHdr, "location")) == NULL)
    {
        return(-1);
    }

    /* according to RFC location headers should be absolute, but some webservers respond with relative
       URL's.  we assume any url that does not start with "http://" or "https://" is a relative url,
       and if we find one, we assume the request keeps the same security, port, and host as the previous
       request */
    if (ds_strnicmp(pLocHdr, "http://", 7) && ds_strnicmp(pLocHdr, "https://", 8))
    {
        char strTemp[288]; // space for max DNS name (253 chars) plus max http url prefix

        // format http url prefix
        ds_snzprintf(strTemp, sizeof(strTemp), "%s://%s:%d", pState->iSecure ? "https" : "http", pState->strHost, pState->iPort);

        // make sure relative URL includes '/' as the first character, required when we format the redirection url
        if (*pLocHdr != '/')
        {
            ds_strnzcat(strTemp, "/", sizeof(strTemp));
        }

        // calculate url prefix length
        iLocPreLen = (int32_t)strlen(strTemp);

        // copy url prefix text if a buffer is specified
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, strTemp, iBufSize);
            pBuffer = (char *)((uint8_t *)pBuffer + iLocPreLen);
            iBufSize -= iLocPreLen;
        }
    }

    // extract location header and return size
    iLocLen = ProtoHttpExtractHeaderValue(pLocHdr, pBuffer, iBufSize, pHdrEnd);
    // if it's a size request add in (possible) url header length
    if ((pBuffer == NULL) && (iBufSize == 0))
    {
        iLocLen += iLocPreLen;
    }

    // return to caller
    return(iLocLen);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpControl

    \Description
        ProtoHttp control function.  Different selectors control different behaviors.

    \Input *pState  - reference pointer
    \Input iSelect  - control selector ('time')
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION'
            'apnd'      The given buffer will be appended to future headers sent
                        by ProtoHttp.  Note that the User-Agent and Accept lines
                        in the default header will be replaced, so if these lines
                        are desired, they should be supplied in the append header.
            'auth'      Enables/disables authentication (default=enabled)
            'ires'      Resize input buffer
            'keep'      Sets keep-alive; zero=disabled, else enabled
            'maxr'      Sets max recv rate (bytes/sec; zero=uncapped)
            'maxs'      Sets max send rate (bytes/sec; zero=uncapped)
            'odth'      Sets ONDATA_THRESHOLD property (default=64k)
            'prxy'      Sets proxy server
            'rmax'      Sets maximum number of redirections (default=3)
            'spam'      Sets debug output verbosity (0...n)
            'time'      Sets ProtoHttp client timeout in milliseconds (default=30s)
            'xtok'      Sets XSTS token enable/disable (default=disabled; iValue=
                        enabled true/false, iValue2=user index, pValue=
                        token URN, only required if different from URL)
        \endverbatim

    \Version 07/02/2013 (jbrookes)
*/
/*******************************************************************************F*/
int32_t ProtoHttpControl(ProtoHttpRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'apnd')
    {
        return(_ProtoHttpSetAppendHeader(pState, (const char *)pValue));
    }
    if (iSelect == 'auth')
    {
        NetPrintf(("protohttpxboxone: [%p] %s authentication\n", pState, pValue ? "enabling" : "disabling"));
        pState->bAuthenticate = iValue ? TRUE : FALSE;
    }
    if (iSelect == 'ires')
    {
        // save 'ideal' max buffer size
        pState->critical.responseBuffer.iInpMaxIdeal = iValue;
        // resize the buffer
        return(_ProtoHttpResizeBuffer(pState, iValue));
    }
    if (iSelect == 'keep')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] setting keep-alive to %d\n", pState, iValue));
        pState->iKeepAlive = pState->iKeepAliveDflt = iValue;
        return(0);
    }
    if (iSelect == 'maxr')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] setting max recv rate to %d bytes/sec\n", pState, iValue));
        pState->RecvRate.uMaxRate = iValue;
        return(0);
    }
    if (iSelect == 'maxs')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] setting max send rate to %d bytes/sec\n", pState, iValue));
        pState->SendRate.uMaxRate = iValue;
        return(0);
    }
    if (iSelect == 'odth')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: [%p] setting ondata_threshold property to %d\n", iValue));
        pState->iOnDataThreshold = iValue;
        return(0);
    }
    if (iSelect == 'prxy')
    {
        NetPrintf(("protohttpxboxone: [%p] setting %s as proxy server\n", pState, pValue));
        ds_strnzcpy(pState->strProxy, (const char *)pValue, sizeof(pState->strProxy));
        return(0);
    }
    if (iSelect == 'rmax')
    {
        pState->iMaxRedirect = iValue;
        return(0);
    }
    if (iSelect == 'spam')
    {
        pState->iVerbose = iValue;
        return(0);
    }
    if (iSelect == 'time')
    {
        NetPrintf(("protohttpxboxone: [%p] setting timeout to %d ms\n", pState, iValue));
        pState->uTimeout = (unsigned)iValue;
        return(0);
    }
    if (iSelect == 'xtok')
    {
        pState->bGetAuthToken = iValue ? TRUE : FALSE;
        pState->iAuthTokenIdx = (int8_t)iValue2;
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: auth token acquisition %s (user index=%d)\n", (pState->bGetAuthToken == TRUE) ? "enabled" : "disabled", iValue2));
        if (pValue != NULL)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: setting token urn=%s\n", pValue));
            ds_strnzcpy(pState->strTokenUrl, (const char *)pValue, sizeof(pState->strTokenUrl));
        }
        else
        {
            pState->strTokenUrl[0] = '\0';
        }
        return(0);
    }
    NetPrintf(("protohttpxboxone: [%p] '%C' control is invalid\n", pState, iSelect));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpStatus

    \Description
        Return status of current HTTP transfer.  Status type depends on selector.

    \Input *pState  - reference pointer
    \Input iSelect  - info selector (see Notes)
    \Input *pBuffer - [out] storage for selector-specific output
    \Input iBufSize - size of buffer

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'body'      negative=failed or pending, else size of body (for 64bit size, get via pBuffer)
        'code'      negative=no response, else server response code (ProtoHttpResponseE)
        'data'      negative=failed, zero=pending, positive=amnt of data ready
        'date'      last-modified date, if available
        'done'      negative=failed, zero=pending, positive=done
        'head'      returns size of header
        'host'      current host copied to output buffer
        'hres'      return hResult containing either the socket error or ssl error or the http status code 
        'htxt'      current received http header text copied to output buffer
        'info'      copies most recent info header received, if any, to output buffer (one time only)
        'imax'      size of input buffer
        'iovr'      returns input buffer overflow size (valid on PROTOHTTP_MINBUFF result code)
        'maxr'      returns configured max recv rate (bytes/sec; zero=uncapped)
        'maxs'      returns configured max send rate (bytes/sec; zero=uncapped)
        'port'      current port
        'rmax'      returns currently configured max redirection count
        'rtxt'      most recent http request header text copied to output buffer
        'time'      TRUE if the client timed out the connection, else FALSE
    \endverbatim

    \Version 07/02/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpStatus(ProtoHttpRefT *pState, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    int32_t iResult;

    if (iSelect == 'body')
    {
        if ((pBuffer != NULL) && (iBufSize == sizeof(pState->responseHeader.iBodySize)))
        {
            ds_memcpy(pBuffer, &pState->responseHeader.iBodySize, iBufSize);
        }
        return(pState->responseHeader.iBodySize);
    }
    // return response code
    if (iSelect == 'code')
    {
        return(pState->responseHeader.iHdrCode);
    }
    // data check: negative=failed, zero=pending, positive=data ready
    if (iSelect == 'data')
    {
        NetCritEnter(&pState->critical.Locker);
        iResult = pState->critical.responseBuffer.iInpLen;
        NetCritLeave(&pState->critical.Locker);
        return(iResult);
    }
    if (iSelect == 'date')
    {
        return(pState->responseHeader.iHdrDate);
    }
    // done check: negative=failed, zero=pending, positive=done
    if (iSelect == 'done')
    {
        ProtoHttpStateE eRequestState;
        int32_t iResult = 0; // default to in progress

        NetCritEnter(&pState->critical.Locker);
        eRequestState = pState->critical.eRequestState;
        NetCritLeave(&pState->critical.Locker);

        if ((eRequestState == ST_IDLE) || (eRequestState == ST_DONE))
        {
            iResult = 1;
        }
        else if (eRequestState == ST_FAIL)
        {
            iResult = -1;
        }

        return(iResult);
    }
    // most recent ssl error (not supported)
    if (iSelect == 'essl')
    {
        return(0);
    }
    // parse the tokens
    if (iSelect == 'head')
    {
        return(pState->responseHeader.iHeadSize);
    }
    // return current host
    if (iSelect == 'host')
    {
        ds_strnzcpy((char *)pBuffer, pState->strHost, iBufSize);
        return(0);
    }
    //return hResult containing the http status code 
    if (iSelect == 'hres')
    {
        // return the http status code if there is one if not return the ssl error in hresult format
        if (pState->responseHeader.iHdrCode > 0)
        {
            return(DirtyErrGetHResult(DIRTYAPI_PROTO_HTTP, pState->responseHeader.iHdrCode, (pState->responseHeader.iHdrCode >= PROTOHTTP_RESPONSE_CLIENTERROR) ? TRUE : FALSE));
        }
        return(0);
    }
    if (iSelect == 'htxt')
    {
        ds_strnzcpy((char *)pBuffer, pState->responseHeader.strHdr, iBufSize);
        return(0);
    }
    // return size of input buffer
    if (iSelect == 'imax')
    {
        NetCritEnter(&pState->critical.Locker);
        iResult = pState->critical.responseBuffer.iInpMax;
        NetCritLeave(&pState->critical.Locker);
        return(iResult);
    }
    // return input overflow amount (valid after PROTOHTTP_MINBUFF result code)
    if (iSelect == 'iovr')
    {
        NetCritEnter(&pState->critical.Locker);
        iResult = pState->critical.responseBuffer.iInpOvr;
        NetCritLeave(&pState->critical.Locker);
        return(iResult);
    }
    // return configured max recv rate
    if (iSelect == 'maxr')
    {
        return(pState->RecvRate.uMaxRate);
    }
    // return configured max send rate
    if (iSelect == 'maxs')
    {
        return(pState->SendRate.uMaxRate);
    }
    // return current port
    if (iSelect == 'port')
    {
        return(pState->iPort);
    }
    // return current redirection max
    if (iSelect == 'rmax')
    {
        return(pState->iMaxRedirect);
    }
    // return most recent http request header text
    if (iSelect == 'rtxt')
    {
        ds_strnzcpy((char *)pBuffer, pState->strRequestHdr, iBufSize);
        return(0);
    }
    // return timeout indicator
    if (iSelect == 'time')
    {
        return(pState->critical.bTimeout);
    }

    // invalid token
    NetPrintfVerbose((pState->iVerbose, 2, "protohttpxboxone: [%p] '%C' status is invalid\n", pState, iSelect));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpCheckKeepAlive

    \Description
        Checks whether a request for the given Url would be a keep-alive transaction.
        Unlike the 'generic' version, this version is not aware of the current
        connection status, only whether the previous request was made to the same
        host/port/security setting.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url to check

    \Output
        int32_t         - TRUE if a request to this Url would use keep-alive, else FALSE

    \Version 07/18/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpCheckKeepAlive(ProtoHttpRefT *pState, const char *pUrl)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // parse the url
    pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // see if a request for this url would use keep-alive
    if ((pState->iKeepAlive > 0) && (pState->iPort == iPort) && (pState->iSecure == iSecure) && !ds_stricmp(pState->strHost, strHost))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pState      - reference pointer

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
void ProtoHttpUpdate(ProtoHttpRefT *pState)
{
    int32_t iResult;

    if (!NetCritTry(&pState->critical.Locker))
    {
        return;
    }

    // check for timeout
    if ((pState->critical.eRequestState != ProtoHttpStateE::ST_IDLE) && (pState->critical.eRequestState != ProtoHttpStateE::ST_DONE) && (pState->critical.eRequestState != ProtoHttpStateE::ST_FAIL))
    {
        if (NetTickDiff(NetTick(), pState->uTimer) >= 0)
        {
            NetPrintf(("protohttpxboxone: [%p] server timeout (state=%s)\n", pState, _ProtoHttp_strRequestState[pState->critical.eRequestState]));
            // set to failure state and mark that a timeout occurred
            pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
            pState->critical.bTimeout = TRUE;
        }
    }

    // waiting for auth token
    if (pState->critical.eRequestState == ProtoHttpStateE::ST_AUTH)
    {
        if ((iResult = DirtyAuthCheckToken(-1, pState->strTokenUrl, NULL, NULL)) == DIRTYAUTH_SUCCESS)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttpxboxone: token acquired; transitioning to connect state\n"));
            pState->critical.eRequestState = ST_CONN;
        }
        else if (iResult == DIRTYAUTH_PENDING)
        {
            NetCritLeave(&pState->critical.Locker);
            return;
        }
        else
        {
            NetPrintf(("protohttpxboxone: token acquisition failed\n"));
            pState->critical.eRequestState = ST_FAIL;
        }
    }

    // issue request
    if (pState->critical.eRequestState == ProtoHttpStateE::ST_CONN)
    {
        if ((iResult = _ProtoHttpRequest(pState)) == 0)
        {
            pState->critical.eRequestState = ProtoHttpStateE::ST_RESP;
        }
        else if (iResult < 0)
        {
            NetPrintf(("protohttpxboxone: [%p] failed to send request\n", pState));
            pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
        }
        // else we re-try; this is specifically to support the OR_INVALID_OXID bug workaround
    }

    // wait for response header
    if (pState->critical.eRequestState == ProtoHttpStateE::ST_RESP)
    {
        // process redirect headers, if any
        for ( ; _ProtoHttpProcessRedirect(pState) > 0; )
            ;

        // implement 302/303 redirection bug workaround
        if (pState->critical.bRedirBugWorkaround)
        {
            NetPrintf(("protohttpxboxone: [%p] executing redirection bug workaround\n", pState));
            // clean up xmlhttp object
            _ProtoHttpXmlHttpDestroy(pState, pState->critical.pXmlHttp);
            // change request to a GET and data size to zero
            pState->eRequestType = PROTOHTTP_REQUESTTYPE_GET;
            pState->critical.iRequestSize = 0;
            // format request url
            _ProtoHttpFormatUrl(pState, pState->strLocation);
            // clear workaround flag and location url to prevent re-issue
            pState->critical.bRedirBugWorkaround = FALSE;
            pState->strLocation[0] = '\0';
            // create new XmlHttp object for this request
            if ((pState->critical.pXmlHttp = _ProtoHttpXmlHttpCreate(pState)) == NULL)
            {
                NetPrintf(("protohttpxboxone: [%p] failed to create new xmlhttp object\n", pState));
                pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
                NetCritLeave(&pState->critical.Locker);
                return;
            }
            // move to conn state to issue redirect request
            pState->critical.eRequestState = ProtoHttpStateE::ST_CONN;
            NetCritLeave(&pState->critical.Locker);
            return;
        }

        // wait until we've received headers
        if (pState->critical.bHeaderReceived == FALSE)
        {
            NetCritLeave(&pState->critical.Locker);
            return;
        }

        // move to process header
        pState->critical.eRequestState = ProtoHttpStateE::ST_HEAD;
    }

    // process received header
    if (pState->critical.eRequestState == ProtoHttpStateE::ST_HEAD)
    {
        // parse the header
        if (_ProtoHttpParseHeader(pState) < 0)
        {
            NetPrintf(("protohttpxboxone: [%p] failed to parse header\n", pState));
            pState->critical.eRequestState = ProtoHttpStateE::ST_FAIL;
            NetCritLeave(&pState->critical.Locker);
            return;
        }

        // move to process body
        pState->critical.eRequestState = ProtoHttpStateE::ST_BODY;
    }

    // process received body
    if ((pState->critical.eRequestState == ProtoHttpStateE::ST_BODY) || (pState->critical.eRequestState == ProtoHttpStateE::ST_DONE))
    {
        if (pState->critical.bBodyReceived)
        {
            // if we didn't get a body length with the header, set it now
            if (pState->responseHeader.iBodySize == -1)
            {
                pState->responseHeader.iBodySize = pState->critical.responseBuffer.iInpTotal;
            }
            // finished receiving response
            pState->critical.eRequestState = ProtoHttpStateE::ST_DONE;

            // clean up XmlHttp object
            if (pState->critical.pXmlHttp != NULL)
            {
                _ProtoHttpXmlHttpDestroy(pState, pState->critical.pXmlHttp);
                pState->critical.pXmlHttp = NULL;
            }
        }
    }

    // failure state -- abort if required
    if (pState->critical.eRequestState == ProtoHttpStateE::ST_FAIL)
    {
        _ProtoHttpAbort(pState);
    }

    // write callback processing
    if (pState->pWriteCb != NULL)
    {
        _ProtoHttpWriteCbProcess(pState);
    }

    NetCritLeave(&pState->critical.Locker);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetCACert

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all HTTP instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

    \Input *pCACert - pointer to certificate data
    \Input iCertSize- certificate size in bytes

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t ProtoHttpSetCACert(const uint8_t *pCACert, int32_t iCertSize)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetCACert2

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all HTTP instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

        This version of the function does not validate the CA at load time.
        The X509 certificate data will be copied and retained until the CA
        is validated, either by use of ProtoHttpValidateAllCA() or by the CA
        being used to validate a certificate.

    \Input *pCACert - pointer to certificate data
    \Input iCertSize- certificate size in bytes

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t ProtoHttpSetCACert2(const uint8_t *pCACert, int32_t iCertSize)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpValidateAllCA

    \Description
        Validate all CA that have been added but not yet been validated.  Validation
        is a one-time process and disposes of the X509 certificate that is retained
        until validation.

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/01/2012 (akirchner)
*/
/********************************************************************************F*/
int32_t ProtoHttpValidateAllCA(void)
{
    return(0);
}

/*F*************************************************************************/
/*!
    \Function ProtoHttpClrCACerts

    \Description
        Clears all dynamic CA certs from the list.

    \Version 11/01/2012 (akirchner)
*/
/**************************************************************************F*/
void ProtoHttpClrCACerts(void)
{
}
