///////////////////////////////////////////////////////////////////////////////
// DXGIHook.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(ORIGIN_PC)

#include <DXGI.h>

#include "IGO.h"
#include "DXGIHook.h"
#include "IGOLogger.h"
#include "DX10Hook.h"
#include "DX11Hook.h"
#include "DX12Hook.h"
#include "HookAPI.h"
#include "../Helpers.h"
#include "IGOSharedStructs.h"
#include "../IGOProxy/IGOAPIInfo.h"

namespace OriginIGO 
{

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    EA::Thread::Futex DXGIHookInstanceMutex;

    struct DXGIHookingCtxt
    {
        DXGIHookingCtxt()
        {
            ZeroMemory(this, sizeof(*this));
        }

        IGOAPIInfo_V1 dx10Offsets;
        IGOAPIInfo_V1 dx11Offsets;
        IGOAPIInfo_V1 dx12Offsets;

        bool dx10InUse;         // dx10 currently in use
        bool dx11InUse;         // dx11 currently in use
        bool dx12InUse;         // dx12 currently in use
        bool dx10Initialized;   // true after we try to setup DX10 - successfully or not
        bool dx11Initialized;   // true after we try to setup DX11 - successfully or not
        bool dx12Initialized;   // true after we try to setup DX12 - successfully or not
    };

