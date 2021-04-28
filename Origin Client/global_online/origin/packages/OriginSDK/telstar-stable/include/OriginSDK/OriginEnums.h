#ifndef __ORIGIN_ENUMS_H__
#define __ORIGIN_ENUMS_H__

/// \defgroup enums Enums
/// \brief This module contains a set of enums that are used by a variety of Origin functions.


/// \ingroup enums
/// \brief Defines types of In-Game Overlay windows.
enum enumIGOWindow
{
    IGOWINDOW_LOGIN = 1,            ///< A reference to the login in-game overlay window.
    IGOWINDOW_PROFILE = 2,          ///< A reference to the profile in-game overlay window.
    IGOWINDOW_RECENT = 3,           ///< A reference to the recent players in-game overlay window.
    IGOWINDOW_FEEDBACK = 4,         ///< A reference to the feedback in-game overlay window.
    IGOWINDOW_FRIENDS = 5,          ///< A reference to the friends in-game overlay window.
    IGOWINDOW_FRIEND_REQUEST = 6,   ///< A reference to the friend request in-game overlay window.
    IGOWINDOW_CHAT = 7,             ///< A reference to the chat in-game overlay window.
    IGOWINDOW_COMPOSE_CHAT = 8,     ///< A reference to the chat composition in-game overlay window.
    IGOWINDOW_INVITE = 9,           ///< A reference to the invite in-game overlay window.
    IGOWINDOW_ACHIEVEMENTS = 10,    ///< A reference to the achievements in-game overlay window.
    IGOWINDOW_STORE = 11,           ///< A reference to the store in-game overlay window.
    IGOWINDOW_CODE_REDEMPTION = 12, ///< A reference to the code redemption in-game overlay window.
    IGOWINDOW_CHECKOUT = 13,        ///< A reference to the checkout in-game overlay window.
    IGOWINDOW_BLOCKED = 14,         ///< A reference to the blocked users in-game overlay window.
    IGOWINDOW_BROWSER = 15,         ///< A reference to the browser in-game overlay window.
    IGOWINDOW_FIND_FRIENDS = 16,    ///< A reference to the find friends in-game overlay window.
    IGOWINDOW_CHANGE_AVATAR = 17,   ///< A reference to the change avatar in-game overlay window.
    IGOWINDOW_GAMEDETAILS = 18,     ///< A reference to the change avatar in-game overlay window.
    IGOWINDOW_BROADCAST = 19,       ///< A reference to the broadcast in-game overlay window.
    IGOWINDOW_UPSELL = 20,          ///< A reference to the upsell in-game overlay windows.
};

