///////////////////////////////////////////////////////////////////////////////
// DX11Hook.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "DX11Hook.h"
#include "DX11Surface.h"
#include "DX10Hook.h"

#if defined(ORIGIN_PC)

#include <windows.h>
#include <d3d11.h>
#include <d3dtypes.h>

#include "HookAPI.h"
#include "IGOApplication.h"
#include "resource.h"

#include "EASTL/hash_map.h"
#include "IGOTrace.h"
#include "InputHook.h"
#include "IGOLogger.h"

#include "ScreenCopyDx11.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

namespace OriginIGO {

    ID3D11PixelShader *gPixelShader = NULL;
    ID3D11VertexShader *gVertexShader = NULL;
    ID3D11SamplerState *gSamplerState = NULL;
    ID3D11BlendState *gBlendState = NULL;
    ID3D11DepthStencilState *gDepthStencilState = NULL;

    // IGO background
    ID3D11PixelShader *gBackgroundPixelShader = NULL;
    ID3D11VertexShader *gBackgroundVertexShader = NULL;

    // saved state

    UINT gSavedSampleMask = 0;
    ID3D11Buffer *gSavedConstantBuffer[5] = { 0 };
    ID3D11RasterizerState* gSavedRasterizerState = NULL;
    ID3D11InputLayout* gSavedInputLayout = NULL;
    ID3D11ShaderResourceView* gSavedShaderResourceView = NULL;
    ID3D11Buffer* gSavedVertexBuffer = NULL;
    UINT gSavedVertexBufferStrides = 0;
    UINT gSavedVertexBufferOffsets = 0;
    D3D11_PRIMITIVE_TOPOLOGY gSavedTopology;
    D3D11_VIEWPORT gSavedVP = { 0 };
    ID3D11RenderTargetView *gBackBufferRenderTargetView = NULL;
    ID3D11RenderTargetView *gSavedRenderTargetView = NULL;
    ID3D11DepthStencilView *gSavedDepthStencilView = NULL;
    ID3D11ClassInstance* gSavedPixelShaderClasses = NULL;
    ID3D11ClassInstance* gSavedVertexShaderClasses = NULL;
    UINT gNumSavedPixelShaderClasses = 0;
    UINT gNumSavedVertexShaderClasses = 0;
    ID3D11PixelShader *gSavedPixelShader = NULL;
    ID3D11VertexShader *gSavedVertexShader = NULL;
    ID3D11SamplerState *gSavedSamplerState = NULL;
    ID3D11BlendState *gSavedBlendState = NULL;
    ID3D11DepthStencilState *gSavedDepthStencilState = NULL;
    FLOAT gBlendFactor[4] = { 0, 0, 0, 0 };
    UINT gSampleMask = 0;
    UINT gStencilRef = 0;

    ID3D11Buffer *gCBNeverChanges = NULL;
    ID3D11Buffer *gCBChangeOnResize = NULL;
    ID3D11Buffer *gCBChangesEveryFrame = NULL;

    ID3D11InputLayout* gVertexLayout = NULL;
    EA::Thread::Futex DX11Hook::mInstanceHookMutex;
    HANDLE DX11Hook::mIGODX11HookDestroyThreadHandle = NULL;

    extern D3DMATRIX gMatOrtho;
    extern D3DMATRIX gMatWorld;
    DWORD DX11Hook::mLastHookTime = 0;
    extern DWORD gPresentHookThreadId;
    extern volatile DWORD gPresentHookCalled;
    IDXGISwapChain* gDX11RenderingSwapChain = NULL;
    extern volatile bool gTestCooperativeLevelHook_or_ResetHook_Called;

    extern HINSTANCE gInstDLL;
    extern bool gInputHooked;
    extern volatile bool gQuitting;

    void CreateScreenCopyDX11(ID3D11Device *pDev, IDXGISwapChain *pSwapChain)
    {
        if (gScreenCopy == NULL)
        {
            gScreenCopy = new ScreenCopyDx11();
            gScreenCopy->Create(pDev, pSwapChain);
        }
    }