    static DXGIHookingCtxt mDXGIHookingCtxt;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool ReadPrecomputedOffsets(const char* apiName, OriginIGO::TelemetryRenderer renderer, HMODULE hD3D, HMODULE hDXGI, void* buffer, size_t bufferSize)
	{
        IGOLogInfo("Checking %s offsets (%p/%p)", apiName, hD3D, hDXGI);

		char d3dDllMd5[33] = { 0 };
		if (!ComputeFileMd5(hD3D, d3dDllMd5))
			return false;

		char dxgiDllMd5[33] = { 0 };
		if (!ComputeFileMd5(hDXGI, dxgiDllMd5))
			return false;

		bool retVal = false;
		WCHAR commonDataPath[MAX_PATH];
		if (GetCommonDataPath(commonDataPath))
		{
			WCHAR fileName[MAX_PATH] = { 0 };
			_snwprintf(fileName, sizeof(fileName), L"%s\\Origin\\IGOCache\\%S_%S_%S.igo", commonDataPath, apiName, d3dDllMd5, dxgiDllMd5);
            IGOLogInfo("Loading cache file '%S'", fileName);

			char* data = NULL;
			size_t dataSize = 0;
			if (ReadFileContent(fileName, &data, &dataSize))
			{
				if (dataSize > sizeof(IGOAPIInfo))
				{
					IGOAPIInfo* apiInfo = reinterpret_cast<IGOAPIInfo*>(data);
					if (apiInfo->version == IGOAPIInfo::CurrentVersion && dataSize == bufferSize)
					{
                        memcpy(buffer, data, bufferSize);
						retVal = true;
					}
				}

				delete[] data;
			}
			
            else
               CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, renderer, "Missing/invalid cache file '%S'", fileName));
		}

		return retVal;
	}

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Handlers
    bool SwapChainReleaseHandler(IDXGISwapChain* swapChain, ULONG* refCount, IGOAPIDXReleaseFcn release)
    {
        if (!swapChain || !refCount || !release)
            return false;

        // What kind of  device are we dealing with?
        bool isDX10Device = DX10Hook::IsValidDevice(swapChain);
        bool isDX11Device = DX11Hook::IsValidDevice(swapChain);
        bool isDX12Device = DX12Hook::IsValidDevice(swapChain);

        // If we are already running, is it still a correct device?
        if (mDXGIHookingCtxt.dx10InUse)
        {
            if (isDX10Device)
            {
                *refCount = DX10Hook::ReleaseHandler(swapChain, release);
                if (*refCount == 0)
                {
                    IGOLogWarn("DX10 Swapchain %p released - dx10InUse=false", swapChain);
                    mDXGIHookingCtxt.dx10InUse = false;
                }

                return true; // handled
            }

            else
            {
                // Unexpected device (DX11/DX12/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Release called on non-DX10 swapchain %p - dx10InUse=false", swapChain);

                DX10Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx10InUse = false;

                return false; // not handled
            }
        }

        if (mDXGIHookingCtxt.dx11InUse)
        {
            if (isDX11Device)
            {
                *refCount = DX11Hook::ReleaseHandler(swapChain, release);
                if (*refCount == 0)
                {
                    IGOLogWarn("DX11 Swapchain %p released - dx11InUse=false", swapChain);
                    mDXGIHookingCtxt.dx11InUse = false;
                }

                return true; // handled
            }

            else
            {
                // Unexpected device (DX10/DX12/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Release called on non-DX11 swapchain %p - dx11InUse=false", swapChain);

                DX11Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx11InUse = false;

                return false; // not handled
            }
        }

        if (mDXGIHookingCtxt.dx12InUse)
        {
            if (isDX12Device)
            {
                *refCount = DX12Hook::ReleaseHandler(swapChain, release);
                if (*refCount == 0)
                {
                    IGOLogWarn("DX12 Swapchain %p released - dx12InUse=false", swapChain);
                    mDXGIHookingCtxt.dx12InUse = false;
                }

                return true; // handled
            }

            else
            {
                // Unexpected device (DX10/DX11/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Release called on non-DX12 swapchain %p - dx12InUse=false", swapChain);

                DX12Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx12InUse = false;

                return false; // not handled
            }
        }

        return false; // not handled
    }

    bool SwapChainPresentHandler(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags, HRESULT* result, IGOAPIDXPresentFcn present)
    {
        if (!swapChain || !result || !present)
            return false;

        // What kind of  device are we dealing with?
        bool isDX10Device = DX10Hook::IsValidDevice(swapChain);
        bool isDX11Device = DX11Hook::IsValidDevice(swapChain);
        bool isDX12Device = DX12Hook::IsValidDevice(swapChain);

        // If we are already running, is it still a correct device?
        if (mDXGIHookingCtxt.dx10InUse)
        {
            if (isDX10Device)
            {
                if (DX10Hook::PresentHandler(swapChain, syncInterval, flags, result, present))
                {
                    IGOLogWarn("DX10 Swapchain %p Present failed - dx10InUse=false", swapChain);
                    mDXGIHookingCtxt.dx10InUse = false;
                }
                return true; // handled
            }

            else
            {
                // Unexpected device (DX11/DX12/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Present called on non-DX10 swapchain %p - dx10InUse=false", swapChain);

                DX10Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx10InUse = false;

                return false; // not handled
            }
        }

        if (mDXGIHookingCtxt.dx11InUse)
        {
            if (isDX11Device)
            {
                if (DX11Hook::PresentHandler(swapChain, syncInterval, flags, result, present))
                {
                    IGOLogWarn("DX11 Swapchain %p Present failed - dx11InUse=false", swapChain);
                    mDXGIHookingCtxt.dx11InUse = false;
                }

                return true; // handled
            }

            else
            {
                // Unexpected device (DX10/DX12/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Present called on non-DX11 swapchain %p - dx11InUse=false", swapChain);

                DX11Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx11InUse = false;

                return false; // not handled
            }
        }

        if (mDXGIHookingCtxt.dx12InUse)
        {
            if (isDX12Device)
            {
                if (DX12Hook::PresentHandler(swapChain, syncInterval, flags, result, present))
                {
                    IGOLogWarn("DX12 Swapchain %p Present failed - dx12InUse=false", swapChain);
                    mDXGIHookingCtxt.dx12InUse = false;
                }

                return true; // handled
            }

            else
            {
                // Unexpected device (DX10/DX11/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Present called on non-DX12 swapchain %p - dx12InUse=false", swapChain);

                DX12Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx12InUse = false;

                return false; // not handled
            }
        }

        // Is it time to start OIG?
        if (isDX10Device)
        {
            if (mDXGIHookingCtxt.dx10Initialized)
            {
                IGOLogWarn("Detected DX10 swapchain %p + initialized - dx10InUse=true", swapChain);

                // Yep, we're good to go
                mDXGIHookingCtxt.dx10InUse = true;
                if (DX10Hook::PresentHandler(swapChain, syncInterval, flags, result, present))
                {
                    IGOLogWarn("DX10 Swapchain %p Present immediately failed - reverting dx10InUse=false", swapChain);
                    mDXGIHookingCtxt.dx10InUse = false; // oops - something happened!
                }

                return true; // handled
            }
        }

        else
        if (isDX11Device)
        {
            if (mDXGIHookingCtxt.dx11Initialized)
            {
                IGOLogWarn("Detected DX11 swapchain %p + initialized - dx11InUse=true", swapChain);

                // Yep, we're good to go
                mDXGIHookingCtxt.dx11InUse = true;
                if (DX11Hook::PresentHandler(swapChain, syncInterval, flags, result, present))
                {
                    IGOLogWarn("DX11 Swapchain %p Present immediately failed - reverting dx11InUse=false", swapChain);
                    mDXGIHookingCtxt.dx11InUse = false; // oops - something happened!
                }

                return true; // handled
            }
        }
        
        else
        if (isDX12Device)
        {
            if (mDXGIHookingCtxt.dx12Initialized)
            {
                IGOLogWarn("Detected DX12 swapchain %p + initialized - dx12InUse=true", swapChain);

                // Yep, we're good to go
                mDXGIHookingCtxt.dx12InUse = true;
                if (DX12Hook::PresentHandler(swapChain, syncInterval, flags, result, present))
                {
                    IGOLogWarn("DX12 Swapchain %p Present immediately failed - reverting dx12InUse=false", swapChain);
                    mDXGIHookingCtxt.dx12InUse = false; // oops - something happened!
                }

                return true; // handled
            }
        }

        return false; // not handled
    }

    bool SwapChainResizeHandler(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags, HRESULT* result, IGOAPIDXResizeFcn resize)
    {
		IGOLogInfo("SwapChainResizeHandler chain=%p, count=%d, width=%d, height=%d, format=0x%08x, flags=0x%08x", swapChain, bufferCount, width, height, newFormat, swapChainFlags);

        if (!swapChain || !result || !resize)
            return false;

        // What kind of  device are we dealing with?
        bool isDX10Device = DX10Hook::IsValidDevice(swapChain);
        bool isDX11Device = DX11Hook::IsValidDevice(swapChain);
		bool isDX12Device = DX12Hook::IsValidDevice(swapChain);

        // If we are already running, is it still a correct device?
        if (mDXGIHookingCtxt.dx10InUse)
        {
            if (isDX10Device)
            {
                if (DX10Hook::ResizeHandler(swapChain, bufferCount, width, height, newFormat, swapChainFlags, result, resize))
                {
                    IGOLogWarn("DX10 Swapchain %p resize failure - dx10InUse=false", swapChain);
                    mDXGIHookingCtxt.dx10InUse = false;
                }

				IGOLogInfo("Result=0x%08x", *result);
                return true; // handled
            }

            else
            {
                // Unexpected device (DX11/DX12/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Resize called on non-DX10 swapchain %p - dx10InUse=false", swapChain);

                DX10Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx10InUse = false;

                return false; // not handled
            }
        }

        if (mDXGIHookingCtxt.dx11InUse)
        {
            if (isDX11Device)
            {
                if (DX11Hook::ResizeHandler(swapChain, bufferCount, width, height, newFormat, swapChainFlags, result, resize))
                {
                    IGOLogWarn("DX11 Swapchain %p resize failure - dx11InUse=false", swapChain);
                    mDXGIHookingCtxt.dx11InUse = false;
                }

				IGOLogInfo("Result=0x%08x", *result);
                return true; // handled
            }

            else
            {
                // Unexpected device (DX10/DX12/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("Resize called on non-DX11 swapchain %p - dx11InUse=false", swapChain);

                DX11Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx11InUse = false;

                return false; // not handled
            }
        }

		if (mDXGIHookingCtxt.dx12InUse)
		{
			if (isDX12Device)
			{
				if (DX12Hook::ResizeBuffersHandler(swapChain, bufferCount, width, height, newFormat, swapChainFlags, result, resize))
				{
					IGOLogWarn("DX12 Swapchain %p resize failure - dx=12InUse=false", swapChain);
					mDXGIHookingCtxt.dx12InUse = false;
				}

				IGOLogInfo("Result=0x%08x", *result);
				return true; // handled
			}

			else
			{
				// Unexpected device (DX10/DX11/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
				IGOLogWarn("Resize called on non-DX12 swapchain %p - dx12InUse=false", swapChain);

				DX12Hook::Cleanup(); // hopefully safe
				mDXGIHookingCtxt.dx12InUse = false;

				return false; // not handled
			}
		}

        return false; // not handled
    }

    bool SwapChainResizeBuffersHandler(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags, HRESULT* result, IGOAPIDXResizeBuffersFcn resizeBuffers)
    {
		IGOLogInfo("SwapChainResizeBuffersHandler chain=%p, count=%d, width=%d, height=%d, format=0x%08x, flags=0x%08x", swapChain, bufferCount, width, height, newFormat, swapChainFlags);

        if (!swapChain || !result || !resizeBuffers)
            return false;

        // Only using this hook for DX12 right now

        // What kind of  device are we dealing with?
        bool isDX12Device = DX12Hook::IsValidDevice(swapChain);

        if (mDXGIHookingCtxt.dx12InUse)
        {
            if (isDX12Device)
            {
                if (DX12Hook::ResizeBuffersHandler(swapChain, bufferCount, width, height, newFormat, swapChainFlags, result, resizeBuffers))
                {
                    IGOLogWarn("DX12 Swapchain %p resizeBuffers failure - dx12InUse=false", swapChain);
                    mDXGIHookingCtxt.dx12InUse = false;
                }

				IGOLogInfo("Result=0x%08x", *result);
                return true; // handled
            }

            else
            {
                // Unexpected device (DX10/DX11/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("ResizeBuffers called on non-DX12 swapchain %p - dx12InUse=false", swapChain);

                DX12Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx12InUse = false;

                return false; // not handled
            }
        }

        return false; // not handled
    }

    bool SwapChainResizeBuffers1Handler(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue, HRESULT* result, IGOAPIDXResizeBuffers1Fcn resizeBuffers1)
    {
		IGOLogInfo("SwapChainResizeBuffers1Handler chain=%p, count=%d, width=%d, height=%d, format=0x%08x, flags=0x%08x, nodes=%p, queues=%p", swapChain, bufferCount, width, height, newFormat, swapChainFlags, pCreationNodeMask, ppPresentQueue);

        if (!swapChain || !result || !resizeBuffers1)
            return false;

        // Only using this hook for DX12 right now

        // What kind of  device are we dealing with?
        bool isDX12Device = DX12Hook::IsValidDevice(swapChain);

        if (mDXGIHookingCtxt.dx12InUse)
        {
            if (isDX12Device)
            {
                if (DX12Hook::ResizeBuffers1Handler(swapChain, bufferCount, width, height, newFormat, swapChainFlags, pCreationNodeMask, ppPresentQueue, result, resizeBuffers1))
                {
                    IGOLogWarn("DX12 Swapchain %p resizeBuffers1 failure - dx12InUse=false", swapChain);
                    mDXGIHookingCtxt.dx12InUse = false;
                }

				IGOLogInfo("Result=0x%08x", *result);
                return true; // handled
            }

            else
            {
                // Unexpected device (DX10/DX11/etc...) -> this shouldn't be happening (should have cleanup on Release count == 0)
                IGOLogWarn("ResizeBuffers1 called on non-DX12 swapchain %p - dx12InUse=false", swapChain);

                DX12Hook::Cleanup(); // hopefully safe
                mDXGIHookingCtxt.dx12InUse = false;

                return false; // not handled
            }
        }

        return false; // not handled
    }