/// \ingroup enums
/// \brief Defines types of SDK events.
///
/// This enumeration relates directly to the bit position of the LSX event identifier.
enum OriginEventIndexEnum {
    ORIGIN_EVENT_FALLBACK = -1,             ///< The fallback id for defining a fallback callback for events that are not registered .
    ORIGIN_EVENT_IGO,       	            ///< The in-game overlay action (param: \ref uint32_t).
    ORIGIN_EVENT_INVITE,    	            ///< The user accepted a game invite (param: \ref OriginInviteT).
    ORIGIN_EVENT_LOGIN,     	            ///< A user has been logged in or out (param: \ref OriginLoginT).
    ORIGIN_EVENT_PROFILE,   	            ///< The local user profile changed (param: \ref OriginProfileChangeT).
    ORIGIN_EVENT_PRESENCE,  	            ///< A subscribed presence has changed (param:  user id, \ref OriginUserT).
    ORIGIN_EVENT_FRIENDS,   	            ///< The friends list has changed (param: OriginFriendT).
    ORIGIN_EVENT_PURCHASE,  	            ///< The user just purchased an item (param:  manifest id, string).
    ORIGIN_EVENT_CONTENT,		            ///< The user just downloaded an item (param: OriginContentT).
    ORIGIN_EVENT_BLOCKED,		            ///< The user just added or removed somebody from the block list, please refresh.
    ORIGIN_EVENT_ONLINE,		            ///< The user received a online status event (param bool).
    ORIGIN_EVENT_ACHIEVEMENT,			    ///< The user receives information about the achieved achievement. (\ref OriginAchievementSetT)
    ORIGIN_EVENT_INVITE_PENDING,            ///< The user receive information about the currently pending invite. (\ref OriginInvitePendingT)
    ORIGIN_EVENT_CHAT_MESSAGE,	            ///< Event indicating that the user received a chat message (param OriginChatT).
    ORIGIN_EVENT_VISIBILITY,	            ///< The user received the new visibility state.
    ORIGIN_EVENT_BROADCAST,				    ///< The user received the new broadcast state.
    ORIGIN_EVENT_CURRENT_USER_PRESENCE,     ///< Current user presence has changed.
    ORIGIN_EVENT_NEW_ITEMS,                 ///< The user acquired new items. The items may be unrelated to the game receiving them.
    ORIGIN_EVENT_CHUNK_STATUS,              ///< The chunk status for the progressive installation has been updated.
    ORIGIN_EVENT_IGO_UNAVAILABLE,		    ///< This event will indicate that the IGO is not available.
    ORIGIN_EVENT_MINIMIZE_REQUEST,          ///< This event indicates the game should minimize itself (Origin user action required)
    ORIGIN_EVENT_RESTORE_REQUEST,           ///< This event indicates the game should restore itself (if previously minimized)
    ORIGIN_EVENT_GAME_MESSAGE,              ///< Receives a game message sent by another game connection.  receives and \ref OriginGameMessageT structure.
    ORIGIN_EVENT_GROUP,                     ///< This event fires when the members of a room change. The event returns the members of the group (\ref OriginFriendT)
    ORIGIN_EVENT_GROUP_ENTER,               ///< This event fires when the user enters a room. The event returns a \ref OriginGroupInfoT structure.
    ORIGIN_EVENT_GROUP_LEAVE,               ///< This event fires when the user leaves a room. The event returns the groupId of the group that is left.
    ORIGIN_EVENT_GROUP_INVITE,              ///< This event indicates that Origin received an invite for the user to join a party. Use \ref OriginEnterGroup with the specified groupId to join. The event returns a \ref OriginGroupInviteT structure.
    ORIGIN_EVENT_VOIP_STATUS,               ///< This event informs when something changed on the voip channel.
    ORIGIN_EVENT_USER_INVITED,              ///< The user you invited to join your game. (ref OriginUserT *)
    ORIGIN_EVENT_CHAT_STATE_UPDATED,        ///< The users chat state is updated. (\ref OriginChatStateUpdateMessage)
    ORIGIN_EVENT_STEAM_STORE,               ///< Open the steam store. Reserved Event.
    ORIGIN_EVENT_STEAM_ACHIEVEMENT,         ///< Grant steam achievement. Reserved Event.
    ORIGIN_NUM_EVENTS            	        ///< The total number of LSX event identifiers.
};

/// \ingroup enums
/// \brief The users' subscriber level for the subscription
enum enumSubscriberLevel
{
    ORIGIN_SUBSCRIBER_LEVEL_NONE,
    ORIGIN_SUBSCRIBER_LEVEL_BASIC,
    ORIGIN_SUBSCRIBER_LEVEL_PREMIER,
};

/// \ingroup enums
/// \brief Defines users' online presence status.
enum enumPresence
{
    ORIGIN_PRESENCE_UNKNOWN,			        ///< It is unknown which presence the user is in
    ORIGIN_PRESENCE_OFFLINE,					///< The user is offline.
    ORIGIN_PRESENCE_ONLINE,						///< The user is online but not playing a game.
    ORIGIN_PRESENCE_INGAME,						///< The user is online and is running a game.
    ORIGIN_PRESENCE_BUSY,						///< The user is online but busy.
    ORIGIN_PRESENCE_IDLE,						///< The user is online but idle (likely away).
    ORIGIN_PRESENCE_JOINABLE,					///< The user is online and in a joinable game state.
    ORIGIN_PRESENCE_JOINABLE_INVITE_ONLY,		///< The user is online and in a joinable but only for invited users.
};

