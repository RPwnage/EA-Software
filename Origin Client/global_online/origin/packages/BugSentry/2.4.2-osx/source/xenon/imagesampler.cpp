/******************************************************************************/
/*!
    ImageSampler

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/imagesampler.h"
#include <xtl.h>
#include <xgraphics.h> // to deal with tiling

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ImageSampler::GetPixelPlatformSpecificInternal

            \brief      Retrieves the pixel information at the given coordinates for this platform.

            \param       x - The x coordinate to retrieve the pixel information.
            \param       y - The y coordinate to retrieve the pixel information.
            \return      ImagePixel structure containing red, green, & blue values.
        */
        /******************************************************************************/
        ImagePixel ImageSampler::GetPixelPlatformSpecificInternal(int x, int y)
        {
            ImagePixel result = {0,0,0};

            // we need to deal with tiling here, this is very inefficient, but it is the best way to do it
            // without allocating a lot of memory
            if (mImageInfo->mPlatformSpecific.mIsTiled)
            {
                // we have a size for a worst case pixel size
                uint8_t pixelRaw[MAX_PIXEL_BYTE_SIZE] = {0};
                POINT point = {0};
                RECT clientRect;

                clientRect.left = x;
                clientRect.right = x + 1;
                clientRect.top = y;
                clientRect.bottom = y + 1;

                // lets get the tiled rect
                XGUntileSurface(
                    pixelRaw, 
                    mImageInfo->mPitch,
                    &point,
                    mImageInfo->mImageData,
                    mImageInfo->mPlatformSpecific.mWidthInBlocks,
                    mImageInfo->mPlatformSpecific.mHeightInBlocks,
                    &clientRect,
                    mImageInfo->mPlatformSpecific.mBytesPerBlock);

                result = GetPixelDataFromFormat(pixelRaw);
            }
            else
            {
                // regular linear texture, use the linear buffer
                result = GetPixelLinearInternal(x, y);
            }

            return result;
        }
    }
}
