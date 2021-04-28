/******************************************************************************/
/*!
    ImageInfo

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_IMAGEINFO_H
#define BUGSENTRY_IMAGEINFO_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/platformimageinfo.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        enum ImageFormat
        {
            IMAGE_FORMAT_X8R8G8B8,
            IMAGE_FORMAT_LE_X8R8G8B8, // little endian
            IMAGE_FORMAT_COUNT
        };

        enum ImageDownSample
        {
            IMAGE_DOWNSAMPLE_NONE,
            IMAGE_DOWNSAMPLE_2X,
            IMAGE_DOWNSAMPLE_4X,
            IMAGE_DOWNSAMPLE_COUNT
        };

        class ImageInfo
        {
        public:
            ImageInfo();

            int mWidth;
            int mHeight;
            int mPitch;
            int mBytesPerPixel;
            ImageFormat mImageFormat;
            ImageDownSample mImageDownSample;
            void* mImageData;
            void* mUserData;
            PlatformImageInfo mPlatformSpecific;
        };

        /******************************************************************************/
        /*! ImageInfo::ImageInfo

            \brief      Constructor

        */
        /******************************************************************************/
        inline ImageInfo::ImageInfo() :
            mWidth(0),
            mHeight(0),
            mPitch(0),
            mBytesPerPixel(0),
            mImageFormat(IMAGE_FORMAT_X8R8G8B8),
            mImageDownSample(IMAGE_DOWNSAMPLE_NONE),
            mImageData(NULL),
            mUserData(NULL)
        {
        }
    }
}

#endif // BUGSENTRY_IMAGEINFO_H
