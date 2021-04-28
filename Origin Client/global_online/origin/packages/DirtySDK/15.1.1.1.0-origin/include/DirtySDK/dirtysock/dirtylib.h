/*H**************************************************************************************/
/*!
    \File    dirtylib.h

    \Description
        Provide basic library functions for use by network layer code.
        This is needed because the network code is platform/project
        independent and needs to rely on a certain set of basic
        functions.

    \Copyright
        Copyright (c) Electronic Arts 2001-2002

    \Version    0.5        08/01/01 (GWS) First Version
    \Version    1.0        12/31/01 (GWS) Redesigned for Tiburon environment
*/
/**************************************************************************************H*/

#ifndef _dirtylib_h
#define _dirtylib_h

/*!
    \Moduledef DirtySock DirtySock

    \Description
        The Dirtysock module provides a portable sockets implementation.

        <b>Overview</b>

            Dirtysock is a platform independent API providing access to common network library
            functions. It it similar in many respects to BSD/Sockets and Winsock, but targeted
            specifically for a real-time game networking environment.  It will be helpful to have
            an understanding of sockets before reading this documentation.

            Unlike BSD and Windows sockets, only the AF_INET address family is supported, and blocking
            read and write calls are not supported.  Instead, the Dirtysock API supports two
            extensions not present in a normal socket environment that allow user-defined callbacks
            and idle functions.  The SocketCallback() function allows you to register a function
            that gets called when an incoming packet arrives on a particular socket. This can be
            used to process incoming packets in essentially real-time. If the \em idle parameter
            of SocketCallback() is nonzero, the registered function will be called approximately
            every \em idle milliseconds. This can be used to deal with resends and things of
            that nature.  Callback and idle functions are called by a private Dirtysock thread.

        <b>Module Dependency Graph</b>

            <img alt="" src="dirtysock.png">

        <b>Initializing DirtySock</b>

            \code

            #include "DirtySDK/dirtysock.h"

            // initialize DirtySock
            SocketCreate();

            \endcode

        <b>BSD Socket Function Equivalency</b>

        The following is a table of BSD socket functions with their Dirtysock equivalents:

                BSD function | Dirtysock function    | Notes
                ------------ | --------------------- | -----
                accept       | SocketAccept()        |  -
                bind         | SocketBind()          |  -
                closesocket  | SocketClose()         |  -
                connect      | SocketConnect()       |  -
                getpeername  | SocketInfo()          |  Call with info = 'peer'
                getsockname  | SocketInfo()          |  Call with info = 'bind'
                getsockopt   | No equivalent         |  -
                htonl        | SocketHtonl()         |  -
                htons        | SocketHtons()         |  -
                inet_addr    | SocketInTextGetAddr   |  -
                inet_ntoa    | SocketInAddrGetText   |  -
                ioctlsocket  | No equivalent         |  -
                listen       | SocketListen()        |  -
                ntohl        | SocketNtohl()         |  -
                ntohs        | SocketNtohs()         |  -
                recv         | SocketRecv()          |  -
                recvfrom     | SocketRecvfrom()      |  -
                select       | No direct equivalent  |  Calling SocketInfo() with info = 'stat' returns connection status
                send         | SocketSend()          |  -
                sendto       | SocketSendto()        |  -
                setsockopt   | No equivalent         |  -
                shutdown     | SocketShutdown()      |  -
                socket       | SocketOpen()          |  -

        <b>Examples</b>

            <em>Create a new stream socket connection</em>

            \code

            struct sockaddr peeraddr;

            pSock = SocketOpen(AF_INET, SOCK_STREAM, 0);
            if (pSock != NULL)
            {
                // init sockaddr structure
                SockaddrInit(&peeraddr, AF_INET);
                SockaddrInSetAddr(&peeraddr, iAddr);
                SockaddrInSetPort(&peeraddr, iPort);

                // start the connect
                SocketConnect(pSock, (struct sockaddr *)&peeraddr, sizeof(peeraddr));
            }

            \endcode

            <em>Check for connection ready</em>

            \code
            // note: as connection resolution is asynchronous, SocketInfo() is usually
            // called repeatedly until either the connection is ready or a given timeout occurs
            if (SocketInfo(pSock, 'stat', NULL, 0) > 0)
            {
                NetPrint("Connection ready\n");
            }

            \endcode

            <em>Send data on a stream socket</em>

            \code

            char data[] = "Data to send to remote host";
            SocketSend(pSock, data, sizeof(data), 0);

            \endcode

            <em>Receive data from a datagram socket</em>

            \code

            char data[256];
            struct sockaddr from;
            int32_t len, fromlen;

            if ((len = SocketRecvfrom(pSock, data, sizeof(data), 0, &from, &fromlen)) > 0)
            {
                NetPrint("Received %d bytes from socket\n",len);
            }

            \endcode
*/

