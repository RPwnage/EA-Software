/*H********************************************************************************/
/*!
    \File serverdiagnostic.c

    \Description
        HTTP diagnostic page that displays server status summary.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 03/13/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/proto/protohttputil.h"
#include "DirtySDK/proto/protohttpserv.h"

#include "dirtycast.h"
#include "ipmask.h"
#include "serverdiagnostic.h"

/*** Defines **********************************************************************/

//! module memid  -  vsdi --> VoipServer DIagnostic
#define SERVERDIAGNOSTIC_MEMID      ('vsdi')


#define SERVERDIAGNOSTIC_VERBOSE    (FALSE)

/*** Type Definitions *************************************************************/

// module state
struct ServerDiagnosticT
{
    //! Module memory goup
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    IPMaskT TrustMask;
    ServerDiagnosticCallbackT *pCallback;
    ServerDiagnosticGetResponseTypeCallbackT *pGetResponseTypeCb;
    void *pUserData;
    char strServerName[32];
    uint32_t uRefreshRate;

    ProtoHttpServRefT* pHttpServ;

    int32_t iBufSize;
    int32_t iBufLen;

    uint16_t uTunnelPort;   //!< when non-zero, will be added as a "TunnelPort" http header to http responses (required for VS/VMapp interaction)

    char    strBuffer[1];   //!< variable length output buffer (must come last!)
};

/*** Variables ********************************************************************/

// Re-useable response for when IPMask does not match
static const char _ServerDiagnostic_Forbidden[] = 
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"><HTML>"
    "<HEAD>"
    "<TITLE>Access Forbidden</TITLE>"
    "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=Windows-1252\">"
    "</HEAD>"
    "<BODY><TT><PRE>"
    "<h1>Access Forbidden</h1>";

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ServerDiagnosticRequestCb

    \Description
        Callback to handle the request

    \Input *pRequest    - data about the request
    \Input *pResponse   - data on how we want to respond
    \Input *pUserData   - user data

    \Output
        int32_t         - zero=success, positive=in-progress, negative=error
*/
/********************************************************************************F*/
static int32_t _ServerDiagnosticRequestCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    ServerDiagnosticT* pServerDiagnostic = (ServerDiagnosticT *)pUserData;
    ServerDiagnosticResponseTypeE eResponseType = SERVERDIAGNOSTIC_RESPONSETYPE_DIAGNOSTIC;
    uint32_t uRefreshRate = pServerDiagnostic->uRefreshRate;
    const char* strQuery = pRequest->strQuery;
    char strName[64], strValue[64];
    char strUrl[sizeof(pRequest->strUrl) + sizeof(pRequest->strQuery)];

    // parse out the refresh rate if it exists in the query string
    while (ProtoHttpGetNextParam(NULL, strQuery, strName, sizeof(strName), strValue, sizeof(strValue), &strQuery) == 0)
    {
        if (ds_stricmp(strName, "refresh") == 0)
        {
            uRefreshRate = strtol(strValue, NULL, 10);
        }
    }

    if (pServerDiagnostic->pGetResponseTypeCb != NULL)
    {
        eResponseType = pServerDiagnostic->pGetResponseTypeCb(pServerDiagnostic, pRequest->strUrl);
    }

    if (eResponseType == SERVERDIAGNOSTIC_RESPONSETYPE_XML)
    {
        // Set content-type
        ds_strnzcpy(pResponse->strContentType, "text/xml", sizeof(pResponse->strContentType));
    }
    else
    {
        // Set content-type
        ds_strnzcpy(pResponse->strContentType, "text/html", sizeof(pResponse->strContentType));

        pServerDiagnostic->iBufSize = ds_snzprintf(pServerDiagnostic->strBuffer, pServerDiagnostic->iBufLen,
            "<HTML><HEAD>"
            "<TITLE>%s Status</TITLE>"
            "<META HTTP-EQUIV=\"Refresh\" CONTENT=%d>"
            "</HEAD>\r\n"
            "<BODY><TT><PRE>",
            pServerDiagnostic->strServerName,
            uRefreshRate);
    }

    // the callbacks are parsing the query manually out of the strUrl so combine
    // them together so we don't break any functionality
    ds_snzprintf(strUrl, sizeof(strUrl), "%s%s", pRequest->strUrl, pRequest->strQuery);

    // call user callback to format stat data
    pServerDiagnostic->iBufSize += pServerDiagnostic->pCallback(pServerDiagnostic, strUrl, pServerDiagnostic->strBuffer + pServerDiagnostic->iBufSize,
                                                                pServerDiagnostic->iBufLen - pServerDiagnostic->iBufSize, pServerDiagnostic->pUserData);

    // append 'TunnelPort' header if tunnel port value is known
    if (pServerDiagnostic->uTunnelPort != 0)
    {
        /* Adding voipserver's tunnel port as an http header
           This is specifically used when the metrics are polled by the VMapp. This mechanism allows the VMapp to know what port to look for when
           fetching the udp recv/send buffer length from /proc/net/udp6. */
        pResponse->iHeaderLen += ds_snzprintf(&pResponse->strHeader[0] + pResponse->iHeaderLen, sizeof(pResponse->strHeader) - pResponse->iHeaderLen, "TunnelPort: %d\r\n", pServerDiagnostic->uTunnelPort);
    }

    // a formatting overflow will safely preserve buffer integrity, but can result in a size greater than the length
    if (pServerDiagnostic->iBufSize > pServerDiagnostic->iBufLen)
    {
        pServerDiagnostic->iBufSize = pServerDiagnostic->iBufLen;
    }

    // set the response and content-length header to the size of the buffer
    pResponse->eResponseCode = PROTOHTTP_RESPONSE_OK;
    pResponse->iContentLength = pServerDiagnostic->iBufSize;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ServerDiagnosticReceiveCb

    \Description
        Callback to read request data

    \Input *pResponse   - data about the request
    \Input *pBuffer     - input buffer to read request data from
    \Input iBufSize     - the size of the input buffer
    \Input *pUserData   - user data

    \Output
        int32_t         - positive=amount of data read, negative=error
