/*H********************************************************************************/
/*!
    \File httpmgr.c

    \Description
        Implements basic http get and post client.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/28/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "dirtysock.h"
#include "protohttpmanager.h"
#include "protossl.h"

#include "zlib.h"
#include "zmem.h"
#include "zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define HTTP_BUFSIZE    (100)
#define HTTP_RATE       (1)
#define HTTP_MAXCMDS    (64)        // max number of in-flight commands

#define HTTP_XTRAHDR0   ""
#define HTTP_XTRAHDR1   "X-Agent: DirtySock HTTP Tester\r\n"    // test "normal" replacement (replaces Accept: header)
#define HTTP_XTRAHDR2   "User-Agent: DirtySock HTTP Tester\r\n" // test "extended" replacement (replaces User-Agent: and Accept: headers)

//$$ tmp -- special test header used for hard-coded multipart/form-data testing -- this should be removed at some point when we have real multipart/form-data support
#define HTTP_XTRAHDR3   "Content-Type: multipart/form-data; boundary=TeStInG\r\n" \
                        "User-Agent: DirtySock HTTP Tester\r\n" \
                        "Accept: */*\r\n"

#define HTTP_APNDHDR    HTTP_XTRAHDR0

/*** Function Prototypes **********************************************************/

static int32_t _CmdHttpMgrIdleCB(ZContext *argz, int32_t argc, char *argv[]);

/*** Type Definitions *************************************************************/

typedef struct HttpStateT    // individual request states
{
    enum
    {
        IDLE, DNLOAD, UPLOAD, MGET
    } state;
    int32_t iHandle;
    char strCookie[1024];
    const char *pInpData;
    int32_t iDataSize;
    int32_t iSentSize;
    char *pOutData;
    int32_t iOutSize;
    int32_t show;
    int32_t count;
    int32_t sttime;
    ZFileT iOutFile;
    ZFileT iInpFile;
    uint8_t bStreaming;
}HttpStateT;

typedef struct HttpMgrRefT      // module state storage
{
    HttpManagerRefT *pHttpManager;
    int32_t iDebugLevel;
    uint8_t bCallbackInstalled;
    char strModuleName[32];
    char strMgetFilename[256];
    const char *pMgetBuffer;
    const char *pMgetOffset;
    uint32_t uMgetStart;
    uint32_t uMgetTransactions;
    uint8_t bMgetShowUrlsOnly;
    uint8_t bRecvAll; // use recvall instead of recv

    // module state
    HttpStateT States[HTTP_MAXCMDS];

} HttpMgrRefT;

/*** Variables ********************************************************************/

static HttpMgrRefT _HttpMgr_Ref = { NULL, 0, 0, "", "", "", "", 0, 0, 0, 0 };

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _HttpCustomHeaderCallback

    \Description
        ProtoHttp send header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - received header
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpMgrRefT)
    
    \Output
        int32_t         - zero

    \Notes
        The header returned should be terminated by a *single* CRLF; ProtoHttp will
        append the final CRLF to complete the header.  The callback may return the
        size of the header, or zero, in which case ProtoHttp will calculate the
        headersize using strlen().

    \Version 02/24/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpCustomHeaderCallback(ProtoHttpRefT *pProtoHttp, char *pHeader, uint32_t uHeaderSize, const char *pData, uint32_t uDataLen, void *pUserRef)
{
    HttpStateT *pState = (HttpStateT *)pUserRef;
    char *pAppend;

    // find append point
    if ((pAppend = stristr(pHeader, "Accept: */*\r\n")) != NULL)
    {
        int32_t uBufSize = uHeaderSize - (pHeader - pAppend);

        // append our header info
        if (pState->strCookie[0] != '\0')
        {
            /* $$todo -- cookies aren't really saved across multiple transactions, so that has
               to be solved before this will work */
            snzprintf(pAppend, uBufSize, "Cookie: %s\r\n%s", pState->strCookie, HTTP_APNDHDR);
        }
        else
        {
            strnzcpy(pAppend, HTTP_APNDHDR, uBufSize);
        }

        // recalc header size
        uHeaderSize = (uint32_t)strlen(pHeader);
    }
    else
    {
        ZPrintf("httpmanager: could not find append point for custom header\n");
    }

    return(uHeaderSize);
}

