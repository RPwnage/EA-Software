///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Telemetry.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Telemetry.h>
#include <EAStdC/EADateTime.h>

#if defined(EA_PLATFORM_WINDOWS)
    #include <Windows.h>
#endif



namespace EA
{
namespace Patch
{


void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryPatchSystemInit&){}
void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryPatchSystemShutdown&){}
void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryDirectoryRetrievalBegin&){}
void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryDirectoryRetrievalEnd&){}
void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryPatchBuildBegin&){}
void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryPatchBuildEnd&){}
void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryPatchCancelBegin&){}
void TelemetryEventCallback::TelemetryEvent(intptr_t, TelemetryPatchCancelEnd&){}



void TelemetryBase::Init()
{
    uint64_t timeSeconds = EA::Patch::GetTimeSeconds(); // This is based on Jan 1, 1970.
    mDateNumber.sprintf("%I64u", timeSeconds); // This assumes we are using EAStdC for sprintf support.

    char8_t timeString[kMinDateStringCapacity];
    time_t  t = (time_t)timeSeconds;    // This theoretically loses information, but in practice won't lose any info for another 25 years.
    EA::Patch::TimeTToDateString(t, timeString); 
    mDate = timeString;

    // To consider: Call this once and save the result. Beware that a static or global version can have memory allocation issues.
    mSystemInfo.Set();
}


} // namespace Patch
} // namespace EA







