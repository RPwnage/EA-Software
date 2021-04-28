/*H********************************************************************************/
/*!
    \File netconncommon.h

    \Description
        Cross-platform netconn data types and private functions.

    \Copyright
        Copyright (c) 2014 Electronic Arts Inc.

    \Version 05/21/2009 (mclouatre) First Version
*/
/********************************************************************************H*/

#ifndef _netconncommon_h
#define _netconncommon_h

/*** Include files ****************************************************************/
#include "netconncommon.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

#ifdef DIRTYSDK_IEAUSER_ENABLED
//! user event callback function prototype
typedef void (NetConnAddLocalUserCallbackT)(struct NetConnCommonRefT *pCommonRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser);
typedef void (NetConnRemoveLocalUserCallbackT)(struct NetConnCommonRefT *pCommonRef, int32_t iLocalUserIndex, const EA::User::IEAUser *pIEAUser);

typedef enum NetConnIEAUserEventTypeE
{
    NETCONN_EVENT_IEAUSER_ADDED = 0,
    NETCONN_EVENT_IEAUSER_REMOVED
} NetConnIEAUserEventTypeE;

typedef struct NetConnIEAUserEventT
{
    struct NetConnIEAUserEventT *pNext; //!< linked list
    const EA::User::IEAUser *pIEAUser;  //!< IEAUser reference
    NetConnIEAUserEventTypeE eEvent;    //!< event type
    int32_t iLocalUserIndex;            //!< local user index
} NetConnIEAUserEventT;
#endif

typedef struct NetConnCommonRefT
{
    // module memory group
    int32_t              iMemGroup;                               //!< module mem group id
    void                 *pMemGroupUserData;                      //!< user data associated with mem group

    int32_t iDebugLevel;

#ifdef DIRTYSDK_IEAUSER_ENABLED
    NetConnIEAUserEventT *pIEAUserFreeEventList;                  //!< list of free IEAUser
    NetConnIEAUserEventT *pIEAUserEventList;                      //!< list of pending NetConnIEAUserEvents - populated by customers with NetConnAddLocaUser()/NetConnRemoveUser()

    NetConnAddLocalUserCallbackT *pAddUserCb;
    NetConnRemoveLocalUserCallbackT *pRemoveUserCb;
#endif

    NetCritT                        crit;
} NetConnCommonRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// get netconn ref
NetConnCommonRefT *NetConnCommonGetRef(void);

// handle common shutdown functionality
void NetConnCommonShutdown(NetConnCommonRefT *pCommonRef);

#ifdef DIRTYSDK_IEAUSER_ENABLED
// handle common startup functionality
NetConnCommonRefT *NetConnCommonStartup(int32_t iNetConnRefSize, NetConnAddLocalUserCallbackT *pAddUserCb, NetConnRemoveLocalUserCallbackT *pRemoveUserCb);

// handle common add user functionality
int32_t NetConnCommonAddLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser);

// handle common remove user functionality
int32_t NetConnCommonRemoveLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser);

// handle common user update functionality
void NetConnCommonUpdateLocalUsers(NetConnCommonRefT *pCommonRef);
#else
// handle common startup functionality
NetConnCommonRefT *NetConnCommonStartup(int32_t iNetConnRefSize);
#endif

#ifdef __cplusplus
}
#endif

#endif // _netconcommon_h

