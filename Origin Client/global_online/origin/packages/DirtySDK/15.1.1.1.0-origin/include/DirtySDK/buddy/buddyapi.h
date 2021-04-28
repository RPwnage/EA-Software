/*H*************************************************************************************/
/*!
    \File buddyapi.h

    \Description
        This module implements the client API for the EA.COM buddy server.

    \Copyright
        Copyright (c) Electronic Arts 2003

    \Version 1.0 01/20/2002 (gschaefer) First Version
*/
/*************************************************************************************H*/

#ifndef _buddyapi_h
#define _buddyapi_h

/*!
\Moduledef BuddyApi BuddyApi
\Modulemember Friend
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/util/displist.h"

/*** Defines ***************************************************************************/

#define BUDDY_MAX_USERS_FOUND   20    //!< max number of  users returned on "find user".
#define BUDDY_DOMAIN_LEN    4
#define BUDDY_PRODUCT_LEN   16
#define BUDDY_TITLE_LEN     40
#define BUDDY_NAME_LEN     32
#define BUDDY_SESSION_ID_LEN 16

#define BUDDY_FLAG_BUDDY  (1<<('B'&31))     //!< user is on buddy list
#define BUDDY_FLAG_IGNORE (1<<('I'&31))     //!< user is on ignore list
#define BUDDY_FLAG_TEMP   (1<<('T'&31))     //!< user is temporary
#define BUDDY_FLAG_SAME   (1<<('M'&31))     //!< user is using same product
#define BUDDY_FLAG_PEND   (1<<('P'&31))     //!< user is pending (waiting for server)
#define BUDDY_FLAG_WATCH  (1<<('W'&31))     //!< watch this user (for temp buddies)
#define BUDDY_FLAG_TEXT   (1<<('X'&31))      //!< supports Text messaging i.e. EA user.
#define BUDDY_FLAG_NOREPLY   (1<<('Y'&31))   //!< bud who wont accept msgs or invites.

#define BUDDY_FLAG_BUD_REQ_SENT  (1<<('S'&31))     //!< I invited him to be my bud
#define BUDDY_FLAG_BUD_REQ_RECEIVED  (1<<('R'&31)) //!< I recd invite to be his bud.
//acceptance is absense of above two bits.. and FLAG_BUDDY

#define BUDDY_FLAG_JOINABLE  (1<<('L'&31))   //!< In a state where you can join me in lobby etc.
#define BUDDY_FLAG_GAME_INVITE_SENT  (1<<('Q'&31))     //!< I invited him to play game
#define BUDDY_FLAG_GAME_INVITE_RECEIVED  (1<<('V'&31))  //!< I recd invite to play game
#define BUDDY_FLAG_GAME_INVITE_ACCEPTED  (1<<('C'&31))  //!<   game invite accepted
#define BUDDY_FLAG_GAME_INVITE_REJECTED  (1<<('J'&31))   //!< game invite rejected --wont be flagged
#define BUDDY_FLAG_GAME_INVITE_READY     (1<<('O'&31))   //!< invitee is now back online, in game.
#define BUDDY_FLAG_GAME_INVITE_TIMEOUT   (1<<('Z'&31))   //!< t.o. waiting for acceptor to get back in game
#define BUDDY_FLAG_GAME_INVITE_REVOKE    (1<<('K'&31))   //!< changed mind --wont be flagged

//! Mask for clearing all game invite flags.
#define BUDDY_MASK_GAME_INVITE ( BUDDY_FLAG_GAME_INVITE_SENT \
            | BUDDY_FLAG_GAME_INVITE_RECEIVED \
            | BUDDY_FLAG_GAME_INVITE_ACCEPTED \
            | BUDDY_FLAG_GAME_INVITE_REJECTED\
            | BUDDY_FLAG_GAME_INVITE_READY \
            | BUDDY_FLAG_GAME_INVITE_TIMEOUT\
            | BUDDY_FLAG_GAME_INVITE_REVOKE )

//! Mask for clearing all game invite when i do revoke--Do not clear Received flag.
#define BUDDY_MASK_GAME_REVOKE_ONLY ( BUDDY_FLAG_GAME_INVITE_SENT \
            | BUDDY_FLAG_GAME_INVITE_ACCEPTED \
            | BUDDY_FLAG_GAME_INVITE_REJECTED\
            | BUDDY_FLAG_GAME_INVITE_READY \
            | BUDDY_FLAG_GAME_INVITE_TIMEOUT\
            | BUDDY_FLAG_GAME_INVITE_REVOKE )

