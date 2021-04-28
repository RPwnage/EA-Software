/*H*************************************************************************************************/
/*!

    \File upnptest.c
    
    \Description
        UPnP tester.

    \Copyright
        Copyright (c) Electronic Arts  ALL RIGHTS RESERVED.
    
    \Version 1.0 01/04/2006 (JLB) Initial version
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <revolution.h>
#include <revolution/mem.h>

// dirtysock includes
#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"
#include "porttestapi.h"
#include "protohttp.h"
#include "protoname.h"
#include "protoupnp.h"

/*** Defines ***************************************************************************/

#define UPNPTEST_VERSION "1.2.1"

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

/*  basic format of the xml document posted to the server is like this:

    <upnpDevice name="Belkin Wireless Home Networking Router">
    <upnpClient user="sloginname" addr="localaddr" mac="macaddr" />
    <upnpResult probe0="success|failure" addmap="success|failure" probe1="success|failure" delmap="success|failure"/>
        <upnpDiscovery>
            <upnpDiscoveryHttp>
            </upnpDiscoveryHttp>        
        </upnpDiscovery>
        <upnpDescription>
            <upnpDescriptionHttp>
            </upnpDescriptionHttp>
            <upnpDescriptionXml>
            </upnpDescriptionXml>
        </upnpDescription>
        <upnpServiceDescription>
            <upnpServiceDescriptionHttp>
            </upnpServiceDescriptionHttp>
            <upnpServiceDescriptionXml>
            </upnpServiceDescriptionXml>
        </upnpServiceDescription>
        <upnpGetExternalIPAddressResponse>
            <upnpGetExternalIPAddressHttp>
            </upnpGetExternalIPAddressHttp>
            <upnpGetExternalIPAddressXml>
            </upnpGetExternalIPAddressXml>
        </upnpGetExternalIPAddressResponse>
        <upnpGetSpecificPortMappingEntryResponse>
            <upnpGetSpecificPortMappingEntryHttp>
            </upnpGetSpecificPortMappingEntryHttp>
            <upnpGetSpecificPortMappingEntryXml>
            </upnpGetSpecificPortMappingEntryXml>
        </upnpGetSpecificPortMappingEntryResponse>
        ...
    </upnpDevice>
*/

// protoupnp macro to get router info and set up a port mapping
static const ProtoUpnpMacroT _UpnpTest_CmdList[] =
{
    // basically the 'addp' macro plus service description
    { 'disc',  0, 0, NULL },
    { 'desc',  0, 0, NULL },
    { 'sdsc',  0, 0, NULL },
    { 'gadr',  0, 0, NULL },
    { 'gprt',  0, 0, NULL },
    { 'aprt',  0, 0, NULL },
    
    // done
    { 0,       0, 0, NULL }
};

static char _UpnpTest_strOutput[256*1024] = "";
static char _UpnpTest_strLogFile[256*1024] = "";
static char _UpnpTest_strDebugTxt[256*1024] = "";

// Public variables


/*** Private Functions *****************************************************************/

static int32_t _UpnpLogPrintf(char *pLog, const char *pFormat, ...)
{
    va_list pFmtArgs;
    static char strText[4096];
    int32_t iLength;

    // format text
	va_start(pFmtArgs, pFormat);
    iLength = vsnzprintf(strText, sizeof(strText), pFormat, pFmtArgs);
    va_end(pFmtArgs);
    
    // append output to logfile
    strcpy(pLog, strText);

    // return length of text    
    return(iLength);
}

#if DIRTYCODE_DEBUG
static int32_t _UpnpTestDebug(void *pParm, const char *pText)
{
    static char *_pDebugTxt = _UpnpTest_strDebugTxt;
    _pDebugTxt += _UpnpLogPrintf(_pDebugTxt, "%s", pText);
    return(1);
}
#endif

static MEMHeapHandle _UPnPTest_hHeap = 0;

static void _UpnpTestWaitCursor(void)
{
    NetConnIdle();
    NetConnSleep(16);
}

