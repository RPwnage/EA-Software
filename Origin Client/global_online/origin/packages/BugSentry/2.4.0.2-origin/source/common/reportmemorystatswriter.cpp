/******************************************************************************/
/*!
    ReportMemoryStatsWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/reportmemorystatswriter.h"

namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ReportMemoryStatsWriter::ReportMemoryStatsWriter

            \brief      ReportMemoryStatsWriter Constructor

            \param       - none.
        */
        /******************************************************************************/
        ReportMemoryStatsWriter::ReportMemoryStatsWriter()
        {
            ResetStats();
        }

        /******************************************************************************/
        /*! ReportMemoryStatsWriter::Write

            \brief      Track the specified size for this write.

            \param       - data: Unused.
            \param       - size: The size of the contents to be written.
            \return      - true if the size was tracked, false if there was an error.
        */
        /******************************************************************************/
        bool ReportMemoryStatsWriter::Write(const void* /*data*/, int size)
        {
            mWriteSize+= size;
            return true;
        }
    }
}
