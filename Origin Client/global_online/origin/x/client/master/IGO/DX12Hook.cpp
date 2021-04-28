///////////////////////////////////////////////////////////////////////////////
// DX12Hook.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(ORIGIN_PC)

#include "IGO.h"
#include "DX12Hook.h"
#include "DX12Surface.h"
#include "resource.h"

#include "../IGOProxy/IGOAPIInfo.h"

#include "HookAPI.h"
#include "IGOApplication.h"
#include "Helpers.h"
#include "DX12ResourceProvider.h"
//#include "combaseapi.h"

#include "EASTL/hash_map.h"
#include "IGOTrace.h"
#include "InputHook.h"
#include "IGOLogger.h"

#include "ScreenCopyDx12.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "IGOSharedStructs.h"
#include "madCHook.h"
#include "IGOTelemetry.h"

#ifdef DEBUG
#include <Initguid.h>
#include <dxgidebug.h>
#endif

#ifndef D3D12_SDK_VERSION
#define D3D12_SDK_VERSION 12.0
#endif

//#define BYPASS_OIG_IN_HOOKS

namespace OriginIGO
{
	EA::Thread::Futex DX12Hook::mInstanceHookMutex;

	extern DWORD gPresentHookThreadId;									// Keep track of the current render thread in case we need to check if we are in a valid context + used to trigger our render timeout check				
	extern volatile DWORD gPresentHookCalled;							// Used to let IGO know we're still alive and kicking
	extern volatile bool gTestCooperativeLevelHook_or_ResetHook_Called; // Used to help with our renderer timeout check

	extern HINSTANCE gInstDLL;											// Our unique DLL instance
	extern bool gInputHooked;											// Did we hook the inputs?
	extern volatile bool gQuitting;										// Are we quitting the game?

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Unused test tracker code for barrier resource - waiting on a real game to see if we need that stuff.
#if 0
	class DX12RenderTargetTracker
	{
	public:
		DX12RenderTargetTracker()
		{
			mTrackedCmdLists.reserve(200);
			memset(&mLatestTargetInfo, 0, sizeof(mLatestTargetInfo));
		}

		void add(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* barrierResource)
		{
			if (!cmdList)
				return;

			TrackedCmdLists::iterator iter = mTrackedCmdLists.begin();
			for (; iter != mTrackedCmdLists.end(); ++iter)
			{
				if (iter->cmdList == cmdList)
				{
					iter->targetInfo.barrierResource = barrierResource;
					return;
				}
			}

			TrackedCmdList newItem;
			memset(&newItem, 0, sizeof(newItem));
			newItem.cmdList = cmdList;
			newItem.targetInfo.barrierResource = barrierResource;

			mTrackedCmdLists.push_back(newItem);
		}

		void add(ID3D12GraphicsCommandList* cmdList, UINT numRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetDescriptors, BOOL rtsSingleHandleToDescriptorRange)
		{
			if (!cmdList)
				return;

			TrackedCmdLists::iterator iter = mTrackedCmdLists.begin();
			for (; iter != mTrackedCmdLists.end(); ++iter)
			{
				if (iter->cmdList == cmdList)
				{
					iter->targetInfo.renderTargetDescriptors = renderTargetDescriptors;
					iter->targetInfo.rtsSingleHandleToDescriptorRange = rtsSingleHandleToDescriptorRange;
					iter->targetInfo.numRenderTargetDescriptors = numRenderTargetDescriptors;

					return;
				}
			}

			TrackedCmdList newItem;
			memset(&newItem, 0, sizeof(newItem));
			newItem.cmdList = cmdList;
			newItem.targetInfo.renderTargetDescriptors = renderTargetDescriptors;
			newItem.targetInfo.rtsSingleHandleToDescriptorRange = rtsSingleHandleToDescriptorRange;
			newItem.targetInfo.numRenderTargetDescriptors = numRenderTargetDescriptors;

			mTrackedCmdLists.push_back(newItem);
		}

		void close(ID3D12GraphicsCommandList* cmdList)
		{
		}

		void remove(ID3D12GraphicsCommandList* cmdList)
		{
			if (!cmdList)
				return;

			TrackedCmdLists::iterator iter = mTrackedCmdLists.begin();
			for (; iter != mTrackedCmdLists.end(); ++iter)
			{
				if (iter->cmdList == cmdList)
				{
					mTrackedCmdLists.erase(iter);
					return;
				}
			}
		}

		void execute(ID3D12CommandQueue* This, ID3D12CommandList* const* cmdLists, UINT count)
		{
			if (!cmdLists || count == 0)
				return;

			// Start from the last cl to get the last PRESENT call setup
			bool targetInfoFound = false;
			for (int idx = count - 1; !targetInfoFound && idx >= 0; --idx)
			{
				ID3D12CommandList* cmdList = cmdLists[idx];

				TrackedCmdLists::reverse_iterator iter = mTrackedCmdLists.rbegin();
				for (; iter != mTrackedCmdLists.rend(); ++iter)
				{
					if (iter->cmdList == cmdList)
					{
						// Does it have all the info we need?
						if (iter->targetInfo.barrierResource) // HACK -  && iter->targetInfo.numRenderTargetDescriptors > 0 && iter->targetInfo.renderTargetDescriptors != NULL)
						{
							mLatestTargetInfo = iter->targetInfo;
							mLatestTargetInfo.cmdQueue = This;
							targetInfoFound = true;
							break;
						}
					}
				}
			}

			// Same but because hopefully we remove items from the back first
			for (int idx = count - 1; idx >= 0; --idx)
			{
				for (int idx = count - 1; !targetInfoFound && idx >= 0; --idx)
				{
					ID3D12CommandList* cmdList = cmdLists[idx];

					TrackedCmdLists::reverse_iterator iter = mTrackedCmdLists.rbegin();
					for (; iter != mTrackedCmdLists.rend(); ++iter)
					{
						if (iter->cmdList == cmdList)
						{
							mTrackedCmdLists.erase(iter);
							break;
						}
					}
				}
			}


			if (!targetInfoFound)
			{
				// HACK - need to figure out really what we need to do here!!!!
				memset(&mLatestTargetInfo, 0, sizeof(mLatestTargetInfo));
				mLatestTargetInfo.cmdQueue = This;
			}
		}