    void DestroyScreenCopyDX11()
    {
        if (gScreenCopy)
        {
            gScreenCopy->Destroy();
            delete gScreenCopy;
            gScreenCopy = NULL;
        }
    }

    void buildProjectionMatrixDX11(int width, int height, int n, int f)
    {
        ZeroMemory(&gMatOrtho, sizeof(gMatOrtho));

        gMatOrtho._11 = 2.0f / (width == 0 ? 1 : width);
        gMatOrtho._22 = 2.0f / (height == 0 ? 1 : height);
        gMatOrtho._33 = -2.0f / (n - f);
        gMatOrtho._34 = 2.0f / (n - f);
        gMatOrtho._44 = 1.0f;
    }

    bool saveRenderState(ID3D11Device *pDev, IDXGISwapChain *pSwapChain)
    {
        ID3D11DeviceContext *pDevContext = NULL;
        ID3D11Texture2D *pBackBuffer = NULL;

        pDev->GetImmediateContext(&pDevContext);

        if (pDevContext)
        {
            // save the render state
            pDevContext->PSGetShader(&gSavedPixelShader, &gSavedPixelShaderClasses, &gNumSavedPixelShaderClasses);
            pDevContext->VSGetShader(&gSavedVertexShader, &gSavedVertexShaderClasses, &gNumSavedVertexShaderClasses);

            pDevContext->VSGetConstantBuffers(0, 1, &(gSavedConstantBuffer[0]));
            pDevContext->VSGetConstantBuffers(0, 1, &(gSavedConstantBuffer[1]));
            pDevContext->VSGetConstantBuffers(1, 1, &(gSavedConstantBuffer[2]));
            pDevContext->VSGetConstantBuffers(2, 1, &(gSavedConstantBuffer[3]));
            pDevContext->PSGetConstantBuffers(2, 1, &(gSavedConstantBuffer[4]));

            pDevContext->PSGetSamplers(0, 1, &gSavedSamplerState);
            pDevContext->OMGetBlendState(&gSavedBlendState, gBlendFactor, &gSampleMask);
            pDevContext->OMGetDepthStencilState(&gSavedDepthStencilState, &gStencilRef);

            pDevContext->RSGetState(&gSavedRasterizerState);
            pDevContext->IAGetInputLayout(&gSavedInputLayout);
            pDevContext->PSGetShaderResources(0, 1, &gSavedShaderResourceView);
            pDevContext->IAGetVertexBuffers(0, 1, &gSavedVertexBuffer, &gSavedVertexBufferStrides, &gSavedVertexBufferOffsets);
            pDevContext->IAGetPrimitiveTopology(&gSavedTopology);
            UINT numVp = 1;
            pDevContext->RSGetViewports(&numVp, &gSavedVP);
            pDevContext->OMGetRenderTargets(1, &gSavedRenderTargetView, &gSavedDepthStencilView);

            // setup context state

            pDevContext->IASetInputLayout(gVertexLayout);
            pDevContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

            // setup context state
            FLOAT BlendFactor[4] = { 0 };
            pDevContext->OMSetDepthStencilState(gDepthStencilState, 0);
            pDevContext->OMSetBlendState(gBlendState, BlendFactor, 0xffffffff);
            pDevContext->VSSetShader(gVertexShader, NULL, 0);
            pDevContext->VSSetConstantBuffers(0, 1, &gCBNeverChanges);
            pDevContext->VSSetConstantBuffers(1, 1, &gCBChangesEveryFrame);
            pDevContext->PSSetShader(gPixelShader, NULL, 0);
            pDevContext->PSSetConstantBuffers(1, 1, &gCBChangesEveryFrame);
            pDevContext->PSSetSamplers(0, 1, &gSamplerState);
            pDevContext->RSSetState(NULL);
            // set new viewport
            D3D11_VIEWPORT vp;


            SAFE_CALL_LOCK_AUTO
                ZeroMemory(&vp, sizeof(vp));
            vp.Width = (FLOAT)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
            vp.Height = (FLOAT)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);
            vp.MinDepth = D3D11_MIN_DEPTH;
            vp.MaxDepth = D3D11_MAX_DEPTH;
            if (vp.Height > 0 && vp.Width > 0)
                pDevContext->RSSetViewports(1, &vp);

            // setup the render target view
            D3D11_TEXTURE2D_DESC desc = { 0 };
            HRESULT hr;

            hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            if (SUCCEEDED(hr) && pBackBuffer != NULL)
            {
                pBackBuffer->GetDesc(&desc);

                // create the render target view for our current back buffer
                D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
                memset(&rtDesc, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
                rtDesc.Format = desc.Format;
                rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtDesc.Texture2D.MipSlice = 0;
                if (FAILED(hr = pDev->CreateRenderTargetView(pBackBuffer, &rtDesc, &gBackBufferRenderTargetView)))
                {
                    IGOLogWarn("CreateRenderTargetView() failed! hr = %x", hr);
                }
                else
                {
                    pDevContext->OMSetRenderTargets(1, &gBackBufferRenderTargetView, NULL);
                }

                SAFE_RELEASE(pBackBuffer);
            }
            else
            {
                IGOLogWarn("GetBuffer() failed! hr = %x", hr);
            }

            RELEASE_IF(pDevContext)
        }

        return true;
    }

