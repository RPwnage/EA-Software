/******************************************************************************/
/*! 
    DesyncReportUploader
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/09/2009 (gsharma)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_DESYNCREPORTUPLOADER_H
#define BUGSENTRY_DESYNCREPORTUPLOADER_H

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
        class DesyncReportUploader : public ReportUploader
        {
        public:
            DesyncReportUploader() : mHttp(NULL), mPosted(0), mDesyncInfoGameBuffer(NULL), mDesyncInfoPosition(NULL), mDesyncInfoPostSize(0), mDesyncInfoPosted(0) {}
            virtual ~DesyncReportUploader();

            virtual void UploadReport(const Reporter &reporter);
            virtual bool DonePosting() const;
            virtual bool SendNextChunk();

        private:
            static const char *BUG_SENTRY_WEBSERVICE;

            void PostReport();

            ProtoHttpRefT* mHttp;
            int mPosted;

            const char* mDesyncInfoGameBuffer;
            char* mDesyncInfoPosition;
            int mDesyncInfoPostSize;
            int mDesyncInfoPosted;
        };
    }
}

/******************************************************************************/

#endif  //  BUGSENTRY_DESYNCREPORTUPLOADER_H