/*F********************************************************************************/
/*!
    \Function _HttpRecvHeaderCallback

    \Description
        ProtoHttp recv header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - received header
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpMgrRefT)
    
    \Output
        None.

    \Version 02/24/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpRecvHeaderCallback(ProtoHttpRefT *pProtoHttp, const char *pHeader, uint32_t uHeaderSize, void *pUserRef)
{
    HttpStateT *pHttp = (HttpStateT *)pUserRef;
    char strBuffer[1024], strName[128];
    const char *pHdrTmp;
    int32_t iLocnSize;

    // check for location header
    if ((iLocnSize = ProtoHttpGetHeaderValue(pProtoHttp, pHeader, "location", NULL, 0, NULL)) > 0)
    {
        ZPrintf("http: location header size=%d\n", iLocnSize);
        if (ProtoHttpGetHeaderValue(pProtoHttp, pHeader, "location", strBuffer, sizeof(strBuffer), NULL) == 0)
        {
            ZPrintf("http: location url='%s'\n", strBuffer);
        }
        else
        {
            ZPrintf("http: error querying location url\n");
        }
    }

    // test ProtoHttpGetNextHeader()
    for (pHdrTmp = pHeader; ProtoHttpGetNextHeader(pProtoHttp, pHdrTmp, strName, sizeof(strName), strBuffer, sizeof(strBuffer), &pHdrTmp) == 0; )
    {
        #if 0
        ZPrintf("http: ===%s: %s\n", strName, strBuffer);
        #endif
    }

    // parse any set-cookie requests
    for (pHdrTmp = pHeader; ProtoHttpGetHeaderValue(pProtoHttp, pHdrTmp, "set-cookie", strBuffer, sizeof(strBuffer), &pHdrTmp) == 0; )
    {
        // print the cookie
        ZPrintf("http: parsed cookie '%s'\n", strBuffer);

        // add field seperator
        if (pHttp->strCookie[0] != '\0')
        {
            strnzcat(pHttp->strCookie, ", ", sizeof(pHttp->strCookie));
        }
        // append to cookie list
        strnzcat(pHttp->strCookie, strBuffer, sizeof(pHttp->strCookie));
    }

    // if we have any cookies, set them here for inclusion in next request
    if (pHttp->strCookie[0] != '\0')
    {
        // format append header
        snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n%s", pHttp->strCookie, HTTP_APNDHDR);
        ProtoHttpControl(pProtoHttp, 'apnd', 0, 0, strBuffer);
    }
    
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list
    
    \Output
        None.

    \Version 02/18/2008 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpMgrUsage(int argc, char *argv[])
{
    if (argc == 2)
    {
        ZPrintf("   execute http get or put operations\n");
        ZPrintf("   usage: %s [cclr|cert|cer2|cver|create|ctrl|destroy|get|mget|put|puts|parse|stat]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "cclr") || !strcmp(argv[2], "cert") || !strcmp(argv[2], "cer2") || !strcmp(argv[2], "cver"))
        {
            ZPrintf("   usage: %s cert|cer2 <certfile> - load certificate file\n", argv[0]);
            ZPrintf("          %s cclr - clear dynamic certificates\n");
            ZPrintf("          %s cver - verify dynamic CA certs\n");
        }
        else if (!strcmp(argv[2], "create"))
        {
            ZPrintf("   usage: %s create <bufsize>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "destroy"))
        {
            ZPrintf("   usage: %s destroy\n", argv[0]);
        }
        else if (!strcmp(argv[2], "get"))
        {
            ZPrintf("   usage: %s get [url] <file>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "mget"))
        {
            ZPrintf("   usage: %s mget <file>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "put"))
        {
            ZPrintf("   usage: %s put [url] [file]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "puts"))
        {
            ZPrintf("   usage: %s puts [url] [file]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "parse"))
        {
            ZPrintf("   usage: %s parse [url]\n", argv[0]);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrAllocState

    \Description
        Allocate an HttpStateT ref for tracking a transaction.

    \Input *pRef        - pointer to httpmgr ref
    
    \Output
        HttpStateT *    - allocated state, or NULL on failure

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static HttpStateT *_CmdHttpMgrAllocState(HttpMgrRefT *pRef)
{
    HttpStateT *pState;
    int32_t iState;

    for (iState = 0; iState < HTTP_MAXCMDS; iState += 1)
    {
        pState = &pRef->States[iState];
        if (pState->iHandle == 0)
        {
            // clear any previous stats
            pState->count = 0;
            pState->show = 0;
            pState->iDataSize = 0;
            pState->iSentSize = 0;
            pState->bStreaming = FALSE;
            pState->iHandle = HttpManagerAlloc(pRef->pHttpManager);

            // set callback user data pointer
            HttpManagerControl(pRef->pHttpManager, pState->iHandle, 'cbup', 0, 0, pState);

            // init start timer
            pState->sttime = NetTick();

            // return initialized state to caller
            return(pState);
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrFreeState

    \Description
        Free an allocated HttpStateT ref.

    \Input *pRef        - pointer to httpmgr ref
    \Input *pState      - pointer to state to free
    

    \Version 02/18/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpMgrFreeState(HttpMgrRefT *pRef, HttpStateT *pState)
{
    // free handle
    HttpManagerFree(pRef->pHttpManager, pState->iHandle);
    // dispose of input data, if any
    if (pState->pInpData != NULL)
    {
        ZMemFree((void *)pState->pInpData);
    }
    // reset state memory
    memset(pState, 0, sizeof(*pState));
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrCheckComplete

    \Description
        See if the HTTP transaction is complete.

    \Input *pRef        - pointer to http ref
    \Input *pCmdName    - module name
    
    \Output
        int32_t         - completion status from ProtoHttpStatus() 'done' selector

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpMgrCheckComplete(HttpManagerRefT *pHttpManager, HttpStateT *pState, const char *pCmdName)
{
    ProtoHttpResponseE eResponse;
    int32_t iResult;

    // check for completion
    if ((iResult = HttpManagerStatus(pHttpManager, pState->iHandle, 'done', NULL, 0)) == 0)
    {
        return(0);
    }

    // get completion result
    eResponse = (ProtoHttpResponseE)HttpManagerStatus(pHttpManager, pState->iHandle, 'code', NULL, 0);
    switch (PROTOHTTP_GetResponseClass(eResponse))
    {
        case PROTOHTTP_RESPONSE_SUCCESSFUL:
            ZPrintf("%s: success (%d)\n", pCmdName, eResponse);
            break;

        case PROTOHTTP_RESPONSE_CLIENTERROR:
            ZPrintf("%s: client error %d\n", pCmdName, eResponse);
            break;

        case PROTOHTTP_RESPONSE_SERVERERROR:
            ZPrintf("%s: server error %d\n", pCmdName, eResponse);
            break;

        default:
            ZPrintf("%s: unexpected result code %d\n", pCmdName, eResponse);
            break;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrMget

    \Description
        Process an mget request

    \Input *pRef        - module state
    
    \Version 02/16/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpMgrMget(HttpMgrRefT *pRef)
{
    const char *pHref = NULL, *pHref2, *pEndQuote;
    char strUrl[1024], strFile[1024];
    char *strArgs[4] = { "httpmgr", "get", "", "" };

    if (pRef->pMgetOffset != NULL)
    {
        // look for hrefs
        for (pHref = strstr(pRef->pMgetOffset, "href=\""); pHref != NULL; pHref = strstr(pHref2, "href=\""))
        {
            // skip href text
            pHref2 = pHref + 6;

            // find trailing quote
            if ((pEndQuote = strchr(pHref2, '"')) == NULL)
            {
                // skip it
                continue;
            }

            // copy the url
            strsubzcpy(strUrl, sizeof(strUrl), pHref2, pEndQuote-pHref2);

            // make sure it's a full URL
            if (strncmp(strUrl, "http://", 7) && strncmp(strUrl, "https://", 8))
            {
                // skip it
                continue;
            }

            // make a filename for the file, skipping http ref
            snzprintf(strFile, sizeof(strFile), "%s-data\\%s", pRef->strMgetFilename, strUrl+7);

            // issue an http command
            strArgs[2] = strUrl;
            strArgs[3] = "";//strFile;

            if (!pRef->bMgetShowUrlsOnly)
            {
                if (CmdHttpMgr(NULL, 3, strArgs) != 0)
                {
                    // safe current url for next time around
                    pRef->pMgetOffset = pHref;
                    break;
                }
                else
                {
                    pRef->uMgetTransactions += 1;
                }
            }
            else
            {
                ZPrintf("%s %s %s %s\n", strArgs[0], strArgs[1], strArgs[2], strArgs[3]);
            }
        }
    }

    // update HttpManager ($$note -- should we need to have this here?)
    HttpManagerUpdate(pRef->pHttpManager);

    // have we parsed the whole buffer?
    if (pHref == NULL)
    {
        // mark that we have completed parsing the buffer
        if (pRef->pMgetOffset != NULL)
        {
            pRef->pMgetOffset = NULL;
        }

        // are all of our transactions complete?
        if (HttpManagerStatus(pRef->pHttpManager, -1, 'busy', NULL, 0) == 0)
        {
            // report time taken for mget request
            ZPrintf("%s: mget completed in %dms (%d transactions)\n", pRef->strModuleName, NetTickDiff(NetTick(), pRef->uMgetStart), pRef->uMgetTransactions);

            // dispose of mget buffer
            ZMemFree((void *)pRef->pMgetBuffer);
            pRef->pMgetBuffer = NULL;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrDownloadProcess

    \Description
        Process a download transaction.

    \Input *pRef    - module state
    
    \Output
        None.

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpMgrDownloadProcess(HttpMgrRefT *pRef, HttpStateT *pState)
{
    char strBuf[1024];
    int32_t iLen;

    // give it time
    HttpManagerUpdate(pRef->pHttpManager);

    // check for data
    if (!pRef->bRecvAll)
    {
        while ((iLen = HttpManagerRecv(pRef->pHttpManager, pState->iHandle, strBuf, 1, sizeof(strBuf))) > 0)
        {
            pState->count += iLen;
            if (pState->iOutFile != 0)
            {
                ZFileWrite(pState->iOutFile, strBuf, iLen);
            }
        }
    }
    else
    {
        // try to receive some data
        if ((iLen = HttpManagerRecvAll(pRef->pHttpManager, pState->iHandle, pState->pOutData, pState->iOutSize)) > 0)
        {
            pState->count = iLen;
            if (pState->iOutFile != 0)
            {
                ZFileWrite(pState->iOutFile, strBuf, iLen);
            }
            iLen = PROTOHTTP_RECVDONE;
        }
        else if (iLen == PROTOHTTP_RECVBUFF)
        {
            char *pNewData;
            int32_t iNewSize;

            // calc new buffer size
            if ((iNewSize = pState->iOutSize) == 0)
            {
                // try getting body size
                if ((iNewSize = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'body', NULL, 0)) > 0)
                {
                    // bump up buffer size for recvall null terminator
                    //$$ TODO V9 -- why 2 required, not 1??
                    iNewSize += 2;
                }
                else
                {
                    // assign a fixed size, since we didn't get a body size
                    iNewSize = 4096;
                }
            }
            else
            {
                iNewSize *= 2;
            }

            // allocate new buffer
            pNewData = ZMemAlloc(iNewSize);
            // if realloc, copy old data and free old pointer
            if (pState->pOutData != NULL)
            {
                memcpy(pNewData, pState->pOutData, pState->iOutSize);
                ZMemFree(pState->pOutData);
            }
            // save new pointer
            pState->pOutData = pNewData;
            pState->iOutSize = iNewSize;
            // swallow error code
            iLen = 0;
            return;
        }
    }

    // see if we should show progress
    if ((pState->count/1024) != (pState->show/1024))
    {
        pState->show = pState->count;
        if (pRef->iDebugLevel > 0)
        {
            ZPrintf("%s: downloaded %d bytes\n", pRef->strModuleName, pState->count);
        }
    }

    // see if we are done
    if ((iLen < 0) && (iLen != PROTOHTTP_RECVWAIT))
    {
        // get the url we issued
        HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'urls', strBuf, sizeof(strBuf));

        // completed successfully?
        if ((iLen == PROTOHTTP_RECVDONE) || (iLen == PROTOHTTP_RECVHEAD))
        {
            int32_t iDlTime = NetTickDiff(NetTick(), pState->sttime);
            int32_t iHdrCode = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'code', NULL, 0);

            ZPrintf("%s: %s download done (%d): %d bytes in %.2f seconds (%.3f k/sec)\n", pRef->strModuleName, strBuf, iHdrCode, pState->count,
                (float)iDlTime/1000.0f, ((float)pState->count * 1000.0f) / ((float)iDlTime * 1024.0f));

            // display some header info (suppress if it's an mget, unless our debuglevel is high)
            if ((pRef->pMgetBuffer == NULL) || (pRef->iDebugLevel > 1))
            {
                if (HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'htxt', strBuf, sizeof(strBuf)) >= 0)
                {
                    ZPrintf("%s response header:\n%s\n", pRef->strModuleName, strBuf);
                }
                
                // display a couple of parsed fields
                if (HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'head', NULL, 0) > 0)
                {
                    time_t tLastMod = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'date', NULL, 0);
                    const char *pTime;

                    if (tLastMod != 0)
                    {
                        if ((pTime = ctime(&tLastMod)) != NULL)
                        {
                            ZPrintf("%s: Last-Modified: %s", pRef->strModuleName, pTime);
                        }
                    }
                    ZPrintf("%s: Content-Length: %d\n", pRef->strModuleName, HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'body', NULL, 0));
                }
            }
        }
        else
        {
            int32_t iSslFail = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'essl', NULL, 0);
            ZPrintf("%s: download failed (err=%d, sslerr=%d)\n", pRef->strModuleName, iLen, iSslFail);
            if ((iSslFail == PROTOSSL_ERROR_CERT_INVALID) || (iSslFail == PROTOSSL_ERROR_CERT_HOST) || (iSslFail == PROTOSSL_ERROR_CERT_NOTRUST))
            {
                ProtoSSLCertInfoT CertInfo;
                if (HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'cert', &CertInfo, sizeof(CertInfo)) == 0)
                {
                    ZPrintf("%s: cert failure (%d): (C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s)\n", pRef->strModuleName, iSslFail,
                        CertInfo.Ident.strCountry, CertInfo.Ident.strState, CertInfo.Ident.strCity, 
                        CertInfo.Ident.strOrg, CertInfo.Ident.strUnit, CertInfo.Ident.strCommon);
                }
                else
                {
                    ZPrintf("%s: could not get cert info\n", pRef->strModuleName);
                }
            }
        }

        // if file exists, close it
        if (pState->iOutFile > 0)
        {
            ZFileClose(pState->iOutFile);
        }
        pState->iOutFile = 0;

        // if output buffer exists, free it
        if (pState->pOutData != NULL)
        {
            pState->pOutData = NULL;
        }
        pState->iOutSize = 0;

        // free state tracking
        _CmdHttpMgrFreeState(pRef, pState);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrUploadProcess

    \Description
        Process an upload transaction.

    \Input *pRef    - module state
    
    \Output
        None.

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpMgrUploadProcess(HttpMgrRefT *pRef, HttpStateT *pState)
{
    char strResponse[1024];
    int32_t iCode, iResult;

    // write data?
    if ((pState->pInpData != NULL) && (pState->iSentSize < pState->iDataSize))
    {
        iResult = HttpManagerSend(pRef->pHttpManager, pState->iHandle, pState->pInpData + pState->iSentSize, pState->iDataSize - pState->iSentSize);
        if (iResult > 0)
        {
            pState->iSentSize += iResult;
            ZPrintf("%s: sent %d bytes\n", pRef->strModuleName, iResult);
        }
    }
    // if streaming upload, signal we are done
    if ((pState->pInpData != NULL) && (pState->iSentSize == pState->iDataSize))
    {
        if (pState->bStreaming == TRUE)
        {
            HttpManagerSend(pRef->pHttpManager, pState->iHandle, NULL, PROTOHTTP_STREAM_END);
            pState->bStreaming = FALSE;
        }

        // done uploading
        ZPrintf("%s: uploaded %d bytes\n", pRef->strModuleName, pState->iSentSize);
        ZMemFree((void *)pState->pInpData);
        pState->pInpData = NULL;
    }

    // give it time
    HttpManagerUpdate(pRef->pHttpManager);

    // see if we've received an HTTP 1xx (INFORMATIONAL) header
    iCode = HttpManagerStatus(pRef->pHttpManager, pState->iHandle, 'info', strResponse, sizeof(strResponse));
    if (PROTOHTTP_GetResponseClass(iCode) == PROTOHTTP_RESPONSE_INFORMATIONAL)
    {
        // got response header, so print it
        NetPrintf(("http: received %d response header\n----------------------------------\n%s----------------------------------\n", iCode, strResponse));
    }

    // done?
    if ((iResult = _CmdHttpMgrCheckComplete(pRef->pHttpManager, pState, pRef->strModuleName)) != 0)
    {
        if (iResult > 0)
        {
            int32_t ultime = NetTickDiff(NetTick(), pState->sttime);
            ZPrintf("%s: upload complete (%d bytes)\n", pRef->strModuleName, pState->iSentSize);
            ZPrintf("upload time: %d bytes in %.2f seconds (%.3f k/sec)\n", pState->iSentSize, (float)ultime/1000.0f,
                ((float)pState->iSentSize * 1000.0f) / ((float)ultime * 1024.0f));

            memset(strResponse, 0, sizeof(strResponse));
            iResult = HttpManagerRecv(pRef->pHttpManager, pState->iHandle, strResponse, 1, sizeof(strResponse));
            if (iResult > 0)
            {
                ZPrintf("http result:\n%s", strResponse);
            }
        }

        _CmdHttpMgrFreeState(pRef, pState);
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpMgrIdleCB

    \Description
        Callback to process while idle

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -
    
    \Output int32_t -

    \Version 09/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpMgrIdleCB(ZContext *argz, int32_t argc, char *argv[])
{
    HttpMgrRefT *pRef = &_HttpMgr_Ref;
    int32_t iState;

    // shut down?
    if ((argc == 0) || (pRef->pHttpManager == NULL))
    {
        if (pRef->pHttpManager != NULL)
        {
            HttpManagerDestroy(pRef->pHttpManager);
            pRef->pHttpManager = NULL;
        }
        return(0);
    }

    for (iState = 0; iState < HTTP_MAXCMDS; iState += 1)
    {
        // process a download
        if (pRef->States[iState].state == DNLOAD)
        {
            _CmdHttpMgrDownloadProcess(pRef, &pRef->States[iState]);
        }

        // process an upload
        if (pRef->States[iState].state == UPLOAD)
        {
            _CmdHttpMgrUploadProcess(pRef, &pRef->States[iState]);
        }
    }

    // process mget request
    if (pRef->pMgetBuffer != NULL)
    {
        _CmdHttpMgrMget(pRef);
    }

    // keep on idling
    return(ZCallback(&_CmdHttpMgrIdleCB, HTTP_RATE));
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttpMgr

    \Description
        Initiate an HTTP transaction.

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -
    
    \Output int32_t -

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdHttpMgr(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iResult = 0, iBufSize = HTTP_BUFSIZE, iNumRefs = 4;
    const char *pFileName = NULL, *pUrl;
    HttpMgrRefT *pRef = &_HttpMgr_Ref;
    HttpStateT *pState = NULL;
    const char *pFileData;
    int32_t iFileSize;

    if (argc < 2)
    {
        return(0);
    }

    // check for help
    if ((argc >= 2) && !strcmp(argv[1], "help"))
    {
        _CmdHttpMgrUsage(argc, argv);
        return(iResult);
    }

    // check for 'parse' command
    if ((argc == 3) && !strcmp(argv[1], "parse"))
    {
        char strKind[5], strHost[128];
        const char *pUri;
        int32_t iPort, iSecure;
        memset(strKind, 0, sizeof(strKind));
        memset(strHost, 0, sizeof(strHost));
        pUri = ProtoHttpUrlParse(argv[2], strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure);
        ZPrintf("parsed url: kind=%s, host=%s, port=%d, secure=%d, uri=%s\n", strKind, strHost, iPort, iSecure, pUri);
        return(0);
    }

    // check for explicit destroy
    if ((argc == 2) && !ds_stricmp(argv[1], "destroy"))
    {
		if (pRef->pHttpManager != NULL)
		{
            HttpManagerDestroy(pRef->pHttpManager);
            pRef->pHttpManager = NULL;
		}
        return(iResult);
    }

    // check for request to set a certificate
    if ((argc == 3) && ((!strcmp(argv[1], "cert")) || (!strcmp(argv[1], "cer2"))))
    {
        const uint8_t *pCertData;
        int32_t iCertSize;

        // try and open file
        if ((pCertData = (const uint8_t *)ZFileLoad(argv[2], &iCertSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            if (!strcmp(argv[1], "cert"))
            {
                ProtoHttpSetCACert(pCertData, iCertSize);
            }
            else
            {
                ProtoHttpSetCACert2(pCertData, iCertSize);
            }
            ZMemFree((void *)pCertData);
        }
        else
        {
            ZPrintf("%s: unable to load certificate file '%s'\n", argv[0], argv[2]);
        }
        return(iResult);
    }
    else if (!strcmp(argv[1], "cclr"))
    {
        ZPrintf("%s: clearing dynamic certs\n", argv[0]);
        ProtoHttpClrCACerts();
        return(0);
    }
    else if (!strcmp(argv[1], "cver"))
    {
        int32_t iInvalid;
        ZPrintf("%s: verifying dynamic CA certs\n", argv[0]);
        if ((iInvalid = ProtoHttpValidateAllCA()) > 0)
        {
            ZPrintf("%s: could not verify %d CA certs\n", iInvalid);
        }
        return(0);
    }

    // check for create request
    if ((argc >= 2) && !strcmp(argv[1], "create"))
    {
        if (argc >= 3)
        {
            iBufSize = strtol(argv[2], NULL, 10);
        }
        if (argc >= 4)
        {
            iNumRefs = strtol(argv[3], NULL, 10);
        }
    }

    // create httpmanager module if necessary
    if (pRef->pHttpManager == NULL)
    {
        ZPrintf("%s: creating module with a %d refs and %dkbyte buffer\n", argv[0], iNumRefs, iBufSize);
        memset(pRef, 0, sizeof(*pRef));
        strnzcpy(pRef->strModuleName, argv[0], sizeof(pRef->strModuleName));
        pRef->pHttpManager = HttpManagerCreate(iBufSize, iNumRefs);
        if (pRef->pHttpManager != NULL)
        {
            HttpManagerCallback(pRef->pHttpManager, _HttpCustomHeaderCallback, _HttpRecvHeaderCallback);
            iResult = ZCallback(_CmdHttpMgrIdleCB, HTTP_RATE);
            pRef->bCallbackInstalled = TRUE;
            pRef->iDebugLevel = 2;
            HttpManagerControl(pRef->pHttpManager, -1, 'spam', pRef->iDebugLevel, 0, NULL);
            HttpManagerControl(pRef->pHttpManager, -1, 'time', 25*1000, 0, NULL);
        }
    }

    // check for create request -- if so, we're done
    if ((argc >= 2) && !strcmp(argv[1], "create"))
    {
        return(iResult);
    }
    else if ((argc > 2) && (argc < 6) && !strcmp(argv[1], "ctrl"))
    {
        int32_t iCmd, iValue = 0, iValue2 = 0;

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        if (argc > 3)
        {
            iValue = strtol(argv[3], NULL, 10);
        }
        
        if (argc > 4)
        {
            iValue2 = strtol(argv[4], NULL, 10);
        }

        // snoop 'spam' 
        if (iCmd == 'spam')
        {
            pRef->iDebugLevel = iValue;
        }

        return(HttpManagerControl(pRef->pHttpManager, /*iHandle*/ -1, iCmd, iValue, iValue2, NULL));
    }
    else if ((argc == 2) && !strcmp(argv[1], "recvall"))
    {
        pRef->bRecvAll = TRUE;
        return(0);
    }
    else if ((argc == 3) && !strcmp(argv[1], "stat"))
    {
        int32_t iCmd;
        char strBufferTemp[1024] = "";

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        iResult = HttpManagerStatus(pRef->pHttpManager, /*iHandle*/ -1, iCmd, strBufferTemp, sizeof(strBufferTemp));
        ZPrintf("%s: ProtoHttpStatus('%s') returned %d\n", argv[0], argv[2], iResult);
        if (strBufferTemp[0] != '\0')
        {
            ZPrintf("%s\n", strBufferTemp);
        }
        return(0);
    }
    // check for setting of base url
    else if ((argc == 3) && !strcmp(argv[1], "base"))
    {
        HttpManagerSetBaseUrl(pRef->pHttpManager, /*iHandle*/ -1, argv[2]);
        return(iResult);
    }
    // check for valid get/put request
	else if ((!ds_stricmp(argv[1], "get") || !ds_stricmp(argv[1], "head") || !ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") ||
              !ds_stricmp(argv[1], "puts") || !ds_stricmp(argv[1], "delete") || !ds_stricmp(argv[1], "options")) &&
            ((argc > 2) || (argc < 5)))
    {
        // allocate a new state record
        pState = _CmdHttpMgrAllocState(pRef);
        if (pState == NULL)
        {
            // if we could not allocate state, return error so upper layer can deal with it
            return(-1);
        }

        // fall-through to code below
    }
    else if (!ds_stricmp(argv[1], "mget"))
    {
        // do nothing, fall through
    }
    else
    {
        ZPrintf("   unrecognized or badly formatted command '%s'\n", argv[1]);
        _CmdHttpMgrUsage(argc, argv);
        return(iResult);
    }

    // locate url and filename
    pUrl = argv[2];
    if (argc > 3)
    {
        pFileName = argv[3];
    }

    if (!pUrl)
    {
        return(0);
    }

    // set append header
    /* NOTE: this doesn't actually work with httpmanager -- at this time there is no ProtoHttp ref, and HttpManager
       doesn't provide it's own 'apnd' buffer per ref, as that would be a lot of wasted memory.  for the moment,
       we assume that the more fully-featured send callback is used instead for this functionality */
