/******************************************************************************/
/*!
    ReportMemoryWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/reportmemorywriter.h"
#include "EAStdC/EAMemory.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ReportMemoryWriter::ReportMemoryWriter

            \brief      ReportMemoryWriter Constructor

            \param       - buffer: The buffer to use for writing the report.
            \param       - size: The size of the buffer.
        */
        /******************************************************************************/
        ReportMemoryWriter::ReportMemoryWriter(void* buffer, int size)
        {
            mBuffer = buffer;
            mMaxSize = size;
            Reset();
        }

        /******************************************************************************/
        /*! ReportMemoryWriter::Write

            \brief      Write the specified data to the buffer.

            \param       - data: The contents of the file.
            \param       - size: The size of the contents to be written.
            \return      - true if the write was successful, false if there was an error.
        */
        /******************************************************************************/
        bool ReportMemoryWriter::Write(const void* data, int size)
        {
            // check if it will fit
            int newRunningSize = mRunningSize + size;
            bool writeSuccess = (newRunningSize <= mMaxSize);

            if (writeSuccess)
            {
                void* dest = &static_cast<char*>(mBuffer)[mRunningSize];

                EA::StdC::Memcpy(dest, data, static_cast<size_t>(size));
                mRunningSize = newRunningSize;
            }

            return writeSuccess;
        }
    }
}
