#ifndef _CONVERSATIONRECORD_H
#define _CONVERSATIONRECORD_H

#include <QDateTime>
#include <QList>
#include <QMutex>

namespace Origin
{
    namespace Engine
    {
        namespace Social
        {
            class ConversationEvent;
        }
    }
}

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    class ConversationHistoryManager;

    ///
    /// \brief Contains the date range and history for an active or historic conversation
    ///
    /// This class is thread-safe
    ///
    class ORIGIN_PLUGIN_API ConversationRecord
    {
        friend class ConversationHistoryManager;
    public:
        /// \brief Creates a new conversation record with the passed start time
        ConversationRecord(const QDateTime &startTime) :
            mStartTime(startTime)
        {
        }

        virtual ~ConversationRecord();

        /// \brief Returns the start time for this conversation
        QDateTime startTime() const
        {
            return mStartTime;
        }
        
        /// \brief Returns the end time for this conversation or null if the conversation is ongoing
        QDateTime endTime() const;

        /// \brief Returns all previous events in this conversation in chronological order
        QList<const ConversationEvent *> events() const;

    protected:
        /// 
        /// \brief Adds a new event to the end of our event history
        ///
        /// The conversation record takes ownership of the event
        ///
        void addEvent(const ConversationEvent *event);

        /// \brief Sets the end time of the conversation to the passed time
        void setEndTime(const QDateTime &endTime);

    private:
        mutable QMutex mStateMutex;

        QDateTime mStartTime;
        QDateTime mEndTime;

        QList<const ConversationEvent*> mEvents;
    };

}
}
}

#endif

