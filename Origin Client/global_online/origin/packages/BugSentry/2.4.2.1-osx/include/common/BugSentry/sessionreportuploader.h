/******************************************************************************/
/*!
    SessionReportUploader

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/23/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_SESSIONREPORT_UPLOADER_H
#define BUGSENTRY_SESSIONREPORT_UPLOADER_H

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
        class SessionReportUploader : public ReportUploader
        {
        public:
            SessionReportUploader() : mHttp(NULL) {}
            virtual ~SessionReportUploader() {}

            virtual void UploadReport(const Reporter &reporter);
            virtual bool DonePosting() const { return true; }   // all uploading occurs in UploadReport(), so it is "done posting" after that function returns
            virtual bool SendNextChunk() { return false; }      // all uploading occurs in UploadReport(), so there are no more chunks to send after that function returns

        private:
            static const char *BUG_SENTRY_WEBSERVICE;

            void PostReport();
            void HttpIdle();

            ProtoHttpRefT* mHttp;
        };
    }
} // namespace EA

#endif // BUGSENTRY_SESSIONREPORT_UPLOADER_H
