/******************************************************************************/
/*!
    IReportWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_IREPORTWRITER_H
#define BUGSENTRY_IREPORTWRITER_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class IReportWriter
        {
        public:
            IReportWriter() {};
            virtual ~IReportWriter() {};

            virtual bool Write(const void* data, int size) = 0;
            virtual char* GetCurrentPosition() const = 0;
        };
    }
}

#endif // BUGSENTRY_IREPORTWRITER_H
