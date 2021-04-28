/******************************************************************************/
/*!
    PlatformImageInfo

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_PLATFORMIMAGEINFO_H
#define BUGSENTRY_PLATFORMIMAGEINFO_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class PlatformImageInfo
        {
        public:
            PlatformImageInfo() : mWidthInBlocks(0), mHeightInBlocks(0), mBytesPerBlock(0), mIsTiled(false) {};

            unsigned int mWidthInBlocks;
            unsigned int mHeightInBlocks;
            unsigned int mBytesPerBlock;
            bool mIsTiled;
        };
    }
}

#endif // BUGSENTRY_PLATFORMIMAGEINFO_H
