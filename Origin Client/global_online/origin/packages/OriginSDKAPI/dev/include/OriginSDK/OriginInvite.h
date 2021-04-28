#ifndef __ORIGIN_INVITE_H__
#define __ORIGIN_INVITE_H__

#include "Origin.h"
#include "OriginTypes.h"

#ifdef __cplusplus
extern "C" 
{
#endif

	
/// \defgroup invite Invite
/// \brief This module contains the functions that allow the game to Invite other players to join in the default user's
/// game in progress.
/// 
/// There are sychronous and asynchronous versions of the \ref OriginSendInvite function. For an explanation of
/// why, see \ref syncvsasync in the <em>Integrator's Guide</em>.
/// 
/// For more information on the integration of this feature, see \ref ia-intguide-invites in the <em>Integrator's Guide</em>.

/// \ingroup invite
/// \brief Invite a list of friends to your game.
/// 
/// This function allows the game to invite other players without needing interaction with the IGO.
/// \param user The local user. Use \ref OriginGetDefaultUser.
/// \param pUserList A list of users.
/// \param userCount The number of users in the pUserList.
/// \param pMessage A message to accompany the invitation.
/// \param timeout The amount of time in milliseconds that you want to wait for this function to succeed.
/// \param callback A callback to a handler function that will receive a notification when the invites are sent, or an error occurred.
/// \param pContext A user-provided context for the callback.
OriginErrorT ORIGIN_SDK_API OriginSendInvite(OriginUserT user, const OriginUserT *pUserList, OriginSizeT userCount, const OriginCharT * pMessage, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext); 

/// \ingroup invite
/// \brief Invite a list of friends to your game.
/// 
/// This function allows the game to invite other players without needing interaction with the IGO.
/// \param user The local user. Use \ref OriginGetDefaultUser.
/// \param pUserList A list of users.
/// \param userCount The number of users in the pUserList.
/// \param pMessage A message to accompany the invitation.
/// \param timeout The amount of time in milliseconds that you want to wait for this function to succeed.
OriginErrorT ORIGIN_SDK_API OriginSendInviteSync(OriginUserT user, const OriginUserT *pUserList, OriginSizeT userCount, const OriginCharT * pMessage, uint32_t timeout); 


/// \ingroup invite
/// \brief Accept a game invite for an invite received through an ORIGIN_EVENT_INVITE_PENDING event.
///
/// This function allows the game to accept game invites from other users. If an invite is available for the specified user the game will receive an invite event, and ORIGIN_SUCCESS will be returned.
/// If no invite is pending ORIGIN_NOT_FOUND will be returned.
/// \param user The local user. Use \ref OriginGetDefaultUser.
/// \param other The user that invited you to a game.
/// \param timeout The amount of time in milliseconds that you want to wait for this function to succeed.
/// \param callback A callback to a handler function that will receive a notification when the invites are sent, or an error occurred.
/// \param pContext A user-provided context for the callback.
OriginErrorT ORIGIN_SDK_API OriginAcceptInvite(OriginUserT user, OriginUserT other, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);

/// \ingroup invite
/// \brief Accept a game invite for an invite received through an ORIGIN_EVENT_INVITE_PENDING event.
///
/// This function allows the game to accept game invites from other users. If an invite is available for the specified user the game will receive an invite event, and ORIGIN_SUCCESS will be returned.
/// If no invite is pending ORIGIN_NOT_FOUND will be returned.
/// \param user The local user. Use \ref OriginGetDefaultUser.
/// \param other The user that invited you to a game.
/// \param timeout The amount of time in milliseconds that you want to wait for this function to succeed.
OriginErrorT ORIGIN_SDK_API OriginAcceptInviteSync(OriginUserT user, OriginUserT other, uint32_t timeout);


#ifdef __cplusplus
}
#endif

#endif //__ORIGIN_INVITE_H__