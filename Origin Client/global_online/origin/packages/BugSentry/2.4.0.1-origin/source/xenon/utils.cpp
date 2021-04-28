/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include <xtl.h>
#include <xgraphics.h> // awesomeness surface description functions
#include "BugSentry/utils.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! Utils::StartFrameBufferAccess

            \brief      Start's Framebuffer access, and collects basic data about the image in the framebuffer.

            \param       imageInfo - The ImageInfo object to store the data into.
            \param       platformGraphicsDevice - The D3D Device pointer (if available).
            \param       platformGraphicsFrameBuffer - The D3D Device frame buffer (if available).
            \param       imageDownSample - The ImageDownSample value to use to compress the screenshot.
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::StartFrameBufferAccess(ImageInfo* imageInfo, void* platformGraphicsDevice, void* platformGraphicsFrameBuffer, ImageDownSample imageDownSample)
        {
            bool success = false;
            HRESULT hr = S_OK;
            LPDIRECT3DDEVICE9 device = static_cast<LPDIRECT3DDEVICE9>(platformGraphicsDevice);
            LPDIRECT3DTEXTURE9 frameBuffer = static_cast<LPDIRECT3DTEXTURE9>(platformGraphicsFrameBuffer);
            const unsigned int BITS_PER_CHAR = 8;

            if (imageInfo)
            {
                if (device && !frameBuffer)
                {
                    hr = device->GetFrontBuffer(&frameBuffer);
                }

                if (SUCCEEDED(hr) && frameBuffer)
                {
                    hr = E_FAIL;
                    D3DLOCKED_RECT lockecRect = {0};
                    XGTEXTURE_DESC textureDescription;

                    // Add a reference to the framebuffer
                    frameBuffer->AddRef();

                    // this function will never fail
                    XGGetTextureDesc(frameBuffer, 0, &textureDescription);

                    if (textureDescription.Format==D3DFMT_A8R8G8B8 ||
                        textureDescription.Format==D3DFMT_X8R8G8B8 || 
                        textureDescription.Format==D3DFMT_LIN_X8R8G8B8 ||
                        textureDescription.Format==D3DFMT_LE_X8R8G8B8)
                    {
                        hr = frameBuffer->LockRect(0, &lockecRect, NULL, D3DLOCK_READONLY);
                    }

                    if (SUCCEEDED(hr))
                    {
                        // everything is good, capture the data
                        imageInfo->mWidth = textureDescription.Width;
                        imageInfo->mHeight = textureDescription.Height;
                        imageInfo->mImageFormat = (textureDescription.Format==D3DFMT_LE_X8R8G8B8) ? IMAGE_FORMAT_LE_X8R8G8B8 : IMAGE_FORMAT_X8R8G8B8;
                        imageInfo->mImageData = lockecRect.pBits;
                        imageInfo->mPitch = lockecRect.Pitch;
                        imageInfo->mUserData = frameBuffer; // keep the frame buffer in user data
                        imageInfo->mImageDownSample = imageDownSample;
                        imageInfo->mBytesPerPixel = XGBitsPerPixelFromFormat(textureDescription.Format) / BITS_PER_CHAR;

                        // platform specific information
                        imageInfo->mPlatformSpecific.mBytesPerBlock = textureDescription.BytesPerBlock;
                        imageInfo->mPlatformSpecific.mHeightInBlocks = textureDescription.HeightInBlocks;
                        imageInfo->mPlatformSpecific.mWidthInBlocks = textureDescription.WidthInBlocks;
                        imageInfo->mPlatformSpecific.mIsTiled = (XGIsTiledFormat(textureDescription.Format) == TRUE);

                        success = true;
                    }
                    else
                    {
                        // something went really wrong
                        frameBuffer->Release();
                    }            
                }
            }

            return success;
        }

        /******************************************************************************/
        /*! Utils::EndFrameBufferAccess

            \brief      End Framebuffer access, we're done accessing it's data.

            \param       imageInfo - The ImageInfo object to store the data into.
            \param       platformGraphicsDevice - The D3D Device pointer (if available).
            \param       platformGraphicsFrameBuffer - The D3D Device frame buffer (if available).
            \return      none.
        */
        /******************************************************************************/
        void Utils::EndFrameBufferAccess(ImageInfo* imageInfo, void* platformGraphicsDevice, void*platformGraphicsFrameBuffer)
        {
            LPDIRECT3DDEVICE9 device = static_cast<LPDIRECT3DDEVICE9>(platformGraphicsDevice);
            LPDIRECT3DTEXTURE9 frameBuffer = static_cast<LPDIRECT3DTEXTURE9>(platformGraphicsFrameBuffer);

            if (imageInfo && imageInfo->mUserData)
            {
                if (device && !frameBuffer)
                {
                    frameBuffer = static_cast<LPDIRECT3DTEXTURE9>(imageInfo->mUserData);
                }

                frameBuffer->UnlockRect(0);
                frameBuffer->Release();
            }
        }

        /******************************************************************************/
        /*! Utils::GetCallstack

            \brief      Get the callstack information (the addresses).

            \param       exceptionPointers - The platform-specific exception information.
            \param       addresses - An array to store the addresses into.
            \param       addressesMax - The max number of entries available in the addresses array.
            \param       addressesFilledCount - The number of entries used when collecting the callstack (optional).
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetCallstack(void* exceptionPointers, unsigned int* addresses, int addressesMax, int* addressesFilledCount)
        {
            bool callstackRetrieved = false;

            if (exceptionPointers && addresses && addressesFilledCount && (addressesMax > 0))
            {
                // lets walk the callstack
                LPEXCEPTION_POINTERS xenonExceptionInformation = static_cast<LPEXCEPTION_POINTERS>(exceptionPointers);        
                unsigned int* frame = reinterpret_cast<unsigned int*>(xenonExceptionInformation->ContextRecord->Gpr1);
                void* instruction = reinterpret_cast<void*>(xenonExceptionInformation->ContextRecord->Iar);
                int addressCount = 0;

                // lets walk the stack as much as possible
                for (int stackIndex = 0; addressCount < (addressesMax - 1); ++stackIndex)
                {
                    addresses[addressCount++] = reinterpret_cast<unsigned int>(instruction);

                    const void* const prevFrame = frame;

                    frame = reinterpret_cast<unsigned int*>(*frame);

                    // If frame is 0 or is not aligned as expected (4 bytes) or is 
                    // a pointer to an address thats before prevFrame, we quit.
                    if(!frame || (reinterpret_cast<unsigned int>(frame) & 15) || (frame < prevFrame))
                    {
                        break;
                    }

                    // lets capture the backchain
                    instruction = reinterpret_cast<void*>(frame[-2]);
                }

                addresses[addressCount] = 0;
                callstackRetrieved = (addressCount > 0); // we found a callstack if we filled anything

                // If requested, return the number of addresses filled
                if (addressesFilledCount)
                {
                    *addressesFilledCount = addressCount;
                }
            }

            return callstackRetrieved;
        }
    }
}