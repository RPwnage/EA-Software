/******************************************************************************/
/*!
    ReportFileWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_REPORT_FILEWRITER_H
#define BUGSENTRY_REPORT_FILEWRITER_H

/*** Includes *****************************************************************/
#include "BugSentry/ireportwriter.h"

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
            FILE* mFile;
            int mWriteSize;
        };
    }
}

#endif //BUGSENTRY_REPORT_FILEWRITER_H
