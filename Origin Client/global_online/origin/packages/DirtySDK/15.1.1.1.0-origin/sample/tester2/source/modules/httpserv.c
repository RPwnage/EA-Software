/*H********************************************************************************/
/*!
    \File httpserv.c

    \Description
        Implements basic http server using ProtoHttpServ

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 09/11/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __linux__
#include <sys/time.h> // gettimeofday
#endif

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h" // NetConnSleep
#include "DirtySDK/proto/protohttpserv.h"

#include "zlib.h"
#include "zmem.h"
#include "zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define HTTPSERV_RATE           (1)
#define HTTPSERV_LISTENPORT     (9000)

/*** Function Prototypes **********************************************************/

/*** Type Definitions *************************************************************/

typedef struct HttpServT
{
    ProtoHttpServRefT *pHttpServ;

    char *pServerCert;
    int32_t iServerCertLen;
    char *pServerKey;
    int32_t iServerKeyLen;
    int32_t iChunkLen;

    char strServerName[128];
    char strFileDir[512];
} HttpServT;

/*** Variables ********************************************************************/

static HttpServT _HttpServ_Ref;
static uint8_t   _HttpServ_bInitialized = FALSE;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _HttpServLoadCertificate

    \Description
        Load pem certificate file and trim begin/end text.

    \Input *pFilename       - name of certificate file to open
    \Input *pCertSize       - [out] storage for size of certificate
    \Input *pStrBegin       - beginning text to trim
    \Input *pStrEnd         - end text to trim

    \Output
        const char *        - base64-encoded certificate data

    \Version 10/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static char *_HttpServLoadCertificate(const char *pFilename, int32_t *pCertSize, const char *pStrBegin, const char *pStrEnd)
{
    char *pCertBuf, *pDataStart, *pDataEnd;
    int32_t iFileSize;

    // load certificate file
    if ((pCertBuf = (char *)ZFileLoad(pFilename, &iFileSize, ZFILE_OPENFLAG_RDONLY)) == NULL)
    {
        return(NULL);
    }

    // find start of certificate data
    if ((pDataStart = ds_stristr(pCertBuf, pStrBegin)) == NULL)
    {
        NetPrintf(("httpserv: could not find pem begin signature\n"));
        return(NULL);
    }
    // skip certificate data
    for (pDataStart += strlen(pStrBegin); *pDataStart == '\r' || *pDataStart == '\n' || *pDataStart == ' '; pDataStart += 1)
        ;
    // remove begin signature from buffer
    memmove(pCertBuf, pDataStart, strlen(pDataStart)+1);

    // find end of certificate data marker
    if ((pDataEnd = ds_stristr(pCertBuf, pStrEnd)) == NULL)
    {
        NetPrintf(("httpserv: could not find pem end signature\n"));
        return(NULL);
    }
    // find end of certficate and remove end signature
    for (pDataEnd -= 1; (*pDataEnd == '\r') || (*pDataEnd == '\n') || (*pDataEnd == ' '); pDataEnd -= 1)
        ;
    pDataEnd[1] = '\0';
    // calculate length
    *pCertSize = (int32_t)(pDataEnd - pCertBuf + 1);
    // return to caller
    return(pCertBuf);
}

