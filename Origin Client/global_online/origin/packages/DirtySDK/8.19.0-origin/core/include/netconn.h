/*H*************************************************************************************/
/*!
    \File    netconn.h
    
    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.
        
    \Copyright
        Copyright (c) Electronic Arts 2001-2009.  ALL RIGHTS RESERVED.
    
    \Version 03/12/2001 (gschaefer) First Version
*/
/*************************************************************************************H*/

#ifndef _netconn_h
#define _netconn_h

/*!
    \Moduledef NetConn NetConn

    \Description

        NetConn provides routines to start up and query the network interface, as well as
        encapsulating various low-level platform-specific functionality.
*/
//@{

/*** Include files *********************************************************************/

#include "platform.h"
#include "netconndefs.h"

/*** Defines ***************************************************************************/

#if defined(DIRTYCODE_XENON)
//! maximum number of local users
#define NETCONN_MAXLOCALUSERS    (4)
#endif

/*** Macros ****************************************************************************/

//! macro to construct parameter for netconn 'loss' control selector, used to simulate packet loss
#define NETCONN_PACKETLOSSPARAM(_uFrequency, _uDuration, _bVerbose) ((uint32_t)((_uFrequency&0xff) << 8) | (_uDuration&0xff) | ((_bVerbose & 0x1) << 31))

/*** Type Definitions ******************************************************************/

typedef struct NetConnUserDataT
{
    char    strName[16];
    void    *pRawData1;
    void    *pRawData2;           
} NetConnUserDataT;

//! ticketing event callback function prototype
#if defined(DIRTYCODE_PS3)
typedef void (NetConnTicketCallbackT)(int32_t iResult, void *pUserData);
#elif defined(DIRTYCODE_PSP2)
typedef void (NetConnTicketCallbackT)(int32_t /* SceNpAuthRequestId */ iId, int32_t iResult, void *pUserData);
#endif

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// bring the network connection module to life
int32_t NetConnStartup(const char *pParams);

// query the list of available connection configurations
int32_t NetConnQuery(const char *pDevice, NetConfigRecT *pList, int32_t iSize);

// bring the networking online with a specific configuration
int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pParms, int32_t iData);

// On platforms that support/require it, bring up the sign-in dialog for another user
int32_t NetConnConnectSub(uint32_t uUserId);

// On platforms that support/require it, disconnect another previously signed-in user
int32_t NetConnDisconnectSub(void *pNpId);

// set module behavior based on input selector
int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2);

// check general network connection status (added param)
int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize);

// return MAC address in textual form
const char *NetConnMAC(void);

// take down the network connection
int32_t NetConnDisconnect(void);

// shutdown the network code and return to idle state
int32_t NetConnShutdown(uint32_t uShutdownFlags);

// return elapsed time in milliseconds
uint32_t NetConnElapsed(void);

// sleep the application (burn cycles) for some number of milliseconds
void NetConnSleep(int32_t iMilliSecs);

// add an idle handler that will get called periodically
int32_t NetConnIdleAdd(void (*proc)(void *data, uint32_t tick), void *data);

// remove a previously added idle handler
int32_t NetConnIdleDel(void (*proc)(void *data, uint32_t tick), void *data);

// provide "life" to the network code
void NetConnIdle(void);

// shut down netconn idle handler
// NOTE: this is meant for internal use only, and should not be called by applications directly
void NetConnIdleShutdown(void);

// Enable or disable the timing of netconnidles
void NetConnTiming(uint8_t uEnableTiming);

#if defined(DIRTYCODE_PS3) || defined(DIRTYCODE_PSP2)
int32_t NetConnRequestTicket(uint32_t uTicketVersionMajor, uint32_t uTicketVersionMinor, const char *pServiceId, const char *pCookie,
    size_t iCookieSize, const char *pEntitlementId, uint32_t uConsumedCount, NetConnTicketCallbackT *pCallback, void *pUserData);
#endif

typedef void (UserInfoCallbackT)(NetConnUserDataT *pUserDataT, void *pData);

#if DIRTYCODE_LOGGING
void NetConnMonitorValue(const char* pName, int32_t iValue);
#endif

#ifdef __cplusplus
}
#endif

//@}

#endif // _netconn_h