#pragma endregion

#pragma region Hooks
    DEFINE_HOOK_SAFE_EX(ULONG, DX10SwapChainReleaseHook, (IDXGISwapChain* pSwapChain), DXGIHookInstanceMutex)

        ULONG refCount = 0;
        if (!SwapChainReleaseHandler(pSwapChain, &refCount, DX10SwapChainReleaseHookNext))
            refCount = DX10SwapChainReleaseHookNext(pSwapChain);

        if (!refCount)
            DX12Hook::SwapChainReleasedNotify(pSwapChain);

        return refCount;
    }

    DEFINE_HOOK_SAFE_EX(HRESULT, DX10SwapChainPresentHook, (IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags), DXGIHookInstanceMutex)

        HRESULT hr = S_OK;
        if (SwapChainPresentHandler(pSwapChain, SyncInterval, Flags, &hr, DX10SwapChainPresentHookNext))
            return hr;

        return DX10SwapChainPresentHookNext(pSwapChain, SyncInterval, Flags);
    }

    DEFINE_HOOK_SAFE_EX(HRESULT, DX10SwapChainResizeHook, (IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags), DXGIHookInstanceMutex)
    
        HRESULT hr = S_OK;
        if (SwapChainResizeHandler(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, &hr, DX10SwapChainResizeHookNext))
            return hr;

        return DX10SwapChainResizeHookNext(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }

    DEFINE_HOOK_SAFE_EX(ULONG, DX11SwapChainReleaseHook, (IDXGISwapChain* pSwapChain), DXGIHookInstanceMutex)

        ULONG refCount = 0;
        if (!SwapChainReleaseHandler(pSwapChain, &refCount, DX11SwapChainReleaseHookNext))
            refCount = DX11SwapChainReleaseHookNext(pSwapChain);

        if (!refCount)
            DX12Hook::SwapChainReleasedNotify(pSwapChain);

        return refCount;
    }

    DEFINE_HOOK_SAFE_EX(HRESULT, DX11SwapChainPresentHook, (IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags), DXGIHookInstanceMutex)

        HRESULT hr = S_OK;
        if (SwapChainPresentHandler(pSwapChain, SyncInterval, Flags, &hr, DX11SwapChainPresentHookNext))
            return hr;

        return DX11SwapChainPresentHookNext(pSwapChain, SyncInterval, Flags);
    }

    DEFINE_HOOK_SAFE_EX(HRESULT, DX11SwapChainResizeHook, (IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags), DXGIHookInstanceMutex)

        HRESULT hr = S_OK;
        if (SwapChainResizeHandler(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, &hr, DX11SwapChainResizeHookNext))
            return hr;

        return DX11SwapChainResizeHookNext(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }
    
    DEFINE_HOOK_SAFE_EX(HRESULT, DX12FactoryCreateSwapChainHook, (IDXGIFactory* This, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain), DXGIHookInstanceMutex)

        IGOLogWarn("DX12FactoryCreateSwapChain");

        HRESULT hr = DX12FactoryCreateSwapChainHookNext(This, pDevice, pDesc, ppSwapChain);
        if (SUCCEEDED(hr))
                DX12Hook::FactoryCreateSwapChainNotify(This, pDevice, pDesc, *ppSwapChain);

        return hr;
    }

    DEFINE_HOOK_SAFE_EX(ULONG, DX12SwapChainReleaseHook, (IDXGISwapChain* pSwapChain), DXGIHookInstanceMutex)

        ULONG refCount = 0;
        if (!SwapChainReleaseHandler(pSwapChain, &refCount, DX12SwapChainReleaseHookNext))
            refCount = DX12SwapChainReleaseHookNext(pSwapChain);

        if (!refCount)
            DX12Hook::SwapChainReleasedNotify(pSwapChain);

        return refCount;
    }

    DEFINE_HOOK_SAFE_EX(HRESULT, DX12SwapChainPresentHook, (IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags), DXGIHookInstanceMutex)

        HRESULT hr = S_OK;
        if (SwapChainPresentHandler(pSwapChain, SyncInterval, Flags, &hr, DX12SwapChainPresentHookNext))
            return hr;

        return DX12SwapChainPresentHookNext(pSwapChain, SyncInterval, Flags);
    }

    DEFINE_HOOK_SAFE_EX(HRESULT, DX12SwapChainResizeBuffersHook, (IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags), DXGIHookInstanceMutex)

        HRESULT hr = S_OK;
        if (SwapChainResizeBuffersHandler(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, &hr, DX12SwapChainResizeBuffersHookNext))
            return hr;

        return DX12SwapChainResizeBuffersHookNext(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }

    DEFINE_HOOK_SAFE_EX(HRESULT, DX12SwapChainResizeBuffers1Hook, (IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue), DXGIHookInstanceMutex)

        HRESULT hr = S_OK;
        if (!SwapChainResizeBuffers1Handler(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue, &hr, DX12SwapChainResizeBuffers1HookNext))
            hr = DX12SwapChainResizeBuffers1HookNext(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);

        DX12Hook::SwapChainResizeBuffers1Notify(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue, hr);

        return hr;
    }

