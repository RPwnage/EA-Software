#include <QMutexLocker>

#include "engine/social/ConversationRecord.h"
#include "engine/social/ConversationEvent.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    ConversationRecord::~ConversationRecord()
    {
        // Delete all of our events
        for(QList<const ConversationEvent *>::ConstIterator it = mEvents.constBegin();
            it != mEvents.constEnd();
            it++)
        {
            delete *it;
        }
    }
        
    QList<const ConversationEvent *> ConversationRecord::events() const
    {
        QMutexLocker locker(&mStateMutex);
        return mEvents;
    }
        
    void ConversationRecord::addEvent(const ConversationEvent *event)
    {
        QMutexLocker locker(&mStateMutex);
        mEvents.append(event);
    }
    
    QDateTime ConversationRecord::endTime() const
    {
        QMutexLocker locker(&mStateMutex);
        return mEndTime;
    }
       
    void ConversationRecord::setEndTime(const QDateTime &endTime)
    {
        QMutexLocker locker(&mStateMutex);
        mEndTime = endTime;
    }
}
}
}
