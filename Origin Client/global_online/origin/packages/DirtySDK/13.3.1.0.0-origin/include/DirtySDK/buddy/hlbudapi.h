/*H*************************************************************************************************/
/*!

    \File hlbudapi.h

    \Description
        This module implements High Level Buddy, (HLB), an application level interface 
        to the Buddy server. This is based on generic EA Sports Requirements for 2004.

    \Notes
        Depends on and encapsulates, the lower level (LL) dirty sock buddy api. 
        (buddyapi.h and .c modules). Clients should either use that interface, 
        or this, as using both in same game WILL cause problems.

    \Todo --  What happens to budd list sort if add a buddy etc.

    \Copyright
        Copyright (c) Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Version 0.3 10/28/2003 (TE) --added voip, challenge support.
    \Version 0.4 09/14/2009 (mclouatre) -- created HLBBudGetSceNpId()

*/
/*************************************************************************************************H*/

#ifndef _HLBApi_h
#define _HLBApi_h

#include "DirtySDK/util/utf8.h"
#include "DirtySDK/buddy/buddyapi.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"

/*!
    \Moduledef HighLevelBuddy HighLevelBuddy

    \Description
        The  module provides routines for interacting with a buddy server.
*/
//@{

/*** Defines ***************************************************************************/


//! State of a Buddy -- same as BUDDY_STAT 
typedef enum HLBStatE {
                            HLB_STAT_UNDEFINED = BUDDY_STAT_UNDEFINED,  
							HLB_STAT_DISC = BUDDY_STAT_DISC,  //offline
                            HLB_STAT_CHAT  = BUDDY_STAT_CHAT, //online
						    HLB_STAT_AWAY = BUDDY_STAT_AWAY, //online, away
						    HLB_STAT_XA = BUDDY_STAT_XA,  //online, extended away
						    HLB_STAT_DND = BUDDY_STAT_DND, //online,do not disturb, aka BUSY
						    HLB_STAT_PASSIVE = BUDDY_STAT_PASS //passive --in game. 
						}HLBStatE;

//! Kinds of Tempory Buddies. Any buddy can be many, or none of these..
typedef enum  HLBTempBuddyTypeE {
							HLB_TEMPBUDDYTYPE_MESG	= (1<<0),	  //Just sent a mesg. 1  (MUST be first in enum)
							HLB_TEMPBUDDYTYPE_CHAL  = (1<<1),     //Challenge buddy. 2
                            HLB_TEMPBUDDYTYPE_TOUR  = (1<<2),	  //Tournament. 4
						    HLB_TEMPBUDDYTYPE_VOIP  = (1<<3),	  //Voice Chat. 8
							HLB_TEMPBUDDYTYPE_CUST1  = (1<<4),	  //Client customization1 16
							HLB_TEMPBUDDYTYPE_CUST2  = (1<<5),	  //Client customization2 32
                            HLB_TEMPBUDDYTYPE_SESSION  = (1<<6),   //Track Xbox Session buds. Wont be counted against
                                                                  //limit of 10 temp buds as MS requires at least 10 of these.

                            HLB_TEMPBUDDYTYPE_MAX	= (1<<7)	  // make this the last, always.
						} HLBTempBuddyTypeE;


 //! Invite responses. Apply to buddy (wannabe) requests  and to (cross) game invites.
typedef enum HLBInviteAnswerE {
 					HLB_INVITEANSWER_ACCEPT =BUDDY_INVITE_RESPONSE_ACCEPT, //accept invite
                    HLB_INVITEANSWER_REJECT =BUDDY_INVITE_RESPONSE_REJECT, //no thank you.
 					HLB_INVITEANSWER_REVOKE =BUDDY_INVITE_RESPONSE_REVOKE, //I changed my mind, please un-invite.  
 					HLB_INVITEANSWER_BLOCK  =BUDDY_INVITE_RESPONSE_BLOCK,  //NO and remove and block bud--apply to Buddy Invites only.
 					HLB_INVITEANSWER_REMOVE =BUDDY_INVITE_RESPONSE_REMOVE, //NO and remove you as my buddy--apply to Game Invites only.
					HLB_INVITEANSWER_TIMEOUT =BUDDY_INVITE_RESPONSE_TIMEOUT,
					HLB_INVITEANSWER_UNKNOWN =BUDDY_INVITE_RESPONSE_UNKNOWN_MAX 
 				} HLBInviteAnswerE;

