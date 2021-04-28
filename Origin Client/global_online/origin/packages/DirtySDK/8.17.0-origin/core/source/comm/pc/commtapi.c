/*H*************************************************************************************************/
/*!

    \File    commtapi.c

    \Description
        Modem comm driver

    \Notes
        The TAPI module relies on the serial module for all of its actual
        packet communication needs.  This code uses TAPI to establish the
        connection, but passes send/peek/recv calls through.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    1.0        02/19/99 (GWS) First Version
    \Version    1.1        02/25/99 (GWS) Alpha Release
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#define TAPI_CURRENT_VERSION 0x00010004
#include <windows.h>
#include <tapi.h>

#include "dirtysock.h"
#include "dirtylib.h"
#include "commall.h"
#include "commser.h"
#include "commtapi.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! private module storage
typedef struct CommTAPIRef
{
	//! common header for all comm modules
	CommRef common;
	//! pointer to serial driver
	CommSerRef *serial;
	//! index of current active tapi device
	DWORD device;
	//! total number of devices
	DWORD devcount;
	//! tapi session reference
	HLINEAPP tapiref;
	//! active line reference
	HLINE lineref;
	//! current call reference
	HCALL callref;
	//! module state
	volatile enum { INIT, DEAD, IDLE, CONN, LIST, OPEN, RECONN, RELIST, CLOSE, QUIT } state;
	//! call attempt identifier (for callback identification)
	LONG callid;
	//! address being called (null for listen)
	char address[256];
    //! remember thread identifier for shutdown
    DWORD threadid;
} CommTAPIRef;

/*** Function Prototypes ***************************************************************/

int32_t CommSerRecover(CommSerRef *ref, const char *addr);

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    lineOpen2
    
    \Description
        Simplified wrapper for lineOpen
        
    \Input appl     - appl
    \Input devid    - devid
    \Input line     - line
    \Input refnum   - refnum
    \Input priv     - priv
    \Input mode     - mode
    
    \Output
        LONG        - return result from lineOpen
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static LONG lineOpen2(HLINEAPP appl, DWORD devid, LPHLINE line, DWORD_PTR refnum, DWORD priv, DWORD mode)
{
	LONG result;
	DWORD version = 0;
	LINEEXTENSIONID extend;

	// figure out the matching version
	result = lineNegotiateAPIVersion(appl, devid, TAPI_CURRENT_VERSION, 0x00020002, &version, &extend);
	if (result != 0)
		return(result);
	// do the open
	return(lineOpen(appl, devid, line, version, 0, refnum, priv, mode, 0));
}