//! Mask for clearing all game invite when i do Reject--Do not clear Sent flag.
#define BUDDY_MASK_GAME_REJECT_ONLY (   BUDDY_FLAG_GAME_INVITE_RECEIVED \
            | BUDDY_FLAG_GAME_INVITE_ACCEPTED \
            | BUDDY_FLAG_GAME_INVITE_REJECTED\
            | BUDDY_FLAG_GAME_INVITE_READY \
            | BUDDY_FLAG_GAME_INVITE_TIMEOUT\
            | BUDDY_FLAG_GAME_INVITE_REVOKE )


//! If on a delete, any of these flags still set, keep the buddy, as this defines a bud.
#define BUDDY_MASK_KEEP_BUDDY ( BUDDY_FLAG_BUDDY  \
            | BUDDY_FLAG_IGNORE \
            | BUDDY_FLAG_TEMP \
            | BUDDY_FLAG_BUD_REQ_SENT \
            | BUDDY_FLAG_BUD_REQ_RECEIVED )


#define BUDDY_STAT_UNDEFINED -1
#define BUDDY_STAT_DISC 0
#define BUDDY_STAT_CHAT 1
#define BUDDY_STAT_AWAY 2
#define BUDDY_STAT_XA   3
#define BUDDY_STAT_DND  4
#define BUDDY_STAT_PASS 5
#define BUDDY_STAT_GAME 6

//! Treatment of my messages at server.
#define BUDDY_FORWARD_FLAG_EMAIL            (1<<0)    //!< Send to email
#define BUDDY_FORWARD_FLAG_SMS              (1<<1)    //!< Send to SMS
#define BUDDY_FORWARD_FLAG_UNKNOWN          (1<<8)    //!< Not loaded from server yet.
#define BUDDY_EMAIL_SMS_ADDR_LEN            254      //!< max length of email or sms address

#define BUDDY_MSG_CHAT      'chat'
#define BUDDY_MSG_ADMIN     'admn'
#define BUDDY_MSG_ROSTER    'rost'
#define BUDDY_MSG_CONNECT   'conn'
#define BUDDY_MSG_AUTHORIZE 'auth'
#define BUDDY_MSG_ADDUSER   'addu'
#define BUDDY_MSG_DELUSER   'delu'
#define BUDDY_MSG_ENDROSTER 'endr'
#define BUDDY_MSG_SEND      'send'
#define BUDDY_MSG_USERS      'user'
#define BUDDY_MSG_BROADCAST  'brdc'
#define BUDDY_MSG_SET_FORWARDING 'epst'
#define BUDDY_MSG_BUD_INVITE   'invt'
#define BUDDY_MSG_GAME_INVITE  'ginv'
#define BUDDY_MSG_PRESENCE   'pget'


#define BUDDY_EASO_BODYLEN  (255)
#define BUDDY_EASO_SUBJLEN  (0)

#define BUDDY_TYPE_APPL "A"         //!< application message
#define BUDDY_TYPE_CHAT "C"         //!< chat message

#define BUDDY_SERV_IDLE 0           //!< idle state (no connection)
#define BUDDY_SERV_CONN 1           //!< connecting to server
#define BUDDY_SERV_NAME 2           //!< server name lookup failure
#define BUDDY_SERV_TIME 3           //!< server timeout
#define BUDDY_SERV_FAIL 4           //!< misc connection failure
#define BUDDY_SERV_ONLN 5           //!< server is online
#define BUDDY_SERV_DISC 6           //!< server has disconnected
#define BUDDY_SERV_RECN 7           //!< waiting to reconnect to server

