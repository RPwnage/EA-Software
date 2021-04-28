/******************************************************************************/
/*!
    ReportFileWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/reportfilewriter.h"
#include <cstdio>

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ReportFileWriter::ReportFileWriter

            \brief      ReportFileWriter Constructor

            \param       filePath - The full path (with filename) to save the crash report.
        */
        /******************************************************************************/
        ReportFileWriter::ReportFileWriter(const char* filePath)
        : mFile(NULL)
        {
            mWriteSize = 0;
            mFile = fopen(filePath, "w+");
        }

        /******************************************************************************/
        /*! ReportFileWriter::~ReportFileWriter

            \brief      ReportFileWriter Destructor

            \param       filePath - The full path (with filename) to save the crash report.
        */
        /******************************************************************************/
        ReportFileWriter::~ReportFileWriter()
        {
            if (mFile)
            {
                fclose(mFile);
                mFile = NULL;
            }
        }

        /******************************************************************************/
        /*! ReportFileWriter::Write

            \brief      Write the specified data to the file on disk.

            \param       data - The contents of the file.
            \param       size - The size of the contents to be written.
            \return      true if the write was successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportFileWriter::Write(const void* data, int size)
        {
            bool success = false;

            if (mFile)
            {
                size_t numBytesWritten = fwrite(data, sizeof(char), size, mFile);
                (void) numBytesWritten;
            }

            return success;
        }
    }
}
