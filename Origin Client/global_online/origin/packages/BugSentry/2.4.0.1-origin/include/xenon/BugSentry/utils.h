/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_UTILS_H
#define BUGSENTRY_UTILS_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif

#include "BugSentry/imageinfo.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class Utils
        {
        public:
            Utils() {};
            ~Utils() {};

            static bool StartFrameBufferAccess(ImageInfo* imageInfo, void* platformGraphicsDevice, void* platformGraphicsFrameBuffer, ImageDownSample imageDownSample);
            static void EndFrameBufferAccess(ImageInfo* imageInfo, void* platformGraphicsDevice, void* platformGraphicsFrameBuffer);
            static bool GetCallstack(void* exceptionPointers, unsigned int* addresses, int addressesMax, int* addressesFilledCount);
        };
    }
}

#endif // BUGSENTRY_UTILS_H