static int32_t _UpnpTestInit(void)
{
    void *pArenaLo, *pArenaHi;
    const int32_t iSize = 4*1024*1024;
    uint32_t uConnStatus;
    int32_t iCounter;

    OSReport("UpnpTest Init\n");

    OSInit();
    NANDInit();

    pArenaLo = OSGetMEM2ArenaLo();
    pArenaHi = OSGetMEM2ArenaHi();

    OSReport("upnptest: creating heap from 0x%08x to 0x%08x (max=0x%08x)\n", (uintptr_t)pArenaLo, (uintptr_t)((uint8_t *)pArenaLo + iSize), (uintptr_t)pArenaHi);
    if ((_UPnPTest_hHeap = MEMCreateExpHeapEx(pArenaLo, iSize, MEM_HEAP_OPT_THREAD_SAFE)) == MEM_HEAP_INVALID_HANDLE)
    {
        OSReport("upnptest: MEMCreateExpHeapEx() failed (err=%d)\n", _UPnPTest_hHeap);
    }
    OSSetMEM2ArenaLo((uint8_t *)pArenaLo + iSize);

    // test it
    pArenaLo = DirtyMemAlloc(96, 'test', 'dflt', NULL);
    DirtyMemFree(pArenaLo, 'test', 'dflt', NULL);

    // connect to network
    NetConnStartup("");
    NetConnConnect(NULL, NULL, 0);

    for (iCounter = 0; ; iCounter++)
    {
        uConnStatus = (uint32_t)NetConnStatus('conn', 0, NULL, 0);
        if (uConnStatus == '+onl')
        {
            NetPrintf(("upnptest: connected to internet\n"));
            break;
        }
        else if (uConnStatus >> 24 == '-')
        {
            NetPrintf(("ticker: error '%c%c%c%c' connecting to internet\n",
                (uConnStatus>>24)&0xff, (uConnStatus>>16)&0xff,
                (uConnStatus>> 8)&0xff, (uConnStatus>> 0)&0xff));
            break;
        }
        
        _UpnpTestWaitCursor();
    }

    // return success
    return(uConnStatus == '+onl' ? 1 : -1);
}

static void _UpnpTestPortTestLogResult(const char *pResult)
{
    NetPrintf(("protoupnp: %s\n", pResult));
}

static void _PortTesterCallback(PortTesterT *pRef, int32_t iStatus, void *pUserData)
{
    static char *_strPtStates[] =
    {
        "  IDLE",               // waiting for something to do
        "  SENDING",            // sending packets to the server
        "  SENT",               // packets sent, waiting for response(s)
        "  RECEIVING",          // some or all packets have been received
        "  SUCCESS",            // all packets received successfully
        "  PARTIALSUCCESS",     // some of the packets received successfully
        "  TIMEOUT",            // a timeout occurred before all packets were received
        "  ERROR",              // an error occured
        "  UNKNOWN"
    };
    _UpnpTestPortTestLogResult(_strPtStates[iStatus]);
}

static PortTesterT *_UpnpTestCreatePortTester(const char *pServerName)
{
    HostentT *pHostent;
    int32_t iServerIP=0;

    // resolve server name
    if ((pHostent = ProtoNameAsync(pServerName, 10*1000)) != NULL)
    {
        for( ; pHostent->Done(pHostent) == FALSE ; )
        {
            // give time to DirtySock idle functions
            NetConnIdle();
        
            // spin wait cursor
            _UpnpTestWaitCursor();
        }

        // extract IP address
        iServerIP = pHostent->addr;

        // dispose of resolver
        pHostent->Free(pHostent);
    }

    // if we couldn't resolve the name, fall back on the hard-coded IP address which might work
    if (iServerIP == 0)
    {
        NetPrintf(("upnptest: unable to resolve port tester name, falling back on IP address\n"));
        iServerIP = SocketInTextGetAddr("159.153.229.211");
    }
    
    // create the porttester
    return(PortTesterCreate(iServerIP, 9660, 2*1000));
}