//! Flags to control what features on injecting a msg --see HLBMsgListMsgInject
typedef enum HLBMsgInjectE {
                    HLB_MSGINJECT_AS_ADMIN_MSG = 1 //ignore username, and set as from an admin i.e. ea_admin
 				} HLBMsgInjectE;

#define HLB_MSGINJECT_PARMS_TAG "PARMS" //!<Tag used for secret parms message injection. 

//! Cross Game invite can have many states..
#define HLB_FLAG_GAME_INVITE_SENT		BUDDY_FLAG_GAME_INVITE_SENT   
#define HLB_FLAG_GAME_INVITE_RECEIVED	BUDDY_FLAG_GAME_INVITE_RECEIVED   
#define HLB_FLAG_GAME_INVITE_ACCEPTED	BUDDY_FLAG_GAME_INVITE_ACCEPTED   
#define HLB_FLAG_GAME_INVITE_READY		BUDDY_FLAG_GAME_INVITE_READY
#define HLB_FLAG_GAME_INVITE_TIMEOUT	BUDDY_FLAG_GAME_INVITE_TIMEOUT


//!status codes and last op status etc.
#define HLB_ERROR_NONE  BUDDY_ERROR_NONE      //!< no error
#define HLB_ERROR_PEND  BUDDY_ERROR_PEND      //!< command is still pending
#define HLB_ERROR_NOTUSER BUDDY_ERROR_NOTUSER     //!< not a user
#define HLB_ERROR_BADPARM  BUDDY_ERROR_BADPARM     //!< bad parameter
#define HLB_ERROR_OFFLINE  BUDDY_ERROR_OFFLINE     //!< server is offline
#define HLB_ERROR_MISSING  BUDDY_ERROR_MISSING     //!< item not found etc.
#define HLB_ERROR_FULL     BUDDY_ERROR_FULL      //!< no space, hit max etc.
#define HLB_ERROR_MEMORY     BUDDY_ERROR_MEMORY      //!< out of memory, possibly user alloc space full?
#define HLB_ERROR_UNSUPPORTED BUDDY_ERROR_UNSUPPORTED
#define HLB_ERROR_SELF    BUDDY_ERROR_SELF  //!< cannot add yourself as buddy.
#define HLB_ERROR_BLOCKED    BUDDY_ERROR_BLOCKED  //!< cannot invite-he has blocked you.


#define HLB_ERROR_PASSWORD  -101
#define HLB_ERROR_HACK	    -102
#define HLB_ERROR_INTERNAL  -103
#define HLB_ERROR_BAD_RESOURCE -104
#define HLB_ERROR_NETWORK   -105
#define HLB_ERROR_AUTH      -106
#define HLB_ERROR_TIMEOUT   -107 
#define HLB_ERROR_CANCEL    -108        //!< operation was cancelled. 
#define HLB_ERROR_NOOP      -109        //!< there is no operation --
#define HLB_ERROR_USER_STATE   -110     //!< some invite answers need right wannabe state.
#define HLB_ERROR_UNKNOWN  -999         //!< unexpected..

#define HLB_MAX_PERM_BUDDIES     100  //!< max number of permanent buddies.
#define HLB_MAX_BLOCKED_BUDDIES  50   //!< max number of buddies blocked.
#define HLB_MAX_TEMP_BUDDIES     10   //!< max number of temporary buddies.
#define HLB_MAX_MESSAGES_ALLOWED 100  //!< total max messages allowed -- to keep memory constrained

#define HLB_CONNECT_TO  60 //60     //!<seconds for connect timeout.
#define HLB_OP_TO       90 //90     //!<seconds for an operation, like add buddy, to timeout.
                                    // keep high, as, it disconnects and reconnects  if t/o..

#define HLB_XGAME_READY_TO 600 //!< seconds buddy has, after accept Xgame, to show up in this game.
#define HLB_GAME_STATE_LEN   32 //!<X limit to size of game invite game state	
 
//!  State of connecting to server--   
typedef enum   HLBConnectStateE{
							 HLB_CS_Disconnected, 
							 HLB_CS_Connecting, 
							 HLB_CS_Connected, 
							 HLB_CS_ErrorAuth, 
							 HLB_CS_ErrorUser, 
							 HLB_CS_ErrorNetwork, 
							 HLB_CS_ErrorServer,
							 HLB_CS_ErrorTimeout 
					} HLBConnectStateE;
