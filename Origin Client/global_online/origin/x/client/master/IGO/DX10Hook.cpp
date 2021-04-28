///////////////////////////////////////////////////////////////////////////////
// DX10Hook.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "DX10Hook.h"
#include "DX11Hook.h"

#if defined(ORIGIN_PC)

#include <windows.h>
#include <d3d10.h>

#include "HookAPI.h"

#include "IGOApplication.h"
#include "resource.h"

#include "EASTL/hash_map.h"
#include "IGOTrace.h"
#include "InputHook.h"
#include "IGOLogger.h"

#include "ScreenCopyDx10.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

namespace OriginIGO {

    ID3D10Effect *gEffect = NULL;
    ID3D10EffectTechnique *gEffectTechnique = NULL;
    ID3D10EffectMatrixVariable* gWorldVariable = NULL;
    ID3D10EffectMatrixVariable* gViewProjectionVariable = NULL;
    ID3D10EffectVectorVariable* gColorVariable = NULL;
    ID3D10EffectShaderResourceVariable* gTextureVariable = NULL;
    extern D3DMATRIX gMatOrtho;
    extern D3DMATRIX gMatIdentity;
    extern D3DMATRIX gMatWorld;
    ID3D10InputLayout* gVertexLayout = NULL;
    ID3D10StateBlock *gStateBlock = NULL;
    EA::Thread::Futex DX10Hook::mInstanceHookMutex;
    HANDLE DX10Hook::mIGODX10HookDestroyThreadHandle = NULL;
    DWORD DX10Hook::mLastHookTime = 0; 
    extern DWORD gPresentHookThreadId;
    extern volatile DWORD gPresentHookCalled;
    IDXGISwapChain* gDX10RenderingSwapChain = NULL;
    extern volatile bool gTestCooperativeLevelHook_or_ResetHook_Called;

    extern HINSTANCE gInstDLL;
    extern bool gInputHooked;
    extern volatile bool gQuitting;

    void CreateScreenCopyDX10(ID3D10Device *pDev, IDXGISwapChain *pSwapChain)
    {
        if(gScreenCopy == NULL)
        {
            gScreenCopy = new ScreenCopyDx10();
            gScreenCopy->Create(pDev, pSwapChain);
        }
    }

    void DestroyScreenCopyDX10()
    {
        if(gScreenCopy)
        {
            gScreenCopy->Destroy();
            delete gScreenCopy;
            gScreenCopy = NULL;
        }
    }

    void buildProjectionMatrixDX10(int width, int height, int n, int f)
    {
	    ZeroMemory(&gMatOrtho, sizeof(gMatOrtho));
	    ZeroMemory(&gMatIdentity, sizeof(gMatIdentity));
	    gMatIdentity._11 = gMatIdentity._22 = gMatIdentity._33 = gMatIdentity._44 = 1.0f;

	    gMatOrtho._11 = 2.0f/(width==0?1:width);
	    gMatOrtho._22 = 2.0f/(height==0?1:height);
	    gMatOrtho._33 = -2.0f/(n - f);
	    gMatOrtho._43 = 2.0f/(n - f);
	    gMatOrtho._44 = 1.0f;
    }

    bool saveRenderState(ID3D10Device *pDev, IDXGISwapChain *pSwapChain)
    {
	    IGO_ASSERT(gStateBlock);
	    if (gStateBlock)
		    gStateBlock->Capture();

	    pDev->IASetInputLayout(gVertexLayout);
	    pDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	    // clear shader resources
	    ID3D10ShaderResourceView *const pSRV2[1] = {NULL};
	    pDev->PSSetShaderResources(0, 1, pSRV2);

	    // set new viewport
	    D3D10_VIEWPORT vp;
	

	    ZeroMemory(&vp, sizeof(vp));
        SAFE_CALL_LOCK_AUTO
	    vp.Width = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
	    vp.Height = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);
	    vp.MinDepth = D3D10_MIN_DEPTH;
	    vp.MaxDepth = D3D10_MAX_DEPTH;
		if (vp.Height > 0 && vp.Width > 0)
			pDev->RSSetViewports(1, &vp);

