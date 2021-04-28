#ifndef __ORIGIN_CHAT_H__
#define __ORIGIN_CHAT_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C" 
{
#endif

	/// \defgroup chat Chat
	/// \brief This module contains the function that provides Origin's instant messaging feature.
	/// 
	/// For more information on the integration of this feature, see \ref ma-intguide-chat in the <em>Integrator's Guide</em>.

    /// \ingroup chat
    /// \brief Creates a chat session
    /// 
    ///	\param[in] user The local user who wants to create the session
    ///	\param[in] recipient The user(s) this chat is being created with
    /// \param[in] recipientCount The number of users this chat is being created with
    OriginErrorT ORIGIN_SDK_API OriginCreateChat(OriginUserT user, OriginUserT recipient[], OriginSizeT recipientCount);

    /// \ingroup chat
    /// \brief Sends a chat message
    /// 
    ///	\param[in] user The local user who wants to send a chat message
    ///	\param[in] message The message to be sent
    /// \param[in] chatId A handle to the chat
    OriginErrorT ORIGIN_SDK_API OriginSendChatMessage(OriginUserT user, const OriginCharT* message, OriginCharT* chatId);

    /// \ingroup chat
    /// \brief Adds a user to a chat
    /// 
    ///	\param[in] user The local user who wants to add a user to this chat.
    ///	\param[in] userToAdd The users to add to the chat
    /// \param[in] count The number of users to add to the chat
    /// \param[in] chatId A handle to the chat
    OriginErrorT ORIGIN_SDK_API OriginAddUserToChat(OriginUserT user, OriginUserT* userToAdd, OriginSizeT count, OriginCharT* chatId);

    /// \ingroup chat
    /// \brief Joins a chat
    /// 
    ///	\param[in] user The local user.
    /// \param[in] chatId A handle to the chat
    OriginErrorT ORIGIN_SDK_API OriginJoinChat(OriginUserT user, OriginCharT* chatId);

    /// \ingroup chat
    /// \brief Leaves a chat
    /// 
    ///	\param[in] user The local user.
    /// \param[in] chatId A handle to the chat
    OriginErrorT ORIGIN_SDK_API OriginLeaveChat(OriginUserT user, OriginCharT* chatId);

    /// \ingroup chat
    /// \brief Queries what users are in a given chat asynchronously
    /// 
    ///	\param[in] user The local user
    /// \param[in] chatId A handle to the chat
    ///	\param[in] callback The callback function to be called when information arrives. 
    ///	\param[in] pContext A user-defined context to be used in the callback.
    /// \param[in] timeout A timeout in milliseconds. 
    /// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
    OriginErrorT ORIGIN_SDK_API OriginQueryChatUsers(OriginUserT user, OriginCharT* chatId, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

    /// \ingroup chat
    /// \brief Queries what users are in a given chat synchronously
    /// 
    ///	\param[in] user The local user
    /// \param[in] chatId A handle to the chat
    /// \param[out] pTotalCount The total number of users available for reading.
    /// \param[in] timeout A timeout in milliseconds. 
    /// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
    OriginErrorT ORIGIN_SDK_API OriginQueryChatUsersSync(OriginUserT user, OriginCharT* chatId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

    /// \ingroup chat
    /// \brief Queries what chats are available
    /// 
    ///	\param[in] user The local user
    ///	\param[in] callback The callback function to be called when information arrives. 
    ///	\param[in] pContext A user-defined context to be used in the callback.
    /// \param[in] timeout A timeout in milliseconds. 
    /// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
    OriginErrorT ORIGIN_SDK_API OriginQueryChatInfo(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

    /// \ingroup chat
    /// \brief Queries what chats are available synchronously
    /// 
    ///	\param[in] user The local user
    /// \param[out] pTotalCount The total number of users available for reading.
    /// \param[in] timeout A timeout in milliseconds. 
    /// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration.
    OriginErrorT ORIGIN_SDK_API OriginQueryChatInfoSync(OriginUserT user, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_CHAT_H__