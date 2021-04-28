#ifndef __ORIGIN_IGO_H__
#define __ORIGIN_IGO_H__

#include "OriginTypes.h"
#include "OriginEnums.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup igo In-Game Overlay
/// \brief This module contains the functions that provide the Origin In-Game Overlay. 
/// 
/// For more information on the integration of this feature, see \ref fa-intguide-ingameoverlay, as well as each of the feature-specific pages, in the <em>Integrator's Guide</em>. 

/// \ingroup igo
/// \brief Toggles the display of the IGO.
/// 
/// This function gives the game control over the visibility of the IGO. Similar to the user-controlled hotkey
/// 'Ctrl+Tab or Shift+`'.
/// \param[in] bShow Indicates whether to show or hide the IGO.
/// \note Only hide the IGO when having the IGO up will interfere with current game actions.
OriginErrorT ORIGIN_SDK_API OriginShowIGO(bool bShow);

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/// \ingroup igo
/// \brief Shows the login dialog.
/// 
/// When Origin is running, but the user is not logged in, the game can popup a login dialog.
/// \param user The user the login dialog should be shown for. Use OriginGetDefaultUser().
/// \return \ref ORIGIN_SUCCESS if the dialog is shown.<br>\ref ORIGIN_IGOSTATE_UP if communication with the IGO fails.
OriginErrorT ORIGIN_SDK_API OriginShowLoginUI(OriginUserT user);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


/// \ingroup igo
/// \brief Shows the profile UI.
/// 
/// Show the user's profile in the IGO.
/// \param user The user for whom the profile should be shown. 
/// \return \ref ORIGIN_SUCCESS if the dialog is shown.<br>\ref ORIGIN_IGOSTATE_UP if communication with the IGO fails.
/// \note The error list is not complete.
OriginErrorT ORIGIN_SDK_API OriginShowProfileUI(OriginUserT user);


/// \ingroup igo
/// \brief Shows the profile UI.
/// 
/// Displays the UI showing the profile of the target user. Also provides UI options to send friend requests and enter
/// feedback.
/// \param user The local user. Use OriginGetDefaultUser().
/// \param target The target user whose profile we want to show.
/// \sa XShowGamerCardUI(DWORD dwUserIndex, XUID XuidPlayer);
OriginErrorT ORIGIN_SDK_API OriginShowFriendsProfileUI(OriginUserT user, OriginUserT target);

/// \ingroup igo
/// \brief Shows the recent player UI.
/// 
/// Displays the UI showing a list of recent players with their UI options for sending friend requests, checks their
/// profiles and allows feedback to be sent. The list of recent players is created via the EdisuAddRecentPlayers
/// function.
/// \param user The local user. Use OriginGetDefaultUser().
/// \sa XShowPlayersUI(DWORD dwUserIndex);
OriginErrorT ORIGIN_SDK_API OriginShowRecentUI(OriginUserT user);

/// \ingroup igo
/// \brief Shows the feedback UI.
/// 
/// Displays the UI allowing the local user to enter feedback (positive or negative gameplay experience, abuse, etc.)
/// about a target user.
/// \param user The local user. Use OriginGetDefaultUser().
/// \param target The target user you want to give feedback about.
/// \sa XShowPlayerReviewUI(DWORD dwUserIndex, XUID XuidFeedbackTarget)
OriginErrorT ORIGIN_SDK_API OriginShowFeedbackUI(OriginUserT user, OriginUserT target);

/// \ingroup igo
/// \brief Shows the IGO browser UI.
/// 
/// Displays the web browser UI opened to the specified URL. Includes flags controlling which aspects of the browser
/// are to be visible in the window (navigation bar, et. al.).<br/>
/// &nbsp;&nbsp;&nbsp;&nbsp;\ref ORIGIN_IGO_BROWSER_NAV&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Shows the navigation & search bar.<br/>
/// &nbsp;&nbsp;&nbsp;&nbsp;\ref ORIGIN_IGO_BROWSER_TAB&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Creates a new tab instead of a new window.<br/>
/// &nbsp;&nbsp;&nbsp;&nbsp;\ref ORIGIN_IGO_BROWSER_ACTIVE&nbsp;&nbsp;&nbsp;&nbsp;Forces a new window or tab to be active/frontmost.<br/>
/// \param[in] iBrowserFeatureFlags Bitwise-or of browser features available to the user.
/// \param[in] pURL Optional initial address to browse to.
OriginErrorT ORIGIN_SDK_API OriginShowBrowserUI(int32_t iBrowserFeatureFlags, const OriginCharT * pURL);

