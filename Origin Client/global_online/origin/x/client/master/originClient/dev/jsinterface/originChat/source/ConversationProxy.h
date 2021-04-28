#ifndef _CONVERSATIONPROXY_H
#define _CONVERSATIONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/
#include <QObject>
#include <QObjectList>
#include <QVariantMap>
#include <QSharedPointer>

#include <engine/social/Conversation.h>
#include <services/plugin/PluginAPI.h>

#include "ConversationRecordProxy.h"

namespace Origin
{

namespace Chat
{
    class Message;
}

namespace Engine
{
namespace Social
{
    class Conversation;
    class ConversationEvent;
    class SocialController;
}
#ifdef ORIGIN_PC
namespace Voice
{
    VoiceController;
}
#endif
}

namespace Client
{
namespace JsInterface
{
    class ORIGIN_PLUGIN_API ConversationProxy : public ConversationRecordProxy
    {
        Q_OBJECT
    public:
        ConversationProxy(
            QWeakPointer<Engine::Social::Conversation> conversation,
            Engine::Social::SocialController *,
            const QString &id);
        virtual ~ConversationProxy();

        Q_PROPERTY(QString id READ id);
        QString id();
        
        Q_PROPERTY(bool finishOnClose READ finishOnClose WRITE setFinishOnClose);
        bool finishOnClose();
        void setFinishOnClose(bool finish);

        Q_PROPERTY(QObjectList previousConversations READ previousConversations);
        QObjectList previousConversations();
        
        Q_PROPERTY(QObjectList participants READ participants);
        QObjectList participants();
        
        Q_PROPERTY(QString type READ type);
        QString type();

        Q_PROPERTY(QString groupName READ groupName)
        QString groupName();

        Q_PROPERTY(QString groupGuid READ groupGuid)
        QString groupGuid();

		Q_PROPERTY(QString voiceChannel READ voiceChannel)
			QString voiceChannel();
        
        Q_PROPERTY(QString channelId READ channelId)
        QString channelId();

        Q_PROPERTY(QString channelName READ channelName)
        QString channelName();

        Q_PROPERTY(QObject* chatChannel READ chatChannel)
        QObject* chatChannel();

        Q_PROPERTY(QObject* chatGroup READ chatGroup)
        QObject* chatGroup();

        Q_INVOKABLE void sendMessage(const QString &body, const QString &threadId = QString());

        Q_INVOKABLE void sendComposing();

        Q_INVOKABLE void sendStopComposing();

        Q_INVOKABLE void sendPaused();

        Q_INVOKABLE void sendGone();

        Q_INVOKABLE QString generateUniqueThreadId();

        Q_INVOKABLE void finish();

        Q_INVOKABLE void inviteRemoteUser(QObject *invitee);

        Q_INVOKABLE void joinVoice();

        Q_INVOKABLE void leaveVoice();

        Q_INVOKABLE void muteSelfInVoice();

        Q_INVOKABLE void unmuteSelfInVoice();

        Q_INVOKABLE void muteUserInVoice(QObject* user);

        Q_INVOKABLE void unmuteUserInVoice(QObject* user);

        Q_INVOKABLE void muteUserByAdmin(QObject* user);

        Q_INVOKABLE QVariant remoteUserVoiceCallState(QObject* user);

        Q_INVOKABLE void requestLeaveConversation();

        Q_INVOKABLE void inviteUsersToRoom(const QObjectList& users);

        Q_PROPERTY(QVariant creationReason READ creationReason);
        QVariant creationReason();

        Q_PROPERTY(QVariant finishReason READ finishReason);
        QVariant finishReason();

        Q_PROPERTY(bool active READ active);
        bool active();

        Q_PROPERTY(bool isVoiceCallInProgress READ isVoiceCallInProgress);
        bool isVoiceCallInProgress();

        Q_PROPERTY(QVariant voiceCallState READ voiceCallState);
        QVariant voiceCallState();

        QSharedPointer<Engine::Social::Conversation> proxied() { return mConversation; }

    signals:
        void eventAdded(QVariantMap);
        void typeChanged();
        void leaveRequested();
        void updateGroupName(const QString&, const QString&);
        void inviteToRoom(const QObjectList& users, const QString&, const QString&);

    private slots:

        void onConversationEvent(const Origin::Engine::Social::ConversationEvent *);

    private:
        QVariantMap eventToDict(const Engine::Social::ConversationEvent *) const;

		QWeakPointer<Engine::Social::Conversation> mConversation;
        Engine::Social::SocialController *mSocialController;
        QString mId;
        bool mFinishOnClose;
    };

	// inlines functions follow:

	inline QString ConversationProxy::voiceChannel()
	{
#if ENABLE_VOICE_CHAT
        QSharedPointer<Origin::Engine::Social::Conversation> conversation = mConversation.toStrongRef();
        if (!conversation.isNull())
        {
            return conversation->voiceChannel();
        }
#endif
        return QString();
	}

}
}
}

#endif