/*!
\Moduledef DirtyLib DirtyLib
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

// define platform-specific options
#if defined(DIRTYCODE_PS3)
 #define CRIT_SECT_LEN (24)
#elif defined(DIRTYCODE_PS4)
 #define CRIT_SECT_LEN (64) //$$TODO - this needs to be evaluated
#elif defined(DIRTYCODE_PC) || defined(DIRTYCODE_WINRT) || defined(DIRTYCODE_WINPRT)
 #if defined(_WIN64)
  #define CRIT_SECT_LEN (56)
 #else
  #define CRIT_SECT_LEN (36)
 #endif
#elif defined(DIRTYCODE_XBOXONE)
#define CRIT_SECT_LEN  (56) //$$TODO - this needs to be evaluated
#elif defined(DIRTYCODE_XENON)
 #define CRIT_SECT_LEN (40)
#elif defined(DIRTYCODE_ANDROID) || defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEOSX)
 #if DIRTYCODE_64BITPTR
  #define CRIT_SECT_LEN (96)
 #else
  #define CRIT_SECT_LEN (72)
 #endif
#else
 #error critical section length must be defined!
#endif

// if not already defined, define NULL
#ifndef NULL
 #ifdef __cplusplus
  #define NULL 0
 #else
  #define NULL ((void *)0)
 #endif
#endif

#ifndef DIRTYCODE_LOGGING
 #if DIRTYCODE_DEBUG
  //in debug mode logging is defaulted to on
  #define DIRTYCODE_LOGGING (1)
 #else
  //if its not specified then turn it off
  #define DIRTYCODE_LOGGING (0)
 #endif
#endif

// debug printing routines
#if DIRTYCODE_LOGGING
 #define NetPrintf(_x) NetPrintfCode _x
 #define NetPrintfVerbose(_x) NetPrintfVerboseCode _x
 #define NetPrintArray(_pMem, _iSize, _pTitle) NetPrintArrayCode(_pMem, _iSize, _pTitle)
 #define NetPrintMem(_pMem, _iSize, _pTitle) NetPrintMemCode(_pMem, _iSize, _pTitle)
 #define NetPrintWrap(_pString, _iWrapCol) NetPrintWrapCode(_pString, _iWrapCol)
#else
 #define NetPrintf(_x) { }
 #define NetPrintfVerbose(_x) { }
 #define NetPrintArray(_pMem, _iSize, _pTitle) { }
 #define NetPrintMem(_pMem, _iSize, _pTitle) { }
 #define NetPrintWrap(_pString, _iWrapCol) { }
 #define NetTimeStampEnable(_bEnableTimeStamp) { }
#endif

// global module shutdown flags
#define NET_SHUTDOWN_NETACTIVE    (1)   //!< leave network active in preparation for launching to account management (Xbox 360 only)
#define NET_SHUTDOWN_THREADSTARVE (2)   //!< special shutdown mode for PS3 that starves threads, allowing for quick exit to XMB

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! critical section definition
typedef struct NetCritT
{
    int32_t data[(CRIT_SECT_LEN+3)/4];  // force int32_t alignment
} NetCritT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 Portable routines implemented in dirtynet.c
*/

