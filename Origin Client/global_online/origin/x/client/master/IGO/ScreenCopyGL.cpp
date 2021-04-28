///////////////////////////////////////////////////////////////////////////////
// ScreenCopyGL.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ScreenCopyGL.h"
#include "IGO.h"
#include "OGLSurface.h"
#include "OGLHook.h"

#include <windows.h>

#include <gl/gl.h>
#include "glext.h"

#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "twitchmanager.h"
#include "IGOLogger.h"
#include "IGOApplication.h"

#define OGLFN(retval, name, params) \
    typedef retval (APIENTRY *name ## Fn) params; \
    extern name ## Fn _ ## name;

#ifdef DEBUG
#define GL(x) _ ## x; \
    if (SHOW_GL_ERRORS) { \
    uint32_t error = _glGetError(); \
    if (error != GL_NO_ERROR) { OriginIGO::IGOLogDebug("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, #x); } \
    }
#else
#define GL(x) _ ## x
#endif

#ifdef DEBUG
#define GL_EXT(fcn, ...) \
    if ( ## fcn) \
    { \
        ## fcn(__VA_ARGS__); \
        if (SHOW_GL_ERRORS) \
        { \
        uint32_t error = _glGetError(); \
        if (error != GL_NO_ERROR) { OriginIGO::IGOLogDebug("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, #fcn); } \
        } \
    }

#else
#define GL_EXT(fcn, ...) \
    if ( ## fcn) \
         ## fcn(__VA_ARGS__);

#endif



#include "OGLFunctions.h"

#define GET_PROC_ADDRESS _wglGetProcAddress
#define QUERY_EXTENSION_ENTRY_POINT(name, type)               \
    name = (type)GET_PROC_ADDRESS(#name);\
    if (! ## name) \
    { \
        OriginIGO::IGOLogWarn("OpenGL extension missing: %s", #name); \
    }


extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
extern PFNGLBINDBUFFERARBPROC glBindBufferARB;
extern PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;
//extern PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
extern PFNGLSECONDARYCOLORPOINTERPROC glSecondaryColorPointer;
extern PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT;
extern PFNGLDRAWBUFFERSARBPROC glDrawBuffersARB;

namespace OriginIGO {

    extern HMODULE gInstDLL;

    ScreenCopyGL::ScreenCopyGL() :
        mWidth(0),
        mHeight(0),
        mSourceWidth(0),
        mSourceHeight(0),
        mCurrentOffscreenBuffer(0),
        mPrevOffscreenBuffer(0),
        mCapturedBufferFilled(false),
        mFPS(0),
        bSupportsBRGA(false)

    {
        //MessageBox(NULL, L"Twitch ScreenCopyGL", L"breakpoint", MB_OK);

        //FBO
        QUERY_EXTENSION_ENTRY_POINT(glGenFramebuffersEXT, PFNGLGENFRAMEBUFFERSEXTPROC);
        QUERY_EXTENSION_ENTRY_POINT(glBindFramebufferEXT, PFNGLBINDFRAMEBUFFEREXTPROC);
        QUERY_EXTENSION_ENTRY_POINT(glRenderbufferStorageEXT, PFNGLRENDERBUFFERSTORAGEEXTPROC);
        QUERY_EXTENSION_ENTRY_POINT(glCheckFramebufferStatusEXT, PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC);
        QUERY_EXTENSION_ENTRY_POINT(glDeleteFramebuffersEXT, PFNGLDELETEFRAMEBUFFERSEXTPROC);
        QUERY_EXTENSION_ENTRY_POINT(glFramebufferTexture2DEXT, PFNGLFRAMEBUFFERTEXTURE2DEXTPROC);
        QUERY_EXTENSION_ENTRY_POINT(glBlendFuncSeparate, PFNGLBLENDFUNCSEPARATEPROC);
        bSupportsBRGA = OGLSurface::isExtensionSupported("GL_EXT_bgra");

        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            mRenderTargetTextureID[i] = 0;
            mRenderTargetTextureScaledID[i] = 0;
        }
        mFrameBuffer = 0;
    }

    ScreenCopyGL::~ScreenCopyGL()
    {
    }

    #pragma region IScreenCopy Implementation

    bool ScreenCopyGL::CreateSurfaces(HWND window, void *context)
    {   

        ReleaseSurfaces();

        RECT windowRect;
        windowRect.bottom=windowRect.left=windowRect.right=windowRect.top=0;

        if(!GetClientRect(window, &windowRect))
            windowRect.bottom=windowRect.left=windowRect.right=windowRect.top=0;

        mSourceWidth=(windowRect.right-windowRect.left);
        mSourceHeight=(windowRect.bottom-windowRect.top);

        if (TTV_SUCCEEDED(TwitchManager::InitializeForStreaming(mSourceWidth, mSourceHeight, mWidth, mHeight, mFPS)))
        {
            _glGetError(); // reset last error
            // create render target texture
            for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
            {
                GL(glGenTextures(1, &mRenderTargetTextureID[i]));                                
                GL(glBindTexture(GL_TEXTURE_2D, mRenderTargetTextureID[i]));                    
                GL(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
                GL(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE));
                GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mSourceWidth, mSourceHeight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL));                        
                GL(glBindTexture(GL_TEXTURE_2D, 0));
            
                // create render scaled target texture
                GL(glGenTextures(1, &mRenderTargetTextureScaledID[i]));                            
                GL(glBindTexture(GL_TEXTURE_2D, mRenderTargetTextureScaledID[i]));                    
                GL(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
                GL(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE));
                GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL));                        
                GL(glBindTexture(GL_TEXTURE_2D, 0));
            }

            // create frame buffer
            GL_EXT(glGenFramebuffersEXT, 1, &mFrameBuffer);
            GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, mFrameBuffer);
        
            
            // unbind draw buffer
            // reset the list of draw buffers.
            GLenum ResetDrawBuffers[] = {GL_NONE};
            GL_EXT(glDrawBuffersARB, 1, ResetDrawBuffers);
            GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, 0);

            // unbind texture and
            GL(glBindTexture(GL_TEXTURE_2D, 0));
        
            return true;
        }

        return false;
    }


    void ScreenCopyGL::ReleaseSurfaces()
    {
        if (TwitchManager::IsTTVDLLReady())
        {
            TwitchManager::TTV_PauseVideo();
        }

        mPrevOffscreenBuffer = mCurrentOffscreenBuffer = 0;
        mCapturedBufferFilled = false;
        
        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            if (mRenderTargetTextureID[i] != 0)
            {
                GL(glDeleteTextures(1, &mRenderTargetTextureID[i]));
                mRenderTargetTextureID[i] = 0;
            }

            if (mRenderTargetTextureScaledID[i] != 0)
            {
                GL(glDeleteTextures(1, &mRenderTargetTextureScaledID[i]));
                mRenderTargetTextureScaledID[i] = 0;
            }
        }        

        if (mFrameBuffer != 0)
            GL_EXT(glDeleteFramebuffersEXT, 1, &mFrameBuffer);
        mFrameBuffer = 0;
    }


    bool ScreenCopyGL::Create(void *window, void *context, int widthOverride, int heightOverride)
    {
        return CreateSurfaces(*((HWND*)window), context);
    }

    int ScreenCopyGL::GetObjectCount()
    {

        return 0;
    }


    bool ScreenCopyGL::Reset(void *pDevice)
    {
        ReleaseSurfaces();
        return true;
    }
    
    bool ScreenCopyGL::Lost()
    {
        ReleaseSurfaces();
        return true;
    }

	bool ScreenCopyGL::Destroy(void *userData)
    {
        ReleaseSurfaces();
        return true;
    }
   
    bool ScreenCopyGL::Render(void *context, void *notUsed)
    {

        if (!IGOApplication::instance())
        {
            TwitchManager::TTV_PauseVideo();
            return true;
        }

        //static int pictureNumber = 0;
        static LARGE_INTEGER last_update = {0, 0};
        static LARGE_INTEGER freq = {0,0};

        if(freq.QuadPart == 0)
        {
            QueryPerformanceFrequency(&freq);
        }

        LARGE_INTEGER current_update;
        QueryPerformanceCounter(&current_update);

        // Feed the encoder with 1.25x the frame rate to prevent stuttering!
        if (TwitchManager::GetUsedPixelBufferCount() > 16 /*too many unencoded frames 1920*1080 -> 16 frames ~ 125MB*/ || ((current_update.QuadPart - last_update.QuadPart) * (mFPS*1.25f))/freq.QuadPart == 0)
            return false;

        last_update = current_update;

        mCurrentOffscreenBuffer++;
        if (mCurrentOffscreenBuffer>=NUM_OFFSCREEN_BUFFERS)
            mCurrentOffscreenBuffer = 0;

        mPrevOffscreenBuffer++;
        // we reached the last frame in our buffer, now start the actual daa transfer into the offscreen buffer
        // cur.  osb   0 1 2 0 1 2 0 1 2 ...
        // prev. osb   - - 0 1 2 0 1 ...
        if (mCurrentOffscreenBuffer==(NUM_OFFSCREEN_BUFFERS-1))
        {
            mPrevOffscreenBuffer = 0;
            mCapturedBufferFilled = true;     
        }

        _glGetError(); // reset last error

        GL(glGetIntegerv(GL_VIEWPORT, mCurrentViewPort));
        GL(glDisable(GL_BLEND));

        //get the back buffer
        GL(glBindTexture(GL_TEXTURE_2D, mRenderTargetTextureID[mCurrentOffscreenBuffer]));                    
        GL(glViewport(0, 0, mSourceWidth, mSourceHeight));
        GL(glReadBuffer(GL_BACK));
        GL(glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, mSourceWidth, mSourceHeight));
        
        // bind frame buffer
        GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, mFrameBuffer);
        
        // bind scaled render target
        GL(glBindTexture(GL_TEXTURE_2D, mRenderTargetTextureScaledID[mCurrentOffscreenBuffer]));                    

        // set render target as our colour attachement #0
        GL_EXT(glFramebufferTexture2DEXT, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mRenderTargetTextureScaledID[mCurrentOffscreenBuffer], 0);

        // set the list of draw buffers.
        GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0};
        GL_EXT(glDrawBuffersARB, 1, DrawBuffers);
        
        // always check that our framebuffer is ok
        GLenum FBOError = GL_FRAMEBUFFER_UNSUPPORTED;
        if (glCheckFramebufferStatusEXT)
            FBOError = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);

        if(FBOError != GL_FRAMEBUFFER_COMPLETE)
        {
            // unbind draw buffer
            // reset the list of draw buffers.
            GLenum ResetDrawBuffers[] = {GL_NONE};
            GL_EXT(glDrawBuffersARB, 1, ResetDrawBuffers);
            GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, 0);

            TwitchManager::SetBroadcastError(FBOError, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
            ReleaseSurfaces();

            // unbind texture and restore viewport + projection
            GL(glBindTexture(GL_TEXTURE_2D, 0));
            GL(glViewport(mCurrentViewPort[0], mCurrentViewPort[1], mCurrentViewPort[2], mCurrentViewPort[3]));
            GL(glEnable(GL_BLEND));

            return false;
        }
        
        // clear our render target - black
        GL(glViewport(0 , 0, mWidth, mHeight));                                
        GL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
        GL(glClear(GL_COLOR_BUFFER_BIT));

        GL(glBindTexture(GL_TEXTURE_2D, mRenderTargetTextureID[mCurrentOffscreenBuffer]));                


        OGLHook::DevContext *deviceContext = (OGLHook::DevContext *)context;
        if (deviceContext && deviceContext->windowShader && deviceContext->windowShader->IsValid())
        {
            // opengl 3.2+ path
 
            // Set window alpha
            deviceContext->windowShader->SetAlpha(1.0);
            
            // calculate aspect ratio and scaling values - game screen vs video resolution
            int gameHeight = (int)(((float)((float)mSourceHeight/(float)mSourceWidth))*mWidth);
            int gameWidth = mWidth;

            if (gameHeight>mHeight)
            {
                float scale = (((float)mHeight) / ((float)gameHeight));
                gameHeight = mHeight;
                gameWidth = (int)(gameWidth * scale);
            }
        
            if (gameWidth>mWidth)
            {
                float scale = (((float)mWidth) / ((float)gameWidth));
                gameWidth = mWidth;
                gameHeight = (int)(gameHeight * scale);
            }
            
            float X = ((mWidth-gameWidth)*0.5f) - (float)mWidth / 2;
            float Y = -((mHeight-gameHeight)*0.5f) + (float)mHeight / 2;

            // Compute/set xform matrix
            GLfloat modelViewMatrix[16];
            ZeroMemory(modelViewMatrix, sizeof(modelViewMatrix));
            modelViewMatrix[0] = (GLfloat)gameWidth;
            modelViewMatrix[5] = (GLfloat)gameHeight;
            modelViewMatrix[10] = modelViewMatrix[15] = 1.0f;
            modelViewMatrix[12] = X;
            modelViewMatrix[13] = Y;


            // setup projection matrix
            ZeroMemory(deviceContext->projMatrix, sizeof(deviceContext->projMatrix));
           
            deviceContext->projMatrix[0]  =  2.0f / (mWidth == 0 ? 1 : mWidth);
            deviceContext->projMatrix[5]  =  2.0f / (mHeight == 0 ? 1 : mHeight);
            deviceContext->projMatrix[10] = -2.0f / (9);
            deviceContext->projMatrix[14] =  2.0f / (9);
            deviceContext->projMatrix[15] =  1.0f;


            GLfloat xForm[16];
            OriginIGO::MultMatrix(xForm, modelViewMatrix, deviceContext->projMatrix);
            deviceContext->windowShader->SetMVPMatrix(xForm);
           
            // Render!
            deviceContext->windowShader->Render();

        }
        else
        {
            // legacy code path

            // set ortho view
            GL(glMatrixMode(GL_PROJECTION));                                
            GL(glPushMatrix());                                                
            GL(glLoadIdentity());                                            
            GL(glOrtho( 0, mWidth, mHeight, 0, -1, 1 ));                
            GL(glMatrixMode(GL_MODELVIEW));                                    
            GL(glPushMatrix());                                                
            GL(glLoadIdentity());                                            

            // calculate aspect ratio and scaling values - game screen vs video resolution
            int gameHeight = (int)(((float)((float)mSourceHeight/(float)mSourceWidth))*mWidth);
            int gameWidth = mWidth;

            if (gameHeight>mHeight)
            {
                float scale = (((float)mHeight) / ((float)gameHeight));
                gameHeight = mHeight;
                gameWidth = (int)(gameWidth * scale);
            }
        
            if (gameWidth>mWidth)
            {
                float scale = (((float)mWidth) / ((float)gameWidth));
                gameWidth = mWidth;
                gameHeight = (int)(gameHeight * scale);
            }

            float scaledRect_left = ((mWidth-gameWidth)*0.5f);
            float scaledRect_right = (gameWidth + (mWidth-gameWidth)*0.5f);
            float scaledRect_top =  ((mHeight-gameHeight)*0.5f);
            float scaledRect_bottom = (gameHeight + (mHeight-gameHeight)*0.5f);

            // draw fullscreen quad
            // set alpha & colour
            GL(glColor4f(1.0f, 1.0f, 1.0f, 1.0f));
            _glBegin(GL_QUADS);                                            
                _glTexCoord2d(0.0,0.0); _glVertex2d(scaledRect_left,scaledRect_top);
                _glTexCoord2d(1.0,0.0); _glVertex2d(scaledRect_right,scaledRect_top);
                _glTexCoord2d(1.0,1.0); _glVertex2d(scaledRect_right,scaledRect_bottom);
                _glTexCoord2d(0.0,1.0); _glVertex2d(scaledRect_left,scaledRect_bottom);
            _glEnd();                                                    

            GL(glMatrixMode( GL_PROJECTION ));                                
            GL(glPopMatrix());                                                
            GL(glMatrixMode( GL_MODELVIEW ));                                
            GL(glPopMatrix());
        }



        // unbind draw buffer
        // reset the list of draw buffers.
        GLenum ResetDrawBuffers[] = {GL_NONE};
        GL_EXT(glDrawBuffersARB, 1, ResetDrawBuffers);
        GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, 0);

        // always check that our framebuffer is ok
        FBOError = GL_FRAMEBUFFER_UNSUPPORTED;
        if (glCheckFramebufferStatusEXT)
            FBOError = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);

        if(FBOError != GL_FRAMEBUFFER_COMPLETE)
        {
            TwitchManager::SetBroadcastError(FBOError, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
            ReleaseSurfaces();

            // unbind texture and restore viewport + projection
            GL(glBindTexture(GL_TEXTURE_2D, 0));
            GL(glViewport(mCurrentViewPort[0], mCurrentViewPort[1], mCurrentViewPort[2], mCurrentViewPort[3]));
            GL(glEnable(GL_BLEND));

            return false;
        }


        if (!mCapturedBufferFilled)
        {
            // unbind texture and restore viewport + projection
            GL(glBindTexture(GL_TEXTURE_2D, 0));
            GL(glViewport(mCurrentViewPort[0], mCurrentViewPort[1], mCurrentViewPort[2], mCurrentViewPort[3]));
            GL(glEnable(GL_BLEND));

            return true;
        }

        TwitchManager::tPixelBuffer *pixelBuffer = TwitchManager::GetUnusedPixelBuffer();
        if (pixelBuffer)
        {
            // get scaled render target data
            GL(glBindTexture( GL_TEXTURE_2D, mRenderTargetTextureScaledID[mPrevOffscreenBuffer]));
            if (!bSupportsBRGA)
            {
                GL(glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixelBuffer->data )); 
                
                // format conversion for opengl
                uint32_t* srcPtr = (uint32_t*)pixelBuffer->data;
                uint32_t* dataPtr = (uint32_t*)pixelBuffer->data;
                
                uint32_t count = mHeight * mWidth;
                if (srcPtr != NULL && count > 0)
                {
                    for (uint32_t i = 0; i < count; ++i)
                    {
                        // from B8G8R8A8
                        // to   R8G8B8A8
                        uint32_t ref = srcPtr[i];
                        uint32_t masked = ref & 0x00ff00ff;
                        uint32_t swap = masked | ((ref & 0xff000000) >> 16) | ((ref & 0x0000ff00) << 16);
                        
                        dataPtr[i] = swap;
                    }
                }
            }
            else
            {
                GL(glGetTexImage( GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, pixelBuffer->data )); 
            }
            
            TwitchManager::TTV_SubmitVideoFrame(pixelBuffer->data, pixelBuffer);
            //TwitchManager::MarkUnusedPixelBuffer(pixelBuffer);
        }

        // unbind texture and restore viewport + projection
        GL(glBindTexture(GL_TEXTURE_2D, 0));
        GL(glViewport(mCurrentViewPort[0], mCurrentViewPort[1], mCurrentViewPort[2], mCurrentViewPort[3]));
        GL(glEnable(GL_BLEND));

        return true;
    }
    
    bool ScreenCopyGL::CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth)
    {
    
        return true;
    }

    bool ScreenCopyGL::CopyFromMemory(void *pDevice, void *pSource, int width, int heigth)
    {
        return false;
    }
#pragma endregion
}

