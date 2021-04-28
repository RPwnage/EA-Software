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
};

/// \ingroup enums
/// \brief Defines types of SDK events.
///
/// This enumeration relates directly to the bit position of the LSX event identifier.
enum OriginEventIndexEnum {
    ORIGIN_EVENT_FALLBACK = -1,             ///< The fallback id for defining a fallback callback for events that are not registered .
    ORIGIN_EVENT_IGO,       	            ///< The in-game overlay action (param: \ref uint32_t).
    ORIGIN_EVENT_INVITE,    	            ///< The user accepted a game invite (param: OriginInviteT).
    ORIGIN_EVENT_LOGIN,     	            ///< A network event has occurred (param: ORIGIN_LOGINSTATUS_*; see \ref ORIGIN_LOGINSTATUS_OFFLINE, etc.).
    ORIGIN_EVENT_PROFILE,   	            ///< The local user profile changed (param: \ref OriginProfileChangeT).
    ORIGIN_EVENT_PRESENCE,  	            ///< A subscribed presence has changed (param:  user id, \ref OriginUserT).
    ORIGIN_EVENT_FRIENDS,   	            ///< The friends list has changed (param: OriginFriendT).
    ORIGIN_EVENT_PURCHASE,  	            ///< The user just purchased an item (param:  manifest id, string).
    ORIGIN_EVENT_CONTENT,		            ///< The user just downloaded an item (param: OriginDownloadT).
    ORIGIN_EVENT_BLOCKED,		            ///< The user just added or removed somebody from the block list, please refresh.
    ORIGIN_EVENT_ONLINE,		            ///< The user received a online status event (param bool).
    ORIGIN_EVENT_ACHIEVEMENT,			    ///< The user receives information about the achieved achievement. (\ref OriginAchievementSetT)
    ORIGIN_EVENT_INVITE_PENDING,            ///< The user receive information about the currently pending invite. (\ref OriginInvitePendingT)
    ORIGIN_EVENT_CHAT_MESSAGE,	            ///< Event indicating that the user received a chat message (param OriginChatT).
    ORIGIN_EVENT_CHAT_INVITE,	            ///< Event indicating that the user is invited to a chat session (param OriginCharT *)
    ORIGIN_EVENT_CHAT_CREATED,	            ///< Event indicating that a chat session is created (param OriginCharT *)
    ORIGIN_EVENT_CHAT_FINISHED,             ///< Event indicating that the chat session has finished. (param OriginCharT *)
    ORIGIN_EVENT_CHAT_UPDATED,              ///< Event indicating that the chat session has been updated. (param OriginCharT *)
    ORIGIN_EVENT_VISIBILITY,	            ///< The user received the new visibility state.
    ORIGIN_EVENT_BROADCAST,				    ///< The user received the new broadcast state.
    ORIGIN_EVENT_CURRENT_USER_PRESENCE,     ///< Current user presence has changed.
    ORIGIN_EVENT_NEW_ITEMS,                 ///< The user acquired new items. The items may be unrelated to the game receiving them.
    ORIGIN_EVENT_CHUNK_STATUS,              ///< The chunk status for the progressive installation has been updated.
    ORIGIN_EVENT_IGO_UNAVAILABLE,		    ///< This event will indicate that the IGO is not available.
    ORIGIN_EVENT_MINIMIZE_REQUEST,          ///< This event indicates the game should minimize itself (Origin user action required)
    ORIGIN_EVENT_RESTORE_REQUEST,           ///< This event indicates the game should restore itself (if previously minimized)
    ORIGIN_EVENT_GAME_MESSAGE,              ///< Receives a game message sent by another game connection.  receives and \ref OriginGameMessageT structure.

    ORIGIN_NUM_EVENTS                       ///< The total number of LSX event identifiers.
};

/// \ingroup enums
/// \brief Defines users' online presence status.
enum enumPresence
{
    ORIGIN_PRESENCE_UNKNOWN = -1,			    ///< It is unknown which presence the user is in
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
    ORIGIN_SETTINGS_IS_TELEMETRY_ENABLED  ///< Indicate whether telemetry is enabled by the user (i.e. the user did not opt-out) 
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

enum enumAchievementMode
{
    ACHIEVEMENT_MODE_ACTIVE,        ///< Query the active achievements. These are the achievements that have progress, or are achieved.
    ACHIEVEMENT_MODE_ALL,           ///< Query all the achievements. This will return all achievements for a game Id including the ones without progress.
};

enum enumProfileChangeItem
{
    PROFILE_CHANGE_ITEM_EAID = 0x0,		    ///< Indicates that the EAID has changed, reload the profile.
    PROFILE_CHANGE_ITEM_AVATAR = 0x1,	    ///< Indicates that the AVATAR has changed, reload the profile and avatar.
    PROFILE_CHANGE_ITEM_SUBSCRIPTION = 0x2,	///< Indicates that the SUBSCRIPTION has changed, reload the profile.
};

enum enumChatStates
{
    CHAT_ONE_TO_ONE,
    CHAT_PENDING_MUC,
    CHAT_MUC,
    CHAT_FINISHED
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


#endif //__ORIGIN_ENUMS_H__
