///////////////////////////////////////////////////////////////////////////////
// ScreenCopyDX11.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ScreenCopyDx11.h"
#include "IGO.h"
#include "resource.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "twitchmanager.h"
#include "IGOLogger.h"
#include "IGOApplication.h"

namespace OriginIGO {

    extern HMODULE gInstDLL;

    ScreenCopyDx11::ScreenCopyDx11() :
        mWidth(0),
        mHeight(0),
        mSourceWidth(0),
        mSourceHeight(0),
        mCurrentOffscreenBuffer(0),
        mPrevOffscreenBuffer(0),
        mCapturedBufferFilled(false),
        mFPS(0),
		mHardwareAccelerated(TwitchManager::TTV_IsOriginEncoder())

    {
        //MessageBox(NULL, L"Twitch ScreenCopyDX11", L"breakpoint", MB_OK);

		IGOLogWarn("ScreenCopyDx11: HA: %d\n", mHardwareAccelerated);

        for (int i=0; i<NUM_OFFSCREEN_BUFFERS; i++)
        {
            mBackBufferOffscreenCPU[i] = NULL;
        }

        mShaderResourceViewScaled = NULL;
        mRenderTargetBBResource = NULL;
        mRenderTargetViewScaled = NULL;
        mRenderTargetTextureScaled = NULL;

        mBackBuffer = NULL;
        mBackBufferCopy = NULL;

        mScreenQuadVBLayout = NULL;
        mScreenQuadVB = NULL;

        mPixelShader = NULL;
        mVertexShader = NULL;
        mSamplerState = NULL;
        mBlendState = NULL;
        mDepthStencilState = NULL;

    }

    ScreenCopyDx11::~ScreenCopyDx11()
    {
    }

    #pragma region IScreenCopy Implementation

