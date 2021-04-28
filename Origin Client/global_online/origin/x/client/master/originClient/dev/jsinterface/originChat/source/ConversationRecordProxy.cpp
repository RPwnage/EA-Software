#include "ConversationRecordProxy.h"

#include <engine/social/ConversationRecord.h>

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    using Engine::Social::ConversationRecord;
    using Engine::Social::ConversationEvent;

    ConversationRecordProxy::ConversationRecordProxy(QWeakPointer<ConversationRecord> conversationRecord) :
        WebWidget::ScriptOwnedObject(NULL),
        mProxied(conversationRecord)
    {
    }

    QDateTime ConversationRecordProxy::startTime()
    {
        QSharedPointer<ConversationRecord> conversation = mProxied;
        if (conversation.isNull())
        {
            return QDateTime();
        }
        return conversation->startTime();
    }
    
    QDateTime ConversationRecordProxy::endTime()
    {
        QSharedPointer<ConversationRecord> conversation = mProxied;
        if (conversation.isNull())
        {
            return QDateTime();
        }

        return conversation->endTime();
    }

    QVariantList ConversationRecordProxy::events()
    {
        QVariantList ret;
#if 0 //disable for origin X
        QSharedPointer<ConversationRecord> conversation = mProxied;
        if (conversation.isNull())
        {
            return ret;
        }

        for (QList<const ConversationEvent *>::ConstIterator it = conversation->events().constBegin();
            it != conversation->events().constEnd();
            it++)
        {
            ret.append(ConversationEventDict::dictFromEvent(*it));
        }
#endif
        return ret;
    }
}
}
}
