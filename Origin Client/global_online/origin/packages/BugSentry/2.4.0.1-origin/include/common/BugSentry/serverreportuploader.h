/******************************************************************************/
/*! 
    ServerReportUploader
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_SERVERREPORTUPLOADER_H
#define BUGSENTRY_SERVERREPORTUPLOADER_H

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
        class ServerReportUploader : public ReportUploader
        {
        public:
            ServerReportUploader() : mHttp(NULL), mPosted(0), mContextDataGameBuffer(NULL), mContextDataPosition(NULL), mContextDataPostSize(0), mContextDataPosted(0) {}
            virtual ~ServerReportUploader();

            virtual void UploadReport(const Reporter &reporter);

            bool DonePosting() const;
            bool SendNextChunk();

        private:
            static const char *BUG_SENTRY_WEBSERVICE;

            void PostReport();

            ProtoHttpRefT* mHttp;
            int mPosted;

            const char* mContextDataGameBuffer;
            char* mContextDataPosition;
            int mContextDataPostSize;
            int mContextDataPosted;
        };
    }
}

/******************************************************************************/

#endif  //  BUGSENTRY_SERVERREPORTUPLOADER_H

