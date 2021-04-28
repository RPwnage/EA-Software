///////////////////////////////////////////////////////////////////////////////
// EbisuTelemetryAPIConcrete.h
//
// Non publicly exposed implementation of an Origin specific telemetry API interface.
// Inherits from the publicly exposed version.
//
// Created by OriginDevSupport
// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EBISUTELEMETRYAPI_CONCRETE_H
#define EBISUTELEMETRYAPI_CONCRETE_H

#include "EbisuTelemetryAPI.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/list.h"

namespace OriginTelemetry
{

class ITelemetryWriter;

/// EbisuTelemetryAPI details that are not exposed in the public interface.
class EbisuTelemetryAPIConcrete : public EbisuTelemetryAPI 
    {
    public:
        explicit EbisuTelemetryAPIConcrete(eastl::shared_ptr<ITelemetryWriter> t_writter);
        virtual ~EbisuTelemetryAPIConcrete();

        /// Sets were the telemetryAPI will send it's telemetry.
        void setTelemetryWriter(eastl::shared_ptr<ITelemetryWriter> t_writter ); 

        /// Set the current user data on login
        virtual void setPersona(uint64_t nucleusid, const char8_t *country, bool isUnderage );
        ///Call this when the user has logged out.
        virtual void logout();
        /// Let the telemetry system know if the user has opted to send telemetry or not.
        virtual void setUserSettingSendNonCriticalTelemetry(bool isSendNonCriticalTelemetry);
        /// Let the telemetry system know that the user is underage or not.
        virtual void setUnderage(bool8_t isUserUnderage);
        /// set the subscription state of the current user
        virtual void setSubscriberStatus(OriginTelemetrySubscriberStatus val);
        /// Is this a BOI build or not
        virtual void setIsBOI( bool isBOI );
        /// Notify the manager that the dynamic config load has finished.
        virtual void startSendingTelemetry();

        /// \brief Add listener to telemetry events
        virtual void AddEventListener(TelemetryEventListener* listener);
        /// \brief Remove listener to telemetry events
        virtual void RemoveEventListener(TelemetryEventListener* listener);

    private:
        //Disable default constructor
        EbisuTelemetryAPIConcrete();
        //Disable copying
        EbisuTelemetryAPIConcrete( const EbisuTelemetryAPIConcrete&);
        EbisuTelemetryAPIConcrete& operator=(const EbisuTelemetryAPIConcrete &);

        ///Stores a Telemetry Event call as TelemetryEvent Object and puts it in a buffer to be sent to the Telemetry server.
        virtual void CaptureTelemetryData(const TelemetryDataFormatTypes *format, ...);

        /// gets the telemetry sending setting from the current telemetry writer.
        virtual bool isSendingNonCriticalTelemetry() const;
        /// gets the users telemetry setting.
        virtual TELEMETRY_SEND_SETTING userSettingSendNonCriticalTelemetry() const;
        /// gets the underage setting from the current telemetry writer.
        virtual TELEMETRY_UNDERAGE_SETTING isTelemetryUserUnderaged() const; 

        /// figure out what locale to use for a user
        uint32_t setLocale(const char8_t * country);

        uint32_t ConvertLocaleToUINT32(eastl::string8& countryString);

        // Telemetry writer interface
        eastl::shared_ptr<ITelemetryWriter> m_telmWriter;

        // List of listeners to call when an event is processed
        eastl::list<TelemetryEventListener*> m_listeners;

    };

} //namespace
#endif // EBISUTELEMETRYAPI_CONCRETE_H
