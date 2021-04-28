/*H********************************************************************************/
/*!
    \File resource.c

    \Description
        Test basic netresource functionality using tester app.

    \Notes

    \Copyright
    Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 12/16/2004 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/web/netresource.h"

#include "zlib.h"
#include "zmem.h"
#include "zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define RESOURCE_VERBOSE (DIRTYCODE_DEBUG && TRUE)

/*** Type Definitions *************************************************************/

typedef struct TesterResourceT
{
    NetResourceT *pNetResource;
    NetResourceXferT *pNetXfer;
    NetResourceXferT *pNetXferTest;
    char *pCache;
    int32_t iCacheLen;
    ZFileT iFile;
    int32_t bFileOpened;
} TesterResourceT;

/*** Variables ********************************************************************/

static TesterResourceT TesterResource = {NULL, NULL, NULL, NULL, 0, 0, 0};

/*** Private Functions ************************************************************/

static int32_t _CmdResourceCB(ZContext *argz, int32_t argc, char *argv[])
{
    // handle the shutdown case
    if (argc == 0)
    {
        // check to make sure we haven't already quit
        // destory things and then quit with no callback
        if(TesterResource.pNetResource)
            NetResourceDestroy(TesterResource.pNetResource);
        TesterResource.pNetResource = NULL;
        return(0);
    }

    return(ZCallback(_CmdResourceCB,15));
}

static uint32_t _HashBuffer(const void *pBuffer, uint32_t uBufferSize)
{
    const uint8_t *p = (const uint8_t *)(pBuffer);
    const uint8_t *pEnd = p + uBufferSize;
    uint32_t uHash = 2166136261UL;
    uint32_t c;

    while (p < pEnd)
    {
        if ((c = *p++) == '\0')
        {
            c = 2;
        }
        uHash = (uHash * 16777619) ^ c;
    }

    return(uHash);
}


