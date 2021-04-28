#ifndef __ORIGIN_GROUPS_H__
#define __ORIGIN_GROUPS_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// \defgroup groups Groups
/// \brief This module contains the functions that implements the Origin SDK groups features.
    

/// \ingroup groups
/// \brief Query the Group Members of a group
/// This function allows you to get the group members of the current group or a named group. The users will be returned in the \ref OriginFriendT structure.
///
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupId An optional group id that allows the game to specify a particular group instead of the default group. Pass in NULL or "" if you want to query the default group
/// \param callback The callback that will handle the returned enumeration.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \param handle The created enumerator. Pass in NULL here if you define a callback. The handle will be passed to the callback.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginQueryGroup(OriginUserT user, const OriginCharT * groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle);

/// \ingroup groups
/// \brief Query the Group Members of a group
/// This function allows you to get the group members of the current group or a named group. The users will be returned in the \ref OriginFriendT structure.
///
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupId An optional group id that allows the game to specify a particular group instead of the default group. Pass in NULL or "" if you want to query the default group
/// \param pTotalCount The number of OriginFriendT records returned.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \param handle The created enumerator. 
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginQueryGroupSync(OriginUserT user, const OriginCharT * groupId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT * handle);

/// \ingroup groups
/// \brief Get the current group meta data.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupId An optional group id that allows the game to specify a particular group instead of the default group. Pass in NULL or "" if you want to query the default group
/// \param callback The callback that will handle the returned information.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginGetGroupInfo(OriginUserT user, const OriginCharT * groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout);

/// \ingroup groups
/// \brief Get the current group meta data.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupId An optional group id that allows the game to specify a particular group instead of the default group. Pass in NULL or "" if you want to query the default group
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \param groupInfo The requested group info.
/// \param handle A resource handle for the strings in the response. Call \ref OriginDestroyHandle to release.
/// \return Returns ORIGIN_SUCCESS when the request is executed successfully. 
OriginErrorT ORIGIN_SDK_API OriginGetGroupInfoSync(OriginUserT user, const OriginCharT * groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle);

/// \ingroup groups
/// \brief Send an invite to play the users game session to the group members. Similar to \ref OriginSendInvite, but doesn't require to list all the members.
/// This function will only succeed if the user is in the \ref ORIGIN_PRESENCE_JOINABLE or \ref ORIGIN_PRESENCE_JOINABLE_INVITE_ONLY state.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param pUserList The users you want to invite to your game, and to your party. \note This is for users who aren't already in your party/group.
/// \param userCount The number of users in the pUserList array.
/// \param pMessage A message to accompany the invite. 
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginSendGroupGameInvite(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);

/// \ingroup groups
/// \brief Send an invite to play the users game session to the group members. Similar to \ref OriginSendInvite, but doesn't require to list all the members.
/// This function will only succeed if the user is in the \ref ORIGIN_PRESENCE_JOINABLE or \ref ORIGIN_PRESENCE_JOINABLE_INVITE_ONLY state.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param pUserList The users you want to invite to your game, and to your party. \note This is for users who aren't already in your party/group.
/// \param userCount The number of users in the pUserList array./// \param pMessage A message to accompany the invite. 
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginSendGroupGameInviteSync(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout);

/// \ingroup groups
/// \brief Create a group. If there is already a group with the same name, then that group will be entered instead of a newly group created. 
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupName Name the group. This name is used when invites are sent to other people to join.
/// \param type The type of group to create. /ref enumGroupType.
/// \param callback The callback that will handle the returned information.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginCreateGroup(OriginUserT user, const OriginCharT *groupName, enumGroupType type, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout);

/// \ingroup groups
/// \brief Create a group 
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupName Name the group. This name is used when invites are sent to other people to join.
/// \param type The type of group to create. /ref enumGroupType.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \param groupInfo The requested group info.
/// \param handle A resource handle for the strings in the response. Call \ref OriginDestroyHandle to release.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginCreateGroupSync(OriginUserT user, const OriginCharT *groupName, enumGroupType type, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle);

/// \ingroup groups
/// \brief This function will allow the user to enter the group that is associated with the game group invite. The user is already a member of the group.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupId The groupId of the group you want to enter. Cannot be "" or NULL.
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginEnterGroup(OriginUserT user, const OriginCharT *groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout);

/// \ingroup groups
/// \brief This function will allow the user to enter the group that is associated with the game group invite. The user is already a member of the group.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param groupId The groupId of the group you want to enter. Cannot be "" or NULL.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \param groupInfo The requested group info.
/// \param handle A resource handle for the strings in the response. Call \ref OriginDestroyHandle to release.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginEnterGroupSync(OriginUserT user, const OriginCharT *groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT *handle);

/// \ingroup groups
/// \brief This function will allow the user to leave the current group. This means that the user will no longer be part of the conversation between the members of the group, either through chat nor voip
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginLeaveGroup(OriginUserT user, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);

/// \ingroup groups
/// \brief This function will allow the user to leave the current group. This means that the user will no longer be part of the conversation between the members of the group, either through chat nor voip
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
OriginErrorT ORIGIN_SDK_API OriginLeaveGroupSync(OriginUserT user, uint32_t timeout);


/// \ingroup groups
/// \brief Send an invite to a list of users to join the group. This is not the same as a game invite, but more like a friends request. The user requires to have invite permissions.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param users A list of users to invite to join the group.
/// \param userCount The number of users to invite to join the group.
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
/// If the user has no permission to do this operation ORIGIN_ERROR_PROXY_UNAUTHORIZED is returned
OriginErrorT ORIGIN_SDK_API OriginInviteUsersToGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);

