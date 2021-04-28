/******************************************************************************/
/*! 
    ReportGenerator
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/02/2009 (gsharma)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_REPORTGENERATOR_H
#define BUGSENTRY_REPORTGENERATOR_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif

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
        class ReportGenerator
        {
        public:
            ReportGenerator() : mWriteInterface(NULL), mBugData(NULL) {}
            virtual ~ReportGenerator() {}

            virtual bool GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config) = 0;

        protected:
            static const char* REPORT_VERSION;

            bool WriteHeader() const;
            bool WriteReportStart() const;
            bool WriteVersion() const;
            bool WriteSku() const;
            bool WriteCreationTime() const;
            bool WriteBuildSignature() const;
            bool WriteReportType(const char* reportType) const;
            bool WriteScreenshot(const BugSentryConfig* config) const;
            bool WriteReportEnd() const;
            bool WriteXmlTagStart(const char* str) const;
            bool WriteXmlTagEnd(const char* str) const;
            bool WriteString(const char* str) const;
            bool WriteSessionId() const;
            bool WriteSystemConfig() const;

            IReportWriter* mWriteInterface;
            const BugData* mBugData;
        };
    }
}

/******************************************************************************/

#endif  //  BUGSENTRY_REPORTGENERATOR_H