		ID3D12CommandQueue* FindCommandQueue(IDXGISwapChain* swapChain, UINT backbufferIdx)
		{
			// TODO: IMPLEMENT THIS PROPERLY!
			if (mLatestTargetInfo.cmdQueue)
			{
				return mLatestTargetInfo.cmdQueue;
			}

			return NULL;
		}

		bool extractCurrentTarget(DX12ResourceProvider::TargetInfo* targetInfo)
		{
			if (targetInfo && mLatestTargetInfo.cmdQueue)
			{
				*targetInfo = mLatestTargetInfo;
				memset(&mLatestTargetInfo, 0, sizeof(mLatestTargetInfo));

				return true;
			}

			return false;
		}

	private:
		struct TrackedCmdList
		{
			ID3D12GraphicsCommandList* cmdList;
			DX12ResourceProvider::TargetInfo targetInfo;
		};

		typedef eastl::vector<TrackedCmdList> TrackedCmdLists;
		TrackedCmdLists mTrackedCmdLists;

		DX12ResourceProvider::TargetInfo mLatestTargetInfo;
	};
#endif
#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region SwapChain Command Queues tracker
	// Keep track of the command queues in use by a swapchain - I would think we always only have one swap chain,
	// but...
	class DX12CommandQueueTracker
	{
	public:
		DX12CommandQueueTracker()
		{
		}

		void CreateSwapChain(IDXGISwapChain* sc, IUnknown* queue)
		{
			if (!sc || !queue)
				return;

			IGOLogWarn("IDXGIFactory CreateSwapChain %p, device %p", sc, queue);

			// Discard previous entry
			RemoveSwapChain(sc, true);

			// Check that this is a direct command queue we can work with
			ID3D12CommandQueue* cmdQueue = ValidateCommandQueue(queue);
			if (cmdQueue)
			{
				SwapChainInfo info;
				info.instance = sc;
				info.queues.push_back(cmdQueue);

				mSwapChains.push_back(info);
			}
		}

		void ResizeBuffers1(IDXGISwapChain* sc, UINT bufferCount, IUnknown* const* queues)
		{
			if (!sc || bufferCount == 0 || !queues)
				return;

			IGOLogWarn("IDXGISwapChain ResizeBuffer1 %p, count=%d, device[0] %p", sc, bufferCount, queues[0]);

			// Discard previous entry
			RemoveSwapChain(sc, true);

			// Check that they are all direct command queues we can work with
			SwapChainInfo info;
			info.instance = sc;
			for (UINT idx = 0; idx < bufferCount; ++idx)
			{
				ID3D12CommandQueue* cmdQueue = ValidateCommandQueue(queues[idx]);
				if (!cmdQueue)
					return;

				info.queues.push_back(cmdQueue);
			}

			mSwapChains.push_back(info);
		}

		void RemoveSwapChain(IDXGISwapChain* sc, bool internalCall = false)
		{
			if (!internalCall)
				IGOLogWarn("Removing swapchain %p", sc);

			for (SwapChains::iterator iter = mSwapChains.begin(); iter != mSwapChains.end(); ++iter)
			{
				if (iter->instance == sc)
				{
					mSwapChains.erase(iter);
					break;
				}
			}
		}

		ID3D12CommandQueue* Queue(IDXGISwapChain* sc, UINT bufferIndex)
		{
			// Find command queue to use for a specific backbuffer
			for (SwapChains::const_iterator iter = mSwapChains.begin(); iter != mSwapChains.end(); ++iter)
			{
				if (iter->instance == sc)
				{
					if (iter->queues.size() == 1)
						return iter->queues[0];

					if (bufferIndex < iter->queues.size())
						return iter->queues[bufferIndex];

					break;
				}
			}

			return NULL;
		}

	private:
		ID3D12CommandQueue* ValidateCommandQueue(IUnknown* queue)
		{
			if (!queue)
				return NULL;

			// Right now, we only work with graphics/direct command queue!
			ID3D12CommandQueue* cmdQueue = NULL;
			HRESULT hr = queue->QueryInterface(IID_PPV_ARGS(&cmdQueue));
			if (FAILED(hr) || !queue)
			{
				IGOLogWarn("Swapchain not associated with command queue");
				return NULL;
			}

			D3D12_COMMAND_QUEUE_DESC queueDesc = cmdQueue->GetDesc();
			if (queueDesc.Type != D3D12_COMMAND_LIST_TYPE_DIRECT)
			{
				IGOLogWarn("Swapchain using unsupported command queue type %d", queueDesc.Type);
				return NULL;
			}

			return cmdQueue;
		}


		struct SwapChainInfo
		{
			IDXGISwapChain* instance;

			typedef eastl::vector<ID3D12CommandQueue*> CommandQueues;
			CommandQueues queues;
		};

		typedef eastl::vector<SwapChainInfo> SwapChains;
		SwapChains mSwapChains;
	};
#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Container for all info related to the DX12 hooking
	struct DX12HookingContext
	{
		DX12HookingContext()
		{
			ZeroMemory(this, sizeof(*this));
		}

		bool ready;							// Whether the OIG hooks are all setup/can safely render OIG layout
		IDXGISwapChain* swapChainInUse;		// Swap chain currently in use (ie called Present() on it)

		uint32_t hookedMask;				// Mask used to quickly check which methods we haven't hooked yet

		DX12ResourceProvider* provider;		// Manages the resources/frame command lists
		DX12CommandQueueTracker cmTracker;	// Tracks the command queues associated with the swapchain backbuffers
	};

	static DX12HookingContext DX12HookingCtxt;

