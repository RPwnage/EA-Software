/*H*************************************************************************************************/
/*!

    \File    commser.h

    \Description
        Serial comm driver

    \Notes
        This is a simple serial transport class which incorporates
        the notions of virtual connections and error free transfer. The
        protocol is optimized for use in a real-time environment where
        latency is more important than bandwidth. Overhead is low with
        the protocol adding only 8 bytes per packet on top of that
        required by Ser itself.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    1.0        02/15/99 (GWS) First Version
    \Version    1.1        02/25/99 (GWS) Alpha release
*/
/*************************************************************************************************H*/

#ifndef _commser_h
#define _commser_h

/*!
\Moduledef CommSer CommSer
\Modulemember Comm
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! basic reference returned/used by all routines
typedef struct CommSerRef CommSerRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the class
CommSerRef *CommSerConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);

// destruct the class
void CommSerDestroy(CommSerRef *what);

// set module callback
void CommSerCallback(CommSerRef *ref, void (*callback)(CommRef *ref, int32_t event));

// resolve an address
int32_t CommSerResolve(CommSerRef *what, const char *addr, char *buf, int32_t len, char div);

// stop the resolver
void CommSerUnresolve(CommSerRef *what);

// listen for a connection
int32_t CommSerListen(CommSerRef *what, const char *addr);

// stop listening
int32_t CommSerUnlisten(CommSerRef *what);

// initiate a connection to a peer
int32_t CommSerConnect(CommSerRef *what, const char *addr);

// terminate a connection
int32_t CommSerUnconnect(CommSerRef *what);

// return current stream status
int32_t CommSerStatus(CommSerRef *what);

// get current tick
uint32_t CommSerTick(CommSerRef *ref);

// send a packet
int32_t CommSerSend(CommSerRef *what, const void *buffer, int32_t length);

// peek at waiting packet
int32_t CommSerPeek(CommSerRef *what, void *target, int32_t length, uint32_t *when);

// receive a packet from the buffer
int32_t CommSerRecv(CommSerRef *what, void *target, int32_t length, uint32_t *when);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commser_h
