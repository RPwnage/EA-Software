#ifndef __ORIGIN_PRESENCE_H__
#define __ORIGIN_PRESENCE_H__

#include "Origin.h"
#include "OriginTypes.h"
#include "OriginEnums.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup pres Presence
/// \brief This module contains the functions for providing Origin's Presence awareness feature.
/// 
/// There are sychronous and asynchronous versions of the \ref OriginQueryPresence function. For an explanation of
/// why, see \ref syncvsasync in the <em>Integrator's Guide</em>.
/// 
/// For more information on the integration of this feature, see \ref la-intguide-presence in the <em>Integrator's Guide</em>.

/// \ingroup pres
/// \brief Subscribes to other users' presence information.
/// 
/// Subscribes to presence updates for a list of users. The subscription list can include both friends and 
/// current/recent players (previously identified via \ref OriginAddRecentPlayers). 
/// By only subscribing to presence that is of interest, impact on the server infrastructure is reduced. 
/// Note that the user is always subscribed to their friends presence and you do not need to use this 
/// function for that (though it is not an error if you do).
/// \param user The user that is requesting the subscription of presence. Use \ref OriginGetDefaultUser.
/// \param otherUsers The list of users whose presence information you want to subscribe to (e.g., friends/recent players etc.).
/// \param userCount The number of users in the otherUsers list.
/// \sa XPresenceSubscribe(DWORD dwUserIndex, DWORD cPeers, const XUID *pPeers)
OriginErrorT ORIGIN_SDK_API OriginSubscribePresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount);

/// \ingroup pres
/// \brief Unsubscribes from other users' presence information.
/// 
/// Unsubscribes from presence updates. The unsubscribe list can include users previously subscribed 
/// to via the \ref OriginSubscribePresence call. Peers included in the list who were not previously 
/// subscribed to are ignored (it is not considered an error).
/// \param user The user that is requesting the unsubscription of presence. Use \ref OriginGetDefaultUser.
/// \param otherUsers The list of users whose presence information you want to unsubscribe from (e.g., friends/recent players etc.). 
/// \param userCount The number of users in the otherUsers list.
/// \sa XPresenceUnsubscribe(DWORD dwUserIndex, DWORD cPeers, const XUID *pPeers)
OriginErrorT ORIGIN_SDK_API OriginUnsubscribePresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount);

