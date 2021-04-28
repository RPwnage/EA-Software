///////////////////////////////////////////////////////////////////////////////
// EALogExtra.h
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Created by Alex Liberman
///////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EALOGEXTRA_H
#define EATRACE_EALOGEXTRA_H


#include <EATrace/EALog_imp.h>


namespace EA {

    namespace Trace {

        /// LogReporterTitleBar implements a LogReporter that traces-out 
        /// to the title bar of a windowed application 
        class LogReporterTitleBar : public EA::Trace::LogReporter 
        {
        public:
            LogReporterTitleBar(const char8_t* pName);

            virtual bool IsFiltered(const EA::Trace::TraceHelper& helper);
            virtual bool IsFiltered(const EA::Trace::LogRecord& record);
            virtual EA::Trace::tAlertResult Report(const EA::Trace::LogRecord& record);
        };

    }   // namespace Trace

}   // namespace EA


#endif  // EATRACE_EALOGEXTRA_H