	struct DX12VolatileRenderStates
	{
		DX12VolatileRenderStates()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Unused code patterns to dynamically lookup methods - keeping around in case we need to do this at some point (although not practical)
	CodePatternFinder::PatternSignature SwapChainPresentCodePatterns[] =
	{
		{
			// Dll ID
			{
				IMAGE_FILE_MACHINE_AMD64,	// Machine 
				7,							// NumberOfsections 
				1427340873,					// TimeDateStamp
				0,							// PointerToSymbolTable
				0,							// NumberOfSymbols
				240,						// SizeOfOptionalHeader
				8226						// Characteristics
			},
			{
				0x48, 0x8D, 0x6C, 0x24, 0xD9,				// lea         rbp, [rsp - 27h]
				0x48, 0x81, 0xEC, 0xE0, 0x00, 0x00, 0x00,	// sub  // rsp, 0E0h
				0x48, 0x8B, 0x05, 0x0D, 0x5B, 0x07, 0x00,	//mov  // rax, qword ptr[7FFFCA998030h]
				0x48, 0x33, 0xC4,							// xor         rax, rsp
				0x48, 0x89, 0x45, 0x1F,						// mov         qword ptr[rbp + 1Fh], rax
				0x83, 0x3D, 0x93, 0x5B, 0x07, 0x00, 0x00,	// cmp         dword ptr[7FFFCA9980C4h], 0
				0x45, 0x8B, 0xF8,							// mov         r15d, r8d
				0x8B, 0xF2,									// mov         esi, edx
				0x44, 0x89, 0x45, 0xBF,						// mov         dword ptr[rbp - 41h], r8d
				0x4C, 0x8B, 0xF1,							// mov         r14, rcx
				0x89, 0x55, 0xAF,							// mov         dword ptr[rbp - 51h], edx
				0x48, 0x89, 0x4D, 0xC7,						// mov         qword ptr[rbp - 39h], rcx
				0xC6, 0x45, 0xB7, 0x00,						// mov         byte ptr[rbp - 49h], 0
				0x0F, 0x85, 0x72, 0x7C, 0x02, 0x00,			// jne         00007FFFCA94A1C0
				0xF6, 0x05, 0x2B, 0x85, 0x07, 0x00, 0x02,	// test        byte ptr[7FFFCA99AA80h], 2
				0x0F, 0x85, 0xEB, 0x7C, 0x02, 0x00,			// jne         00007FFFCA94A246
				0x33, 0xC0,									// xor         eax, eax
				0xC7, 0x45, 0xCF, 0x01, 0x00, 0x00, 0x00,	// mov         dword ptr[rbp - 31h], 1
				0x44, 0x89, 0x7D, 0xD3,						// mov         dword ptr[rbp - 2Dh], r15d
				0x89, 0x45, 0xD7,							// mov         dword ptr[rbp - 29h], eax
				0x88, 0x45, 0xA8,							// mov         byte ptr[rbp - 58h], al
				0x88, 0x45, 0xA7,							// mov         byte ptr[rbp - 59h], al
				0x88, 0x45, 0xA9,							// mov         byte ptr[rbp - 57h], al
				0x4D, 0x85, 0xF6,							// test        r14, r14
				0x0F, 0x84, 0xF5, 0x7C, 0x02, 0x00			// je          00007FFFCA94A272
			},
			{},												// no need for a mask
			109
		}
	};