    void restoreRenderState(ID3D11Device *pDev)
    {
        ID3D11DeviceContext *pDevContext = NULL;
        pDev->GetImmediateContext(&pDevContext);

        if (pDevContext)
        {
            pDevContext->PSSetShader(gSavedPixelShader, &gSavedPixelShaderClasses, gNumSavedPixelShaderClasses);
            pDevContext->VSSetShader(gSavedVertexShader, &gSavedVertexShaderClasses, gNumSavedVertexShaderClasses);

            pDevContext->VSSetConstantBuffers(0, 1, &(gSavedConstantBuffer[0]));
            pDevContext->VSSetConstantBuffers(0, 1, &(gSavedConstantBuffer[1]));
            pDevContext->VSSetConstantBuffers(1, 1, &(gSavedConstantBuffer[2]));
            pDevContext->VSSetConstantBuffers(2, 1, &(gSavedConstantBuffer[3]));
            pDevContext->PSSetConstantBuffers(2, 1, &(gSavedConstantBuffer[4]));


            pDevContext->PSSetSamplers(0, 1, &gSavedSamplerState);
            pDevContext->OMSetBlendState(gSavedBlendState, gBlendFactor, gSampleMask);
            pDevContext->OMSetDepthStencilState(gSavedDepthStencilState, gStencilRef);

            pDevContext->RSSetState(gSavedRasterizerState);
            pDevContext->IASetInputLayout(gSavedInputLayout);
            pDevContext->PSSetShaderResources(0, 1, &gSavedShaderResourceView);
            pDevContext->IASetVertexBuffers(0, 1, &gSavedVertexBuffer, &gSavedVertexBufferStrides, &gSavedVertexBufferOffsets);
            pDevContext->IASetPrimitiveTopology(gSavedTopology);
            pDevContext->RSSetViewports(1, &gSavedVP);
            pDevContext->OMSetRenderTargets(1, &gSavedRenderTargetView, gSavedDepthStencilView);


            RELEASE_IF(gSavedConstantBuffer[0]);
            RELEASE_IF(gSavedConstantBuffer[1]);
            RELEASE_IF(gSavedConstantBuffer[2]);
            RELEASE_IF(gSavedConstantBuffer[3]);
            RELEASE_IF(gSavedConstantBuffer[4]);
            RELEASE_IF(gSavedVertexShaderClasses);
            RELEASE_IF(gSavedPixelShaderClasses);

            RELEASE_IF(gSavedPixelShader);
            RELEASE_IF(gSavedVertexShader);
            RELEASE_IF(gSavedSamplerState);
            RELEASE_IF(gSavedBlendState);

            RELEASE_IF(gSavedDepthStencilState);
            RELEASE_IF(gSavedRasterizerState);
            RELEASE_IF(gSavedBlendState);
            RELEASE_IF(gSavedSamplerState);
            RELEASE_IF(gSavedInputLayout);
            RELEASE_IF(gSavedShaderResourceView);
            RELEASE_IF(gSavedVertexBuffer);
            RELEASE_IF(gBackBufferRenderTargetView);
            RELEASE_IF(gSavedRenderTargetView);
            RELEASE_IF(gSavedDepthStencilView);

            RELEASE_IF(pDevContext)
        }
    }

