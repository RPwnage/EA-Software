///////////////////////////////////////////////////////////////////////////////
// ExceptionSystem.h
//
// Copyright (c) 2010, Electronic Arts Inc. All rights reserved.
//
// We implement a system here which is based on EACallstack/ExceptionHandler/BugSentry package. 
// EACallstack+ExceptionHandler provide us the callstack information to go along 
// with crash(Exception raised) and BugSentry reports it to a central database.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGIN_EXCEPTIONSYSTEM_H
#define ORIGIN_EXCEPTIONSYSTEM_H

#include <EACallstack/EAAddressRep.h>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            class ExceptionHandler;
            class ExceptionHandlerClient;
            class ExceptionReportWriter;

            // ExceptionSystem
            //
            // Our Exception System is capable of catching an exception and reporting it to a 
            // central database(BugSentry). It can be more modular (for example, we can 
            // modify the symbol load to take a file path instead but we don't need it for now.
            class ORIGIN_PLUGIN_API ExceptionSystem
            {
            public:
                ExceptionSystem();
               ~ExceptionSystem();

                void start();
                
                bool enabled() const;
                void setEnabled(bool enabled);

            private:
                void SetupEACallstack();
                void SetupExceptionHandler();

                ExceptionHandler                  *mExceptionHandler;
                ExceptionHandlerClient            *mExceptionHandlerClient;
                ExceptionReportWriter             *mExceptionReportWriter;
                EA::Callstack::AddressRepCache     mAddressRepCache;
            };
        }
    }
}

#endif // Header include guard