#pragma endregion

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // This method is to load the pre-computed offsets for all the D3D APIs currently loaded / check whether we share the
    // SwapChain method offsets / redirect to the proper API.
	void DXGIHook::Setup()
	{
        // We call this each time to load a DLL in case the ones we are looking for a "sub-loads" of other modules - so don't lock
        // on this and have a potential deadlock - just try really hard!
        OriginIGO::AutoTryFutex m(DXGIHookInstanceMutex);
        if (!m.Locked())
        {
            IGOLogWarn("Avoided deadlock - skipping DLL checks");
            return;
        }

        // Are we already using an API?
        if (mDXGIHookingCtxt.dx10InUse || mDXGIHookingCtxt.dx11InUse || mDXGIHookingCtxt.dx12InUse)
            return;

#ifndef LDR_IS_DATAFILE
#define LDR_IS_DATAFILE(handle)         (((ULONG_PTR)(handle)) & (ULONG_PTR)1)
#endif
#ifndef LDR_IS_IMAGEMAPPING
#define LDR_IS_IMAGEMAPPING(handle)     (((ULONG_PTR)(handle)) & (ULONG_PTR)2)
#endif
#ifndef LDR_IS_RESOURCE
#define LDR_IS_RESOURCE(handle)         (LDR_IS_IMAGEMAPPING(handle) || LDR_IS_DATAFILE(handle))
#endif

#pragma region DX10
		if (!mDXGIHookingCtxt.dx10Initialized)
		{
            // Did we already load the offsets/just having problems hooking before of 3rd party DLL?
            if (mDXGIHookingCtxt.dx10Offsets.version == 0)
            {
                // Do we have the DLLs we require?
                HMODULE hDXGI = GetModuleHandle(L"dxgi.dll");
                if (LDR_IS_RESOURCE(hDXGI))
                    IGOLogWarn("USING DXGI.DLL LOADED AS RESOURCE!!!");

                HMODULE hD3D10 = GetModuleHandle(L"d3d10.dll");
                if (LDR_IS_RESOURCE(hD3D10))
                    IGOLogWarn("USING D3D10.DLL LOADED AS RESOURCE!!!");

                if (hDXGI && hD3D10)
                {
                    IGOLogInfo("Reading DX10 offsets");
                    ReadPrecomputedOffsets("DX10",  OriginIGO::TelemetryRenderer_DX10, hD3D10, hDXGI, &mDXGIHookingCtxt.dx10Offsets, sizeof(mDXGIHookingCtxt.dx10Offsets));
                    if (mDXGIHookingCtxt.dx10Offsets.version == 0)
                    {
                        // We couldn't find the offsets -> consider that we're done
                        mDXGIHookingCtxt.dx10Initialized = true;
                        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "Failed to load pre-computed offsets."));
                    }

                    else
                    {
                        // Also check we have valid data to work with
                        IGOAPIInfo_V1* offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx10Offsets);
                        if (offsets->DX10.swapChainReleaseOffset == 0 || offsets->DX10.swapChainPresentOffset == 0 || offsets->DX10.swapChainResizeOffset == 0)
                        {
                            // No good!
                            mDXGIHookingCtxt.dx10Initialized = true;
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "Invalid pre-computed offsets."));
                        }

                        // Meaning we are pointing to methods!
                        offsets->DX10.swapChainReleaseAddr = static_cast<IGOAPIDXReleaseFcn>(ValidateMethod(hDXGI, offsets->DX10.swapChainReleaseOffset, OriginIGO::TelemetryRenderer_DX10));
                        offsets->DX10.swapChainPresentAddr = static_cast<IGOAPIDXPresentFcn>(ValidateMethod(hDXGI, offsets->DX10.swapChainPresentOffset, OriginIGO::TelemetryRenderer_DX10));
                        offsets->DX10.swapChainResizeAddr  = static_cast<IGOAPIDXResizeFcn>(ValidateMethod(hDXGI, offsets->DX10.swapChainResizeOffset, OriginIGO::TelemetryRenderer_DX10));
                        if (!offsets->DX10.swapChainReleaseAddr || !offsets->DX10.swapChainPresentOffset || !offsets->DX10.swapChainResizeOffset)
                        {
                            // No good!
                            mDXGIHookingCtxt.dx10Initialized = true;
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX10, "Pre-computed offsets don't match live data."));
                        }
                    }
                }
            }

            // Do we need to hook missing methods? Make sure we don't override other hooks if we have the same offsets
            if (mDXGIHookingCtxt.dx10Offsets.version != 0 && !mDXGIHookingCtxt.dx10Initialized)
            {
                IGOAPIInfo_V1* dx10Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx10Offsets);
                IGOAPIInfo_V1* dx11Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx11Offsets);
                IGOAPIInfo_V1* dx12Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx12Offsets);

                bool initialized = true;
                if ((!isDX11SwapChainReleaseHooked || dx11Offsets->DX11.swapChainReleaseAddr != dx10Offsets->DX10.swapChainReleaseAddr)
                    && (!isDX12SwapChainReleaseHooked || dx12Offsets->DX12.swapChainReleaseAddr != dx10Offsets->DX10.swapChainReleaseAddr))
                {
                    HOOKCODE_SAFE(dx10Offsets->DX10.swapChainReleaseAddr, DX10SwapChainReleaseHook);
                    initialized &= isDX10SwapChainReleaseHooked;
                }

                if ((!isDX11SwapChainPresentHooked || dx11Offsets->DX11.swapChainPresentAddr != dx10Offsets->DX10.swapChainPresentAddr)
                    && (!isDX12SwapChainPresentHooked || dx12Offsets->DX12.swapChainPresentAddr != dx10Offsets->DX10.swapChainPresentAddr))
                {
                    HOOKCODE_SAFE(dx10Offsets->DX10.swapChainPresentAddr, DX10SwapChainPresentHook);
                    initialized &= isDX10SwapChainPresentHooked;
                }

                if (!isDX11SwapChainResizeHooked || dx11Offsets->DX11.swapChainResizeAddr != dx10Offsets->DX10.swapChainResizeAddr)
                {
                    HOOKCODE_SAFE(dx10Offsets->DX10.swapChainResizeAddr, DX10SwapChainResizeHook);
                    initialized &= isDX10SwapChainResizeHooked;
                }

                mDXGIHookingCtxt.dx10Initialized = initialized;
            }
        }
