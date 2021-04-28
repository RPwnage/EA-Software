/*H*************************************************************************************/
/*!
    \File    friendapi.h

    \Description
        The FriendApi module abstracts the different first-party friend apis. It
        locally maintains a generic local list of friends, and it provides higher-level
        modules with the ability of registering a notification callback reporting
        changes in the list of friends on a per-friend basis.

    \Copyright
        Copyright (c) Electronic Arts 2010.

    \Version 04/19/10 (mclouatre)
*/
/*************************************************************************************H*/

#ifndef _friendapi_h
#define _friendapi_h

/*!
    \Moduledef Friend Friend

    \Description
        The Friend module abstracts the 1st party friends functionality.

        <b>Overview</b>

        Overview TBD
*/

/*!
\Moduledef FriendApi FriendApi
\Modulemember Friend
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

// xenon friend id (xuid) is a 64-bit value which turns out to be a 20-char base-10 numeral string
// ps3 friend id is always set to "unavailable"
#define FRIENDAPI_FRIENDID_MAXLEN       (20)

// xenon friend name is filled with the gamertag which is limited to 15 chars (XUSER_MAX_NAME_LENGTH) in the XONLINE_FRIEND struct
// ps3 friend name is filled with the PSN online id, which is a 16-char string (see SceNpId struct)
#define FRIENDAPI_FRIENDNAME_MAXLEN     (16)    // max string length for friend name

// xenon: currently not used
// ps3: matches value of SCE_NP_BASIC_MAX_MESSAGE_SIZE (maximum message data size)
#define FRIENDAPI_EVENTMSG_MAXSIZE      (512)

// xenon: MS Game Developer support confirmed that XBOX360 currently does not support more than 100 friends per gamer
// ps3: PS3 doc mentions that the number of friends per gamer is limited to 100 (See NP_Basic-Reference_e.pdf)
#define FRIENDAPI_MAX_FRIENDS           (100)

// ps3 blocked user id is filled with the PSN online id, which is a 16-char string (see SceNpId struct)
#define FRIENDAPI_BLOCKEDUSERID_MAXLEN  (16)    // max string length for blocked user id

// ps3: PS3 doc mentions that the maximum entry count of the block list is 50 (See NP_Common-Reference_e.pdf)
#define FRIENDAPI_MAX_BLOCKED_USERS     (50)

// max number of registered callbacks
#define FRIENDAPI_MAX_CALLBACKS         (8)

//! FriendApi event types
typedef enum FriendApiEventsE
{
    FRIENDAPI_EVENT_UNKNOWN,                    //!< unknown friend event
    FRIENDAPI_EVENT_FRIEND_UPDATED,             //!< a friend was added or modified
    FRIENDAPI_EVENT_FRIEND_REMOVED,             //!< a friend was removed
    FRIENDAPI_EVENT_BLOCKLIST_UPDATED,          //!< the list of blocked friends has changed
    FRIENDAPI_EVENT_BLOCK_USER_RESULT,          //!< completion notification for FriendApiBlockUser()
    FRIENDAPI_EVENT_MSG_RCVED,                  //!< message received
    FRIENDAPI_EVENT_INIT_COMPLETE,              //!< signal completion of the the first upload of friendlist and blocklist
    FRIENDAPI_EVENT_MAX                         //!< number of events
} FriendApiEventsE;

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! opaque module ref
typedef struct FriendApiRefT FriendApiRefT;

//! generic friend data structure
typedef struct FriendDataT
{
    char strFriendId[FRIENDAPI_FRIENDID_MAXLEN+1];      //!< friend id
    char strFriendName[FRIENDAPI_FRIENDNAME_MAXLEN+1];  //!< friend name
    void *pRawData1;                                    //!< pointer to 1st party friend data structure 1
} FriendDataT;


// $todo v9
// deprecate BlockedUserDataT and replace it with a generic user type concept (something like DirtyUserT)
// DS should be able to convert the genere user identifier into a platform-specific user identifier

//! generic blocked user data structure
typedef struct BlockedUserDataT
{
    char strBlockedUserId[FRIENDAPI_BLOCKEDUSERID_MAXLEN+1]; //!< blocked user id
    void *pRawData1;                                    //!< pointer to 1st party friend data
} BlockedUserDataT;

typedef struct FriendEventDataUnknownT
{
    void * pRawData1;                               //!< message size
    void * pRawData2;                               //!< event message
    FriendDataT *pFriendData;                       //!< pointer to friend data to be passed to the callbacks
}FriendEventDataUnknownT;

typedef struct FriendEventDataFriendUpdatedT
{
    uint8_t bPresence;                              //!< TRUE - online; FALSE - offline
    void * pRawData1;                               //!< message size
    void * pRawData2;                               //!< event message
    FriendDataT *pFriendData;                       //!< pointer to friend data to be passed to the callbacks
}FriendEventDataFriendUpdatedT;

typedef struct FriendEventDataFriendRemovedT
{
    FriendDataT *pFriendData;                       //!< pointer to friend data to be passed to the callbacks
}FriendEventDataFriendRemovedT;

typedef struct FriendEventDataBlocklistUpdatedT
{
    uint8_t bPresence;                              //!< TRUE - online; FALSE - offline
    BlockedUserDataT *pBlockedUserData;             //!< pointer to friend data to be passed to the callbacks
}FriendEventDataBlocklistUpdatedT;

typedef struct FriendEventDataBlockUserResultT
{
    uint8_t bSuccess;                               //!< TRUE - success; FALSE - failure  (only meaningful with FRIENDAPI_EVENT_BLOCK_USER_RESULT)
} FriendEventDataBlockUserResultT;

typedef struct FriendEventDataMsgRcvedT
{
    void * pRawData1;                               //!< message size
    void * pRawData2;                               //!< event message
    FriendDataT *pFriendData;                       //!< pointer to friend data to be passed to the callbacks
}FriendEventDataMsgRcvedT;

typedef union FriendEventTypeDataT
{
    FriendEventDataUnknownT          Unknown;
    FriendEventDataFriendUpdatedT    FriendUpdated;
    FriendEventDataFriendRemovedT    FriendRemoved;
    FriendEventDataBlocklistUpdatedT BlockListUpdated;
    FriendEventDataBlockUserResultT  BlockUserResult;
    FriendEventDataMsgRcvedT         MsgRcved;
} FriendEventTypeDataT;

typedef struct FriendEventDataT
{
    FriendApiEventsE     eventType;
    FriendEventTypeDataT eventTypeData;
    uint8_t uUserIndex;
    void *pRawData1;                    /* platform-specific friend event data structure 1
                                            - PS3:   It points to event type value
                                            - Xenon: It is null */
    void *pRawData2;                    /* platform-specific friend event data structure 2
                                            - PS3:   It points to event error code
                                            - Xenon: It is null */
    void *pRawData3;                    /* platform-specific friend event data structure 3
                                            - PS3:   When not null, it points to a request id
                                            - Xenon: It is null */
}FriendEventDataT;