/*F********************************************************************************/
/*!
    \Function _HttpServProcessGet

    \Description
        Process GET/HEAD request

    \Input *pServerState   - module state
    \Input *pUrl        - request url
    \Input *pHeader     - request header
    \Input *pRequestInfo- request info

    \Output
        int32_t         - response code

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServProcessGet(HttpServT *pServerState, const char *pUrl, const char *pHeader, ProtoHttpServRequestInfoT *pRequestInfo)
{
    char strFilePath[4096];
    ZFileStatT FileStat;
    int32_t iFileLen, iResult;
    ZFileT iFileId;

    // if empty url, substitute default
    if (*(pUrl+1) == '\0')
    {
        pUrl = "/index.html";
    }

    // create filepath
    ds_snzprintf(strFilePath, sizeof(strFilePath), "%s/%s", pServerState->strFileDir, pUrl + 1);

    // stat the file
    if ((iResult = ZFileStat(strFilePath, &FileStat)) != ZFILE_ERROR_NONE)
    {
        // no file
        ZPrintf("httpserv: could not stat file '%s'\n", strFilePath);
        pRequestInfo->iResponseCode = PROTOHTTP_RESPONSE_NOTFOUND;
        return(-1);
    }

    // see if url refers to a file
    if ((iFileId = ZFileOpen(strFilePath, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) == ZFILE_INVALID)
    {
        ZPrintf("httpserv: could not open file '%s' for reading\n", strFilePath);
        pRequestInfo->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-2);
    }
    // get the file size
    if ((iFileLen = (int32_t)ZFileSize(iFileId)) < 0)
    {
        ZPrintf("httpserv: error %d getting file size\n", iFileLen);
        pRequestInfo->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-3);
    }

    // set request info
    pRequestInfo->iDataSize = iFileLen;
    pRequestInfo->iChunkLength = pServerState->iChunkLen;
    pRequestInfo->uModTime = FileStat.uTimeModify;
    pRequestInfo->pRequestData = (void *)iFileId;
    pRequestInfo->iResponseCode = PROTOHTTP_RESPONSE_OK;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServProcessPut

    \Description
        Process PUT/POST request

    \Input *pServerState   - module state
    \Input *pUrl        - request url
    \Input *pHeader     - request header
    \Input *pRequestInfo- request info

    \Output
        int32_t         - response code

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServProcessPut(HttpServT *pServerState, const char *pUrl, const char *pHeader, ProtoHttpServRequestInfoT *pRequestInfo)
{
    char strFilePath[1024];
    ZFileT iFileId;

    // create filepath
    ds_snzprintf(strFilePath, sizeof(strFilePath), "%s\\%s", pServerState->strFileDir, pUrl + 1);

    // try to open file
    if ((iFileId = ZFileOpen(strFilePath, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY)) == ZFILE_INVALID)
    {
        ZPrintf("httpserv: could not open file '%s' for writing\n", strFilePath);
        pRequestInfo->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-2);
    }

    // we've processed the request
    pRequestInfo->pRequestData = (void *)(uintptr_t)iFileId;
    pRequestInfo->iResponseCode = PROTOHTTP_RESPONSE_CREATED;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServRequestCb

    \Description
        ProtoHttpServ request callback handler

    \Input eRequestType - request type
    \Input *pUrl        - request url
    \Input *pHeader     - request header
    \Input *pRequestInfo- request info
    \Input *pUserData   - callback user data

    \Output
        int32_t         - response code

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServRequestCb(ProtoHttpRequestTypeE eRequestType, const char *pUrl, const char *pHeader, ProtoHttpServRequestInfoT *pRequestInfo, void *pUserData)
{
    HttpServT *pServerState = (HttpServT *)pUserData;
    int32_t iResult = -1;

    // init default response values
    pRequestInfo->iResponseCode = PROTOHTTP_RESPONSE_NOTIMPLEMENTED;
    pRequestInfo->pRequestData = (void *)(uintptr_t)ZFILE_INVALID;
    pRequestInfo->uModTime = 0;

    // handle the request
    if ((eRequestType == PROTOHTTP_REQUESTTYPE_GET) || (eRequestType == PROTOHTTP_REQUESTTYPE_HEAD))
    {
        pRequestInfo->iDataSize = 0;
        iResult = _HttpServProcessGet(pServerState, pUrl, pHeader, pRequestInfo);
    }
    if ((eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        iResult = _HttpServProcessPut(pServerState, pUrl, pHeader, pRequestInfo);
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServWriteCb

    \Description
        ProtoHttpServ write callback handler

    \Input *pServerState   - module state
    \Input *pBuffer     - data to write
    \Input *pRequestData- request-specific user data
    \Input *pUserData   - user data

    \Output
        int32_t         - negative=failure, else bytes written

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServWriteCb(const char *pBuffer, int32_t iBufSize, void *pRequestData, void *pUserData)
{
    //HttpServT *pServerState = (HttpServT *)pUserData;
    ZFileT iFileId = (ZFileT)(uintptr_t)pRequestData;
    int32_t iResult = 0;

    if (iFileId == ZFILE_INVALID)
    {
        return(0);
    }

    // check for upload completion
    if (pBuffer == NULL)
    {
        ZFileClose(iFileId);
    }
    else if ((iResult = ZFileWrite(iFileId, (void *)pBuffer, iBufSize)) < 0)
    {
        NetPrintf(("httpserv: error %d writing to file\n", iResult));
    }
    // return result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServReadCb

    \Description
        ProtoHttpServ read callback handler

    \Input *pServerState   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServReadCb(char *pBuffer, int32_t iBufSize, void *pRequestData, void *pUserData)
{
    //HttpServT *pServerState = (HttpServT *)pUserData;
    ZFileT iFileId = (ZFileT)(uintptr_t)pRequestData;
    int32_t iResult = 0;

    if ((pRequestData == NULL) || (iFileId == ZFILE_INVALID))
    {
        return(0);
    }

    // check for download completion
    if (pBuffer == NULL)
    {
        ZFileClose(iFileId);
    }
    else if ((iResult = ZFileRead(iFileId, pBuffer, iBufSize)) < 0)
    {
        ZPrintf("httpserv: error %d reading from file\n", iResult);
    }
    // return result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServLogCb

    \Description
        ProtoHttpServ logging function

    \Input *pText       - text to print
    \Input *pUserData   - user data (module state)

    \Output
        int32_t         - zero

    \Version 12/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServLogCb(const char *pText, void *pUserData)
{
    ZPrintf("%s", pText);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpServIdleCB

    \Description
        Callback to process while idle

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 09/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpServIdleCB(ZContext *argz, int32_t argc, char *argv[])
{
    HttpServT *pRef = &_HttpServ_Ref;

    // shut down?
    if (argc == 0)
    {
        return(0);
    }

    // update protohttpserv module
    ProtoHttpServUpdate(pRef->pHttpServ);

    // keep on idling
    return(ZCallback(&_CmdHttpServIdleCB, HTTPSERV_RATE));
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpServUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list

    \Version 09/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpServUsage(int argc, char *argv[])
{
    if (argc <= 2)
    {
        ZPrintf("httpserv:    basic http server\n");
        ZPrintf("httpserv:    usage: %s [chunked|filedir|listen|setcert|setkey|ciph|vers|vmin]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "ccrt"))
        {
            ZPrintf("httpserv:    usage: %s ccrt [level]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "chunked"))
        {
            ZPrintf("httpserv:    usage: %s chunked [chunklen]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "filedir"))
        {
            ZPrintf("httpserv:    usage: %s filedir <directory>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "listen"))
        {
            ZPrintf("httpserv:    usage: %s listen [listenport] <secure>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "setcert"))
        {
            ZPrintf("httpserv:    usage: %s setcert [certfile]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "setkey"))
        {
            ZPrintf("httpserv:    usage: %s setkey [certkey]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "ciph"))
        {
            ZPrintf("httpserv:    usage: %s ciph [ciphers]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "vers"))
        {
            ZPrintf("httpserv:    usage: %s vers [version]\n", argv[0]);
        }
        else if (!strcmp(argv[2], "vmin"))
        {
            ZPrintf("httpserv:    usage: %s vmin [version]\n", argv[0]);
        }
    }
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttpServ

    \Description
        Simple HTTP server

    \Input *argz    - context
    \Input argc     - command count
    \Input *argv[]  - command arguments

    \Output int32_t -

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdHttpServ(ZContext *argz, int32_t argc, char *argv[])
{
    HttpServT *pServerState = &_HttpServ_Ref;
    int32_t iResult = 0;

    if ((argc < 2) || !ds_stricmp(argv[1], "help"))
    {
        _CmdHttpServUsage(argc, argv);
        return(iResult);
    }

    // if not initialized yet, do so now
    if (_HttpServ_bInitialized == FALSE)
    {
        memset(pServerState, 0, sizeof(*pServerState));
        ds_strnzcpy(pServerState->strServerName, "HttpServ", sizeof(pServerState->strServerName));
        ds_strnzcpy(pServerState->strFileDir, "c:\\temp\\httpserv", sizeof(pServerState->strFileDir));
        _HttpServ_bInitialized = TRUE;
    }

    // check for filedir set
    if ((argc == 3) && !ds_stricmp(argv[1], "filedir"))
    {
        ds_strnzcpy(pServerState->strFileDir, argv[2], sizeof(pServerState->strFileDir));
        return(iResult);
    }

    // check for listen
    if ((argc >= 2) && (argc < 5) && !ds_stricmp(argv[1], "listen"))
    {
        int32_t iPort = HTTPSERV_LISTENPORT;
        int32_t iSecure = 0; // insecure by default

        if ((argc > 3) && !ds_stricmp(argv[3], "secure"))
        {
            iSecure = 1;
        }
        if (argc > 2)
        {
            iPort = (int32_t)strtol(argv[2], NULL, 10);
            if ((iPort < 1) || (iPort > 65535))
            {
                ZPrintf("httpserv: invalid port %d specified in listen request\n", iPort);
                return(-1);
            }
        }

        // destroy previous httpserv ref, if any
        if (pServerState->pHttpServ != NULL)
        {
            ProtoHttpServDestroy(pServerState->pHttpServ);
        }

        // create new httpserv ref
        if ((pServerState->pHttpServ = ProtoHttpServCreate(iPort, iSecure, pServerState->strServerName)) == NULL)
        {
            ZPrintf("httpserv: could not create httpserv state on port %d\n", iPort);
            return(-2);
        }

        // set up httpserv callbacks
        ProtoHttpServCallback(pServerState->pHttpServ, _HttpServRequestCb, _HttpServReadCb, _HttpServWriteCb, _HttpServLogCb, pServerState);

        // install recurring update
        iResult = ZCallback(_CmdHttpServIdleCB, HTTPSERV_RATE);
    }

    // the following functions require http serv state
    if (pServerState->pHttpServ == NULL)
    {
        ZPrintf("%s: '%s' requires httpserv creation (use 'listen' command)\n", argv[0], argv[1]);
        return(iResult);
    }
    
    // check for client cert level specification
    if ((argc == 3) && !ds_stricmp(argv[1], "ccrt"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'ccrt', (int32_t)strtol(argv[2], NULL, 10), 0, NULL));
    }

    // check for chunked transfer (download) enable
    if ((argc >= 2) && !ds_stricmp(argv[1], "chunked"))
    {
        int32_t iChunkLen = 4096;
        if (argc > 2)
        {
            iChunkLen = (int32_t)strtol(argv[2], NULL, 10);
        }
        ZPrintf("httpserv: enabling chunked transfers and setting chunk size to %d\n", iChunkLen);
        pServerState->iChunkLen = iChunkLen;
        return(iResult);
    }

    // check for cipher set
    if ((argc == 3) && !ds_stricmp(argv[1], "ciph"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'ciph', (int32_t)strtol(argv[2], NULL, 16), 0, NULL));
    }

    // check for server certificate
    if ((argc == 3) && !ds_stricmp(argv[1], "setcert"))
    {
        if ((pServerState->pServerCert = _HttpServLoadCertificate(argv[2], &pServerState->iServerCertLen,
            "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----")) != NULL)
        {
            iResult = ProtoHttpServControl(pServerState->pHttpServ, 'scrt', pServerState->iServerCertLen, 0, pServerState->pServerCert);
        }
        else
        {
            ZPrintf("%s: could not load certificate '%s'\n", argv[0], argv[2]);
            iResult = -1;
        }
        return(iResult);
    }

    // check for server private key
    if ((argc == 3) && !ds_stricmp(argv[1], "setkey"))
    {
        if ((pServerState->pServerKey = _HttpServLoadCertificate(argv[2], &pServerState->iServerKeyLen,
            "-----BEGIN RSA PRIVATE KEY-----", "-----END RSA PRIVATE KEY-----")) != NULL)
        {
            iResult = ProtoHttpServControl(pServerState->pHttpServ, 'skey', pServerState->iServerKeyLen, 0, pServerState->pServerKey);
        }
        else
        {
            ZPrintf("httpserv: could not load private key '%s'\n", argv[2]);
            iResult = -1;
        }
        return(iResult);
    }

    // check for version set
    if ((argc == 3) && !ds_stricmp(argv[1], "vers"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'vers', (int32_t)strtol(argv[2], NULL, 16), 0, NULL));
    }

    // check for min version set
    if ((argc == 3) && !ds_stricmp(argv[1], "vmin"))
    {
        return(ProtoHttpServControl(pServerState->pHttpServ, 'vmin', (int32_t)strtol(argv[2], NULL, 16), 0, NULL));
    }

    return(iResult);
}
