/******************************************************************************/
/*!
    CrashReportGenerator

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_CRASH_REPORT_GENERATOR_H
#define BUGSENTRY_CRASH_REPORT_GENERATOR_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/reportgenerator.h"

/*** Forward Declarations *****************************************************/
namespace EA
{
    namespace BugSentry
    {
        class BugSentryConfig;
        class BugData;
        class IReportWriter;
    }
}

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class CrashReportGenerator : public ReportGenerator
        {
        public:
            CrashReportGenerator() : mContextDataPosition(NULL) {}
            virtual ~CrashReportGenerator() {}

            virtual bool GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config);
            char* GetContextDataPosition() const { return mContextDataPosition; }

        private:
            bool WriteCategoryId() const;
            bool WriteCallstack() const;
            bool WriteThreads() const;
            bool WriteContextData();
            bool WriteMemDump();

            char* mContextDataPosition;
        };
    }
}

#endif //BUGSENTRY_CRASH_REPORT_GENERATOR_H