/*F*************************************************************************************************/
/*!
    \Function    lineClose2
    
    \Description
        Simplified wrapper for lineClose
        
    \Input line     - line
    
    \Output
        LONG        - output from lineClose, or LINEERR_INVALLINEHANDLE
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static LONG lineClose2(HLINE line)
{
	return(line != (HLINE)-1 ? lineClose(line) : LINEERR_INVALLINEHANDLE);
}

/*F*************************************************************************************************/
/*!
    \Function    lineGetHandle
    
    \Description
        Simplified wrapper for lineGetId
        
    \Input line     - line
    
    \Output
        LONG        - line handle
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static HANDLE lineGetHandle(HLINE line)
{
    LONG result;
#if defined(_WIN64)
    uint64_t tempHandle = (uint64_t)-1;
#else
    uint32_t tempHandle = (uint32_t)-1;
#endif
    struct {
        VARSTRING var;
        HANDLE hand;
        char name[256];
    } info;

    // get the comm port info
    memset(&info, 0, sizeof(info));
    info.var.dwTotalSize = sizeof(info);
    result = lineGetID(line, 0, (HCALL)0, LINECALLSELECT_LINE, &info.var, "comm/datamodem");
    return(result != 0 ? (HANDLE)tempHandle : info.hand);
}

/*F*************************************************************************************************/
/*!
    \Function    lineMakeCall2
    
    \Description
        Simplified wrapper for lineMakeCall
        
    \Input line     - line
    \Input call     - call
    \Input addr     - addr
    \Input ctry     - ctry
    
    \Output
        LONG        - result of linkeMakeCall
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static LONG lineMakeCall2(HLINE line, LPHCALL call, LPCSTR addr, DWORD ctry)
{
	struct {
		LINECALLPARAMS head;
		char body[1024];
	} dial;

	// setup to dial
	memset(&dial, 0, sizeof(dial));
	dial.head.dwTotalSize = sizeof(dial);

	// This is where we configure the line for DATAMODEM usage.
	dial.head.dwBearerMode = LINEBEARERMODE_VOICE;
	dial.head.dwMediaMode  = LINEMEDIAMODE_DATAMODEM;

	// This specifies that we want to use only IDLE calls and
	// don't want to cut into a call that might not be IDLE (ie, in use).
	dial.head.dwCallParamFlags = LINECALLPARAMFLAGS_IDLE;
                                
	// if there are multiple addresses on line, use first anyway.
	// It will take a more complex application than a simple tty app
	// to use multiple addresses on a line anyway.
	dial.head.dwAddressMode = LINEADDRESSMODE_ADDRESSID;
	dial.head.dwAddressID = 0;

	// Address we are dialing.
	dial.head.dwDisplayableAddressOffset = sizeof(dial.head);
	strcpy(dial.body, addr);
	dial.head.dwDisplayableAddressSize = (DWORD)strlen(dial.body);

	// initiate the calling sequence
	return(lineMakeCall(line, call, dial.body, ctry, &dial.head));
}

/*F*************************************************************************************************/
/*!
    \Function    _CommTAPICallback

    \Description
        Process TAPI callback messages

    \Input device   - device
    \Input msg      - msg
    \Input _ref     - CommTAPI module ref
    \Input parm1    - parm1
    \Input parm2    - parm2
    \Input parm3    - parm3

    \Output
        None.

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static VOID PASCAL _CommTAPICallback(DWORD device, DWORD msg, DWORD_PTR _ref, DWORD_PTR parm1, DWORD_PTR parm2, DWORD_PTR parm3)
{
    LONG result;
    CommTAPIRef *ref = (CommTAPIRef *) _ref;
#if defined(_WIN64)
    uint64_t tempHandle = (uint64_t)-1;
#else
    uint32_t tempHandle = (uint32_t)-1;
#endif

	// process a disconnect
	if ((msg == LINE_CALLSTATE) && (parm1 == LINECALLSTATE_DISCONNECTED)) {

		// setup for new listen after failed answer attempt
		if (ref->state == LIST) {
			// close any old line
			lineClose(ref->lineref);
			ref->lineref = (HLINE)-1;
			// open the device for use
			result = lineOpen2(ref->tapiref, ref->device, &ref->lineref, (DWORD_PTR)ref, LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_DATAMODEM);
			return;
		}

		// redial after failed dial attempt
		if (ref->state == CONN) {
			// close any old line
			lineClose(ref->lineref);
			ref->lineref = (HLINE)-1;
			// open the line
			result = lineOpen2(ref->tapiref, ref->device, &ref->lineref, (DWORD_PTR)ref, LINECALLPRIVILEGE_NONE, LINEMEDIAMODE_DATAMODEM);
			if (result != 0) {
				ref->state = CLOSE;
				return;
			}
			// initiate a dial attempt
			ref->callid = lineMakeCall2(ref->lineref, &ref->callref, ref->address, 0);
			if (ref->callid <= 0) {
				lineClose(ref->lineref);
				ref->lineref = (HLINE)-1;
				ref->state = CLOSE;
				return;
			}
		}

		// handle listen recovery
		if ((ref->state == OPEN) && (CommSerStatus(ref->serial) == COMM_ONLINE) && 
			(ref->address[0] == 0)) {
			// put into reconnect mode
			ref->state = RELIST;
			// tell serial driver to go into hold mode
			CommSerRecover(ref->serial, NULL);
			// kill modem port
			lineClose(ref->lineref);
			ref->lineref = (HLINE)-1;
			// start listening for a ring
			result = lineOpen2(ref->tapiref, ref->device, &ref->lineref, (DWORD_PTR)ref, LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_DATAMODEM);
			if (result != 0)
				ref->state = CLOSE;
			return;
		}

		// handle connect recovery
		if ((ref->state == OPEN) && (CommSerStatus(ref->serial) == COMM_ONLINE)
			&& (ref->address[0] != 0)) {
			// put into reconnect mode
			ref->state = RECONN;
			// tell serial driver to go into hold mode
			CommSerRecover(ref->serial, NULL);
			// kill modem port
			lineClose(ref->lineref);
			ref->lineref = (HLINE)-1;

			// reopen the line
			result = lineOpen2(ref->tapiref, ref->device, &ref->lineref, (DWORD_PTR)ref, LINECALLPRIVILEGE_NONE, LINEMEDIAMODE_DATAMODEM);
			if (result != 0) {
				ref->state = CLOSE;
				return;
			}
			// initiate a new dial attempt
			ref->callid = lineMakeCall2(ref->lineref, &ref->callref, ref->address, 0);
			if (ref->callid <= 0) {
				lineClose(ref->lineref);
				ref->lineref = (HLINE)-1;
				ref->state = CLOSE;
				return;
			}
			return;
		}
	}

	// process a ring
	if ((msg == LINE_CALLSTATE) && (parm1 == LINECALLSTATE_OFFERING)) {
		// only care in listen mode
		if ((ref->state == LIST) || (ref->state == RELIST)) {
			ref->callref = (HLINE)device;
			ref->callid = lineAnswer(ref->callref, NULL, 0);
			if (ref->callid == 0) {
				parm1 = LINECALLSTATE_CONNECTED;
			}
		}
	}

	// process a connect
	if ((msg == LINE_CALLSTATE) && (parm1 == LINECALLSTATE_CONNECTED)) {

		// handle normal first-time connect
		if ((ref->state == LIST) || (ref->state == CONN)) {
			char addr[256];
			HANDLE hand = lineGetHandle(ref->lineref);
			// make sure handle is valid
			if (hand == (HANDLE)tempHandle) {
				lineClose2(ref->lineref);
				ref->lineref = (HLINE)-1;
				ref->state = CLOSE;
				return;
			}
			// start the data flow
			wsprintf(addr, "TAPI%d:", hand);
			if (ref->state == CONN)
				CommSerConnect(ref->serial, addr);
			else
				CommSerListen(ref->serial, addr);
			ref->state = OPEN;
			return;
		}

		// handle a reconnect
		if ((ref->state == RELIST) || (ref->state == RECONN)) {
			char addr[256];
			HANDLE hand = lineGetHandle(ref->lineref);
			// make sure handle is valid
			if (hand == (HANDLE)tempHandle) {
				lineClose2(ref->lineref);
				ref->lineref = (HLINE)-1;
				ref->state = CLOSE;
				return;
			}
			// recover the data flow
			wsprintf(addr, "TAPI%d:", hand);
			CommSerRecover(ref->serial, addr);
			ref->state = OPEN;
			return;
		}
	}
}


/*F*************************************************************************************************/
/*!
    \Function    CommTAPIThread

    \Description
        Private thread with message queue (TAPI needs one

    \Input ref      - module ref

    \Output
        int32_t     - zero

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t CommTAPIThread(CommTAPIRef *ref)
{
    MSG msg;
    int32_t result;

    // create the message queue
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

    // attempt to initialize tapi
    result = lineInitialize(&ref->tapiref, GetModuleHandle(NULL), &_CommTAPICallback,
        NULL, &ref->devcount);
    if (ref->devcount == 0) {
        ref->state = DEAD;
        return(0);
    }

    // signal we are alive
    ref->state = IDLE;

    // bogus message loop for wsaasync
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // signal we are dead
    ref->state = DEAD;
    return(0);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    CommTAPIConstruct
    
    \Description
        Construct the class
        
    \Input maxwid   - max record width
    \Input maxinp   - input packet buffer size
    \Input maxout   - output packet buffer size
    
    \Output
        int32_t         - zero
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
CommTAPIRef *CommTAPIConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout)
{
    HANDLE thread;

	// create class storage
	CommTAPIRef *ref = HeapAlloc(GetProcessHeap(), 0, sizeof(*ref));
	if (ref == NULL)
		return(NULL);
	memset(ref, 0, sizeof(*ref));

	// initialize the callback routines
	ref->common.Construct = (CommAllConstructT *)CommTAPIConstruct;
	ref->common.Destroy = (CommAllDestroyT *)CommTAPIDestroy;
	ref->common.Resolve = (CommAllResolveT *)CommTAPIResolve;
	ref->common.Unresolve = (CommAllUnresolveT *)CommTAPIUnresolve;
	ref->common.Listen = (CommAllListenT *)CommTAPIListen;
	ref->common.Unlisten = (CommAllUnlistenT *)CommTAPIUnlisten;
	ref->common.Connect = (CommAllConnectT *)CommTAPIConnect;
	ref->common.Unconnect = (CommAllUnconnectT *)CommTAPIUnconnect;
    ref->common.Callback = (CommAllCallbackT *)CommTAPICallback;
	ref->common.Status = (CommAllStatusT *)CommTAPIStatus;
    ref->common.Tick = (CommAllTickT *)CommTAPITick;
	ref->common.Send = (CommAllSendT *)CommTAPISend;
	ref->common.Peek = (CommAllPeekT *)CommTAPIPeek;
	ref->common.Recv = (CommAllRecvT *)CommTAPIRecv;

    // remember max packet width
    ref->common.maxwid = maxwid;
    ref->common.maxinp = maxinp;
    ref->common.maxout = maxout;

	// create a tapi message thread
    ref->state = INIT;
	thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&CommTAPIThread, (LPVOID)ref, 0, &ref->threadid);
	if (thread == NULL) {
		// pretty darn ugly
	}
	CloseHandle(thread);

    // make sure thread is running
    while (ref->state == INIT)
        Sleep(0);
    if (ref->state == DEAD) {
		HeapFree(GetProcessHeap(), 0, ref);
		return(NULL);
    }

	// get serial driver ready
	ref->serial = CommSerConstruct(maxwid, maxinp, maxout);

	// init the handles
	ref->lineref = (HLINE)-1;
	ref->callref = (HLINE)-1;

	// init our state
	ref->state = IDLE;
	return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIDestruct
    
    \Description
        Destruct the class
        
    \Input *ref     - module ref
    
    \Output
        None.
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
void CommTAPIDestroy(CommTAPIRef *ref)
{
    // get rid of serial port
	CommSerDestroy(ref->serial);
    // done with tapi
	lineShutdown(ref->tapiref);
    // kill the message thread
    ref->state = QUIT;
    PostThreadMessage(ref->threadid, WM_QUIT, 0, 0);
    while (ref->state == QUIT)
        Sleep(0);
    // release our memory
	HeapFree(GetProcessHeap(), 0, ref);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPICallback

    \Description
        Set upper layer callback

    \Input *ref         - reference pointer
    \Input *callback    - socket generating callback

    \Output
        None.

    \Version    1.0        03/10/03 (JLB) Copied from CommSrpCallback()
*/
/*************************************************************************************************F*/
void CommTAPICallback(CommTAPIRef *ref, void (*callback)(CommRef *ref, int32_t event))
{
    //ref->callproc = callback;
    //ref->gotevent |= 2;
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIResolve
    
    \Description
        Resolve an address
        
    \Input *ref     - module ref
    \Input *addr    - resolve address
    \Input *buf     - target buffer
    \Input len      - target length (min 64 bytes)
    \Input div      - divider char
    
    \Output
        int32_t         - <0=error, 0=complete (COMM_NOERROR), >0=in progress (COMM_PENDING)

    \Notes
        Target list is always double null terminated allowing null
        to be used as the divider character if desired. when COMM_PENDING
        is returned, target buffer is set to "~" until completion.
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIResolve(CommTAPIRef *ref, const char *addr, char *buf, int32_t len, char div)
{
	char *s;
	DWORD devid;
	LONG result;
	DWORD version;
	LINEEXTENSIONID extend;
	union {
		LINEDEVCAPS parm;
		char data[1024];
	} caps;
	char *org = buf;

	// default to error
	buf[0] = '*';
	buf[1] = 0;
	buf[2] = 0;

	// handle null request special
	if ((addr == NULL) || (addr[0] == 0))
		return(COMM_BADPARM);

	// sanity buffer check
	if ((len < 64) || ((unsigned)len < strlen(addr)+2))
		return(COMM_MINBUFFER);

	// walk the device list
	for (devid = 0; devid < ref->devcount; ++devid) {
		// negotiate compatible version with device
		result = lineNegotiateAPIVersion(ref->tapiref, devid, TAPI_CURRENT_VERSION, 0x00020002,
			&version, &extend);
		if (result != 0)
			continue;
		// get the capabilities (the name)
		memset(&caps, 0, sizeof(caps));
		caps.parm.dwTotalSize = sizeof(caps);
		result = lineGetDevCaps(ref->tapiref, devid, version, 0, &caps.parm);
        // filter out non-modem devices
        if (caps.parm.dwRingModes == 0)
            continue;
		// get pointer to device name
		s = &caps.data[caps.parm.dwLineNameOffset];
		// see if we are resolving or creating localhost list
		if (strcmp(addr, "localhost") == 0) {
			// check for buffer overflow
			if (strlen(s)+2 > (unsigned)len)
				return(COMM_MINBUFFER);
			// add item to buffer
			if (buf != org)
				*buf++ = div, --len;
			strcpy(buf, s);
			buf += strlen(s);
			len -= (int32_t)strlen(s);
		} else {
			// see if this is the matching item
			if (stricmp(addr, s) == 0) {
				wsprintf(buf, "%d%c", devid, 0);
				return(1);
			}
		}
	}

	// complete
	*buf++ = 0;
	*buf++ = 0;
	return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIResolve
    
    \Description
        Stop the resolver
        
    \Input *ref     - module ref
    
    \Output
        None

    \Notes
        Target list is always double null terminated allowing null
        to be used as the divider character if desired. when COMM_PENDING
        is returned, target buffer is set to "~" until completion.
            
    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
void CommTAPIUnresolve(CommTAPIRef *ref)
{
	return;
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIListen
    
    \Description
        Listen for a connection
        
    \Input *ref     - module ref
    \Input *addr    - port to listen on (only :port portion used)
    
    \Output
        int32_t         - negative=error, zero=ok

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIListen(CommTAPIRef *ref, const char *addr)
{
	char resolve[64];

	// make sure in right state
	if (ref->state != IDLE)
		return(COMM_BADSTATE);

	// see if we should auto-resolve the name
	// (we do this on tapi listen because it is only proto which needs it)
	if ((addr[0] < '0') || (addr[0] > '9')) {
		CommTAPIResolve(ref, addr, resolve, sizeof(resolve), 0);
		addr = resolve;
	}
	
	// get device index
	for (ref->device = 0; (*addr >= '0') && (*addr <= '9'); ++addr)
		ref->device = (ref->device * 10) + (*addr & 15);

	// null address indicates listen
	ref->address[0] = 0;

	// should be looking for a ring
	ref->state = LIST;

	// send a bogus disconnect notice to callback
	_CommTAPICallback(ref->device, LINE_CALLSTATE, (DWORD_PTR)ref, LINECALLSTATE_DISCONNECTED, 0, 0);
	return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIUnlisten
    
    \Description
        Stop listening
        
    \Input *ref     - module ref
    
    \Output
        int32_t         - negative=error, zero=ok

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIUnlisten(CommTAPIRef *ref)
{
	return(CommSerUnlisten(ref->serial));
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIConnect
    
    \Description
        Initiate a connection to a peer
        
    \Input *ref     - module ref
    \Input *addr    - address in ip-address:port form
    
    \Output
        int32_t         - negative=error, zero=ok

    \Notes
        Does not currently perform dns translation

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIConnect(CommTAPIRef *ref, const char *addr)
{
	// make sure in right state
	if (ref->state != IDLE)
		return(COMM_BADSTATE);
	
	// figure out which device to use
	for (ref->device = 0; (*addr >= '0') && (*addr <= '9'); ++addr)
		ref->device = (ref->device * 10) + (*addr & 15);
	addr = strchr(addr, ':');
	if (addr == NULL)
		return(COMM_BADADDRESS);
	strcpy(ref->address, addr+1);

	// want to connect
	ref->state = CONN;

	// send a bogus disconnect notice to callback
	_CommTAPICallback(ref->device, LINE_CALLSTATE, (DWORD_PTR)ref, LINECALLSTATE_DISCONNECTED, 0, 0);
	return(0);
}	

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIUnconnect
    
    \Description
        Terminate a connection
        
    \Input *ref     - module ref
    
    \Output
        int32_t         - negative=error, zero=ok

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIUnconnect(CommTAPIRef *ref)
{
	if ((ref->state == OPEN) || (ref->state == CLOSE))
		CommSerUnconnect(ref->serial);
	if ((ref->state == CONN) || (ref->state == OPEN) || (ref->state == CLOSE))
		lineClose(ref->lineref);
	return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIStatus
    
    \Description
        Return current stream status
        
    \Input *ref     - module ref
    
    \Output
        int32_t         - COMM_CONNECTING, COMM_OFFLINE, COMM_ONLINE or COMM_FAILURE

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIStatus(CommTAPIRef *ref)
{
	if ((ref->state == CONN) || (ref->state == LIST))
		return(COMM_CONNECTING);
	if ((ref->state == IDLE) || (ref->state == CLOSE))
		return(COMM_OFFLINE);
	if ((ref->state == RELIST) || (ref->state == RECONN))
		return(COMM_ONLINE);
	if (ref->state == OPEN)
		return(CommSerStatus(ref->serial));
	return(COMM_FAILURE);
	
	
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPITick

    \Description
        Return current tick

    \Input *ref         - reference pointer

    \Output
        uint32_t    - elapsed milliseconds

    \Version    1.0        03/10/03 (JLB) Copied from CommSRPTick
*/
/*************************************************************************************************F*/
uint32_t CommTAPITick(CommTAPIRef *ref)
{
    return(NetTick());
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPISend
    
    \Description
        Send a packet
        
    \Input *ref     - module ref
    \Input *buffer  - pointer to data
    \Input *length  - length of data
    
    \Output
        int32_t         - negative=error, zero=ok

    \Notes
        Zero length packets may not be sent (they are ignored)

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPISend(CommTAPIRef *ref, const void *buffer, int32_t length)
{
	return(CommSerSend(ref->serial, buffer, length));
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIPeek
    
    \Description
        Peek at waiting packet
        
    \Input *ref     - module ref
    \Input *target  - target buffer
    \Input *length  - buffer length
    \Input *when    - tick received at
    
    \Output
        int32_t         - negative=nothing pending, else packet length

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIPeek(CommTAPIRef *ref, void *target, int32_t length, uint32_t *when)
{
	return(CommSerPeek(ref->serial, target, length, when));
}

/*F*************************************************************************************************/
/*!
    \Function    CommTAPIRecv
    
    \Description
        Receive a packet from the buffer
        
    \Input *ref     - module ref
    \Input *target  - target buffer
    \Input *length  - buffer length
    \Input *when    - tick received at
    
    \Output
        int32_t         - negative=error, else packet length

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTAPIRecv(CommTAPIRef *ref, void *target, int32_t length, uint32_t *when)
{
	return(CommSerRecv(ref->serial, target, length, when));
}