#include "ConversationProxy.h"
#include "OriginSocialProxy.h"
#include "RemoteUserProxy.h"
#include "ConversionHelpers.h"

#include <engine/social/SocialController.h>
#include <engine/social/ConversationEvent.h>
#include <engine/social/ConversationHistoryManager.h>
#include <engine/voice/VoiceController.h>

#include <chat/Connection.h>
#include <chat/Conversable.h>
#include <chat/Message.h>
#include <chat/MucRoom.h>
        
#include "ClientFlow.h"
#include "widgets/social/source/SocialViewController.h"
#include "flows/SummonToGroupChannelFlow.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/voice/VoiceService.h"

#include "TelemetryAPIDLL.h"

#include <typeinfo>

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    using namespace Engine::Social;

    // class ConversationProxy

    ConversationProxy::ConversationProxy(
        QWeakPointer<Conversation> conversation,
        Engine::Social::SocialController *socialController, 
        const QString &id) :

        ConversationRecordProxy(conversation),
        mConversation(conversation),
        mSocialController(socialController),
        mId(id),
        mFinishOnClose(false)
    {
        ORIGIN_VERIFY_CONNECT(
                conversation.data(), SIGNAL(eventAdded(const Origin::Engine::Social::ConversationEvent *)),
                this, SLOT(onConversationEvent(const Origin::Engine::Social::ConversationEvent *)));
        
        ORIGIN_VERIFY_CONNECT(
            conversation.data(), SIGNAL(typeChanged(Origin::Engine::Social::Conversation::ConversationType)),
            this, SIGNAL(typeChanged()));

        ORIGIN_VERIFY_CONNECT(
            conversation.data(), SIGNAL(updateGroupName(const QString&, const QString&)),
            this, SIGNAL(updateGroupName(const QString&, const QString&)));       

        ORIGIN_VERIFY_CONNECT(this, SIGNAL(inviteToRoom(const QObjectList&, const QString&, const QString&)), 
            ClientFlow::instance()->socialViewController(), SLOT(onInviteToRoom(const QObjectList&, const QString&, const QString&)));
    }

    ConversationProxy::~ConversationProxy()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (mFinishOnClose && conversation.isNull() == false)
        {
            conversation->finish(Engine::Social::Conversation::UserFinished);
        }
    }

    QString ConversationProxy::id()
    {
        return mId;
    }

    QObjectList ConversationProxy::previousConversations()
    {
        QObjectList ret;
        ConversationHistoryManager *historyManager = mSocialController->conversationHistory();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (!conversation.isNull()) {
            QSharedPointer<ConversationRecord> prevConvRecord = historyManager->previousRecordForConversable(mConversation.toStrongRef()->conversable());

            if (prevConvRecord)
            {
                ret << new ConversationRecordProxy(prevConvRecord);
            }
        }
        

        return ret;
    }

    void ConversationProxy::onConversationEvent(const ConversationEvent *evt)
    {
        if (!evt)
            return;

        QVariantMap evtDict(eventToDict(evt));

        if (0) //Origin::Services::readSetting(Origin::Services::SETTING_LogVoip))
        {
            ORIGIN_LOG_DEBUG << "ConversationProxy: reflecting " << typeid(*evt).name();

            QVariantMap::const_iterator last(evtDict.constEnd());
            for (QVariantMap::const_iterator i(evtDict.constBegin()); i != last; ++i)
            {
                ORIGIN_LOG_DEBUG << "    {" << i.key() << ", " << i.value().toString() << "}";
            }
        }

        emit eventAdded(evtDict);
    }

    // TBD: do we need this?
    //void ConversationProxy::onVoiceCallStateChanged(Origin::Engine::Social::Conversation::VoiceCallState newCallState)
    //{
    //    emit voiceCallStateChanged(voiceCallState());
    //}

    QObjectList ConversationProxy::participants()
    {
        QObjectList ret;
#if 0 //disable for OriginX
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull() || !conversation->conversable())
        {
            return ret;
        }

		QSet<Chat::ConversableParticipant> participants(mConversation.toStrongRef()->conversable()->participants());

        for(QSet<Chat::ConversableParticipant>::ConstIterator it = participants.constBegin();
            it != participants.constEnd();
            it++)
        {
            QObject *proxy = NULL;
            OriginSocialProxy* osp = OriginSocialProxy::instance();
            if (osp)
            {
                proxy = osp->remoteUserProxyByRemoteUser(it->remoteUser());
            }

            if (proxy)
            {
                ret.append(proxy);
            }
        }
