/******************************************************************************/
/*!
    ReportMemoryWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_REPORT_MEMORYWRITER_H
#define BUGSENTRY_REPORT_MEMORYWRITER_H

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
        class ReportMemoryWriter : public IReportWriter
        {
        public:
            ReportMemoryWriter(void* buffer, int size);
            virtual ~ReportMemoryWriter() {};

            virtual bool Write(const void* data, int size);
            virtual char* GetCurrentPosition() const { return &static_cast<char*>(mBuffer)[mRunningSize]; }

            int GetTotalWriteSize() const { return mRunningSize; }

        private:
            void Reset() { mRunningSize = 0; }

            void *mBuffer;
            int mMaxSize;
            int mRunningSize;
        };
    }
}

#endif //BUGSENTRY_REPORT_MEMORYWRITER_H
