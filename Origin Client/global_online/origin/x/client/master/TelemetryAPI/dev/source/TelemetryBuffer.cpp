///////////////////////////////////////////////////////////////////////////////
// TelemetryBuffer.cpp
//
//  A Telemetry Event storage class
//
// Created by Origin DevSupport
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "TelemetryEvent.h"
#include "TelemetryBuffer.h"
#include "TelemetryLogger.h"
#include "eathread/eathread_futex.h"

namespace OriginTelemetry
{

    TelemetryBuffer::TelemetryBuffer(size_t maxBufferSize):
        m_telemetryEventQueue(),
        m_maxBufferSize(maxBufferSize),
        m_bufferSize(0),
        m_lock(NULL)
    {
    }


    TelemetryBuffer::~TelemetryBuffer()
    {
        if(m_lock && m_lock->HasLock())
        {
            m_lock->Unlock();
        }
        if(m_lock)
        {
            delete m_lock;
        }
        m_lock=NULL;
    }

    EA::Thread::Futex& TelemetryBuffer::GetLock()
    {
        if(m_lock==NULL)
            m_lock = new EA::Thread::Futex();

        return *m_lock; 
    }

    void TelemetryBuffer::write(const TelemetryEvent &telmEvent)
    {
        makeRoomForEvent(telmEvent.getBufferSize());
        EA::Thread::AutoFutex m(this->GetLock());

        //insert into buffer
        m_telemetryEventQueue.push_back(telmEvent);

        m_bufferSize += telmEvent.getBufferSize();
    }

    void TelemetryBuffer::writeFront(const TelemetryEvent &telmEvent)
    {
        if ((m_bufferSize + telmEvent.getBufferSize()) > m_maxBufferSize)
        {
            return;
        }

        EA::Thread::AutoFutex m(this->GetLock());

        //insert into buffer
        m_telemetryEventQueue.push_front(telmEvent);

        m_bufferSize += telmEvent.getBufferSize();
    }

    bool TelemetryBuffer::read(TelemetryEvent &outEvent)
    {
        EA::Thread::AutoFutex m(this->GetLock());
        if( !m_telemetryEventQueue.empty() )
        {
            //pop from buffer
            outEvent = m_telemetryEventQueue.front();
            m_telemetryEventQueue.pop_front();
            m_bufferSize -= outEvent.getBufferSize();
            return true;
        }

        return false;
        
    }

    int32_t TelemetryBuffer::numberOfBufferItems() const
    {
        return m_telemetryEventQueue.size();
    }

    bool TelemetryBuffer::isEmpty() const
    {
        return m_telemetryEventQueue.empty();
    }

    void TelemetryBuffer::makeRoomForEvent(size_t newEventSize)
    {
        bool bufferNotEmpty = true;

        while( ((m_bufferSize + newEventSize) > m_maxBufferSize) && bufferNotEmpty )
        {
            //The buffer is full so we need to throw an event out to make the new one fit.
            TelemetryEvent lostEvent;
            bufferNotEmpty = read(lostEvent);
            TelemetryLogger::GetTelemetryLogger().logTelemetryWarning("TelemetryBuffer is full removing an event to make space");
            TelemetryLogger::GetTelemetryLogger().logLostTelemetryEvent(lostEvent);
        }
    }


} //namespace