/// \ingroup groups
/// \brief Send an invite to a list of users to join the group. This is not the same as a game invite, but more like a friends request. The user requires to have invite permissions.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param users A list of users to invite to join the group.
/// \param userCount The number of users to invite to join the group.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is executed successfully. 
/// If the user has no permission to do this operation ORIGIN_ERROR_PROXY_UNAUTHORIZED is returned
OriginErrorT ORIGIN_SDK_API OriginInviteUsersToGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout);


/// \ingroup groups
/// \brief Remove the list of users from the group. The user needs to have permission to remove users from a group.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param users A list of users to remove from the group.
/// \param userCount The number of users to remove from the group.
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is sent successfully. Use the OriginErrorT value returned in the callback to determine whether the command successfully executed.
/// If the user has no permission to do this operation ORIGIN_ERROR_PROXY_UNAUTHORIZED is returned
OriginErrorT ORIGIN_SDK_API OriginRemoveUsersFromGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
   
/// \ingroup groups
/// \brief Remove the list of users from the group. The user needs to have permission to remove users from a group.
/// \param user The current user. Pass in OriginGetDefaultUser();
/// \param users A list of users to remove from the group.
/// \param userCount The number of users to remove from the group.
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return Returns ORIGIN_SUCCESS when the request is executed successfully. 
/// If the user has no permission to do this operation ORIGIN_ERROR_PROXY_UNAUTHORIZED is returned
OriginErrorT ORIGIN_SDK_API OriginRemoveUsersFromGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout);


/// \ingroup groups
/// \brief Enables the voip channel for the current group
/// \param bEnable Enables/Disables the VIOP channel for the group.
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return ORIGIN_PENDING when successfully initiated the VOIP channel monitor the Voip Events. EBISU_ERROR_NOT_READY if there is no microphone available. 
OriginErrorT ORIGIN_SDK_API OriginEnableVoip(bool bEnable, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);

/// \ingroup groups
/// \brief Enables the voip channel for the current group
/// \param bEnable Enables/Disables the VIOP channel for the group.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return ORIGIN_SUCCESS when successfully enabled the VOIP channel. EBISU_ERROR_NOT_FOUND if there is no microphone available. 
OriginErrorT ORIGIN_SDK_API OriginEnableVoipSync(bool bEnable, uint32_t timeout);

/// \ingroup groups
/// \brief Get the voip channel status for the current group
/// \param callback The callback that will tell the status of the Voip channel.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return ORIGIN_SUCCESS when successfully enabled the VOIP channel. 
OriginErrorT ORIGIN_SDK_API OriginGetVoipStatus(OriginVoipStatusCallback callback, void * pContext, uint32_t timeout);

/// \ingroup groups
/// \brief Get the voip channel status for the current group
/// \param status The structure that will receive the voip status.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return ORIGIN_SUCCESS when successfully received the VOIP status. 
OriginErrorT ORIGIN_SDK_API OriginGetVoipStatusSync(OriginVoipStatusT * status, uint32_t timeout);

/// \ingroup groups
/// \brief Mute a user from voip
/// \param bMute boolean indicating whether to mute or unmute the user/channel.
/// \param userId The user to mute. if userId == OriginGetDefaultUser() then this function will mute the current user.
/// \param groupId The groupId of the Group the user is in. Use NULL when referring to the default party.
/// \param callback The callback that will tell whether the operation was successful.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return ORIGIN_SUCCESS the user is muted from the conversation
OriginErrorT ORIGIN_SDK_API OriginMuteUser(bool bMute, OriginUserT userId, const OriginCharT *groupId, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);

/// \ingroup groups
/// \brief Mute a user from voip
/// \param bMute boolean indicating whether to mute or unmute the user/channel.
/// \param userId The user to mute. if userId == OriginGetDefaultUser() then this function will mute the current user.
/// \param groupId The groupId of the Group the user is in. Use NULL when referring to the default party.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \return ORIGIN_SUCCESS the user is muted from the conversation
OriginErrorT ORIGIN_SDK_API OriginMuteUserSync(bool bMute, OriginUserT userId, const OriginCharT *groupId, uint32_t timeout);

/// \ingroup groups
/// \brief Get the mute state of all members of the group.
/// \param groupId The groupId of the Group the user is in. Use NULL when referring to the default party.
/// \param callback The callback that will handle the returned enumeration.
/// \param pContext The application context for this callback.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \param handle The created enumerator. Pass in NULL here if you define a callback. The handle will be passed to the callback.
/// \return ORIGIN_SUCCESS the user is muted from the conversation
OriginErrorT ORIGIN_SDK_API OriginQueryMuteState(const OriginCharT *groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle);

/// \ingroup groups
/// \brief Get the mute state of all members of the group.
/// \param groupId The groupId of the Group the user is in. Use NULL when referring to the default party.
/// \param total The total number of mute state records.
/// \param timeout The time in milliseconds the game is willing to wait for a reply.
/// \param handle The created enumerator. Pass in NULL here if you define a callback. The handle will be passed to the callback.
/// \return ORIGIN_SUCCESS The query successfully complete.
OriginErrorT ORIGIN_SDK_API OriginQueryMuteStateSync(const OriginCharT *groupId, OriginSizeT * total, uint32_t timeout, OriginHandleT * handle);


#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_GROUPS_H__