#endif
        return ret;
    }
        
    void ConversationProxy::sendMessage(const QString &body, const QString &threadId)
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull() || !conversation->conversable())
        {
            return;
        }

        if (threadId.isNull())
        {
			conversation->conversable()->sendMessage(body);
        }
        else
        {
			conversation->conversable()->sendMessage(body, threadId);
        }
    }

    void ConversationProxy::sendComposing()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
		if (conversation.isNull() || !conversation->conversable())
        {
            return;
        }

		conversation->conversable()->sendComposing();
    }

    void ConversationProxy::sendStopComposing()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull() || !conversation->conversable())
        {
            return;
        }

        conversation->conversable()->sendStopComposing();
    }

    void ConversationProxy::sendPaused()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();

        if (conversation.isNull() || !conversation->conversable())
        {
            return;
        }

        conversation->conversable()->sendPaused();
    }

    void ConversationProxy::sendGone()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();

        if (conversation.isNull() || !conversation->conversable())
        {
            return;
        }

        conversation->conversable()->sendGone();
    }
        
    QString ConversationProxy::generateUniqueThreadId()
    {
        return Chat::Message::generateUniqueThreadId();
    }
    
    QVariantMap ConversationProxy::eventToDict(const ConversationEvent *e) const
    {
#if 0
        return ConversationEventDict::dictFromEvent(e);
#else
        return QVariantMap();
#endif
    }

    void ConversationProxy::finish()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (!conversation.isNull())
        {
            conversation->finish(Engine::Social::Conversation::UserFinished);
            mFinishOnClose = false;
        }
    }
        
    bool ConversationProxy::finishOnClose()
    {
        return mFinishOnClose;
    }

    void ConversationProxy::setFinishOnClose(bool finish)
    {
        mFinishOnClose = finish; 
    }

    void ConversationProxy::inviteRemoteUser(QObject *invitee)
    {
        RemoteUserProxy *proxy = dynamic_cast<RemoteUserProxy*>(invitee);
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();

        if (!proxy || conversation.isNull())
        {
            return;
        }

        conversation->inviteRemoteUser(proxy->proxied());
    }

    QString ConversationProxy::type()
    {
        using namespace Engine::Social;

        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();

        if (conversation.isNull())
        {
            return QString();
        }

        switch (conversation->type())
        {
        case Conversation::OneToOneType:
            return "ONE_TO_ONE";
        case Conversation::GroupType:
            return "MULTI_USER";
        case Conversation::DestroyedType:
            return "DESTROYED";
        case Conversation::KickedType:
            return "KICKED";
        case Conversation::LeftGroupType:
            return "LEFT_GROUP";
        }

        // Make MSVC++ happy
        return QString();
    }

    QString ConversationProxy::groupName()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QString();
        }
		return conversation->getGroupName();
    }

    QString ConversationProxy::groupGuid()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QString();
        }
        return conversation->getGroupGuid();
    }

    QString ConversationProxy::channelId()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QString();
        }
        return conversation->getChannelId();
    }

    QString ConversationProxy::channelName()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QString();
        }
        return conversation->getChannelName();
    }

    QObject* ConversationProxy::chatChannel()
    {
        QObject *proxy = NULL;
#if 0 //disable for OriginX
        OriginSocialProxy* osp = OriginSocialProxy::instance();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (osp && !conversation.isNull())
        {
			proxy = (QObject*)osp->chatChannelProxyByChatChannel(conversation->getChatChannel());
        }
#endif
        return proxy;
    }

    QObject* ConversationProxy::chatGroup()
    {
        QObject *proxy = NULL;
#if 0 //disable for OriginX
        OriginSocialProxy* osp = OriginSocialProxy::instance();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (osp && !conversation.isNull())
        {
			proxy = (QObject*)osp->chatGroupProxyByChatGroup(conversation->getGroup());
        }
#endif
        return proxy;
    }

    void ConversationProxy::joinVoice()
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if(isVoiceEnabled && !conversation.isNull())
			conversation->joinVoice();
#endif
    }

    void ConversationProxy::leaveVoice()
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (isVoiceEnabled && !conversation.isNull())
            conversation->leaveVoice();
#endif
    }

    void ConversationProxy::muteUserInVoice(QObject* user)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (isVoiceEnabled && !conversation.isNull())
        {
            RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
			conversation->muteUser(QString::number(remoteUser->proxied()->nucleusId()));
        }
#endif
    }

    void ConversationProxy::unmuteUserInVoice(QObject* user)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (isVoiceEnabled  && !conversation.isNull())
        {
            RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
			conversation->unmuteUser(QString::number(remoteUser->proxied()->nucleusId()));
        }
#endif
    }

    void ConversationProxy::muteUserByAdmin(QObject* user)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (isVoiceEnabled && !conversation.isNull())
        {
            RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
			conversation->muteUserByAdmin(QString::number(remoteUser->proxied()->nucleusId()));
        }
#endif
    }
    void ConversationProxy::muteSelfInVoice()
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (isVoiceEnabled && !conversation.isNull())
        {
            conversation->muteSelf();
        }
#endif
    }

    void ConversationProxy::unmuteSelfInVoice()
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (isVoiceEnabled && !conversation.isNull())
        {
            conversation->unmuteSelf();
        }