static uint32_t _UpnpTestPortTest(PortTesterT *pPortTester)
{
    int32_t eStatus, iResult;
    uint32_t bDone;
    
    // start initial port test
    _UpnpTestPortTestLogResult("Starting port test of port 3658");
    PortTesterTestPort(pPortTester, 3658, 5, _PortTesterCallback, NULL);
    
    for(bDone = FALSE ; bDone == FALSE ; )
    {
        // give time to DirtySock idle functions (including porttester idle function)
        NetConnIdle();
        
        // get port tester status
        eStatus = PortTesterStatus(pPortTester, 'stat', NULL, 0);
        if (eStatus == PORTTESTER_STATUS_TIMEOUT)
        {
            _UpnpTestPortTestLogResult("-timeout");
            bDone = TRUE;
        }
        else if (eStatus == PORTTESTER_STATUS_ERROR)
        {
            _UpnpTestPortTestLogResult("-error");
            bDone = TRUE;
        }
        else if (eStatus == PORTTESTER_STATUS_UNKNOWN)
        {
            _UpnpTestPortTestLogResult("-unknown");
            bDone = TRUE;
        }
        else if (eStatus == PORTTESTER_STATUS_SUCCESS)
        {
            _UpnpTestPortTestLogResult("+success");
            bDone = TRUE;
        }
        else if (eStatus == PORTTESTER_STATUS_PARTIALSUCCESS)
        {
            _UpnpTestPortTestLogResult("+partialsuccess");
            bDone = TRUE;
        }

        // spin wait cursor
        _UpnpTestWaitCursor();
    }

    if (eStatus == PORTTESTER_STATUS_SUCCESS)
    {
        iResult = 1;
    }
    else if (eStatus == PORTTESTER_STATUS_PARTIALSUCCESS)
    {
        iResult = 2;
    }
    else
    {
        iResult = 0;
    }
    return(iResult);
}

static char *_UpnpTestAddResponse(ProtoUpnpRefT *pProtoUpnp, char *pResponse, int32_t iElapsed)
{
    static char strHead[512], strHeadReq[512], strBody[16*1024], strBodyReq[2048];
    char strRequestName[64];
    int32_t iMacro;
    
    // get current command name
    iMacro = ProtoUpnpStatus(pProtoUpnp, 'macr', strRequestName, sizeof(strRequestName));

    // get http request header
    ProtoUpnpStatus(pProtoUpnp, 'rtxt', strHeadReq, sizeof(strHeadReq));
    
    // get http request body
    ProtoUpnpStatus(pProtoUpnp, 'rbdy', strBodyReq, sizeof(strBodyReq));

    // get http response header
    ProtoUpnpStatus(pProtoUpnp, 'htxt', strHead, sizeof(strHead));
    
    // get http response body
    ProtoUpnpStatus(pProtoUpnp, 'body', strBody, sizeof(strBody));
    
    // add response to server response
    if (iMacro == 'disc')
    {
        pResponse += snzprintf(pResponse, sizeof(_UpnpTest_strOutput) - (pResponse - _UpnpTest_strOutput),
            "<upnpDiscovery elapsed=\"%d\">\r\n"
            "    <upnpDiscoveryHttp>\r\n"
            "%s"
            "    </upnpDiscoveryHttp>\r\n"
            "</upnpDiscovery>\r\n",
            iElapsed, strBody);
    }
    else
    {
        pResponse += snzprintf(pResponse, sizeof(_UpnpTest_strOutput) - (pResponse - _UpnpTest_strOutput),
            "<upnp%s elapsed=\"%d\">\r\n"
            "    <upnp%sRequest>\r\n"
            "        <upnpRequestHttp>\r\n"
            "%s"
            "        </upnpRequestHttp>\r\n"        
            "        <upnpRequestXml>\r\n"
            "%s"
            "        </upnpRequestXml>\r\n"        
            "    </upnp%sRequest>\r\n"        
            "    <upnp%sResponse>\r\n"        
            "        <upnpResponseHttp>\r\n"
            "%s"
            "        </upnpResponseHttp>\r\n"        
            "        <upnpResponseXml>\r\n"
            "%s"
            "        </upnpResponseXml>\r\n"        
            "    </upnp%sResponse>\r\n"        
            "</upnp%s>\r\n",
            strRequestName, iElapsed,
                strRequestName,
                    strHeadReq,
                    strBodyReq,
                strRequestName,
                strRequestName,
                    strHead,
                    strBody,
                strRequestName,
            strRequestName);
    }

    // clear buffers after use
    memset(strHeadReq, 0, sizeof(strHeadReq));
    memset(strBodyReq, 0, sizeof(strBodyReq));
    memset(strHead, 0, sizeof(strHead));
    memset(strBody, 0, sizeof(strBody));

    return(pResponse);
}

