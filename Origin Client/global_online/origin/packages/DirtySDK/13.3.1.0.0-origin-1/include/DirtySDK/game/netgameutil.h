/*H*************************************************************************************************/
/*!

    \File    netgameutil.h

    \Description
        Group of functions to help setup network games

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2001-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        01/09/01 (GWS) First Version
    \Version    1.1        11/18/02 (JLB) Moved to NetGame hierarchy

*/
/*************************************************************************************************H*/


#ifndef _netgameutil_h
#define _netgameutil_h

/*!
\Module NetGame
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/comm/commall.h"

/*** Defines ***************************************************************************/

#define NETGAME_CONN_LISTEN     (1)    //!< listen for connect
#define NETGAME_CONN_CONNECT    (2)    //!< connect to peer
#define NETGAME_CONN_AUTO       (NETGAME_CONN_LISTEN|NETGAME_CONN_CONNECT) //!< (debug only) autoconnect

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct NetGameUtilRefT NetGameUtilRefT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the game setup module
NetGameUtilRefT *NetGameUtilCreate(void);

// destroy the game setup module
void NetGameUtilDestroy(NetGameUtilRefT *ref);

// reset the game setup module
void NetGameUtilReset(NetGameUtilRefT *ref);

// set internal GameUtil parameters
void NetGameUtilControl(NetGameUtilRefT *pRef, int32_t iKind, int32_t iValue);

// get a connection (connect/listen)
int32_t NetGameUtilConnect(NetGameUtilRefT *ref, int32_t conn, const char *addr, CommAllConstructT *pConstruct);

// check connection status
void *NetGameUtilComplete(NetGameUtilRefT *ref);

// return status info
int32_t NetGameUtilStatus(NetGameUtilRefT *ref, int32_t iSelect, void *pBuf, int32_t iBufSize);

// send out an advertisement
void NetGameUtilAdvert(NetGameUtilRefT *ref, const char *kind, const char *name, const char *note);

// cancel current advertisement
void NetGameUtilWithdraw(NetGameUtilRefT *ref, const char *kind, const char *name);

// find ip address of a specific advertisement
uint32_t NetGameUtilLocate(NetGameUtilRefT *ref, const char *kind, const char *name);

// return a list of all advertisements
int32_t NetGameUtilQuery(NetGameUtilRefT *ref, const char *kind, char *buf, int32_t max);

#ifdef __cplusplus
}
#endif

//@}

#endif // _netgameutil_h

