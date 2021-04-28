/******************************************************************************/
/*!
    SessionReportGenerator

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/23/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_SESSION_REPORT_GENERATOR_H
#define BUGSENTRY_SESSION_REPORT_GENERATOR_H

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
        class SessionReportGenerator : public ReportGenerator
        {
        public:
            virtual ~SessionReportGenerator() {}

            virtual bool GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config);
            virtual bool DonePosting() const { return true; }   // all uploading occurs in UploadReport(), so it is "done posting" after that function returns
            virtual bool SendNextChunk() { return false; }      // all uploading occurs in UploadReport(), so there are no more chunks to send after that function returns

        private:
            virtual bool WriteSessionStart() const;
            virtual bool WriteSessionEnd() const;
            virtual bool WriteSessionType() const;
            virtual bool WriteSessionServerType() const;
            virtual bool WriteSessionServerName() const;
        };
    }
}

#endif //BUGSENTRY_SESSION_REPORT_GENERATOR_H
