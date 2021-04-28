/*H********************************************************************************/
/*!
    \File http2.c

    \Description
        Test the ProtoHttp2 client

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttp2.h"
#include "DirtySDK/proto/protossl.h"
#include "libsample/zmem.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

//! default address for the grpc server we are testing
#define DEFAULT_GRPC_SERVER ("http://10.14.141.14:50051")

/*** Type Definitions *************************************************************/

//! wrapper for the data payloads used for our tests
typedef struct DataT
{
    int32_t iSize;
    uint8_t pData[64];
} DataT;

typedef struct Http2RefT
{
    ProtoHttp2RefT *pHttp;  //!< http2 module ref
    int32_t iStreamId;      //!< main stream identifier

    char strGrpcHost[32];   //!< the default host to run grpc tests

    uint8_t *pBuf;          //!< buffer for reading data
    int32_t iBufSize;       //!< size of the buffer

    const uint8_t *pData;   //!< buffer for sending data
    int32_t iDataSize;      //!< size of data

    const DataT *pStream;   //!< streaming data
    int32_t iIndex;         //!< index of the data for client/bi-directional sends
    int32_t iMaxIndex;      //!< max index of data
} Http2RefT;

/*** Variables ********************************************************************/

// instance for used in testing
static Http2RefT _Http2 = { NULL, 0, DEFAULT_GRPC_SERVER, NULL, 0, NULL, 0, NULL, 0, 0};

// used for the RouteGuide/GetFeature unary RPC
static const DataT _GetFeature[] =
{
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0x9a, 0xa6,
            0x8c, 0xc3, 0x01, 0x10, 0x96, 0x9f, 0x98, 0x9c,
            0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } }
};

// used for the RouteGuide/ListFeatures server streaming RPC
static const DataT _ListFeatures[] =
{
    { 43, { 0x00, 0x00, 0x00, 0x00, 0x26, 0x0a, 0x11, 0x08,
            0x80, 0x88, 0xde, 0xbe, 0x01, 0x10, 0x80, 0xd1,
            0xaf, 0x9a, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01,
            0x12, 0x11, 0x08, 0x80, 0xe2, 0xa2, 0xc8, 0x01,
            0x10, 0x80, 0xab, 0xf4, 0xa3, 0xfd, 0xff, 0xff,
            0xff, 0xff, 0x01 } }
};

// used for the RouteGuide/RecordRoute client streaming RPC
static const DataT _RecordRoute[] =
{
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0x8f, 0xbd, 0xbc, 0xc2, 0x01, 0x10, 0xed, 0xff, 0x9a, 0x9c, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0xb8, 0xeb, 0xcd, 0xc2, 0x01, 0x10, 0xb5, 0xf2, 0x9d, 0x9d, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0xfc, 0xed, 0x9d, 0xc5, 0x01, 0x10, 0xd4, 0xdc, 0xeb, 0x9a, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0xb8, 0xde, 0xa2, 0xc8, 0x01, 0x10, 0xc0, 0xaa, 0xfb, 0x9e, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0xc5, 0x88, 0xb5, 0xc5, 0x01, 0x10, 0xbf, 0xe8, 0xa0, 0x9d, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0xb6, 0x85, 0x8b, 0xc8, 0x01, 0x10, 0x9f, 0xdf, 0x83, 0x9c, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0xfb, 0xfa, 0xd2, 0xc1, 0x01, 0x10, 0xe6, 0xc1, 0x8c, 0x9e, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0x98, 0xcd, 0xdf, 0xc6, 0x01, 0x10, 0xf9, 0xa8, 0x81, 0x9e, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0x99, 0xbf, 0xf4, 0xc4, 0x01, 0x10, 0xf3, 0x9c, 0xd0, 0x9e, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x08, 0x8f, 0xa8, 0xc3, 0xc4, 0x01, 0x10, 0xd5, 0xf4, 0xa0, 0x9d, 0xfd, 0xff, 0xff, 0xff, 0xff, 0x01 } }
};

