///////////////////////////////////////////////////////////////////////////////
// ScreenCopyGL.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef __SCREEN_COPY_GL_H__
#define __SCREEN_COPY_GL_H__

#include "IScreenCopy.h"
#include <windows.h>
#include <gl/gl.h>

namespace OriginIGO {

    class ScreenCopyGL : public IScreenCopy
    {
    public:
    
        //////////////////////////////////////////////////////////
        /// \brief ScreenCopy Constructor.
        ScreenCopyGL();

        //////////////////////////////////////////////////////////
        /// \brief ScreenCopy Destructor.
        virtual ~ScreenCopyGL();

    private:

        //////////////////////////////////////////////////////////
        /// \brief Create the surfaces used by ScreenCopy
        /// \param window our rendering window.
        /// \param desc Information about the current back buffer format.
        /// \return true if succeeded.
        bool CreateSurfaces(HWND window, void *context = 0);

        //////////////////////////////////////////////////////////
        /// \brief Release the surfaces used by ScreenCopy
        void ReleaseSurfaces();

    public: // implementation of IScreenCopy

        #pragma region IScreenCopy
        //////////////////////////////////////////////////////////
        /// \brief Call resources that will 'survive' a device lost scenario.
        /// \param window The rendering window.
        /// \param context The device to create all the resources on.
        /// \return returns true if all resources are allocated.
        virtual bool Create(void *window, void *context = 0, int widthOverride = 0, int heightOverride = 0);
    
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
		virtual bool Destroy(void *userData = NULL);

        //////////////////////////////////////////////////////////
        /// \brief Perform the render operation.
        /// \param context The context to perform the render operation on.
        /// \return returns true if the render operation succeeded.
        virtual bool Render(void *context, void *notUsed = 0);
               
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
    #pragma endregion



    private:
        int    mWidth;    ///< The width of the output image
        int mHeight;    ///< The height of the output image
        int    mSourceWidth;    ///< The width of the source image
        int mSourceHeight;    ///< The height of the source image
        int mFPS;        ///< The number of frames per second

        static const unsigned int NUM_OFFSCREEN_BUFFERS = 3;    // we use 3 independant buffers, this keeps the GPU unstalled most of the time
        int mCurrentOffscreenBuffer;
        int mPrevOffscreenBuffer;
        bool mCapturedBufferFilled;

        GLuint mRenderTargetTextureID[NUM_OFFSCREEN_BUFFERS];
        GLuint mRenderTargetTextureScaledID[NUM_OFFSCREEN_BUFFERS];
        GLuint mFrameBuffer;
        
        GLint mCurrentViewPort[4];
        bool bSupportsBRGA;
        
    typedef struct QuadVertex{
        float v[3];
        float t[2];
    }QuadVertex;

    };

}

#endif //__SCREEN_COPY_GL_H__