#pragma endregion

#pragma region DX11
		if (!mDXGIHookingCtxt.dx11Initialized)
		{
            // Did we already load the offsets/just having problems hooking before of 3rd party DLL?
            if (mDXGIHookingCtxt.dx11Offsets.version == 0)
            {
                // Do we have the DLLs we require?
                HMODULE hDXGI = GetModuleHandle(L"dxgi.dll");
                if (LDR_IS_RESOURCE(hDXGI))
                    IGOLogWarn("USING DXGI.DLL LOADED AS RESOURCE!!!");

                HMODULE hD3D11 = GetModuleHandle(L"d3d11.dll");
                if (LDR_IS_RESOURCE(hD3D11))
                    IGOLogWarn("USING D3D11.DLL LOADED AS RESOURCE!!!");

                if (hDXGI && hD3D11)
                {
                    IGOLogInfo("Reading DX11 offsets");
                    ReadPrecomputedOffsets("DX11", OriginIGO::TelemetryRenderer_DX11, hD3D11, hDXGI, &mDXGIHookingCtxt.dx11Offsets, sizeof(mDXGIHookingCtxt.dx11Offsets));
                    if (mDXGIHookingCtxt.dx11Offsets.version == 0)
                    {
                        // We couldn't find the offsets -> consider that we're done
                        mDXGIHookingCtxt.dx11Initialized = true;
                        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "Failed to load pre-computed offsets."));
                    }

                    else
                    {
                        // Also check we have valid data to work with
                        IGOAPIInfo_V1* offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx11Offsets);
                        if (offsets->DX11.swapChainReleaseOffset == 0 || offsets->DX11.swapChainPresentOffset == 0 || offsets->DX11.swapChainResizeOffset == 0)
                        {
                            // No good!
                            mDXGIHookingCtxt.dx11Initialized = true;
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "Invalid pre-computed offsets."));
                        }

                        // Meaning we are pointing to methods!
                        offsets->DX11.swapChainReleaseAddr = static_cast<IGOAPIDXReleaseFcn>(ValidateMethod(hDXGI, offsets->DX11.swapChainReleaseOffset, OriginIGO::TelemetryRenderer_DX11));
                        offsets->DX11.swapChainPresentAddr = static_cast<IGOAPIDXPresentFcn>(ValidateMethod(hDXGI, offsets->DX11.swapChainPresentOffset, OriginIGO::TelemetryRenderer_DX11));
                        offsets->DX11.swapChainResizeAddr  = static_cast<IGOAPIDXResizeFcn>(ValidateMethod(hDXGI, offsets->DX11.swapChainResizeOffset, OriginIGO::TelemetryRenderer_DX11));
                        if (!offsets->DX11.swapChainReleaseAddr || !offsets->DX11.swapChainPresentOffset || !offsets->DX11.swapChainResizeOffset)
                        {
                            // No good!
                            mDXGIHookingCtxt.dx11Initialized = true;
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX11, "Pre-computed offsets don't match live data."));
                        }
                    }
                }
            }

            // Do we need to hook missing methods? Make sure we don't override other hooks if we have the same offsets
            if (mDXGIHookingCtxt.dx11Offsets.version != 0 && !mDXGIHookingCtxt.dx11Initialized)
            {
                IGOAPIInfo_V1* dx10Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx10Offsets);
                IGOAPIInfo_V1* dx11Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx11Offsets);
                IGOAPIInfo_V1* dx12Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx12Offsets);
                
                bool initialized = true;
                if ((!isDX10SwapChainReleaseHooked || dx10Offsets->DX10.swapChainReleaseAddr != dx11Offsets->DX11.swapChainReleaseAddr)
                    && (!isDX12SwapChainReleaseHooked || dx12Offsets->DX12.swapChainReleaseAddr != dx11Offsets->DX11.swapChainReleaseAddr))
                {
                    HOOKCODE_SAFE(dx11Offsets->DX11.swapChainReleaseAddr, DX11SwapChainReleaseHook);
                    initialized &= isDX11SwapChainReleaseHooked;
                }

                if ((!isDX10SwapChainPresentHooked || dx10Offsets->DX10.swapChainPresentAddr != dx11Offsets->DX11.swapChainPresentAddr)
                    && (!isDX12SwapChainPresentHooked || dx12Offsets->DX12.swapChainPresentAddr != dx11Offsets->DX11.swapChainPresentAddr))
                {
                    HOOKCODE_SAFE(dx11Offsets->DX11.swapChainPresentAddr, DX11SwapChainPresentHook);
                    initialized &= isDX11SwapChainPresentHooked;
                }

                if (!isDX10SwapChainResizeHooked || dx10Offsets->DX10.swapChainResizeAddr != dx11Offsets->DX11.swapChainResizeAddr)
                {
                    HOOKCODE_SAFE(dx11Offsets->DX11.swapChainResizeAddr, DX11SwapChainResizeHook);
                    initialized &= isDX11SwapChainResizeHooked;
                }

                mDXGIHookingCtxt.dx11Initialized = initialized;
            }
        }