//! optional friend notification callback
//! Note: for some event types, pEventData and pFriendData can be NULL!
typedef void (FriendApiNotifyCbT) (FriendApiRefT *pFriendApiRef, FriendEventDataT *pEventData, void *pNotifyCbUserData);

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the friend api object
DIRTYCODE_API FriendApiRefT *FriendApiCreate(void);

// register callback for friend notifications
DIRTYCODE_API int32_t FriendApiAddCallback(FriendApiRefT *pFriendApiRef, FriendApiNotifyCbT *pNotifyCb, void *pNotifyCbUserData);

// unregister callback for friend notifications
DIRTYCODE_API int32_t FriendApiRemoveCallback(FriendApiRefT *pFriendApiRef, FriendApiNotifyCbT *pNotifyCb, void *pNotifyCbUserData);

// give module time to perform periodic processing
DIRTYCODE_API void FriendApiUpdate(FriendApiRefT *pFriendApiRef);

// gets the list of friends in the user-provided buffers (also returns corresponding list of 1st-party friend data structures)
DIRTYCODE_API int32_t FriendApiGetFriendsList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendsCount);

// gets the current version of the list of friends
DIRTYCODE_API int32_t FriendApiGetFriendsListVersion(FriendApiRefT *pFriendApiRef, uint32_t * pVersion);

// get status information
DIRTYCODE_API int32_t FriendApiStatus(FriendApiRefT *pFriendApiRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// control function
DIRTYCODE_API int32_t FriendApiControl(FriendApiRefT *pFriendApiRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// destroy the friend api object
DIRTYCODE_API void FriendApiDestroy(FriendApiRefT *pFriendApiRef);

// $todo v9 - For the following 3 functions
// the pUserId parameter should point to a generic user type concept (something like DirtyUserT)
// DS should be able to convert the genere user identifier into a platform-specific user identifier

// returns whether a user is friend or not
DIRTYCODE_API uint32_t FriendApiIsUserFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// returns whether a user is a 'real' friend or not
DIRTYCODE_API uint32_t FriendApiIsUserRealFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// returns whether a user is blocked or not (does not have to be a friend)
DIRTYCODE_API uint32_t FriendApiIsUserBlocked(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// block a user (does not have to be a friend)
DIRTYCODE_API int32_t FriendApiBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// unblock a user (does not have to be a friend)
DIRTYCODE_API int32_t FriendApiUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId);

// gets the current version of the list of blocked users (ps3 only, always returns 0 on other platforms)
DIRTYCODE_API int32_t FriendApiGetBlockListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion);

// $todo v9 - FriendApiGetBlockList
// the pBlockList parameter should point to an array of generic user type concept (something like DirtyUserT)
// instead of an array of BlockedUserDataT

// gets the list of blocked users in the user-provided buffers (ps3 only, always returns empty list on other platforms)
DIRTYCODE_API int32_t FriendApiGetBlockList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, BlockedUserDataT *pBlockList, int32_t *pUsersCount);

#ifdef __cplusplus
}
#endif

//@}

#endif // _friendapi_h