//!   Messages from Server --TBD 

typedef enum HLBVOIPE{
                        HLB_VOIP_NONE = 0, //!< not doing much
                        HLB_VOIP_PLUGGED, //!<  got headset plugged in.
                        HLB_VOIP_TALKING  //!<  plugged in and talking,
            } HLBVOIPE;


//! Possible Async operations...
typedef enum  HLBOpE { 
     HLB_OP_NONE =0,
     HLB_OP_CONNECT=1,   
     HLB_OP_ADD_BUD,  
     HLB_OP_DELETE_BUD, 
     HLB_OP_BLOCK_BUD, 
     HLB_OP_LOAD, 
     HLB_OP_FIND_USERS, 
     HLB_OP_SEND_MSG,
     HLB_OP_SET_FORWARDING,
     HLB_OP_INVITE,
     HLB_OP_ANY 
    } HLBOpE;


//! tell if buddies or msg from buddy changed or not. -1 cause 0 is valid.
#define HLB_NO_CHANGES -1
#define HLB_CHANGED 99 

//! how big a presence string
#define HLB_PRESENCE_LEN 100

//! how big  game invite session id string 
#define HLB_SESSION_ID_LEN BUDDY_SESSION_ID_LEN

//!   Only one group needed to send messages to.
#define HLB_GROUP_ID 1

//! How is Chat messages encoded in line protocol.
#define HLB_MSG_TYPE_CHAT            BUDDY_MSG_CHAT
#define HLB_MSG_TYPE_INJECT          'injc'

#define HLB_CHAT_KIND_CHAT 'chat'     //!< kind of chat message --just chat
//#define HLB_CHAT_KIND_CLUBRECRUIT 'club'  //!< chat message -- a special recruit
#define HLB_CHAT_KIND_ADMIN 'admn'        //!< chat message --a special from eaadmin..


//! maximum message length.
#define HLB_ABSOLUTE_MAX_MSG_LEN        200
//! maximum size of buddy name.
#define HLB_BUDDY_NAME_LEN            32

//! Number of seconds the server will wait to deliver the message
#define HLB_MSG_KEEP_SECS          259200     

//! Reference to this API -- private
typedef struct   HLBApiRefT  HLBApiRefT;  //HL Api Ref --private 

typedef struct  HLBBudT HLBBudT;

//
// operation callback function prototype
typedef void (HLBOpCallbackT)(HLBApiRefT *pRef, /*HLBOpE*/ int32_t op,  int32_t opStatus , void *pContext ); 

// message callback function prototype
typedef void (HLBMsgCallbackT)(HLBApiRefT *pRef,  BuddyApiMsgT *pMsg , void *pContext ); 

// game invites, or future buddy state change callback function prototype
typedef void (HLBBudActionCallbackT)(HLBApiRefT *pRef,  HLBBudT  *pBuddy, int32_t eAction, void *pContext ); 

//! Callback for when Buddy's presence changes--either state or text string can change.
typedef void (HLBPresenceCallbackT)(HLBApiRefT *pRef,  HLBBudT  *pBuddy,                         
                                                   HLBStatE eOldState,   void *pContext );

//! Function to register for custom sort. Use pRecptr1->pUserRef to get to the buddy.
typedef int32_t (HLBRosterSortFunctionT) ( void *pSortref, int32_t iSortcon, void *pRecptr1, void *pRecptr2);

//HLBBud  -- The Buddy ----represent your buddy or someone who sent you a message etc. 
//Throughout API, assumes buddy name is globally unique, and hence is  key.

//BUDDY Class

