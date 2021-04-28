///////////////////////////////////////////////////////////////////////////////
// ScreenCopyDX9.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ScreenCopyDx9.h"
#include "IGO.h"
#include "resource.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "twitchmanager.h"
#include "IGOLogger.h"
#include "IGOApplication.h"

namespace OriginIGO {

    ScreenCopyDx9::ScreenCopyDx9() :
        mWidth(0),
        mHeight(0),
        mSourceWidth(0),
        mSourceHeight(0),
        mCurrentOffscreenBuffer(0),
        mPrevOffscreenBuffer(0),
        mCapturedBufferFilled(false),
        mFPS(0)
    {
        //MessageBox(NULL, L"Twitch ScreenCopyDX9", L"breakpoint", MB_OK);

        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            mBackBufferOffscreenCPU[i] = NULL;
            mRenderTargetScaled[i] = NULL;
        }
        mRenderTargetScaledRect.left = mRenderTargetScaledRect.right = mRenderTargetScaledRect.top = mRenderTargetScaledRect.bottom = 0;
    }

    ScreenCopyDx9::~ScreenCopyDx9()
    {
    }

    #pragma region IScreenCopy Implementation

    bool ScreenCopyDx9::CreateSurfaces(CComPtr<IDirect3DDevice9> &pDevice9, const D3DSURFACE_DESC &desc)
    {   
        ReleaseSurfaces();
        HRESULT hr = E_FAIL;

        mSourceWidth = desc.Width;
        mSourceHeight = desc.Height;

        if (TTV_SUCCEEDED(TwitchManager::InitializeForStreaming(desc.Width, desc.Height, mWidth, mHeight, mFPS)))
        {
            for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
            {
                // create image surface to store the image
                if(FAILED(hr=pDevice9->CreateRenderTarget(mWidth, mHeight, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, false, &mRenderTargetScaled[i], NULL)))
                {
                    IGOLogWarn("CreateRenderTarget() failed! hr = %x", hr);
                    TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                    ReleaseSurfaces();
                    return false;
                }
                if(FAILED(hr=pDevice9->CreateOffscreenPlainSurface(mWidth, mHeight, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &mBackBufferOffscreenCPU[i], NULL)))
                {
                    IGOLogWarn("CreateOffscreenPlainSurface() failed! hr = %x", hr);
                    TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                    ReleaseSurfaces();
                    return false;
                }
            }

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

            mRenderTargetScaledRect.left = (long) ((mWidth-gameWidth)*0.5f);
            mRenderTargetScaledRect.right = (long) (gameWidth + (mWidth-gameWidth)*0.5f);
            mRenderTargetScaledRect.top = (long) ((mHeight-gameHeight)*0.5f);
            mRenderTargetScaledRect.bottom = (long) (gameHeight + (mHeight-gameHeight)*0.5f);

            return true;
        }

        return false;
    }


    void ScreenCopyDx9::ReleaseSurfaces()
    {
        if (TwitchManager::IsTTVDLLReady())
        {
            TwitchManager::TTV_PauseVideo();
        }

        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            SAFE_RELEASE(mBackBufferOffscreenCPU[i]);
            SAFE_RELEASE(mRenderTargetScaled[i]);
        }

        mPrevOffscreenBuffer = mCurrentOffscreenBuffer = 0;
        mCapturedBufferFilled = false;

    }


    bool ScreenCopyDx9::Create(void *pDevice, void *pSwapChain, int widthOverride, int heightOverride)
    {
        CComPtr<IDirect3DDevice9> pDevice9 = (IDirect3DDevice9 *)pDevice;

        CComPtr<IDirect3DSurface9> pBackbuffer;
        D3DSURFACE_DESC desc;
    
        pDevice9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackbuffer);

        if(pBackbuffer != NULL)
        {
            pBackbuffer->GetDesc(&desc);
            if ((widthOverride > 0 && heightOverride > 0) && widthOverride < (int)desc.Width || heightOverride < (int)desc.Height)
            {
                // modify our dimesions to match the override
                desc.Width = widthOverride;
                desc.Height = heightOverride;
            }

            CreateSurfaces(pDevice9, desc);
        }
        return true;
    }

    int ScreenCopyDx9::GetObjectCount()
    {
        int objects = 0;
        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            if (mBackBufferOffscreenCPU[i])
            objects++;

            if (mRenderTargetScaled[i])
                objects++;
        }

        return objects;
    }


    bool ScreenCopyDx9::Reset(void *pDevice)
    {
        return true;
    }
    
    bool ScreenCopyDx9::Lost()
    {
        ReleaseSurfaces();
        return true;
    }

	bool ScreenCopyDx9::Destroy(void *userData)
    {
        ReleaseSurfaces();
        return true;
    }

    bool ScreenCopyDx9::Render(void *pDevice, void *pSwapChain)
    {
        if (!IGOApplication::instance())
        {
            TwitchManager::TTV_PauseVideo();
            return true;
        }

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

        CComPtr<IDirect3DDevice9> pDevice9 = (IDirect3DDevice9 *)pDevice;
        CComPtr<IDirect3DSurface9> pBackBuffer;
    
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

        HRESULT hr = S_OK;

        if(SUCCEEDED(hr=pDevice9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
        {
            // clear our target surface to black
            pDevice9->ColorFill(mRenderTargetScaled[mCurrentOffscreenBuffer], NULL, 0);

            // copy RT to non multi sample RT in order to make "GetRenderTargetData" work + scale to twitch size
            if(SUCCEEDED(hr=pDevice9->StretchRect(pBackBuffer, NULL, mRenderTargetScaled[mCurrentOffscreenBuffer], &mRenderTargetScaledRect, D3DTEXF_NONE)))
            {
                D3DLOCKED_RECT lockedRect;
                if(mCapturedBufferFilled && SUCCEEDED(hr=mBackBufferOffscreenCPU[mPrevOffscreenBuffer]->LockRect( &lockedRect, NULL, D3DLOCK_NOSYSLOCK|D3DLOCK_DONOTWAIT|D3DLOCK_READONLY)))
                {
                    TwitchManager::tPixelBuffer *pixelBuffer = TwitchManager::GetUnusedPixelBuffer();
                    if (pixelBuffer)
                    {
                        CopySurfaceData(lockedRect.pBits, pixelBuffer->data, mWidth, mHeight, lockedRect.Pitch);

                        TwitchManager::TTV_SubmitVideoFrame(pixelBuffer->data, pixelBuffer);
                    }                        

                    mBackBufferOffscreenCPU[mPrevOffscreenBuffer]->UnlockRect();
                }


                return true;
            }
        }
        TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
        ReleaseSurfaces();

        return false;
    }


    bool ScreenCopyDx9::CopyRT(void *pDevice, void *pSwapChain)
    {

        CComPtr<IDirect3DDevice9> pDevice9 = (IDirect3DDevice9 *)pDevice;
        
        HRESULT hr = S_OK;
        if(SUCCEEDED(hr=pDevice9->GetRenderTargetData(mRenderTargetScaled[mCurrentOffscreenBuffer], mBackBufferOffscreenCPU[mCurrentOffscreenBuffer])))
        {
            return true;
        }

        TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
        ReleaseSurfaces();

        return false;
    }

    bool ScreenCopyDx9::CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth)
    {
    
        return true;
    }

    bool ScreenCopyDx9::CopyFromMemory(void *pDevice, void *pSource, int width, int heigth)
    {
        return false;
    }


    #pragma endregion

}
