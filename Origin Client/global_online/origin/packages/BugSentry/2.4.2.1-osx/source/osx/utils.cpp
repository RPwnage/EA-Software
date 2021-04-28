/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/utils.h"
#include <cstdio>

#include "EACallstack/EACallstack.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
	  /******************************************************************************/
        /*! Utils::Utils

            \brief      Constructor - load kernel32.dll and get the address of the function RtlCaptureStackBackTrace.

            \param       - none.
            \return      - none.
        */
        /******************************************************************************/
        Utils::Utils()
        {
            EA::Callstack::InitCallstack();
        }

        Utils::~Utils()
        {
            EA::Callstack::ShutdownCallstack();
        }

	  /******************************************************************************/
        /*! Utils::GetCallstack

            \brief      Get the callstack information (the addresses).

            \param       exceptionPointers - The platform-specific exception information.
            \param       addresses - An array to store the addresses into.
            \param       addressesMax - The max number of entries available in the addresses array.
            \param       addressesFilledCount - The number of entries used when collecting the callstack (optional).
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetCallstack(void *exceptionPointers, uintptr_t* addresses, int addressesMax, int* addressesFilledCount)
        {
            *addressesFilledCount = EA::Callstack::GetCallstack(reinterpret_cast<void**>(addresses), addressesMax);
            return *addressesFilledCount > 0;
        }
 
    }
}