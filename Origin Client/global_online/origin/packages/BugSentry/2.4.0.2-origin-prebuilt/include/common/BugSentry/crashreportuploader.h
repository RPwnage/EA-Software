/******************************************************************************/
/*!
    CrashReportUploader

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_CRASHREPORT_UPLOADER_H
#define BUGSENTRY_CRASHREPORT_UPLOADER_H

/*** Includes *****************************************************************/
#include "BugSentry/reportuploader.h"

/*** Forward Declarations *****************************************************/
namespace EA
{
    namespace BugSentry
    {
        class Reporter;
    }
}

struct ProtoHttpRefT;

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class CrashReportUploader : public ReportUploader
        {
        public:
            CrashReportUploader() : mHttp(NULL), mContextDataGameBuffer(NULL), mContextDataPosition(NULL), mContextDataPostSize(0) {}
            virtual ~CrashReportUploader() {}

            virtual void UploadReport(const Reporter &reporter);
            virtual bool DonePosting() const { return true; }   // all uploading occurs in UploadReport(), so it is "done posting" after that function returns
            virtual bool SendNextChunk() { return false; }      // all uploading occurs in UploadReport(), so there are no more chunks to send after that function returns

        private:
            static const char *BUG_SENTRY_WEBSERVICE;

            void PostReport();
            void HttpIdle();

            ProtoHttpRefT* mHttp;
            const char* mContextDataGameBuffer;
            char* mContextDataPosition;
            int mContextDataPostSize;
        };
    }
} // namespace EA

#endif // BUGSENTRY_CRASHREPORT_UPLOADER_H
