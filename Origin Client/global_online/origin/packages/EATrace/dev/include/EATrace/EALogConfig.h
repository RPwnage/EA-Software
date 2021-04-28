/////////////////////////////////////////////////////////////////////////////
// EATrace/EALogConfig.h
//
// Copyright (c) 2005 Electronic Arts Inc.
// Created by Jon Parise
// Maintained by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EALOGCONFIG_H
#define EATRACE_EALOGCONFIG_H


#include <EABase/eabase.h>


namespace EA
{
    namespace Allocator
    {
        class ICoreAllocator;
    }

    namespace IO
    {
        class IniFile;
    }

    namespace Trace
    {
        /* Sample config.ini file:

        ; Global Lord of the Rings Logging Configuration File
        ;
        ; $Revision: #4 $
        ; $DateTime: 2005/10/27 10:15:08 $
        ;
        ; This file contains the global logging configuration.  Each section describes
        ; a single log reporter.  The following values may be specific for each:
        ;
        ;    Type:     Report type: debugger, file, console, dialog
        ;    Level:    The default minimum log level to which this reporter will respond.
        ;    Filter:   Comma-separated list of group:level pairs which build on the
        ;              Level value with more specific filters.
        ;    Formatter:The name of the log record formatter.  Default is 'detailed'.
        ;    Filename: The filename to which the log will be written ('file' type only).
        ;
        ; Acceptable level names:
        ;    all, none, min, debug, info, warn, error, fatal, max
        ;
        ; Acceptable formatter names:
        ;    prefixed, simple, fancy
        
        [AppDebugger]
        Level     = all
        Filter    = RenderWare:none
        Formatter = prefixed
        
        [ConsoleWindow]
        Type      = console
        Level     = all
        Filter    = RenderWare:none
        Formatter = prefixed
        
        [Everything]
        Type      = file
        Filename  = LOTR.log
        Level     = all
        Formatter = detailed

        */

        /// Configure the logging system based on a configuration file.
        ///
        /// \param  config      Reference to the configuration file.
        /// \param  pReporter   Optionally, the name of the specific reporter
        ///                     (described in the configuration file) that
        ///                     should be configured.  If \a pReporter is NULL,
        ///                     all reporters will be configured.
        /// \param  pAllocator  Optionally, a pointer to the ICoreAllocator
        ///                     instance from which to allocate the log objects.
        ///
        bool Configure(IO::IniFile& config, const wchar_t* pReporter = NULL, Allocator::ICoreAllocator* pAllocator = NULL);

    } // namespace Trace

} // namespace EA

#endif // Header include guard

