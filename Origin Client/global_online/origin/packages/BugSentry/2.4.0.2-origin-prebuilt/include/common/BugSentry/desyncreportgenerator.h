/******************************************************************************/
/*! 
    DesyncReportGenerator
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/17/2009 (gsharma)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_DESYNCREPORTGENERATOR_H
#define BUGSENTRY_DESYNCREPORTGENERATOR_H

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
        class DesyncReportGenerator : public ReportGenerator
        {
        public:
            DesyncReportGenerator() : mDesyncDataPosition(NULL) {}
            virtual ~DesyncReportGenerator() {}

            virtual bool GenerateReport(IReportWriter* writeInterface, const BugData* bugData, const BugSentryConfig* config);
            char* GetDesyncInfoPosition() const { return mDesyncDataPosition; }

        private:
            bool WriteCategoryId() const;
            bool WriteDesyncId() const;
            bool WriteDesyncData();

            char* mDesyncDataPosition;
        };
    }
}

/******************************************************************************/

#endif  //  BUGSENTRY_DESYNCREPORTGENERATOR_H

