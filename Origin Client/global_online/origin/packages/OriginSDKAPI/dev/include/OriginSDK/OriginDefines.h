/// \defgroup defs Defines
/// \brief This module contains a wide variety of defines used by a variety of Origin functions. 

#ifndef __ORIGIN_DEFINES_H__
#define __ORIGIN_DEFINES_H__

/// \ingroup defs
/// \brief The other user is not a friend of this user.
/// 
#define ORIGIN_FRIEND_NONE			0

/// \ingroup defs
/// \brief The other user is a friend of this user.
/// 
#define ORIGIN_FRIEND_MUTUAL		1

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
/// This will prevent the SDK from starting Origin.
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
#define ORIGIN_FLAG_FORCE_CONTENT_ID            (1<<11)

/// \ingroup defs
/// \brief Specify this flag when you don't want the Origin SDK to start a trial thread when the content is a free trial.
/// 
/// Prevent the Origin SDK to start a trial thread.
#define ORIGIN_FLAG_DISABLE_TRIAL_THREAD	    (1<<12)

/// \ingroup defs
/// \brief Enable the IGO API.
/// 
/// When this flag is set the OriginSDK will use the new IGO API instead of the old IGO via API hooking,
/// so you have to run the IGO via direct API calls from OriginInGame.h
#define ORIGIN_FLAG_ENABLE_IGO_API (1<<12)


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
/// \brief The content is a browser app (angular single page app.).  Specifying this flag will change the behavior of the container to be more friendly for SPAs.
/// 
#define ORIGIN_IGO_BROWSER_APP				    (1<<3)

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


/// \ingroup defs
/// \brief Utilization Presets.
/// Putting the download utilization in turbo mode might significantly influence game performance.
#define ORIGIN_DOWNLOAD_UTILIZATION_TURBO           2.0f

/// \ingroup defs
/// \brief Utilization Presets.
/// Origin will download as fast as deemed reasonable during game play.
#define ORIGIN_DOWNLOAD_UTILIZATION_FULLSPEED       1.0f

/// \ingroup defs
/// \brief Utilization Presets.
/// Origin will download at a 75 % utilization. Depending of your network speeds there may be
#define ORIGIN_DOWNLOAD_UTILIZATION_3_QUARTER_SPEED 0.75f

/// \ingroup defs
/// \brief Utilization Presets.
/// Origin will download at a 50 % utilization.
#define ORIGIN_DOWNLOAD_UTILIZATION_HALVE_SPEED     0.5f

/// \ingroup defs
/// \brief Utilization Presets.
/// Origin will download at a 25 % utilization.
#define ORIGIN_DOWNLOAD_UTILIZATION_QUARTER_SPEED   0.25f

/// \ingroup defs
/// \brief Utilization Presets.
/// Origin will download as fast as deemed reasonable during game play.
#define ORIGIN_DOWNLOAD_UTILIZATION_TRICKLE         0.1f

/// \ingroup defs
/// \brief Utilization Presets.
/// Origin will stop downloading when the utilization is set to zero. 
/// Only use this when constant frame rate is required, and restore to one as soon as the requirement is no longer necessary.
#define ORIGIN_DOWNLOAD_UTILIZATION_STOPPED         0.0f



#endif //__ORIGIN_DEFINES_H__
