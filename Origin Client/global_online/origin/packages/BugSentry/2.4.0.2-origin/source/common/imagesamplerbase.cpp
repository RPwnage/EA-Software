/******************************************************************************/
/*!
    ImageSamplerBase

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/imagesamplerbase.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ImageSamplerBase::ImageSamplerBase

            \brief      Constructor

            \param       - screenshotInfo: The image information of the screenshot.
        */
        /******************************************************************************/
        ImageSamplerBase::ImageSamplerBase(const ImageInfo* screenshotInfo) : mImageInfo(NULL), mDownSamplingScale(1), mWidth(0), mHeight(0)
        {
            mImageInfo = screenshotInfo;

            switch (mImageInfo->mImageDownSample)
            {
                case IMAGE_DOWNSAMPLE_2X:
                {
                    mDownSamplingScale = 2;
                    break;
                }
                case IMAGE_DOWNSAMPLE_4X:
                {
                    mDownSamplingScale = 4;
                    break;
                }
                case IMAGE_DOWNSAMPLE_NONE:
                default:
                {
                    mDownSamplingScale = 1;
                    break;
                } 
            }

            mWidth = mImageInfo->mWidth / mDownSamplingScale;
            mHeight = mImageInfo->mHeight / mDownSamplingScale;
        }

        /******************************************************************************/
        /*! ImageSamplerBase::GetPixel

            \brief      Retrieves the pixel information at the given coordinates.

            \param       - x: The x coordinate to retrieve the pixel information.
            \param       - y: The y coordinate to retrieve the pixel information.
            \return      - ImagePixel structure containing red, green, & blue values.
        */
        /******************************************************************************/
        ImagePixel ImageSamplerBase::GetPixel(int x, int y)
        {
            ImagePixel result = {0,0,0};
            int samplingWindow = mDownSamplingScale;
            uint32_t resultPixel[3] = {0}; // RGB in long format for temporary computations
            float_t inverseSamplingScale = 0;

            // transform to our sampling scale to sample the larger image if needed
            x *= mDownSamplingScale;
            y *= mDownSamplingScale;

            inverseSamplingScale = 1.0f / (mDownSamplingScale * mDownSamplingScale);

            for (int samplingOnY = 0; samplingOnY < samplingWindow; ++samplingOnY)
            {
                for (int samplingOnX = 0; samplingOnX < samplingWindow; ++samplingOnX)
                {
                    ImagePixel current = GetPixelPlatformSpecificInternal(x + samplingOnX, y + samplingOnY);
                    resultPixel[0] += current.mRed;
                    resultPixel[1] += current.mGreen;
                    resultPixel[2] += current.mBlue;
                }
            }

            result.mRed = static_cast<uint8_t>(resultPixel[0] * inverseSamplingScale);
            result.mGreen = static_cast<uint8_t>(resultPixel[1] * inverseSamplingScale);
            result.mBlue = static_cast<uint8_t>(resultPixel[2] * inverseSamplingScale);

            return result;
        }

        /******************************************************************************/
        /*! ImageSamplerBase::GetPixelLinearInternal

            \brief      Retrieves the pixel information at the given coordinates.

            \param       - x: The x coordinate to retrieve the pixel information.
            \param       - y: The y coordinate to retrieve the pixel information.
            \return      - ImagePixel structure containing red, green, & blue values.
        */
        /******************************************************************************/
        ImagePixel ImageSamplerBase::GetPixelLinearInternal(int x, int y)
        {
            ImagePixel result = {0,0,0};
            const uint8_t *charData = static_cast<const uint8_t*>(mImageInfo->mImageData);
            const uint8_t *pixelData = 0;

            // clamp x and y
            x = ClampValue(x, 0, mImageInfo->mWidth - 1);
            y = ClampValue(y, 0, mImageInfo->mHeight - 1);

            // got something
            pixelData = &charData[(y * mImageInfo->mPitch) + (x * mImageInfo->mBytesPerPixel)];

            // the actual data depends on the data format
            result = GetPixelDataFromFormat(pixelData);

            return result;
        }

        /******************************************************************************/
        /*! ImageSamplerBase::GetPixelDataFromFormat

            \brief      Retrieves the pixel information using the image format.

            \param       - data: The RGB data.
            \return      - ImagePixel structure containing red, green, & blue values.
        */
        /******************************************************************************/
        ImagePixel ImageSamplerBase::GetPixelDataFromFormat(const uint8_t* data)
        {
            ImagePixel result = {0,0,0};

            if (mImageInfo->mImageFormat == IMAGE_FORMAT_X8R8G8B8)
            {
                result.mRed = data[1];
                result.mGreen = data[2];
                result.mBlue = data[3];
            }
            else if (mImageInfo->mImageFormat == IMAGE_FORMAT_LE_X8R8G8B8)
            {
                result.mRed = data[2];
                result.mGreen = data[1];
                result.mBlue = data[0];
            }

            return result;
        }

        /******************************************************************************/
        /*! ImageSamplerBase::ClampValue

            \brief      Retrieves the pixel information using the image format.

            \param       - value: The value to clamp.
            \param       - minValue: The min value to clamp to.
            \param       - maxValue: The max value to clamp to.
            \return      - clamped value.
        */
        /******************************************************************************/
        int ImageSamplerBase::ClampValue(int value, int minValue, int maxValue)
        {
            int result = value;

            result = (result > maxValue) ? maxValue : result;
            result = (result < minValue) ? minValue : result;

            return result;
        }
    }
}
