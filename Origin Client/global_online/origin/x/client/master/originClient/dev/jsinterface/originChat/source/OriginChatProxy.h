#ifndef _ORIGINCHATPROXY_H
#define _ORIGINCHATPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/
#include <QObject>
#include <QString>
#include <QVariant>
#include <QHash>
#include <QWeakPointer>
#include <functional>

#include "engine/social/Conversation.h"
#include "services/plugin/PluginAPI.h"


namespace Origin
{

namespace Engine
{
namespace Social
{
    class SocialController;
}
namespace Voice
{
    class VoiceController;
}
}

namespace Client
{
namespace JsInterface
{
	class ConversationProxy;

    class ORIGIN_PLUGIN_API OriginChatProxy : public QObject
    {
        Q_OBJECT
    public:
        OriginChatProxy(
            Engine::Social::SocialController *socialController,
            QObject *parent = NULL);
        ~OriginChatProxy();

        // To be called from C++ to allocate an ID for a conversable
        static OriginChatProxy *instance();

        QString idForConversation(QSharedPointer<Engine::Social::Conversation>);
        QSharedPointer<Engine::Social::Conversation> conversationForId(const QString &id);
        QString conversationIdForGroupGuid(const QString &id);

        Q_INVOKABLE QVariant getConversationById(const QString &);

        Q_PROPERTY(bool showTimestamps READ showTimestamps);
        bool showTimestamps();

        Q_PROPERTY(int maximumOutgoingMessageBytes READ maximumOutgoingMessageBytes);
        int maximumOutgoingMessageBytes();

        Q_PROPERTY(QObjectList conversations READ conversations);
        QObjectList conversations();

        Q_PROPERTY(bool multiUserChatCapable READ multiUserChatCapable);
        bool multiUserChatCapable();

    signals:
        void showTimestampsChanged();
        void multiUserChatCapabilityChanged();

    private slots:
        void settingChanged();

        void conversationDestroyed(QObject *);

        void conversationFinished();

    private:
        Engine::Social::SocialController *mSocialController;

        bool mShowTimestamps;

        unsigned int mNextConversationId; 

		typedef struct {
			unsigned int id;
			QWeakPointer<Engine::Social::Conversation> conversation;
			QString guid;
		} ConversationInfo;

        class ConversationInfoLookup : public std::list< ConversationInfo > {

        public:

            ConversationInfoLookup::iterator findById(const unsigned int id);
            ConversationInfoLookup::const_iterator findById(const unsigned int id) const;

            ConversationInfoLookup::iterator findbyConversation(const Engine::Social::Conversation& conversation);
            ConversationInfoLookup::const_iterator findbyConversation(const Engine::Social::Conversation& conversation) const;

            ConversationInfoLookup::iterator findbyGuid(const QString& guid);
            ConversationInfoLookup::const_iterator findbyGuid(const QString& guid) const;

        private:

            struct IdCompare : public std::binary_function< ConversationInfo, const unsigned int, bool >
            {
                bool operator() (const ConversationInfo& lhs, const unsigned int id) const
                {
                    return lhs.id == id;
                }
            };

            struct GuidCompare : public std::binary_function< ConversationInfo, const QString&, bool >
            {
                bool operator() (const ConversationInfo& lhs, const QString& guid) const
                {
                    return lhs.guid == guid;
                }
            };

            struct ConversationCompare : public std::binary_function< ConversationInfo, const Engine::Social::Conversation&, bool >
            {
                bool operator() (const ConversationInfo& lhs, const Engine::Social::Conversation& conversation) const
                {
                    return lhs.conversation.data() == &conversation;
                }
            };

        };

        ConversationInfoLookup mConversationInfos;
    };

    // inlines

    inline OriginChatProxy::ConversationInfoLookup::iterator OriginChatProxy::ConversationInfoLookup::findById(const unsigned int id)
    {
        return std::find_if(begin(), end(), std::bind2nd<IdCompare>(IdCompare(), id));
    }

    inline OriginChatProxy::ConversationInfoLookup::const_iterator OriginChatProxy::ConversationInfoLookup::findById(const unsigned int id) const
    {
        return std::find_if(begin(), end(), std::bind2nd<IdCompare>(IdCompare(), id));
    }

    inline OriginChatProxy::ConversationInfoLookup::iterator OriginChatProxy::ConversationInfoLookup::findbyConversation(const Engine::Social::Conversation& conversation)
    {
        return std::find_if(begin(), end(), std::bind2nd<ConversationCompare>(ConversationCompare(), conversation));
    }

    inline OriginChatProxy::ConversationInfoLookup::const_iterator OriginChatProxy::ConversationInfoLookup::findbyConversation(const Engine::Social::Conversation& conversation) const
    {
        return std::find_if(begin(), end(), std::bind2nd<ConversationCompare>(ConversationCompare(), conversation));
    }

    inline OriginChatProxy::ConversationInfoLookup::iterator OriginChatProxy::ConversationInfoLookup::findbyGuid(const QString& guid)
    {
        return std::find_if(begin(), end(), std::bind2nd<GuidCompare>(GuidCompare(), guid));
    }

    inline OriginChatProxy::ConversationInfoLookup::const_iterator OriginChatProxy::ConversationInfoLookup::findbyGuid(const QString& guid) const
    {
        return std::find_if(begin(), end(), std::bind2nd<GuidCompare>(GuidCompare(), guid));
    }

}
}
}

#endif

