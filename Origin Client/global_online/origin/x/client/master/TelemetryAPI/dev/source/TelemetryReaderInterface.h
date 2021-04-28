///////////////////////////////////////////////////////////////////////////////
// TelemetryWritter.h
//
// Interface which allows TelemetryEvent objects to be read from a buffer
//
// Created by Origin DevSupport
// Copyright (c) 2010-2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EBISUTELEMETRY_READER_H
#define EBISUTELEMETRY_READER_H


namespace OriginTelemetry
{
    class TelemetryEvent;

    class ITelemetryReader
    {
        public:
            /// \brief constructor
            ITelemetryReader(){}
            /// \brief destructor
            virtual ~ITelemetryReader(){}
            /// \brief re insert a TelemetryEvent to the front of the buffer
            virtual void reInsertTelemetryEvent(TelemetryEvent &telmEvent) = 0;
            /// \brief returns a TelemetryEvent object from a TelemetryBuffer
            virtual TelemetryEvent readTelemetry() = 0;
            /// returns the current users locale
            virtual uint32_t locale() const = 0;
            /// \brief returns the current users nucleusId.
            virtual uint64_t nucleusId() const = 0;
            /// \brief returns the current users nucleusId hashed.
            virtual uint64_t hashedNucleusId() const = 0;

        private:
            /// \brief disable copying
            ITelemetryReader( const ITelemetryReader&);
            /// \brief disable copying
            ITelemetryReader& operator=(const ITelemetryReader &);
    };

} //namespace
#endif // EBISUTELEMETRY_READER_H
