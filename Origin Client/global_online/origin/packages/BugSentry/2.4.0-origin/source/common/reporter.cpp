/******************************************************************************/
/*!
    Reporter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/reporter.h"
#include "BugSentry/reportfilewriter.h"
#include "BugSentry/reportmemorywriter.h"
#include "BugSentry/reportmemorystatswriter.h"
#include "BugSentry/reportgenerator.h"
#include "BugSentry/reportuploader.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! Reporter::UploadReport

            \brief      Reports the crash back to EA, using GOS' BugSentry

            \param       - none.
            \return      - none.
        */
        /******************************************************************************/
        void Reporter::UploadReport()
        {
            if (mBugData && mReportUploader)
            {
                mReportUploader->UploadReport(*this);
            }
        }

        /******************************************************************************/
        /*! Reporter::CreateFileReport

            \brief      Generates a crash report to disk, specified by the filePath

            \param       - filePath: The full path (with filename) to save the crash report.
            \return      - true if the report was written to disk, false if there was an error.
        */
        /******************************************************************************/
        bool Reporter::CreateFileReport(const char* filePath) const
        {
            bool success = false;

            if (mBugData && mReportGenerator && filePath)
            {
                ReportFileWriter writer(filePath);
                success = mReportGenerator->GenerateReport(&writer, mBugData, mConfig);
            }

            return success;
        }

        /******************************************************************************/
        /*! Reporter::CreateInMemoryReport

            \brief      Generates a crash report in the specified memory buffer.

            \param       - buffer: Pointer to a buffer to save the crash report to.
            \param       - bufferByteSize: Size of the buffer in bytes.
            \param       - bufferBytesUsed: Number of bytes used in the buffer for the report (optional).
            \return      - true if the report was generated in the buffer, false if there was an error.
        */
        /******************************************************************************/
        bool Reporter::CreateInMemoryReport(void* buffer, int bufferByteSize, int* bufferBytesUsed) const
        {
            bool success = false;

            if (mBugData && mReportGenerator && buffer && (bufferByteSize > 0))
            {
                ReportMemoryWriter writer(buffer, bufferByteSize);
                success = mReportGenerator->GenerateReport(&writer, mBugData, mConfig);

                // in case the caller was interested on knowing how much data we actually wrote
                if (bufferBytesUsed)
                {
                    *bufferBytesUsed = writer.GetTotalWriteSize();
                }
            }

            return success;
        }

        /******************************************************************************/
        /*! Reporter::CalculateReportByteSize

            \brief      Calculates the formatted report's size in bytes, useful for
                        allocating the correct sized buffer for in-memory reports.

            \param       - none.
            \return      - number of bytes the formatted crash report will take.
        */
        /******************************************************************************/
        int Reporter::CalculateReportByteSize() const
        {
            int size = 0;

            if (mBugData && mReportGenerator)
            {
                ReportMemoryStatsWriter writer;

                if (mReportGenerator->GenerateReport(&writer, mBugData, mConfig))
                {
                    size = writer.GetTotalWriteSize();
                }
            }

            return size;
        }
    }
}