	    return true;
    }

    void restoreRenderState(ID3D10Device *pDev)
    {
	    IGO_ASSERT(gStateBlock);
	    if (gStateBlock)
		    gStateBlock->Apply();
    }

    void ReleaseD3D10Objects()
    {
        DestroyScreenCopyDX10();

	    RELEASE_IF(gEffect);
	    gEffectTechnique = NULL;
	    gWorldVariable = NULL;
	    gViewProjectionVariable = NULL;
	    gColorVariable = NULL;
	    gTextureVariable = NULL;
	    RELEASE_IF(gVertexLayout);
	    RELEASE_IF(gStateBlock);
    }
  
    bool DX10HandleLostDevice(HRESULT hr)
    {
#ifndef DXGI_ERROR_DEVICE_REMOVED
#define DXGI_ERROR_DEVICE_REMOVED 0x887A0005
#endif
        if (hr != DXGI_ERROR_DEVICE_REMOVED)
            return false;

        TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_HARDWARE, TelemetryRenderer_DX10, "Detected TDR scenario");

        ReleaseD3D10Objects();
        IGOApplication::deleteInstance();

        return true;
    }

    void Init(ID3D10Device *pDev)
    {
	    HRESULT hr;
	
	    ReleaseD3D10Objects();
	    IGOLogDebug("Initialize DX10");
	
	    // create the IGO IGOApplication::instance()
	    if (!IGOApplication::instance())
		    IGOApplication::createInstance(RendererDX10, pDev);

        // Load Compiled Effect from DLL resource
	    IGOLogDebug("Loading igo.fxo file");
        const char* pIGOFx;
        DWORD dwIGOFxSize = 0;
        if (!LoadEmbeddedResource(IDR_IGOFX, &pIGOFx, &dwIGOFxSize))
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "Failed to load shader resources."));
            ReleaseD3D10Objects();
            return;
        }

        // Create Effect
	    IGOLogDebug("Creating effect from igo.fxo");
	    HMODULE hD3D10 = GetModuleHandle(L"d3d10.dll");
	    typedef HRESULT (WINAPI *LPD3D10CREATEEFFECTFROMMEMORY)(LPCVOID pData, SIZE_T DataLength, UINT FXFlags, ID3D10Device *pDevice, ID3D10EffectPool *pEffectPool, ID3D10Effect **ppEffect);
	    LPD3D10CREATEEFFECTFROMMEMORY s_D3D10CreateEffectFromMemory = (LPD3D10CREATEEFFECTFROMMEMORY)GetProcAddress(hD3D10, "D3D10CreateEffectFromMemory");
        hr = s_D3D10CreateEffectFromMemory != NULL ? s_D3D10CreateEffectFromMemory(pIGOFx, dwIGOFxSize, 0, pDev, NULL, &gEffect) : E_FAIL;
	    if (FAILED(hr))
	    {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "D3DX10CreateEffectFromMemory() failed.hr = %x", hr));
            gEffect = NULL;
            ReleaseD3D10Objects();
            return;
	    }

	    if (gEffect)
	    {
		    gEffectTechnique = gEffect->GetTechniqueByName("Render");
		    IGO_ASSERT(gEffectTechnique);

		    gWorldVariable = gEffect->GetVariableByName("World")->AsMatrix();
		    gViewProjectionVariable = gEffect->GetVariableByName("ViewProj")->AsMatrix();
		    gColorVariable = gEffect->GetVariableByName("Color")->AsVector();
		    gTextureVariable = gEffect->GetVariableByName("Texture")->AsShaderResource();

		    IGO_ASSERT(gWorldVariable);
		    IGO_ASSERT(gViewProjectionVariable);
		    IGO_ASSERT(gColorVariable);
		    IGO_ASSERT(gTextureVariable);

            if (!gWorldVariable || !gViewProjectionVariable || !gColorVariable || !gTextureVariable)
            {
                ReleaseD3D10Objects();
                IGOLogWarn("Shader variables not found!");
                return;
            }
	    }

	    // Bind the variables
	    gWorldVariable->SetMatrix((float*)&gMatWorld);
	    gViewProjectionVariable->SetMatrix((float*)&gMatOrtho);

        // Define the input layout
        D3D10_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = sizeof( layout ) / sizeof( layout[0] );

        D3D10_PASS_DESC PassDesc;	
        gEffectTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
        hr = pDev->CreateInputLayout(layout, numElements, PassDesc.pIAInputSignature,
                                              PassDesc.IAInputSignatureSize, &gVertexLayout);
	    if (FAILED(hr))
	    {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "CreateInputLayout() failed.hr = %x", hr));
            ReleaseD3D10Objects();
            return;
        }

	    // create state block
	    typedef HRESULT (WINAPI *LPD3D10StateBlockMaskEnableAll)(D3D10_STATE_BLOCK_MASK *pMask);
	    LPD3D10StateBlockMaskEnableAll s_D3D10StateBlockMaskEnableAll = (LPD3D10StateBlockMaskEnableAll)GetProcAddress(hD3D10, "D3D10StateBlockMaskEnableAll");

	    typedef HRESULT (WINAPI *LPD3D10CreateStateBlock)(ID3D10Device *pDevice, D3D10_STATE_BLOCK_MASK *pStateBlockMask, ID3D10StateBlock **ppStateBlock);
	    LPD3D10CreateStateBlock s_D3D10CreateStateBlock = (LPD3D10CreateStateBlock)GetProcAddress(hD3D10, "D3D10CreateStateBlock");

	    D3D10_STATE_BLOCK_MASK mask;
        if (s_D3D10CreateStateBlock && s_D3D10StateBlockMaskEnableAll)
        {
            s_D3D10StateBlockMaskEnableAll(&mask);
            hr = s_D3D10CreateStateBlock(pDev, &mask, &gStateBlock);
            if (FAILED(hr) || !gStateBlock)
            {
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "s_D3D10CreateStateBlock() failed.hr = %x", hr));
                ReleaseD3D10Objects();
                return;
            }
        }
        else
        {
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "s_D3D10CreateStateBlock or s_D3D10StateBlockMaskEnableAll failed"));
            ReleaseD3D10Objects();
            return;
        }
    }

    // Present Hook
    bool DX10Hook::PresentHandler(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags, HRESULT* Result, IGOAPIDXPresentFcn PresentFcn)
    {
        EA::Thread::AutoFutex m(DX10Hook::mInstanceHookMutex); // Used to be safe relative to our destroy thread - but maybe we can remove that thread all together after code cleanup

        if (!pSwapChain || !Result || !PresentFcn)
            return false;

        gPresentHookCalled = GetTickCount();
        gDX10RenderingSwapChain = pSwapChain;

		if (Flags & DXGI_PRESENT_TEST)
        {
			*Result = PresentFcn(pSwapChain, SyncInterval, Flags);
            return DX10HandleLostDevice(*Result);
        }

        {
            SAFE_CALL_LOCK_AUTO
            if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererDX10))
            {
                *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
                DX10HandleLostDevice(*Result);
                DX10Hook::CleanupLater();
		        IGOLogWarn("Invalid D3D10 device from SwapChain. Could be D3D11 renderer instead.");

                return true; // Cleanup in place
            }
        }
      
        if( IGOApplication::isPendingDeletion())
        {
			IGOApplication::deleteInstance();
            ReleaseD3D10Objects();
        }

	    if (!gInputHooked && !gQuitting)
		    InputHook::TryHookLater(&gInputHooked);

	    if (!IGOApplication::instance())
	    {
		    // first time. we initialize the IGO
		    ID3D10Device *pDevice = NULL;
		    HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D10Device), (void **)&pDevice);
	
		    if (SUCCEEDED(hr) && pDevice)
		    {
			    Init(pDevice);

			    RELEASE_IF(pDevice);

			    DXGI_SWAP_CHAIN_DESC desc;
			    pSwapChain->GetDesc(&desc);
			    gWindowedMode = desc.Windowed == TRUE;
                gPresentHookThreadId = GetCurrentThreadId();
                // hook windows message
                if (!gQuitting)
                {
	        	    InputHook::HookWindow(desc.OutputWindow);
                }
		    }
		    else
		    {
			    IGO_TRACEF("Invalid D3D10 device from SwapChain. Could be D3D11 renderer instead.");
			    IGOLogWarn("Invalid D3D10 device from SwapChain. Could be D3D11 renderer instead.");
			    *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
                DX10HandleLostDevice(*Result);
                DX10Hook::CleanupLater();

			    return true; // Cleanup in place
		    }
		
		    *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
            return DX10HandleLostDevice(*Result);
	    }
        SAFE_CALL_LOCK_ENTER
	    SAFE_CALL(IGOApplication::instance(), &IGOApplication::startTime);
        SAFE_CALL_LOCK_LEAVE

	    DXGI_SWAP_CHAIN_DESC desc;
	    if (SUCCEEDED(pSwapChain->GetDesc(&desc)))
	    {
            // hook windows message
            if (!gQuitting)
            {
	        	InputHook::HookWindow(desc.OutputWindow);
            }
            gWindowedMode = desc.Windowed == TRUE;
		    // check for resize
            int newWidth = desc.BufferDesc.Width;
            int newHeight = desc.BufferDesc.Height;
            SAFE_CALL_LOCK_ENTER
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, newWidth, newHeight);
            if (desc.OutputWindow)
            {
          	    RECT rect = {0};
        	    GetClientRect(desc.OutputWindow, &rect);
	            UINT wndWidth = rect.right - rect.left;
	            UINT wndHeight = rect.bottom - rect.top;
      		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, wndWidth, wndHeight);
            }

            SAFE_CALL(IGOApplication::instance(), &IGOApplication::setSRGB, ScreenCopyDx10::isSRGB_format(desc.BufferDesc.Format));
            SAFE_CALL_LOCK_LEAVE

			buildProjectionMatrixDX10(newWidth, newHeight, 1, 10);
            if (gViewProjectionVariable)
                gViewProjectionVariable->SetMatrix((float*)&gMatOrtho);

	    }

	    // render
	    ID3D10Device *pDevice = NULL;
	    HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D10Device), (void **)&pDevice);
	    if (SUCCEEDED(hr) && pDevice)
	    {
		    if (saveRenderState(pDevice, pSwapChain))
		    {
                if (TwitchManager::IsBroadCastingInitiated())
                    CreateScreenCopyDX10(pDevice, pSwapChain);
                else
                    DestroyScreenCopyDX10();

                if (TwitchManager::IsBroadCastingInitiated())
                {
                    TwitchManager::TTV_PollTasks();
                    if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
                    {
                        bool s = SAFE_CALL(gScreenCopy, &IScreenCopy::Render, pDevice, pSwapChain);
                        if (!s)
                            DestroyScreenCopyDX10();
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
		    IGO_TRACEF("Invalid D3D10 device from SwapChain. Could be D3D11 renderer instead.");
		    IGOLogWarn("Invalid D3D10 device from SwapChain. Could be D3D11 renderer instead.");
		    *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
            DX10HandleLostDevice(*Result);
            DX10Hook::CleanupLater();
		    return true; // Cleanup in place
	    }
        SAFE_CALL_LOCK_ENTER
	    SAFE_CALL(IGOApplication::instance(), &IGOApplication::stopTime);
        SAFE_CALL_LOCK_LEAVE

	    *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
        return DX10HandleLostDevice(*Result);
    }

    // Resize Hook
    bool DX10Hook::ResizeHandler(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, HRESULT* Result, IGOAPIDXResizeFcn ResizeFcn)
    {
        EA::Thread::AutoFutex m(DX10Hook::mInstanceHookMutex); // Used to be safe relative to our destroy thread - but maybe we can remove that thread all together after code cleanup

        if (!pSwapChain || !Result || !ResizeFcn)
            return false;

        {
            SAFE_CALL_LOCK_AUTO
                if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererDX10))
                {
                    *Result = ResizeFcn(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
                    DX10HandleLostDevice(*Result);
                    DX10Hook::CleanupLater();
		            IGOLogWarn("Invalid D3D10 device from SwapChain. Could be D3D11 renderer instead.");

                    return true; // reset triggered
                }
        }

        // clear all objects
	    SAFE_CALL_LOCK_ENTER
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::clearWindows, NULL);
		SAFE_CALL_LOCK_LEAVE
	    
		if( IGOApplication::isPendingDeletion())
			IGOApplication::deleteInstance();
		
		// release other buffers
	    ReleaseD3D10Objects();
        gTestCooperativeLevelHook_or_ResetHook_Called = true;

        HRESULT retval = ResizeFcn(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        if (!DX10HandleLostDevice(retval))
        {
            if (FAILED(retval))
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "Resize failed hr = %x.", retval));

	        // render
	        ID3D10Device *pDevice = NULL;
	        HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D10Device), (void **)&pDevice);
	        if (SUCCEEDED(retval) && SUCCEEDED(hr) && pDevice)
	        {
		        Init(pDevice);
		        RELEASE_IF(pDevice);

		        DXGI_SWAP_CHAIN_DESC desc;
		        pSwapChain->GetDesc(&desc);
		        gWindowedMode = desc.Windowed == TRUE;
		        InputHook::HookWindow(desc.OutputWindow);
	        }

            buildProjectionMatrixDX10(Width, Height, 1, 10);
            if (gViewProjectionVariable)
                gViewProjectionVariable->SetMatrix((float*)&gMatOrtho);

            SAFE_CALL_LOCK_ENTER
		        SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
		        SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, Width, Height);
		        SAFE_CALL(IGOApplication::instance(), &IGOApplication::reset);
            SAFE_CALL_LOCK_LEAVE
        }

        *Result = retval;

        return FAILED(retval); // lose it if we can't reinitialize the resources
    }

    ULONG DX10Hook::ReleaseHandler(IDXGISwapChain* pSwapChain, IGOAPIDXReleaseFcn ReleaseFcn)
    {
        if (!pSwapChain || !ReleaseFcn)
            return 0;

        if (gDX10RenderingSwapChain != NULL && gDX10RenderingSwapChain != pSwapChain)
            IGOLogWarn("ReleaseHook wrong swap chain!");

        // I don't believe we need any SAFE_CALL_LOCK_ENTER/LEAVE here...
        ULONG ul = 0;
        IGOApplication* instance = IGOApplication::instance();
        if (!gCalledFromInsideIGO && gDX10RenderingSwapChain == pSwapChain && instance)
	    {
		    gCalledFromInsideIGO++;
		    pSwapChain->AddRef();
		    ULONG currRefCount = ReleaseFcn(pSwapChain);
            
		    if (currRefCount == 1)
		    {
			    // The sawp chain is being destroyed
                gDX10RenderingSwapChain = NULL;

                ReleaseD3D10Objects();
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

	DX10Hook::DX10Hook()
    {
    }


    DX10Hook::~DX10Hook()
    {
	}

    DWORD WINAPI IGODX10HookDestroyThread(LPVOID *lpParam)
    {
        DX10Hook::Cleanup();
        
        CloseHandle(DX10Hook::mIGODX10HookDestroyThreadHandle);
        DX10Hook::mIGODX10HookDestroyThreadHandle = NULL;
	    return 0;
    }


    void DX10Hook::CleanupLater()
    {
        if (DX10Hook::mIGODX10HookDestroyThreadHandle==NULL)
            DX10Hook::mIGODX10HookDestroyThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGODX10HookDestroyThread, NULL, 0, NULL);
    }

    //bool DX10Hook::IsReadyForRehook()
    //{
    //    // limit re-hooking to have a 15 second timeout!
    //    if (GetTickCount() - mLastHookTime < REHOOKCHECK_DELAY)
    //        return false;

    //    return true;
    //}

    void DX10Hook::Cleanup()
    {
        EA::Thread::AutoFutex m(DX10Hook::mInstanceHookMutex);

        // only kill IGO if it was created by this render
        SAFE_CALL_LOCK_ENTER
            if (IGOApplication::instance() != NULL && (IGOApplication::instance()->rendererType() == RendererDX10))
            {
            bool safeToCall = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isThreadSafe);
            SAFE_CALL_LOCK_LEAVE

                if (safeToCall)
                {
                    IGOApplication::deleteInstance();
                    ReleaseD3D10Objects();
                }

                else
                    IGOApplication::deleteInstanceLater();
            }
            else
            {
                SAFE_CALL_LOCK_LEAVE
            }
    }

    bool DX10Hook::IsValidDevice(IDXGISwapChain* swapChain)
    {
        if (!swapChain)
            return false;

        ID3D10Device* device = NULL;
        HRESULT hr = swapChain->GetDevice(__uuidof(device), (void **)&device);
        if (FAILED(hr) || !device)
            return false;

        RELEASE_IF(device);
        return true;
    }
}
#endif // ORIGIN_PC
