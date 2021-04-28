///////////////////////////////////////////////////////////////////////////////
// TelemetryBuffer.h
//
// A Telemetry Event storage class
//
// Created by Origin DevSupport
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGINTELEMETRY_TELEMETRYBUFFER_H
#define ORIGINTELEMETRY_TELEMETRYBUFFER_H

#include "EASTL/deque.h"

//Forward Declaration
namespace EA
{
    namespace Thread
    {
        class Futex;
    }
}

namespace OriginTelemetry
{
    //Forward Declaration
    class TelemetryEvent;

    class TelemetryBuffer
    {
    public: 
        /// TelemetryEvent buffer constructor.
        /// Takes the maximum size to allow the buffer to grow to.
        /// Once the buffer reaches max size. The oldest events will be removed to make room for new events.
        TelemetryBuffer( size_t maxBufferSize = 256000);
        virtual ~TelemetryBuffer();

        /// Write to the buffer.
        /// If the buffer is full the oldest events up to the size needed will be removed and this event will be inserted.
        virtual void write(const TelemetryEvent&);

        /// Write to the buffer - insert at the front.
        /// If the size of the event + the current size are bigger than the max size of the buffer, we discard the event.
        virtual void writeFront(const TelemetryEvent&);

        /// Get the oldest event from the buffer.
        /// Copies over the event passed in with the oldest event from the buffer. Unless the buffer was empty.
        /// In which case it will leave the passed in event unchanged
        /// \return returns false if the buffer was empty, else true.
        virtual bool read(TelemetryEvent&);

        /// Return number of TelemetryEvents in the queue
        int32_t numberOfBufferItems() const;

        /// Is this buffer empty or not.
        bool isEmpty() const;

    private:
        // disable copying of this class
        TelemetryBuffer(const TelemetryBuffer&); // no implementation 
        TelemetryBuffer& operator=(const TelemetryBuffer&); // no implementation 

        /// Get the internal futex for this class
        EA::Thread::Futex &GetLock();

        /// Helper function that will make sure that any new event will fit by removing the oldest events if necessary.
        void makeRoomForEvent(size_t newEventSize);
        
        eastl::deque<TelemetryEvent> m_telemetryEventQueue;
        size_t m_maxBufferSize;
        size_t m_bufferSize;

        EA::Thread::Futex *m_lock;

    };


} //namespace


#endif //ORIGINTELEMETRY_TELEMETRYBUFFER_H