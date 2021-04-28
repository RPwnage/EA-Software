/// \defgroup defs Defines
/// \brief This module contains a wide variety of defines used by a variety of Origin functions. 

#ifndef __ORIGIN_DEFINES_H__
#define __ORIGIN_DEFINES_H__

/// \ingroup defs
/// \brief The version number of this SDK.
/// 
#define ORIGIN_SDK_VERSION			2

/// \ingroup defs
/// \brief The version string that the Origin Client will see in an connection attempt.
/// 
#define ORIGIN_SDK_VERSION_STR		"9.5.0.3"

/// \ingroup defs
/// \brief This converts the component parts of a version number into a single number.
///
#define ORIGIN_VERSION(a,b,c,d)		((a)<<24) + ((b)<<20) + ((c)<<16) + (d)

/// \ingroup defs
/// \brief The minimal version of the Origin necessary to run this SDK.
/// 
/// \note This will change for each release. Please Update When Necessary.
#define ORIGIN_MIN_ORIGIN_VERSION		ORIGIN_VERSION(9,4,0,0)

/// \ingroup defs
/// \brief The other user is not a friend of this user.
/// 
#define ORIGIN_FRIEND_NONE			0

/// \ingroup defs
/// \brief The other user is a friend of this user.
/// 
#define ORIGIN_FRIEND_MUTUAL			1

/// \ingroup defs
/// \brief The other user invited this user to be your friend.
/// 
#define ORIGIN_FRIEND_INVITED		2

/// \ingroup defs
/// \brief The other user declined your friend invitation.
/// 
#define ORIGIN_FRIEND_DECLINED		3

/// \ingroup defs
/// \brief The other user has requested you to be their friend.
/// 
#define ORIGIN_FRIEND_REQUEST		4


/// \ingroup defs
/// \brief The in-game overlay action (param: \ref uint32_t).
/// 
#define ORIGIN_EVENT_IGO					1<<ORIGIN_EVENTINDEX_IGO

/// \ingroup defs
/// \brief The user accepted a game invite (param: OriginInviteT).
/// 
#define ORIGIN_EVENT_INVITE				1<<ORIGIN_EVENTINDEX_Invite

/// \ingroup defs
/// \brief A network event has occurred (param: ORIGIN_LOGINSTATUS_*; see \ref ORIGIN_LOGINSTATUS_OFFLINE, etc.).
/// 
#define ORIGIN_EVENT_LOGIN				1<<ORIGIN_EVENTINDEX_Login

/// \ingroup defs
/// \brief The local user profile changed (param: \ref uint32_t).
/// 
#define ORIGIN_EVENT_PROFILE			1<<ORIGIN_EVENTINDEX_Profile

/// \ingroup defs
/// \brief A subscribed presence has changed (param:  user id, \ref OriginUserT).
/// 
#define ORIGIN_EVENT_PRESENCE			1<<ORIGIN_EVENTINDEX_Presence

/// \ingroup defs
/// \brief The friends list has changed (param: OriginFriendT).
/// 
#define ORIGIN_EVENT_FRIENDS			1<<ORIGIN_EVENTINDEX_Friends

/// \ingroup defs
/// \brief The user just purchased an item (param:  manifest id, string).
/// 
#define ORIGIN_EVENT_PURCHASE			1<<ORIGIN_EVENTINDEX_Purchase

/// \ingroup defs
/// \brief The user just downloaded an item (param: OriginDownloadT).
/// 
#define ORIGIN_EVENT_CONTENT			1<<ORIGIN_EVENTINDEX_Content

/// \ingroup defs
/// \brief The user just added or removed somebody from the block list, please refresh.
/// 
#define ORIGIN_EVENT_BLOCKED			1<<ORIGIN_EVENTINDEX_Blocked

/// \ingroup defs
/// \brief Event indicating that the user received a chat message (param OriginChatT).
/// 
#define ORIGIN_EVENT_CHAT_MESSAGE		1<<ORIGIN_EVENTINDEX_ChatMessage

/// \ingroup defs
/// \brief Event indicating that the user is invited to a chat session (param OriginCharT *)
/// 
#define ORIGIN_EVENT_CHAT_INVITE        1<<ORIGIN_EVENTINDEX_ChatInvite

/// \ingroup defs
/// \brief Event indicating that a chat session is created (param OriginCharT *)
/// 
#define ORIGIN_EVENT_CHAT_CREATED        1<<ORIGIN_EVENTINDEX_ChatCreated

/// \ingroup defs
/// \brief Event indicating that the chat session has finished. (param OriginCharT *)
/// 
#define ORIGIN_EVENT_CHAT_FINISHED      1<<ORIGIN_EVENTINDEX_ChatFinished

/// \ingroup defs
/// \brief Event indicating that the chat session has been updated. (param OriginCharT *)
/// 
#define ORIGIN_EVENT_CHAT_UPDATED      1<<ORIGIN_EVENTINDEX_ChatUpdated

/// \ingroup defs
/// \brief The user received a online status event (param bool).
/// 
#define ORIGIN_EVENT_ONLINE				1<<ORIGIN_EVENTINDEX_Online