// reset net idle list
DIRTYCODE_API void NetIdleReset(void);

// remove a function to the idle callback list.
DIRTYCODE_API void NetIdleAdd(void (*proc)(void *ref), void *ref);

// call all the functions in the idle list.
DIRTYCODE_API void NetIdleDel(void (*proc)(void *ref), void *ref);

// make sure all idle calls have completed
DIRTYCODE_API void NetIdleDone(void);

// add a function to the idle callback list
DIRTYCODE_API void NetIdleCall(void);

// print memory as a c-style array (do not call directly; use NetPrintArray() wrapper)
DIRTYCODE_API void NetPrintArrayCode(const void *pMem, int32_t iSize, const char *pTitle);

// print memory as hex (do not call directly; use NetPrintMem() wrapper)
DIRTYCODE_API void NetPrintMemCode(const void *pMem, int32_t iSize, const char *pTitle);

// print input buffer with wrapping (do not call directly; use NetPrintWrap() wrapper)
DIRTYCODE_API void NetPrintWrapCode(const char *pData, int32_t iWrapCol);

/*
 Platform-specific routines implemented in dirtynet<platform>.c
*/

// initialize the network library functions.
DIRTYCODE_API void NetLibCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity);

// shutdown the network library.
DIRTYCODE_API void NetLibDestroy(uint32_t uShutdownFlags);

#if defined(DIRTYCODE_WINRT)
// suspends the execution of the current thread until the time-out interval elapses
DIRTYCODE_API void NetSleep(int32_t iMilliSecs);
#endif

// return an increasing tick count with millisecond scale
DIRTYCODE_API uint32_t NetTick(void);

// return microsecond timer, intended for debug timing purposes only
DIRTYCODE_API uint64_t NetTickUsec(void);

// return signed difference between new tick count and old tick count (new - old)
#define NetTickDiff(_uNewTime, _uOldTime) ((signed)((_uNewTime) - (_uOldTime)))

// return 32-bit hash from given input string
DIRTYCODE_API int32_t NetHash(const char *pString);

// A simple psuedo-random sequence generator
DIRTYCODE_API uint32_t NetRand(uint32_t uLimit);

// return 32-bit hash from given buffer
DIRTYCODE_API int32_t NetHashBin(const void *pBuffer, uint32_t uLength);

// diagnostic output routine (do not call directly, use NetPrintf() wrapper
DIRTYCODE_API int32_t NetPrintfCode(const char *fmt, ...);

// diagnostic output routine (do not call directly, use NetPrintf() wrapper
DIRTYCODE_API void NetPrintfVerboseCode(int32_t iVerbosityLevel, int32_t iCheckLevel, const char *pFormat, ...);

// hook into debug output
#if DIRTYCODE_LOGGING
DIRTYCODE_API void NetPrintfHook(int32_t (*pPrintfDebugHook)(void *pParm, const char *pText), void *pParm);

// provide time stamp for logging
DIRTYCODE_API int32_t NetTimeStamp(char *pBuffer, int32_t iLen);

// enable logging time stamp
DIRTYCODE_API void NetTimeStampEnable(uint8_t bEnableTimeStamp);
#endif

// initialize a critical section for use -- includes name for verbose debugging on some platforms
DIRTYCODE_API void NetCritInit(NetCritT *pCrit, const char *pCritName);

// release resources and destroy critical section
DIRTYCODE_API void NetCritKill(NetCritT *pCrit);

// attempt to gain access to critical section
DIRTYCODE_API int32_t NetCritTry(NetCritT *pCrit);

// enter a critical section, blocking if needed
DIRTYCODE_API void NetCritEnter(NetCritT *pCrit);

// leave a critical section
DIRTYCODE_API void NetCritLeave(NetCritT *pCrit);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtylib_h