/// \ingroup enums
/// \brief Defines the state of friend relationships.
enum enumFriendState
{
    ORIGIN_FRIENDSTATE_UNKNOWN,		///< The friend state couldn't be determined/default value used.
    ORIGIN_FRIENDSTATE_NONE,		///< There is no friend relation between you and this user.
    ORIGIN_FRIENDSTATE_MUTUAL,    	///< You are a mutual friend to this user.
    ORIGIN_FRIENDSTATE_INVITED,   	///< You invited this user to be a friend.
    ORIGIN_FRIENDSTATE_DECLINED,  	///< The user declined to be your friend.
    ORIGIN_FRIENDSTATE_REQUEST,   	///< The user is requesting you to be his friend.
    ORIGIN_FRIENDSTATE_ITEM_COUNT 	///< The total number of defined friend relationships.
};

/// \ingroup enums
/// \brief Defines the permissions granted to a user.
enum enumPermission
{
    ORIGIN_PERM_MULTIPLAYER,		///< The user is allowed to play multiplayer games.
    ORIGIN_PERM_PURCHASE,			///< The user is allowed to purchase content.
    ORIGIN_PERM_TRIAL,				///< The current game is in a trial.
};

/// \ingroup enums
/// \brief Defines the settings items.
enum enumSettings
{
    ORIGIN_SETTINGS_LANGUAGE,		      ///< Language setting
    ORIGIN_SETTINGS_ENVIRONMENT,	      ///< Environment setting
    ORIGIN_SETTINGS_IS_IGO_AVAILABLE,	  ///< Return "true" if IGO is available, "false" if IGO is not available
    ORIGIN_SETTINGS_IS_IGO_ENABLED,       ///< Indicate whether the IGO is enabled by the user. If this returns "true" it doesn't mean that the IGO is available for use. Check \ref ORIGIN_SETTINGS_IS_IGO_AVAILABLE to see whether the IGO is actually available.
    ORIGIN_SETTINGS_IS_TELEMETRY_ENABLED, ///< Indicate whether telemetry is enabled by the user (i.e. the user did not opt-out) 
    ORIGIN_SETTINGS_IS_MANUAL_OFFLINE     ///< If Origin is manually offline.
};

/// \ingroup enums
/// \brief Defines the settings items.
enum enumGameInfo
{
    ORIGIN_GAME_INFO_UPTODATE,		        ///< Game up to date flag
    ORIGIN_GAME_INFO_LANGUAGES,             ///< Set of languages that the user is entitled to play the game in
    ORIGIN_GAME_INFO_FREE_TRIAL,            ///< Check whether the game is a free trial.
    ORIGIN_GAME_INFO_EXPIRATION,            ///< Check whether the game has an expiration set.
    ORIGIN_GAME_INFO_EXPIRATION_DURATION,   ///< Returns the number of seconds remaining until expiration.
    ORIGIN_GAME_INFO_INSTALLED_VERSION,     ///< Get the installed version of the game.
    ORIGIN_GAME_INFO_INSTALLED_LANGUAGE,    ///< The language/locale the title is installed in.
    ORIGIN_GAME_INFO_AVAILABLE_VERSION,     ///< Get the available version of the game.
    ORIGIN_GAME_INFO_DISPLAY_NAME,          ///< The name of the game.
    ORIGIN_GAME_INFO_FULLGAME_PURCHASED,    ///< If this game is a free trial and the user has purchased the full game, then this flag will return true.
	ORIGIN_GAME_INFO_FULLGAME_RELEASED,		///< If this game is a free trial and the user owns the full game, then this flag will indicate whether the full game is released.
	ORIGIN_GAME_INFO_FULLGAME_RELEASE_DATE, ///< If this game is a free trial and the user owns the full game, then this flag will indicate when the full game is/will be released.
	ORIGIN_GAME_INFO_MAX_GROUP_SIZE,        ///< The max number of users that can join a group/party created by the game.
    ORIGIN_GAME_INFO_ENTITLEMENT_SOURCE,    ///< Indication where the game is purchased. Valid values are: ORIGIN, SUBSCRIPTION, STEAM, EPIC, UPLAY, AMAZON, etc.
};


