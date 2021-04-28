///////////////////////////////////////////////////////////////////////////////
// ExceptionHandler.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
//
// We implement a system here which is based on EACallstack/ExceptionHandler/BugSentry package. 
// EACallstack+ExceptionHandler provide us the callstack information to go along 
// with crash(Exception raised) and BugSentry reports it to a central database.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGIN_EXCEPTIONHANDLER_H
#define ORIGIN_EXCEPTIONHANDLER_H

#include <ExceptionHandler/ExceptionHandler.h>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            class ORIGIN_PLUGIN_API ExceptionHandler : public EA::Debug::ExceptionHandler
            {
            public:
                ExceptionHandler(bool minidumpEnabled);

                virtual size_t GetReportFilePath(char* buffer, size_t capacity, ReportTypeMask reportType)
                {
                    return EA::Debug::ExceptionHandler::GetReportFilePath(buffer, capacity, reportType);
                }

                void SetBaseName(const char* buffer)
                {
                    strcpy(mBaseReportName, buffer);
                }
            };
        
            // This client is notified by the ExceptionHandler package for the ExceptionHandler events.
            class ORIGIN_PLUGIN_API ExceptionHandlerClient : public EA::Debug::ExceptionHandlerClient
            {
            public:
                ExceptionHandlerClient();

                // We can report to BugSentry with relevant information in this callback.
                void Notify(EA::Debug::ExceptionHandler* pHandler, EA::Debug::ExceptionHandler::EventId id);
            };
        }
    }
}

#endif // Header include guard
