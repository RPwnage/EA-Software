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
	ORIGIN_EVENTINDEX_IGO,       	        ///< The bit position of the LSX event identifier for IGO events.
	ORIGIN_EVENTINDEX_Invite,    	        ///< The bit position of the LSX event identifier for invite events.
	ORIGIN_EVENTINDEX_Login,     	        ///< The bit position of the LSX event identifier for login events.
	ORIGIN_EVENTINDEX_Profile,   	        ///< The bit position of the LSX event identifier for profile events.
	ORIGIN_EVENTINDEX_Presence,  	        ///< The bit position of the LSX event identifier for presence events.
	ORIGIN_EVENTINDEX_Friends,   	        ///< The bit position of the LSX event identifier for friends events.
	ORIGIN_EVENTINDEX_Purchase,  	        ///< The bit position of the LSX event identifier for purchase events.
	ORIGIN_EVENTINDEX_Content,		        ///< The bit position of the LSX event identifier for download events.
	ORIGIN_EVENTINDEX_Blocked,		        ///< The bit position of the LSX event identifier for block list events.
	ORIGIN_EVENTINDEX_Online,		        ///< The bit position of the LSX event identifier for online status events.
    ORIGIN_EVENTINDEX_Achievement,          ///< The bit position of the LSX event identifier for achievement events.
    ORIGIN_EVENTINDEX_InvitePending,        ///< The bit position of the LSX event identifier for pending invite events.
	ORIGIN_EVENTINDEX_ChatMessage,	        ///< The bit position of the LSX event identifier for chat message events.
    ORIGIN_EVENTINDEX_ChatInvite,	        ///< The bit position of the LSX event identifier for chat invite events.
    ORIGIN_EVENTINDEX_ChatCreated,	        ///< The bit position of the LSX event identifier for chat created events.
    ORIGIN_EVENTINDEX_ChatFinished,         ///< The bit position of the LSX event identifier for chat finished events.
    ORIGIN_EVENTINDEX_ChatUpdated,          ///< The bit position of the LSX event identifier for chat updated events.
	ORIGIN_EVENTINDEX_Visibility,	        ///< The bit position of the LSX event identifier for visibility events.
	ORIGIN_EVENTINDEX_Broadcast,	        ///< The bit position of the LSX event identifier for broadcast events.
    ORIGIN_EVENTINDEX_CurrentUserPresence,  ///< The bit position of the LSX event identifier for current userpresence events.
    ORIGIN_EVENTINDEX_NewItems,             ///< The bit position of the LSX event identifier for new acquired items.
	ORIGIN_EVENTINDEX_ChunkStatus,          ///< The bit position of the LSX event identifier for chunk status events.
	ORIGIN_EVENTINDEX_IgoUnavailable,		///< The bit position of the LSX event identifier for the igo couldn't be shown.

	ORIGIN_NUM_EVENTS            	///< The total number of LSX event identifiers.
};

/// \ingroup enums
/// \brief Defines users' online presence status.
enum enumPresence
{
	ORIGIN_PRESENCE_OFFLINE,		///< The user is offline.
	ORIGIN_PRESENCE_ONLINE,			///< The user is online but not playing a game.
	ORIGIN_PRESENCE_INGAME,			///< The user is online and is running a game.
	ORIGIN_PRESENCE_BUSY,			///< The user is online but busy.	
	ORIGIN_PRESENCE_IDLE,			///< The user is online but idle (likely away).
	ORIGIN_PRESENCE_JOINABLE,		///< The user is online and in a joinable game state.
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
	ORIGIN_SETTINGS_LANGUAGE,		///< Language setting
	ORIGIN_SETTINGS_ENVIRONMENT,	///< Environment setting
	ORIGIN_SETTINGS_IS_IGO_AVAILABLE,	///< Return "true" if IGO is available, "false" if IGO is not available
    ORIGIN_SETTINGS_IS_IGO_ENABLED,     ///< Indicate whether the IGO is enabled by the user. If this returns "true" it doesn't mean that the IGO is available for use. Check \ref ORIGIN_SETTINGS_IS_IGO_AVAILABLE to see whether the IGO is actually available.
};

/// \ingroup enums
/// \brief Defines the settings items.
enum enumGameInfo
{
	ORIGIN_GAME_INFO_UPTODATE,		///< Game up to date flag
	ORIGIN_GAME_INFO_LANGUAGES,     ///< Set of languages that the user is entitled to play the game in
	ORIGIN_GAME_INFO_FREE_TRIAL,    ///< Check whether the game is a free trial.
    ORIGIN_GAME_INFO_EXPIRATION,    ///< Check whether the game has an expiration set.
    ORIGIN_GAME_INFO_EXPIRATION_DURATION,    ///< Returns the number of seconds remaining until expiration.
};


/// \ingroup enums
/// \brief Defines the severity of debug messages.
enum enumDebugLevel
{
	ORIGIN_LEVEL_NONE,				///< No debug information.
	ORIGIN_LEVEL_SEVERE,			///< Severe issues only. This maps to \ref ORIGIN_LEVEL_0.
	ORIGIN_LEVEL_MAJOR,				///< Severe and Major issues. This maps to \ref ORIGIN_LEVEL_1.
	ORIGIN_LEVEL_MINOR,				///< Severe, Major and Minor issues. This maps to \ref ORIGIN_LEVEL_2.
	ORIGIN_LEVEL_TRIVIAL,			///< Severe issues only. This maps to \ref ORIGIN_LEVEL_3.
	ORIGIN_LEVEL_EVERYTHING			///< Trivial and above. This maps to \ref ORIGIN_LEVEL_4.
};

enum enumContentState
{
	CONTENT_STATE_DOWNLOADING,		///< The content is being downloaded.
	CONTENT_STATE_INSTALLING,		///< The content is being installed.
	CONTENT_STATE_READY,			///< The content is ready for use
	CONTENT_STATE_BUSY,				///< The content is busy, either with some internal process, or is in use.
	CONTENT_STATE_UNKNOWN,			///< The content is in a bad state.
	CONTENT_STATE_WAITING			///< The content is waiting for user action.
};

enum enumAchievementMode
{
    ACHIEVEMENT_MODE_ACTIVE,        ///< Query the active achievements. These are the achievements that have progress, or are achieved.
    ACHIEVEMENT_MODE_ALL,           ///< Query all the achievements. This will return all achievements for a game Id including the ones without progress.
};

enum enumProfileChangeItem
{
	PROFILE_CHANGE_ITEM_EAID = 0x0,		///< Indicates that the EAID has changed.
	PROFILE_CHANGE_ITEM_AVATAR = 0x1,	///< Indicates that the AVATAR has changed.
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

#endif //__ORIGIN_ENUMS_H__