#pragma endregion

#pragma region DX12
		if (!mDXGIHookingCtxt.dx12Initialized)
		{
            // Did we already load the offsets/just having problems hooking before of 3rd party DLL?
            if (mDXGIHookingCtxt.dx12Offsets.version == 0)
            {
                // Do we have the DLLs we require?
                HMODULE hDXGI = GetModuleHandle(L"dxgi.dll");
                if (LDR_IS_RESOURCE(hDXGI))
                    IGOLogWarn("USING DXGI.DLL LOADED AS RESOURCE!!!");

                HMODULE hD3D12 = GetModuleHandle(L"d3d12.dll");
                if (LDR_IS_RESOURCE(hD3D12))
                    IGOLogWarn("USING D3D12.DLL LOADED AS RESOURCE!!!");

                if (hDXGI && hD3D12)
                {
                    IGOLogInfo("Reading DX12 offsets");
                    ReadPrecomputedOffsets("DX12", OriginIGO::TelemetryRenderer_DX12, hD3D12, hDXGI, &mDXGIHookingCtxt.dx12Offsets, sizeof(mDXGIHookingCtxt.dx12Offsets));
                    if (mDXGIHookingCtxt.dx12Offsets.version == 0)
                    {
                        // We couldn't find the offsets -> consider that we're done
                        mDXGIHookingCtxt.dx12Initialized = true;
                        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX12, "Failed to load pre-computed offsets."));
                    }

                    else
                    {
                        // Also check we have valid data to work with
                        IGOAPIInfo_V1* offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx12Offsets);
                        if (offsets->DX12.factoryCreateSwapChainOffset == 0 || offsets->DX12.swapChainReleaseOffset == 0 
                            || offsets->DX12.swapChainPresentOffset == 0 || offsets->DX12.swapChainResizeBuffersOffset == 0 || offsets->DX12.swapChainResizeBuffers1Offset == 0)
                        {
                            // No good!
                            mDXGIHookingCtxt.dx12Initialized = true;
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX12, "Invalid pre-computed offsets."));
                        }

                        // Meaning we are pointing to methods!
                        offsets->DX12.factoryCreateSwapChainAddr = static_cast<IGOAPIDXCreateSwapChainFcn>(ValidateMethod(hDXGI, offsets->DX12.factoryCreateSwapChainOffset, OriginIGO::TelemetryRenderer_DX12));
                        offsets->DX12.swapChainReleaseAddr = static_cast<IGOAPIDXReleaseFcn>(ValidateMethod(hDXGI, offsets->DX12.swapChainReleaseOffset, OriginIGO::TelemetryRenderer_DX12));
                        offsets->DX12.swapChainPresentAddr = static_cast<IGOAPIDXPresentFcn>(ValidateMethod(hDXGI, offsets->DX12.swapChainPresentOffset, OriginIGO::TelemetryRenderer_DX12));
                        offsets->DX12.swapChainResizeBuffersAddr  = static_cast<IGOAPIDXResizeBuffersFcn>(ValidateMethod(hDXGI, offsets->DX12.swapChainResizeBuffersOffset, OriginIGO::TelemetryRenderer_DX12));
                        offsets->DX12.swapChainResizeBuffers1Addr  = static_cast<IGOAPIDXResizeBuffers1Fcn>(ValidateMethod(hDXGI, offsets->DX12.swapChainResizeBuffers1Offset, OriginIGO::TelemetryRenderer_DX12));
                        if (!offsets->DX12.factoryCreateSwapChainAddr || !offsets->DX12.swapChainReleaseAddr || !offsets->DX12.swapChainPresentOffset 
                            || !offsets->DX12.swapChainResizeBuffersOffset || !offsets->DX12.swapChainResizeBuffers1Offset)
                        {
                            // No good!
                            mDXGIHookingCtxt.dx12Initialized = true;
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX12, "Pre-computed offsets don't match live data."));
                        }
                    }
                }
            }

            // Do we need to hook missing methods? Make sure we don't override other hooks if we have the same offsets
            if (mDXGIHookingCtxt.dx12Offsets.version != 0 && !mDXGIHookingCtxt.dx12Initialized)
            {
                IGOAPIInfo_V1* dx10Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx10Offsets);
                IGOAPIInfo_V1* dx11Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx11Offsets);
                IGOAPIInfo_V1* dx12Offsets = static_cast<IGOAPIInfo_V1*>(&mDXGIHookingCtxt.dx12Offsets);
                
                bool initialized = true;
                if ((!isDX10SwapChainReleaseHooked || dx10Offsets->DX10.swapChainReleaseAddr != dx12Offsets->DX12.swapChainReleaseAddr)
                    && (!isDX11SwapChainReleaseHooked || dx11Offsets->DX11.swapChainReleaseAddr != dx12Offsets->DX12.swapChainReleaseAddr))
                {
                    HOOKCODE_SAFE(dx12Offsets->DX12.swapChainReleaseAddr, DX12SwapChainReleaseHook);
                    initialized &= isDX12SwapChainReleaseHooked;
                }

                if ((!isDX10SwapChainPresentHooked || dx10Offsets->DX10.swapChainPresentAddr != dx12Offsets->DX12.swapChainPresentAddr)
                    && (!isDX11SwapChainPresentHooked || dx11Offsets->DX11.swapChainPresentAddr != dx12Offsets->DX12.swapChainPresentAddr))
                {
                    HOOKCODE_SAFE(dx12Offsets->DX12.swapChainPresentAddr, DX12SwapChainPresentHook);
                    initialized &= isDX12SwapChainPresentHooked;
                }

                // ResizeBuffers for DX12 = Resize for DX10/DX11
                if ((!isDX10SwapChainResizeHooked || dx10Offsets->DX10.swapChainResizeAddr != dx12Offsets->DX12.swapChainResizeBuffersAddr)
                    && (!isDX11SwapChainResizeHooked || dx11Offsets->DX11.swapChainResizeAddr != dx12Offsets->DX12.swapChainResizeBuffersAddr))
                {
                    HOOKCODE_SAFE(dx12Offsets->DX12.swapChainResizeBuffersAddr, DX12SwapChainResizeBuffersHook);
                    initialized &= isDX12SwapChainResizeBuffersHooked;
                }

                HOOKCODE_SAFE(dx12Offsets->DX12.swapChainResizeBuffers1Addr, DX12SwapChainResizeBuffers1Hook);
                initialized &= isDX12SwapChainResizeBuffers1Hooked;

                HOOKCODE_SAFE(dx12Offsets->DX12.factoryCreateSwapChainAddr, DX12FactoryCreateSwapChainHook);
                initialized &= isDX12FactoryCreateSwapChainHooked;

                // Make sure we have what we need to synchronize rendering!
                // This check can go once we stop supporting XP/use Windows SDK >= 10
                initialized &= GetNotXPSupportedAPIs()->createEventEx != NULL;

                mDXGIHookingCtxt.dx12Initialized = initialized;
            }
        }
#pragma endregion
	}

    bool DXGIHook::IsDX10Hooked()
    {
        return mDXGIHookingCtxt.dx10Initialized;
    }

    bool DXGIHook::IsDX11Hooked()
    {
        return mDXGIHookingCtxt.dx11Initialized;
    }

    bool DXGIHook::IsDX12Hooked()
    {
        return mDXGIHookingCtxt.dx12Initialized;
    }

}
#endif // ORIGIN_PC
