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

            // Not implemented

            return result;
        }
    }
}