/// \ingroup enums
/// \brief Defines the severity of debug messages.
enum enumDebugLevel
{
    ORIGIN_LEVEL_NONE,				///< No debug information.
    ORIGIN_LEVEL_SEVERE,			///< Severe issues only. This maps to \ref ORIGIN_LEVEL_0.
    ORIGIN_LEVEL_MAJOR,				///< Severe and Major issues. This maps to \ref ORIGIN_LEVEL_1.
    ORIGIN_LEVEL_MINOR,				///< Severe, Major and Minor issues. This maps to \ref ORIGIN_LEVEL_2.
    ORIGIN_LEVEL_TRIVIAL,			///< Trivial and Above. This maps to \ref ORIGIN_LEVEL_3.
    ORIGIN_LEVEL_EVERYTHING			///< All errors. This maps to \ref ORIGIN_LEVEL_4.
};

/// \ingroup enums
/// \brief The states content can be in.
enum enumContentState
{
    CONTENT_STATE_DOWNLOADING,		///< The content is being downloaded.
    CONTENT_STATE_INSTALLING,		///< The content is being installed.
    CONTENT_STATE_READY,			///< The content is ready for use
    CONTENT_STATE_BUSY,				///< The content is busy, either with some internal process, or is in use.
    CONTENT_STATE_UNKNOWN,			///< The content is in a bad state.
    CONTENT_STATE_WAITING,			///< The content is waiting for user action.
    CONTENT_STATE_QUEUED,		    ///< The content is queued for download
};

/// \ingroup enums
/// \brief The modes in which achievements can be retreived.
enum enumAchievementMode
{
    ACHIEVEMENT_MODE_ACTIVE,        ///< Query the active achievements. These are the achievements that have progress, or are achieved.
    ACHIEVEMENT_MODE_ALL,           ///< Query all the achievements. This will return all achievements for a game Id including the ones without progress.
};

/// \ingroup enums
/// \brief The type of what changed in the Profile Change notification.
enum enumProfileChangeItem
{
    PROFILE_CHANGE_ITEM_UNKNOWN,		///< Indicates that the EAID has changed, reload the profile.
    PROFILE_CHANGE_ITEM_EAID,		    ///< Indicates that the EAID has changed, reload the profile.
    PROFILE_CHANGE_ITEM_AVATAR,	    	///< Indicates that the AVATAR has changed, reload the profile and avatar.
    PROFILE_CHANGE_ITEM_SUBSCRIPTION,	///< Indicates that the SUBSCRIPTION has changed, reload the profile.
};

/// \ingroup enums
/// \brief Defines the broadcast state changes.

enum enumBroadcastState
{
    ORIGIN_BROADCAST_DIALOG_OPEN,		        ///< This indicates the main broadcast dialog has been opened. This is the dialog shown when the user has a Twitch account associated with an Origin account. This event will get fire anytime the dialog is opened regardless of whether the user opened it via the OIG or the SDK triggers the dialog.
    ORIGIN_BROADCAST_DIALOG_CLOSED,		        ///< This indicates the main broadcast dialog has been closed.
    ORIGIN_BROADCAST_ACCOUNTLINKDIALOG_OPEN,    ///< This indicates the Twitch login UI, which allows the user to associate a Twitch account with an Origin account, is open.
    ORIGIN_BROADCAST_ACCOUNT_DISCONNECTED,      ///< This indicates that no Twitch account is associated with the Origin account.
    ORIGIN_BROADCAST_STARTED,                   ///< This indicates that the game has passed the ORIGIN_BROADCAST_START_PENDING state and is actively broadcasting.
    ORIGIN_BROADCAST_STOPPED,                   ///< This indicates the game broadcast stopped.
    ORIGIN_BROADCAST_BLOCKED,                   ///< This indicates the game broadcast cannot start because either the game is unreleased or is on the Twitch blacklist.
    ORIGIN_BROADCAST_START_PENDING,             ///< This indicates Twitch is in the process of starting to broadcast, but is not actively broadcasting yet.
    ORIGIN_BROADCAST_ERROR                      ///< This indicates there was an error while attempting to start a broadcast.
};

