///////////////////////////////////////////////////////////////////////////////
// TelemetryEvent.h
//
// A class to encapsulate a telemetry event sent by the client.
//
// Created by Origin DevSupport
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef ORIGINTELEMETRY_TELEMETRYEVENT_H
#define ORIGINTELEMETRY_TELEMETRYEVENT_H

#include "GenericTelemetryElements.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/map.h"

namespace Telemetry{ struct TelemetryApiEvent3T; }

namespace OriginTelemetry
{

    enum EventTypeCritical
    {
        NON_CRITICAL,
        CRITICAL,
        NOT_SET
    };

    using namespace Telemetry;
    class TelemetryEvent
    {
    public: 

        enum CREATE_TELEMETRY_EVENT_ERRORS
        {
            CREATE_TELEMETRY_EVENT_NO_ERRORS,
            CREATE_TELEMETRY_EVENT_ERROR_UNKNOWN_ATTRIB_TYPE,
            CREATE_TELEMETRY_EVENT_ERROR_TOO_MANY_ATTRIBUTES
        };

        /// default constructor that creates an empty event.
        TelemetryEvent(): m_eventBufferSharedPtr(), m_eventBufferSize(0), m_isCritical(NOT_SET), m_eventCounter(0), m_criticalTypeCounter(0), mSubscriberStatus(TELEMETRY_SUBSCRIBER_STATUS_NOT_SET){}

        /// constructor that takes list of event attributes. Used by client core.
        /// \param format list of attributes that constitute the event
        explicit TelemetryEvent(const TelemetryDataFormatTypes *format, va_list vl);
        
        /// copy constructor.
        TelemetryEvent( const TelemetryEvent &eventToCopy): 
            m_eventBufferSharedPtr(eventToCopy.m_eventBufferSharedPtr), 
            m_eventBufferSize(eventToCopy.m_eventBufferSize), 
            m_isCritical(eventToCopy.m_isCritical), 
            m_eventCounter(eventToCopy.m_eventCounter), 
            m_criticalTypeCounter(eventToCopy.m_criticalTypeCounter), 
            mSubscriberStatus(eventToCopy.mSubscriberStatus)
        { 
        }

        /// destructor.
        ~TelemetryEvent(){}

        /// assignment operator. Uses the copy and swap idiom. Not passing by reference in intentional.
        TelemetryEvent& operator= (TelemetryEvent eventToCopy)
        {
            swap(*this, eventToCopy);
            return *this;
        }

        //Swap method used to implement operator= copy and swap idiom.
        friend void swap(TelemetryEvent& first, TelemetryEvent& second) // nothrow
        {
            eastl::swap(first.m_eventBufferSharedPtr, second.m_eventBufferSharedPtr); 
            eastl::swap(first.m_eventBufferSize, second.m_eventBufferSize);
            eastl::swap(first.m_isCritical, second.m_isCritical);
            eastl::swap(first.m_eventCounter, second.m_eventCounter);
            eastl::swap(first.m_criticalTypeCounter, second.m_criticalTypeCounter);
            eastl::swap(first.mSubscriberStatus, second.mSubscriberStatus);
        }

        /// check if the event is empty.
        bool isEmpty() { return m_eventBufferSize == 0;}

        /// Function that inserts the internal telemetry event into the passed in event3.
        void deserializeToEvent3( TelemetryApiEvent3T& event3);

        /// getter for serialized buffer size.
        inline size_t getBufferSize() const {return m_eventBufferSize;}

        /// getter for timestamp.
        uint64_t getTimeStampUTC() const;

        /// getter for module string.
        eastl::string8 getModuleStr() const;

        /// getter for group string.
        eastl::string8 getGroupStr() const;

        /// getter for string string.
        eastl::string8 getStringStr() const;

        /// getter for parameter string
        eastl::string8 getParams() const;

        /// getter for metrics ID.
        uint32_t getMetricID() const;

        /// getter is this a critical telemetry hook or not.
        inline EventTypeCritical isCritical() const {return m_isCritical;}

        /// getter for the attribute map.
        /// The map returned by this function is sent to listeners.
        eastl::map<eastl::string8, TelemetryFileAttributItem*> getAttributeMap() const;

        inline uint32_t eventCount() const {return m_eventCounter;}
        inline uint32_t eventCriticalTypeCount() const {return m_criticalTypeCounter;}

        /// Set the subscription status of telemetry user that created this event.
        /// We store this per hook in-case the subscriber status changes so we report each
        /// event based on what the status was when it was generated.
        void setSubscriberStatus(OriginTelemetrySubscriberStatus status){ mSubscriberStatus = status; }

        // Get the subscriber status as a human readable string.
        const char* subscriberStatus() const;

    private:
        /// for common getter
        enum MGS
        {
            MODULE
            , GROUP
            , STRING
            , SKIP_MGS
        };

        /// common getter
        eastl::string8 getMGS(MGS myIdx) const;

        /// Parses attributes from attribMemoryPtr and stores them in the given telemetry file chunk.
        /// \param newChunk telemetry file chunk where the attributes are supposed to be stored
        /// \param attribMemoryPtr pointer to the buffer where the attributes are stored
        /// \param type attribute type
        /// \param t union where the attribute is to be stored
        CREATE_TELEMETRY_EVENT_ERRORS StoreAttribute(TelemetryFileChunk *newChunk, char8_t *&attribMemoryPtr, TelemetryDataFormatTypes type, TelemetryDataFormatType t);

        ///Helper function for extracting a telemetry attribute from a raw buffer
        inline void GetAttribute(const char8_t *src, char8_t *dst, uint64_t size) const {memcpy(dst, src, (size_t)size);}

        eastl::shared_ptr<char8_t> m_eventBufferSharedPtr;
        size_t m_eventBufferSize;
        EventTypeCritical m_isCritical;
        //Global counters for total, critical and non-critical events.
        static uint32_t s_eventCounter;
        static uint32_t s_criticalCounter;
        static uint32_t s_nonCriticalCounter;
        //Member counters for total and Critical or Non-critical events.
        uint32_t m_eventCounter;
        uint32_t m_criticalTypeCounter; //This will count either critical or non-critical depending on it's type.
        OriginTelemetrySubscriberStatus mSubscriberStatus;
    };

} //namespace OriginTelemetry


#endif // ORIGINTELEMETRY_TELEMETRYEVENT_H