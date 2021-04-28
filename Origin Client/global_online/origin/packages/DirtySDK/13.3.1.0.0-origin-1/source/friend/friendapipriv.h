/*H********************************************************************************/
/*!
    \File friendapipriv.h

    \Description
        Header for private friend api functions.

    \Copyright
        Copyright (c) Electronic Arts 2010.

    \Version 02/01/2010 (mclouatre)
*/
/********************************************************************************H*/

#ifndef _friendapipriv_h
#define _friendapipriv_h

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock/netconn.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct FriendApiCallbackT
{
    uint8_t bInitFinished[NETCONN_MAXLOCALUSERS];   //<! whether or not friendlist and blocklist were updated
    FriendApiNotifyCbT *pNotifyCb;                  //!< user-provided notification callbacks
    void *pNotifyCbUserData;                        //!< user data associated with the notification callbacks
} FriendApiCallbackT;

//! friend api common ref
typedef struct FriendApiCommonRefT
{
    NetCritT notifyCrit;                //!< critical section used to ensure thread-safe usage of pNotifyCb and pNotifyCbUserData

    int32_t iMemGroup;                  //!< dirtymem memory group
    void *pMemGroupUserData;            //!< dirtymem memory group user data

    uint32_t uFriendListVersion;        //!< current version of the friends list
    uint32_t uBlockListVersion;         //!< current version of the block list

    FriendApiCallbackT callback[FRIENDAPI_MAX_CALLBACKS];

    uint8_t pad[3];                     //!< pad to four-byte alignment
} FriendApiCommonRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the platform-specific friend api module
FriendApiRefT *FriendApiPrivCreate(void);

// give module time to perform periodic processing
void FriendApiPrivUpdate(FriendApiRefT *pFriendApiRef);

// returns the list of friends in the user-provided buffer
int32_t FriendApiPrivGetFriendsList(FriendApiRefT *pFriendApiRef,  uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendCount);

// get status information
int32_t FriendApiPrivStatus(FriendApiRefT *pFriendApiRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// destroy the platform-specific friend api module
void FriendApiPrivDestroy(FriendApiRefT *pFriendApiRef);

// returns whether a user is friend or not
uint32_t FriendApiPrivIsUserFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// returns whether a user is blocked or not (does not have to be a friend)
uint32_t FriendApiPrivIsUserBlocked(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// block a user (does not have to be a friend)
int32_t FriendApiPrivBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// unblock a user (does not have to be a friend)
int32_t FriendApiPrivUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// returns the current version of the list of blocked users (ps3 only, always returns 0 on other platforms)
int32_t FriendApiPrivGetBlockList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, BlockedUserDataT *pBlockList, int32_t *pUsersCount);

// gets the version of the list of blocked users (ps3 only, always returns empty list on other platforms)
int32_t FriendApiPrivGetBlockListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion);

// signals all registered callbacks
void FriendApiPrivNotifyEvent(FriendApiRefT *pFriendApiRef, FriendEventDataT *pEventData);

#ifdef __cplusplus
};
#endif

#endif // _friendapipriv_h