#endif
    }

    QVariant ConversationProxy::creationReason()
    {
        using Engine::Social::Conversation;

        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QVariant();
        }

        switch (conversation->creationReason())
        {
        case Conversation::JoinMucRoom:
            return "JOIN_MUC_ROOM";
        case Conversation::IncomingMessage:
            return "INCOMING_MESSAGE";
        case Conversation::InternalRequest:
            return "INTERNAL_REQUEST";
        case Conversation::Observe:
            return "OBSERVE";
        case Conversation::SDK:
            return "SDK";
        case Conversation::StartVoice:
            return "STARTVOICE";
        case Conversation::CreationReasonUnset:
        default:
            return QVariant();
        }

        return QVariant();
    }

    QVariant ConversationProxy::finishReason()
    {
        using Engine::Social::Conversation;

        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QVariant();
        }

        switch (conversation->finishReason())
        {
            case Conversation::UserFinished:
                return "USER_FINISHED";
            case Conversation::UserFinished_Invisible:
                return "USER_FINISHED_INVISIBLE";
            case Conversation::ForciblyFinished:
                return "FORCIBLY_FINISHED";
            case Conversation::NotFinished:
                return QVariant();
        }

        return QVariant();
    }
        
    QVariant ConversationProxy::voiceCallState()
    {
        using Engine::Social::Conversation;

        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QVariant();
        }

        int state = conversation->voiceCallState();

        switch(state)
        {
        case Conversation::VOICECALLSTATE_CONNECTED:
            return "VOICECALL_CONNECTED";
        case Conversation::VOICECALLSTATE_DISCONNECTED:
            return "VOICECALL_DISCONNECTED";
        case Conversation::VOICECALLSTATE_INCOMINGCALL:
            return "VOICECALL_INCOMING";
        case Conversation::VOICECALLSTATE_OUTGOINGCALL:
            return "VOICECALL_OUTGOING";
        }

        QString proxiedState((state & Conversation::VOICECALLSTATE_CONNECTED) ? "VOICECALL_CONNECTED" : "VOICECALL_DISCONNECTED");
        if (state & Conversation::VOICECALLSTATE_INCOMINGCALL)
            proxiedState.append(",VOICECALL_INCOMING");
        if (state & Conversation::VOICECALLSTATE_OUTGOINGCALL)
            proxiedState.append(",VOICECALL_OUTGOING");
        if (state & Conversation::VOICECALLSTATE_MUTED)
            proxiedState.append(",VOICECALL_MUTED");
        if (state & Conversation::VOICECALLSTATE_ADMIN_MUTED)
            proxiedState.append(",VOICECALL_ADMIN_MUTED");
        return QVariant(proxiedState);
    }

    QVariant ConversationProxy::remoteUserVoiceCallState(QObject* user)
    {
        using Engine::Social::Conversation;

        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return QVariant();
        }

        RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);

		int state = conversation->remoteUserVoiceCallState(QString::number(remoteUser->proxied()->nucleusId()));

        switch(state)
        {
        case Conversation::REMOTEVOICESTATE_NONE:
            return "REMOTE_VOICE_NONE";
        case Conversation::REMOTEVOICESTATE_VOICE:
            return "REMOTE_VOICE_ACTIVE";
        case Conversation::REMOTEVOICESTATE_LOCAL_MUTE:
            return "REMOTE_VOICE_LOCAL_MUTE";
        case Conversation::REMOTEVOICESTATE_REMOTE_MUTE:
            return "REMOTE_VOICE_REMOTE_MUTE";
        }

        QString proxiedState;
        if (state & Conversation::REMOTEVOICESTATE_VOICE)
            proxiedState.append(",REMOTE_VOICE_ACTIVE");
        if (state & Conversation::REMOTEVOICESTATE_LOCAL_MUTE)
            proxiedState.append(",REMOTE_VOICE_LOCAL_MUTE");
        if (state & Conversation::REMOTEVOICESTATE_REMOTE_MUTE)
            proxiedState.append(",REMOTE_VOICE_REMOTE_MUTE");

        return QVariant(proxiedState);
    }

    void ConversationProxy::requestLeaveConversation()
    {
        emit leaveRequested();
    }

    void ConversationProxy::inviteUsersToRoom(const QObjectList& users)
    {
        emit inviteToRoom(users, this->groupGuid(), this->channelId());
    }

    bool ConversationProxy::active()
    {
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (conversation.isNull())
        {
            return false;
        }

		return conversation->active();
    }

    bool ConversationProxy::isVoiceCallInProgress()
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        QSharedPointer<Conversation> conversation = mConversation.toStrongRef();
        if (isVoiceEnabled && !conversation.isNull())
        {
            return conversation->isVoiceCallInProgress();
        }
#endif
        return false;
    }

}
}
}
