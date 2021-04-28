/*H*************************************************************************************************/
/*!

    \File    commtcp.h

    \Description
        This is a simple packet based TCP transport class. It is optimized
        for use in real-time environments (expending additional bandwidth for
        the sake of better latency). It uses the same packet based interface
        as the other Comm modules. Packet overhead is very low at only 2
        extra bytes per packet beyond what TCP adds.

    \Notes
        Unfortunately, because of delayed ack handling in many TCP implementations,
        performance of this module is far below that of the UDP version. Also, it
        is much more sensitive to data loss. While this is kept around for historical
        reasons, it should not be used for new applications.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2002.  ALL RIGHTS RESERVED.

    \Version    0.5        02/19/99 (GWS) First Version
    \Version    1.0        02/25/99 (GWS) Alpha release
    \Version    1.1        07/27/99 (GWS) Initial release
    \Version    2.0        10/28/99 (GWS) Revised to use winsock 1.1/2.0
    \Version    2.1        12/28/99 (GWS) Removed winsock 1.1 support
    \Version    2.2        07/10/00 (GWS) Reverted to winsock 1.1/2.0 version
    \Version    2.3        11/20/02 (JLB) Added Send() flags parameter

*/
/*************************************************************************************************H*/


#ifndef _commtcp_h
#define _commtcp_h

/*!
\Module Comm
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

// basic reference returned/used by all routines
typedef struct CommTCPRef CommTCPRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the class
CommTCPRef *CommTCPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);

// destruct the class
void CommTCPDestroy(CommTCPRef *what);

// resolve an address
int32_t CommTCPResolve(CommTCPRef *what, const char *addr, char *buf, int32_t len, char div);

// stop the resolver
void CommTCPUnresolve(CommTCPRef *what);

// listen for a connection
int32_t CommTCPListen(CommTCPRef *what, const char *addr);

// stop listening
int32_t CommTCPUnlisten(CommTCPRef *what);

// initiate a connection to a peer
int32_t CommTCPConnect(CommTCPRef *what, const char *addr);

// terminate a connection
int32_t CommTCPUnconnect(CommTCPRef *what);

// set event callback hook
void CommTCPCallback(CommTCPRef *what, void (*callback)(CommRef *ref, int32_t event));

// return current stream status
int32_t CommTCPStatus(CommTCPRef *what);

// return current clock tick
uint32_t CommTCPTick(CommTCPRef *what);

// send a packet
int32_t CommTCPSend(CommTCPRef *what, const void *buffer, int32_t length, uint32_t flags);

// peek at waiting packet
int32_t CommTCPPeek(CommTCPRef *what, void *target, int32_t length, uint32_t *when);

// receive a packet from the buffer
int32_t CommTCPRecv(CommTCPRef *what, void *target, int32_t length, uint32_t *when);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commtcp_h






