/******************************************************************************/
/*!
    ReportFileWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_REPORT_FILEWRITER_H
#define BUGSENTRY_REPORT_FILEWRITER_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/ireportwriter.h"
#include <xtl.h>

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class ReportFileWriter : public IReportWriter
        {
        public:
            ReportFileWriter(const char* filePath);
            virtual ~ReportFileWriter();

            virtual bool Write(const void* data, int size);
            virtual char* GetCurrentPosition() const { return NULL; }

            int GetTotalWriteSize() const { return mWriteSize; }

        private:
            HANDLE mFile;
            int mWriteSize;
        };
    }
}

#endif //BUGSENTRY_REPORT_FILEWRITER_H