static int32_t _UpnpTestAddMapping(ProtoUpnpRefT *pProtoUpnp, char **ppResponse)
{
    uint32_t uElapsed = NetTick();
    int32_t iCmd;
    
    // try and create a port mapping
    ProtoUpnpControl(pProtoUpnp, 'macr', 'user', 0, _UpnpTest_CmdList);

    // execute macro to completion
    for( ; ProtoUpnpStatus(pProtoUpnp, 'macr', NULL, 0) != 0; )
    {
        // give time to DirtySock idle functions
        NetConnIdle();

        // update upnp module
        ProtoUpnpUpdate(pProtoUpnp);

        // see if current macro has completed
        if ((ProtoUpnpStatus(pProtoUpnp, 'idle', NULL, 0) == TRUE) && (ProtoUpnpStatus(pProtoUpnp, 'macr', NULL, 0) != 0))
        {
            *ppResponse = _UpnpTestAddResponse(pProtoUpnp, *ppResponse, (signed)(NetTick() - uElapsed));
            uElapsed = NetTick();
        }
        else if ((NetTick() - uElapsed) > 30*1000)   // timed out?
        {
            // get command and log the timeout error
            if ((iCmd = ProtoUpnpStatus(pProtoUpnp, 'macr', NULL, 0)) == 0)
            {
                iCmd = 'unkn';
            }
            NetPrintf(("upnptest: timeout executing '%c%c%c%c' command\n",
                (iCmd>>24)&0xff, (iCmd>>16)&0xff, (iCmd>>8)&0xff, iCmd&0xff));
            *ppResponse = _UpnpTestAddResponse(pProtoUpnp, *ppResponse, -1);
            // abort current command
            ProtoUpnpControl(pProtoUpnp, 'abrt', 0, 0, NULL);
        }
        
        // spin wait cursor
        _UpnpTestWaitCursor();
    }
    
    return((ProtoUpnpStatus(pProtoUpnp, 'stat', NULL, 0) & PROTOUPNP_STATUS_ADDPORTMAP) != 0);
}

static int32_t _UpnpTestDelMapping(ProtoUpnpRefT *pProtoUpnp, char **ppResponse)
{
    uint32_t uElapsed = NetTick();
    
    // try and create a port mapping
    ProtoUpnpControl(pProtoUpnp, 'dprt', 0, 0, NULL);

    // execute macro to completion
    for( ; (ProtoUpnpStatus(pProtoUpnp, 'idle', NULL, 0) == FALSE) ; )
    {
        // give time to DirtySock idle functions
        NetConnIdle();

        // update upnp module
        ProtoUpnpUpdate(pProtoUpnp);

        // see if current macro has completed
        if (ProtoUpnpStatus(pProtoUpnp, 'idle', NULL, 0) == TRUE)
        {
            *ppResponse = _UpnpTestAddResponse(pProtoUpnp, *ppResponse, (signed)(NetTick() - uElapsed));
        }
        else if ((NetTick() - uElapsed) > 30*1000)   // timed out?
        {
            // log the timeout error
            NetPrintf(("upnptest: timeout executing 'dprt' command\n"));
            *ppResponse = _UpnpTestAddResponse(pProtoUpnp, *ppResponse, -1);
            // abort current command
            ProtoUpnpControl(pProtoUpnp, 'abrt', 0, 0, NULL);
        }

        // spin wait cursor
        _UpnpTestWaitCursor();
    }
    
    return((ProtoUpnpStatus(pProtoUpnp, 'stat', NULL, 0) & PROTOUPNP_STATUS_DELPORTMAP) != 0);
}

static void _UpnpTestGetAccountName(char *pNameBuf, int iNameBufSize)
{
    strnzcpy(pNameBuf, "james", iNameBufSize);
}

static int32_t _UpnpTestPostLog(const char *pLogFile, int32_t iLogSize)
{
    int32_t iPosted, iResult = 1, iSent;
    
    // create http ref to post with
    ProtoHttpRefT *pProtoHttp = ProtoHttpCreate(4*1024);

    // set verbose mode
    ProtoHttpControl(pProtoHttp, 'spam', 1, 0, NULL);

    // post the file
    iPosted = ProtoHttpPost(pProtoHttp, "http://easo.stest.ea.com/easo/editorial/common/2006/upnplog/upnplog.jsp",
        pLogFile, iLogSize, PROTOHTTP_PUT);
    
    // wait for completion
    for ( ; iPosted < iLogSize; )
    {
        NetConnIdle();
        
        // post log to server
        if ((iSent = ProtoHttpSend(pProtoHttp, pLogFile+iPosted, iLogSize-iPosted)) < 0)
        {
            iResult = 0;
            break;
        }

        // increment buffer pointer
        iPosted += iSent;
        
        // update the module        
        ProtoHttpUpdate(pProtoHttp);
        
        // give it some time
        _UpnpTestWaitCursor();
    }

    return(iResult);
}