// used for the RouteGuide/RouteChat bi-directional streaming RPC
static const DataT _RouteChat[] = 
{
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x0a, 0x00, 0x12, 0x0d, 0x46, 0x69, 0x72, 0x73, 0x74, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 25, { 0x00, 0x00, 0x00, 0x00, 0x14, 0x0a, 0x02, 0x10, 0x01, 0x12, 0x0e, 0x53, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 24, { 0x00, 0x00, 0x00, 0x00, 0x13, 0x0a, 0x02, 0x08, 0x01, 0x12, 0x0d, 0x54, 0x68, 0x69, 0x72, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 23, { 0x00, 0x00, 0x00, 0x00, 0x12, 0x0a, 0x00, 0x12, 0x0e, 0x46, 0x6f, 0x75, 0x72, 0x74, 0x68, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x0a, 0x00, 0x12, 0x0d, 0x46, 0x69, 0x72, 0x73, 0x74, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 25, { 0x00, 0x00, 0x00, 0x00, 0x14, 0x0a, 0x02, 0x10, 0x01, 0x12, 0x0e, 0x53, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 24, { 0x00, 0x00, 0x00, 0x00, 0x13, 0x0a, 0x02, 0x08, 0x01, 0x12, 0x0d, 0x54, 0x68, 0x69, 0x72, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 23, { 0x00, 0x00, 0x00, 0x00, 0x12, 0x0a, 0x00, 0x12, 0x0e, 0x46, 0x6f, 0x75, 0x72, 0x74, 0x68, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x0a, 0x00, 0x12, 0x0d, 0x46, 0x69, 0x72, 0x73, 0x74, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 25, { 0x00, 0x00, 0x00, 0x00, 0x14, 0x0a, 0x02, 0x10, 0x01, 0x12, 0x0e, 0x53, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 24, { 0x00, 0x00, 0x00, 0x00, 0x13, 0x0a, 0x02, 0x08, 0x01, 0x12, 0x0d, 0x54, 0x68, 0x69, 0x72, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 23, { 0x00, 0x00, 0x00, 0x00, 0x12, 0x0a, 0x00, 0x12, 0x0e, 0x46, 0x6f, 0x75, 0x72, 0x74, 0x68, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x0a, 0x00, 0x12, 0x0d, 0x46, 0x69, 0x72, 0x73, 0x74, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 25, { 0x00, 0x00, 0x00, 0x00, 0x14, 0x0a, 0x02, 0x10, 0x01, 0x12, 0x0e, 0x53, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 24, { 0x00, 0x00, 0x00, 0x00, 0x13, 0x0a, 0x02, 0x08, 0x01, 0x12, 0x0d, 0x54, 0x68, 0x69, 0x72, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 23, { 0x00, 0x00, 0x00, 0x00, 0x12, 0x0a, 0x00, 0x12, 0x0e, 0x46, 0x6f, 0x75, 0x72, 0x74, 0x68, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 22, { 0x00, 0x00, 0x00, 0x00, 0x11, 0x0a, 0x00, 0x12, 0x0d, 0x46, 0x69, 0x72, 0x73, 0x74, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 25, { 0x00, 0x00, 0x00, 0x00, 0x14, 0x0a, 0x02, 0x10, 0x01, 0x12, 0x0e, 0x53, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 24, { 0x00, 0x00, 0x00, 0x00, 0x13, 0x0a, 0x02, 0x08, 0x01, 0x12, 0x0d, 0x54, 0x68, 0x69, 0x72, 0x64, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } },
    { 23, { 0x00, 0x00, 0x00, 0x00, 0x12, 0x0a, 0x00, 0x12, 0x0e, 0x46, 0x6f, 0x75, 0x72, 0x74, 0x68, 0x20, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65 } }

};

/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdHttp2CleaupSend

    \Description
        Cleans up the module ref for a request send

    \Input *pHttp2  - module ref for this test to cleanup

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHttp2CleaupSend(Http2RefT *pHttp2)
{
    pHttp2->pData = NULL;
    pHttp2->iDataSize = 0;
    pHttp2->pStream = NULL;
    pHttp2->iMaxIndex = 0;
    pHttp2->iIndex = 0;
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2IdleCb

    \Description
        Update for the http2 testing module

    \Input *pArgz   - environment
    \Input iArgc    - standard number of arguments
    \Input *pArgv[] - standard arg list

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CmdHttp2IdleCb(ZContext *pArgz, int32_t iArgc, char *pArgv[])
{
    int32_t iResult;
    Http2RefT *pHttp2 = &_Http2;

    // clean up the ref if needed
    if (iArgc == 0)
    {
        if (pHttp2->pHttp != NULL)
        {
            ProtoHttp2Destroy(pHttp2->pHttp);
            ProtoSSLClrCACerts();
            pHttp2->pHttp = NULL;
        }
        return(0);
    }

    // try to send data
    if (pHttp2->pData != NULL)
    {
        if (pHttp2->iDataSize > 0)
        {
            int32_t iDataSent;
            if ((iDataSent = ProtoHttp2Send(pHttp2->pHttp, pHttp2->iStreamId, pHttp2->pData, pHttp2->iDataSize)) < 0)
            {
                return(1);
            }
            pHttp2->pData += iDataSent;
            pHttp2->iDataSize -= iDataSent;
        }
        if (pHttp2->iIndex < pHttp2->iMaxIndex)
        {
            pHttp2->pData = pHttp2->pStream[pHttp2->iIndex].pData;
            pHttp2->iDataSize = pHttp2->pStream[pHttp2->iIndex].iSize;
            pHttp2->iIndex += 1;
        }
        else
        {
            _CmdHttp2CleaupSend(pHttp2);
            ProtoHttp2Send(pHttp2->pHttp, pHttp2->iStreamId, NULL, PROTOHTTP2_STREAM_END);
        }
    }

    // try to read data
    if (pHttp2->iStreamId != 0)
    {
        uint8_t bDone;
        iResult = ProtoHttp2RecvAll(pHttp2->pHttp, pHttp2->iStreamId, pHttp2->pBuf, pHttp2->iBufSize);
        bDone = iResult != PROTOHTTP2_RECVWAIT && iResult != PROTOHTTP2_RECVBUFF;

        // handle finished
        if (iResult > 0)
        {
            iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'head', NULL, 0);
            ZPrintf("http2: 'head' %d\n", iResult);

            iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'body', NULL, 0);
            ZPrintf("http2: 'body' %d\n", iResult);

            iResult = ProtoHttp2Status(pHttp2->pHttp, pHttp2->iStreamId, 'done', NULL, 0);
            ZPrintf("http2: 'done' %s\n", iResult ? "TRUE" : "FALSE");
        }
        // increase size if needed
        else if (iResult == PROTOHTTP2_RECVBUFF)
        {
            if (pHttp2->iBufSize == 0)
            {
                pHttp2->pBuf = ZMemAlloc(4096);
                pHttp2->iBufSize = 4096;
            }
            else
            {
                uint8_t *pNewBuf = ZMemAlloc(pHttp2->iBufSize * 2);
                ds_memcpy(pNewBuf, pHttp2->pBuf, pHttp2->iBufSize);

                ZMemFree(pHttp2->pBuf);
                pHttp2->pBuf = pNewBuf;
                pHttp2->iBufSize *= 2;
            }

            ZPrintf("http2: allocated larger buffer: new size %d\n", pHttp2->iBufSize);
        }
        else if (iResult == PROTOHTTP2_RECVFAIL)
        {
            ZPrintf("http2: receive failed\n");
        }

        // cleanup now that we are done
        if (bDone == TRUE)
        {
            ProtoHttp2StreamFree(pHttp2->pHttp, pHttp2->iStreamId);
            pHttp2->iStreamId = 0;

            ZMemFree(pHttp2->pBuf);
            pHttp2->pBuf = NULL;
            pHttp2->iBufSize = 0;
        }
    }

    // update the ref
    ProtoHttp2Update(pHttp2->pHttp);

    return(ZCallback(_CmdHttp2IdleCb, 1)); // slow enough to allow us to test abort
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2WriteCb

    \Description
        Callback when we receive data (if registered)

    \Input *pState      - module state
    \Input *pCbInfo     - information about the request
    \Input *pData       - data we are receiving
    \Input iDataSize    - size of the data
    \Input *pUserData   - user specific data

    \Output
        int32_t         - result of the operation

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CmdHttp2WriteCb(ProtoHttp2RefT *pState, const ProtoHttp2WriteCbInfoT *pCbInfo, const uint8_t *pData, int32_t iDataSize, void *pUserData)
{
    Http2RefT *pHttp2 = (Http2RefT *)pUserData;
    ZPrintf("received %d\n", iDataSize);

    // if the request is complete then cleanup the stream
    if ((iDataSize == PROTOHTTP2_RECVDONE) || (iDataSize == PROTOHTTP2_RECVFAIL))
    {
        ProtoHttp2StreamFree(pState, pCbInfo->iStreamId);
        pHttp2->iStreamId = 0;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _CmdHttp2CreateGrpcRequest

    \Description
        Wrapper to create grpc request

    \Input *pHttp2      - module state
    \Input *pUri        - the path of the request
    \Input *pPayload    - pointer to array of payload data
    \Input iNumPayload  - number of entries in array

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHttp2CreateGrpcRequest(Http2RefT *pHttp2, const char *pUri, const DataT *pPayload, int32_t iNumPayloads)
{
    char strUrl[128];

    // setup url
    ds_snzprintf(strUrl, sizeof(strUrl), "%s/%s", pHttp2->strGrpcHost, pUri);

    // update headers, we set null first to clear the previous headers
    ProtoHttp2Control(pHttp2->pHttp, 0, 'apnd', 0, 0, NULL);
    ProtoHttp2Control(pHttp2->pHttp, 0, 'apnd', 0, 0, (void *)"te: trailers\r\ncontent-type: application/grpc\r\n");

    // create the request
    if (ProtoHttp2Request(pHttp2->pHttp, strUrl, NULL, PROTOHTTP2_STREAM_BEGIN, PROTOHTTP_REQUESTTYPE_POST, &pHttp2->iStreamId) < 0)
    {
        ZPrintf("http2: failed to create request %s\n", pUri);
        return;
    }
    // set the array and num indexies
    pHttp2->pStream = pPayload;
    pHttp2->iMaxIndex = iNumPayloads;
    // set index to 1 as we always read first entry
    pHttp2->iIndex = 1;
    // read the first entry
    pHttp2->pData = (*pPayload).pData;
    pHttp2->iDataSize = (*pPayload).iSize;
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttp2

    \Description
        Entrypoint for the http2 testing module

    \Input *pArgz   - environment
    \Input iArgc    - standard number of arguments
    \Input *pArgv[] - standard arg list

    \Output
        int32_t     - standard return value

    \Version 12/01/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t CmdHttp2(ZContext *pArgz, int32_t iArgc, char *pArgv[])
{
    Http2RefT *pHttp2 = &_Http2;

    // allocate the module if needed
    if (pHttp2->pHttp == NULL)
    {
        if ((pHttp2->pHttp = ProtoHttp2Create(0)) == NULL)
        {
            ZPrintf("http2: could not allocate module state\n");
            return(1);
        }
    }

    // make sure there are enough parameters
    if (iArgc < 2)
    {
        return(0);
    }

    // close the connection
    if (ds_stricmp(pArgv[1], "close") == 0)
    {
        ProtoHttp2Close(pHttp2->pHttp);
    }
    // update the logging
    else if (ds_stricmp(pArgv[1], "spam") == 0)
    {
        ProtoHttp2Control(pHttp2->pHttp, PROTOHTTP2_INVALID_STREAMID, 'spam', atoi(pArgv[2]), 0, NULL);
    }
    else if (ds_stricmp(pArgv[1], "grpc") == 0)
    {
        ds_strnzcpy(pHttp2->strGrpcHost, pArgv[2], sizeof(pHttp2->strGrpcHost));
    }

    if (pHttp2->iStreamId == 0)
    {
        // handle the normal get/head/options
        if ((ds_stricmp(pArgv[1], "get") == 0) || (ds_stricmp(pArgv[1], "head") == 0) || (ds_stricmp(pArgv[1], "options") == 0))
        {
            // a bit nasty but gets the job done
            ProtoHttpRequestTypeE eRequestType = *pArgv[1] == 'g' ? PROTOHTTP_REQUESTTYPE_GET : *pArgv[1] == 'h' ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_OPTIONS;

            if ((iArgc == 4) && (ds_stricmp(pArgv[3], "-cb") == 0))
            {
                ProtoHttp2RequestCb(pHttp2->pHttp, pArgv[2], NULL, 0, eRequestType, &pHttp2->iStreamId, _CmdHttp2WriteCb, &_Http2);
            }
            else
            {
                ProtoHttp2Request(pHttp2->pHttp, pArgv[2], NULL, 0, eRequestType, &pHttp2->iStreamId);
            }
        }
        else if (ds_stricmp(pArgv[1], "test1") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/GetFeature", _GetFeature, 0);
        }
        else if (ds_stricmp(pArgv[1], "test2") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/ListFeatures", _ListFeatures, 0);
        }
        else if (ds_stricmp(pArgv[1], "test3") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/RecordRoute", _RecordRoute, sizeof(_RecordRoute) / sizeof(*_RecordRoute));
        }
        else if (ds_stricmp(pArgv[1], "test4") == 0)
        {
            _CmdHttp2CreateGrpcRequest(pHttp2, "routeguide.RouteGuide/RouteChat", _RouteChat, sizeof(_RouteChat) / sizeof(*_RouteChat));
        }
    }
    else
    {
        if (ds_stricmp(pArgv[1], "abort") == 0)
        {
            // abort the request
            ProtoHttp2Abort(pHttp2->pHttp, pHttp2->iStreamId);

            // cleanup send data
            _CmdHttp2CleaupSend(pHttp2);

            // cleanup stream
            ProtoHttp2StreamFree(pHttp2->pHttp, pHttp2->iStreamId);
            pHttp2->iStreamId = 0;
        }
    }

    return(ZCallback(_CmdHttp2IdleCb, 1));
}
