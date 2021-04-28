/******************************************************************************/
/*!
    Reporter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_REPORTER_H
#define BUGSENTRY_REPORTER_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif

/*** Forward Declarations *****************************************************/
namespace EA
{
    namespace BugSentry
    {
        class BugData;
        class ReportGenerator;
        class ReportUploader;
        class BugSentryConfig;
    }
}

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class Reporter
        {
        public:
            Reporter() : mBugData(NULL), mReportGenerator(NULL), mReportUploader(NULL), mConfig(NULL) {}
            virtual ~Reporter() {}

            void UploadReport();
            bool CreateFileReport(const char* filePath) const;
            bool CreateInMemoryReport(void* buffer, int bufferByteSize, int* bufferBytesUsed) const;
            int CalculateReportByteSize() const;

            BugData* mBugData;
            ReportGenerator* mReportGenerator;
            ReportUploader* mReportUploader;
            BugSentryConfig* mConfig;
        };
    }
}

#endif // BUGSENTRY_REPORTER_H
