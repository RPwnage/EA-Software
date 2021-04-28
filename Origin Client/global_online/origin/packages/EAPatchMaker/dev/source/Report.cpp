/////////////////////////////////////////////////////////////////////////////
// EAPatchMaker/Report.cpp
//
// Copyright (c) Electronic Arts Inc. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchMaker/Report.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EADateTime.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if defined(EA_PLATFORM_WINDOWS)
    #ifdef _MSC_VER
        #pragma warning(push, 0) // Microsoft headers generate warnings at the high warning levels we use.
    #endif
    #include <Windows.h>
    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#endif



namespace EA
{
namespace Patch
{


/// ReportImpl
///
/// This is the default report function. 
/// It does not append any newlines to the output nor does it require
/// the user to do so. It simply passes on the input to stdout.
/// If the user wants the output to have a newline, the user must supply it.
/// This allows the user to report multiple text items to the same line if desired.
///
static void ReportImpl(const char8_t* pMessage)
{
    if(pMessage)
    {
        // It's possible that the underlying print back end can't handle large
        // output sizes. For example, the OutputDebugStringA call below drops
        // chars beyond about 4096.
        size_t       length = EA::StdC::Strlen(pMessage); // It might be faster to make a custom Strlen which quits after N chars.
        const size_t kMaxLength = 1024;

        if(length > kMaxLength)
        {
            for(size_t i = 0, copiedLength = 0; i < length; i += copiedLength)
            {
                char8_t buffer[kMaxLength + 1];
                size_t  c;

                copiedLength = ((length - i) >= kMaxLength) ? kMaxLength : (length - i);
                for(c = 0; c < copiedLength; c++)
                    buffer[c] = pMessage[i + c];
                buffer[c] = 0;

                ReportImpl(buffer);
            }
        }
        else
        {
            #if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_XENON) // No need to do this for Microsoft console platforms, as the fputs below covers that.
                OutputDebugStringA(pMessage);
            #endif

            // Note: If we want to support additional platforms, then copy the 
            // code from the EATest package at EATest/source/EATest.cpp. Some platforms
            // may support fputs but you still can't use them for one reason or another.

            fputs(pMessage, stdout);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// ReportVerbosityV
//
void ReportVerbosityV(unsigned minVerbosity, const char8_t* pFormat, va_list arguments)
{
    if(pFormat && (GetReportVerbosity() >= minVerbosity))
    {
        #if defined(EA_PLATFORM_DESKTOP)
        const int kBufferSize = 2048;
        #else
        const int kBufferSize = 512;
        #endif
        char buffer[kBufferSize];

        const int nReturnValue = EA::StdC::Vsnprintf(buffer, kBufferSize, pFormat, arguments);

        if((nReturnValue >= 0) && (nReturnValue < (int)kBufferSize))
            ReportImpl(buffer);
        else if(nReturnValue < 0) // If we simply didn't have enough buffer space.
        {
            ReportImpl("Invalid format specified.\n    Format: ");
            ReportImpl(pFormat);
        }
        else // Else we simply didn't have enough buffer space.
        {
            char* pBuffer = new char[nReturnValue + 1];

            if(pBuffer)
            {
                EA::StdC::Vsnprintf(pBuffer, nReturnValue + 1, pFormat, arguments);
                ReportImpl(pBuffer);
                delete[] pBuffer;
            }
            else
                ReportImpl("Unable to allocate buffer space for large printf.\n");
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// Report
//
void Report(const char8_t* pFormat, ...)
{
    va_list arguments;
    va_start(arguments, pFormat);
    ReportVerbosityV(0, pFormat, arguments);
    va_end(arguments);
}


///////////////////////////////////////////////////////////////////////////////
// Report
//
void ReportVerbosity(unsigned minVerbosity, const char8_t* pFormat, ...)
{
    va_list arguments;
    va_start(arguments, pFormat);
    ReportVerbosityV(minVerbosity, pFormat, arguments);
    va_end(arguments);
}


int ReportVerbosityCondition(bool condition, unsigned minVerbosity, const char* pFormat, ...)
{
    va_list arguments;
    va_start(arguments, pFormat);
    if(condition)
        ReportVerbosityV(minVerbosity, pFormat, arguments);
    va_end(arguments);

    return condition ? 1 : 0;
}




///////////////////////////////////////////////////////////////////////////////
// GetReportVerbosity / SetReportVerbosity
///////////////////////////////////////////////////////////////////////////////

unsigned gReportVerbosity = kReportVerbosityDefault;

unsigned GetReportVerbosity()
{
    return gReportVerbosity;
}


void SetReportVerbosity(unsigned reportVerbosity)
{
    gReportVerbosity = reportVerbosity;
}


} // namespace Patch

} // namespace EA