static int32_t _UpnpTestOutputXml(ProtoUpnpRefT *pProtoUpnp, const char *pXmlBuf, uint32_t bProbe0, uint32_t bAddMap, uint32_t bProbe1, uint32_t bDelMap)
{
    char *pLine;
    const char *pTagStart, *pTagEnd, *pTmpBuf;
    char strLine[256], strModel[256], strAcctName[17], strClientText[128];
    const char *_result[] = { "failure", "success", "partial" };
    char *pLog = _UpnpTest_strLogFile;
    int32_t iLevel, iIndent, iNewLevel;

    // print start tag
    ProtoUpnpStatus(pProtoUpnp, 'dnam', strModel, sizeof(strModel));
    pLog += _UpnpLogPrintf(pLog, "<upnpDevice name=\"%s\">\r\n", strModel);

    // try to get account name
    memset(strAcctName, 0, sizeof(strAcctName));
    _UpnpTestGetAccountName(strAcctName, sizeof(strAcctName));

    // print upnpClient tag
    snzprintf(strClientText, sizeof(strClientText), "user=\"%s\" addr=\"%a\" mac=\"%s\" vers=\"%s\" date=\"%s\"",
        strAcctName, NetConnStatus('addr', 0, NULL, 0), NetConnMAC(), UPNPTEST_VERSION, __DATE__);
    NetPrintf(("%s\n", strClientText));
    pLog += _UpnpLogPrintf(pLog, "\t<upnpClient %s/>\r\n", strClientText);

    // print probe result
    pLog += _UpnpLogPrintf(pLog, "\t<upnpResult probe0=\"%s\" addmap=\"%s\" probe1=\"%s\" delmap=\"%s\"/>\r\n",
        _result[bProbe0], _result[bAddMap], _result[bProbe1], _result[bDelMap]);

    // add xml to buffer
    for (iLevel = 1; *pXmlBuf != '\0'; )
    {
        // copy to line buffer until tag end, crlf, or eos
        pLine = strLine;
        pTagStart = (*pXmlBuf == '<') ? pXmlBuf+1 : NULL;
        pTagEnd = NULL;
        iNewLevel = iLevel;
        
        // check for close tag
        iNewLevel = ((*pXmlBuf == '<') && (*(pXmlBuf+1) == '/')) ? iLevel-2 : iLevel;
        while ((*pXmlBuf != '>') && (*pXmlBuf != '\r') && (*pXmlBuf != '\0'))
        {
            if ((*pXmlBuf == ' ') && (pTagEnd == NULL))
            {
                pTagEnd = pXmlBuf;
            }
            *pLine++ = *pXmlBuf++;
        }
        if (*pXmlBuf == '>')
        {
            if ((*(pXmlBuf-1) != '/') && (*(pXmlBuf-1) != '?'))
            {
                iNewLevel += 1;
            }
            if (pTagEnd == NULL)
            {
                pTagEnd = pXmlBuf;
            }
            *pLine++ = *pXmlBuf++;
        }
        // scan ahead and see if we have a matching end tag
        if (pTagStart != NULL)
        {
            pTmpBuf = pXmlBuf;
            while ((*pTmpBuf != '>') && (*pTmpBuf != '\r') && (*pTmpBuf != '\0'))
            {
                if ((*pTmpBuf == '<') && (*(pTmpBuf+1) == '/') && (!strncmp(pTagStart, pTmpBuf+2, pTagEnd-pTagStart)))
                {
                    while ((*pXmlBuf != '>') && (*pXmlBuf != '\r') && (*pXmlBuf != '\0'))
                    {
                        *pLine++ = *pXmlBuf++;
                    }
                    if (*pXmlBuf == '>')
                    {
                        *pLine++ = *pXmlBuf++;
                    }
                    iNewLevel--;
                    break;
                }
                pTmpBuf++;
            }
        }
        while ((*pXmlBuf == '\r') || (*pXmlBuf == '\n') || (*pXmlBuf == '\t') || (*pXmlBuf == ' '))
        {
            pXmlBuf++;
        }
        *pLine = '\0';
        
        // indent based on level
        if (iNewLevel < iLevel)
        {
            iLevel = iNewLevel;
        }
        for (iIndent = 0; iIndent < iLevel; iIndent++)
        {
            pLog += _UpnpLogPrintf(pLog, "\t");
        }
        iLevel = iNewLevel;
        
        // output the string
        pLog += _UpnpLogPrintf(pLog, "%s\r\n", strLine);
    }

    // add debug output
    pLog += _UpnpLogPrintf(pLog, "\t<upnpDebugOutput>\r\n");
    // copy debug output line by line
    for (pTmpBuf = _UpnpTest_strDebugTxt, pLine = strLine; *pTmpBuf != '\0'; pTmpBuf++)
    {
        if ((*pTmpBuf == '\r') || (*pTmpBuf == '\n'))
        {
            *pLine = '\0';
            pLog += _UpnpLogPrintf(pLog, "\t\t%s\r\n", strLine);
            pLine = strLine;
        }
        else
        {
            *pLine++ = *pTmpBuf;
        }
    }
    pLog += _UpnpLogPrintf(pLog, "\t</upnpDebugOutput>\r\n");    

    // print end tag
    pLog += _UpnpLogPrintf(pLog, "</upnpDevice>\r\n");

    // post log file to server
    return(_UpnpTestPostLog(_UpnpTest_strLogFile, pLog - _UpnpTest_strLogFile));
}