#define BUDDY_ERROR_NONE    0       //!< no error
#define BUDDY_ERROR_PEND    -1      //!< command is still pending
#define BUDDY_ERROR_NOTUSER -2      //!< not a user
#define BUDDY_ERROR_BADPARM -3      //!< bad parameter
#define BUDDY_ERROR_OFFLINE -4      //!< server is offline
#define BUDDY_ERROR_FULL -5      //!< no more room to add
#define BUDDY_ERROR_UNSUPPORTED -6      //!< feaure not supported on platform
#define BUDDY_ERROR_SELF     -7   //!< cannot add oneself
#define BUDDY_ERROR_MISSING    -8  //!< item not found etc.
#define BUDDY_ERROR_TIMEOUT    -10  //!< app level timeout.
#define BUDDY_ERROR_BLOCKED    -11  //!< user has blocked your invites.
#define BUDDY_ERROR_MEMORY    -12  //!< memory alloc failure -- user alloc block is full?
#define BUDDY_ERROR_MAX      -13

//#define BUDDY_LANG_...  --NO longer used, see Locale

// list of support prods ---THIS IS DEFUNCT --
//#define BUDDY_PROD_xxx            'xxx3'   // This is defunct
// see hlbudapi.h ---Resource string on login to uniquely identify products.

// list of official unicode/utf8 symbols for presence information
#define BUDDY_SYMBOL_C      "\xc2\xa9"
#define BUDDY_SYMBOL_R      "\xc2\xae"
#define BUDDY_SYMBOL_TM     "\xe2\x84\xa2"


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! Xbfriend response and action types for games and buddies.
typedef enum BuddyInviteResponseTypeE {
    BUDDY_INVITE_RESPONSE_ACCEPT  =0,
    BUDDY_INVITE_RESPONSE_REJECT ,
    BUDDY_INVITE_RESPONSE_REVOKE ,
    BUDDY_INVITE_RESPONSE_BLOCK  ,
    BUDDY_INVITE_RESPONSE_REMOVE ,
    BUDDY_INVITE_RESPONSE_INVITE,
    BUDDY_INVITE_RESPONSE_MUTE,
    BUDDY_INVITE_RESPONSE_JOIN,
    BUDDY_INVITE_RESPONSE_TIMEOUT,
    BUDDY_INVITE_RESPONSE_UNKNOWN_MAX  //make it the last one
}BuddyInviteResponseTypeE;  //see aConvertAction



typedef struct BuddyApiRefT BuddyApiRefT;

//! roster entry
typedef struct BuddyRosterT
{
    char strUser[BUDDY_NAME_LEN+1];       //!< user name
    //char strProd[20];       //!< localized product name -- see strTitle
    char strPres[80];       //!< presence string (empty=unavailable)
    char strTitle[BUDDY_TITLE_LEN+1];       //!< title name string (empty=unavailable)
    char strPresExtra[64];    //!< extra presence string (empty=unavailable)
    uint32_t uPresExFlags;  //!< extra client specified presence flags
    uint16_t uLang;        //!< language buddy is in, if known -if not 'en'.
    char iShow;             //!< current show status
    uint32_t uFlags;    //!< attribute flags
    int32_t iPend;              //!< pending translation number [private]
    void *pUserRef;         //!< user reference value
    uint32_t uBudUserVal;   //!< user reference value
    char strSEID[BUDDY_SESSION_ID_LEN+1]; //! ps2 only -- senders game session id.
} BuddyRosterT;

//! info message (chat, extended info, etc)
typedef struct BuddyApiMsgT
{
    int32_t iType;                  //!< message classification (fail/mesg/extd)
    int32_t iKind;                  //!< kind of message (from server packet)
    int32_t iCode;                  //!< code of message (from server packet)
    uint32_t uTimeStamp;    //!< time of message
    uint32_t uUserFlags;    //!< flags for client's usage on received messages.
    const char *pData;          //!< callback message data
} BuddyApiMsgT;

typedef struct BuddyApiUserT
{
    char  name[17];
    char  domain[BUDDY_DOMAIN_LEN +1 ];
    char  product[ BUDDY_PRODUCT_LEN +1 ];
} BuddyApiUserT;


//! buddy msg or list callback function prototype
typedef void (BuddyApiCallbackT)(BuddyApiRefT *pRef, BuddyApiMsgT *pMsg, void *pUserData);

//! callback for client to free any "user ref" ptr memory, prior to buddy being deleted.
typedef void (BuddyApiDelCallbackT)(BuddyApiRefT *pRef, BuddyRosterT *pBuddy, void *pUserData);

//! buddy change callback function prototype --game invite or presence etc. changed
typedef void (BuddyApiBuddyChangeCallbackT)(BuddyApiRefT *pRef, BuddyRosterT *pBuddy, int32_t iKind,
    int32_t iAction, void *pUserData);

