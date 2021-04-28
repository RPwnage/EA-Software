///////////////////////////////////////////////////////////////////////////////
// TelemetryWritter.h
//
// Interface which allows TelemetryEvent objects to be inserted into buffers
//
// Created by Origin DevSupport
// Copyright (c) 2010-2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EBISUTELEMETRY_WRITTER_H
#define EBISUTELEMETRY_WRITTER_H

#include "TelemetryEvent.h"
#include "GenericTelemetryElements.h"

namespace OriginTelemetry
{
    class ITelemetryWriter
    {
        public:
            /// \brief constructor
            ITelemetryWriter(){}
            /// \brief destructor
            virtual ~ITelemetryWriter(){}
            /// \brief insert a TelemetryEvent into a buffer
            virtual void insertTelemetryEvent(TelemetryEvent &telmEvent) = 0;
            /// \brief sets the persona's locale
            virtual void setLocale(uint32_t val) = 0;
            /// \brief sets the nucleus id for the current telemetry writer
            virtual void setNucleusId(uint64_t) = 0;
            /// sets if the current telemetry writer should treat telemetry as coming from an under aged user.
            virtual void setUnderage( bool isUserUnderage) = 0;
            /// get if the current telemetry writer is treating telemetry as coming from an under aged user.
            virtual TELEMETRY_UNDERAGE_SETTING isUnderage() const = 0;
            /// sets if the current telemetry writer should send non-critical telemetry or not.
            virtual void setUserSettingSendNonCriticalTelemetry( bool isSendNonCriticalTelemetry) = 0;
            /// returns what the current telemetry writer has for the users telemetry send setting.
            virtual TELEMETRY_SEND_SETTING userSettingSendNonCriticalTelemetry() const = 0;
            /// returns if the current telemetry writer is sending non-critical telemetry or not.
            virtual bool isSendNonCriticalTelemetry() const = 0;
            /// set the subscriber status for the current telemetry writer.
            virtual void setSubscriberStatus(OriginTelemetrySubscriberStatus status) = 0;

        private:
            /// \brief disable copying
            ITelemetryWriter( const ITelemetryWriter&);
            /// \brief disable copying
            ITelemetryWriter& operator=(const ITelemetryWriter &);
    };

} //namespace
#endif // EBISUTELEMETRY_WRITTER_H
