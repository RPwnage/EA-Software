/*H*************************************************************************************************/
/*!

    \File    commtapi.h

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

#ifndef _commtapi_h
#define _commtapi_h

/*!
\Moduledef CommTAPI CommTAPI
\Modulemember Comm
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! basic reference returned/used by all routines
typedef struct CommTAPIRef CommTAPIRef;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the class
DIRTYCODE_API CommTAPIRef *CommTAPIConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout);

// destruct the class
DIRTYCODE_API void CommTAPIDestroy(CommTAPIRef *what);

// set module callback
DIRTYCODE_API void CommTAPICallback(CommTAPIRef *ref, void (*callback)(CommRef *ref, int32_t event));

// resolve an address
DIRTYCODE_API int32_t CommTAPIResolve(CommTAPIRef *what, const char *addr, char *buf, int32_t len, char div);

// stop the resolver
DIRTYCODE_API void CommTAPIUnresolve(CommTAPIRef *what);

// listen for a connection
DIRTYCODE_API int32_t CommTAPIListen(CommTAPIRef *what, const char *addr);

// stop listening
DIRTYCODE_API int32_t CommTAPIUnlisten(CommTAPIRef *what);

// initiate a connection to a peer
DIRTYCODE_API int32_t CommTAPIConnect(CommTAPIRef *what, const char *addr);

// terminate a connection
DIRTYCODE_API int32_t CommTAPIUnconnect(CommTAPIRef *what);

// return current stream status
DIRTYCODE_API int32_t CommTAPIStatus(CommTAPIRef *what);

// get current tick
DIRTYCODE_API uint32_t CommTAPITick(CommTAPIRef *ref);

// send a packet
DIRTYCODE_API int32_t CommTAPISend(CommTAPIRef *what, const void *buffer, int32_t length);

// peek at waiting packet
DIRTYCODE_API int32_t CommTAPIPeek(CommTAPIRef *what, void *target, int32_t length, uint32_t *when);

// receive a packet from the buffer
DIRTYCODE_API int32_t CommTAPIRecv(CommTAPIRef *what, void *target, int32_t length, uint32_t *when);

#ifdef __cplusplus
}
#endif

//@}

#endif // _commtapi_h