/// \ingroup defs
/// \brief The user receives information about the achieved achievement. (\ref OriginAchievementSetT)
/// 
#define ORIGIN_EVENT_ACHIEVEMENT		1<<ORIGIN_EVENTINDEX_Achievement

/// \ingroup defs
/// \brief The user receive information about the currently pending invite. (\ref OriginInvitePendingT)
/// 
#define ORIGIN_EVENT_INVITE_PENDING		1<<ORIGIN_EVENTINDEX_InvitePending

/// \ingroup defs
/// \brief The user received the new visibility state. 
/// 
#define ORIGIN_EVENT_VISIBILITY		1<<ORIGIN_EVENTINDEX_Visibility

/// \ingroup defs
/// \brief The user received the new broadcast state. 
/// 
#define ORIGIN_EVENT_BROADCAST		1<<ORIGIN_EVENTINDEX_Broadcast


/// \ingroup defs
/// \brief Current user presence has changed.
/// 
#define ORIGIN_EVENT_CURRENT_USER_PRESENCE			1<<ORIGIN_EVENTINDEX_CurrentUserPresence

/// \ingroup defs
/// \brief The user acquired new items. The items may be unrelated to the game receiving them.
/// 
#define ORIGIN_EVENT_NEW_ITEMS		1<<ORIGIN_EVENTINDEX_NewItems

/// \ingroup defs
/// \brief The chunk status for the progressive installation has been updated.
/// 
#define ORIGIN_EVENT_CHUNK_STATUS		1<<ORIGIN_EVENTINDEX_ChunkStatus

/// \ingroup defs
/// \brief This event will indicate that the IGO is not available.
/// 
#define ORIGIN_EVENT_IGO_UNAVAILABLE		1<<ORIGIN_EVENTINDEX_IgoUnavailable

/// \ingroup defs
/// \brief The IGO was deactivated (the game can proceed).
/// 
#define ORIGIN_IGOSTATE_DOWN			0

/// \ingroup defs
/// \brief The IGO was activated (it now has input focus and is fully visible).
/// 
#define ORIGIN_IGOSTATE_UP			1


/// \ingroup defs
/// \brief The IGO couldn't be shown for reason
#define ORIGIN_IGO_UNAVAILABLE_REASON_RESULTION_TOO_LOW		0x80000001

/// \ingroup defs
/// \brief The client is not logged in.
/// 
#define ORIGIN_LOGINSTATUS_OFFLINE				0

/// \ingroup defs
/// \brief The client is logged in and connected.
/// 
#define ORIGIN_LOGINSTATUS_ONLINE				1

/// \ingroup defs
/// \brief The client is logged in, but Core is not connected.
/// 
#define ORIGIN_LOGINSTATUS_ONLINE_DISCONNECTED	2

/// \ingroup defs
/// \brief Specify this flag when running the unit test.
/// 
/// This will prevent the SDK from starting Origin and loading IGO.
#define ORIGIN_FLAG_UNIT_TEST_MODE				(1<<8)

/// \ingroup defs
/// \brief Specify this flag when you do not want OriginSDK to start Origin.
/// 
/// This will prevent the SDK from starting Origin and loading IGO.
#define ORIGIN_FLAG_DO_NOT_START_ORIGIN			(1<<9)

/// \ingroup defs
/// \brief Enable multiple instances of the Origin SDK
/// 
/// When this flag is set multiple instances of the Origin SDK will be allowed.
#define ORIGIN_FLAG_ALLOW_MULTIPLE_SDK_INSTANCES (1<<10)

/// \ingroup defs
/// \brief Force the game specified contentId over the environment provided contentId.
/// 
/// When this flag is set the OriginSDK will not get the contentId from the environment, otherwise
/// it will prefer the environment provided contentId over the game provided contentId.
#define ORIGIN_FLAG_FORCE_CONTENT_ID (1<<11)

/// \ingroup defs
/// \brief Shows navigation bars.
/// 
#define ORIGIN_IGO_BROWSER_NAV					(1<<0)

/// \ingroup defs
/// \brief Opens the URL in a new tab.
/// 
#define ORIGIN_IGO_BROWSER_TAB					(1<<1)

/// \ingroup defs
/// \brief Makes the current browser window/tab active.
/// 
#define ORIGIN_IGO_BROWSER_ACTIVE				(1<<2)

/// \ingroup defs
/// \brief The permission return code when a permission is denied.
/// 
#define ORIGIN_PERMISSION_DENIED						0

/// \ingroup defs
/// \brief The permission is granted in all situations.
/// 
#define ORIGIN_PERMISSION_GRANTED					1

/// \ingroup defs
/// \brief The permission is granted but only for friends.
/// 
#define ORIGIN_PERMISSION_GRANTED_FRIENDS_ONLY		2

/// \ingroup defs
/// \brief Tells the \ref OriginReadEnumeration and \ref OriginReadEnumerationSync functions to read the total amount of data. 
/// 
#define ORIGIN_READ_ALL_DATA			0xFFFFFFFF	///< 

/// \ingroup defs
/// \brief Port on Origin to which SDK/game connects to exchange LSX messages.
///
#define ORIGIN_LSX_PORT 3216

#endif //__ORIGIN_DEFINES_H__
