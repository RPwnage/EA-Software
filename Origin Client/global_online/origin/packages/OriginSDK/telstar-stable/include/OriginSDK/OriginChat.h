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
    /// \brief Sends a chat message
    /// 
    ///	\param[in] fromUser The local user who wants to send a chat message
    ///	\param[in] toUser optional target user for the message. When sending to a groupId set this to 0
    ///	\param[in] message The message to be sent
    /// \param[in] groupId The groupId of the group you want to send the message to, if empty send to the default group, unless toUser is non-zero.
    OriginErrorT ORIGIN_SDK_API OriginSendChatMessage(OriginUserT fromUser, OriginUserT toUser, const OriginCharT * thread, const OriginCharT* message, const OriginCharT* groupId);

#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_CHAT_H__