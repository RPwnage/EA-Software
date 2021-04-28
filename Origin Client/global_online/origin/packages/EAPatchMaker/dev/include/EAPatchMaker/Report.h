/////////////////////////////////////////////////////////////////////////////
// EAPatchMaker/Report.h
//
// Copyright (c) Electronic Arts Inc. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHMAKER_REPORT_H
#define EAPATCHMAKER_REPORT_H


#include <EAPatchMaker/Config.h>
#include <stdarg.h>


namespace EA
{
    namespace Patch
    {
        /// Report
        /// Provides a way to call Report with sprintf-style arguments. 
        /// This function will call the Report function after formatting the output.
        /// This function acts just like printf, except that the output goes to the 
        /// given report function.
        ///
        /// The user needs to supply a newline if the user wants newlines, as the report
        /// function will not append one. The user may supply multiple newlines if desired. 
        /// This is a low level function which user code can use to directly write 
        /// information to the debug output. This function is also used by the higher
        /// level functionality here to write output.
        ///
        /// This function is the equivalent of ReportVerbosity(0, pFormat, ...). The effect
        /// of this function is that it always prints its output. It should be used only
        /// for cases where you always want output.
        ///
        /// Example usage:
        ///     Report("Time passed: %d\n", timeDelta);
        ///
        void Report(const char8_t* pFormat, ...);


        /// ReportVerbosity
        /// Same as Report, but is silent unless GetVerbosity() is >= the value specified as minVerbosity.
        /// Typically to do a non-error trace print, you would specify a minVerbosity of 1.
        /// If you are writing an error output, you can specify minVerbosity or 0, which is the same
        /// as calling Report().
        ///
        /// Example usage:
        ///     ReportVerbosity(1, "Time passed: %d\n", timeDelta);
        ///
        void ReportVerbosity(unsigned minVerbosity, const char8_t* pFormat, ...);
        void ReportVerbosityV(unsigned minVerbosity, const char8_t* pFormat, va_list args);


        /// ReportVerbosityCondition
        /// Calls ReportVerbosity if condition is true. Returns 1 if the condition is true, else 0.
        int ReportVerbosityCondition(bool condition, unsigned minVerbosity, const char* pFormat, ...);


        ///////////////////////////////////////////////////////////////////////
        /// GetReportVerbosity / SetReportVerbosity
        ///
        /// Default verbosity is 1 (kReportVerbosityDefault), which means that all 
        /// Report calls will be printed. If you set the verbosity to 1 then Report 
        /// will not be printed, but ReportVerbosity(1, ...) will.
        ///
        const unsigned kReportVerbosityDefault = 1;     /// The default return value for GetVerbosity.
        const unsigned kReportVerbosityError   = 0;     /// ReportVerbosity(kReportVerbosityError, ...) always displays.
        const unsigned kReportVerbosityTrace   = 1;     /// GetVerbosity must return >= 1 for ReportVerbosity(kReportVerbosityTrace, ...) to display.
        const unsigned kReportVerbosityRemark  = 2;     /// GetVerbosity must return >= 2 for ReportVerbosity(kReportVerbosityRemark, ...) to display.

        unsigned GetReportVerbosity();
        void     SetReportVerbosity(unsigned verbosity);


    } // namespace Patch

} // namespace EA


#endif // Header include guard




