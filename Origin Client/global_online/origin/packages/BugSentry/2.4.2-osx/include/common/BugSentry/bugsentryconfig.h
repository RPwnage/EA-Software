/******************************************************************************/
/*!
    BugSentryConfig

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     06/23/2009 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_BUGSENTRYCONFIG_H
#define BUGSENTRY_BUGSENTRYCONFIG_H

/*** Includes *****************************************************************/
#include "BugSentry/imageinfo.h"

/*** Forward Declarations *****************************************************/
namespace EA
{
    namespace Allocator
    {
        class ICoreAllocator;
    }
}

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        enum BugSentryMode
        {
            BUGSENTRYMODE_DEV, // DO NOT USE - This is for BugSentry development purposes only.
            BUGSENTRYMODE_TEST, // Use when you don't want reports to be sent to the official server
            BUGSENTRYMODE_PROD  // Use this mode when you want reports sent to the official server - this should be used in the ship version!
        };

        class BugSentryConfig
        {
        public:
            BugSentryConfig();

            EA::Allocator::ICoreAllocator *mAllocator;
            EA::BugSentry::BugSentryMode mMode;
            bool mEnableScreenshot;
            EA::BugSentry::ImageDownSample mImageDownSample;
            bool mWriteToFile;
            const char* mFilePath;
            const char* mSku;
            const char* mBuildSignature;

#if defined(EA_PLATFORM_WINDOWS)
            bool mUseHeuristicStackWalk;
#endif

#if defined(EA_PLATFORM_REVOLUTION)
            unsigned int mRsoBaseAddress;
#endif
        };

        /******************************************************************************/
        /*! BugSentryConfig::BugSentryConfig

            \brief      Constructor

        */
        /******************************************************************************/
        inline BugSentryConfig::BugSentryConfig() :
             mAllocator(NULL)
            ,mMode(BUGSENTRYMODE_PROD)
            ,mEnableScreenshot(true)
            ,mImageDownSample(IMAGE_DOWNSAMPLE_NONE)
            ,mWriteToFile(false)
            ,mFilePath(NULL)
            ,mSku(NULL)
            ,mBuildSignature(NULL)
#if defined(EA_PLATFORM_WINDOWS)
            ,mUseHeuristicStackWalk(false)
#endif
#if defined(EA_PLATFORM_REVOLUTION)
            ,mRsoBaseAddress(NULL)
#endif
        {
        }
    }
}

#endif // BUGSENTRY_BUGSENTRYCONFIG_H
