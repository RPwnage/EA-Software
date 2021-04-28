/******************************************************************************/
/*!
    ImageSampler

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_IMAGESAMPLER_H
#define BUGSENTRY_IMAGESAMPLER_H

/*** Includes *****************************************************************/
#include "BugSentry/imagesamplerbase.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class ImageSampler : public ImageSamplerBase
        {
        public:
            ImageSampler(const ImageInfo* screenshotInfo) : ImageSamplerBase(screenshotInfo) {};
            virtual ~ImageSampler() {};

        private:
            virtual ImagePixel GetPixelPlatformSpecificInternal(int x, int y);
        };
    }
}

#endif // BUGSENTRY_IMAGESAMPLER_H
