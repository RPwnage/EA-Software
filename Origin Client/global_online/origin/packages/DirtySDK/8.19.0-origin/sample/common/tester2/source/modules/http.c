/*H********************************************************************************/
/*!
    \File http.c

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
#include "protohttp.h"
#include "protossl.h"

#include "zlib.h"
#include "zmem.h"
#include "zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define HTTP_BUFSIZE    (4096)
#define HTTP_RATE       (1)

#define HTTP_XTRAHDR0   ""
#define HTTP_XTRAHDR1   "X-Agent: DirtySock HTTP Tester\r\n"    // test "normal" replacement (replaces Accept: header)
#define HTTP_XTRAHDR2   "User-Agent: DirtySock HTTP Tester\r\n" // test "extended" replacement (replaces User-Agent: and Accept: headers)

//$$ tmp -- special test header used for hard-coded multipart/form-data testing -- this should be removed at some point when we have real multipart/form-data support
#define HTTP_XTRAHDR3   "Content-Type: multipart/form-data; boundary=TeStInG\r\n" \
                        "User-Agent: DirtySock HTTP Tester\r\n" \
                        "Accept: */*\r\n"

#define HTTP_APNDHDR    HTTP_XTRAHDR0

/*** Function Prototypes **********************************************************/

static int32_t _CmdHttpIdleCB(ZContext *argz, int32_t argc, char *argv[]);

/*** Type Definitions *************************************************************/

typedef struct HttpRefT      // module state storage
{
    ProtoHttpRefT *http;
    int32_t show;
    int32_t count;
    int32_t sttime;
    const char *pData;
    int32_t iDataSize;
    int32_t iSentSize;
    ZFileT iOutFile;
    int32_t iDebugLevel;
    uint8_t bCallbackInstalled;
    uint8_t bStreaming;
    uint8_t bUseWriteCb;
    char strModuleName[32];
    char strCookie[2048];
    char strApndHdr[2048];

    // module state
    enum
    {
        IDLE, DNLOAD, UPLOAD
    } state;
} HttpRefT;

/*** Variables ********************************************************************/

