#include <revolution.h>
#include <revolution/mem.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"
#include "protohttp.h"
#include "protoping.h"

static void _OSInit(void);

int main( void )
{
    uint32_t uConnStatus;
    int32_t iCounter;
    
    _OSInit();
    
    OSReport("ticker start\n");

    NetPrintf(("ticker: initializing DirtySock\n"));

    NetConnStartup("-gamecode=RNFE -authcode=");
    
    NetConnConnect(NULL, NULL, 0);

    for (iCounter = 0; ; iCounter++)
    {
        uConnStatus = (uint32_t)NetConnStatus('conn', 0, NULL, 0);
        if (uConnStatus == '+onl')
        {
            uint32_t backEndEnvironment = NetConnStatus('envi', 0, NULL, 0);

            if (backEndEnvironment == NETCONN_PLATENV_TEST)
            {
                NetPrintf(("ticker: connected to internet; authserver is DWC_AUTHSERVER_DEBUG\n"));
            }
            else if (backEndEnvironment == NETCONN_PLATENV_PROD)
            {
                NetPrintf(("ticker: connected to internet; authserver is DWC_AUTHSERVER_RELEASE\n"));
            }
            else
            {
                NetPrintf(("ticker: error - invalid authserver!\n"));
            }
            break;
        }
        else if (uConnStatus >> 24 == '-')
        {
            NetPrintf(("ticker: error '%c%c%c%c' connecting to internet\n",
                (uConnStatus>>24)&0xff, (uConnStatus>>16)&0xff,
                (uConnStatus>> 8)&0xff, (uConnStatus>> 0)&0xff));
            break;
        }

        NetConnIdle();
        NetConnSleep(100);
    }

    // try a simple HTTP test
    if (uConnStatus == '+onl')
    {
        ProtoHttpRefT *pHttp = ProtoHttpCreate(4096);
        ProtoPingRefT *pPing = ProtoPingCreate(16);
        char strBuffer[1024];
        ProtoPingResponseT PingResponse;
        uint32_t uTimeout;
        int32_t iLen;
        
        NetPrintf(("ticker: http test\n"));

        ProtoHttpControl(pHttp, 'time', 120*1000, 0, NULL);
        
        // issue up to 16 http/ping requests
        for (uTimeout = NetTick()-1, iLen=0, iCounter=0; iCounter < 16; )
        {
            // see if its time to query
            if ((uTimeout != 0) && (NetTick() > uTimeout))
            {
                ProtoHttpGet(pHttp, "http://quote.yahoo.com/d/quotes.csv?s=^DJI,^SPC,^IXIC,ERTS,SNE,AOL,YHOO,^AORD,^N225,^FTSE&f=sl1c1&e=.csv", FALSE);
                ProtoPingRequestServer(pPing, SocketInTextGetAddr("198.105.192.69"), NULL, 0, 1000, -1);
                uTimeout = NetTick()+20*1000;
                iLen = 0;
                iCounter += 1;
            }

            // check for ping response
            if (ProtoPingResponse(pPing, NULL, NULL, &PingResponse) != 0)
            {
                NetPrintf(("ticker: ping response from %a: %dms\n", PingResponse.uAddr, PingResponse.iPing));
            }

            // update protohttp
            ProtoHttpUpdate(pHttp);

            // read incoming data into buffer
            if (iLen == 0)
            {
                if ((iLen = ProtoHttpRecvAll(pHttp, strBuffer, sizeof(strBuffer)-1)) > 0)
                {
                    // null-terminate response
                    strBuffer[iLen] = '\0';
            
                    // print response
                    NetPrintf(("ticker: received response\n"));
                    NetPrintf(("%s", strBuffer));
                }
            }
            NetConnIdle();
        }
        
        if (pPing != NULL)
        {
            ProtoPingDestroy(pPing);
        }
        if (pHttp != NULL)
        {
            ProtoHttpDestroy(pHttp);
        }
    }
    
    NetConnShutdown(0);
    
    NetPrintf(("ticker: done\n"));
    return(0);
}

static MEMHeapHandle _Ticker_hHeap = 0;

static void _OSInit(void)
{
    void *pArenaLo;
    void *pArenaHi;
    const int32_t iSize = 4*1024*1024;

    OSInit();

    NANDInit();

    pArenaLo = OSGetMEM2ArenaLo();
    pArenaHi = OSGetMEM2ArenaHi();

    OSReport("ticker: creating heap from 0x%08x to 0x%08x (max=0x%08x)\n", (uintptr_t)pArenaLo, (uintptr_t)((uint8_t *)pArenaLo + iSize), (uintptr_t)pArenaHi);
    if ((_Ticker_hHeap = MEMCreateExpHeapEx(pArenaLo, iSize, MEM_HEAP_OPT_THREAD_SAFE)) == MEM_HEAP_INVALID_HANDLE)
    {
        OSReport("ticker: MEMCreateExpHeapEx() failed (err=%d)\n", _Ticker_hHeap);
    }
    OSSetMEM2ArenaLo((uint8_t *)pArenaLo + iSize);

    // test it
    pArenaLo = DirtyMemAlloc(96, 'test', 'dflt', NULL);
    DirtyMemFree(pArenaLo, 'test', 'dflt', NULL);
}


void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    void *pMem;

    pMem = MEMAllocFromExpHeapEx(_Ticker_hHeap, (uint32_t)iSize, 32);
    if (iMemModule != 'nsoc')
    {
        DirtyMemDebugAlloc(pMem, iSize, iMemModule, iMemGroup, pMemGroupUserData);
    }

    return(pMem);
}

void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    MEMFreeToExpHeap(_Ticker_hHeap, pMem);
    if (iMemModule != 'nsoc')
    {
        DirtyMemDebugFree(pMem, 0, iMemModule, iMemGroup, pMemGroupUserData);
    }
}