	CodePatternFinder::PatternSignature SerializeRootSignatureCodePatterns[] =
	{
		{
			// Dll ID - DEFAULT ENTRY
			{
				IMAGE_FILE_MACHINE_AMD64,	// Machine 
				0,							// NumberOfsections 
				0,							// TimeDateStamp - 0 = DEFAULT!
				0,							// PointerToSymbolTable
				0,							// NumberOfSymbols
				0,							// SizeOfOptionalHeader
				0							// Characteristics
			},
			{
				0x48, 0x83, 0xEC, 0x28,			    // sub         rsp, 28h
				0xE8, 0x00, 0x00, 0x00, 0x00,       // call        00007FFB35F557C0
				0x3D, 0x05, 0x40, 0x00, 0x80,       // cmp         eax, 80004005h
				0xB9, 0x57, 0x00, 0x07, 0x80,       // mov         ecx, 80070057h
				0x0F, 0x45, 0xC8,					// cmovne      ecx, eax
				0x8B, 0xC1,							// mov         eax, ecx
				0x48, 0x83, 0xC4, 0x28,				// add         rsp, 28h
				0xC3								// ret
			},
			{
				0xff, 0xff, 0xff, 0xff,			    // sub         rsp, 28h
				0xff, 0x00, 0x00, 0x00, 0x00,       // call        00007FFB35F557C0
				0xff, 0xff, 0xff, 0xff, 0xff,       // cmp         eax, 80004005h
				0xff, 0xff, 0xff, 0xff, 0xff,       // mov         ecx, 80070057h
				0xff, 0xff, 0xff,					// cmovne      ecx, eax
				0xff, 0xff,							// mov         eax, ecx
				0xff, 0xff, 0xff, 0xff,				// add         rsp, 28h
				0xff								// ret
			},
			29
		}
	};
#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IsSupportedRenderBuffer(DXGI_FORMAT format)
	{
		// TODO: not sure what to put in there yet
		if (format == DXGI_FORMAT_B8G8R8A8_UNORM 
			|| format == DXGI_FORMAT_R8G8B8A8_UNORM)
			return true;

		return false;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	void CreateScreenCopyDX12(void* device, void* swapChain, int widthOverride, int heightOverride)
	{
		if (gScreenCopy == NULL)
		{
			gScreenCopy = new ScreenCopyDx12();
			gScreenCopy->Create(device, swapChain, widthOverride, heightOverride);
		}
	}

	void DestroyScreenCopyDX12(bool forceReset)
	{
		if (gScreenCopy)
		{
			gScreenCopy->Destroy(&forceReset);
			delete gScreenCopy;
			gScreenCopy = NULL;
		}
	}

	void BuildProjectionMatrixDX12(int width, int height, int n, int f, D3DMATRIX* xForm)
	{
		ZeroMemory(xForm, sizeof(D3DMATRIX));

		xForm->_11 = 2.0f / (width == 0 ? 1 : width);
		xForm->_22 = 2.0f / (height == 0 ? 1 : height);
		xForm->_33 = -2.0f / (n - f);
		xForm->_34 = 2.0f / (n - f);
		xForm->_44 = 1.0f;
	}

	bool SaveRenderState(ID3D12Device* pDev, DX12VolatileRenderStates* renderStates)
	{
		return true;
	}

	void RestoreRenderState(ID3D12Device *pDev, const DX12VolatileRenderStates& renderStates)
	{
	}

#pragma region Unused hooks related to the tracking of the barrier resources - waiting on a real game to see if we need that stuff.

	//DEFINE_HOOK_SAFE_VOID(DX12CommandQueueExecuteCommandListsHook, (ID3D12CommandQueue* This, UINT NumCommandLists, ID3D12CommandList *const *ppCommandLists))

	//	EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
	//	if (!DX12HookingCtxt.ready)
	//	{
	//		DX12CommandQueueExecuteCommandListsHookNext(This, NumCommandLists, ppCommandLists);
	//		return;
	//	}

	//	D3D12_COMMAND_QUEUE_DESC desc = This->GetDesc();
	//	if (desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
	//		DX12HookingCtxt.tracker.execute(This, ppCommandLists, NumCommandLists);

	//	DX12CommandQueueExecuteCommandListsHookNext(This, NumCommandLists, ppCommandLists);
	//}

	//DEFINE_HOOK_SAFE(ULONG, DX12GraphicsCommandListReleaseHook, (ID3D12GraphicsCommandList* This))

	//	EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
	//	if (!DX12HookingCtxt.ready)
	//		return DX12GraphicsCommandListReleaseHookNext(This);

	//	return DX12GraphicsCommandListReleaseHookNext(This);
	//}

	//DEFINE_HOOK_SAFE(HRESULT, DX12GraphicsCommandListCloseHook, (ID3D12GraphicsCommandList* This))

	//	EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
	//	if (!DX12HookingCtxt.ready)
	//		return DX12GraphicsCommandListCloseHookNext(This);

	//	DX12HookingCtxt.tracker.close(This);

	//	return DX12GraphicsCommandListCloseHookNext(This);
	//}

	//DEFINE_HOOK_SAFE(HRESULT, DX12GraphicsCommandListResetHook, (ID3D12GraphicsCommandList* This, ID3D12CommandAllocator *pAllocator, ID3D12PipelineState *pInitialState))

	//	EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
	//	if (!DX12HookingCtxt.ready)
	//		return DX12GraphicsCommandListResetHookNext(This, pAllocator, pInitialState);

	//	DX12HookingCtxt.tracker.remove(This);

	//	return DX12GraphicsCommandListResetHookNext(This, pAllocator, pInitialState);
	//}

	//DEFINE_HOOK_SAFE_VOID(DX12GraphicsCommandListClearStateHook, (ID3D12GraphicsCommandList* This, ID3D12PipelineState *pPipelineState))
	//
	//	EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
	//	if (!DX12HookingCtxt.ready)
	//	{
	//		DX12GraphicsCommandListClearStateHookNext(This, pPipelineState);
	//		return;
	//	}

	//	DX12HookingCtxt.tracker.remove(This);

	//	DX12GraphicsCommandListClearStateHookNext(This, pPipelineState);
	//}

	//DEFINE_HOOK_SAFE(HRESULT, DX12GraphicsCommandListResourceBarrierHook, (ID3D12GraphicsCommandList* This, UINT NumBarriers, const D3D12_RESOURCE_BARRIER* pBarriers))

	//	EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
	//	if (!DX12HookingCtxt.ready)
	//		return DX12GraphicsCommandListResourceBarrierHookNext(This, NumBarriers, pBarriers);

	//	// Are we transitioning to present a render target?
	//	if (pBarriers->Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION
	//		&& pBarriers->Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE
	//		&& pBarriers->Transition.StateAfter == D3D12_RESOURCE_STATE_PRESENT)
	//		DX12HookingCtxt.tracker.add(This, pBarriers->Transition.pResource);

	//	return DX12GraphicsCommandListResourceBarrierHookNext(This, NumBarriers, pBarriers);
	//}

	//DEFINE_HOOK_SAFE_VOID(DX12GraphicsCommandListOMSetRenderTargetsHook, (ID3D12GraphicsCommandList* This, UINT NumRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors, BOOL RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor))
	//
	//	EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
	//	if (!DX12HookingCtxt.ready)
	//		return DX12GraphicsCommandListOMSetRenderTargetsHookNext(This, NumRenderTargetDescriptors, pRenderTargetDescriptors, RTsSingleHandleToDescriptorRange, pDepthStencilDescriptor);

	//	// Are there targets we could potentially use to render OIG?
	//	if (pRenderTargetDescriptors && NumRenderTargetDescriptors > 0)
	//		DX12HookingCtxt.tracker.add(This, NumRenderTargetDescriptors, pRenderTargetDescriptors, RTsSingleHandleToDescriptorRange);

	//	return DX12GraphicsCommandListOMSetRenderTargetsHookNext(This, NumRenderTargetDescriptors, pRenderTargetDescriptors, RTsSingleHandleToDescriptorRange, pDepthStencilDescriptor);
	//}
#pragma endregion

	// Hook all the leftover hooks we require for OIG to properly operate - the plan here is never to unhook once we got all we
	// need EXCEPT if for some reason the game unloads the DLLs we rely on
	bool FinalizeSetupProcess(ID3D12Device* device, IDXGISwapChain* swapChain)
	{
		// Are we actually ready?
		if (DX12HookingCtxt.ready)
            return true;

        // First and foremost, we need our resource provider
        if (!DX12HookingCtxt.provider)
        {
            // We need access to this method to serialize our root signatures
            HMODULE hD3D12 = GetModuleHandle(L"d3d12.dll");
            PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(hD3D12, "D3D12SerializeRootSignature"));

            /* Keeping this code around as an example in case we need to do this kind of stuff later on
            if (!serializeRootSignature)
            {
            // Let's try it the hard way
            CodePatternFinder finder(SerializeRootSignatureCodePatterns, _countof(SerializeRootSignatureCodePatterns));
            CodePatternFinder::FindInstanceResult result = finder.GetMethod(hD3D12, (void**)&serializeRootSignature);
            if (result != CodePatternFinder::FindInstanceResult_ONE)
            serializeRootSignature = NULL;
            }
            */


            // Setup our resource provider
            DX12HookingCtxt.provider = new DX12ResourceProvider(serializeRootSignature);

#if DEBUG
            //CComPtr<ID3D12Debug> debugController;
            //PFN_D3D12_GET_DEBUG_INTERFACE d3d12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(GetProcAddress(hD3D12, "D3D12GetDebugInterface"));
            //HRESULT hr = d3d12GetDebugInterface(IID_PPV_ARGS(&debugController));
            //if (SUCCEEDED(hr))
            //	debugController->EnableDebugLayer();
#endif
        }

		// K, well first make sure we don't already have a renderer setup!
		{
			SAFE_CALL_LOCK_AUTO
			if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererDX12))
			{
				CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_DX12, "DX12 cannot finalize setup because different renderer in use (%d)", IGOApplication::instance()->rendererType()));
				return false;
			}
		}

		// Do we support the format?
		DXGI_SWAP_CHAIN_DESC desc;
		HRESULT hr = swapChain->GetDesc(&desc);
		if (FAILED(hr) || !IsSupportedRenderBuffer(desc.BufferDesc.Format))
		{
			CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_DX12, "DX12 can't access swap chain info/unsupported render buffer type (0x%08x / %d)", hr, desc.BufferDesc.Format));
			return false;
		}

		// Start checking which methods we are missing
        const uint32_t RequiredHookedMethodCnt = 0;
		uint32_t requiredHookedMethodMask = (1 << RequiredHookedMethodCnt) - 1;

