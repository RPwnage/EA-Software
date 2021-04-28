///////////////////////////////////////////////////////////////////////////////
// ScreenCopyDX10.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ScreenCopyDx10.h"
#include "IGO.h"
#include "resource.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "twitchmanager.h"
#include "IGOLogger.h"
#include "IGOApplication.h"

namespace OriginIGO {

    extern HMODULE gInstDLL;

    ScreenCopyDx10::ScreenCopyDx10() :
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
        //MessageBox(NULL, L"Twitch ScreenCopyDX10", L"breakpoint", MB_OK);

        for (int i=0; i<NUM_OFFSCREEN_BUFFERS; i++)
        {
            mBackBufferOffscreenCPU[i] = NULL;
        }

        mShaderResourceViewScaled = NULL;
        mRenderTargetBBResource = NULL;
        mRenderTargetViewScaled = NULL;
        mRenderTargetTextureScaled = NULL;

        mEffect = NULL;
        mTechnique = NULL;
        mBackBuffer = NULL;
        mBackBufferCopy = NULL;

        mScreenQuadVBLayout = NULL;
        mScreenQuadVB = NULL;

        mTextureVariable = NULL;

    }

    ScreenCopyDx10::~ScreenCopyDx10()
    {
    }

    #pragma region IScreenCopy Implementation

    bool ScreenCopyDx10::isSRGB_format(DXGI_FORMAT f)
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

    bool ScreenCopyDx10::CreateSurfaces(CComPtr<ID3D10Device> &pDevice10, const D3D10_TEXTURE2D_DESC &desc)
    {   
        ReleaseSurfaces();
    
        mSourceWidth = desc.Width;
        mSourceHeight = desc.Height;

        if (TTV_SUCCEEDED(TwitchManager::InitializeForStreaming(desc.Width, desc.Height, mWidth, mHeight, mFPS)))
        {
            // create our shader effect
	        HRESULT hr;

     
            // Load Compiled Effect from DLL resource
            IGOLogInfo("Loading igo.fxo file");
            HRSRC hRes = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOFX), RT_RCDATA);
            IGO_ASSERT(hRes != NULL);
            const char * pIGOFx;
            DWORD dwIGOFxSize = 0;
            HGLOBAL hIGOFx = LoadResource(gInstDLL, hRes);
            pIGOFx = (const char *)LockResource(hIGOFx);
            dwIGOFxSize = SizeofResource(gInstDLL, hRes);

            // if any of these fail, the resouce is not properly compiled into the dll
            IGO_ASSERT(hIGOFx != NULL);
            IGO_ASSERT(pIGOFx != NULL);
            IGO_ASSERT(dwIGOFxSize > 0);

            // Create Effect
            IGOLogInfo("Creating effect from igostream.fxo");
            HMODULE hD3D10 = GetModuleHandle(L"d3d10.dll");
            typedef HRESULT (WINAPI *LPD3D10CREATEEFFECTFROMMEMORY)(LPCVOID pData, SIZE_T DataLength, UINT FXFlags, ID3D10Device *pDevice, ID3D10EffectPool *pEffectPool, ID3D10Effect **ppEffect);
            LPD3D10CREATEEFFECTFROMMEMORY s_D3D10CreateEffectFromMemory = (LPD3D10CREATEEFFECTFROMMEMORY)GetProcAddress(hD3D10, "D3D10CreateEffectFromMemory");
            hr = s_D3D10CreateEffectFromMemory(pIGOFx, dwIGOFxSize, 0, pDevice10, NULL, &mEffect);
     
            if( FAILED(hr) || !mEffect )
            {
                IGO_TRACEF("D3DX10CreateEffectFromMemory failed. hr =  hr = %x", hr);
                IGOLogWarn("D3DX10CreateEffectFromMemory failed. hr =  hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }


	        // Obtain the technique
			if (mHardwareAccelerated)
				mTechnique = mEffect->GetTechniqueByName( "RenderQuad_YUV" );
			else
				mTechnique = mEffect->GetTechniqueByName( "RenderQuad" );
	        IGO_ASSERT(mTechnique);
	        mTextureVariable = mEffect->GetVariableByName( "Texture" )->AsShaderResource();
	        IGO_ASSERT(mTextureVariable);

            // Create our quad input layout
            const D3D10_INPUT_ELEMENT_DESC layout[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0 },
            };

            UINT numElements = sizeof(layout)/sizeof(D3D10_INPUT_ELEMENT_DESC);

            // Create the input layout
            D3D10_PASS_DESC PassDesc;
            mTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
            SAFE_RELEASE(mScreenQuadVBLayout);
            if( FAILED( hr=pDevice10->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &mScreenQuadVBLayout ) ) )
            {
                IGO_TRACEF("CreateInputLayout failed. hr =  hr = %x", hr);
                IGOLogWarn("CreateInputLayout failed. hr =  hr = %x", hr);
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
            
            D3D10_BUFFER_DESC vbdesc =
            {
            4*sizeof(QuadVertex),
            D3D10_USAGE_IMMUTABLE,
            D3D10_BIND_VERTEX_BUFFER,
            0,
            0
            };
    
            SAFE_RELEASE(mScreenQuadVB);

            D3D10_SUBRESOURCE_DATA InitData;
            InitData.pSysMem = svQuad;
            InitData.SysMemPitch = 0;
            InitData.SysMemSlicePitch = 0;
            pDevice10->CreateBuffer( &vbdesc, &InitData, &mScreenQuadVB );

            // create twitch viewport
            mVideoStreamerVP[0].TopLeftX=0;
            mVideoStreamerVP[0].TopLeftY=0;
            mVideoStreamerVP[0].Width=mWidth;
            mVideoStreamerVP[0].Height=mHeight;
            mVideoStreamerVP[0].MinDepth=0;
            mVideoStreamerVP[0].MaxDepth=1;

            UINT numViewports=1;
            pDevice10->RSGetViewports(&numViewports, &mBackupVP[0]);


            // Create the render target texture for twitch
            D3D10_TEXTURE2D_DESC twitchDesc;
            memset( &twitchDesc, 0, sizeof(desc) );

            twitchDesc.Width = mWidth;
            twitchDesc.Height = mHeight;

            twitchDesc.MipLevels = 1;
            twitchDesc.ArraySize = 1;
            twitchDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            twitchDesc.SampleDesc.Count = 1;
            twitchDesc.Usage = D3D10_USAGE_DEFAULT;
            twitchDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;

            SAFE_RELEASE(mRenderTargetTextureScaled);
            if(FAILED(hr=pDevice10->CreateTexture2D( &twitchDesc, NULL, &mRenderTargetTextureScaled )))
            {
                IGOLogWarn("CreateTexture2D() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }
                        
            // Create the render target view
            D3D10_RENDER_TARGET_VIEW_DESC rtDesc;
            memset(&rtDesc, 0, sizeof(D3D10_RENDER_TARGET_VIEW_DESC));
            rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
            rtDesc.Texture2D.MipSlice = 0;
            SAFE_RELEASE(mRenderTargetViewScaled)
            if(FAILED(hr=pDevice10->CreateRenderTargetView( mRenderTargetTextureScaled, &rtDesc, &mRenderTargetViewScaled )))
            {
                IGOLogWarn("CreateRenderTargetView() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            // Create the shader resource view
            D3D10_SHADER_RESOURCE_VIEW_DESC srDesc;
            memset(&srDesc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
            srDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
            srDesc.Texture2D.MostDetailedMip = 0;
            srDesc.Texture2D.MipLevels = 1;

            SAFE_RELEASE(mShaderResourceViewScaled);
            if(FAILED(hr=pDevice10->CreateShaderResourceView( mRenderTargetTextureScaled, &srDesc, &mShaderResourceViewScaled )))
            {
                IGOLogWarn("CreateShaderResourceView() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            D3D10_TEXTURE2D_DESC backBufferCopyDesc;
            memcpy(&backBufferCopyDesc, &desc, sizeof(D3D10_TEXTURE2D_DESC));
            //backBufferCopyDesc.ArraySize = 1;
            backBufferCopyDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            backBufferCopyDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
            backBufferCopyDesc.Usage = D3D10_USAGE_DEFAULT;
            backBufferCopyDesc.SampleDesc.Count = 1;
            backBufferCopyDesc.SampleDesc.Quality = 0;

            // we need a copy of the back buffer, because the default one does not have D3D10_BIND_SHADER_RESOURCE flag and could be using MSAA
            SAFE_RELEASE(mBackBufferCopy);
            if(FAILED(hr=pDevice10->CreateTexture2D( &backBufferCopyDesc, NULL, &mBackBufferCopy )))
            {
                IGOLogWarn("CreateTexture2D() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            memset(&srDesc, 0, sizeof(D3D10_SHADER_RESOURCE_VIEW_DESC));
            srDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
            srDesc.Texture2D.MostDetailedMip = 0;
            srDesc.Texture2D.MipLevels = 1;

            SAFE_RELEASE(mRenderTargetBBResource);
            if(FAILED(hr=pDevice10->CreateShaderResourceView( mBackBufferCopy, &srDesc, &mRenderTargetBBResource )))
            {
                IGOLogWarn("CreateShaderResourceView() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

        
            UINT supportFormat=0;
            if(FAILED(hr=pDevice10->CheckFormatSupport(desc.Format, &supportFormat)))
            {
                IGOLogWarn("CheckFormatSupport() failed! hr = %x", hr);
                TwitchManager::SetBroadcastError(hr, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
                ReleaseSurfaces();
                return false;
            }

            for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
            {
                D3D10_TEXTURE2D_DESC offscreenDesc;
                memset( &offscreenDesc, 0, sizeof(D3D10_TEXTURE2D_DESC) );
                offscreenDesc.Width = mWidth;
                offscreenDesc.Height = mHeight;
                offscreenDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                offscreenDesc.MipLevels = 1;
                offscreenDesc.ArraySize = 1;
                offscreenDesc.SampleDesc.Count = 1;
                offscreenDesc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
                offscreenDesc.Usage = D3D10_USAGE_STAGING;
                offscreenDesc.BindFlags = 0;
                if(FAILED(hr=pDevice10->CreateTexture2D( &offscreenDesc, NULL, &(mBackBufferOffscreenCPU[i]))))
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


    void ScreenCopyDx10::ReleaseSurfaces()
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

        SAFE_RELEASE(mEffect);
        SAFE_RELEASE(mBackBuffer);
        SAFE_RELEASE(mBackBufferCopy);

        SAFE_RELEASE(mScreenQuadVBLayout);
        SAFE_RELEASE(mScreenQuadVB);

    }


    bool ScreenCopyDx10::Create(void *pDevice, void *pSwapChain, int widthOverride, int heightOverride)
    {
        bool result = true;

        CComPtr<ID3D10Device> pDevice10 = (ID3D10Device *)pDevice;
        CComPtr<IDXGISwapChain> pSwapChain10 = (IDXGISwapChain *)pSwapChain;

        D3D10_TEXTURE2D_DESC desc;

        pSwapChain10->GetBuffer( 0, __uuidof( ID3D10Texture2D ), (LPVOID*)&mBackBuffer );
    
        if(mBackBuffer != NULL)
        {
            mBackBuffer->GetDesc(&desc);
            result = CreateSurfaces(pDevice10, desc);
        }

        SAFE_RELEASE(mBackBuffer);
        return result;
    }

    int ScreenCopyDx10::GetObjectCount()
    {
        int objects = 0;
        for (int i = 0; i< NUM_OFFSCREEN_BUFFERS; i++)
        {
            if (mBackBufferOffscreenCPU[i])
            objects++;
        }

        return objects;
    }


    bool ScreenCopyDx10::Reset(void *pDevice)
    {
        return true;
    }
    
    bool ScreenCopyDx10::Lost()
    {
        ReleaseSurfaces();
        return true;
    }

	bool ScreenCopyDx10::Destroy(void *userData)
    {
        ReleaseSurfaces();
        return true;
    }

    bool ScreenCopyDx10::Render(void *pDevice, void *pSwapChain)
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

            CComPtr<ID3D10Device> pDevice10 = (ID3D10Device *)pDevice;
            CComPtr<IDXGISwapChain> pSwapChain10 = (IDXGISwapChain *)pSwapChain;

            pSwapChain10->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&mBackBuffer);
            if (mBackBuffer)
            {
                D3D10_TEXTURE2D_DESC desc;
                mBackBuffer->GetDesc(&desc);

                if (desc.SampleDesc.Count > 1)  // MSAA render tagets have to be resolved
                    pDevice10->ResolveSubresource(mBackBufferCopy, D3D10CalcSubresource(0, 0, 1), mBackBuffer, D3D10CalcSubresource(0, 0, 1), DXGI_FORMAT_R8G8B8A8_UNORM);//desc.Format);
                else
                    pDevice10->CopyResource(mBackBufferCopy, mBackBuffer);

                SAFE_RELEASE(mBackBuffer);
            }

            // backup context states
            ID3D10RenderTargetView *backupRenderTargetView = NULL;
            ID3D10DepthStencilView *backupDepthStencilView = NULL;
            ID3D10InputLayout *backupInputLayout = NULL;
            UINT backupStride = 0;
            UINT backupOffset = 0;
            ID3D10Buffer *backupBuffers[1] = { 0 };
            D3D10_PRIMITIVE_TOPOLOGY backupTopology;
            ID3D10ShaderResourceView *backupShaderResourceView[1] = { 0 };

            pDevice10->PSGetShaderResources(0, 1, backupShaderResourceView);
            pDevice10->IAGetPrimitiveTopology(&backupTopology);
            pDevice10->IAGetVertexBuffers(0, 1, backupBuffers, &backupStride, &backupOffset);
            pDevice10->IAGetInputLayout(&backupInputLayout);
            pDevice10->OMGetRenderTargets(1, &backupRenderTargetView, &backupDepthStencilView);


            // set context states
            UINT stride = sizeof(QuadVertex);
            UINT offset = 0;
            ID3D10Buffer *pBuffers[1] = { mScreenQuadVB };
            UINT numViewports = 1;

            pDevice10->RSSetViewports(numViewports, &mVideoStreamerVP[0]);
            pDevice10->OMSetRenderTargets(1, &mRenderTargetViewScaled, NULL);
            pDevice10->IASetInputLayout(mScreenQuadVBLayout);
            pDevice10->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);
            pDevice10->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            mTextureVariable->SetResource(mRenderTargetBBResource);

            // render our quad
            D3D10_TECHNIQUE_DESC techDesc;
            mTechnique->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {
                mTechnique->GetPassByIndex(p)->Apply(0);
                pDevice10->Draw(4, 0);
            }

            if (mTextureVariable != NULL)
            {
                ID3D10ShaderResourceView *const pSRV = NULL;
                mTextureVariable->SetResource(pSRV);

                ID3D10ShaderResourceView *const pSRV2[1] = { NULL };
                pDevice10->PSSetShaderResources(0, 1, pSRV2);
            }

            // restore original states

            pDevice10->IASetInputLayout(backupInputLayout);
            pDevice10->IASetVertexBuffers(0, 1, backupBuffers, &backupStride, &backupOffset);
            pDevice10->IASetPrimitiveTopology(backupTopology);
            pDevice10->PSSetShaderResources(0, 1, backupShaderResourceView);
            pDevice10->OMSetRenderTargets(1, &backupRenderTargetView, backupDepthStencilView);
            pDevice10->RSSetViewports(numViewports, &mBackupVP[0]);

            SAFE_RELEASE(backupDepthStencilView);
            SAFE_RELEASE(backupRenderTargetView);
            SAFE_RELEASE(backupInputLayout);
            SAFE_RELEASE(backupBuffers[0]);
            SAFE_RELEASE(backupShaderResourceView[0]);

            // copy texture data to offscreen buffer
            pDevice10->CopyResource((mBackBufferOffscreenCPU[mCurrentOffscreenBuffer]), mRenderTargetTextureScaled);

            if (mHardwareAccelerated)
            {
                const float ClearColor_Black[4] = { 0.0625f, 0.5f, 0.0f, 0.5f }; // YUV black
                pDevice10->ClearRenderTargetView(mRenderTargetViewScaled, ClearColor_Black);// for YUV to black screen (no green bars)
            }
            else
            {
                const float ClearColor_Black[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // RGBA
                pDevice10->ClearRenderTargetView(mRenderTargetViewScaled, ClearColor_Black);// dispose hint...
            }

            // reset viewport
            pDevice10->RSSetViewports(numViewports, &mBackupVP[0]);

            // copy offscreen buffer to twitch
            D3D10_MAPPED_TEXTURE2D mappedTex2D = { 0 };

            if (mCapturedBufferFilled && SUCCEEDED(hr = mBackBufferOffscreenCPU[mPrevOffscreenBuffer]->Map(0, D3D10_MAP_READ, 0, &mappedTex2D)))
            {
                TwitchManager::tPixelBuffer *pixelBuffer = TwitchManager::GetUnusedPixelBuffer();
                if (pixelBuffer)
                {
                    CopySurfaceData(mappedTex2D.pData, pixelBuffer->data, mWidth, mHeight, mappedTex2D.RowPitch);
                    TwitchManager::TTV_SubmitVideoFrame(pixelBuffer->data, pixelBuffer);

                    /*
                    #pragma comment(lib, "C:\\DEV\\EBISU\\global_online\\origin\\DL\\OriginLegacyApp\\dev\\source\\IGO\\Microsoft DirectX 9+10 SDK\\Lib\\x86\\d3dx10.lib")
                    eastl::string16 filename = L"c:\\temp\\test.";
                    eastl::string16 filenameExt = L".png";
                    eastl::string16 picFile;
                    wchar_t tmpBuff[16] = {0};
                    _itow_s(pictureNumber, tmpBuff, 16, 10);
                    pictureNumber++;

                    picFile = filename + tmpBuff + filenameExt;

                    hr=D3DX10SaveTextureToFile(mBackBufferOffscreenCPU[mCurrentOffscreenBuffer], D3DX10_IFF_PNG, picFile.c_str() );
                    */
                    //TwitchManager::MarkUnusedPixelBuffer(pixelBuffer);
                }
                mBackBufferOffscreenCPU[mPrevOffscreenBuffer]->Unmap(0);
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

    bool ScreenCopyDx10::CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth)
    {
    
        return true;
    }

    bool ScreenCopyDx10::CopyFromMemory(void *pDevice, void *pSource, int width, int heigth)
    {
        return false;
    }
#pragma endregion
}

