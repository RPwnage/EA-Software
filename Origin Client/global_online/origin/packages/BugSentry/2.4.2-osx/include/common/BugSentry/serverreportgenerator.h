/******************************************************************************/
/*! 
    ServerReportGenerator
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_SERVERREPORTGENERATOR_H
#define BUGSENTRY_SERVERREPORTGENERATOR_H

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
        class ServerReportGenerator : public ReportGenerator
        {
        public:
            ServerReportGenerator() : mContextDataPosition(NULL) {}
            virtual ~ServerReportGenerator() {}

            virtual bool GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config);
            char* GetContextDataPosition() const { return mContextDataPosition; }

        private:
            bool WriteCategoryId() const;
            bool WriteServerType() const;
            bool WriteServerName() const;
            bool WriteServerError() const;
            bool WriteContextData();

            char* mContextDataPosition;
        };
    }
}

/******************************************************************************/

#endif  //  BUGSENTRY_SERVERREPORTGENERATOR_H

