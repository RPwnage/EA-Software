/******************************************************************************/
/*!
    ImageSampler

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/24/2009 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_IMAGESAMPLER_H
#define BUGSENTRY_IMAGESAMPLER_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
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
