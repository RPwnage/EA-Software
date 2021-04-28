/******************************************************************************/
/*!
    ReportFileWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/24/2009 (lvillegas/mbaylis)
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
        ReportFileWriter::ReportFileWriter(const char* filePath)
        {
            // Not implemented
        }

        /******************************************************************************/
        /*! ReportFileWriter::~ReportFileWriter

            \brief      ReportFileWriter Destructor

            \param       filePath - The full path (with filename) to save the crash report.
        */
        /******************************************************************************/
        ReportFileWriter::~ReportFileWriter()
        {
            // Not implemented
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
            bool success = true;

            // Not implemented

            return success;
        }
    }
}