    void ReleaseD3D11Objects()
    {
        DestroyScreenCopyDX11();

        RELEASE_IF(gSavedConstantBuffer[0]);
        RELEASE_IF(gSavedConstantBuffer[1]);
        RELEASE_IF(gSavedConstantBuffer[2]);
        RELEASE_IF(gSavedConstantBuffer[3]);
        RELEASE_IF(gSavedConstantBuffer[4]);
        RELEASE_IF(gSavedVertexShaderClasses);
        RELEASE_IF(gSavedPixelShaderClasses);

        RELEASE_IF(gSavedPixelShader);
        RELEASE_IF(gSavedVertexShader);
        RELEASE_IF(gSavedSamplerState);
        RELEASE_IF(gSavedBlendState);

        RELEASE_IF(gSavedDepthStencilState);
        RELEASE_IF(gSavedRasterizerState);
        RELEASE_IF(gSavedBlendState);
        RELEASE_IF(gSavedSamplerState);
        RELEASE_IF(gSavedInputLayout);
        RELEASE_IF(gSavedShaderResourceView);
        RELEASE_IF(gSavedVertexBuffer);
        RELEASE_IF(gBackBufferRenderTargetView);
        RELEASE_IF(gSavedRenderTargetView);
        RELEASE_IF(gSavedDepthStencilView);

        RELEASE_IF(gCBNeverChanges);
        RELEASE_IF(gCBChangesEveryFrame);
        RELEASE_IF(gPixelShader);
        RELEASE_IF(gVertexShader);
        RELEASE_IF(gSamplerState);
        RELEASE_IF(gBlendState);
        RELEASE_IF(gDepthStencilState);
        RELEASE_IF(gVertexLayout);
    }

    bool DX11HandleLostDevice(HRESULT hr)
    {
#ifndef DXGI_ERROR_DEVICE_REMOVED
#define DXGI_ERROR_DEVICE_REMOVED 0x887A0005
#endif
        if (hr != DXGI_ERROR_DEVICE_REMOVED)
            return false;

        TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_HARDWARE, TelemetryRenderer_DX11, "Detected TDR scenario");

        ReleaseD3D11Objects();
        IGOApplication::deleteInstance();