/*** Public Functions ******************************************************************/


int main(int32_t argc, char *argv[])
{
    PortTesterT *pPortTester;
    ProtoUpnpRefT *pProtoUpnp;
    char *pOutput = _UpnpTest_strOutput;
    uint32_t bAddMap, bDelMap, bPosted, iProbe0, iProbe1;

    // init and connect to network
    if (_UpnpTestInit() < 0)
    {
        NetPrintf(("upnptest: could not connect to network\n"));
        return(-1);
    }

    // create port tester
    if ((pPortTester = _UpnpTestCreatePortTester("plum.online.ea.com")) != NULL)
    {
        NetPrintf(("upnptest: created port tester\n"));
    }
    else
    {
        NetPrintf(("upnptest: failed to create port tester\n"));
    }

    // set debug output hook
    #if DIRTYCODE_LOGGING
    NetPrintfHook(_UpnpTestDebug, NULL);
    #endif

    // do port test
    iProbe0 = _UpnpTestPortTest(pPortTester);
    NetPrintf(("upnptest: starting upnp configuration\n"));

    // create protoupnp module
    pProtoUpnp = ProtoUpnpCreate();

    // configure ProtoUpnp
    ProtoUpnpControl(pProtoUpnp, 'port', 3658, 0, NULL);

    // add port mapping
    if ((bAddMap = _UpnpTestAddMapping(pProtoUpnp, &pOutput)) != 0)
    {
        NetPrintf(("upnptest: port mapping added\n"));
    }
    else
    {
        NetPrintf(("upnptest: failed to add port mapping\n"));
    }

    // do port test
    iProbe1 = _UpnpTestPortTest(pPortTester);

    // delete the mapping
    bDelMap = _UpnpTestDelMapping(pProtoUpnp, &pOutput);

    // clear debug output hook
    #if DIRTYCODE_LOGGING
    NetPrintfHook(NULL, NULL);
    #endif

    // post xml to server (and to debug output)
    if ((bPosted = _UpnpTestOutputXml(pProtoUpnp, _UpnpTest_strOutput, iProbe0, bAddMap, iProbe1, bDelMap)) != 0)
    {
        NetPrintf(("upnptest: log posted to server\n"));
    }
    else
    {
        NetPrintf(("upnptest: failed to post log to server\n"));
    }

    // done
    NetPrintf(("upnptest: done\n"));

    // wait forever
    for( ;  ; )
    {
        // give time to DirtySock idle functions
        NetConnIdle();

        // spin wait cursor
        _UpnpTestWaitCursor();
    }
    return(0);
}

void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    void *pMem;

    pMem = MEMAllocFromExpHeapEx(_UPnPTest_hHeap, (uint32_t)iSize, 32);
    if (iMemModule != 'nsoc')
    {
        DirtyMemDebugAlloc(pMem, iSize, iMemModule, iMemGroup, pMemGroupUserData);
    }

    return(pMem);
}

void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    MEMFreeToExpHeap(_UPnPTest_hHeap, pMem);
    if (iMemModule != 'nsoc')
    {
        DirtyMemDebugFree(pMem, 0, iMemModule, iMemGroup, pMemGroupUserData);
    }
}
