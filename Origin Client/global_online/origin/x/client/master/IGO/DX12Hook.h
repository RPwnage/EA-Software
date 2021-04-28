///////////////////////////////////////////////////////////////////////////////
// DX12Hook.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DX12HOOK_H
#define DX12HOOK_H

#include "IGO.h"

#if defined(ORIGIN_PC)
#include <DXGI.h>
#include "../IGOProxy/IGOAPIInfo.h"
#include "eathread/eathread_futex.h"

namespace OriginIGO 
{
	class DX12Hook
	{
	public:
		DX12Hook();
		~DX12Hook();

		static void Cleanup();
		
        static void FactoryCreateSwapChainNotify(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain* pSwapChain);
        static void SwapChainResizeBuffers1Notify(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue, HRESULT Result);
        static void SwapChainReleasedNotify(IDXGISwapChain *pSwapChain);

        static ULONG ReleaseHandler(IDXGISwapChain *pSwapChain, IGOAPIDXReleaseFcn ReleaseFcn);
        static bool PresentHandler(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags, HRESULT* Result, IGOAPIDXPresentFcn PresentFcn);
        static bool ResizeBuffersHandler(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, HRESULT* Result, IGOAPIDXResizeBuffersFcn ResizeBuffersFcn);
        static bool ResizeBuffers1Handler(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue, HRESULT* Result, IGOAPIDXResizeBuffers1Fcn ResizeBuffers1Fcn);

        static bool IsValidDevice(IDXGISwapChain* swapChain);

		// Accessed from Info Display for debugging
		static void ForceTDR();
		static void ForceScreenCapture();

	private:
   		static EA::Thread::Futex mInstanceHookMutex;
	};
}
#endif

#endif