*/
/********************************************************************************F*/
static int32_t _ServerDiagnosticReceiveCb(ProtoHttpServRequestT *pRequest, const char *pBuffer, int32_t iBufSize, void *pUserData)
{
    if (pBuffer == NULL || iBufSize == 0)
    {
        return(0);
    }

    // Server Diagnostic doesn't support taking any body data for their requests
    // so we just let ProtoHttpServ that we processed all the data
    return(iBufSize);
}

/*F********************************************************************************/
/*!
    \Function _ServerDiagnosticSendCb

    \Description
        Callback to send response data

    \Input *pResponse   - data about the response
    \Input *pBuffer     - output buffer to write response to
    \Input iBufSize     - the size of the output buffer
    \Input *pUserData   - user data

    \Output
        int32_t         - positive=amount of data written, negative=error
*/
/********************************************************************************F*/
static int32_t _ServerDiagnosticSendCb(ProtoHttpServResponseT *pResponse, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    ServerDiagnosticT *pServerDiagnostic = (ServerDiagnosticT *)pUserData;
    int32_t iResult;

    // done sending the response, reset the state
    if (pBuffer == NULL || iBufSize == 0)
    {
        pServerDiagnostic->iBufSize = 0;
        pServerDiagnostic->strBuffer[0] = '\0';
        return(0);
    }

    // copy the response into the buffer and increment the position
    if ((iResult = ds_strsubzcpy(pBuffer, iBufSize, pServerDiagnostic->strBuffer + pResponse->iContentRead, pServerDiagnostic->iBufSize)) > 0)
    {
        return(iResult);
    }
    else
    {
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function _ServerDiagnosticHeaderCb

    \Description
        Does processing on the request headers before handling the request

    \Input *pRequest    - [in/out] data used to handle the request
    \Input *pResponse   - [out] data used to handle the response (in the case of errors)
    \Input *pUserData   - user data

    \Output
        int32_t         - zero=success, negative=error
*/
/********************************************************************************F*/
static int32_t _ServerDiagnosticHeaderCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    ServerDiagnosticT *pServerDiagnostic = (ServerDiagnosticT *)pUserData;

    // check trust settings
    if (!IPMaskMatch(&pServerDiagnostic->TrustMask, pRequest->uAddr))
    {
        DirtyCastLogPrintf("serverdiagnostic: ignoring request from untrusted address %a\n", pRequest->uAddr);
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_FORBIDDEN;

        // save the response for sending later
        pServerDiagnostic->iBufSize = ds_snzprintf(pServerDiagnostic->strBuffer, pServerDiagnostic->iBufLen, "%s", _ServerDiagnostic_Forbidden);
        pResponse->iContentLength = pServerDiagnostic->iBufSize;

        // Set content-type
        ds_strnzcpy(pResponse->strContentType, "text/html", sizeof(pResponse->strContentType));
        return(-1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ServerDiagnosticLogCb

    \Description
        Logging function for ProtoHttpServ

    \Input *pText       - text to log
    \Input *pUserData   - user data

    \Output
        int32_t         - success of logging
*/
/********************************************************************************F*/
static void _ServerDiagnosticLogCb(const char *pText, void *pUserData)
{
#if SERVERDIAGNOSTIC_VERBOSE
    DirtyCastLogPrintf("%s", pText);
#endif
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ServerDiagnosticCreate

    \Description
        Create ServerDiagnostic module.

    \Input uStatusPort      - port to listen on for status inquiries
    \Input *pCallback       - callback to format stats info
    \Input *pUserData       - user data ref for callback
    \Input *pServerName     - name that will appear in html response
    \Input iBufLen          - size of read/response buffer to allocate
    \Input *pTagPrefix      - tag name prefix
    \Input *pCommandTags    - command-line config arguments
    \Input *pConfigTags     - config-file config arguments
    
    \Output
        ServerDiagnosticT * - module state, or NULL if create failed

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
ServerDiagnosticT *ServerDiagnosticCreate(uint32_t uStatusPort, ServerDiagnosticCallbackT *pCallback, void *pUserData, const char *pServerName, int32_t iBufLen, const char *pTagPrefix, const char *pCommandTags, const char *pConfigTags)
{
    ServerDiagnosticT *pServerDiagnostic;
    int32_t iMemSize = sizeof(*pServerDiagnostic) + iBufLen - sizeof(pServerDiagnostic->strBuffer);
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // create module state
    if ((pServerDiagnostic = (ServerDiagnosticT *)DirtyMemAlloc(iMemSize, SERVERDIAGNOSTIC_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("serverdiagnostic: could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pServerDiagnostic, iMemSize);
    pServerDiagnostic->iMemGroup = iMemGroup;
    pServerDiagnostic->pMemGroupUserData = pMemGroupUserData;
    pServerDiagnostic->pCallback = pCallback;
    pServerDiagnostic->pUserData = pUserData;
    pServerDiagnostic->iBufLen = iBufLen;
    ds_strnzcpy(pServerDiagnostic->strServerName, pServerName, sizeof(pServerDiagnostic->strServerName));
    pServerDiagnostic->uRefreshRate = 60*60;    // default refresh rate to very slow once an hour

    // create protohttpserv module
    if ((pServerDiagnostic->pHttpServ = ProtoHttpServCreate((int32_t)uStatusPort, FALSE, pServerName)) == NULL)
    {
        DirtyCastLogPrintf("serverdiagnostic: unable to create protohttpserv module on port %u\n", uStatusPort);
        ServerDiagnosticDestroy(pServerDiagnostic);
        return(NULL);
    }
    ProtoHttpServCallback(pServerDiagnostic->pHttpServ, _ServerDiagnosticRequestCb, _ServerDiagnosticReceiveCb, _ServerDiagnosticSendCb, _ServerDiagnosticHeaderCb, _ServerDiagnosticLogCb, (void*)pServerDiagnostic);

    // init TrustMask from config
    IPMaskInit(&pServerDiagnostic->TrustMask, pTagPrefix, "trustmatch", "trust", pCommandTags, pConfigTags, TRUE);

    NetPrintf(("serverdiagnostic: listening on diagnostic socket bound to port %d\n", uStatusPort));

    // return module state to caller
    return(pServerDiagnostic);
}

/*F********************************************************************************/
/*!
    \Function ServerDiagnosticDestroy

    \Description
        Destroy ServerDiagnostic module.

    \Input *pServerDiagnostic   - module state.
    
    \Output
        None.

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
void ServerDiagnosticDestroy(ServerDiagnosticT *pServerDiagnostic)
{
    if (pServerDiagnostic->pHttpServ != NULL)
    {
        ProtoHttpServDestroy(pServerDiagnostic->pHttpServ);
    }
    DirtyMemFree(pServerDiagnostic, SERVERDIAGNOSTIC_MEMID, pServerDiagnostic->iMemGroup, pServerDiagnostic->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ServerDiagnosticCallback

    \Description
        Set ServerDiagnostic callbacks

    \Input *pServerDiagnostic   - module state.
    \Input *pCallback           - callback to format stats info
    \Input *pGetResponseTypeCb  - (optional) callback to get response type
    
    \Version 10/13/2010 (jbrookes)
*/
/********************************************************************************F*/
void ServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, ServerDiagnosticCallbackT *pCallback, ServerDiagnosticGetResponseTypeCallbackT *pGetResponseTypeCb)
{
    pServerDiagnostic->pCallback = pCallback;
    pServerDiagnostic->pGetResponseTypeCb = pGetResponseTypeCb;
}

/*F********************************************************************************/
/*!
    \Function ServerDiagnosticUpdate

    \Description
        Update ServerDiagnostic module

    \Input *pServerDiagnostic   - module state.
    
    \Output
        None.

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
void ServerDiagnosticUpdate(ServerDiagnosticT *pServerDiagnostic)
{
    // update the httpserv module so it can process requests
    // and send responses
    ProtoHttpServUpdate(pServerDiagnostic->pHttpServ);
}

/*F********************************************************************************/
/*!
    \Function ServerDiagnosticStatus

    \Description
        Format uptime into output buffer.

    \Input *pServerDiagnostic   - module state
    \Input iStatus              - status selector
    \Input iValue               - selector-specific
    \Input *pBuffer             - buffer pointer [selector-specific]
    \Input iBufLen              - size of buffer
    
    \Output
        int32_t                 - selector result

    \Version 03/15/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t ServerDiagnosticStatus(ServerDiagnosticT *pServerDiagnostic, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufLen)
{
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ServerDiagnosticControl

    \Description
        Format uptime into output buffer.

    \Input *pServerDiagnostic   - module state
    \Input iControl             - control selector
    \Input iValue               - control value
    \Input iValue2              - control value
    \Input *pValue              - control value

    \Output
        int32_t                 - selector result

    \Version 01/03/2019 (mclouatre)
*/
/********************************************************************************F*/
int32_t ServerDiagnosticControl(ServerDiagnosticT *pServerDiagnostic, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'tprt')
    {
        pServerDiagnostic->uTunnelPort = iValue;
        DirtyCastLogPrintf("serverdiagnostic: 'tprt' used to register tunnel port %d\n", pServerDiagnostic->uTunnelPort);
        return(0);
    }
    return(-1);
}


/*F********************************************************************************/
/*!
    \Function ServerDiagnosticFormatUptimeBlaze

    \Description
        Format uptime into output buffer.

    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - size of buffer to format data into
    \Input uCurTime             - current time
    \Input uStartTime           - starting time

    \Output
        int32_t                 - number of characters written to buffer

    \Version 03/13/2007 (cvienneau)
*/
/********************************************************************************F*/
int32_t ServerDiagnosticFormatUptimeBlaze(char *pBuffer, int32_t iBufSize, time_t uCurTime, time_t uStartTime)
{
    uint32_t uUpDays, uUpHours, uUpMinutes, uUpSeconds;
    int32_t iOffset = 0;

    uUpSeconds = (uint32_t)difftime(uCurTime, uStartTime);
    uUpDays = uUpSeconds/(60*60*24);
    uUpSeconds -= uUpDays*60*60*24;
    uUpHours = uUpSeconds/(60*60);
    uUpSeconds -= uUpHours*60*60;
    uUpMinutes = uUpSeconds/60;
    uUpSeconds -= uUpMinutes*60;

    iOffset = ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, ",Uptime=");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%2d%s", uUpDays, "d");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%2d%s", uUpHours, "h");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%2d%s", uUpMinutes, "m");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%2d%s", uUpSeconds, "s");

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function ServerDiagnosticFormatUptime

    \Description
        Format uptime into output buffer.

    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - size of buffer to format data into
    \Input uCurTime             - current time
    \Input uStartTime           - starting time
    
    \Output
        int32_t                 - number of characters written to buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t ServerDiagnosticFormatUptime(char *pBuffer, int32_t iBufSize, time_t uCurTime, time_t uStartTime)
{
    uint32_t uUpDays, uUpHours, uUpMinutes, uUpSeconds;
    int32_t iOffset;

    uUpSeconds = (uint32_t)difftime(uCurTime, uStartTime);
    uUpDays = uUpSeconds/(60*60*24);
    uUpSeconds -= uUpDays*60*60*24;
    uUpHours = uUpSeconds/(60*60);
    uUpSeconds -= uUpHours*60*60;
    uUpMinutes = uUpSeconds/60;
    uUpSeconds -= uUpMinutes*60;

    iOffset = ds_snzprintf(pBuffer, iBufSize, "server uptime: ");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%d %s, ", uUpDays, uUpDays==1?"day":"days");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%d %s, ", uUpHours, uUpHours==1?"hour":"hours");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%d %s, ", uUpMinutes, uUpMinutes==1?"minute":"minutes");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%d %s\n", uUpSeconds, uUpSeconds==1?"second":"seconds");

    return(iOffset);
}