#pragma region Unused code to dynamically hook queue-related methods for the resource barrier tracking. Would need to move to IGOProxy if we need to actually use that stuff.
#if 0
		if (DX12HookingCtxt.hookedMask == requiredHookedMethodMask)
		{
			// TODO: I guess I added a condition to reset the ready flag and I need to handle it here now!
			return false;
		}

		uint32_t currentMask = ... (1 << 5); // skipping initial precomputed hooks

		// Time to deal with DX12 API itself
		{
			D3D12_COMMAND_QUEUE_DESC cmdQueueDesc;
			ZeroMemory(&cmdQueueDesc, sizeof(cmdQueueDesc));
			cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			CComPtr<ID3D12CommandQueue> cmdQueue;
			HRESULT hr = device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));
			if (SUCCEEDED(hr))
			{
				DX12_NAME_OBJECT(cmdQueue, L"SetupCmdQueue");

				currentMask <<= 1;
				if (!(currentMask & DX12HookingCtxt.hookedMask))
				{
					ID3D12CommandQueue* cmdQueuePtr = cmdQueue;
					HOOKCODE_SAFE(GetInterfaceMethod(cmdQueuePtr, 10), DX12CommandQueueExecuteCommandListsHook);
					if (isDX12CommandQueueExecuteCommandListsHooked)
						DX12HookingCtxt.hookedMask |= currentMask;
				}

				// A command allocator
				CComPtr<ID3D12CommandAllocator> cmdAllocator;
				hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
				if (SUCCEEDED(hr))
				{
					DX12_NAME_OBJECT(cmdAllocator, L"SetupCmdAllocator");

					CComPtr<ID3DBlob> pOutBlob;
					CD3DX12_DESCRIPTOR_RANGE descRange[4]; // Perfomance TIP: Order from most frequent to least frequent
					descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);		// 1 frequently changed diffuse - t0
					descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);		// 1 per window constant buffer - b0 - object xform
					descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);		// 1 per frame constant buffer - b1 - proj xform
					descRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);	// 1 static sampler - s0

					CD3DX12_ROOT_PARAMETER rootParameters[4];
					rootParameters[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
					rootParameters[1].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_ALL);
					rootParameters[2].InitAsDescriptorTable(1, &descRange[2], D3D12_SHADER_VISIBILITY_ALL);
					rootParameters[3].InitAsDescriptorTable(1, &descRange[3], D3D12_SHADER_VISIBILITY_PIXEL);

					CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
					descRootSignature.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
					hr = DX12HookingCtxt.serializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, NULL);
					if (SUCCEEDED(hr))
					{
						CComPtr<ID3D12RootSignature> rootSignature;
						hr = device->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
						if (SUCCEEDED(hr))
						{
							DX12_NAME_OBJECT(rootSignature, L"SetupRootSignature");

							HRSRC resourceInfo = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGODX12VSFX), RT_RCDATA);
							HGLOBAL hRes = LoadResource(gInstDLL, resourceInfo);
							D3D12_SHADER_BYTECODE vertexShader;
							vertexShader.pShaderBytecode = LockResource(hRes);
							vertexShader.BytecodeLength = SizeofResource(gInstDLL, resourceInfo);

							resourceInfo = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOPSFX), RT_RCDATA);
							hRes = LoadResource(gInstDLL, resourceInfo);
							D3D12_SHADER_BYTECODE pixelShader;
							pixelShader.pShaderBytecode = LockResource(hRes);
							pixelShader.BytecodeLength = SizeofResource(gInstDLL, resourceInfo);

							CComPtr<ID3D12PipelineState> pso;
							D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
							ZeroMemory(&psoDesc, sizeof(psoDesc));
							D3D12_INPUT_ELEMENT_DESC layout[] =
							{
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
							};
							UINT numElements = sizeof(layout) / sizeof(layout[0]);

							psoDesc.InputLayout.pInputElementDescs = layout;
							psoDesc.InputLayout.NumElements = numElements;
							psoDesc.pRootSignature = rootSignature;
							psoDesc.VS = vertexShader;
							psoDesc.PS = pixelShader;
							psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
							psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
							psoDesc.DepthStencilState.DepthEnable = FALSE;
							psoDesc.DepthStencilState.StencilEnable = FALSE;
							psoDesc.SampleMask = UINT_MAX;
							psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
							psoDesc.NumRenderTargets = 1;
							psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
							psoDesc.SampleDesc.Count = 1;

							hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
							if (SUCCEEDED(hr))
							{
								DX12_NAME_OBJECT(pso, L"SetupPSO");

								CComPtr<ID3D12GraphicsCommandList> cmdList;
								hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, pso, IID_PPV_ARGS(&cmdList));
								if (SUCCEEDED(hr))
								{
									DX12_NAME_OBJECT(cmdList, L"SetupCommandList");
									ID3D12GraphicsCommandList* cmdListPtr = cmdList;

									//currentMask <<= 1;
									//if (!(currentMask & DX12HookingCtxt.hookedMask))
									//{
									//	HOOKCODE_SAFE(GetInterfaceMethod(cmdListPtr, 2), DX12GraphicsCommandListReleaseHook);
									//	if (isDX12GraphicsCommandListReleaseHooked)
									//		DX12HookingCtxt.hookedMask |= currentMask;
									//}

									currentMask <<= 1;
									if (!(currentMask & DX12HookingCtxt.hookedMask))
									{
										HOOKCODE_SAFE(GetInterfaceMethod(cmdListPtr, 9), DX12GraphicsCommandListCloseHook);
										if (isDX12GraphicsCommandListCloseHooked)
											DX12HookingCtxt.hookedMask |= currentMask;
									}

									currentMask <<= 1;
									if (!(currentMask & DX12HookingCtxt.hookedMask))
									{
										HOOKCODE_SAFE(GetInterfaceMethod(cmdListPtr, 10), DX12GraphicsCommandListResetHook);
										if (isDX12GraphicsCommandListResetHooked)
											DX12HookingCtxt.hookedMask |= currentMask;
									}

									currentMask <<= 1;
									if (!(currentMask & DX12HookingCtxt.hookedMask))
									{
										HOOKCODE_SAFE(GetInterfaceMethod(cmdListPtr, 11), DX12GraphicsCommandListClearStateHook);
										if (isDX12GraphicsCommandListClearStateHooked)
											DX12HookingCtxt.hookedMask |= currentMask;
									}

									currentMask <<= 1;
									if (!(currentMask & DX12HookingCtxt.hookedMask))
									{
										HOOKCODE_SAFE(GetInterfaceMethod(cmdListPtr, 26), DX12GraphicsCommandListResourceBarrierHook);
										if (isDX12GraphicsCommandListResourceBarrierHooked)
											DX12HookingCtxt.hookedMask |= currentMask;
									}

									currentMask <<= 1;
									if (!(currentMask & DX12HookingCtxt.hookedMask))
									{
										HOOKCODE_SAFE(GetInterfaceMethod(cmdListPtr, 46), DX12GraphicsCommandListOMSetRenderTargetsHook);
										if (isDX12GraphicsCommandListOMSetRenderTargetsHooked)
											DX12HookingCtxt.hookedMask |= currentMask;
									}
								}
							}
						}
					}
				}
			}
		}