/// \ingroup pres
/// \brief Gets the presence status for a list of users.
/// 
/// Queries the current presence status for a list of friends and/or recent players.
///	\param user The user who is requesting the presence update. Use \ref OriginGetDefaultUser. 
/// \param otherUsers The list of users whose presence you want to update (e.g., friends/recent players etc.).
/// \param userCount The number of users in the otherUsers list.
/// \param callback Sets a user callback to be called when data comes in.
/// \param pContext Adds an optional user context to the callback.
/// \param timeout A timeout in milliseconds. 
/// \param pHandle A handle for OriginEnumerate functions (see \ref enumerate) to return OriginFriendT structures.
/// \sa XPresenceCreateEnumerator(DWORD dwUserIndex, DWORD cPeers, const XUID *pPeers, DWORD dwStartingIndex, DWORD dwPeersToReturn, DWORD *pcbBuffer, HANDLE *ph)
OriginErrorT ORIGIN_SDK_API OriginQueryPresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup pres
/// \brief Waits for the presence information for a list of users.
/// 
/// Queries the current presence status for a list of friends and/or recent players.
///	\param user The user who is requesting that the presence be updated. Use \ref OriginGetDefaultUser. 
/// \param otherUsers The list of users from who you want the presence update (e.g., friends/recent players etc.).
/// \param userCount The number of users in the otherUsers list.
/// \param pTotalCount The total number of records in the transaction.
/// \param timeout A timeout in milliseconds. 
/// \param pHandle A handle for OriginEnumerate functions (see \ref enumerate) to return OriginFriendT structures.
/// \sa XPresenceCreateEnumerator(DWORD dwUserIndex, DWORD cPeers, const XUID *pPeers, DWORD dwStartingIndex, DWORD dwPeersToReturn, DWORD *pcbBuffer, HANDLE *ph)
OriginErrorT ORIGIN_SDK_API OriginQueryPresenceSync(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup pres
/// \brief Sets the presence for the current user.
/// 
/// Sets the extended presence for the game. Origin will set basic presence (which game the user is playing, if they
/// appear idle, etc.), but this function allows the game to provide more detail. If the user is in a joinable game
/// session, they must pass the session identifier. If not in game or not in a joinable game, the session identifier
/// should be NULL. Note that \ref ORIGIN_PRESENCE_OFFLINE is not a valid state and will return an error. (Any option for a
/// user to 'hide themselves' online would be available exclusively via the Origin desktop application, not through the
/// SDK.)
/// \param user The user who is requesting a change to his or her presence. Use \ref OriginGetDefaultUser. 
/// \param presence One of ORIGIN_PRESENCE_XXX (see \ref enumPresence).
/// \param pPresenceString Additional public presence information like: 'Playing FIFA11' (NULL=no custom presence).
/// \param pGamePresenceString A game-specific presence string that only has meaning for the same title (NULL=no game data).
/// \param pSession	A session string (0=not in game session).
/// \sa XUserSetContext(DWORD dwUserIndex, DWORD dwContextId, DWORD dwContextValue)
OriginErrorT ORIGIN_SDK_API OriginSetPresence(OriginUserT user, enumPresence presence, const OriginCharT *pPresenceString, const OriginCharT* pGamePresenceString, const OriginCharT *pSession);

/// \ingroup pres
/// \brief Controls the visibility of the user.
///
/// This controls the visibility of the users presence for other players.
/// visible == true --> The users presence is visible for other users.
/// visible == false --> The user will appear offline for other users.
/// \note Setting the users presence to invisible without the user explicitly doing so in the game UI will constitute a OCR
/// \param user The user for whom you are requesting the presence visibility change for. Use \ref OriginGetDefaultUser. 
/// \param visible The boolean indicating whether the users presence should be visible to other players.
OriginErrorT ORIGIN_SDK_API OriginSetPresenceVisibility(OriginUserT user, bool visible);

/// \ingroup pres
/// \brief Request the visibility of the user.
///
/// This function queries the visibility of the users presence to other players.
/// visible == true --> The users presence is visible for other users.
/// visible == false --> The user will appear offline for other users.
/// \param user The user for whom you are requesting the presence visibility for. Use \ref OriginGetDefaultUser. 
/// \param visible The boolean indicating whether the users presence should be visible to other players.
OriginErrorT ORIGIN_SDK_API OriginGetPresenceVisibility(OriginUserT user, bool * visible);

/// \ingroup pres
/// \brief Retrieves the presence for the current user.
/// 
/// Queries the current extended presence state for the game. This returns the state previously set with
/// \ref OriginSetPresence for the selected user.
///	\param user The user whose presence you want to get. Use \ref OriginGetDefaultUser. 
///	\param pPresence Receives the current ORIGIN_PRESENCE_XXX context (see \ref enumPresence).
/// \param pPresenceString An optional presence string (NULL=do not return).
/// \param presenceStringLen The length of the buffer allocated for the pPresenceString (will truncate if the string is longer).
/// \param pGamePresenceString An optional game presence string (NULL=don't return).
/// \param gamePresenceStringLen The length of the buffer allocated for the pGamePresenceString (will truncate if string is longer; potential for disaster).
/// \param pSessionString The current game session (NULL=don't return).
/// \param sessionStringLen The length of the buffer allocated for the pSessionString (will truncate if string is longer; potential for disaster).
/// \sa XUserGetContext(DWORD dwUserIndex, XUSER_CONTEXT *pContext, PXOVERLAPPED *pxOverlapped)
OriginErrorT ORIGIN_SDK_API OriginGetPresence(OriginUserT user, enumPresence * pPresence, OriginCharT *pPresenceString, OriginSizeT presenceStringLen, OriginCharT *pGamePresenceString, OriginSizeT gamePresenceStringLen, OriginCharT *pSessionString, OriginSizeT sessionStringLen);

#ifdef __cplusplus
}
#endif

#endif //__ORIGIN_PRESENCE_H__