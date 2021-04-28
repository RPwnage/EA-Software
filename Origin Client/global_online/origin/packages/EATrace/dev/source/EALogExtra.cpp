///////////////////////////////////////////////////////////////////////////////
// EALogExtra.cpp
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Created by Alex Liberman
///////////////////////////////////////////////////////////////////////////////


#include <EATrace/internal/Config.h>
#include <EATrace/EALogExtra.h>
#include <EAStdC/EAString.h>

#if defined(EA_PLATFORM_WINDOWS)
    #pragma warning(push, 0)
    #include <Windows.h>
    #pragma warning(pop)
#endif


namespace EA {
    namespace Trace {

        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            BOOL CALLBACK EnumWinProc(HWND hwnd, LPARAM lParam)
            {
                DWORD dwProcessId;
                GetWindowThreadProcessId(hwnd, &dwProcessId);   // If the given hwnd belongs to this process...
                
                if(dwProcessId == GetCurrentProcessId())
                {
                    HWND* pHWND = (HWND*)lParam;
                    *pHWND = hwnd;
                    return false;  // End the enumeration.
                }

                return true; // Continue the enumeration.
            }

            /// GetMainHWND
            /// Returns the HWND for what is likely the main window of an application
            /// If the application has multiple topmost-level windows, then this function
            /// returns the first of them.
            HWND GetMainHWND()
            {
                HWND gResult = NULL;
                EnumWindows(EnumWinProc, (LPARAM)(uintptr_t)&gResult);
                return gResult;
            }
        #endif


        /// class LogReporterTitleBar
        LogReporterTitleBar::LogReporterTitleBar(const char8_t* pName) 
            : Trace::LogReporter(pName)
        { 
        }

        bool LogReporterTitleBar::IsFiltered(const Trace::TraceHelper& helper) 
        {
            return ((helper.GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(helper); 
        }

        bool LogReporterTitleBar::IsFiltered(const Trace::LogRecord& record)   
        { 
            const tOutputType outputType = record.GetTraceHelper()->GetOutputType();
            const bool filtered = LogReporter::IsFiltered(record);
            return ((outputType & kOutputTypeText) == 0) || filtered; 
        }

        /// Trace a LogRecord to the title bar
        tAlertResult LogReporterTitleBar::Report(const Trace::LogRecord& record)
        {
            const char8_t* const pOutput = mFormatter->FormatRecord(record);
            const size_t kMaxTitleLen = 240;
            const size_t kMaxSuffixLen = 16; // = strlen(" [DebugRelease]") + 1
            char16_t buf[kMaxTitleLen + kMaxSuffixLen];

            const char8_t* const pEnd    = EA::StdC::Strchr(pOutput, '\n');
            const size_t copyLen = pEnd ? pEnd - pOutput : EA::StdC::Strlen(pOutput);
            const int len = EA::StdC::Strlcpy(buf, pOutput, kMaxTitleLen, copyLen);

            #ifdef EA_CHAR16
                // To consider: We may or may not want to force the following text onto the title bar.
                #if defined(EA_DEBUG)
                    EA::StdC::Strcpy(buf + len, EA_CHAR16(" [Debug]"));
                #elif defined(EA_DEBUGRELEASE)
                    EA::StdC::Strcpy(buf + len, EA_CHAR16(" [DebugRelease]"));
                #elif defined(EA_RELEASE)
                    EA::StdC::Strcpy(buf + len, EA_CHAR16(" [Release]"));
                #else
                    (void)len;
                #endif
            #else
                (void)len;
            #endif

            #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                HWND appHwnd(GetMainHWND());
                SetWindowTextW(appHwnd, buf);
            #else
                // Need to implement for the given platform.
            #endif

            return kAlertResultNone;
        }

    }   // namespace Trace
}   // namespace EA











