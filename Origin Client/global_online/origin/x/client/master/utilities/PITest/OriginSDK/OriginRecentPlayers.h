#ifndef __ORIGIN_RECENTPLAYERS_H__
#define __ORIGIN_RECENTPLAYERS_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup recplay Recent Players
/// \brief This module contains a function that allows Origin to get a list of Recent Players, users that the local user
/// has recently played with from his or her known games. 
/// 
/// For more information on the integration of this feature, see \ref ka-intguide-recentplayers in the <em>Integrator's Guide</em>.

/// \ingroup recplay
/// \brief Creates a list of recent players.
///
/// Because a game is free to use any matchmaking and multiplayer technology that it prefers, Origin has no implicit
/// knowledge of which other users someone has recently played with. This function allows the game to provide such a
/// list to Origin. Typically, this function is called when the game starts in multiplayer mode and is called during the
/// game if new players drop in.
/// \param user The local User.
/// \param pRecentList The list of recent users to add.
/// \param recentCount The number of recent users to add.
/// \param timeout The amount of time to wait for a reply from Origin in milliseconds.
/// \param callback The callback to call when the recent players are added.
/// \param pContext A user defined context, which will be included in the callback.
OriginErrorT ORIGIN_SDK_API OriginAddRecentPlayers(OriginUserT user, const OriginUserT *pRecentList, OriginSizeT recentCount, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);

/// \ingroup recplay
/// \brief Creates a list of recent players.
///
/// Because a game is free to use any matchmaking and multiplayer technology that it prefers, Origin has no implicit
/// knowledge of which other users someone has recently played with. This function allows the game to provide such a
/// list to Origin. Typically, this function is called when the game starts in multiplayer mode and is called during the
/// game if new players drop in.
/// \param user The local User.
/// \param pRecentList The list of recent users to add.
/// \param recentCount The number of recent users to add.
/// \param timeout The amount of time to wait for a reply from Origin in milliseconds.
OriginErrorT ORIGIN_SDK_API OriginAddRecentPlayersSync(OriginUserT user, const OriginUserT *pRecentList, OriginSizeT recentCount, uint32_t timeout);


/// \ingroup recplay
/// \brief Query the recent player list.
/// \param[in] user The local User.
/// \param[in] bThisGameOnly indicate whether the recent players are only from this game, or from any game played in Origin. 
/// \param[in] callback The function to call when this function completes.
/// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
/// \param[out] handle A handle to the enumerator object. This can be NULL.
OriginErrorT ORIGIN_SDK_API OriginQueryRecentPlayer(OriginUserT user, bool bThisGameOnly, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *handle);


/// \ingroup recplay
/// \brief Query the recent player list.
/// \param[in] user The local User.
/// \param[in] bThisGameOnly indicate whether the recent players are only from this game, or from any game played in Origin.
/// \param[out] total The total number of recent player records. OriginFriendT
/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
/// \param[out] handle A handle to the enumerator object. 
OriginErrorT ORIGIN_SDK_API OriginQueryRecentPlayerSync(OriginUserT user, bool bThisGameOnly, OriginSizeT *total, uint32_t timeout, OriginHandleT *handle);


#ifdef __cplusplus
}
#endif

#endif //__ORIGIN_RECENTPLAYERS_H__