/// \ingroup enums
/// \brief Progressive installation chunk states
enum enumChunkState
{
    ORIGIN_PROGRESSIVE_INSTALLATION_UNKNOWN,    ///< The game should never see this value.
    ORIGIN_PROGRESSIVE_INSTALLATION_PAUSED,     ///< Indication that the download/installation has been stopped, either by the user, or because of network issues/Origin being offline.
    ORIGIN_PROGRESSIVE_INSTALLATION_QUEUED,     ///< The chunk is in the queue to be downloaded.
    ORIGIN_PROGRESSIVE_INSTALLATION_ERROR,		///< The chunk is having problems downloading.
    ORIGIN_PROGRESSIVE_INSTALLATION_DOWNLOADING,///< The chunk is currently being downloaded.
    ORIGIN_PROGRESSIVE_INSTALLATION_INSTALLING, ///< The chunk is currently being installed. This possibly only applies to chunk 0
    ORIGIN_PROGRESSIVE_INSTALLATION_INSTALLED,  ///< The chunk is currently installed, and available for use.
    ORIGIN_PROGRESSIVE_INSTALLATION_BUSY,       ///< The chunk is doing some processing.
};

/// \ingroup enums
/// \brief
enum enumChunkType
{
    ORIGIN_CHUNK_TYPE_UNKNOWN,			///< The game should never see this value.
    ORIGIN_CHUNK_TYPE_REQUIRED,			///< This chunk should be downloaded to make the game fully functional.
    ORIGIN_CHUNK_TYPE_RECOMMENDED,		///< This chunk is not required, but will improve the game quality
    ORIGIN_CHUNK_TYPE_NORMAL,			///< This chunk provides additional content for the game that will improve.
    ORIGIN_CHUNK_TYPE_ONDEMAND,			///< This chunk will not be downloading unless the game explicitly requests it.
};

/// \ingroup enums
/// \brief Restart options
enum enumRestartGameOptions
{
    ORIGIN_RESTARTGAME_NORMAL,								///< When the current game terminates relaunch the game and have the default update flow take place.
    ORIGIN_RESTARTGAME_FORCE_UPDATE_FOR_GAME,				///< When the current game terminates, initiate an update, and relaunch the game.
};

/// \ingroup enums
/// \brief QueryContent options
enum enumQueryContentOptions
{
    ORIGIN_QUERY_CONTENT_ALL        = 0xFFFFFFFF,	    ///< All types of content will be queried.
    ORIGIN_QUERY_CONTENT_BASE_GAME  = (1 << 0),         ///< Include Base Games in the query results.
    ORIGIN_QUERY_CONTENT_DLC        = (1 << 1),	        ///< Include DLC in the query results
    ORIGIN_QUERY_CONTENT_ULC        = (1 << 2),	        ///< Include ULC in the query results
    ORIGIN_QUERY_CONTENT_OTHER      = 0xFFFFFFF8,	    ///< Include Other in the query results - In future versions this may contain less items if specific flags are introduced.
};

/// \ingroup enums
/// \brief The type of the group you want to create.
enum enumGroupType
{
    ORIGIN_GROUP_TYPE_DEFAULT = -1, ///< Use this if you don't know or don't care about the group type.
    ORIGIN_GROUP_TYPE_PUBLIC,       ///< The group can be joined without being invited to the group
    ORIGIN_GROUP_TYPE_PRIVATE,      ///< The group can only be joined through an invite to the group.
};