#if 0
    if (pState->strCookie[0] != '\0')
    {
        char strBuffer[1024];
        snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n%s", pState->strCookie, HTTP_APNDHDR);
        HttpManagerControl(pRef->pHttpManager, pState->iHandle, 'apnd', 0, 0, strBuffer);
    }
    else
    {
        HttpManagerControl(pRef->pHttpManager, pState->iHandle, 'apnd', 0, 0, HTTP_APNDHDR);
    }
#endif

    // see if we're uploading or downloading
    if (!ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") || !ds_stricmp(argv[1], "puts"))
    {
        ZPrintf("%s: uploading %s to %s\n", argv[0], pFileName, pUrl);

        // try and open file
        if ((pFileData = ZFileLoad(pFileName, &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            if (ds_stricmp(argv[1], "puts"))
            {
                // initiate put/post transaction
                if ((pState->iSentSize = HttpManagerPost(pRef->pHttpManager, pState->iHandle, pUrl, pFileData, iFileSize, !ds_stricmp(argv[1], "put") ? PROTOHTTP_PUT : PROTOHTTP_POST)) < 0)
                {
                    ZPrintf("%s: error %d initiating send\n", pState->iSentSize);
                    return(0);
                }
                else
                {
                    ZPrintf("%s: uploading %d bytes\n", argv[0], pState->iSentSize);
                }
            }
            else
            {
                // initiate streaming put
                HttpManagerPost(pRef->pHttpManager, pState->iHandle, pUrl, NULL, PROTOHTTP_STREAM_BEGIN, PROTOHTTP_PUT);
                pState->bStreaming = TRUE;
            }

            // remember data
            pState->pInpData = pFileData;
            pState->iDataSize = iFileSize;

            // wait for reply
            pState->state = UPLOAD;
        }
        else
        {
            ZPrintf("%s: unable to load file '%s'\n", argv[0], pFileName);
        }
    }
    else if (!ds_stricmp(argv[1], "head") || !ds_stricmp(argv[1], "get"))
    {
        if (pFileName != NULL)
        {
            ZPrintf("%s: downloading %s to %s\n", argv[0], pUrl, pFileName);
            pState->iOutFile = ZFileOpen(pFileName, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY|ZFILE_OPENFLAG_CREATE);
        }
        else
        {
            ZPrintf("%s: downloading %s\n", argv[0], pUrl);
        }
        
        // initiate get transaction
        if ((iResult = HttpManagerGet(pRef->pHttpManager, pState->iHandle, pUrl, !ds_stricmp(argv[1], "head") ? PROTOHTTP_HEADONLY : PROTOHTTP_HEADBODY)) == 0)
        {
            pState->state = DNLOAD;
        }
    }
    else if (!ds_stricmp(argv[1], "mget") && (pRef->pMgetBuffer == NULL))
    {
        if ((pFileData = ZFileLoad(argv[2], &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            // set up for mget process
            pRef->pMgetBuffer = pRef->pMgetOffset = pFileData;
            pRef->uMgetStart = NetTick();
            strnzcpy(pRef->strMgetFilename, argv[2], sizeof(pRef->strMgetFilename));
        }
    }
    else if (!ds_stricmp(argv[1], "delete"))
    {
        if (HttpManagerRequest(pRef->pHttpManager, pState->iHandle, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE) == 0)
        {
            pState->state = DNLOAD;
        }
    }
    else if (!ds_stricmp(argv[1], "options"))
    {
        if (HttpManagerRequest(pRef->pHttpManager, pState->iHandle, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE) == 0)
        {
            pState->state = DNLOAD;
        }
    }
    else
    {
        ZPrintf("%s: unrecognized request %s\n", argv[0], argv[1]);
    }
    return(iResult);
}