static void _ResourceHTTPCB(NetResourceT *pState, NetResourceXferT *pXfer, void *pDataBuf, int32_t iDataLen)
{
    static uint32_t uEntrantNum = 0;
    uint32_t uType='0000';
    uint32_t uIdent=0;
    char buf[100];
    
    if(pXfer != NULL)
    {
        uType = pXfer->uType;
        uIdent = pXfer->uIdent;
    }

    switch(pXfer->iStatus)
    {
        case NETRSRC_STAT_START:
            #if RESOURCE_VERBOSE
            ZPrintf("resourceCB: START   (%c%c%c%c-%08X) --------------------\n", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
            #endif
            break;
        case NETRSRC_STAT_SIZE:
            // this is normally where I would allocate a data buffer
            // big enough to hold the pXfer->iLength data coming in
            // I can use the pBuffer convenience pointer in the
            // pXfer structure to maintain the memory location I
            // allocated.
            #if RESOURCE_VERBOSE
            ZPrintf("resourceCB: SIZE    (%c%c%c%c-%08X) size [%u]\n",
                NETRSRC_PRINTFTYPEIDENT(uType, uIdent), pXfer->iLength);
            #endif
            pXfer->uAmtReceived=0;
            break;
        case NETRSRC_STAT_DATA:
            // this is normally where I would memcpy the incoming data
            // from pDataBuf which is iDataLen int32_t into my allocated
            // memory buffer I created in the SIZE state
            pXfer->uAmtReceived += iDataLen;
            // occasionally print out some data stuff
            if((pXfer->uAmtReceived == (uint32_t)iDataLen) ||
               (pXfer->uAmtReceived == (uint32_t)(pXfer->iLength)) || 
               ((uEntrantNum++ % 200) == 0))
            {
                ZPrintf("resourceCB: DATA    (%c%c%c%c-%08X) pDataBuf [0x%X] iDataLen [%d] uAmtReceived [%u] seq [%d] \n",
                    NETRSRC_PRINTFTYPEIDENT(uType, uIdent), pDataBuf, iDataLen, pXfer->uAmtReceived, pXfer->iSeqNum);
            }
            // see if we need to put data in a file since we have no buffer
            if(TesterResource.pCache == NULL)
            {
                if(!TesterResource.bFileOpened)
                {
                    sprintf(buf,"%c%c%c%c-%08X", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
                    TesterResource.iFile = ZFileOpen(buf,ZFILE_OPENFLAG_WRONLY | ZFILE_OPENFLAG_BINARY | ZFILE_OPENFLAG_CREATE);
                    TesterResource.bFileOpened = 1;
                }   
                ZFileWrite(TesterResource.iFile, pDataBuf, iDataLen);
            }
            break;
        case NETRSRC_STAT_DONE:
            // suck all of the data out at once if we've enabled the cache
            if(TesterResource.pCache)
            {
                sprintf(buf,"%c%c%c%c-%08X", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
                TesterResource.iFile = ZFileOpen(buf,ZFILE_OPENFLAG_WRONLY | ZFILE_OPENFLAG_BINARY | ZFILE_OPENFLAG_CREATE);
                ZFileWrite(TesterResource.iFile, pDataBuf, iDataLen);
            }
            // see if we opened the file earlier
            if (TesterResource.bFileOpened)
            {
                ZFileClose(TesterResource.iFile);
            }

            ZPrintf("resourceCB: DONE    (%c%c%c%c-%08X) pDataBuf [0x%X] iDataLen [%d] uAmtReceived [%u] seq [%d] hash[0x%X]\n",
                NETRSRC_PRINTFTYPEIDENT(uType, uIdent), pDataBuf, iDataLen, pXfer->uAmtReceived, pXfer->iSeqNum, _HashBuffer(pDataBuf, iDataLen));

            break;
        case NETRSRC_STAT_ERROR:
            ZPrintf("resourceCB: ERROR   (%c%c%c%c-%08X)\n", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
            break;
        case NETRSRC_STAT_CANCEL:
            ZPrintf("resourceCB: CANCEL  (%c%c%c%c-%08X)\n", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
            break;
        case NETRSRC_STAT_TIMEOUT:
            ZPrintf("resourceCB: TIMEOUT (%c%c%c%c-%08X)\n", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
            break;
        case NETRSRC_STAT_MISSING:
            ZPrintf("resourceCB: MISSING (%c%c%c%c-%08X)\n", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
            break;
        case NETRSRC_STAT_FINISH:
            #if RESOURCE_VERBOSE
            ZPrintf("resourceCB: FINISH  (%c%c%c%c-%08X) --------------------\n", NETRSRC_PRINTFTYPEIDENT(uType, uIdent));
            #endif
            break;
    }
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdResource

    \Description
        Command funciton - takes input from user and 
        executes something in netresource.

        \Input *argz    - 
        \Input argc     - number of arguments to the function
        \Input *argv[]   - array of strings as arguments

    \Output Return Code - 0 if no error, >0 otherwise

    \Version 12/16/2004 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdResource(ZContext *argz, int32_t argc, char *argv[])
{
    int16_t iPriority = NETRSRC_PRIO_DEFAULT;

    // check usage
    if (argc < 2)
    {
        ZPrintf("   test the netresource module\n");
        ZPrintf("   usage: %s <command> - use one of the following commands: \n", argv[0]);
        ZPrintf("    create                - create a netresource instance\n");
        ZPrintf("    destroy               - shutdown the netresource module\n");
        ZPrintf("    cancel                - cancel a pending netresource fetch\n");
        ZPrintf("    cache <size in bytes> - create a cache, or disable by specifying no size\n");
        ZPrintf("    printcache            - print the contents of the cache (if running in debug mode)\n");
        ZPrintf("    fetch <type> <id hex #> <optional: dontcache/keepifpossible>\n");
        ZPrintf("    fetchstring <id string> <optional: dontcache/keepifpossible>\n");
        return(0);
    }

    if(argc >= 2)
    {
        char *strCmd = argv[1];
        if (strcmp(strCmd,"create")==0)
        {
            if(argc == 2)
                TesterResource.pNetResource = NetResourceCreate("dirtysock.ea.com/temp");
            else
                TesterResource.pNetResource = NetResourceCreate(argv[2]);

            #if RESOURCE_VERBOSE
            ZPrintf("NetResourceCreate returned pointer [0x%X]\n",TesterResource.pNetResource);
            #endif

            // start the callback
            ZCallback(_CmdResourceCB,50);
        }
        else if ((strcmp(strCmd,"destroy") == 0) && (TesterResource.pNetResource))
        {
            // destroy the module state object
            NetResourceDestroy(TesterResource.pNetResource);
            TesterResource.pNetResource=NULL;
            #if RESOURCE_VERBOSE
            ZPrintf("NetResourceDestroy called.\n");
            #endif
        }
        else if ((strcmp(strCmd,"cancel") == 0) && (TesterResource.pNetResource))
        {
            // destroy the module state object
            NetResourceCancel(TesterResource.pNetResource,TesterResource.pNetXfer->iSeqNum);
            #if RESOURCE_VERBOSE
            ZPrintf("NetResourceCancel called.\n");
            #endif
        }
        else if ((strcmp(strCmd,"cache") == 0) && (TesterResource.pNetResource))
        {
            // get rid of any old cache
            if(TesterResource.pCache)
            {
                ZMemFree(TesterResource.pCache);
                TesterResource.pCache = NULL;
                TesterResource.iCacheLen = 0;
            }
            // now build a new one
            if(argc==2)
            {
                NetResourceCache(TesterResource.pNetResource,NULL,0);
            }
            else
            {
                // create a cache
                TesterResource.iCacheLen = atoi(argv[2]);
                TesterResource.pCache = (char *)ZMemAlloc(TesterResource.iCacheLen * sizeof(char));
                NetResourceCache(TesterResource.pNetResource,TesterResource.pCache,TesterResource.iCacheLen);
            }
        }
        else if ((strcmp(strCmd,"fetch") == 0) && (TesterResource.pNetResource) && (argc >= 4))
        {
            uint32_t uType=0, uIdent=0;
            char *strType = argv[2];
            char *strIdent = argv[3];

            TesterResource.pNetXfer = (NetResourceXferT *)ZMemAlloc(sizeof(NetResourceXferT));

            uType = (strType[0] << 24) + (strType[1] << 16) + (strType[2] << 8) + strType[3];
            sscanf(strIdent,"%X",&uIdent);

            if(argc >= 5)
            {
                if(ds_stricmp(argv[4],"dontcache") == 0)
                {
                    #if RESOURCE_VERBOSE
                    ZPrintf("Fetching with DONTCACHE priority in cache.\n");
                    #endif
                    iPriority = NETRSRC_PRIO_DONTCACHE;
                }
                else if(ds_stricmp(argv[4],"keepifpossible") == 0)
                {
                    #if RESOURCE_VERBOSE
                    ZPrintf("Fetching with KEEPIFPOSSIBLE priority in cache.\n");
                    #endif
                    iPriority = NETRSRC_PRIO_KEEPIFPOSSIBLE;
                }
                else
                {
                    ZPrintf("Unknown cache priority [%s].  Using DEFAULT priority.\n",argv[4]);
                }
            }
            else
            {
                #if RESOURCE_VERBOSE
                ZPrintf("Fetching with DEFAULT priority in cache.\n");
                #endif
            }

            // initiate a fetch
            NetResourceFetch(TesterResource.pNetResource, TesterResource.pNetXfer, uType, uIdent, &_ResourceHTTPCB, 10000, iPriority);
        }
        else if ((strcmp(strCmd, "fetchtest") == 0) && (TesterResource.pNetResource))
        {
            TesterResource.pNetXferTest = (NetResourceXferT *)ZMemAlloc(5 * sizeof(NetResourceXferT));
            ZMemSet(TesterResource.pNetXferTest, 0, sizeof(*(TesterResource.pNetXferTest)));

            NetResourceFetch(TesterResource.pNetResource, &(TesterResource.pNetXferTest[0]), 'mpeg', 1, &_ResourceHTTPCB, 10000, NETRSRC_PRIO_DEFAULT);
            NetResourceFetch(TesterResource.pNetResource, &(TesterResource.pNetXferTest[1]), 'mpeg', 2, &_ResourceHTTPCB, 10000, NETRSRC_PRIO_DEFAULT);
            NetResourceFetch(TesterResource.pNetResource, &(TesterResource.pNetXferTest[2]), 'mpeg', 1, &_ResourceHTTPCB, 10000, NETRSRC_PRIO_DEFAULT);
            NetResourceFetch(TesterResource.pNetResource, &(TesterResource.pNetXferTest[3]), 'mpeg', 4, &_ResourceHTTPCB, 10000, NETRSRC_PRIO_DEFAULT);
            NetResourceFetch(TesterResource.pNetResource, &(TesterResource.pNetXferTest[4]), 'mpeg', 1, &_ResourceHTTPCB, 10000, NETRSRC_PRIO_DEFAULT);
        }
        else if ((strcmp(strCmd,"fetchstring") == 0) && (TesterResource.pNetResource) && (argc >= 3))
        {
            char *pName = argv[2];

            TesterResource.pNetXfer = (NetResourceXferT *)ZMemAlloc(sizeof(NetResourceXferT));

            if(argc >= 4)
            {
                if(ds_stricmp(argv[3],"dontcache") == 0)
                {
                    #if RESOURCE_VERBOSE
                    ZPrintf("Fetching with DONTCACHE priority in cache.\n");
                    #endif
                    iPriority = NETRSRC_PRIO_DONTCACHE;
                }
                else if(ds_stricmp(argv[3],"keepifpossible") == 0)
                {
                    #if RESOURCE_VERBOSE
                    ZPrintf("Fetching with KEEPIFPOSSIBLE priority in cache.\n");
                    #endif
                    iPriority = NETRSRC_PRIO_KEEPIFPOSSIBLE;
                }
                else
                {
                    ZPrintf("Unknown cache priority [%s].  Using DEFAULT priority.\n",argv[4]);
                }
            }
            else
            {
                #if RESOURCE_VERBOSE
                ZPrintf("Fetching with DEFAULT priority in cache.\n");
                #endif
            }

            // initiate a fetch
            NetResourceFetchString(TesterResource.pNetResource, TesterResource.pNetXfer, pName, &_ResourceHTTPCB, 10000, iPriority);
        }
        else if ((strcmp(strCmd,"printcache") == 0) && (TesterResource.pNetResource))
        {
#if DIRTYCODE_LOGGING
#if RESOURCE_VERBOSE
            NetResourceCachePrint(TesterResource.pNetResource);
#endif
#endif
        }
        else
        {
            ZPrintf("UNKNOWN NETRESOURCE COMMAND [%s].\n",strCmd);
        }
    }

    return(0);
}


