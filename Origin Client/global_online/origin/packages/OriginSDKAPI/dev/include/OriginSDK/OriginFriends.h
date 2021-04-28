#ifndef __ORIGIN_FRIENDS_H__
#define __ORIGIN_FRIENDS_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/// \defgroup friends Friends
/// \brief This module contains the functions for Origin's basic social networking features.
/// 
/// There are sychronous and asynchronous versions of these two functions, \ref OriginQueryFriends and 
/// \ref OriginQueryAreFriends. For an explanation of why, see \ref syncvsasync in the <em>Integrator's Guide</em>.
/// 
/// For more information on the integration of this feature, see \ref ha-intguide-friends in the <em>Integrator's Guide</em>.

/// \ingroup friends
/// \brief Queries a user's friends' information <a href="ba-intguide-designpatterns.html#snycvsasync">asynchronously</a>. 
/// 
/// Begin enumerating the user's friends list, returning a collection of OriginFriendT records.
/// *** No assumptions should be made on the total number of friends returned ***
///	\param[in] user The local user. Use OriginGetDefaultUser().
///	\param[in] callback The callback function to be called when information arrives. 
///	\param[in] pContext A user-defined context to be used in the callback.
/// \param[in] timeout A timeout in milliseconds. 
/// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
/// \sa XFriendsCreateEnumerator(DWORD dwUserIndex, DWORD dwStartingIndex, DWORD dwFriendsToReturn, DWORD *pcbBuffer, HANDLE *ph)
OriginErrorT ORIGIN_SDK_API OriginQueryFriends(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup friends
/// \brief Queries a user's friends' information <a href="ba-intguide-designpatterns.html#snycvsasync">synchronously</a>. 
/// 
/// Begin enumerating the user's friends list, returning a collection of OriginFriendT records.
/// *** No assumptions should be made on the total number of friends returned ***
///	\param[in] user The local user. Use OriginGetDefaultUser().
/// \param[out] pTotalCount The total number of users available for reading.
/// \param[in] timeout A timeout in milliseconds.
/// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources 
/// associated with this enumeration.
/// \sa XFriendsCreateEnumerator(DWORD dwUserIndex, DWORD dwStartingIndex, DWORD dwFriendsToReturn, DWORD *pcbBuffer, HANDLE *ph)
OriginErrorT ORIGIN_SDK_API OriginQueryFriendsSync(OriginUserT user, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup friends
/// \brief Checks the relationship between two users <a href="ba-intguide-designpatterns.html#snycvsasync">asynchronously</a>. 
/// 
/// Determines the relationship between a local user and one or more other users.
/// \param user The local user. Use OriginGetDefaultUser().
/// \param pUserList A pointer to a list of user IDs.
/// \param iUserCount The number of users in the pUserList.
///	\param[in] callback The callback function to be called when information arrives. 
///	\param[in] pContext A user-defined context to be used in the callback.
/// \param[in] timeout A timeout in milliseconds. 
/// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
/// \sa XUserAreUsersFriends(DWORD dwUserIndex, PXUID pXuids, DWORD dwXuidCount, PBOOL pfResult, PXOVERLAPPED pOverlapped)
OriginErrorT ORIGIN_SDK_API OriginQueryAreFriends(OriginUserT user, const OriginUserT *pUserList, OriginSizeT iUserCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup friends
/// \brief Checks the relationship between two users <a href="ba-intguide-designpatterns.html#snycvsasync">synchronously</a>.
/// 
/// Determines the relationship between a local user and one or more other users.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] pUserList A pointer to a list of user IDs.
/// \param[in] iUserCount The number of user IDs.
/// \param[out] pTotalCount The number of records ready to be read.
/// \param[in] timeout A timeout in milliseconds. 
/// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
/// \sa XUserAreUsersFriends(DWORD dwUserIndex, PXUID pXuids, DWORD dwXuidCount, PBOOL pfResult, PXOVERLAPPED pOverlapped)
OriginErrorT ORIGIN_SDK_API OriginQueryAreFriendsSync(OriginUserT user, const OriginUserT *pUserList, OriginSizeT iUserCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup friends
/// \brief Request a user to become a friend.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToAdd The nucleus user id of the user that is requested to be friend.
/// \param[in] timeout A timeout in milliseconds.
/// \param[in] callback The handler to be called with the result of the request.
/// \param[in] pContext A game provided context that will be passed back into the callback.
OriginErrorT ORIGIN_SDK_API OriginRequestFriend(OriginUserT user, OriginUserT userToAdd, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext); 

/// \ingroup friends
/// \brief Request a user to become a friend.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToAdd The nucleus user id of the user that is requested to be added.
/// \param[in] timeout A timeout in milliseconds.
OriginErrorT ORIGIN_SDK_API OriginRequestFriendSync(OriginUserT user, OriginUserT userToAdd, uint32_t timeout); 

/// \ingroup friends
/// \brief Accept an external user's friend request.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToAccept The nucleus user id of the external user that has requested the local user to be their friend.
/// \param[in] timeout A timeout in milliseconds.
/// \param[in] callback The handler to be called with the result of the request.
/// \param[in] pContext A game provided context that will be passed back into the callback.
OriginErrorT ORIGIN_SDK_API OriginAcceptFriendInvite(OriginUserT user, OriginUserT userToAccept, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext); 

/// \ingroup friends
/// \brief Accept an external user's friend request.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToAccept The nucleus user id of the external user that has requested the local user to be their friend.
/// \param[in] timeout A timeout in milliseconds.
OriginErrorT ORIGIN_SDK_API OriginAcceptFriendInviteSync(OriginUserT user, OriginUserT userToAccept, uint32_t timeout); 


/// \ingroup friends
/// \brief Request to unfriend a user.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToRemove The nucleus user id of the user that is requested to be remove.
/// \param[in] timeout A timeout in milliseconds.
/// \param[in] callback The handler to be called with the result of the request.
/// \param[in] pContext A game provided context that will be passed back into the callback.
OriginErrorT ORIGIN_SDK_API OriginRemoveFriend(OriginUserT user, OriginUserT userToRemove, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext); 

/// \ingroup friends
/// \brief Request to unfriend a user.
/// \param[in] user The local user. Use OriginGetDefaultUser().
/// \param[in] userToRemove The nucleus user id of the user that is requested to be remove.
/// \param[in] timeout A timeout in milliseconds.
OriginErrorT ORIGIN_SDK_API OriginRemoveFriendSync(OriginUserT user, OriginUserT userToRemove, uint32_t timeout); 


#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_FRIENDS_H__