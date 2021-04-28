///////////////////////////////////////////////////////////////////////////////
// IScreenCopy.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef __ISCREEN_COPY_H__
#define __ISCREEN_COPY_H__

#include "eabase\eabase.h"
#include <string.h> // memcpy

namespace OriginIGO {

    class IScreenCopy
    {
    public:
        virtual ~IScreenCopy(){}
    
        //////////////////////////////////////////////////////////
        /// \brief Call resources that will 'survive' a device lost scenario.
        /// \param pDevice The device to create all the resources on.
        /// \return returns true if all resources are allocated.
        virtual bool Create(void *pDevice = 0, void *pSwapChain = 0, int widthOverride = 0, int heightOverride = 0) = 0;

        //////////////////////////////////////////////////////////
        /// \brief Create all the resources that will need to be released when a device is lost.
        /// \param pDevice The device to create the resources on.
        /// \return Returns true if all resources are allocated.
        virtual bool Reset(void *pDevice) = 0;

        //////////////////////////////////////////////////////////
        /// \brief Release all resources that have to be released when a device is lost.
        /// \param pDevice The device to release the resources on.
        /// \return Returns true if all resources are properly released.
        virtual bool Lost() = 0;

	    //////////////////////////////////////////////////////////
	    /// \brief Release all resources we created on this device.
		virtual bool Destroy(void *userData = NULL) = 0;

        //////////////////////////////////////////////////////////
        /// \brief Perform the render operation.
        /// \param pDevice The device to perform the render operation on.
        /// \return returns true if the render operation succeeded.
        virtual bool Render(void *pDevice, void *pSwapChain = 0) = 0;

        //////////////////////////////////////////////////////////
        /// \brief Perform the copy operation of a render target operation.
        /// \param pDevice The device to perform the copy operation on.
        /// \return returns true if the copy operation succeeded.
        virtual bool CopyRT(void *pDevice, void *pSwapChain = 0) { return false; };
        
        //////////////////////////////////////////////////////////
        /// \brief Copy the texture from device texture to memory
        /// \param [in] pDevice the device to copy from.
        /// \param [in] pDestination The system memory to copy the texture to.
        /// \param [in,out] The buffer size of the result. Pass in the available size of the buffer. If the buffer is too small size will return the necessary amount needed. 
        /// \param [out] width The horizontal size of the image.
        /// \param [out] height The vertical size of the image.
        virtual bool CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth) = 0;

        //////////////////////////////////////////////////////////
        /// \brief Copy the memory texture to the device to be rendered in the next render call.
        ///    \param [in] pDevice The device to render the texture to.
        /// \param [in] pSource The memory to copy the texture contents from.
        /// \param [in] width The width of the texture
        /// \param [in] height The height of the texture
        virtual bool CopyFromMemory(void *pDevice, void *pSource, int width, int heigth) = 0;

        //////////////////////////////////////////////////////////
        /// \brief Returns the DirectX object count for our hooks
        virtual int GetObjectCount() = 0;


        //////////////////////////////////////////////////////////
        /// \brief Internal copy routine for RGBA8 surfaces
        void CopySurfaceData(const void *src, void *dst, uint32_t width, uint32_t height, uint32_t pitch)
        {
            if (pitch == width*4)
            {
                memcpy(dst, src, 4*width*height);
            }
            else
            {                    
                uint32_t lineWidth = 4*width;
                unsigned char *dstPtr = (unsigned char *)dst;
                unsigned char *srcPtr = (unsigned char *)src;
                for (uint32_t i = 0; i < height; i++)
                {
                    memcpy(dstPtr, srcPtr, lineWidth);
                    dstPtr += lineWidth;
                    srcPtr += pitch;
                }
            }
        }
    };

    static IScreenCopy * gScreenCopy = 0;
}

#endif //__ISCREEN_COPY_H__