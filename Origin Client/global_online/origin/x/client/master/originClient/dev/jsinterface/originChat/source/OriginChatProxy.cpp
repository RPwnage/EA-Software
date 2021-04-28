#include "OriginChatProxy.h"
#include "ConversationProxy.h"

#include <chat/JabberID.h>

#include <services/debug/DebugService.h>
#include <services/settings/SettingsManager.h>
#include <services/session/SessionService.h>
#include <engine/social/SocialController.h>
#include <engine/social/Conversation.h>
#include <engine/social/ConversationManager.h>
#include <engine/voice/VoiceController.h>

namespace
{
    using namespace Origin;
    Client::JsInterface::OriginChatProxy *SingletonInstance = NULL; 
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    using namespace Services;

    OriginChatProxy::OriginChatProxy(
        Engine::Social::SocialController *socialController,
        QObject *parent) :
        QObject(parent),
        mSocialController(socialController),
        mShowTimestamps(false),
        mNextConversationId(1)
    {
        ORIGIN_ASSERT(SingletonInstance == NULL);
        SingletonInstance = this;

        // Init our showTimestamps property and listen for changed
        settingChanged();

        Engine::Social::ConversationManager *convManager = socialController->conversations();

        ORIGIN_VERIFY_CONNECT(
                convManager, SIGNAL(multiUserChatCapabilityChanged(bool)),
                this, SIGNAL(multiUserChatCapabilityChanged()));

        ORIGIN_VERIFY_CONNECT(
            SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
            this, SLOT(settingChanged()));
    }
        
    OriginChatProxy::~OriginChatProxy()
    {
        ORIGIN_ASSERT(SingletonInstance == this);
        SingletonInstance = NULL;
    }

    OriginChatProxy* OriginChatProxy::instance()
    {
        return SingletonInstance;
    }
        
    QString OriginChatProxy::idForConversation(QSharedPointer<Engine::Social::Conversation> conversation)
    {
        Engine::Social::Conversation* conv = conversation.data();
        if (!conv)
            return QString();

        ConversationInfoLookup::const_iterator where = mConversationInfos.findbyConversation(*conv);
        if (where != mConversationInfos.end()) {
            return QString::number(where->id);
        }

        // Assign a new ID
        unsigned int conversationId = mNextConversationId++;

        ConversationInfo ci = { conversationId, conversation.toWeakRef(), conversation->getGroupGuid() };
        mConversationInfos.insert(mConversationInfos.end(), ci);

        // Listen for conversation destruction to clean up our maps
        ORIGIN_VERIFY_CONNECT(
                conversation.data(), SIGNAL(destroyed(QObject *)),
                this, SLOT(conversationDestroyed(QObject *)));

        // Listen for conversation finish to clean up our maps
        ORIGIN_VERIFY_CONNECT(
                conversation.data(), SIGNAL(conversationFinished(Origin::Engine::Social::Conversation::FinishReason)), 
                this, SLOT(conversationFinished()));

        return QString::number(conversationId);
    }
        
    QSharedPointer<Engine::Social::Conversation> OriginChatProxy::conversationForId(const QString &strId)
    {
        bool parsedOk;
        unsigned int conversationId = strId.toUInt(&parsedOk);

        if (!parsedOk)
        {
            return QSharedPointer<Engine::Social::Conversation>();
        }

        ConversationInfoLookup::const_iterator where = mConversationInfos.findById(conversationId);
        if (where == mConversationInfos.end()) {
            return QSharedPointer<Engine::Social::Conversation>();
        }

        return where->conversation.toStrongRef();
    }

    QString OriginChatProxy::conversationIdForGroupGuid(const QString &groupGuid)
    {
        ConversationInfoLookup::const_iterator where = mConversationInfos.findbyGuid(groupGuid);
        if (where == mConversationInfos.end()) {
            return QString();
        }

        return QString::number(where->id);
    }

    void OriginChatProxy::conversationDestroyed(QObject *convObject)
    {
        Engine::Social::Conversation *conversation = static_cast<Engine::Social::Conversation*>(convObject);
        if (!conversation)
            return;

        ConversationInfoLookup::const_iterator where = mConversationInfos.findbyConversation(*conversation);
        if (where == mConversationInfos.end())
            return;

        mConversationInfos.erase(where);
    }

    void OriginChatProxy::conversationFinished()
    {
        Engine::Social::Conversation* conversation = dynamic_cast<Engine::Social::Conversation*>(sender());
        if (!conversation)
            return;

        ConversationInfoLookup::iterator where = mConversationInfos.findbyConversation(*conversation);
        if (where == mConversationInfos.end())
            return;

        where->guid.clear();
    }

    
    QVariant OriginChatProxy::getConversationById(const QString &strId)
    {
		// see if we have a ConversationProxy cached 
        bool parsedOk;
        unsigned int conversationId = strId.toUInt(&parsedOk);
        if (!parsedOk)
            return QVariant();

        ConversationInfoLookup::iterator where = mConversationInfos.findById(conversationId);
        if (where == mConversationInfos.end())
            return QVariant();

        // Create a new proxy
        ConversationProxy *conversationProxy = new ConversationProxy(where->conversation.toStrongRef(), mSocialController, strId);
        return QVariant::fromValue<QObject*>(conversationProxy);
    }

    bool OriginChatProxy::showTimestamps()
    {
        return mShowTimestamps;
    }

    void OriginChatProxy::settingChanged()
    {
        const bool showTimestamps = readSetting(SETTING_SHOWTIMESTAMPS, Session::SessionService::currentSession());

        if (mShowTimestamps != showTimestamps)
        {
            mShowTimestamps = showTimestamps;
            emit showTimestampsChanged();
        }
    }
        
    int OriginChatProxy::maximumOutgoingMessageBytes()
    {
        return 2048;
    }

    QObjectList OriginChatProxy::conversations()
    {
        QObjectList ret;

        ConversationInfoLookup::iterator last(mConversationInfos.end());
        for (ConversationInfoLookup::iterator i(mConversationInfos.begin());
            i != last;
            ++i)
        {
            if (i->guid.isEmpty())
                continue;

            ConversationProxy* proxy = new ConversationProxy(i->conversation, mSocialController, QString::number(i->id));
            ret.append(proxy);
        }

        return ret;
    }

    bool OriginChatProxy::multiUserChatCapable()
    {
        return mSocialController->conversations()->multiUserChatCapable(); 
    }
}
}
}
