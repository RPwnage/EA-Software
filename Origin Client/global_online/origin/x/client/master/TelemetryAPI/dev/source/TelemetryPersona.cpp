///////////////////////////////////////////////////////////////////////////////
// TelemetryPersona.cpp
//
// Keeps the Telemetry persona values
//// Copyright (c) 2010-2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#include "TelemetryPersona.h"
#include "TelemetryBuffer.h"
#include "TelemetryAPIManager.h"
#include "TelemetryLogger.h"

#include "Utilities.h"

#include "EAStdC/EAString.h"
#include "EAAssert/eaassert.h"

namespace OriginTelemetry
{

TelemetryPersona::TelemetryPersona() :
 mLocale(0)
 ,mNucleusId(0)
 ,mHashedNucleusId(0)
,mUserTelemetrySendSetting(SEND_TELEMETRY_SETTING_NOT_SET)
,mUserUnderageSetting(TELEMETRY_UNDERAGE_NOT_SET)
,mSubscriberStatus(TELEMETRY_SUBSCRIBER_STATUS_NOT_SET)
{
    mCriticalBuffer = new TelemetryBuffer();
    mNonCriticalBuffer = new TelemetryBuffer();
    TelemetryLogger::GetTelemetryLogger().logPersona(mNucleusId, locale());
}

TelemetryPersona::~TelemetryPersona()
{
    if(mCriticalBuffer)
    {
        if(!mCriticalBuffer->isEmpty())
        {
            TelemetryLogger::GetTelemetryLogger().logTelemetryWarning(eastl::string8().append_sprintf(
                "TelemetryPersona: Critical event buffer was not empty when destroyed there were %d events left for persona %I64u.", mCriticalBuffer->numberOfBufferItems(), mNucleusId) );
            logLostEvents(mCriticalBuffer);
        }
        delete mCriticalBuffer;
        mCriticalBuffer = NULL;
    }
    deleteNonCriticalBuffer();
}

void TelemetryPersona::logLostEvents( TelemetryBuffer* buffer)
{
    while( !buffer->isEmpty())
    {
        TelemetryEvent lostEvent;
        buffer->read(lostEvent);
        TelemetryLogger::GetTelemetryLogger().logLostTelemetryEvent(lostEvent);
    }
}

void TelemetryPersona::logUnsentEvents( TelemetryBuffer* buffer)
{
    while( !buffer->isEmpty())
    {
        TelemetryEvent unsentEvent;
        buffer->read(unsentEvent);
        TelemetryLogger::GetTelemetryLogger().logUnsentTelemetryEvent(unsentEvent);
    }
}

uint64_t TelemetryPersona::nucleusId() const
{
    if( isSendNonCriticalTelemetry() )
        return mNucleusId;
    return hashedNucleusId();
}

void TelemetryPersona::setNucleusId(uint64_t nucleusId)
{ 
    if (nucleusId == mNucleusId)
    {
        EA_FAIL_MSG("TelemetryPersona Nucleus Id was set to the same value. This should not happen.");
        return;
    }
    mNucleusId = nucleusId;
    mHashedNucleusId = 0;

    if(mNucleusId > 0)
    {
        eastl::string8 nucidString;
        nucidString.append_sprintf("%I64u", mNucleusId);
        mHashedNucleusId = NonQTOriginServices::Utilities::calculateFNV1A(nucidString.c_str());
    }

    TelemetryLogger::GetTelemetryLogger().logPersona(mNucleusId, locale());

    TelemetryAPIManager::getInstance().notify(TELEMETRY_MANAGER_EVENT_LOGIN);
}

void TelemetryPersona::insertTelemetryEvent(TelemetryEvent &telmEvent)
{
    if (telmEvent.isEmpty())
    {
        TelemetryLogger::GetTelemetryLogger().logTelemetryWarning("Attempted to insert an empty TelemetryEvent into persona buffer.");
        return;
    }

    //Set the subscriber status to this users subscriber status.
    telmEvent.setSubscriberStatus(mSubscriberStatus);
    // Log all new events to the telemetry.xml file (only logs if enabled) 
    // We do this here so they will have the subscriber state.
    TelemetryLogger::GetTelemetryLogger().logTelemetryEvent(telmEvent);

    switch (telmEvent.isCritical())
    {
        // If event is critical, insert it into critical buffer
        case CRITICAL:
            mCriticalBuffer->write(telmEvent);
            break;
        // If event is non-critical, insert it into non-critical buffer
        case NON_CRITICAL:
            if (mNonCriticalBuffer)
            {
                mNonCriticalBuffer->write(telmEvent);
            }
            else
                // log unsent event
                TelemetryLogger::GetTelemetryLogger().logUnsentTelemetryEvent(telmEvent);
            break;
        // Unknown enum value
        default:
        {
            EA_FAIL_MSG("Could not determine if telemetry events was critical or not.");
            TelemetryLogger::GetTelemetryLogger().logTelemetryWarning("Inserted Event with unknown criticality into non-critical buffer.");
            if(mNonCriticalBuffer)
                mNonCriticalBuffer->write(telmEvent);
        }
            break;
    }
}

void TelemetryPersona::reInsertTelemetryEvent(TelemetryEvent &telmEvent)
{
    if (telmEvent.isEmpty())
    {
        TelemetryLogger::GetTelemetryLogger().logTelemetryWarning("Attempted to insert an empty TelemetryEvent into persona buffer.");
        return;
    }
    switch (telmEvent.isCritical())
    {
        // If event is critical, insert it into critical buffer
    case CRITICAL:
        mCriticalBuffer->writeFront(telmEvent);
        break;
        // If event is non-critical, insert it into non-critical buffer
    case NON_CRITICAL:
        if (mNonCriticalBuffer)
        {
            mNonCriticalBuffer->writeFront(telmEvent);
        }
        else
            // log unsent event
            TelemetryLogger::GetTelemetryLogger().logUnsentTelemetryEvent(telmEvent);
        break;
        // Unknown enum value
    default:
    {
        EA_FAIL_MSG("Could not determine if telemetry events was critical or not.");
        TelemetryLogger::GetTelemetryLogger().logTelemetryWarning("Inserted Event with unknown criticality into non-critical buffer.");
        if (mNonCriticalBuffer)
            mNonCriticalBuffer->writeFront(telmEvent);
    }
    break;
    }
}

TelemetryEvent TelemetryPersona::readTelemetry()
{
    TelemetryEvent telmEvent;

    //Prioritize critical events to be sent first.
    if( !mCriticalBuffer->isEmpty() )
    {
        mCriticalBuffer->read(telmEvent);
    }
    else if(isSendNonCriticalTelemetry())
    {
        if (mNonCriticalBuffer)
        {
            mNonCriticalBuffer->read(telmEvent);
        }
    }
        
    // return the final resulting event, empty or not
    return telmEvent;
}
// if TELEMETRY_UNDERAGE_TRUE erase non-critical buffer
void TelemetryPersona::setUnderage(bool isUserUnderage)
{
    mUserUnderageSetting = isUserUnderage ? TELEMETRY_UNDERAGE_TRUE : TELEMETRY_UNDERAGE_FALSE;

    switch (mUserUnderageSetting)
    {
    case TELEMETRY_UNDERAGE_TRUE:
        deleteNonCriticalBuffer();
        break;
    case TELEMETRY_UNDERAGE_FALSE:
        if (!mNonCriticalBuffer && mUserTelemetrySendSetting != SEND_TELEMETRY_SETTING_CRITICAL_ONLY)
        {
            mNonCriticalBuffer = new TelemetryBuffer;
        }
        break;
    default:
    case TELEMETRY_UNDERAGE_NOT_SET:
        break;
    }

}

// if SEND_TELEMETRY_SETTING_CRITICAL_ONLY delete non-critical buffer (DTOR? add flag to not to log lost events.)
void TelemetryPersona::setUserSettingSendNonCriticalTelemetry(bool isSendNonCriticalTelemetry)
{
    // Also handle if the user opted in during Origin's execution
    mUserTelemetrySendSetting = isSendNonCriticalTelemetry ? SEND_TELEMETRY_SETTING_ALL : SEND_TELEMETRY_SETTING_CRITICAL_ONLY;

    switch (mUserTelemetrySendSetting)
    {
    case SEND_TELEMETRY_SETTING_CRITICAL_ONLY:
        deleteNonCriticalBuffer();
        break;
    case SEND_TELEMETRY_SETTING_ALL:
        if (!mNonCriticalBuffer && mUserUnderageSetting != TELEMETRY_UNDERAGE_TRUE)
        {
            mNonCriticalBuffer = new TelemetryBuffer;
        }
        break;
    case SEND_TELEMETRY_SETTING_NOT_SET:
    default:
        break;
    }
}

bool TelemetryPersona::isSendNonCriticalTelemetry() const
{
    return (mUserTelemetrySendSetting == SEND_TELEMETRY_SETTING_ALL && mUserUnderageSetting == TELEMETRY_UNDERAGE_FALSE);
}

void TelemetryPersona::deleteNonCriticalBuffer()
{
    if (mNonCriticalBuffer)
    {
        if (!mNonCriticalBuffer->isEmpty())
        {
            TelemetryLogger::GetTelemetryLogger().logTelemetryWarning(eastl::string8().append_sprintf(
                "TelemetryPersona: NON-Critical event buffer was not empty when destroyed there were %d events left for persona %I64u.", mNonCriticalBuffer->numberOfBufferItems(), mNucleusId));
            logUnsentEvents(mNonCriticalBuffer);
        }
        delete mNonCriticalBuffer;
        mNonCriticalBuffer = NULL;
    }
}

}//namespace
