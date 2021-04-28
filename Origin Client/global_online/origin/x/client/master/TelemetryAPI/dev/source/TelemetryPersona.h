///////////////////////////////////////////////////////////////////////////////
// TelemetryPersona.h
//
// Keeps the Telemetry persona values
//// Copyright (c) 2010-2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef TelemetryPersona_h__
#define TelemetryPersona_h__


#include "TelemetryWriterInterface.h"
#include "TelemetryReaderInterface.h"

#include "GenericTelemetryElements.h"

#include <EASTL/string.h>

namespace OriginTelemetry
{
    class TelemetryBuffer;

    class TelemetryPersona : public ITelemetryWriter, public ITelemetryReader
    {
    public:
        /// \brief default CTOR
        TelemetryPersona();
        
        /// \brief returns locale as int
        inline uint32_t locale() const{ return mLocale;}
        /// \brief sets locale
        inline void setLocale(uint32_t val){ mLocale = val;}

        /// \brief returns nucleusId.
        uint64_t nucleusId() const;
        /// \brief sets nucleusId.
        void setNucleusId(uint64_t nucleusId);

        /// set if the user is underage or not.
        void setUnderage(bool isUserUnderage );
        /// get if this persona is for under aged user.
        inline TELEMETRY_UNDERAGE_SETTING isUnderage() const { return mUserUnderageSetting; }

        /// Set whether this user has opted to send non-critical telemetry or not.
        void setUserSettingSendNonCriticalTelemetry(bool isSendNonCriticalTelemetry );
        /// Set whether this user has opted to send non-critical telemetry or not.
        inline TELEMETRY_SEND_SETTING  userSettingSendNonCriticalTelemetry() const { return mUserTelemetrySendSetting;}

        /// Are we sending non-critical telemetry or not.
        bool isSendNonCriticalTelemetry() const;

        /// \brief insert event into a buffer (critical or non-critical)
        void insertTelemetryEvent(TelemetryEvent &telmEvent);

        /// \brief insert event into a buffer (to the front) (critical or non-critical)
        void reInsertTelemetryEvent(TelemetryEvent &telmEvent);

        /// \brief reads from either the critical or non-critical buffer and returns a TelemetryEvent object
        TelemetryEvent readTelemetry();

        /// \brief returns hashed nucleusid
        uint64_t hashedNucleusId() const {return mHashedNucleusId;}

        /// set the subscription status of the current telemetry user.
        void setSubscriberStatus(OriginTelemetrySubscriberStatus status){ mSubscriberStatus = status; }

        /// \brief DTOR
        virtual ~TelemetryPersona();

    private:
        // disabled copy
        TelemetryPersona(const TelemetryPersona&);
        TelemetryPersona& operator=(const TelemetryPersona&);
        TelemetryPersona& operator&(const TelemetryPersona&);

        ///Helper function for logging out telemetry events that we lose on destruction.
        void logLostEvents(TelemetryBuffer* buffer);

        ///Helper function for logging out unsent telemetry events that we lose on destruction.
        void logUnsentEvents( TelemetryBuffer* buffer);

        /// \brief helper function to delete critical buffer
        void deleteNonCriticalBuffer();

        uint32_t mLocale;
        uint64_t mNucleusId;
        uint64_t mHashedNucleusId;
        
        TELEMETRY_SEND_SETTING mUserTelemetrySendSetting;

        TELEMETRY_UNDERAGE_SETTING mUserUnderageSetting;

        // Buffers
        TelemetryBuffer *mCriticalBuffer;
        TelemetryBuffer *mNonCriticalBuffer;

        OriginTelemetrySubscriberStatus mSubscriberStatus;

    };
}

#endif // TelemetryPersona_h__
