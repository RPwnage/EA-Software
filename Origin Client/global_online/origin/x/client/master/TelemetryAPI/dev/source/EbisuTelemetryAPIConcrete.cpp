///////////////////////////////////////////////////////////////////////////////
// EbisuTelemetryAPIConcrete.cpp
//
// Non publicly exposed implementation of an Origin specific telemetry API interface.
// Inherits from the publicly exposed version.
//
// Created by Origin DevSupport
// Copyright (c) Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "EbisuTelemetryAPIConcrete.h"
#include "TelemetryWriterInterface.h"
#include "TelemetryEvent.h"
#include "TelemetryLogger.h"
#include "TelemetryAPIManager.h"
#include "TelemetryConfig.h"
#include "SystemInformation.h"
#include "Utilities.h"

#include "EAAssert/eaassert.h"
#include <eathread/eathread_thread.h>

namespace OriginTelemetry
{

EbisuTelemetryAPIConcrete::EbisuTelemetryAPIConcrete( eastl::shared_ptr<ITelemetryWriter> t_writter):
    m_telmWriter(t_writter)
    ,m_listeners()
{
}

EbisuTelemetryAPIConcrete::~EbisuTelemetryAPIConcrete()
{
}

void EbisuTelemetryAPIConcrete::setTelemetryWriter( eastl::shared_ptr<ITelemetryWriter> t_writter )
{
    m_telmWriter = t_writter;
}

void EbisuTelemetryAPIConcrete::setPersona(uint64_t nucleusid, const char8_t *country, bool isUnderage )
{
    uint32_t locale = setLocale(country);

    if (m_telmWriter)
    {
        //Locale is set first since setNucleusId triggers the persona logging.
        m_telmWriter->setLocale(locale);
        m_telmWriter->setNucleusId(nucleusid);
        m_telmWriter->setUnderage(isUnderage);
    }
    else
    {
        EA_FAIL_MSG("Error: telemetry writer is NULL");
    }
}

void EbisuTelemetryAPIConcrete::logout()
{
    TelemetryAPIManager::getInstance().notify(TELEMETRY_MANAGER_EVENT_LOGOUT);
}

void EbisuTelemetryAPIConcrete::setUserSettingSendNonCriticalTelemetry(bool isSendNonCriticalTelemetry)
{
    if (m_telmWriter)
    {
        m_telmWriter->setUserSettingSendNonCriticalTelemetry(isSendNonCriticalTelemetry);
    }
    else
    {
        EA_FAIL_MSG("Error: telemetry writer is NULL");
    }
}

void EbisuTelemetryAPIConcrete::setUnderage(bool8_t isUserUnderage)
{
    if (m_telmWriter)
    {
        m_telmWriter->setUnderage(isUserUnderage);
    }
    else
    {
        EA_FAIL_MSG("Error: telemetry writer is NULL");
    }
}

void EbisuTelemetryAPIConcrete::setIsBOI( bool isBOI )
{
    TelemetryConfig::setIsBOI( isBOI );
}

void EbisuTelemetryAPIConcrete::startSendingTelemetry()
{
    TelemetryAPIManager::getInstance().notify(TELEMETRY_MANAGER_EVENT_READY_TO_SEND);
}

void EbisuTelemetryAPIConcrete::CaptureTelemetryData(const TelemetryDataFormatTypes *format, ...)
{
    va_list vl;
    va_start( vl, format );
    TelemetryEvent newEvent = TelemetryEvent( format, vl);
    va_end( vl );

    if(!newEvent.isEmpty())
    {
        if(!m_telmWriter)
        {
            EA_FAIL_MSG("TelemetryEvent sent after the telemetryWriter has been destroyed.");
            TelemetryLogger::GetTelemetryLogger().logTelemetryError("EbisuTelemetryAPIConcrete::CaptureTelemetryData() : TelemetryEvent sent after the telemetryWriter has been destroyed.");
            TelemetryLogger::GetTelemetryLogger().logLostTelemetryEvent(newEvent);
            return;
        }

        // Insert the event into a buffer
        m_telmWriter->insertTelemetryEvent(newEvent);

        for(auto it = m_listeners.begin(); it != m_listeners.end(); ++it)
        {
            TelemetryEventListener* listener = (*it);
            if(listener)
            {
                listener->processEvent(newEvent.getTimeStampUTC(), newEvent.getModuleStr().c_str(), newEvent.getGroupStr().c_str(), newEvent.getStringStr().c_str(), newEvent.getAttributeMap());
            }
        }

        // Telemetry Breakpoints
        if( TelemetryConfig::isTelemetryBreakpointsEnabled() )
        {
            va_start( vl, format );
            HandleTelemetryBreakpoint(format, vl);
            va_end( vl );
        }
    }
    else
    {
        EA_FAIL_MSG("Failed creating a Telemetry event.");
        TelemetryLogger::GetTelemetryLogger().logTelemetryError("EbisuTelemetryAPIConcrete::CaptureTelemetryData() : Failed creating a Telemetry event.");
        return;
    }

}

bool EbisuTelemetryAPIConcrete::isSendingNonCriticalTelemetry() const
{
    if (m_telmWriter)
    {
        return m_telmWriter->isSendNonCriticalTelemetry();
    }
    else
    {
        EA_FAIL_MSG("Error: telemetry writer is NULL");
    }
    return false;
}

TELEMETRY_SEND_SETTING EbisuTelemetryAPIConcrete::userSettingSendNonCriticalTelemetry() const
{
    if (m_telmWriter)
    {
        return m_telmWriter->userSettingSendNonCriticalTelemetry();
    }
    else
    {
        EA_FAIL_MSG("Error: telemetry writer is NULL");
    }
    return SEND_TELEMETRY_SETTING_NOT_SET;
}

TELEMETRY_UNDERAGE_SETTING EbisuTelemetryAPIConcrete::isTelemetryUserUnderaged() const
{
    if (m_telmWriter)
    {
        return m_telmWriter->isUnderage();
    }
    else
    {
        EA_FAIL_MSG("Error: telemetry writer is NULL");
    }
    return TELEMETRY_UNDERAGE_NOT_SET;
}

// TODO review this function. Looks messy
uint32_t EbisuTelemetryAPIConcrete::setLocale(const char8_t * country)
{
    uint32_t locale = 0;

    // Override the locale with EACore.ini setting (if it exists)
    if (TelemetryConfig::localeOverride().size() > 0)
    {
        locale = ConvertLocaleToUINT32(TelemetryConfig::localeOverride());
    }

    //  Case 1:  When the user logs in we get the billing country and pass that 2 letter
    //  code here where it is expanded to a 4 letter code using the language from OS.
    else if (country != NULL)
    {
        eastl::string8 tmp(country);

        //  Expand locale if it is only 2 characters long
        if (tmp.size() == 2)
        {
            //  Get the OS language
            eastl::string8 lang(NonQTOriginServices::SystemInformation::instance().GetUserLocale()); // "enUS" or "uz"

            //  Create language/country code
            lang.resize(2);
            tmp = lang;
            tmp += country;
        }

        //  Convert locale string into number
        NonQTOriginServices::Utilities::FourChars_2_UINT32(reinterpret_cast<char8_t*>(&locale), tmp.c_str(), tmp.size());
    }

    //  Case 2:  Anonymous telemetry uses the locale from the OS.
    else
    {
        //  Ensure that locale is in the form language/country
        eastl::string8 tmp = NonQTOriginServices::SystemInformation::instance().GetUserLocale();
        if (tmp.size() == 2)
        {
            eastl::string8 country(tmp);
            country.make_upper();
            tmp += country;
        }
        locale = ConvertLocaleToUINT32(tmp);
    }

    return locale;
}

// Set the current users Subscriber state.
void EbisuTelemetryAPIConcrete::setSubscriberStatus(OriginTelemetrySubscriberStatus val)
{
    m_telmWriter->setSubscriberStatus(val );
}

//    Parameter countryString will be changed to 4 letter code if necessary
uint32_t EbisuTelemetryAPIConcrete::ConvertLocaleToUINT32(eastl::string8& countryString)
{
    if (countryString.size() <= 2)
    {    
        eastl::string8 tmp("xx");
        tmp += countryString;
        countryString = tmp;
    }

    uint32_t ret=0;
    const char8_t* telemetryNoneString = "none";

    if (!countryString.empty())
        NonQTOriginServices::Utilities::FourChars_2_UINT32(reinterpret_cast<char8_t*>(&ret), countryString.c_str(), countryString.length());
    else
        NonQTOriginServices::Utilities::FourChars_2_UINT32(reinterpret_cast<char8_t*>(&ret), telemetryNoneString, sizeof(telemetryNoneString)/sizeof(telemetryNoneString[0]));

    return ret;
}


void EbisuTelemetryAPIConcrete::AddEventListener(TelemetryEventListener* listener)
{
    m_listeners.push_back(listener);
}

void EbisuTelemetryAPIConcrete::RemoveEventListener(TelemetryEventListener* listener)
{
    m_listeners.remove(listener); 
}


} //namespace
