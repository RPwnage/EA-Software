/******************************************************************************/
/*!
    ReportMemoryStatsWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_REPORT_MEMORYSTATSWRITER_H
#define BUGSENTRY_REPORT_MEMORYSTATSWRITER_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/ireportwriter.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class ReportMemoryStatsWriter : public IReportWriter
        {
        public:
            ReportMemoryStatsWriter();
            virtual ~ReportMemoryStatsWriter() {};

            virtual bool Write(const void *data, int size);
            virtual char* GetCurrentPosition() const { return NULL; }

            int GetTotalWriteSize() const { return mWriteSize; }

        private:
            void ResetStats() { mWriteSize = 0; }

            int mWriteSize;
        };
    }
}

#endif //BUGSENTRY_REPORT_MEMORYSTATSWRITER_H