#ifdef __cplusplus
extern "C" {
#endif
// Get user name of this buddy. Local.
char * HLBBudGetName(  HLBBudT  *pBuddy );   

// Xenon aka Xbox360  ONLY --get XUID of buddy.
uint64_t HLBBudGetXenonXUID(  HLBBudT  *pBuddy );

#if defined(DIRTYCODE_PS3) || defined(DIRTYCODE_PSP2)
// Sony ONLY --get SceNpID of buddy.
void HLBBudGetSceNpId(HLBBudT *pBuddy, SceNpId *pSceNpId);
#endif 

#if defined(DIRTYCODE_PS3)
//! Callback for when Buddy's presence changes--either state or text string can change.
typedef void (HLBSceNpBasicEventCallbackT)(int iEvent, int iRetCode, uint32_t uRequestId, void *pContext);

// PS3 ONLY -- Set the callback that HlBudApi will forward sceNpBasicRegisterHandler() events to.
// NOTE: Do not block or call a PS3 SDK API within this callback.
void HLBBudSetSceNpBasicEventCallback(HLBApiRefT *pHLBApi, HLBSceNpBasicEventCallbackT *pSceNpBasicEventCallback, void *pCallbackContext);
#endif

//Get the state of Cross Game invites --Xbox only!
uint32_t HLBBudGetGameInviteFlags( HLBBudT  *pBuddy  ) ;

// Does Buddy have a headset plugged in, talking or none? Returns VOIPState_Enum. Local;.
int32_t HLBBudGetVOIPState (  HLBBudT  *pBuddy);

// Does Buddy have my game and is ready to talk?
int32_t HLBBudCanVoiceChat (  HLBBudT  *pBuddy);  

// Someone who sent me a message, may not be a real buddy. Local.
int32_t HLBBudIsTemporary(  HLBBudT *pBuddy );   

// Do I ignore this person? Local.
int32_t HLBBudIsBlocked(  HLBBudT  *pBuddy );    


// This is really a buddy, dual commited. Local.
int32_t HLBBudIsRealBuddy( HLBBudT  *pBuddy ); 

 //  Has this buddy, has invited me to be his buddy. Local.
int32_t HLBBudIsWannaBeMyBuddy( HLBBudT  *pBuddy ); 

 //  Have I invited this user to be my buddy. Local.
int32_t HLBBudIsIWannaBeHisBuddy( HLBBudT  *pBuddy ); 
 
 // Return Title played by this buddy, based on language set in Presence.  
char * HLBBudGetTitle( HLBBudT  *pBuddy);

// Is this buddy one of these temp buddy kinds -- can be true for permanent buddies too.
int32_t HLBBudTempBuddyIs(  HLBBudT  *pBuddy, int32_t tempBuddyType ); 

// It this buddy a member of group that's been set up to send msg. See HLBListAddToGroup.
int32_t HLBBudIsInGroup(  HLBBudT  *pBuddy );

// Get buddy state  --if need support for states other than ones below..
int32_t HLBBudGetState(  HLBBudT  *pBuddy);

// Is buddy playing a  game? -- ie is he in passive state.
int32_t HLBBudIsPassive(  HLBBudT  *pBuddy);

// Is buddy Online and in chat mode?
int32_t HLBBudIsAvailableForChat (  HLBBudT  *pBuddy);

// Does buddy logged into same game? Local.
int32_t HLBBudIsSameProduct (  HLBBudT  *pBuddy); 

// Can I join this buddy -- in Xbox, from presence.
int32_t HLBBudIsJoinable(  HLBBudT  *pBuddy);

//Join buddy in another game; He MUST be online,joinable in other title. 
int32_t HLBBudJoinGame( HLBBudT *pBuddy, HLBOpCallbackT *cb , void * pContext );

// Is buddy available for my game -- must be online and not playing.   
int32_t HLBBudCanPlayMyGame(  HLBBudT  *pBuddy);  

// Is 'special' temp (i.e.admin)  buddy who WONT accept messages or invites etc.   
int32_t HLBBudIsNoReplyBud(  HLBBudT  *pBuddy);  

// Return presence, if not, make default one based on other's product.Local.
void HLBBudGetPresence( HLBBudT  *pBuddy , char * presence); 

// Return any extra  presence string  other client may have sent -eg score of game buddy is playing.
void HLBBudGetPresenceExtra( HLBBudT  *pBuddy , char * strExtraPres, int32_t iStrSize );

//! Set or reset joinable status in presence; 
int32_t HLBApiPresenceJoinable ( HLBApiRefT  * pApiRef, int32_t bOn , int32_t bSendImmediate);
//
//MESSAGE LIST and Messages 
//
/*!
<b>HLBMsgList</b>  -- Buddy's Message List  -- List of messages from a buddy 
Each buddy has a message list. Actual data structure used is  TBD.
This will contain all the messages from that buddy, as the list fills up (hits max msg per buddy)
the earlier messages will be deleted. 
Assumes single threaded model and Update() not called while iterating through message list. 

*/



// Get message at index position, if read is true, mark message as read. Local.
BuddyApiMsgT *  HLBMsgListGetMsgByIndex(  HLBBudT  *pBuddy, int32_t index, int32_t read ); 

// Return  first unread, starting from startPos. If read is true, mark message as read.
BuddyApiMsgT *HLBMsgListGetFirstUnreadMsg (  HLBBudT  *pBuddy, int32_t startPos , int32_t read);

// Inject a message, as if from some buddy. Message callbacks etc will be called.  

int32_t HLBMsgListMsgInject (  HLBApiRefT  * pApiRef, int32_t iKind, const char *pSenderName, const char *pMsgBody,  
                           const char *pMsgParams, /* HLBMsgInjectE */uint32_t uMsgInjectFlags, uint32_t uTime);

//Get the  BODY of message, text, converting from utf8  to 8 bit chars, if trans table set.
void HLBMsgListGetMsgText (HLBApiRefT  * pApiRef,BuddyApiMsgT *pMsg, char * strText, int32_t iLen);

// Get number of unread messages from this buddy. Local. ?todo do we need?
int32_t HLBMsgListGetUnreadCount( HLBBudT  *pBuddy  ); 

// Get total number of messages from this buddy. Local.
int32_t HLBMsgListGetTotalCount ( HLBBudT  *pBuddy  );   

// Zap msg at index position. Local.
void HLBMsgListDelete(  HLBBudT *pBuddy, int32_t index );       

// Delete all, or just read msgs from this buddy. Local. If bZapReadOnly is true will delete just read msgs.
void HLBMsgListDeleteAll(  HLBBudT  *pBuddy, int32_t bZapReadOnly   ); 

//
//BUDDY LIST API -- the list of buddys -- add, send a msg etc.
//

// HLBList -- The buddy list -- The user's  list of buddies.
// Get a Buddy by name. Local.
HLBBudT *HLBListGetBuddyByName ( HLBApiRefT *  pApiRef  , const char * name );

// Get a Buddy by index into the buddy list. Local.
HLBBudT  *HLBListGetBuddyByIndex ( HLBApiRefT  * pApiRef, int32_t index  );

// Given a buddy name, find the index into the list. Local. 
int32_t HLBListGetIndexByName ( HLBApiRefT  * pApiRef, const char * name );  

// Get a count of buddies by various flags (types).
int32_t HLBListGetBuddyCountByFlags ( HLBApiRefT  *pApiRef, uint32_t uTypeFlag);

// Return count of all  buddies in list; TODO -- add support for type?
int32_t HLBListGetBuddyCount ( HLBApiRefT  *pApiRef);  

// Ignore this buddy. Async --will call OpCallback if registered.
int32_t HLBListBlockBuddy (   HLBApiRefT  *pApiRef, const char * name , HLBOpCallbackT *cb , void * pContext ) ; 

// No longer Ignore this buddy. Async --will call OpCallback if registered.
int32_t HLBListUnBlockBuddy ( HLBApiRefT  *pApiRef,   const char * name,   HLBOpCallbackT *cb , void * pContext );

// Drastic, no callback function to nuke all invites, prior to say, Block a buddy.
int32_t HLBListCancelAllInvites ( HLBApiRefT  *pApiRef,  const char  *pName);

 // Request that this person  be my buddy. Async. Will be added to my IWannabeHisBuddy list.
int32_t HLBListInviteBuddy( HLBApiRefT  *pApiRef, const char * name , HLBOpCallbackT *cb , void * pContext );  
 
// Respond to an invite
int32_t HLBReceiveMsgAttachment(HLBApiRefT *pHLBApi, void *pContext);

// Respond to an invite or change state of one i sent, --applies to wannabe buddy invites only
int32_t HLBListAnswerInvite  (  HLBApiRefT  *pApiRef,  const char * name , 
									/*HLBInviteAnswerE */ int32_t  iInviteAction, 
 						 			HLBOpCallbackT *cb , void * pContext ); 

// Respond to a game invite or revoke  of one i sent. Apply to game invites only.
int32_t HLBListAnswerGameInvite  (  HLBApiRefT  *pApiRef,  const char * name , 
									/*HLBInviteAnswerE*/ int32_t iInviteAction, 
 						 			HLBOpCallbackT *cb , void * pContext ); 
// Cancel any game invites I sent -- also see HLBListAnswerGameInvite
int32_t HLBListCancelGameInvite  (  HLBApiRefT  *pApiRef, HLBOpCallbackT *cb , void * pContext ); 
    
// Send Answerable IM response message, to server.
//int32_t HLBListSendAnswerResponseMsg(  HLBApiRefT  * pApiRef, int32_t *msgId, int32_t buttonId, char * responseText);

// Invite buddy to game, he MUST be in another title. Async.
int32_t HLBListGameInviteBuddy( HLBApiRefT  *pApiRef, const char * name , const char * pGameInviteState,
                           HLBOpCallbackT *cb , void * pContext );  

// Ensure user is no longer a buddy of mine. Async.
int32_t HLBListUnMakeBuddy( HLBApiRefT  *pApiRef, const char * name , HLBOpCallbackT *cb , void * pContext );  

// Flag /add as a tourney,challenge or other buddy. Sync, local call.
int32_t HLBListFlagTempBuddy (  HLBApiRefT  *pApiRef,  const char * name, int32_t tempBuddyType);

 // Unflag from temp buddy; . Sync, local call. 
int32_t HLBListUnFlagTempBuddy (  HLBApiRefT  *pApiRef, const char * name,   int32_t tempBuddyType);

 // Explicitly delete this temp buddy. Will ignore request if buddy is permanent. Sync local call.
int32_t HLBListDeleteTempBuddy (  HLBApiRefT  *pApiRef, const char * name);

//sort function to be set if client wants custom sort of buddy list -- However API will have a reasonable default sort.
void HLBListSetSortFunction (HLBApiRefT  * pApiRef,  HLBRosterSortFunctionT   * sortCallbackFunction  );   // custom sort, will  have default if this is not set

// Sort list of buddies. Once sorted, list will be frozen until re-sorted,  
int32_t  HLBListSort (HLBApiRefT  * pApiRef); 

// Disable Sorting
void HLBListDisableSorting( HLBApiRefT  *  pApiRef );

// Has the  list of  buddies  or their presence changed ?  Local. HLB_NO_CHANGE if none.
int32_t  HLBListChanged(HLBApiRefT  * pApiRef);     

// Has messages from any buddies changed? Local. Returns buddy index, or HLB_NO_CHANGE if none.
int32_t HLBListBuddyWithMsg(HLBApiRefT  * pApiRef);  

// On closing screen, need to clear temp buddies w/o messages etc. 
int32_t HLBListDeleteNoMsgTempBuddies (HLBApiRefT  * pApiRef );  

// Can create a list of buddies, to send a message to. 

// Add user to group to whom I will send a common msg.
int32_t HLBListAddToGroup ( HLBApiRefT  * pApiRef, const char * buddyName); 

// Remove this buddy from group Iam going to send to.
int32_t HLBListRemoveFromGroup ( HLBApiRefT  * pApiRef, const char * buddyName); 

// Clear group of users so I can re-use it. Try not to use -- more efficient to clear group when sending msg.
int32_t HLBListClearGroup ( HLBApiRefT  * pApiRef ); 

// Send message to group of users I created earlier. Reset group if clearGroup is set.
int32_t HLBListSendMsgToGroup (HLBApiRefT  * pApiRef,    int32_t msgType , const char * msg , int32_t bClearGroup,
									HLBOpCallbackT *cb , void * pContext);  

// Send an actual chat message.
int32_t HLBListSendChatMsg(  HLBApiRefT  * pApiRef, const char * name, const char * body,
									HLBOpCallbackT *cb , void * pContext);

// Add a message, to this user, from another app, like Challenge or VOIP.   
//int32_t HLBListAddMsg(  HLBApiRefT  * pApiRef, const char * fromName, int32_t msgType, const char * msg);




// HLBApi  -- the High Level Buddy Api;
// Contains buddy list, which in turn contains list of buddies. Each buddy contains list 
// of messages received


// Create the API. . Local.
HLBApiRefT *HLBApiCreate(  const char *pProduct, const char *pRelease, 
                           int32_t iRefMemGroup, void *pRefMemGroupUserData);

//Create the API, and override memory alloc/free functions.
HLBApiRefT *HLBApiCreate2( BuddyMemAllocT *pAlloc,  BuddyMemFreeT *pFree,
                           const char *pProduct, const char *pRelease, 
                           int32_t iRefMemGroup, void *pRefMemGroupUserData);

// Set a callback to write out  debug messages within API. Local.
void HLBApiSetDebugFunction (  HLBApiRefT  * pApiRef, void *pDebugData, void (*pDebugProc)(void *pData, const char *pText));

// Application  may override default constants.-1 leaves it as default. Only valid before HLBApiConnect is called
void HLBApiOverrideConstants (HLBApiRefT  * pApiRef, 
        int32_t bNoTextSupport,
        int32_t connect_timeout, 
        int32_t op_timeout,  
        int32_t max_perm_buddies, 
        int32_t max_temp_buddies,
        int32_t msgs_per_perm_buddy,
        int32_t msgs_per_temp_buddy,
        int32_t max_msg_len,
        int32_t bTrackSendMsgInOpState,
        int32_t iGameInviteAccept_timeout);

//!Application  may override default for max msg per buddy -- FIFA emergency.
void HLBApiOverrideMaxMessagesPerBuddy (HLBApiRefT  * pApiRef, int32_t  max_msgs_per_perm_buddy);

//! Set to 1 if on XBL and NTSC cannot play each other, but, have same title id.
void HLBApiOverrideXblIsSameProductCheck (HLBApiRefT  * pApiRef, int32_t bDontUseXBLSameTitleIdCheck);

// Configure  and initialize any data structures in the API. Local.
int32_t HLBApiInitialize( HLBApiRefT  * pApiRef, const char * prodid , uint32_t uLocality ); 

// Connect to buddy server. Local. key can be lkey or user:password. Register connect c/b or poll to find state of connect.
int32_t HLBApiConnect( HLBApiRefT  * pApiRef, const char * pServer, int32_t port,  const char *key, const char *pRsrc   ); 

// What is state of connection to the server.  see HLBConnectState_Enum . Local. Can be used to poll state instead of callback.
 int32_t HLBApiGetConnectState ( HLBApiRefT  * pApiRef);

// Informs caller when connect, disconnect or a re-connect happens. Not neeeded if polling. Local.
int32_t HLBApiRegisterConnectCallback( HLBApiRefT  * pApiRef, HLBOpCallbackT * pMsgCallback, void * pAppData);

// Informs caller when Cross Game Invite activity happens. Not neeeded if polling. Local.
int32_t HLBApiRegisterMsgAttachmentCallback( HLBApiRefT  * pApiRef, HLBBudActionCallbackT * pActionCallback, void * pAppData);

// Informs caller when Cross Game Invite activity happens. Not neeeded if polling. Local.
int32_t HLBApiRegisterGameInviteCallback( HLBApiRefT  * pApiRef, HLBBudActionCallbackT * pActionCallback, void * pAppData);

//! Optional callback --Informe caller when buddy presence changes --state or text.
int32_t HLBApiRegisterBuddyPresenceCallback( HLBApiRefT * pApiRef, HLBPresenceCallbackT * pListCallback, void * pAppData);

// Disconnect  from server. Local.
void HLBApiDisconnect ( HLBApiRefT  * pApiRef) ;  

// Destroy api and any memory held. Local.
void HLBApiDestroy( HLBApiRefT   *pState);       

// Give API and lower levels some time to process. must be called periodically.  Local.
void HLBApiUpdate (HLBApiRefT  * pApiRef) ;  

// Suspend buddy api, cleanup and put buddy in passive state --used when want to go into game. Local.
int32_t  HLBApiSuspend( HLBApiRefT  * pApiRef); 

// Refresh roster  and put buddy in online state -- used  on return from game, or on startup. Async.
void *  HLBApiResume(HLBApiRefT  * pApiRef);  

// state of last operation --Implies only one operation at  a time. See HLB_ERROR_XXX for status.
int32_t HLBApiGetLastOpStatus (HLBApiRefT  * pApiRef) ;

// cancel any pending operations -- will clear any operational state and call op callback, with status HLB_ERROR_CANCEL.
void HLBApiCancelOp (HLBApiRefT  * pApiRef) ;

// state of last operation --pending, done, error etc.Implies only one operation of a given type at time. Local.     
//int32_t   HLBApiGetOpStatus ( HLBApiRefT  * pApiRef, /* HLBOp_Enum */  int32_t  opEnum ) ; 

//Find a user  or list of users based on wildcard search --clears UsersFound array. Async.
int32_t HLBApiFindUsers( HLBApiRefT  * pApiRef, const char * name , HLBOpCallbackT * pOpCallback, void * pAppData) ;

// Return  user name from last FindUser operation. Local, Network. Null item not found.
BuddyApiUserT *HLBApiUserFound ( HLBApiRefT  * pApiRef , int32_t iItem);

// Inform caller on new message waiting. Not neeeded if polling. Local.
int32_t HLBApiRegisterNewMsgCallback( HLBApiRefT  * pApiRef, HLBMsgCallbackT  *pMsgCallback, void * pAppData ); 

// Inform caller on buddy list  or buddy presence change.  Not neeeded if polling. Local.
int32_t HLBApiRegisterBuddyChangeCallback( HLBApiRefT  * pApiRef, HLBOpCallbackT * pMsgCallback, void * pAppData); 


//Set my presence as "Offline" if OnOff is true.Reset if bOnOff is false.
//Once set, will be remembered, and override any other Presence settings.
int32_t HLBApiPresenceOffline (  HLBApiRefT  * pApiRef, int32_t bOnOff);

//Set presence with string for other games.   
void HLBApiPresenceDiff (  HLBApiRefT  * pApiRef,  const char * globalPres);

//! Set presence my presence state and presence string for this product
void HLBApiPresenceSame (  HLBApiRefT  * pApiRef, int32_t iPresState, const char * strSamePres);

//Set "extra"  presence for scores etc, must call HLBApiPresenceSend to send.
void HLBApiPresenceExtra (  HLBApiRefT  * pApiRef,  const char * strExtraPres);

//Send presence msg, with string for this game and others. 
int32_t HLBApiPresenceSend ( HLBApiRefT  * pApiRef,  /*HLBStatE*/int32_t iBuddyState , int32_t VOIPState, const char * gamePres,   int32_t waitMsecs);

// Send presence related to this user's voip i.e. headset status.
int32_t  HLBApiPresenceVOIPSend ( HLBApiRefT  * pApiRef,  /*HLBVOIPE*/ int32_t iVOIPState );

//Send previously set presence. 
int32_t HLBApiPresenceSendSetPresence ( HLBApiRefT  * pApiRef);

// Set or clear my msg forwarding to email/sms etc.
int32_t HLBApiSetEmailForwarding( HLBApiRefT  * pApiRef,int32_t bEnable, const char * strEmail, 
															HLBOpCallbackT * pOpCallback, void *pCalldata );

int32_t HLBApiGetEmailForwarding( HLBApiRefT  * pApiRef, int32_t *bEnable, char * strEmail );

// Get my msg forwarding flags and email address etc if set.  
int32_t HLBApiGetForwarding( HLBApiRefT  * pApiRef, unsigned *puForwardingFlags, char * strEmail );

//!Set if want api to translate utf8 chars to 8 bit characters in Presence, and Message Text.
void HLBApiSetUtf8TransTbl ( HLBApiRefT  * pApiRef, Utf8TransTblT *pTransTbl);

//!Get the "Official" title name for this game. pLang ignored for now--defaults to English. 
int32_t HLBApiGetMyTitleName (HLBApiRefT  * pApiRef,  const char * pLang, const int32_t iBufSize, char * pTitleName);

//!Get the " Official" Xbox title name for a given title, based on my locale-language.
int32_t HLBApiGetTitleName (HLBApiRefT  * pApiRef, const uint32_t uTitleId,  const int32_t iBufSize, char * pTitleName);

// Set the SessionID that is passed to the game invitation --Xbox Only
int32_t HLBListSetGameInviteSessionID(HLBApiRefT *pApiRef, const char *sSessionID);


// Get the SessionID for a particular user --works on Xbox only -- for game invites
int32_t HLBListGetGameSessionID(HLBApiRefT *pApiRef, const char * name, char *sSessionID, const int32_t iBufSize);

// set the Enumerator index used only by xbox360 
int32_t HLBApiSetUserIndex(HLBApiRefT *pApiRef, int32_t iEnumeratorIndex);

// Get the Enumerator index used only by xbox360 
int32_t HLBApiGetUserIndex(HLBApiRefT *pApiRef);

#if defined(DIRTYCODE_PS3) || defined(DIRTYCODE_PSP2)
void HLBAddPlayersToHistory(HLBApiRefT *pHLBApi, DirtyAddrT *pPlyrAddrs[], int32_t iNumAddrs);
#endif

#ifdef __cplusplus
}
#endif

//@}
#endif // _HLBApi_h
