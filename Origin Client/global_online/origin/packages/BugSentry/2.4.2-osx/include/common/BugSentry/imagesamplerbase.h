/******************************************************************************/
/*!
    ImageSamplerBase

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_IMAGESAMPLER_BASE_H
#define BUGSENTRY_IMAGESAMPLER_BASE_H

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
        struct ImagePixel
        {
            unsigned char mRed;
            unsigned char mGreen;
            unsigned char mBlue;
        };

        class ImageSamplerBase
        {
        public:
            ImageSamplerBase(const ImageInfo* screenshotInfo);
            virtual ~ImageSamplerBase() {};

            ImagePixel GetPixel(int x, int y);
            int GetWidth() { return mWidth; }
            int GetHeight() { return mHeight; }

        protected:
            static const int MAX_PIXEL_BYTE_SIZE = 16; // 4 float components

            virtual ImagePixel GetPixelPlatformSpecificInternal(int x, int y) = 0;

            ImagePixel GetPixelLinearInternal(int x, int y);
            ImagePixel GetPixelDataFromFormat(const unsigned char* data);
            int ClampValue(int value, int minValue, int maxValue);

            const ImageInfo* mImageInfo;
            int mDownSamplingScale;
            int mWidth;
            int mHeight;
        };
    }
}

#endif // BUGSENTRY_IMAGESAMPLER_BASE_H