#endif
#pragma endregion

		if (DX12HookingCtxt.hookedMask == requiredHookedMethodMask)
			DX12HookingCtxt.ready = true;

		return false; // Let's wait until the next loop before doing anything
	}

	// This is where we setup all the resources necessary to render IGO... except for textures!
	bool AllocateD3D12Objects(ID3D12Device* device, IDXGISwapChain3* swapChain, UINT backbufferIndex)
	{
		bool success = false;

		// Let's find the queue to use for the current backbuffer
		ID3D12CommandQueue* cmdQueue = DX12HookingCtxt.cmTracker.Queue(swapChain, backbufferIndex);
		if (cmdQueue)
		{
			success = DX12HookingCtxt.provider->Init(device, swapChain, cmdQueue);
			if (success)
			{
				if (!IGOApplication::instance())
					IGOApplication::createInstance(RendererDX12, DX12HookingCtxt.provider);
			}
		}
		else
			CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_DX12, "DX12 can't find command queue for swapchain %p/%d", swapChain, backbufferIndex));

		return success;
	}

	void ReleaseD3D12Objects(bool forceReset = false)
	{
		DestroyScreenCopyDX12(forceReset);

        if (DX12HookingCtxt.provider)
		    DX12HookingCtxt.provider->Reset(forceReset);
	}

	bool DX12HandleLostDevice(HRESULT hr)
	{
#ifndef DXGI_ERROR_DEVICE_REMOVED
#define DXGI_ERROR_DEVICE_REMOVED 0x887A0005
#endif
        if (hr != DXGI_ERROR_DEVICE_REMOVED)
            return false;

		TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_HARDWARE, TelemetryRenderer_DX12, "Detected TDR scenario");

		ReleaseD3D12Objects(true);
		IGOApplication::deleteInstance();

        return true;
	}

    // Notifications used to keep track of the set of swapchains/queues
    void DX12Hook::FactoryCreateSwapChainNotify(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain* pSwapChain)
    {
        EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
        DX12HookingCtxt.cmTracker.CreateSwapChain(pSwapChain, pDevice);
    }

    void DX12Hook::SwapChainResizeBuffers1Notify(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue, HRESULT result)
    {
        EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
        if (SUCCEEDED(result))
            DX12HookingCtxt.cmTracker.ResizeBuffers1(pSwapChain, BufferCount, ppPresentQueue);
        else
            DX12HookingCtxt.cmTracker.RemoveSwapChain(pSwapChain);
    }

    void DX12Hook::SwapChainReleasedNotify(IDXGISwapChain *pSwapChain)
    {
        EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);
        DX12HookingCtxt.cmTracker.RemoveSwapChain(pSwapChain);
    }

    ULONG DX12Hook::ReleaseHandler(IDXGISwapChain* pSwapChain, IGOAPIDXReleaseFcn ReleaseFcn)
    {
#ifdef BYPASS_OIG_IN_HOOKS
		return 0;
#else
        if (DX12HookingCtxt.swapChainInUse != NULL && DX12HookingCtxt.swapChainInUse != pSwapChain)
			IGOLogWarn("DX12SwapChainReleaseHook on different swap chain!");
		
		// I don't believe we need any SAFE_CALL_LOCK_ENTER/LEAVE here...
		ULONG ul = 0;
		IGOApplication* instance = IGOApplication::instance();
        if (!gCalledFromInsideIGO && DX12HookingCtxt.swapChainInUse == pSwapChain && instance)
		{
			gCalledFromInsideIGO++;
            pSwapChain->AddRef();
            ULONG currRefCount = ReleaseFcn(pSwapChain);
		
			if (currRefCount == 1)
			{
				// The swap chain is being destroyed
				DX12HookingCtxt.swapChainInUse = NULL;

				ReleaseD3D12Objects();
				IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!	
				IGOLogWarn("Releasing dx12 swap chain");
		
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
#endif
	}

    bool DX12Hook::ResizeBuffersHandler(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, HRESULT* Result, IGOAPIDXResizeBuffersFcn ResizeBuffersFcn)
    {
#ifdef BYPASS_OIG_IN_HOOKS
        return false;
#else
		EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);

        if (!pSwapChain || !Result || !ResizeBuffersFcn)
            return false;

        if (!DX12HookingCtxt.ready)
            return false;

		// Clear all objects
		SAFE_CALL_LOCK_ENTER
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::clearWindows, NULL);
		SAFE_CALL_LOCK_LEAVE

		if (IGOApplication::isPendingDeletion())
			IGOApplication::deleteInstance();

		// release other buffers
		ReleaseD3D12Objects();
		gTestCooperativeLevelHook_or_ResetHook_Called = true;

        HRESULT retval = ResizeBuffersFcn(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        if (!DX12HandleLostDevice(retval))
		{
            if (FAILED(retval))
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX12, "ResizeBuffers failed hr = %x.", retval));

			DXGI_SWAP_CHAIN_DESC desc = { 0 };
            HRESULT hr = pSwapChain->GetDesc(&desc);
			if (SUCCEEDED(hr))
			{
				gWindowedMode = desc.Windowed == TRUE;
				InputHook::HookWindow(desc.OutputWindow);
			}
			else
				IGOLogWarn("pSwapChain->GetDesc failed on resize -> no HWND!");

			SAFE_CALL_LOCK_ENTER
				SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
			    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, Width, Height);
			    SAFE_CALL(IGOApplication::instance(), &IGOApplication::reset);
			SAFE_CALL_LOCK_LEAVE
		}

        *Result = retval;
        return FAILED(retval); // lose it if we can't reinitialize the resources