static HttpRefT _Http_Ref = { NULL, 0, 0, 0, NULL, 0, 0, ZFILE_INVALID, 0, 0, 0, 0, "", "", "", IDLE  };

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _HttpCustomHeaderCallback

    \Description
        ProtoHttp custom header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - header to be sent
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpRefT)

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
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpRecvHeaderCallback

    \Description
        ProtoHttp recv header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - received header
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpRefT)

    \Output
        int32_t         - zero

    \Version 02/24/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpRecvHeaderCallback(ProtoHttpRefT *pProtoHttp, const char *pHeader, uint32_t uHeaderSize, void *pUserRef)
{
    HttpRefT *pHttp = (HttpRefT *)pUserRef;
    char strBuffer[2048], strName[128];
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

    // if this is a redirection update append header with any new cookies
    if ((PROTOHTTP_GetResponseClass(ProtoHttpStatus(pProtoHttp, 'code', NULL, 0)) == PROTOHTTP_RESPONSE_REDIRECTION) && (pHttp->strCookie[0] != '\0'))
    {
        // format append header
        snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n%s", pHttp->strCookie, HTTP_APNDHDR);
        strnzcat(strBuffer, pHttp->strApndHdr, sizeof(strBuffer));
        ProtoHttpControl(pProtoHttp, 'apnd', 0, 0, strBuffer);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpDownloadProcessData

    \Description
        Process data received in a download operation

    \Input *pRef    - module state
    \Input *pStrBuf - data
    \Input iLen     - data length or PROTOHTTP_RECV*

    \Version 05/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpDownloadProcessData(HttpRefT *pRef, const char *pStrBuf, int32_t iLen)
{
    if (iLen > 0)
    {
        pRef->count += iLen;
        if (pRef->iOutFile != ZFILE_INVALID)
        {
            ZFileWrite(pRef->iOutFile, (void *)pStrBuf, iLen);
        }
    }

    // see if we should show progress
    if ((pRef->count/1024) != (pRef->show/1024))
    {
        pRef->show = pRef->count;
        ZPrintf("%s: downloaded %d bytes\n", pRef->strModuleName, pRef->count);
    }

    // see if we are done
    if ((iLen < 0) && (iLen != PROTOHTTP_RECVWAIT))
    {
        // completed successfully?
        if ((iLen == PROTOHTTP_RECVDONE) || (iLen == PROTOHTTP_RECVHEAD))
        {
            int32_t iDlTime = NetTick() - pRef->sttime;
            char strRespHdr[1024];

            ZPrintf("%s: download complete (%d bytes)\n", pRef->strModuleName, pRef->count);
            ZPrintf("%s download time: %d bytes in %.2f seconds (%.3f k/sec)\n", pRef->strModuleName, pRef->count,
                (float)iDlTime/1000.0f,
                ((float)pRef->count * 1000.0f) / ((float)iDlTime * 1024.0f));

            // display response header
            if (ProtoHttpStatus(pRef->http, 'htxt', strRespHdr, sizeof(strRespHdr)) >= 0)
            {
                ZPrintf("%s response header:\n%s\n", pRef->strModuleName, strRespHdr);
            }

            // display a couple of parsed fields
            if (ProtoHttpStatus(pRef->http, 'head', NULL, 0) > 0)
            {
                time_t tLastMod = ProtoHttpStatus(pRef->http, 'date', NULL, 0);
                const char *pTime;

                if (tLastMod != 0)
                {
                    if ((pTime = ctime(&tLastMod)) != NULL)
                    {
                        ZPrintf("%s: Last-Modified: %s", pRef->strModuleName, pTime);
                    }
                }
                ZPrintf("%s: Content-Length: %d\n", pRef->strModuleName, ProtoHttpStatus(pRef->http, 'body', NULL, 0));
            }
        }
        else // failure
        {
            int32_t iSslFail = ProtoHttpStatus(pRef->http, 'essl', NULL, 0);
            ZPrintf("%s: download failed (err=%d, sslerr=%d)\n", pRef->strModuleName, iLen, iSslFail);
            if ((iSslFail == PROTOSSL_ERROR_CERT_INVALID) || (iSslFail == PROTOSSL_ERROR_CERT_HOST) || 
                (iSslFail == PROTOSSL_ERROR_CERT_NOTRUST) || (iSslFail == PROTOSSL_ERROR_CERT_REQUEST))
            {
                ProtoSSLCertInfoT CertInfo;
                if (ProtoHttpStatus(pRef->http, 'cert', &CertInfo, sizeof(CertInfo)) == 0)
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
        if (pRef->iOutFile != ZFILE_INVALID)
        {
            ZFileClose(pRef->iOutFile);
            pRef->iOutFile = ZFILE_INVALID;
        }

        // set to idle state
        pRef->state = IDLE;
    }
}

/*F********************************************************************************/
/*!
    \Function _HttpWriteCb

    \Description
        Implementation of ProtoHttp write callback

    \Input *pState      - http module state
    \Input *pWriteInfo  - callback info 
    \Input *pData       - transaction data pointer
    \Input iDataSize    - size of data
    \Input *pUserData   - user callback data

    \Output
        int32_t         - zero

    \Version 05/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpWriteCb(ProtoHttpRefT *pState, const ProtoHttpWriteCbInfoT *pWriteInfo, const char *pData, int32_t iDataSize, void *pUserData)
{
    HttpRefT *pRef = (HttpRefT *)pUserData;
    //NetPrintf(("http: writecb (%d,%d) %d bytes\n", pWriteInfo->eRequestType, pWriteInfo->eRequestResponse, iDataSize));
    _HttpDownloadProcessData(pRef, pData, iDataSize);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list

    \Output
        None.

    \Version 02/18/2008 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpUsage(int argc, char *argv[])
{
    if (argc == 2)
    {
        ZPrintf("   execute http get or put operations\n");
        ZPrintf("   usage: %s [cclr|cert|cer2|cver|create|ctrl|debug|destroy|get|head|put|post|puts|delete|options|parse|stat]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "cclr") || !strcmp(argv[2], "cert") || !strcmp(argv[2], "cer2") || !strcmp(argv[2], "cver"))
        {
            ZPrintf("   usage: %s cert|cer2 <certfile> - load certificate file\n", argv[0]);
            ZPrintf("          %s cclr - clear dynamic CA certs\n");
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
        else if (!strcmp(argv[2], "get") || !strcmp(argv[2], "head") || !strcmp(argv[2], "options") || !strcmp(argv[2], "delete"))
        {
            ZPrintf("   usage: %s %s <options> [url] <file>\n", argv[0], argv[2]);
        }
        else if (!strcmp(argv[2], "put") || !strcmp(argv[2], "puts") || !strcmp(argv[2], "post"))
        {
            ZPrintf("   usage: %s %s <options> [url] [file]\n", argv[0], argv[2]);
        }
        else if (!strcmp(argv[2], "debug"))
        {
            ZPrintf("   usage: %s debug [debuglevel]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "parse"))
        {
            ZPrintf("   usage: %s parse [url]\n", argv[0]);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpCheckComplete

    \Description
        See if the HTTP transaction is complete.

    \Input *pRef        - pointer to http ref
    \Input *pCmdName    - module name

    \Output
        int32_t         - completion status from ProtoHttpStatus() 'done' selector

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpCheckComplete(HttpRefT *ref, const char *pCmdName)
{
    ProtoHttpResponseE eResponse;
    int32_t iResult;

    // check for completion
    if ((iResult = ProtoHttpStatus(ref->http, 'done', NULL, 0)) == 0)
    {
        return(0);
    }

    // get completion result
    eResponse = (ProtoHttpResponseE)ProtoHttpStatus(ref->http, 'code', NULL, 0);
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
    \Function _CmdHttpDownloadProcess

    \Description
        Process a download transaction.

    \Input *pRef    - module state

    \Output
        None.

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpDownloadProcess(HttpRefT *pRef)
{
    char strBuf[1024];
    int32_t iLen;

    // give it time
    ProtoHttpUpdate(pRef->http);

    // if we're not doing the write callback thing, poll for data
    if (!pRef->bUseWriteCb)
    {
    while ((iLen = ProtoHttpRecv(pRef->http, strBuf, 1, sizeof(strBuf))) > 0)
    {
            _HttpDownloadProcessData(pRef, strBuf, iLen);
                }
            }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpUploadProcess

    \Description
        Process an upload transaction.

    \Input *pRef    - module state

    \Output
        None.

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpUploadProcess(HttpRefT *pRef)
{
    char strResponse[1024];
    int32_t iCode, iResult;

    // write data?
    if ((pRef->pData != NULL) && (pRef->iSentSize < pRef->iDataSize))
    {
        iResult = ProtoHttpSend(pRef->http, pRef->pData + pRef->iSentSize, pRef->iDataSize - pRef->iSentSize);
        if (iResult > 0)
        {
            pRef->iSentSize += iResult;
            ZPrintf("%s: sent %d bytes (%d total)\n", pRef->strModuleName, iResult, pRef->iSentSize);
        }
    }
    // check for upload completion
    if ((pRef->pData != NULL) && (pRef->iSentSize == pRef->iDataSize))
    {
        // if streaming upload, signal we are done
        if (pRef->bStreaming == TRUE)
        {
            ProtoHttpSend(pRef->http, NULL, PROTOHTTP_STREAM_END);
            pRef->bStreaming = FALSE;
        }

        // done uploading
        ZPrintf("%s: uploaded %d bytes\n", pRef->strModuleName, pRef->iSentSize);
        ZMemFree((void *)pRef->pData);
        pRef->pData = NULL;
    }

    // give it time
    ProtoHttpUpdate(pRef->http);

    // see if we've received an HTTP 1xx (INFORMATIONAL) header
    iCode = ProtoHttpStatus(pRef->http, 'info', strResponse, sizeof(strResponse));
    if (PROTOHTTP_GetResponseClass(iCode) == PROTOHTTP_RESPONSE_INFORMATIONAL)
    {
        // got response header, so print it
        NetPrintf(("http: received %d response header\n----------------------------------\n%s----------------------------------\n", iCode, strResponse));
    }

    // done?
    if ((iResult = _CmdHttpCheckComplete(pRef, pRef->strModuleName)) != 0)
    {
        if (iResult > 0)
        {
            int32_t ultime = NetTick() - pRef->sttime;
            ZPrintf("%s: upload complete (%d bytes)\n", pRef->strModuleName, pRef->iSentSize);
            ZPrintf("upload time: %d bytes in %.2f seconds (%.3f k/sec)\n", pRef->iSentSize, (float)ultime/1000.0f,
                ((float)pRef->iSentSize * 1000.0f) / ((float)ultime * 1024.0f));

            memset(strResponse, 0, sizeof(strResponse));
            iResult = ProtoHttpRecv(pRef->http, strResponse, 1, sizeof(strResponse));
            if (iResult > 0)
            {
                ZPrintf("http result:\n%s", strResponse);
            }
        }
        pRef->state = IDLE;
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdIdleCB

    \Description
        Callback to process while idle

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 09/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpIdleCB(ZContext *argz, int32_t argc, char *argv[])
{
    HttpRefT *pRef = &_Http_Ref;

    // shut down?
    if ((argc == 0) || (pRef->http == NULL))
    {
        if (pRef->http != NULL)
        {
            ProtoHttpDestroy(pRef->http);
            pRef->http = NULL;
        }
        return(0);
    }

    // process a download
    if (pRef->state == DNLOAD)
    {
        _CmdHttpDownloadProcess(pRef);
    }

    // process an upload
    if (pRef->state == UPLOAD)
    {
        _CmdHttpUploadProcess(pRef);
    }

    // keep on idling
    return(ZCallback(&_CmdHttpIdleCB, HTTP_RATE));
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttp

    \Description
        Initiate an HTTP transaction.

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 10/28/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdHttp(ZContext *argz, int32_t argc, char *argv[])
{
    const char *pFileName = NULL, *pUrl;
    HttpRefT *pRef = &_Http_Ref;
    int32_t iResult = 0, iBufSize = HTTP_BUFSIZE;
    char strBuffer[4096];
    int32_t iStartArg = 2; // first arg past get/put/delete/whatever

    if (argc < 2)
    {
        return(0);
    }

    // check for help
    if ((argc >= 2) && !strcmp(argv[1], "help"))
    {
        _CmdHttpUsage(argc, argv);
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
        if (pRef->http != NULL)
        {
            ProtoHttpDestroy(pRef->http);
            pRef->http = NULL;
        }
        return(iResult);
    }

    // check for create request
    if ((argc == 3) && !strcmp(argv[1], "create"))
    {
        iBufSize = strtol(argv[2], NULL, 10);
    }

    // check for request to set a certificate
    if ((argc == 3) && ((!strcmp(argv[1], "cert")) || (!strcmp(argv[1], "cer2"))))
    {
        const uint8_t *pFileData;
        int32_t iFileSize;

        // try and open file
        if ((pFileData = (const uint8_t *)ZFileLoad(argv[2], &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            if (!strcmp(argv[1], "cert"))
            {
                ProtoHttpSetCACert(pFileData, iFileSize);
            }
            else
            {
                ProtoHttpSetCACert2(pFileData, iFileSize);
            }
            ZMemFree((void *)pFileData);
        }
        else
        {
            ZPrintf("%s: unable to load certificate file '%s'\n", argv[0], argv[2]);
        }
        return(iResult);
    }
    else if (!strcmp(argv[1], "cclr"))
    {
        ZPrintf("%s: clearing dynamic CA certs\n", argv[0]);
        ProtoHttpClrCACerts();
        return(0);
    }
    else if (!strcmp(argv[1], "cver"))
    {
        int32_t iInvalid;
        ZPrintf("%s: verifying dynamic CA certs\n", argv[0]);
        if ((iInvalid = ProtoHttpValidateAllCA()) > 0)
        {
            ZPrintf("%s: could not verify %d CA certs\n", argv[0], iInvalid);
        }
        return(0);
    }

    // create protohttp module if necessary
    if (pRef->http == NULL)
    {
        ZPrintf("%s: creating module with a %dkbyte buffer\n", argv[0], iBufSize);
        memset(pRef, 0, sizeof(*pRef));
        strnzcpy(pRef->strModuleName, argv[0], sizeof(pRef->strModuleName));
        pRef->http = ProtoHttpCreate(iBufSize);
        if (pRef->http != NULL)
        {
            ProtoHttpCallback(pRef->http, _HttpCustomHeaderCallback, _HttpRecvHeaderCallback, pRef);
            iResult = ZCallback(_CmdHttpIdleCB, HTTP_RATE);
            pRef->bCallbackInstalled = TRUE;
            pRef->iDebugLevel = 1;
        }
        pRef->iOutFile = ZFILE_INVALID;
    }

    // check for create request -- if so, we're done
    if ((argc <= 3) && !strcmp(argv[1], "create"))
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

        return(ProtoHttpControl(pRef->http, iCmd, iValue, iValue2, NULL));
    }
    else if ((argc == 3) && !strcmp(argv[1], "stat"))
    {
        int32_t iCmd;

        iCmd  = argv[2][0] << 24;
        iCmd |= argv[2][1] << 16;
        iCmd |= argv[2][2] << 8;
        iCmd |= argv[2][3];

        iResult = ProtoHttpStatus(pRef->http, iCmd, strBuffer, sizeof(strBuffer));
        ZPrintf("http: ProtoHttpStatus('%s') returned %d\n", argv[2], iResult);
        if (strBuffer[0] != '\0')
        {
            ZPrintf("%s\n", strBuffer);
        }
        return(0);
    }
    // check for setting of base url
    else if ((argc == 3) && !strcmp(argv[1], "base"))
    {
        ProtoHttpSetBaseUrl(pRef->http, argv[2]);
        return(iResult);
    }
    // check for debug setting
    else if (!strcmp(argv[1], "debug") && (argc == 3))
    {
        int32_t iDebugLevel = strtol(argv[2], NULL, 10);
        pRef->iDebugLevel = iDebugLevel;
        return(iResult);
    }
    // check for valid get/put request
    else if ((!ds_stricmp(argv[1], "get") || !ds_stricmp(argv[1], "head") || !ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") ||
              !ds_stricmp(argv[1], "puts") || !ds_stricmp(argv[1], "delete") || !ds_stricmp(argv[1], "options")) &&
            ((argc > 2) || (argc < 5)))
    {
        int32_t iArg;

        // clear any previous stats
        pRef->count = 0;
        pRef->show = 0;
        pRef->iDataSize = 0;
        pRef->iSentSize = 0;
        pRef->bStreaming = FALSE;

        // init start timer
        pRef->sttime = NetTick();

        // set up append header
        strnzcpy(pRef->strApndHdr, HTTP_APNDHDR, sizeof(pRef->strApndHdr));

        // check for args
        for (iArg = 2; (iArg < argc) && (argv[iArg][0] == '-'); iArg += 1)
        {
            if (!ds_strnicmp(argv[iArg], "-header=", 8))
            {
                strnzcat(pRef->strApndHdr, argv[iArg]+8, sizeof(pRef->strApndHdr));
                strnzcat(pRef->strApndHdr, "\r\n", sizeof(pRef->strApndHdr));
            }
            if (!ds_strnicmp(argv[iArg], "-writecb", 8))
            {
                pRef->bUseWriteCb = TRUE;
            }
            // skip any option arguments to find url and (optionally) filename
            iStartArg += 1;
        }

        // fall-through to code below
    }
    else
    {
        ZPrintf("   unrecognized or badly formatted command '%s'\n", argv[1]);
        _CmdHttpUsage(argc, argv);
        return(iResult);
    }

    // locate url and filename
    pUrl = argv[iStartArg];
    if (argc > iStartArg)
    {
        pFileName = argv[iStartArg+1];
    }

    if (!pUrl)
    {
        return(0);
    }

    // set timeout value
    ProtoHttpControl(pRef->http, 'time', 25*1000, 0, NULL);
    // set append header
    strBuffer[0] = '\0';
    if (pRef->strCookie[0] != '\0')
    {
        snzprintf(strBuffer, sizeof(strBuffer), "Cookie: %s\r\n", pRef->strCookie);
    }
    strnzcat(strBuffer, pRef->strApndHdr, sizeof(strBuffer));
    ProtoHttpControl(pRef->http, 'apnd', 0, 0, strBuffer);
    // set debug level
    ProtoHttpControl(pRef->http, 'spam', pRef->iDebugLevel, 0, NULL);

    // see if we're uploading or downloading
    if (!ds_stricmp(argv[1], "put") || !ds_stricmp(argv[1], "post") || !ds_stricmp(argv[1], "puts"))
    {
        const char *pFileData;
        int32_t iFileSize;

        ZPrintf("%s: uploading %s to %s\n", argv[0], pFileName, pUrl);

        // try and open file
        if ((pFileData = ZFileLoad(pFileName, &iFileSize, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) != NULL)
        {
            if (ds_stricmp(argv[1], "puts"))
            {
                // initiate put/post transaction
                ZPrintf("%s: uploading %d bytes\n", argv[0], iFileSize);
                if ((pRef->iSentSize = ProtoHttpPost(pRef->http, pUrl, pFileData, iFileSize, !ds_stricmp(argv[1], "put") ? PROTOHTTP_PUT : PROTOHTTP_POST)) < 0)
                {
                    ZPrintf("%s: error %d initiating send\n", pRef->iSentSize);
                    return(0);
                }
                else if (pRef->iSentSize > 0)
                {
                    ZPrintf("%s: uploaded %d bytes\n", argv[0], pRef->iSentSize);
                }
            }
            else
            {
                // initiate streaming put
                ProtoHttpPost(pRef->http, pUrl, NULL, PROTOHTTP_STREAM_BEGIN, PROTOHTTP_PUT);
                pRef->bStreaming = TRUE;
            }

            // remember data
            pRef->pData = pFileData;
            pRef->iDataSize = iFileSize;

            // wait for reply
            pRef->state = UPLOAD;
        }
        else
        {
            ZPrintf("%s: unable to load file '%s'\n", argv[0], pFileName);
        }
    }
    else
    {
        ProtoHttpRequestTypeE eRequestType;

        if (pFileName != NULL)
        {
            ZPrintf("%s: downloading %s to %s\n", argv[0], pUrl, pFileName);
            pRef->iOutFile = ZFileOpen(pFileName, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY|ZFILE_OPENFLAG_CREATE);
        }
        else
        {
            ZPrintf("%s: downloading %s\n", argv[0], pUrl);
        }

        // map to protohttp request type
        if (!ds_stricmp(argv[1], "head"))
        {
            eRequestType = PROTOHTTP_REQUESTTYPE_HEAD;
        }
        else if (!ds_stricmp(argv[1], "get"))
            {
            eRequestType = PROTOHTTP_REQUESTTYPE_GET;
        }
        else if (!ds_stricmp(argv[1], "delete"))
        {
            eRequestType = PROTOHTTP_REQUESTTYPE_DELETE;
            }
        else if (!ds_stricmp(argv[1], "options"))
        {
            eRequestType = PROTOHTTP_REQUESTTYPE_OPTIONS;
        }
        else
            {
            ZPrintf("%s: unrecognized request %s\n", argv[0], argv[1]);
            return(-1);
            }

        if (pRef->bUseWriteCb)
        {
            iResult = ProtoHttpRequestCb(pRef->http, pUrl, NULL, 0, eRequestType, _HttpWriteCb, pRef);
        }
        else
        {
            iResult = ProtoHttpRequest(pRef->http, pUrl, NULL, 0, eRequestType);
        }

        if (iResult == 0)
        {
            pRef->state = DNLOAD;
        }
    }

    return(iResult);
}