/// \ingroup igo
/// \brief Shows the friend UI.
/// 
/// Displays the Origin friends list via the in-game overlay. Shows the current friends list, what they are doing
/// (presence) plus options to message them or invite them into game (if the customer is in the joinable game).
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \sa XShowFriendsUI(DWORD dwUserIndex)
OriginErrorT ORIGIN_SDK_API OriginShowFriendsUI(OriginUserT user);

/// \ingroup igo
/// \brief Bring up the find friend UI.
/// 
/// Displays the Origin find friends UI in the in-game overlay. 
/// \param[in] user The local user. Use OriginGetDefaultUser().
OriginErrorT ORIGIN_SDK_API OriginShowFindFriendsUI(OriginUserT user);

/// \ingroup igo
/// \brief Bring up the change avatar UI
/// 
/// Displays the Origin change avatar UI in the in-game overlay. 
/// \param[in] user The local user. Use OriginGetDefaultUser().
OriginErrorT ORIGIN_SDK_API OriginShowChangeAvatarUI(OriginUserT user);

/// \ingroup igo
/// \brief Shows the friend request UI.
/// 
/// Displays the Origin friends request dialog via the in-game overlay. It lets the user confirm they want to send a
/// friend invite to the target user and enter an optional message.
///	\param[in] user The local user. Use OriginGetDefaultUser().
///	\param[in] other The user you want to become friends with.
/// \sa XShowFriendRequestUI(DWORD dwUserIndex, XUID xuidUser)
OriginErrorT ORIGIN_SDK_API OriginRequestFriendUI(OriginUserT user, OriginUserT other);

/// \ingroup igo
/// \brief Shows the compose chat UI.
/// 
/// Displays the Origin compose message interface via the in-game overlay with a pre-populated list of recipients and
/// pre-populated message text. Note that Origin Chat is subtly different from X360 Messages. While messages can be
/// delivered to users who are offline, there is no concept of a mailbox as with X360 Messages. The message is consumed
/// once a user reads it.
///
/// See also: XShowChatComposeUI(DWORD dwUserIndex, CONST XUID *pXuidRecipients, DWORD cRecipients, LPCWSTR wszText)
///
///	\param[in] user The local user. Use OriginGetDefaultUser().
///	\param[in] pUserList A list of recipients to send the message to.
///	\param[in] iUserCount The number of recipients in the user list.
///	\param[in] pMessage A null-terminated text message.
///
OriginErrorT ORIGIN_SDK_API OriginShowComposeChatUI(OriginUserT user, const OriginUserT *pUserList, OriginSizeT iUserCount, const OriginCharT *pMessage);

/// \ingroup igo
/// \brief Shows the invite UI.
/// 
/// Displays the Origin game invitation interface via the in-game overlay. This allows the current user to invite other
/// users into their game. The user interface allows the user to enter an optional message to be delivered with the
/// invitation.
///	\param[in] user The local user. Use OriginGetDefaultUser().
///	\param[in] pInviteList The list of invitees. (optional)
///	\param[in] iInviteCount The number of invitees. (optional. use 0)
/// \param[in] pMessage An optional default message for the invite.
/// \sa XShowGameInviteUI(DWORD dwUserIndex, CONST XUID *pXuidRecipients, DWORD cRecipients, LPCWSTR wszUnused)
OriginErrorT ORIGIN_SDK_API OriginShowInviteUI(OriginUserT user, const OriginUserT *pInviteList, OriginSizeT iInviteCount, const OriginCharT *pMessage);

/// \ingroup igo
/// \brief Shows Achievement UI.
/// 
/// Display the achievement UI in the IGO for the specified persona and game.
///	\param[in] user The local user. Use OriginGetDefaultUser().
///	\param[in] persona The local persona. Use OriginGetDefaultPersona(), or specify a game specific persona.
/// \param[in] pGameId optional parameter for showing the achievements of games other than the current. Use NULL in most cases.
OriginErrorT ORIGIN_SDK_API OriginShowAchievementUI(OriginUserT user, OriginPersonaT persona, const char *pGameId);

/// \ingroup igo
/// \brief Bring up the game's page in the Origin store
/// 
/// Displays the game's page in the Origin store in the in-game overlay. 
OriginErrorT ORIGIN_SDK_API OriginShowGameDetailsUI(OriginUserT user);

/// \ingroup igo
/// \brief Bring up the broadcast UI
/// 
/// Displays the broadcast ui in the in-game overlay. 
OriginErrorT ORIGIN_SDK_API OriginShowBroadcastUI(OriginUserT user);

};

#ifdef __cplusplus
//}
#endif

#endif //__ORIGIN_IGO_H__