#endif
	}

    bool DX12Hook::ResizeBuffers1Handler(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue, HRESULT* Result, IGOAPIDXResizeBuffers1Fcn ResizeBuffers1Fcn)
    {
#ifdef BYPASS_OIG_IN_HOOKS
        return false;
#else
		EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);

        if (!pSwapChain || !Result || !ResizeBuffers1Fcn)
            return false;

        if (!DX12HookingCtxt.ready)
            return false;

		// Clear all objects
		SAFE_CALL_LOCK_ENTER
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::clearWindows, NULL);
		SAFE_CALL_LOCK_LEAVE

			if (IGOApplication::isPendingDeletion())
				IGOApplication::deleteInstance();

		// release other buffers
		ReleaseD3D12Objects();
		gTestCooperativeLevelHook_or_ResetHook_Called = true;

        HRESULT retval = ResizeBuffers1Fcn(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
        if (!DX12HandleLostDevice(retval))
		{
            if (FAILED(retval))
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX12, "ResizeBuffers1 failed hr = %x.", retval));

			DXGI_SWAP_CHAIN_DESC desc = { 0 };
            HRESULT hr = pSwapChain->GetDesc(&desc);
			if (SUCCEEDED(hr))
			{
				gWindowedMode = desc.Windowed == TRUE;
				InputHook::HookWindow(desc.OutputWindow);
			}
			else
				IGOLogWarn("SwapChain GetDesc failed on resize -> no HWND!");

			SAFE_CALL_LOCK_ENTER
				SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, Width, Height);
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::reset);
			SAFE_CALL_LOCK_LEAVE
		}

        *Result = retval;
        return FAILED(retval); // lose it if we can't reinitialize the resources
