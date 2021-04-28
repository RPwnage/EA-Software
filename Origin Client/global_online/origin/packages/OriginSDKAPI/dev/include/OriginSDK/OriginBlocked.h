#ifndef __ORIGIN_BLOCKED_H__
#define __ORIGIN_BLOCKED_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup blocked Block List
/// \brief This module contains the functions for accessing a user's Block List information.
/// 
/// There are sychronous and asynchronous versions of \ref OriginQueryBlockedUsers.
/// For an explanation of why, see \ref syncvsasync in the <em>Integrator's Guide</em>.
/// 
/// For more information on the integration of this feature, see \ref ja-intguide-ablockeduser in the <em>Integrator's Guide</em>.

/// \ingroup blocked
/// \brief Queries a user's blocked user information <a href="ba-intguide-designpatterns.html#snycvsasync">asynchronously</a>. The enumeration will return \ref OriginFriendT structures, but only the EAID, UserId, and PersonaId are filled in. 
/// 
/// Begin enumerating the user's blocked user list, returning a collection of OriginFriendT records.
///	\param[in] user The local user. Use OriginGetDefaultUser().
///	\param[in] callback The callback function to be called when information arrives. 
///	\param[in] pContext A user-defined context to be used in the callback.
/// \param[in] timeout A timeout in milliseconds. 
/// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
OriginErrorT ORIGIN_SDK_API OriginQueryBlockedUsers(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup blocked
/// \brief Queries a user's blocked user information <a href="ba-intguide-designpatterns.html#snycvsasync">synchronously</a>. The enumeration will return \ref OriginFriendT structures, but only the EAID, UserId, and PersonaId are filled in.
/// 
/// Begin enumerating the user's blocked user list, returning a collection of OriginFriendT records.
///	\param[in] user The local user. Use OriginGetDefaultUser().
/// \param[out] pTotalCount The total number of users available for reading.
/// \param[in] timeout A timeout in milliseconds.
/// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources 
/// associated with this enumeration.
OriginErrorT ORIGIN_SDK_API OriginQueryBlockedUsersSync(OriginUserT user, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup blocked
/// \brief Request to block a user.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToBlock The nucleus user id of the user that is requested to be blocked.
/// \param[in] timeout A timeout in milliseconds.
/// \param[in] callback The handler to be called with the result of the request.
/// \param[in] pContext A game provided context that will be passed back into the callback.
OriginErrorT ORIGIN_SDK_API OriginBlockUser(OriginUserT user, OriginUserT userToBlock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext); 

/// \ingroup blocked
/// \brief Request to block a user.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToBlock The nucleus user id of the user that is requested to be blocked.
/// \param[in] timeout A timeout in milliseconds.
OriginErrorT ORIGIN_SDK_API OriginBlockUserSync(OriginUserT user, OriginUserT userToBlock, uint32_t timeout); 

/// \ingroup blocked
/// \brief Request to unblock a user.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToUnblock The nucleus user id of the user that is requested to be unblocked.
/// \param[in] timeout A timeout in milliseconds.
/// \param[in] callback The handler to be called with the result of the request.
/// \param[in] pContext A game provided context that will be passed back into the callback.
OriginErrorT ORIGIN_SDK_API OriginUnblockUser(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext); 

/// \ingroup blocked
/// \brief Request to unblock a user.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToUnblock The nucleus user id of the user that is requested to be unblocked.
/// \param[in] timeout A timeout in milliseconds.
OriginErrorT ORIGIN_SDK_API OriginUnblockUserSync(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout); 

#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_BLOCKED_H__