        return true;
    }

    bool CreateVertexShaderFromResource(ID3D11Device *pDev, int resourceID, ID3D11VertexShader** shader)
    {
        DWORD shaderCodeSize = 0;
        const char* shaderCode = NULL;
        if (LoadEmbeddedResource(resourceID, &shaderCode, &shaderCodeSize))
        {
            HRESULT hr = pDev->CreateVertexShader(shaderCode, shaderCodeSize, NULL, shader);
            if (SUCCEEDED(hr))
                return true;

            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreateVertexShaderFromResource(%d) failed. hr = %x", resourceID, hr));
            IGO_ASSERT(0);
        }

        return false;
    }

    bool CreatePixelShaderFromResource(ID3D11Device *pDev, int resourceID, ID3D11PixelShader** shader)
    {
        DWORD shaderCodeSize = 0;
        const char* shaderCode = NULL;
        if (LoadEmbeddedResource(resourceID, &shaderCode, &shaderCodeSize))
        {
            HRESULT hr = pDev->CreatePixelShader(shaderCode, shaderCodeSize, NULL, shader);
            if (SUCCEEDED(hr))
                return true;

            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreatePixelShaderFromResource(%d) failed. hr = %x", resourceID, hr));
            IGO_ASSERT(0);
        }

        return false;
    }
        
    void Init(ID3D11Device *pDev)
    {

        HRESULT hr;

        if (!pDev)
            return;

        ReleaseD3D11Objects();
        IGOLogDebug("Initialize DX11");

        // create the IGO IGOApplication::instance()
        if (!IGOApplication::instance())
            IGOApplication::createInstance(RendererDX11, pDev);

        // Load Compiled Vertex & Pixel Shader from DLL resource
        if (!CreateVertexShaderFromResource(pDev, IDR_IGOVSFX, &gVertexShader)
            || !CreatePixelShaderFromResource(pDev, IDR_IGOPSFX, &gPixelShader)
            || !CreateVertexShaderFromResource(pDev, IDR_IGOVSBACKGROUNDFX, &gBackgroundVertexShader)
            || !CreatePixelShaderFromResource(pDev, IDR_IGOPSBACKGROUNDFX, &gBackgroundPixelShader))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "Failed to create shaders."));
            ReleaseD3D11Objects();
            return;
        }

        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));

        // Create the constant buffers
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(OriginIGO::DX11CBNeverChanges);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = pDev->CreateBuffer(&bd, NULL, &gCBNeverChanges);
        IGO_ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreateBuffer() failed. hr = %x", hr));
            ReleaseD3D11Objects();
            return;
        }

        bd.ByteWidth = sizeof(OriginIGO::DX11CBChangesEveryFrame);
        hr = pDev->CreateBuffer(&bd, NULL, &gCBChangesEveryFrame);
        IGO_ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreateBuffer() failed. hr = %x", hr));
            ReleaseD3D11Objects();
            return;
        }

        ID3D11DeviceContext *pDevContext = NULL;
        pDev->GetImmediateContext(&pDevContext);
        if (pDevContext && gCBNeverChanges)
        {
            OriginIGO::DX11CBNeverChanges cbNeverChanges;
            cbNeverChanges.mViewProj = gMatOrtho;
            pDevContext->UpdateSubresource(gCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0);

            RELEASE_IF(pDevContext);
        }
        else
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "GetImmediateContext() failed."));
            ReleaseD3D11Objects();
            return;
        }
    

        // Define the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = sizeof(layout) / sizeof(layout[0]);

        DWORD shaderCodeSize = 0;
        const char* shaderCode = NULL;
        LoadEmbeddedResource(IDR_IGOVSFX, &shaderCode, &shaderCodeSize);
        hr = pDev->CreateInputLayout(layout, numElements, shaderCode, shaderCodeSize, &gVertexLayout);
        IGO_ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreateInputLayout() failed. hr = %x", hr));
            ReleaseD3D11Objects();
            return;
        }

        // create sampler state
        D3D11_SAMPLER_DESC sampDesc;
        ZeroMemory(&sampDesc, sizeof(sampDesc));
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = pDev->CreateSamplerState(&sampDesc, &gSamplerState);
        IGO_ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreateSamplerState() failed. hr = %x", hr));
            ReleaseD3D11Objects();
            return;
        }

        // create blend state
        D3D11_BLEND_DESC blendDesc;
        ZeroMemory(&blendDesc, sizeof(blendDesc));
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = pDev->CreateBlendState(&blendDesc, &gBlendState);
        IGO_ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreateBlendState() failed. hr = %x", hr));
            ReleaseD3D11Objects();
            return;
        }

        // create depth stencil state
        D3D11_DEPTH_STENCIL_DESC depthDesc;
        ZeroMemory(&depthDesc, sizeof(depthDesc));
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        hr = pDev->CreateDepthStencilState(&depthDesc, &gDepthStencilState);
        IGO_ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "CreateDepthStencilState() failed. hr = %x", hr));
            ReleaseD3D11Objects();
            return;
        }
    }

    // Present Hook
    bool DX11Hook::PresentHandler(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags, HRESULT* Result, IGOAPIDXPresentFcn PresentFcn)
    {
        EA::Thread::AutoFutex m(DX11Hook::mInstanceHookMutex); // Used to be safe relative to our destroy thread - but maybe we can remove that thread all together after code cleanup

        if (!pSwapChain || !Result || !PresentFcn)
            return false;

        gPresentHookCalled = GetTickCount();
        gDX11RenderingSwapChain = pSwapChain;

        if (Flags & DXGI_PRESENT_TEST)
        {
            *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
            return DX11HandleLostDevice(*Result);
        }

        {
            SAFE_CALL_LOCK_AUTO
                if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererDX11))
                {
                    *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
                    DX11HandleLostDevice(*Result);
                    DX11Hook::CleanupLater();
                    IGOLogWarn("Invalid D3D11 device from SwapChain. Could be D3D10 renderer instead.");

                    return true; // Cleanup in place
                }
        }

        if (!gInputHooked && !gQuitting)
            InputHook::TryHookLater(&gInputHooked);

        if (IGOApplication::isPendingDeletion())
        {
            IGOApplication::deleteInstance();
            ReleaseD3D11Objects();
        }

        if (!IGOApplication::instance())
        {
            // first time. we initialize the IGO
            ID3D11Device *pDevice = NULL;
            HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pDevice);

            if (SUCCEEDED(hr) && pDevice)
            {
                Init(pDevice);

                RELEASE_IF(pDevice);

                DXGI_SWAP_CHAIN_DESC desc = { 0 };
                hr = pSwapChain->GetDesc(&desc);
                if (SUCCEEDED(hr))
                {
                    gWindowedMode = desc.Windowed == TRUE;
                    gPresentHookThreadId = GetCurrentThreadId();
                    // hook windows message
                    if (!gQuitting)
                    {
                        InputHook::HookWindow(desc.OutputWindow);
                    }
                }
                else
                    IGOLogWarn("pSwapChain->GetDesc failed -> no HWND!");
            }
            else
            {
                IGO_TRACEF("Invalid D3D11 device from SwapChain. Could be D3D10 renderer instead.");
                IGOLogWarn("Invalid D3D11 device from SwapChain. Could be D3D10 renderer instead.");
                *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
                DX11HandleLostDevice(*Result);
                DX11Hook::CleanupLater();

			    return true; // Cleanup in place
            }

            *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
            return DX11HandleLostDevice(*Result);
        }
        SAFE_CALL_LOCK_ENTER
            SAFE_CALL(IGOApplication::instance(), &IGOApplication::startTime);
        SAFE_CALL_LOCK_LEAVE

            DXGI_SWAP_CHAIN_DESC desc = { 0 };
        if (SUCCEEDED(pSwapChain->GetDesc(&desc)))
        {
            // hook windows message
            if (!gQuitting)
            {
                InputHook::HookWindow(desc.OutputWindow);
            }

            gWindowedMode = desc.Windowed == TRUE;
            SAFE_CALL_LOCK_ENTER
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
                if (desc.OutputWindow)
                {
          	        RECT rect = {0};
        	        GetClientRect(desc.OutputWindow, &rect);
	                UINT wndWidth = rect.right - rect.left;
	                UINT wndHeight = rect.bottom - rect.top;
      		        SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, wndWidth, wndHeight);
                }
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::setSRGB, ScreenCopyDx11::isSRGB_format(desc.BufferDesc.Format));
            SAFE_CALL_LOCK_LEAVE
        }

        // check for resize & render
        ID3D11Device *pDevice = NULL;
        HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pDevice);
        if (SUCCEEDED(hr) && pDevice)
        {
            if (saveRenderState(pDevice, pSwapChain))
            {
                ID3D11DeviceContext *pDevContext = NULL;
                pDevice->GetImmediateContext(&pDevContext);
                if (pDevContext && gCBNeverChanges)
                {
                    uint32_t w = (uint32_t)desc.BufferDesc.Width;
                    uint32_t h = (uint32_t)desc.BufferDesc.Height;
                    SAFE_CALL_LOCK_ENTER
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, w, h);
                    SAFE_CALL_LOCK_LEAVE
                    buildProjectionMatrixDX11(w, h, 1, 10);
                    OriginIGO::DX11CBNeverChanges cbNeverChanges;
                    cbNeverChanges.mViewProj = gMatOrtho;
                    pDevContext->UpdateSubresource(gCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0);
                    RELEASE_IF(pDevContext)
                }

                if (TwitchManager::IsBroadCastingInitiated())
                    CreateScreenCopyDX11(pDevice, pSwapChain);
                else
                    DestroyScreenCopyDX11();

                if (TwitchManager::IsBroadCastingInitiated())
                {
                    TwitchManager::TTV_PollTasks();
                    if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
                    {
                        bool s = SAFE_CALL(gScreenCopy, &IScreenCopy::Render, pDevice, pSwapChain);
                        if (!s)
                            DestroyScreenCopyDX11();
                    }
                }
                SAFE_CALL_LOCK_ENTER
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::render, pDevice);
                SAFE_CALL_LOCK_LEAVE
                    restoreRenderState(pDevice);
            }

            RELEASE_IF(pDevice);
        }
        else
        {
            IGO_TRACEF("Invalid D3D11 device from SwapChain. Could be D3D10 renderer instead.");
            IGOLogWarn("Invalid D3D11 device from SwapChain. Could be D3D10 renderer instead.");
            *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
            DX11HandleLostDevice(*Result);
            DX11Hook::CleanupLater();

            SAFE_CALL_LOCK_ENTER
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::stopTime);
            SAFE_CALL_LOCK_LEAVE
            return true; // Cleanup in place
        }
        SAFE_CALL_LOCK_ENTER
            SAFE_CALL(IGOApplication::instance(), &IGOApplication::stopTime);
        SAFE_CALL_LOCK_LEAVE

        *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
        return DX11HandleLostDevice(*Result);
    }

    // Resize Hook
    bool DX11Hook::ResizeHandler(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, HRESULT* Result, IGOAPIDXResizeFcn ResizeFcn)
    {
        EA::Thread::AutoFutex m(DX11Hook::mInstanceHookMutex); // Used to be safe relative to our destroy thread - but maybe we can remove that thread all together after code cleanup

        if (!pSwapChain || !Result || !ResizeFcn)
            return false;

        {
            SAFE_CALL_LOCK_AUTO
            if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererDX11))
            {
                *Result = ResizeFcn(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
                DX11HandleLostDevice(*Result);
                DX11Hook::CleanupLater();
                IGOLogWarn("Invalid D3D11 device from SwapChain. Could be D3D10 renderer instead.");

                return true; // reset triggered
            }
        }

        // clear all objects
        SAFE_CALL_LOCK_ENTER
            SAFE_CALL(IGOApplication::instance(), &IGOApplication::clearWindows, NULL);
        SAFE_CALL_LOCK_LEAVE

        if (IGOApplication::isPendingDeletion())
            IGOApplication::deleteInstance();

        // release other buffers
        ReleaseD3D11Objects();
        gTestCooperativeLevelHook_or_ResetHook_Called = true;

        HRESULT retval = ResizeFcn(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        if (!DX11HandleLostDevice(retval))
        {
            if (FAILED(retval))
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "Resize failed hr = %x.", retval));

            // render
            ID3D11Device *pDevice = NULL;
            HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pDevice);
            if (SUCCEEDED(retval) && SUCCEEDED(hr) && pDevice)
            {
                Init(pDevice);

                DXGI_SWAP_CHAIN_DESC desc = { 0 };
                hr = pSwapChain->GetDesc(&desc);
                if (SUCCEEDED(hr))
                {
                    gWindowedMode = desc.Windowed == TRUE;
                    InputHook::HookWindow(desc.OutputWindow);
                }
                else
                    IGOLogWarn("pSwapChain->GetDesc failed -> no HWND!");

                ID3D11DeviceContext *pDevContext = NULL;
                pDevice->GetImmediateContext(&pDevContext);

                if (pDevContext && gCBNeverChanges)
                {
                    buildProjectionMatrixDX11(Width, Height, 1, 10);

                    OriginIGO::DX11CBNeverChanges cbNeverChanges;
                    cbNeverChanges.mViewProj = gMatOrtho;
                    pDevContext->UpdateSubresource(gCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0);

                    RELEASE_IF(pDevContext)
                }

                RELEASE_IF(pDevice)

                SAFE_CALL_LOCK_ENTER
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, Width, Height);
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::reset);
                SAFE_CALL_LOCK_LEAVE
            }
        }

        *Result = retval;

        return FAILED(retval); // lose it if we can't reinitialize the resources
    }

    ULONG DX11Hook::ReleaseHandler(IDXGISwapChain* pSwapChain, IGOAPIDXReleaseFcn ReleaseFcn)
    {
        if (!pSwapChain || !ReleaseFcn)
            return 0;

        if (gDX11RenderingSwapChain != NULL && gDX11RenderingSwapChain != pSwapChain)
            IGOLogWarn("ReleaseHook wrong swap chain!");

        // I don't believe we need any SAFE_CALL_LOCK_ENTER/LEAVE here...
        ULONG ul = 0;
        IGOApplication* instance = IGOApplication::instance();
        if (!gCalledFromInsideIGO && gDX11RenderingSwapChain == pSwapChain && instance)
	    {
		    gCalledFromInsideIGO++;
		    pSwapChain->AddRef();
		    ULONG currRefCount = ReleaseFcn(pSwapChain);
            
		    if (currRefCount == 1)
		    {
			    // The swap chain is being destroyed
                gDX11RenderingSwapChain = NULL;

                ReleaseD3D11Objects();
			    IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!	
                IGOLogWarn("Releasing dx11 swap chain");

                gCalledFromInsideIGO--;
	            ul = ReleaseFcn(pSwapChain);
                return ul;
		    }

            gCalledFromInsideIGO--;
            ul = ReleaseFcn(pSwapChain);
            return ul;
        }

        ul = ReleaseFcn(pSwapChain);
	    return ul;
    }

    DX11Hook::DX11Hook()
    {
    }


    DX11Hook::~DX11Hook()
    {
    }

    DWORD WINAPI IGODX11HookDestroyThread(LPVOID *lpParam)
    {
        DX11Hook::Cleanup();

        CloseHandle(DX11Hook::mIGODX11HookDestroyThreadHandle);
        DX11Hook::mIGODX11HookDestroyThreadHandle = NULL;
        return 0;
    }


    void DX11Hook::CleanupLater()
    {
        if (DX11Hook::mIGODX11HookDestroyThreadHandle == NULL)
            DX11Hook::mIGODX11HookDestroyThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGODX11HookDestroyThread, NULL, 0, NULL);
    }

    //bool DX11Hook::IsReadyForRehook()
    //{
    //    // limit re-hooking to have a 15 second timeout!
    //    if (GetTickCount() - mLastHookTime < REHOOKCHECK_DELAY)
    //        return false;

    //    return true;
    //}

    void DX11Hook::Cleanup()
    {
        EA::Thread::AutoFutex m(DX11Hook::mInstanceHookMutex);

        // only kill IGO if it was created by this render
        SAFE_CALL_LOCK_ENTER
            if (IGOApplication::instance() != NULL && (IGOApplication::instance()->rendererType() == RendererDX11))
            {
            bool safeToCall = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isThreadSafe);
            SAFE_CALL_LOCK_LEAVE

                if (safeToCall)
                {
                    ReleaseD3D11Objects();
                    IGOApplication::deleteInstance();
                }

                else
                    IGOApplication::deleteInstanceLater();
            }
            else
            {
                SAFE_CALL_LOCK_LEAVE
            }
    }

    bool DX11Hook::IsValidDevice(IDXGISwapChain* swapChain)
    {
        if (!swapChain)
            return false;

        ID3D11Device* device = NULL;
        HRESULT hr = swapChain->GetDevice(__uuidof(device), (void **)&device);
        if (FAILED(hr) || !device)
            return false;

        RELEASE_IF(device);
        return true;
    }
}
#endif // ORIGIN_PC
