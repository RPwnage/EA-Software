/******************************************************************************/
/*!
    ReportUploader

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_REPORT_UPLOADER_H
#define BUGSENTRY_REPORT_UPLOADER_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/bugsentryconfig.h"

/*** Forward Declarations *****************************************************/
namespace EA
{
    namespace BugSentry
    {
        class Reporter;
    }
}

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class ReportUploader
        {
        public:
            ReportUploader();
            virtual ~ReportUploader();

            static const int MAX_SERVER_POST_SIZE = 512 * 1024; // Includes any game-supplied data buffers
            static const int MAX_REPORT_SIZE = 64 * 1024;       // Excludes any game-supplied data buffers

            virtual void UploadReport(const Reporter &reporter) = 0;
            virtual bool DonePosting() const = 0;
            virtual bool SendNextChunk() = 0;

        protected:
            static const char *BUG_SENTRY_DEV_URL;
            static const char *BUG_SENTRY_TEST_URL;
            static const char *BUG_SENTRY_PROD_URL;

#if defined(EA_PLATFORM_XENON)
            static const char *BUG_SENTRY_TEST_UNSECURE_URL;
#endif

            static const int PROTO_HTTP_TIMEOUT = 25*1000;
            static const int PROTO_HTTP_SPAMLEVEL = 2;
            static const int URL_LENGTH = 64;

            void FormatReport(const Reporter &reporter);
            const char* GetReportingBaseUrl(EA::BugSentry::BugSentryMode mode) const;

            char mPostDataBuffer[MAX_REPORT_SIZE];
            const char *mPostData;
            int mPostSize;
            char mUrl[URL_LENGTH];

        private:
            bool mCallNetConnShutdown;
        };
    }
} // namespace EA

#endif // BUGSENTRY_REPORT_UPLOADER_H
