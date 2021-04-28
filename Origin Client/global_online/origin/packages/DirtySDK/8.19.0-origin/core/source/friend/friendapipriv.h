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

/*** Defines **********************************************************************/
//! max number of registered callbacks
#define FRIENDAPI_MAX_CALLBACKS       (8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! friend api common ref
typedef struct FriendApiCommonRefT
{
    int32_t iMemGroup;              //!< dirtymem memory group
    void *pMemGroupUserData;        //!< dirtymem memory group user data

#if defined(DIRTYCODE_PS3)
    NetCritT notifyCrit;            //!< critical section used to ensure thread-safe usage of pNotifyCb and pNotifyCbUserData
    /*
    The following two variables are to be accessed safely within callbackCrit. They are
    all used in the friendapi code and in the callback registered with the NP Basic utility.
    So they need to be protected with a critical section to guarantee thread safety.
     */
#endif
    FriendApiNotifyCbT *pNotifyCb[FRIENDAPI_MAX_CALLBACKS]; //!< user-provided notification callbacks
    void *pNotifyCbUserData[FRIENDAPI_MAX_CALLBACKS]; //!< user data associated with the notification callbacks

    uint32_t uVersion;              //!< current version of the friends list

    uint8_t bInCallback;            //!< whether we are in the context of the user's callback being invoked
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
void FriendApiPrivGetFriendsList(FriendApiRefT *pFriendApiRef,  uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendCount);

// get status information
int32_t FriendApiPrivStatus(FriendApiRefT *pFriendApiRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// control function
int32_t FriendApiPrivControl(FriendApiRefT *pFriendApiRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// destroy the platform-specific friend api module
void FriendApiPrivDestroy(FriendApiRefT *pFriendApiRef);

// returns whether a user is blocked or not (does not have to be a friend)
uint32_t FriendApiPrivIsUserBlocked(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pGamerId);

// block a user (does not have to be a friend)
int32_t FriendApiPrivBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// unblock a user (does not have to be a friend)
int32_t FriendApiPrivUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// returns the current version of the list of blocked users (ps3 only, always returns 0 on other platforms)
void FriendApiPrivGetBlockList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, BlockedUserDataT *pBlockList, int32_t *pUsersCount);

// returns the list of blocked users in the user-provided buffers (ps3 only, always returns empty list on other platforms)
uint32_t FriendApiPrivGetBlockListVersion(FriendApiRefT *pFriendApiRef);

#ifdef __cplusplus
};
#endif

#endif // _friendapipriv_h