//! Function prototype for custom mem allocation function.
typedef void *(BuddyMemAllocT)(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

//! Function prototype for custom mem free function.
typedef void (BuddyMemFreeT)(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create a new buddy interface client
DIRTYCODE_API BuddyApiRefT *BuddyApiCreate2(BuddyMemAllocT *pAlloc,  BuddyMemFreeT *pFree,
                              const char *pProduct, const char *pRelease, int32_t iRefMemGroup, void *pRefMemGroupUserData);

// Destroy an existing client
DIRTYCODE_API void BuddyApiDestroy(BuddyApiRefT *pState);

// Assign the debug callback
DIRTYCODE_API void BuddyApiDebug(BuddyApiRefT *pState, void *pDebugData, void (*pDebugProc)(void *pData, const char *pText));

// Config the language, initial same product identifier and locale for translating title name.
DIRTYCODE_API int32_t BuddyApiConfig(BuddyApiRefT *pState, const char *pSame, uint32_t uLocality);

// Connect to the buddy server
DIRTYCODE_API int32_t BuddyApiConnect(BuddyApiRefT *pState, const char *pServer, int32_t iPort, int32_t iTimeout, const char *pLKey, const char *pRsrc, BuddyApiCallbackT *pCallback, void *pCalldata);

// Disconnect from the buddy server
DIRTYCODE_API int32_t BuddyApiDisconnect(BuddyApiRefT *pState);

// Return the current connection status
DIRTYCODE_API int32_t BuddyApiStatus(BuddyApiRefT *pState);

// Return the resource name used by this api. See Connect.
DIRTYCODE_API char * BuddyApiResource(BuddyApiRefT *pState);

// Return the domain name used by this api.-is tied to resource - See Connect.
DIRTYCODE_API char * BuddyApiDomain(BuddyApiRefT *pState);

// Flush the roster from memory
DIRTYCODE_API void BuddyApiFlush(BuddyApiRefT *pState);

// Get the roster from the server
DIRTYCODE_API DispListRef *BuddyApiRoster(BuddyApiRefT *pState, BuddyApiCallbackT *pCallback, void *pCalldata);

// Get the roster list pointer (no allocation, no server interaction)
DIRTYCODE_API DispListRef *BuddyApiRosterList(BuddyApiRefT *pState);

// Add a user to the buddy roster
DIRTYCODE_API int32_t BuddyApiAdd(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);

// Invite a user to be my buddy.
DIRTYCODE_API int32_t BuddyApiBuddyInvite(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);

// Invite a user to be play a game.
DIRTYCODE_API int32_t BuddyApiGameInvite(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout,
                         int32_t iSessionId, const char *pGameInviteState);

// Set optional callback for when Buddy changes presence or game invite etc.
DIRTYCODE_API void BuddyApiRegisterBuddyChangeCallback(BuddyApiRefT *pState, BuddyApiBuddyChangeCallbackT *pCallback, void *pCalldata);

// Set optional callback for client to free user ref ptr, when buddy is being deleted from mem.
DIRTYCODE_API void BuddyApiRegisterBuddyDelCallback(BuddyApiRefT *pState, BuddyApiDelCallbackT *pDelCallback, void *pCalldata);

// Respond to user request to be my buddy.
DIRTYCODE_API int32_t BuddyApiRespondBuddy(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyInviteResponseTypeE eResponse,
                    BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);

// Respond to game invite requests.
DIRTYCODE_API int32_t BuddyApiRespondGame(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyInviteResponseTypeE eResponse,
                    BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout, int32_t iSessionId);

// Join a buddy in another game.
DIRTYCODE_API int32_t BuddyApiJoinGame(BuddyApiRefT *pState, BuddyRosterT *pBuddy,
                    BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);


// Add a user to the buddy roster
DIRTYCODE_API int32_t BuddyApiDel(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);

// Find a user in the buddy roster
DIRTYCODE_API BuddyRosterT *BuddyApiFind(BuddyApiRefT *pState, const char *pUser);

// Set or clear msg forwarding to email/sms etc.
DIRTYCODE_API int32_t BuddyApiSetForwarding(BuddyApiRefT *pState, unsigned uForwardingFlags, const char * strEmail,
                             BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);

// Get msg forwarding flags and email address etc if set.
DIRTYCODE_API int32_t BuddyApiGetForwarding(BuddyApiRefT *pState, unsigned *puForwardingFlags, char * strEmail );

// Search for users by name.
DIRTYCODE_API int32_t BuddyApiFindUsers(BuddyApiRefT *pState, const char *pUserPattern,
                             BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);

// After FindUsers returned, use this to extract each user found.
DIRTYCODE_API BuddyApiUserT  *BuddyApiUserFound(BuddyApiRefT *pState, int32_t item);

// Prepare to send our presence information to the server
DIRTYCODE_API void BuddyApiPresInit(BuddyApiRefT *pState);

// Set the presence information intended for the same product
DIRTYCODE_API void BuddyApiPresSame(BuddyApiRefT *pState, const char *pSame);

// Set the presence information intended for different products
DIRTYCODE_API void BuddyApiPresDiff(BuddyApiRefT *pState, const char *pDiff, const char *pLang);

// Set or reset bit indicating other Xbox games can "join" or meet me.
DIRTYCODE_API void BuddyApiPresJoinable(BuddyApiRefT *pState, int32_t bOn);

// Set any extra, client supplied,  transparent,  extra presence flags.
DIRTYCODE_API void BuddyApiPresExFlags (BuddyApiRefT *pState,  uint32_t uFlags);

// Set flag to set no text capability to server -if dont support text messages.
DIRTYCODE_API void BuddyApiPresNoText (BuddyApiRefT *pState);

// Set any extra presence string
DIRTYCODE_API void BuddyApiPresExtra (BuddyApiRefT *pState, const char *strExtraPres);

// Send our presence information to the server
DIRTYCODE_API int32_t BuddyApiPresSend(BuddyApiRefT *pState, const char *pRsrc, int32_t iShow, int32_t iDelay, int32_t bDontAutoRefreshRoster);

// Force into state where it  wont listen to Xbox callbacks etc.  regardless of Presence state
DIRTYCODE_API void BuddyApiSuspendXDK( BuddyApiRefT *pState);

// Force into state where it  will listen to Xbox callbacks etc. regardless of Presence state
DIRTYCODE_API void BuddyApiResumeXDK( BuddyApiRefT *pState);

// Send a message to a remote user
DIRTYCODE_API int32_t BuddyApiSend(BuddyApiRefT *pState, const char *pMesg, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);

// Send a message to a list of users
DIRTYCODE_API int32_t BuddyApiBroadcast(BuddyApiRefT *pState, const char *pMesg, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);
// Set receive callback
DIRTYCODE_API void BuddyApiRecv(BuddyApiRefT *pState, BuddyApiCallbackT *pCallback, void *pCalldata);

// Handle periodic update, and return game tick.
DIRTYCODE_API unsigned BuddyApiUpdate(BuddyApiRefT *pState);

//!Get the "Official" Xbox title name for a given title id, based on my locale-language.
DIRTYCODE_API int32_t BuddyApiGetTitleName (BuddyApiRefT *pState, const uint32_t uTitleId,  const int32_t iBufSize, char * pTitleName);

//!Get the "Official" title name for this game. pLang ignored for now--defaults to English.
DIRTYCODE_API int32_t BuddyApiGetMyTitleName (BuddyApiRefT *pState, const char * pLang, const int32_t iBufSize, char * pTitleName);

//temp test method prior to Server side Xbox integration..
DIRTYCODE_API void _BuddyApiSetTalkToXbox (BuddyApiRefT *pState, int32_t bOn);

// refresh buddy's title from XDK for xbox.
DIRTYCODE_API void BuddyApiRefreshTitle ( BuddyApiRefT * pBudRef,  BuddyRosterT *pBuddy);

//! Set my Session id, for an xbox game invite.
DIRTYCODE_API int32_t BuddyApiSetGameInviteSessionID(BuddyApiRefT *pState, const char *sSessionID);

//! Get this users game session id.
DIRTYCODE_API int32_t BuddyApiGetUserSessionID(BuddyApiRefT *pState, const char * strGamertag, char *sSessionID, const int32_t iBufSize);



#ifdef __cplusplus
}
#endif

//@}

#endif // _buddyapi_h