#endif
	}

	// This is the first phase to our setup process: find the swap chain Present method -> if ever called, we will complete the setup (ie hook the additional device methods we rely on)
    bool DX12Hook::PresentHandler(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags, HRESULT* Result, IGOAPIDXPresentFcn PresentFcn)
    {
#ifdef BYPASS_OIG_IN_HOOKS
        return false;
#else
        EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);

		// Let our main IGO thread know that we're still alive and kicking
		gPresentHookCalled = GetTickCount();

		// We need the IDXGISwapChain3 interface to access the backbuffer to use for rendering
		CComPtr<IDXGISwapChain3> swapChain = NULL;
        HRESULT hr = pSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
		if (FAILED(hr) || !swapChain)
		{
			CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_DX12, "DX12SwapChainPresent called with no-IDXGISwapChain3"));
            *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
            return DX12HandleLostDevice(*Result);
		}

		// Get the device using this swap chain
		CComPtr<ID3D12Device> device = NULL;
		hr = swapChain->GetDevice(IID_PPV_ARGS(&device));

		DX12_NAME_OBJECT(device, L"PresentDevice");

		if (!FinalizeSetupProcess(device, swapChain)    // Is OIG setup done for DX12?
            || (Flags & DXGI_PRESENT_TEST))             // Do we need to do anything?
        {
            *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
			return DX12HandleLostDevice(*Result);
        }
        
		/////

		// Just in case we haven't hooked the inputs yet
		if (!gInputHooked && !gQuitting)
			InputHook::TryHookLater(&gInputHooked);

		// Did we postpone a reset until we were back on the render thread?
		if (IGOApplication::isPendingDeletion())
		{
			IGOApplication::deleteInstance();
			ReleaseD3D12Objects();
		}

		// We're going to need the swap chain current info, let's just get it now
		DXGI_SWAP_CHAIN_DESC scDesc;
		hr = swapChain->GetDesc(&scDesc);
		if (FAILED(hr))
		{
			CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_DX12, "DX12 can't access swap chain info/unsupported render buffer type (0x%08x)", hr));
            *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
			return DX12HandleLostDevice(*Result);
		}

		// Hook the window message handling functions if necessary
		if (!gQuitting)
			InputHook::HookWindow(scDesc.OutputWindow);

		gWindowedMode = scDesc.Windowed == TRUE;

		// Make sure we setup our resources for rendering
		UINT backbufferIndex = swapChain->GetCurrentBackBufferIndex();
		if (!AllocateD3D12Objects(device, swapChain, backbufferIndex))
		{
			IGOApplication::deleteInstance();
			ReleaseD3D12Objects();

            *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
			return DX12HandleLostDevice(*Result);
		}

		if (!IGOApplication::instance())
		{
			// Remember our thread id to ensure we do work on the proper thread - I don't think this applies to DX12 at all, but
			// I'll keep it around for the time being
			gPresentHookThreadId = GetCurrentThreadId();
		}


		SAFE_CALL_LOCK_ENTER
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::startTime);

			// Set the rendering resolution
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
			if (scDesc.OutputWindow)
			{
				RECT rect = { 0 };
				GetClientRect(scDesc.OutputWindow, &rect);
				UINT wndWidth = rect.right - rect.left;
				UINT wndHeight = rect.bottom - rect.top;
				SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, wndWidth, wndHeight);
			}
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setSRGB, ScreenCopyDx12::isSRGB_format(scDesc.BufferDesc.Format));
		SAFE_CALL_LOCK_LEAVE

		{
			DX12VolatileRenderStates renderStates;
			if (SaveRenderState(device, &renderStates))
			{
				uint32_t w = (uint32_t)scDesc.BufferDesc.Width;
				uint32_t h = (uint32_t)scDesc.BufferDesc.Height;
				SAFE_CALL_LOCKED(IGOApplication::instance(), &IGOApplication::setScreenSize, w, h);

				D3DMATRIX projMatrix;
				int nearZ = 1;
				int farZ = 10;
				BuildProjectionMatrixDX12(w, h, nearZ, farZ, &projMatrix);

				ID3D12CommandQueue* cmdQueue = DX12HookingCtxt.cmTracker.Queue(swapChain, backbufferIndex);
				if (cmdQueue)
				{
					DX12_NAME_OBJECT(cmdQueue, L"PresentCommandQueue");

					DX12ResourceProvider::TargetInfo targetInfo;
					targetInfo.cmdQueue = cmdQueue;
					targetInfo.swapChain = swapChain;
					targetInfo.currentBackbufferIdx = backbufferIndex;
					if (SUCCEEDED(hr))
					{
						if (TwitchManager::IsBroadCastingInitiated())
							CreateScreenCopyDX12(device, swapChain, w, h);
						else
							DestroyScreenCopyDX12(false);

						if (TwitchManager::IsBroadCastingInitiated())
						{
							TwitchManager::TTV_PollTasks();
							if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
							{
								bool s = SAFE_CALL(gScreenCopy, &IScreenCopy::Render, &targetInfo, NULL);
								if (!s)
									DestroyScreenCopyDX12(false);
							}
						}

						if (DX12HookingCtxt.provider->BeginCommandList(projMatrix, w, h, targetInfo))
						{
							SAFE_CALL_LOCKED(IGOApplication::instance(), &IGOApplication::render, DX12HookingCtxt.provider);
							DX12HookingCtxt.provider->EndCommandList();
						}
					}
				}

				RestoreRenderState(device, renderStates);
			}
		}

		SAFE_CALL_NOARGS_LOCKED(IGOApplication::instance(), &IGOApplication::stopTime);

		/////

        *Result = PresentFcn(pSwapChain, SyncInterval, Flags);
        return DX12HandleLostDevice(*Result);
#endif
	}

	void DX12Hook::Cleanup()
	{
		EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);

		DX12HookingCtxt.ready = false;
		ReleaseD3D12Objects();

		// Let's avoid a deadlock with the application instance - we can always delay its deletion
		SAFE_CALL_LOCK_ENTER
			if (IGOApplication::instance() != NULL && (IGOApplication::instance()->rendererType() == RendererDX12))
			{
				bool safeToCall = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isThreadSafe);
				SAFE_CALL_LOCK_LEAVE

					if (safeToCall)
						IGOApplication::deleteInstance();

					else
						IGOApplication::deleteInstanceLater();
			}
			else
			{
				SAFE_CALL_LOCK_LEAVE
			}

#if 0 // DEBUG
		{
			// Check for what we could be leaking
			HMODULE dxgiDebug = GetModuleHandle(L"DXGIDebug.dll");
			if (!dxgiDebug)
				LoadLibrary(L"DXGIDebug.dll");

			if (dxgiDebug)
			{
				typedef HRESULT(*WINAPI DXGIGetDebugInterfaceFn)(REFIID riid, void **ppDebug);
				DXGIGetDebugInterfaceFn getDebugInterface = reinterpret_cast<DXGIGetDebugInterfaceFn>(GetProcAddress(dxgiDebug, "DXGIGetDebugInterface"));
				if (getDebugInterface)
				{
					CComPtr<IDXGIDebug> debugController;
					HRESULT hr = getDebugInterface(IID_PPV_ARGS(&debugController));
					if (SUCCEEDED(hr))
					{
						hr = debugController->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
						if (FAILED(hr))
							IGOLogWarn("Failed to report live objects (0x%08x)", hr);
					}
				}

				FreeLibrary(dxgiDebug);
			}
		}
#endif
	}

    bool DX12Hook::IsValidDevice(IDXGISwapChain* swapChain)
    {
        if (!swapChain)
            return false;

        ID3D12Device* device = NULL;
        HRESULT hr = swapChain->GetDevice(__uuidof(device), (void **)&device);
        if (FAILED(hr) || !device)
            return false;

        RELEASE_IF(device);
        return true;
    }

	void DX12Hook::ForceTDR()
	{
		EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);

		if (DX12HookingCtxt.provider)
			DX12HookingCtxt.provider->ForceTDR();
	}

	void DX12Hook::ForceScreenCapture()
	{
		EA::Thread::AutoFutex m(DX12Hook::mInstanceHookMutex);

		ScreenCopyDx12::ResetScreenCaptures();
	}
}

#endif // ORIGIN_PC