    bool ScreenCopyDx11::isSRGB_format(DXGI_FORMAT f)
    {
        switch(f)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;
        }
        return false;
    }

    bool ScreenCopyDx11::CreateSurfaces(CComPtr<ID3D11Device> &pDevice11, const D3D11_TEXTURE2D_DESC &desc)
    {   
        ReleaseSurfaces();
    
        mSourceWidth = desc.Width;
        mSourceHeight = desc.Height;

        if (TTV_SUCCEEDED(TwitchManager::InitializeForStreaming(desc.Width, desc.Height, mWidth, mHeight, mFPS)))
        {
            // create our shader effect
	        HRESULT hr;

             // Load Compiled Vertex & Pixel Shader from DLL resource
            HRSRC hRes = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOVSQUADFX), RT_RCDATA);
            IGO_ASSERT(hRes != NULL);
            const char * pIGOVS;
            DWORD dwIGOVSSize = 0;
            HGLOBAL hIGOVS = LoadResource(gInstDLL, hRes);
            pIGOVS = (const char *)LockResource(hIGOVS);
            dwIGOVSSize = SizeofResource(gInstDLL, hRes);

            // if any of these fail, the resouce is not properly compiled into the dll
            IGO_ASSERT(hIGOVS != NULL);
            IGO_ASSERT(pIGOVS != NULL);
            IGO_ASSERT(dwIGOVSSize > 0);

			// setup YUV shader if needed
			if (mHardwareAccelerated)
			{
				hRes = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOPSQUADFX_YUV), RT_RCDATA);
			}
			else
			{
				hRes = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOPSQUADFX), RT_RCDATA);
			}
	        IGO_ASSERT(hRes != NULL);
	        const char * pIGOPS;
            DWORD dwIGOPSSize = 0;
            HGLOBAL hIGOPS = LoadResource(gInstDLL, hRes);
            pIGOPS = (const char *)LockResource(hIGOPS);
            dwIGOPSSize = SizeofResource(gInstDLL, hRes);

            // if any of these fail, the resouce is not properly compiled into the dll
            IGO_ASSERT(hIGOPS != NULL);
            IGO_ASSERT(pIGOPS != NULL);
            IGO_ASSERT(dwIGOPSSize > 0);

            // Create Effect
            hr = pDevice11->CreateVertexShader(pIGOVS, dwIGOVSSize, NULL, &mVertexShader);
            if (FAILED(hr))
            {
                IGOLogWarn("CreateVertexShader() failed. hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                IGO_ASSERT(SUCCEEDED(hr));
                return false;
            }

            hr = pDevice11->CreatePixelShader(pIGOPS, dwIGOPSSize, NULL, &mPixelShader);
            if (FAILED(hr))
            {
                IGOLogWarn("CreatePixelShader() failed. hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                IGO_ASSERT(SUCCEEDED(hr));
                return false;
            }

            D3D11_BUFFER_DESC bd;
            ZeroMemory(&bd, sizeof(bd));


            // Define the input layout
            D3D11_INPUT_ELEMENT_DESC layout[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            UINT numElements = sizeof( layout ) / sizeof( layout[0] );

            hr = pDevice11->CreateInputLayout(layout, numElements, pIGOVS, dwIGOVSSize, &mScreenQuadVBLayout);
            if (FAILED(hr))
            {
                IGOLogWarn("CreateInputLayout() failed. hr = %x", hr);
                IGO_ASSERT(SUCCEEDED(hr));
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            // create sampler state
            D3D11_SAMPLER_DESC sampDesc;
            ZeroMemory(&sampDesc, sizeof(sampDesc));
            sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            sampDesc.MinLOD = 0;
            sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
            hr = pDevice11->CreateSamplerState(&sampDesc, &mSamplerState);
            if (FAILED(hr))
            {
                IGOLogWarn("CreateSamplerState() failed. hr = %x", hr);
                IGO_ASSERT(SUCCEEDED(hr));
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }
            // create blend state
            D3D11_BLEND_DESC blendDesc;
            ZeroMemory(&blendDesc, sizeof(blendDesc));
            blendDesc.RenderTarget[0].BlendEnable = FALSE;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

            hr = pDevice11->CreateBlendState(&blendDesc, &mBlendState);
            if (FAILED(hr))
            {
                IGOLogWarn("CreateBlendState() failed. hr = %x", hr);
                IGO_ASSERT(SUCCEEDED(hr));
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }
            // create depth stencil state
            D3D11_DEPTH_STENCIL_DESC depthDesc;
            ZeroMemory(&depthDesc, sizeof(depthDesc));
            depthDesc.DepthEnable = FALSE;
            depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            hr = pDevice11->CreateDepthStencilState(&depthDesc, &mDepthStencilState);
            if (FAILED(hr))
            {
                IGOLogWarn("CreateDepthStencilState() failed. hr = %x", hr);
                IGO_ASSERT(SUCCEEDED(hr));
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }
            // Create a screen quad for all render to texture operations
            QuadVertex svQuad[4];


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
            
            
            float scaledRect_left;
            float scaledRect_right;
            float scaledRect_top;
            float scaledRect_bottom;

            scaledRect_left = ((mWidth-gameWidth)*0.5f);
            scaledRect_right = (gameWidth + (mWidth-gameWidth)*0.5f);
            scaledRect_top = ((mHeight-gameHeight)*0.5f);
            scaledRect_bottom = (gameHeight + (mHeight-gameHeight)*0.5f);
            
            // translate
            scaledRect_left -= mWidth*0.5f;
            scaledRect_right -= mWidth*0.5f;
            scaledRect_top -= mHeight*0.5f;
            scaledRect_bottom -= mHeight*0.5f;

            // scale
            scaledRect_left *=  2.0f/mWidth;
            scaledRect_right *= 2.0f/mWidth;
            scaledRect_top *= 2.0f/mHeight;
            scaledRect_bottom *= 2.0f/mHeight;

            svQuad[0].pos = D3DXVECTOR4(scaledRect_left, scaledRect_bottom, 0.5f, 1.0f);
            svQuad[0].tex = D3DXVECTOR2(0.0f, 0.0f);
    
            svQuad[1].pos = D3DXVECTOR4(scaledRect_right, scaledRect_bottom, 0.5f, 1.0f);
            svQuad[1].tex = D3DXVECTOR2(1.0f, 0.0f);
    
            svQuad[2].pos = D3DXVECTOR4(scaledRect_left, scaledRect_top, 0.5f, 1.0f);
            svQuad[2].tex = D3DXVECTOR2(0.0f, 1.0f);
    
            svQuad[3].pos = D3DXVECTOR4(scaledRect_right, scaledRect_top, 0.5f, 1.0f);
            svQuad[3].tex = D3DXVECTOR2(1.0f, 1.0f);

            D3D11_BUFFER_DESC vbdesc =
            {
            4*sizeof(QuadVertex),
            D3D11_USAGE_IMMUTABLE,
            D3D11_BIND_VERTEX_BUFFER,
            0,
            0
            };
            
            SAFE_RELEASE(mScreenQuadVB);

            D3D11_SUBRESOURCE_DATA InitData;
            InitData.pSysMem = svQuad;
            InitData.SysMemPitch = 0;
            InitData.SysMemSlicePitch = 0;
            pDevice11->CreateBuffer( &vbdesc, &InitData, &mScreenQuadVB );

            // create twitch viewport
            mVideoStreamerVP[0].TopLeftX=0;
            mVideoStreamerVP[0].TopLeftY=0;
            mVideoStreamerVP[0].Width=(float)mWidth;
            mVideoStreamerVP[0].Height=(float)mHeight;
            mVideoStreamerVP[0].MinDepth=0;
            mVideoStreamerVP[0].MaxDepth=1;

            CComPtr<ID3D11DeviceContext> pContext11;
            pDevice11->GetImmediateContext(&pContext11);
        
            UINT numViewports=1;
            pContext11->RSGetViewports(&numViewports, &mBackupVP[0]);


            // Create the render target texture for twitch
            D3D11_TEXTURE2D_DESC twitchDesc;
            memset( &twitchDesc, 0, sizeof(desc) );

            twitchDesc.Width = mWidth;
            twitchDesc.Height = mHeight;

            twitchDesc.MipLevels = 1;
            twitchDesc.ArraySize = 1;
            twitchDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            twitchDesc.SampleDesc.Count = 1;
            twitchDesc.Usage = D3D11_USAGE_DEFAULT;
            twitchDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            SAFE_RELEASE(mRenderTargetTextureScaled);
            if(FAILED(hr=pDevice11->CreateTexture2D( &twitchDesc, NULL, &mRenderTargetTextureScaled )))
            {
                IGOLogWarn("CreateTexture2D() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }
                        
            // Create the render target view
            D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
            memset(&rtDesc, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
            rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtDesc.Texture2D.MipSlice = 0;
            SAFE_RELEASE(mRenderTargetViewScaled)
            if(FAILED(hr=pDevice11->CreateRenderTargetView( mRenderTargetTextureScaled, &rtDesc, &mRenderTargetViewScaled )))
            {
                IGOLogWarn("CreateRenderTargetView() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            // Create the shader resource view
            D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
            memset(&srDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
            srDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srDesc.Texture2D.MostDetailedMip = 0;
            srDesc.Texture2D.MipLevels = 1;

            SAFE_RELEASE(mShaderResourceViewScaled);
            if(FAILED(hr=pDevice11->CreateShaderResourceView( mRenderTargetTextureScaled, &srDesc, &mShaderResourceViewScaled )))
            {
                IGOLogWarn("CreateShaderResourceView() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

			// not sure if this is necessary (supposed to remove green borders)
			if (mHardwareAccelerated)
			{
				const float ClearColor_Black[4] = {0.0625f, 0.5f, 0.0f, 0.5f}; // YUxV
				pContext11->ClearRenderTargetView(mRenderTargetViewScaled, ClearColor_Black);// dispose hint...
			}

            D3D11_TEXTURE2D_DESC backBufferCopyDesc;
            memcpy(&backBufferCopyDesc, &desc, sizeof(D3D11_TEXTURE2D_DESC));
            //backBufferCopyDesc.ArraySize = 1;
            backBufferCopyDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            backBufferCopyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            backBufferCopyDesc.Usage = D3D11_USAGE_DEFAULT;
            backBufferCopyDesc.SampleDesc.Count = 1;
            backBufferCopyDesc.SampleDesc.Quality = 0;

            // we need a copy of the back buffer, because the default one does not have D3D11_BIND_SHADER_RESOURCE flag and could be using MSAA
            SAFE_RELEASE(mBackBufferCopy);
            if(FAILED(hr=pDevice11->CreateTexture2D( &backBufferCopyDesc, NULL, &mBackBufferCopy )))
            {
                IGOLogWarn("CreateTexture2D() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            memset(&srDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
            srDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srDesc.Texture2D.MostDetailedMip = 0;
            srDesc.Texture2D.MipLevels = 1;

            SAFE_RELEASE(mRenderTargetBBResource);
            if(FAILED(hr=pDevice11->CreateShaderResourceView( mBackBufferCopy, &srDesc, &mRenderTargetBBResource )))
            {
                IGOLogWarn("CreateShaderResourceView() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

        
            UINT supportFormat=0;
            if(FAILED(hr=pDevice11->CheckFormatSupport(desc.Format, &supportFormat)))
            {
                IGOLogWarn("CheckFormatSupport() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
            {
                D3D11_TEXTURE2D_DESC offscreenDesc;
                memset( &offscreenDesc, 0, sizeof(D3D11_TEXTURE2D_DESC) );
                offscreenDesc.Width = mWidth;
                offscreenDesc.Height = mHeight;
                offscreenDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                offscreenDesc.MipLevels = 1;
                offscreenDesc.ArraySize = 1;
                offscreenDesc.SampleDesc.Count = 1;
                offscreenDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                offscreenDesc.Usage = D3D11_USAGE_STAGING;
                offscreenDesc.BindFlags = 0;

                if(FAILED(pDevice11->CreateTexture2D( &offscreenDesc, NULL, &(mBackBufferOffscreenCPU[i]))))
                {
                    IGOLogWarn("CreateTexture2D() failed! hr = %x", hr);
                    TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                    ReleaseSurfaces();
                    return false;
                }
            }
            return true;
        }

        return false;
    }


    void ScreenCopyDx11::ReleaseSurfaces()
    {
        if (TwitchManager::IsTTVDLLReady())
        {
            TwitchManager::TTV_PauseVideo();
        }

        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            SAFE_RELEASE(mBackBufferOffscreenCPU[i]);
        }

        mPrevOffscreenBuffer = mCurrentOffscreenBuffer = 0;
        mCapturedBufferFilled = false;

        SAFE_RELEASE(mShaderResourceViewScaled);
        SAFE_RELEASE(mRenderTargetBBResource);
        SAFE_RELEASE(mRenderTargetViewScaled);
        SAFE_RELEASE(mRenderTargetTextureScaled);

        SAFE_RELEASE(mBackBuffer);
        SAFE_RELEASE(mBackBufferCopy);

        SAFE_RELEASE(mScreenQuadVBLayout);
        SAFE_RELEASE(mScreenQuadVB);

        SAFE_RELEASE(mPixelShader);
        SAFE_RELEASE(mVertexShader);
        SAFE_RELEASE(mSamplerState);
        SAFE_RELEASE(mBlendState);
        SAFE_RELEASE(mDepthStencilState);

    }


    bool ScreenCopyDx11::Create(void *pDevice, void *pSwapChain, int widthOverride, int heightOverride)
    {
        bool result = true;

        CComPtr<ID3D11Device> pDevice11 = (ID3D11Device *)pDevice;
        CComPtr<IDXGISwapChain> pSwapChain11 = (IDXGISwapChain *)pSwapChain;

        D3D11_TEXTURE2D_DESC desc;

        pSwapChain11->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&mBackBuffer );
    
        if(mBackBuffer != NULL)
        {
            mBackBuffer->GetDesc(&desc);
            result = CreateSurfaces(pDevice11, desc);
        }

        SAFE_RELEASE(mBackBuffer);
        return result;
    }

    int ScreenCopyDx11::GetObjectCount()
    {
        int objects = 0;
        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            if (mBackBufferOffscreenCPU[i])
            objects++;
        }

        return objects;
    }


    bool ScreenCopyDx11::Reset(void *pDevice)
    {
        return true;
    }
    
    bool ScreenCopyDx11::Lost()
    {
        ReleaseSurfaces();
        return true;
    }

	bool ScreenCopyDx11::Destroy(void *userData)
    {
        ReleaseSurfaces();
        return true;
    }

	typedef struct
	{
		void *src;
		void *dst;
		uint32_t width;
		uint32_t height;
		uint32_t pitch;
	} MultiThreadedSurfaceCopyType;

#define MAX_COPY_THREADS 4
static MultiThreadedSurfaceCopyType gSurfaceCopyInfo[MAX_COPY_THREADS];
static HANDLE gSurfaceCopyThreadHandle[MAX_COPY_THREADS] = { NULL, NULL, NULL, NULL};

static HANDLE gSubmitFrameThreadHandle = NULL;
static TwitchManager::tPixelBuffer *gPixelBuffer = NULL;

static DWORD WINAPI CopySurfaceData8Thread(LPVOID *lpParam)
{
	MultiThreadedSurfaceCopyType *param = (MultiThreadedSurfaceCopyType *) lpParam;

    if (param->pitch == param->width)
    {
        memcpy(param->dst, param->src, param->width*param->height);
    }
    else
    {					
        uint32_t lineWidth = param->width;
        unsigned char *dstPtr = (unsigned char *)param->dst;
        unsigned char *srcPtr = (unsigned char *)param->src;
        for (uint32_t i = 0; i < param->height; i++)
        {
            memcpy(dstPtr, srcPtr, lineWidth);
            dstPtr += lineWidth;
            srcPtr += param->pitch;
        }
    }
	
	return 0;
}

void ThreadedCopySurfaceData8(int thread, void *src, void *dst, uint32_t width, uint32_t height, uint32_t pitch)
{
	gSurfaceCopyInfo[thread].src = src;
	gSurfaceCopyInfo[thread].dst = dst;
	gSurfaceCopyInfo[thread].width = width;
	gSurfaceCopyInfo[thread].height = height;
	gSurfaceCopyInfo[thread].pitch = pitch;

	if (gSurfaceCopyThreadHandle[thread])
	{
		TerminateThread(gSurfaceCopyThreadHandle[thread], 0);
		CloseHandle(gSurfaceCopyThreadHandle[thread]);
		gSurfaceCopyThreadHandle[thread] = NULL;
	}
	gSurfaceCopyThreadHandle[thread] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CopySurfaceData8Thread, &gSurfaceCopyInfo[thread], 0, NULL);
}

void WaitForCopyCompletion()
{
	WaitForMultipleObjects(MAX_COPY_THREADS, gSurfaceCopyThreadHandle, true, 1000);

	for (int i = 0; i < MAX_COPY_THREADS; i++)
	{
		CloseHandle(gSurfaceCopyThreadHandle[i]);
		gSurfaceCopyThreadHandle[i] = NULL;
	}
}

static DWORD WINAPI SubmitFrameThread(LPVOID *lpParam)
{
	if (!(TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning()))
		return 1;

	if (gPixelBuffer)
		TwitchManager::TTV_SubmitVideoFrame(gPixelBuffer->data, gPixelBuffer);

	return 0;
}

    bool ScreenCopyDx11::Render(void *pDevice, void *pSwapChain)
    {
        HRESULT hr = E_FAIL;
        if (pDevice && pSwapChain)
        {
            if (!IGOApplication::instance())
            {
                TwitchManager::TTV_PauseVideo();
                return true;
            }

            //static int pictureNumber = 0;
            static LARGE_INTEGER last_update = { 0, 0 };
            static LARGE_INTEGER freq = { 0, 0 };

            if (freq.QuadPart == 0)
            {
                QueryPerformanceFrequency(&freq);
            }

            LARGE_INTEGER current_update;
            QueryPerformanceCounter(&current_update);

            // Feed the encoder with 1.25x the frame rate to prevent stuttering!
            if (TwitchManager::GetUsedPixelBufferCount() > 16 /*too many unencoded frames 1920*1080 -> 16 frames ~ 125MB*/ || ((current_update.QuadPart - last_update.QuadPart) * (mFPS*1.25f)) / freq.QuadPart == 0)
                return true;

            last_update = current_update;

            mCurrentOffscreenBuffer++;
            if (mCurrentOffscreenBuffer >= NUM_OFFSCREEN_BUFFERS)
                mCurrentOffscreenBuffer = 0;

            mPrevOffscreenBuffer++;
            // we reached the last frame in our buffer, now start the actual daa transfer into the offscreen buffer
            // cur.  osb   0 1 2 0 1 2 0 1 2 ...
            // prev. osb   - - 0 1 2 0 1 ...
            if (mCurrentOffscreenBuffer == (NUM_OFFSCREEN_BUFFERS - 1))
            {
                mPrevOffscreenBuffer = 0;
                mCapturedBufferFilled = true;
            }

            CComPtr<ID3D11Device> pDevice11 = (ID3D11Device *)pDevice;
            CComPtr<IDXGISwapChain> pSwapChain11 = (IDXGISwapChain *)pSwapChain;
            CComPtr<ID3D11DeviceContext> pContext11;

            pDevice11->GetImmediateContext(&pContext11);
            pSwapChain11->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&mBackBuffer);
            if (mBackBuffer)
            {
                D3D11_TEXTURE2D_DESC desc;
                mBackBuffer->GetDesc(&desc);

                if (desc.SampleDesc.Count > 1)  // MSAA render tagets have to be resolved
                    pContext11->ResolveSubresource(mBackBufferCopy, D3D11CalcSubresource(0, 0, 1), mBackBuffer, D3D11CalcSubresource(0, 0, 1), DXGI_FORMAT_R8G8B8A8_UNORM);
                else
                    pContext11->CopyResource(mBackBufferCopy, mBackBuffer);

                SAFE_RELEASE(mBackBuffer);
            }

            UINT backupSampleMask = 0;
            ID3D11RasterizerState* backupRasterizerState = NULL;
            ID3D11PixelShader *backupPixelShader = NULL;
            ID3D11VertexShader *backupVertexShader = NULL;
            ID3D11SamplerState *backupSamplerState = NULL;
            ID3D11BlendState *backupBlendState = NULL;
            ID3D11DepthStencilState *backupDepthStencilState = NULL;
            FLOAT backupBlendFactor[4] = { 0, 0, 0, 0 };
            UINT backupStencilRef = 0;
            ID3D11ShaderResourceView *backupShaderResourceView[1] = { 0 };
            D3D11_PRIMITIVE_TOPOLOGY backupTopology;
            UINT backupStride = 0;
            UINT backupOffset = 0;
            ID3D11Buffer *backupBuffers[1] = { 0 };
            ID3D11InputLayout *backupInputLayout = NULL;
            ID3D11RenderTargetView *backupRenderTargetView = NULL;
            ID3D11DepthStencilView *backupDepthStencilView = NULL;


            // backup context states
            pContext11->PSGetShader(&backupPixelShader, NULL, NULL);
            pContext11->VSGetShader(&backupVertexShader, NULL, NULL);
            pContext11->PSGetSamplers(0, 1, &backupSamplerState);
            pContext11->OMGetBlendState(&backupBlendState, backupBlendFactor, &backupSampleMask);
            pContext11->OMGetDepthStencilState(&backupDepthStencilState, &backupStencilRef);
            pContext11->RSGetState(&backupRasterizerState);
            pContext11->PSGetShaderResources(0, 1, backupShaderResourceView);
            pContext11->IAGetPrimitiveTopology(&backupTopology);
            pContext11->IAGetVertexBuffers(0, 1, backupBuffers, &backupStride, &backupOffset);
            pContext11->IAGetInputLayout(&backupInputLayout);
            pContext11->OMGetRenderTargets(1, &backupRenderTargetView, &backupDepthStencilView);

            // setup context state
            FLOAT BlendFactor[4] = { 0 };
            UINT numViewports = 1;
            UINT stride = sizeof(QuadVertex);
            UINT offset = 0;
            ID3D11Buffer *pBuffers[1] = { mScreenQuadVB };

            pContext11->RSSetViewports(numViewports, &mVideoStreamerVP[0]);
            pContext11->OMSetRenderTargets(1, &mRenderTargetViewScaled, NULL);
            pContext11->IASetInputLayout(mScreenQuadVBLayout);
            pContext11->PSSetShaderResources(0, 1, &mRenderTargetBBResource);
            pContext11->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);
            pContext11->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            pContext11->OMSetDepthStencilState(mDepthStencilState, 0);
            pContext11->OMSetBlendState(mBlendState, BlendFactor, 0xffffffff);
            pContext11->VSSetShader(mVertexShader, NULL, 0);
            pContext11->PSSetShader(mPixelShader, NULL, 0);
            pContext11->PSSetSamplers(0, 1, &mSamplerState);
            pContext11->RSSetState(NULL);

            // render our quad
            pContext11->Draw(4, 0);

            // restore original states
            pContext11->IASetInputLayout(backupInputLayout);
            pContext11->IASetVertexBuffers(0, 1, backupBuffers, &backupStride, &backupOffset);
            pContext11->IASetPrimitiveTopology(backupTopology);
            pContext11->PSSetShaderResources(0, 1, backupShaderResourceView);
            pContext11->OMSetRenderTargets(1, &backupRenderTargetView, backupDepthStencilView);
            pContext11->RSSetViewports(numViewports, &mBackupVP[0]);
            pContext11->OMSetDepthStencilState(backupDepthStencilState, backupStencilRef);
            pContext11->OMSetBlendState(backupBlendState, backupBlendFactor, backupSampleMask);
            pContext11->VSSetShader(backupVertexShader, NULL, 0);
            pContext11->PSSetShader(backupPixelShader, NULL, 0);
            pContext11->PSSetSamplers(0, 1, &backupSamplerState);
            pContext11->RSSetState(backupRasterizerState);

            SAFE_RELEASE(backupInputLayout);
            SAFE_RELEASE(backupBuffers[0]);
            SAFE_RELEASE(backupShaderResourceView[0]);
            SAFE_RELEASE(backupDepthStencilView);
            SAFE_RELEASE(backupRenderTargetView);
            SAFE_RELEASE(backupRasterizerState);
            SAFE_RELEASE(backupPixelShader);
            SAFE_RELEASE(backupVertexShader);
            SAFE_RELEASE(backupSamplerState);
            SAFE_RELEASE(backupBlendState);
            SAFE_RELEASE(backupDepthStencilState);

            // copy texture data to offscreen buffer
            pContext11->CopyResource((mBackBufferOffscreenCPU[mCurrentOffscreenBuffer]), mRenderTargetTextureScaled);

            if (mHardwareAccelerated)
            {
                const float ClearColor_Black[4] = { 0.0625f, 0.5f, 0.0f, 0.5f }; // YUxV		
                pContext11->ClearRenderTargetView(mRenderTargetViewScaled, ClearColor_Black);// for YUV to black screen (no green bars)
            }
            else
            {
                const float ClearColor_Black[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // RGBA
                pContext11->ClearRenderTargetView(mRenderTargetViewScaled, ClearColor_Black);// dispose hint...
            }

            // reset viewport
            pContext11->RSSetViewports(numViewports, &mBackupVP[0]);

            // copy offscreen buffer to twitch
            D3D11_MAPPED_SUBRESOURCE mappedTex2D = { 0 };

            if (mCapturedBufferFilled && SUCCEEDED(hr = pContext11->Map(mBackBufferOffscreenCPU[mPrevOffscreenBuffer], 0, D3D11_MAP_READ, 0, &mappedTex2D)))
            {
                TwitchManager::tPixelBuffer *pixelBuffer = TwitchManager::GetUnusedPixelBuffer();
                if (pixelBuffer)
                {
#if 0 // multi-thread
                    unsigned char *data = pixelBuffer->data;
                    unsigned char *src_data = (unsigned char *) mappedTex2D.pData;
                    int height_slice = mHeight/4;
                    int height_remaining = mHeight;
                    ThreadedCopySurfaceData8(0, src_data, data, mWidth, height_slice, mappedTex2D.RowPitch);
                    src_data += height_slice * mappedTex2D.RowPitch;
                    data += mWidth * height_slice;
                    height_remaining -= height_slice;

                    ThreadedCopySurfaceData8(1, src_data, data, mWidth, height_slice, mappedTex2D.RowPitch);
                    src_data += height_slice * mappedTex2D.RowPitch;
                    data += mWidth * height_slice;
                    height_remaining -= height_slice;
                    ThreadedCopySurfaceData8(2, src_data, data, mWidth, height_slice, mappedTex2D.RowPitch);
                    src_data += height_slice * mappedTex2D.RowPitch;
                    data += mWidth * height_slice;
                    height_remaining -= height_slice;
                    ThreadedCopySurfaceData8(3, src_data, data, mWidth, height_remaining, mappedTex2D.RowPitch);

                    WaitForCopyCompletion();
#else
                    CopySurfaceData(mappedTex2D.pData, pixelBuffer->data, mWidth, mHeight, mappedTex2D.RowPitch);
#endif
#if 0
                    gPixelBuffer = pixelBuffer;
                    if (gSubmitFrameThreadHandle)
                    {
                        DWORD exitCode = 0;

                        DWORD waitResult = ::WaitForSingleObject(gSubmitFrameThreadHandle, 1000);
                        if (waitResult == WAIT_OBJECT_0)
                        {
                            if (GetExitCodeThread(gSubmitFrameThreadHandle, &exitCode))
                            {
                                if (exitCode != STILL_ACTIVE)
                                {
                                    CloseHandle(gSubmitFrameThreadHandle);
                                    gSubmitFrameThreadHandle = NULL;
                                }
                            }
                        }
                        else
                        {
                            OriginIGO::IGOLogWarn("gSubmitFrameThreadHandle failed to terminate normally...");
                            TerminateThread(gSubmitFrameThreadHandle, 0);
                            CloseHandle(gSubmitFrameThreadHandle);
                            gSubmitFrameThreadHandle = NULL;
                        }
                    }

                    if (gSubmitFrameThreadHandle == NULL)
                        gSubmitFrameThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SubmitFrameThread, gInstDLL, 0, NULL);
#else
                    TwitchManager::TTV_SubmitVideoFrame(pixelBuffer->data, pixelBuffer);
#endif
                    /*
                    #pragma comment(lib, "C:\\DEV\\EBISU\\global_online\\origin\\DL\\OriginLegacyApp\\dev\\source\\IGO\\Microsoft DirectX 9+10 SDK\\Lib\\x86\\d3dx11d.lib")
                    eastl::string16 filename = L"c:\\temp\\test.";
                    eastl::string16 filenameExt = L".png";
                    eastl::string16 picFile;
                    wchar_t tmpBuff[16] = {0};
                    _itow_s(pictureNumber, tmpBuff, 16, 10);
                    pictureNumber++;

                    picFile = filename + tmpBuff + filenameExt;

                    hr=D3DX11SaveTextureToFile(pContext11, mBackBufferOffscreenCPU[mCurrentOffscreenBuffer], D3DX11_IFF_PNG, picFile.c_str() );
                    */
                    //TwitchManager::MarkUnusedPixelBuffer(pixelBuffer);
                }
                pContext11->Unmap(mBackBufferOffscreenCPU[mPrevOffscreenBuffer], 0);
                return true;
            }
            else
                if (!mCapturedBufferFilled)
                    return true;
        }
        TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
        ReleaseSurfaces();
        return false;
    }

    bool ScreenCopyDx11::CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth)
    {
    
        return true;
    }

    bool ScreenCopyDx11::CopyFromMemory(void *pDevice, void *pSource, int width, int heigth)
    {
        return false;
    }
#pragma endregion
}

