/*H*************************************************************************************************/
/*!

    \File    commsrp.h

    \Description
        This is CommSRP (Selectively Reliable Protocol), a datagram packet-based
        transport class.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    0.5        01/03/03 (JLB) Initial Version, based on CommTCP
    \Version    0.7        01/07/03 (JLB) Working unreliable transport, based on CommUDP
    \Version    0.8        01/08/03 (JLB) Working reliable transport.
    \Version    0.9        02/09/03 (JLB) Added support for sending zero-byte packets, and
                                          fixed PS2 alignment issue.

*/
/*************************************************************************************************H*/


#ifndef _commsrp_h
#define _commsrp_h

/*!
\Module Comm
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

// basic reference returned/used by all routines
typedef struct CommSRPRef CommSRPRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the class
CommSRPRef *CommSRPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);

// destruct the class
void CommSRPDestroy(CommSRPRef *what);

// resolve an address
int32_t CommSRPResolve(CommSRPRef *what, const char *addr, char *buf, int32_t len, char div);

// stop the resolver
void CommSRPUnresolve(CommSRPRef *what);

// listen for a connection
int32_t CommSRPListen(CommSRPRef *what, const char *addr);

// stop listening
int32_t CommSRPUnlisten(CommSRPRef *what);

// initiate a connection to a peer
int32_t CommSRPConnect(CommSRPRef *what, const char *addr);

// terminate a connection
int32_t CommSRPUnconnect(CommSRPRef *what);

// set event callback hook
void CommSRPCallback(CommSRPRef *what, void (*callback)(CommRef *ref, int32_t event));

// return current stream status
int32_t CommSRPStatus(CommSRPRef *what);

// return current clock tick
uint32_t CommSRPTick(CommSRPRef *what);

// send a packet
int32_t CommSRPSend(CommSRPRef *what, const void *buffer, int32_t length, uint32_t flags);

// peek at waiting packet
int32_t CommSRPPeek(CommSRPRef *what, void *target, int32_t length, uint32_t *when);

// receive a packet from the buffer
int32_t CommSRPRecv(CommSRPRef *what, void *target, int32_t length, uint32_t *when);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commsrp_h