/// \ingroup enums
/// \brief The chat state of the user.
enum enumChatState
{
    ORIGIN_CHAT_STATE_DEFAULT = -1,     ///< Use this if you don't know or don't care about the mute state.
    ORIGIN_CHAT_STATE_STARTED_WRITING,  ///< User is writing a message.
    ORIGIN_CHAT_STATE_STOPPED_WRITING,  ///< User stopped writing a message.
    ORIGIN_CHAT_STATE_STARTED_TALKING,  ///< User is talking over VoIP.
    ORIGIN_CHAT_STATE_STOPPED_TALKING,  ///< User stopped talking over VoIP.
};

/// \ingroup enums
/// \brief The voip status information
enum enumVoipState
{
	ORIGIN_VOIP_STATE_UNKNOWN = -1,				///< Default value 
	ORIGIN_VOIP_STATE_CHANNEL_DISCONNECTED,		///< The voip channel is disconnected. This resets all mute states.
	ORIGIN_VOIP_STATE_CHANNEL_CONNECTING,		///< The voip channel is connecting.
	ORIGIN_VOIP_STATE_CHANNEL_CONNECTED,		///< The voip channel is connected. The user should be able to talk over voip now.
	ORIGIN_VOIP_STATE_USER_TALKING_START,		///< The user started talking. 
	ORIGIN_VOIP_STATE_USER_TALKING_END,			///< The user stopped talking.
	ORIGIN_VOIP_STATE_USER_MUTED_LOCALLY,		///< The local user muted the remote user.
	ORIGIN_VOIP_STATE_USER_MUTED_REMOTELY,		///< The remote user muted himself.
	ORIGIN_VOIP_STATE_USER_UNMUTED_LOCALLY,		///< The local user unmuted the remote user.
	ORIGIN_VOIP_STATE_USER_UNMUTED_REMOTELY,	///< The remote user unmuted himself.
	ORIGIN_VOIP_STATE_USER_JOINED,				///< The user joined the channel.
	ORIGIN_VOIP_STATE_USER_LEFT,				///< The user left the channel.
	ORIGIN_VOIP_STATE_UNAVAILABLE,				///< Voip is not available.
};

/// \ingroup enums
/// \brief The mute state of the user.
enum enumMuteState
{
	ORIGIN_VOIP_MUTE_STATE_NONE,				///< The remote user hasn't joined voip.
	ORIGIN_VOIP_MUTE_STATE_UNMUTED,				///< The remote user is not muted.
	ORIGIN_VOIP_MUTE_STATE_LOCAL_MUTED,			///< The remote user is muted by the user locally.
	ORIGIN_VOIP_MUTE_STATE_REMOTE_MUTED,		///< The remote user muted himself.
	ORIGIN_VOIP_MUTE_STATE_LOCAL_REMOTE_MUTED,	///< The remote user is muted by the local user and he muted himself
};

enum enumLoginReasonCode
{
    // Undefined.
    ORIGIN_LOGINREASONCODE_UNDEFINED,
    // The user logged itself in or out.
    ORIGIN_LOGINREASONCODE_USER_INITIATED,
    // The user is already logged in.
    ORIGIN_LOGINREASONCODE_ALREADY_ONLINE,
    // Network error. Non 200 HTTP response code.
    ORIGIN_LOGINREASONCODE_NETWORK_ERROR,
    // User credentials are invalid.
    ORIGIN_LOGINREASONCODE_INVALID_CREDENTIALS,
    // Could not refresh access token.
    ORIGIN_LOGINREASONCODE_ACCESSTOKEN_REFRESH_ERROR
};


enum enumSteamStoreFlag
{
    ORIGIN_STEAMOVERLAYTOSTOREFLAG_NONE,
    ORIGIN_STEAMOVERLAYTOSTOREFLAG_ADDTOCART,
    ORIGIN_STEAMOVERLAYTOSTOREFLAG_ADDTOCARTANDSHOW,
};

#endif //__ORIGIN_ENUMS_H__
