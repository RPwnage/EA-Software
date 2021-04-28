/*H*************************************************************************************************/
/*!

    \File    commudp.h

    \Description
        This is a simple UDP transport class which incorporations the notions of virtual
        connections and error free transfer. The protocol is optimized for use in a real-time
        environment where latency is more important than bandwidth. Overhead is low with
        the protocol adding only 8 bytes per packet on top of that required by UDP itself.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    0.1        02/09/99 (GWS) First Version
    \Version    0.2        02/14/99 (GWS) Revised and enhanced
    \Version    0.5        02/14/99 (GWS) Alpha release
    \Version    1.0        07/30/99 (GWS) Final release
    \Version    2.0        10/27/99 (GWS) Revised to use winsock 1.1/2.0
    \Version    2.1        12/04/99 (GWS) Removed winsock 1.1 support
    \Version    2.2        01/12/00 (GWS) Fixed receive tick bug
    \Version    2.3        06/12/00 (GWS) Added fastack for low-latency nets
    \Version    2.4        07/07/00 (GWS) Added firewall penetrator
    \Version    3.0        12/04/00 (GWS) Reported to dirtysock
    \Version    3.1        11/20/02 (JLB) Added Send() flags parameter
    \Version    3.2        02/18/03 (JLB) Fixes for multiple connection support
    \Version    3.3        05/06/03 (GWS) Allowed poke to come from any IP
    \Version    3.4        09/02/03 (JLB) Added unreliable packet type
    \Version    4.0        09/12/03 (JLB) Per-send optional unreliable transport

*/
/*************************************************************************************************H*/


#ifndef _commudp_h
#define _commudp_h

/*!
\Module Comm
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

// basic reference returned/used by all routines
typedef struct CommUDPRef CommUDPRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the class
CommUDPRef *CommUDPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);

// destruct the class
void CommUDPDestroy(CommUDPRef *what);

// resolve an address
int32_t CommUDPResolve(CommUDPRef *what, const char *addr, char *buf, int32_t len, char iDiv);

// resolve an address
void CommUDPUnresolve(CommUDPRef *what);

// listen for a connection
int32_t CommUDPListen(CommUDPRef *what, const char *addr);

// stop listening
int32_t CommUDPUnlisten(CommUDPRef *what);

// initiate a connection to a peer
int32_t CommUDPConnect(CommUDPRef *what, const char *addr);

// terminate a connection
int32_t CommUDPUnconnect(CommUDPRef *what);

// set event callback hook
void CommUDPCallback(CommUDPRef *what, void (*callback)(void *ref, int32_t event));

// return current stream status
int32_t CommUDPStatus(CommUDPRef *what);

// control connection behavior
int32_t CommUDPControl(CommUDPRef *pRef, int32_t iControl, int32_t iValue, void *pValue);

// return current clock tick
uint32_t CommUDPTick(CommUDPRef *what);

// send a packet
int32_t CommUDPSend(CommUDPRef *what, const void *buffer, int32_t length, uint32_t flags);

// peek at waiting packet
int32_t CommUDPPeek(CommUDPRef *what, void *target, int32_t length, uint32_t *when);

// receive a packet from the buffer
int32_t CommUDPRecv(CommUDPRef *what, void *target, int32_t length, uint32_t *when);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commudp_h

