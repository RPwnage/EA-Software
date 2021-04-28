/******************************************************************************/
/*!
    ReportFileWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/reportfilewriter.h"

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
        ReportFileWriter::ReportFileWriter(const char* filePath) : mFile(NULL)
        {
            // TODO (mbaylis) : handle case of no HD.
            mWriteSize = 0;
            mFile = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }

        /******************************************************************************/
        /*! ReportFileWriter::~ReportFileWriter

            \brief      ReportFileWriter Destructor

            \param       filePath - The full path (with filename) to save the crash report.
        */
        /******************************************************************************/
        ReportFileWriter::~ReportFileWriter()
        {
            if (mFile && (mFile != INVALID_HANDLE_VALUE))
            {
                CloseHandle(mFile);
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

            if (mFile && (size > 0))
            {
                DWORD bytesWritten = 0;
                WriteFile(mFile, data, static_cast<DWORD>(size), &bytesWritten, NULL);

                // check if we actually wrote anything
                success = (static_cast<int>(bytesWritten) == size) ? true : false;
                mWriteSize += success ? size : 0;
            }

            return success;
        }
    }
}
