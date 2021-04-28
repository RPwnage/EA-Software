///////////////////////////////////////////////////////////////////////////////
// ScreenCopyMantle.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2014 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef __SCREEN_COPY_Mantle_H__
#define __SCREEN_COPY_Mantle_H__

#include "IScreenCopy.h"
#include <mantle.h>
#include "MantleObjects.h"

namespace OriginIGO {

    class ScreenCopyMantle : public IScreenCopy
    {
    public:
    
        //////////////////////////////////////////////////////////
        /// \brief ScreenCopy Constructor.
        ScreenCopyMantle();

        //////////////////////////////////////////////////////////
        /// \brief ScreenCopy Destructor.
        virtual ~ScreenCopyMantle();

    private:

	    //////////////////////////////////////////////////////////
	    /// \brief Create the surfaces used by ScreenCopy
		/// \param deviceQueue Is our current render command queue.
		/// \param rt Information about the current back buffer format.
		/// \return true if succeeded.
		bool CreateSurfaces(void *context);

        //////////////////////////////////////////////////////////
        /// \brief Release the surfaces used by ScreenCopy
        void ReleaseSurfaces(GR_CMD_BUFFER cmd, int deviceIndex = -1);

    public: // implementation of IScreenCopy

        #pragma region IScreenCopy
        //////////////////////////////////////////////////////////
        /// \brief Call resources that will 'survive' a device lost scenario.
        /// \param pDevice The device to create all the resources on.
        /// \return returns true if all resources are allocated.
        virtual bool Create(void *pDevice = 0, void *renderTarget = 0, int widthOverride = 0, int heightOverride = 0);
    
        //////////////////////////////////////////////////////////
        /// \brief Create all the resources that will need to be released when a device is lost.
        /// \param pDevice The device to create the resources on.
        /// \return Returns true if all resources are allocated.
        virtual bool Reset(void *pDevice);

        //////////////////////////////////////////////////////////
        /// \brief Release all resources that have to be released when a device is lost.
        /// \param pDevice The device to release the resources on.
        /// \return Returns true if all resources are properly released.
        virtual bool Lost();

	    //////////////////////////////////////////////////////////
	    /// \brief Release all resources we created on this device.
	    virtual bool Destroy(void *context = NULL);

	    //////////////////////////////////////////////////////////
	    /// \brief Perform the render operation.
	    /// \param pDevice The device to perform the render operation on.
	    /// \return returns true if the render operation succeeded.
		virtual bool Render(void *context, void *renderTarget = 0);

        //////////////////////////////////////////////////////////
        /// \brief Copy the texture from device texture to memory
        /// \param [in] pDevice the device to copy from.
        /// \param [in] pDestination The system memory to copy the texture to.
        /// \param [in,out] The buffer size of the result. Pass in the available size of the buffer. If the buffer is too small size will return the necessary amount needed. 
        /// \param [out] width The horizontal size of the image.
        /// \param [out] height The vertical size of the image.
        virtual bool CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth);

        //////////////////////////////////////////////////////////
        /// \brief Copy the memory texture to the device to be rendered in the next render call.
        ///    \param [in] pDevice The device to render the texture to.
        /// \param [in] pSource The memory to copy the texture contents from.
        /// \param [in] width The width of the texture
        /// \param [in] height The height of the texture
        virtual bool CopyFromMemory(void *pDevice, void *pSource, int width, int heigth);

        //////////////////////////////////////////////////////////
        /// \brief Returns the DirectX object count for propper release hook calculations
        virtual int GetObjectCount();

        //////////////////////////////////////////////////////////
        /// \brief Returns true if we have a SRGB colour format
        //static bool isSRGB_format(DXGI_FORMAT f);
#pragma endregion



    private:
        int    mWidth;    ///< The width of the output image
        int mHeight;    ///< The height of the output image
        int    mSourceWidth;    ///< The width of the source image
        int mSourceHeight;    ///< The height of the source image
        int mFPS;        ///< The number of frames per second

        static const unsigned int NUM_OFFSCREEN_BUFFERS = 3; // we use independant buffers, this keeps the GPU bussy most of the time
        unsigned int mCurrentOffscreenBuffer[GR_MAX_PHYSICAL_GPUS];
        unsigned int mPrevOffscreenBuffer[GR_MAX_PHYSICAL_GPUS];
        bool mCapturedBufferFilled[GR_MAX_PHYSICAL_GPUS];
		bool mHardwareAccelerated;

        DescriptorSet      *mDescriptorSet[GR_MAX_PHYSICAL_GPUS];
        Sampler            *mSampler[GR_MAX_PHYSICAL_GPUS];
        Mesh               *mMesh[GR_MAX_PHYSICAL_GPUS];
        Pipeline           *mPipeline[GR_MAX_PHYSICAL_GPUS];
        ConstantBuffer     *mNeverChangeConstants[GR_MAX_PHYSICAL_GPUS];
        ConstantBuffer        *mPerPassConstants[GR_MAX_PHYSICAL_GPUS];

		RenderPass		   *mRenderPass[GR_MAX_PHYSICAL_GPUS][NUM_OFFSCREEN_BUFFERS];
		RenderPass		   *mRenderPass2[GR_MAX_PHYSICAL_GPUS];
		RenderState        *mRenderState[GR_MAX_PHYSICAL_GPUS];
		Drawable			*mDrawable[GR_MAX_PHYSICAL_GPUS];
		Shader				*mShaderPs[GR_MAX_PHYSICAL_GPUS];
		Shader				*mShaderVs[GR_MAX_PHYSICAL_GPUS];

        RenderTarget    *mRenderTarget[GR_MAX_PHYSICAL_GPUS];
        RenderTarget    *mRenderTargetScaled[GR_MAX_PHYSICAL_GPUS][NUM_OFFSCREEN_BUFFERS];
        Texture            *mBackBufferCopy[GR_MAX_PHYSICAL_GPUS][NUM_OFFSCREEN_BUFFERS];
        Texture            *mBackBufferOffscreenCPU[GR_MAX_PHYSICAL_GPUS][NUM_OFFSCREEN_BUFFERS];
    };

}

#endif //__SCREEN_COPY_Mantle_H__