/******************************************************************************/
/*!
    ImageSampler

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/24/2009 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/imagesampler.h"

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

            // Pull the pixel out of the YUV frontbuffer (xfb)
            // Sample a pixel & write it out
            // Find the scanline requested in the XFB
            int width = mImageInfo->mWidth;
            YUV422* frameBufferyuv = static_cast<YUV422*>(mImageInfo->mImageData);
            YUV422* pin = &frameBufferyuv[ width*y ];  // Could also use mPitch here
            YUV444  pout;

            if (x % 2 == 0)
            {
                // even, there will ALWAYS be an odd pixel next to us!
                pout.y = pin[x].y;
                pout.u = pin[x].uv;
                pout.v = pin[x+1].uv;
            }
            else
            {
                pout.y = pin[x].y;
                if (x != width - 2)  // Is this correct?
                {
                    pout.u = (pin[x-1].uv + pin[x+1].uv)/2;
                    pout.v = (pin[x].uv   + pin[x+2].uv)/2;
                }
                else
                {
                    pout.u = pin[x-1].uv;   // odd, there will ALWAYS be a pixel to our left
                    pout.v = pin[x].uv;
                }
            }

            // Convert YUV444 to RGB
            int r = (1164*((signed long)pout.y -  16) +
                1596*((signed long)pout.v - 128) + 500)/1000;

            int g = (1164*((signed long)pout.y -  16) -
                813*((signed long)pout.v - 128) -
                391*((signed long)pout.u - 128) + 500)/1000;

            int b = (1164*((signed long)pout.y -  16) +
                2018*((signed long)pout.u - 128) + 500)/1000;

            r = ClampValue(r, 0, 255);
            g = ClampValue(g, 0, 255);
            b = ClampValue(b, 0, 255);

            result.mRed   = r;
            result.mGreen = g;
            result.mBlue  = b;
            return result;
        }
    }
}
