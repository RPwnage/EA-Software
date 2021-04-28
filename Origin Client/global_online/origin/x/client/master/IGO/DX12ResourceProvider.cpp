///////////////////////////////////////////////////////////////////////////////
// DX12RESOURCEPROVIDER_H.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "DX12ResourceProvider.h"

//#include <synchapi.h>
//#include <pix_win.h>

#include "IGOLogger.h"
#include "IGOSharedStructs.h"
#include "resource.h"
#include "HookAPI.h"
#include "IGOApplication.h"
#include "IGOTelemetry.h"
#include "Helpers.h"

//#define DEBUG_TEXTURES											// Add logs to help debugging texture management
//#define MULTIPLE_UPDATES_PER_FRAME_ARE_HANDLED_AT_THE_IPC_LEVEL	// We could get multiple updates for the same texture in one frame -> should fix that at the IPC-level, but for now...

namespace OriginIGO
{
	extern HINSTANCE gInstDLL;

	// DX12 APIs to use
#if 0 // In case we go back to hooking command queue/command list ops
	EXTERN_NEXT_FUNC(void, DX12CommandQueueExecuteCommandListsHook, (ID3D12CommandQueue* This, UINT Count, ID3D12CommandList *const *ppCommandLists))
	EXTERN_NEXT_FUNC(HRESULT, DX12GraphicsCommandListCloseHook, (ID3D12GraphicsCommandList* This))
	EXTERN_NEXT_FUNC(HRESULT, DX12GraphicsCommandListResetHook, (ID3D12GraphicsCommandList* This, ID3D12CommandAllocator *pAllocator, ID3D12PipelineState *pInitialState))
	EXTERN_NEXT_FUNC(HRESULT, DX12GraphicsCommandListResourceBarrierHook, (ID3D12GraphicsCommandList* This, UINT Count, const D3D12_RESOURCE_BARRIER* pBarriers))

	#define DX12CommandQueueExecuteCommandListsFcn(This, Count, ppCommandLists) DX12CommandQueueExecuteCommandListsHookNext(This, Count, ppCommandLists)
	#define DX12GraphicsCommandListCloseFcn(This) DX12GraphicsCommandListCloseHookNext(This)
	#define DX12GraphicsCommandListResetFcn(This, pAllocator, pInitialState) DX12GraphicsCommandListResetHookNext(This, pAllocator, pInitialState)
	#define DX12GraphicsCommandListResourceBarrierFcn(This, Count, pBarriers) DX12GraphicsCommandListResourceBarrierHookNext(This, Count, pBarriers)
#else
	#define DX12CommandQueueExecuteCommandListsFcn(This, Count, ppCommandLists) This->ExecuteCommandLists(Count, ppCommandLists)
	#define DX12GraphicsCommandListCloseFcn(This) This->Close()
	#define DX12GraphicsCommandListResetFcn(This, pAllocator, pInitialState) This->Reset(pAllocator, pInitialState)
	#define DX12GraphicsCommandListResourceBarrierFcn(This, Count, pBarriers) This->ResourceBarrier(Count, pBarriers)
#endif

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SetResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
	{
		D3D12_RESOURCE_BARRIER barrier = {};

		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = StateBefore;
		barrier.Transition.StateAfter = StateAfter;

		DX12GraphicsCommandListResourceBarrierFcn(commandList, 1, &barrier);
	}

	bool ExtractDeviceAndNodeMask(ID3D12CommandQueue* queue, ID3D12Device** device, UINT* nodeMask)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = queue->GetDesc();
		if (queueDesc.Type != D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 trying to use non-render(%d) command queue!", queueDesc.Type));
			return false;
		}

		if (nodeMask)
			*nodeMask = queueDesc.NodeMask;

		if (device)
		{
			HRESULT hr = queue->GetDevice(IID_PPV_ARGS(device));
			if (FAILED(hr))
			{
				CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 unable to access queue device!"));
				return false;
			}

			(*device)->Release();
		}

		return true;
	}

	void WaitForCompletion(ID3D12CommandQueue* cmdQueue, ID3D12Fence* fence, uint64_t currentFenceID)
	{
		IGOLogWarn("Wait on complete for queue=%p", cmdQueue);

		// Wait until we know all the pushed command lists have been processed
        static NotXPSupportedAPIs::CreateEventExFn createEventEx = GetNotXPSupportedAPIs()->createEventEx;
        HANDLE eventHandle = createEventEx ? createEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS) : NULL;
		if (eventHandle)
		{
			const uint64_t lastCompletedFence = fence->GetCompletedValue();
			const uint64_t currentFence = currentFenceID;

			// Signal and increment the current fence
			{
				if (cmdQueue)
					cmdQueue->Signal(fence, currentFence);

				if (lastCompletedFence < currentFence)
				{
					HRESULT hr = fence->SetEventOnCompletion(currentFence, eventHandle);
					if (SUCCEEDED(hr))
					{
						DWORD timeoutInMS = 1000;
						DWORD result = WaitForSingleObject(eventHandle, timeoutInMS);
						if (result != WAIT_OBJECT_0)
						{
							CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 timeout waiting on fence (%d/%d) for queue=%p", lastCompletedFence, currentFence, cmdQueue));
						}
					}

					else
						IGOLogWarn("Failed to setup queue fence to signal of end of all frames resource usage - hr=0x%08x - queue=%p", hr, cmdQueue);
				}
			}

			CloseHandle(eventHandle);
		}

		else
			IGOLogWarn("Unable to create event to properly wait on frame resource completion!!");
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Constant buffer structures
	struct DX12ResourceProvider::CBPerFrame
	{
		D3DMATRIX projMatrix;
	};

	struct DX12ResourceProvider::CBPerWindow
	{
		D3DMATRIX worldMatrix;
		float color[4];
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	class DX12Texture
	{
	public:
		DX12Texture(int texID, uint64_t fenceID, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuViewHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuViewHandle)
		{
			mTexID = texID;
			mFenceID = fenceID;
			mViewIndex = 0;
			mCPUViewHandle = cpuViewHandle;
			mGPUViewHandle = gpuViewHandle;
			mCurrentGPUViewHandle.ptr = 0;
			mReadyForDeletion = false;
		}

		~DX12Texture()
		{
			if (mDefaultHeaps.size() > 0)
				IGOLogWarn("Texture %d has %d default heaps still open", mTexID, mDefaultHeaps.size());

			if (mUploadHeaps.size() > 0)
				IGOLogWarn("Texture %d has %d upload heaps still open", mTexID, mUploadHeaps.size());

			for (StatefulHeaps::iterator iter = mDefaultHeaps.begin(); iter != mDefaultHeaps.end(); ++iter)
				delete *iter;

			for (StatefulHeaps::iterator iter = mUploadHeaps.begin(); iter != mUploadHeaps.end(); ++iter)
				delete *iter;
		}

		void ReadyForDeletion()
		{
			mReadyForDeletion = true;
		}

		bool IsValid() const
		{
			return mDefaultHeaps.size() > 0			// ie we have uploaded something
				&& mCurrentGPUViewHandle.ptr != 0;	// ie this is not the first frame after creating the texture (we want to wait for the frame after the one that triggered the texture upload)
		}

		CD3DX12_GPU_DESCRIPTOR_HANDLE View(uint64_t fenceID) const
		{
			if (mDefaultHeaps.size() > 0)
			{
				StatefulHeap* sHeap = mDefaultHeaps[mDefaultHeaps.size() - 1];
				sHeap->fenceID = fenceID;
#ifdef DEBUG_TEXTURES
				IGOLogInfo("Updating fence=%llu for texID=%d, heap=%p - currentGPUViewHandle=0x%016llx", fenceID, mTexID, sHeap, mCurrentGPUViewHandle.ptr);
#endif
			}

			return mCurrentGPUViewHandle;
		}

		bool UpdateState(uint64_t fenceID)
		{
			// If invalid, delete it!
			if (!IsValid())
				return mReadyForDeletion;

			// Do we have any GPU resources we are done with? note we keep the latest entry, except if we are ready for completion
			StatefulHeaps::reverse_iterator rbegin = mDefaultHeaps.rbegin();
			if (!mReadyForDeletion && rbegin != mDefaultHeaps.rend())
				++rbegin;

			for (StatefulHeaps::reverse_iterator iter = rbegin; iter != mDefaultHeaps.rend(); )
			{
				StatefulHeap* heap = *iter;
				if (heap->fenceID < fenceID)
				{
#ifdef DEBUG_TEXTURES
					IGOLogInfo("Deleting default heap=%p for texID=%d (heap fence=%llu, ref fence=%llu)", heap, mTexID, heap->fenceID, fenceID);
#endif
					delete heap;
					iter = mDefaultHeaps.erase(iter);
				}

				else
					++iter;
			}

			// Same with our upload resources
			rbegin = mUploadHeaps.rbegin();
			if (!mReadyForDeletion && rbegin != mUploadHeaps.rend())
				++rbegin;

			for (StatefulHeaps::reverse_iterator iter = rbegin; iter != mUploadHeaps.rend(); )
			{
				StatefulHeap* heap = *iter;
				if (heap->fenceID < fenceID)
				{
#ifdef DEBUG_TEXTURES
					IGOLogInfo("Deleting upload heap=%p for texID=%d (heap fence=%llu, ref fence=%llu)", heap, mTexID, heap->fenceID, fenceID);
#endif
					delete heap;
					iter = mUploadHeaps.erase(iter);
				}

				else
					++iter;
			}

			// Are we ready to notify the NodeResources instance to remove this texture entry entirely?
			bool cleanedUp = mReadyForDeletion ? mDefaultHeaps.empty() && mUploadHeaps.empty() : false;

#ifdef DEBUG_TEXTURES
			if (cleanedUp)
				IGOLogWarn("Texture %d ready for deletion", mTexID);
#endif
			return cleanedUp;
		}

		void UpdateContent(ID3D12Device* device, UINT nodeMask, ID3D12GraphicsCommandList* cmdList, uint64_t fenceID, uint32_t width, uint32_t height, const char* data, size_t dataSize)
		{
#ifdef _DEBUG
			static uint64_t TexDefaultHeapID = 0;
			static uint64_t TexUploadHeapID = 0;
#endif

			UINT16 arraySize = 1;
			UINT16 mipLevels = 1;
			DXGI_FORMAT format = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isSRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
			CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize, mipLevels);

			const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;

			// Can we reuse the latest GPU heap we committed?
			StatefulHeap* statefulDefaultHeap = NULL;
			if (!mDefaultHeaps.empty())
			{
				StatefulHeap* sHeap = mDefaultHeaps[mDefaultHeaps.size() - 1];
				if (sHeap->width == width && sHeap->height == height && sHeap->format == format)
				{
#ifdef DEBUG_TEXTURES
					IGOLogInfo("Reusing default heap %p for texID=%d, width=%d, height=%d, fenceID=%llu/%llu", sHeap, mTexID, width, height, fenceID, sHeap->fenceID);
#endif
					sHeap->fenceID = fenceID;
					statefulDefaultHeap = sHeap;
				}
			}
			
			// Or do we need to create a new one?
			bool newDefaultHeap = false;
			if (!statefulDefaultHeap)
			{
				CD3DX12_HEAP_PROPERTIES texHeap(D3D12_HEAP_TYPE_DEFAULT, nodeMask, nodeMask);
				D3D12_HEAP_FLAGS texHeapFlags = D3D12_HEAP_FLAG_NONE;

				StatefulHeap* sHeap = new StatefulHeap;
				sHeap->fenceID = fenceID;
				sHeap->width = width;
				sHeap->height = height;
				sHeap->format = format;

				HRESULT hr = device->CreateCommittedResource(&texHeap, texHeapFlags, &texDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&sHeap->texture));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create texture GPU resource (0x%08x)", hr));
					delete sHeap;
					return;
				}

				newDefaultHeap = true;
				mDefaultHeaps.push_back(sHeap);
				statefulDefaultHeap = mDefaultHeaps[mDefaultHeaps.size() - 1];

#ifdef DEBUG_TEXTURES
				IGOLogInfo("Creating new default heap %p for texID=%d, width=%d, height=%d, fenceID=%llu", sHeap, mTexID, width, height, fenceID);
#endif

				DX12_NAME_OBJECT(sHeap->texture, L"TexDefaultHeap_%ld", TexDefaultHeapID++);
			}

			// Now do the same for the upload heap!But now we have to make sure the previous upload was completed
			StatefulHeap* statefulUploadHeap = NULL;
			if (!mUploadHeaps.empty())
			{
				StatefulHeap* sHeap = mUploadHeaps[mUploadHeaps.size() - 1];
				if (sHeap->width == width && sHeap->height == height && sHeap->format == format && sHeap->fenceID < fenceID)
				{
#ifdef DEBUG_TEXTURES
					IGOLogInfo("Reusing upload heap %p for texID=%d, width=%d, height=%d, fenceID=%llu/%llu", sHeap, mTexID, width, height, fenceID, sHeap->fenceID);
#endif
					sHeap->fenceID = fenceID;
					statefulUploadHeap = sHeap;
				}
			}

			if (!statefulUploadHeap)
			{
				UINT64 uploadBufferSize = GetRequiredIntermediateSize(statefulDefaultHeap->texture, 0, num2DSubresources);

				CD3DX12_HEAP_PROPERTIES texUploadHeap(D3D12_HEAP_TYPE_UPLOAD, nodeMask, nodeMask);
				D3D12_HEAP_FLAGS texUploadHeapFlags = D3D12_HEAP_FLAG_NONE;
				CD3DX12_RESOURCE_DESC texUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

				StatefulHeap* sHeap = new StatefulHeap;
				sHeap->fenceID = fenceID;
				sHeap->width = width;
				sHeap->height = height;
				sHeap->format = format;

				HRESULT hr = device->CreateCommittedResource(&texUploadHeap, texUploadHeapFlags, &texUploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sHeap->texture));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create texture upload resource (0x%08x)", hr));
					
					if (newDefaultHeap)
					{
						delete statefulDefaultHeap;
						mDefaultHeaps.pop_back();
					}

					delete sHeap;
					return;
				}

				mUploadHeaps.push_back(sHeap);
				statefulUploadHeap = mUploadHeaps[mUploadHeaps.size() - 1];

#ifdef DEBUG_TEXTURES
				IGOLogInfo("Creating new upload heap %p for texID=%d, uploadBufferSize=%llu, fenceID=%llu", sHeap, mTexID, uploadBufferSize, fenceID);
#endif

				DX12_NAME_OBJECT(sHeap->texture, L"TexUploadHeap_%ld", TexUploadHeapID++);
			}

			// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
			D3D12_SUBRESOURCE_DATA texResource = {};
			texResource.pData = data;
			texResource.RowPitch = width * 4;
			texResource.SlicePitch = dataSize;

			D3D12_RESOURCE_STATES initialState = newDefaultHeap ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			SetResourceBarrier(cmdList, statefulDefaultHeap->texture, initialState, D3D12_RESOURCE_STATE_COPY_DEST);
			if (UpdateSubresources(cmdList, statefulDefaultHeap->texture, statefulUploadHeap->texture, 0, 0, num2DSubresources, &texResource) == 0)
				IGOLogWarn("Failed to upload texture resources for %d", mTexID);
			SetResourceBarrier(cmdList, statefulDefaultHeap->texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			// create shader resource view descriptor
			const UINT cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuViewHandle(mCPUViewHandle, mViewIndex, cbvSrvDescriptorSize);
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuViewHandle(mGPUViewHandle, mViewIndex, cbvSrvDescriptorSize);

#ifdef DEBUG_TEXTURES
			IGOLogWarn("TexID=%d, cpuViewHandle=0x%016llx, gpuViewHandle=0x%016llx (before 0x%016llx)", mTexID, cpuViewHandle.ptr, gpuViewHandle.ptr, mCurrentGPUViewHandle.ptr);
#endif

			mCurrentGPUViewHandle = gpuViewHandle;

			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Format = texDesc.Format;
			shaderResourceViewDesc.Texture2D.MipLevels = texDesc.MipLevels;
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			device->CreateShaderResourceView(statefulDefaultHeap->texture, &shaderResourceViewDesc, cpuViewHandle);

			// Prepare for next view to use
			mViewIndex = (mViewIndex + 1) % DX12ResourceProvider::BUFFER_FRAME_CNT;
		}

		void ForceTDR()
		{
			if (!mDefaultHeaps.empty())
			{
				IGOLogWarn("TRIGGERING TDR...");
				StatefulHeap* sHeap = mDefaultHeaps[mDefaultHeaps.size() - 1];
                sHeap->texture = NULL; // ie call internal Release();
			}
		}

	private:
		uint64_t mFenceID;
		uint32_t mViewIndex;	// Next offset to use for our view in the heap (0 -> BUFFER_FRAME_CNT - 1)
		int mTexID;

		struct StatefulHeap
		{
			CComPtr<ID3D12Resource> texture;
			uint64_t fenceID;
			uint32_t width;
			uint32_t height;
			DXGI_FORMAT format;
		};

		typedef eastl::vector<StatefulHeap*> StatefulHeaps;
		StatefulHeaps mUploadHeaps;	// Current set of heaps used to upload textures
		StatefulHeaps mDefaultHeaps;	// Current set of heaps in GPU memory

		CD3DX12_CPU_DESCRIPTOR_HANDLE mCPUViewHandle;			// Base cpu view handle accessible for this texture (BUFFER_FRAME_CNT entries available)
		CD3DX12_GPU_DESCRIPTOR_HANDLE mGPUViewHandle;			// Base gpu view handle accessible for this texture (BUFFER_FRAME_CNT entries avaialable)
		CD3DX12_GPU_DESCRIPTOR_HANDLE mCurrentGPUViewHandle;	// Current view to use pointing the latest texture update 
		bool mReadyForDeletion;									// Have to wait until GPU isn't using the associated views before deleting the object
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	class DX12ResourceProvider::NodeResources
	{
	public:
		NodeResources(ID3D12Device* device, IDXGISwapChain3* swapChain, ID3D12CommandQueue* cmdQueue, UINT nodeMask, PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignatureFcn)
		{
			IGOLogInfo("Creating new NodeResources: device=%p, swapchain=%p, cmdQueue=%p, nodeMask=%d", device, swapChain, cmdQueue, nodeMask);

			mIsValid = false;
			mDevice = device;
			mNodeMask = nodeMask;
			mBackbufferCnt = 0;
			mCurrentFenceID = 0;
			mCurrentFrameIdx = 0;
			mResourcesInitialized = false;

			ZeroMemory(&mTextures[0], sizeof(mTextures));

			DXGI_SWAP_CHAIN_DESC swDesc;
			HRESULT hr = swapChain->GetDesc(&swDesc);
			if (FAILED(hr))
			{
				CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to query swap chain desc (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
				return;
			}

			for (int idx = 0; idx < DX12ResourceProvider::BUFFER_FRAME_CNT; ++idx)
			{
				mCmdListFence[idx] = 0;

				// A command allocator
				hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdAllocator[idx]));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 unable to create cmd allocator (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mCmdAllocator[idx], L"RPCommandAllocator%d", idx);

				hr = device->CreateCommandList(nodeMask, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocator[idx], mBgPso, IID_PPV_ARGS(&mCmdList[idx]));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create initial command list (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mCmdList[idx], L"RPCommandList%d", idx);

				// We use the first command list to initialize our setup/loading data, but we need to close the other ones so that we don't fail there 'Reset' when
				// preparing for the new render loop
				if (idx > 0)
				{
					hr = DX12GraphicsCommandListCloseFcn(mCmdList[idx]);
					if (FAILED(hr))
						IGOLogWarn("Failed to immediately close command list %d on Init", idx);
				}
			}

			// Create root signature
			{
				ID3DBlob* signature = RootSignature(serializeRootSignatureFcn);
				if (!signature)
					return;

				hr = device->CreateRootSignature(nodeMask, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create root signature (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mRootSignature, L"RPRootSignature");
			}

			// Descriptor heaps
			{
				// - For render target view
				mBackbufferCnt = swDesc.BufferCount;
				if (mBackbufferCnt == 0 || mBackbufferCnt > MAX_RENDER_TARGET_CNT)
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 Too many back buffers to support (%d/%d) - device=%p, queue=%p, nodeMask=%d", mBackbufferCnt, MAX_RENDER_TARGET_CNT, device, cmdQueue, nodeMask));
					return;
				}

				D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
				descHeap.NumDescriptors = mBackbufferCnt;
				descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				descHeap.NodeMask = nodeMask;
				hr = device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&mRtvDescriptorHeap));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create rtv descriptor heap (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mRtvDescriptorHeap, L"RP_RTVHeap");

				// - For shader resource views and constant buffer views
				D3D12_DESCRIPTOR_HEAP_DESC descHeapCbvSrv = {};
				descHeapCbvSrv.NumDescriptors = MAX_CBV_SRV_DESCRIPTOR_CNT;
				descHeapCbvSrv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				descHeapCbvSrv.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				descHeapCbvSrv.NodeMask = nodeMask;
				hr = device->CreateDescriptorHeap(&descHeapCbvSrv, IID_PPV_ARGS(&mCbvSrvDescriptorHeap));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create cbv/srv descriptor heap (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mRtvDescriptorHeap, L"RP_CBV_SRVHeap");

				// - For sampler
				D3D12_DESCRIPTOR_HEAP_DESC descHeapSampler = {};
				descHeapSampler.NumDescriptors = 1;
				descHeapSampler.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
				descHeapSampler.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				descHeapSampler.NodeMask = nodeMask;
				hr = device->CreateDescriptorHeap(&descHeapSampler, IID_PPV_ARGS(&mSamplerDescriptorHeap));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create sampler descriptor heap (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mSamplerDescriptorHeap, L"RP_SamplerHeap");
			}

			// setup sampler
			{
				// get a handle to the start of the descriptor heap
				D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = mSamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

				// create the wrapping sampler
				D3D12_SAMPLER_DESC sampDesc;
				ZeroMemory(&sampDesc, sizeof(sampDesc));
				sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
				sampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
				sampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
				sampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
				sampDesc.MinLOD = 0;
				sampDesc.MaxLOD = D3D12_FLOAT32_MAX;
				sampDesc.MipLODBias = 0.0f;
				sampDesc.MaxAnisotropy = 1;
				sampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
				sampDesc.BorderColor[0] = sampDesc.BorderColor[1] = sampDesc.BorderColor[2] = sampDesc.BorderColor[3] = 0;
				device->CreateSampler(&sampDesc, samplerHandle);
			}

			// Create PSOs
			{
				// Setup render states
				D3D12_INPUT_ELEMENT_DESC layout[] =
				{
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				};

				D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
				inputLayoutDesc.pInputElementDescs = layout;
				inputLayoutDesc.NumElements = _countof(layout);

				CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);

				CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
				blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

				CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
				depthStencilDesc.DepthEnable = false;
				depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
				depthStencilDesc.StencilEnable = FALSE;

				D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
				ZeroMemory(&psoDesc, sizeof(psoDesc));
				psoDesc.pRootSignature = mRootSignature;
				psoDesc.InputLayout = inputLayoutDesc;
				psoDesc.VS = VertexShaderInfo();
				psoDesc.PS = PixelShaderInfo();
				psoDesc.RasterizerState = rasterizerDesc;
				psoDesc.DepthStencilState = depthStencilDesc;
				psoDesc.BlendState = blendDesc;
				psoDesc.SampleMask = UINT_MAX;
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				psoDesc.NumRenderTargets = 1;
				psoDesc.RTVFormats[0] = swDesc.BufferDesc.Format;
				psoDesc.SampleDesc.Count = 1;
				psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
				psoDesc.NodeMask = nodeMask;

				hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPso));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create pipeline state object (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mPso, L"RP_PSO");

                D3D12_GRAPHICS_PIPELINE_STATE_DESC bgPsoDesc;
                ZeroMemory(&bgPsoDesc, sizeof(bgPsoDesc));
                bgPsoDesc.pRootSignature = mRootSignature;
                bgPsoDesc.InputLayout = inputLayoutDesc;
                bgPsoDesc.VS = BackgroundVertexShaderInfo();
                bgPsoDesc.PS = BackgroundPixelShaderInfo();
                bgPsoDesc.RasterizerState = rasterizerDesc;
                bgPsoDesc.DepthStencilState = depthStencilDesc;
                bgPsoDesc.BlendState = blendDesc;
                bgPsoDesc.SampleMask = UINT_MAX;
                bgPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                bgPsoDesc.NumRenderTargets = 1;
                bgPsoDesc.RTVFormats[0] = swDesc.BufferDesc.Format;
                bgPsoDesc.SampleDesc.Count = 1;
                bgPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                bgPsoDesc.NodeMask = nodeMask;

                hr = device->CreateGraphicsPipelineState(&bgPsoDesc, IID_PPV_ARGS(&mBgPso));
                if (FAILED(hr))
                {
                    CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create background pipeline state object (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
                    return;
                }

                DX12_NAME_OBJECT(mBgPso, L"RP_PSO");
			}

			// Setup render target
			{
				CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				for (UINT idx = 0; idx < mBackbufferCnt; ++idx)
				{
					hr = swapChain->GetBuffer(idx, IID_PPV_ARGS(&mRenderTarget[idx]));
					if (FAILED(hr))
					{
						CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to access swap chain buffer %d (0x%08x) - device=%p, queue=%p, nodeMask=%d", idx, hr, device, cmdQueue, nodeMask));
						return;
					}

					device->CreateRenderTargetView(mRenderTarget[idx], nullptr, cpuHandle);
					cpuHandle.Offset(1, descriptorSize);

					DX12_NAME_OBJECT(mRenderTarget[idx], L"SwapChainBB%d", idx);
				}
			}

			// Create command list for initial GPU setup
			{
				// create the vertex buffer
				{
					struct QuadVertex
					{
						float x, y, z;            //!< The transformed position for the vertex.
						float tu1, tv1;            //!< texture coordinates
					};

					static const QuadVertex vtx[] =
					{
						{ 0, 0, 1, 0.0f, 0.0f },
						{ 1, 0, 1, 1.0f, 0.0f },
						{ 0, -1, 1, 0.0f, 1.0f },
						{ 1, -1, 1, 1.0f, 1.0f },
					};

					size_t vertexDataSize = sizeof(vtx);
					CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, nodeMask, nodeMask);
					CD3DX12_RESOURCE_DESC vertexDataDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexDataSize);
					hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &vertexDataDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mVertexBuffer));

					// schedule a copy to get data in the buffer
					if (SUCCEEDED(hr))
					{
						DX12_NAME_OBJECT(mVertexBuffer, L"RPCommittedVB");

						heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, nodeMask, nodeMask);
						hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &vertexDataDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexBufferUploadTmp));

						if (SUCCEEDED(hr))
						{
							DX12_NAME_OBJECT(mVertexBufferUploadTmp, L"RPUploadVB");

							// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
							D3D12_SUBRESOURCE_DATA bufResource = {};
							bufResource.pData = vtx;
							bufResource.RowPitch = vertexDataSize;
							bufResource.SlicePitch = bufResource.RowPitch;

							SetResourceBarrier(mCmdList[mCurrentFrameIdx], mVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

							PIX_SET_MARKER(mCmdList[mCurrentFrameIdx], L"CL %d:Copy vertex buffer data to default resource...", mCurrentFrameIdx);
							UpdateSubresources(mCmdList[0], mVertexBuffer, mVertexBufferUploadTmp, 0, 0, 1, &bufResource);
							PIX_SET_MARKER(mCmdList[mCurrentFrameIdx], L"CL %d:Vertex buffer copy complete", mCurrentFrameIdx);

							SetResourceBarrier(mCmdList[mCurrentFrameIdx], mVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
						}

						else
						{
							CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create intermediate upload vertex buffer (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
							return;
						}
					}

					else
					{
						CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create vertex buffer (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
						return;
					}

					// create vertex buffer description
					mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
					mVertexBufferView.SizeInBytes = static_cast<UINT>(vertexDataSize);
					mVertexBufferView.StrideInBytes = sizeof(QuadVertex);
				}

				// Create fence object
				hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
				mCurrentFenceID = 1;

				DX12_NAME_OBJECT(mFence, L"RPFence");

				hr = DX12GraphicsCommandListCloseFcn(mCmdList[mCurrentFrameIdx]);
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to close the initial command list (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				// The setup will be sent over once we know which command queue to use -> see BeginCommandList
			}

			mIsValid = true;
		}

		~NodeResources()
		{
			for (int idx = 0; idx < DX12ResourceProvider::MAX_SUPPORTED_WINDOWS; ++idx)
			{
				if (mTextures[idx])
				{
					delete mTextures[idx];
					mTextures[idx] = 0;
				}
			}

			for (UINT idx = 0; idx < mBackbufferCnt; ++idx)
				mRenderTarget[idx].Release();

			mFence.Release();

			for (int idx = 0; idx < DX12ResourceProvider::BUFFER_FRAME_CNT; ++idx)
			{
				mCmdList[idx].Release();
				mCmdAllocator[idx].Release();
			}

			mCbvSrvDescriptorHeap.Release();
			mRtvDescriptorHeap.Release();
			mSamplerDescriptorHeap.Release();

			mPso.Release();
            mBgPso.Release();
			mRootSignature.Release();

			mVertexBuffer.Release();
			mVertexBufferUploadTmp.Release();
		}

		bool IsValid() const { return mIsValid; }

		void WaitForCompletion(ID3D12CommandQueue* cmdQueue)
		{
			if (!IsValid())
				return;

			OriginIGO::WaitForCompletion(cmdQueue, mFence, mCurrentFenceID);
		}

		bool BeginCommandList(ID3D12CommandQueue* queue, UINT backbufferIdx, const D3DMATRIX& projMatrix, uint32_t width, uint32_t height)
		{
			if (!IsValid())
				return false;

			if (!mResourcesInitialized)
			{
				ID3D12CommandList* commandLists[] = { mCmdList[mCurrentFrameIdx] };
				DX12CommandQueueExecuteCommandListsFcn(queue, _countof(commandLists), commandLists);

				// Prepare for next round
				mCmdListFence[mCurrentFrameIdx] = mCurrentFenceID;
				queue->Signal(mFence, mCurrentFenceID);
				++mCurrentFenceID;

				mResourcesInitialized = true;
				return false; // stop the rendering for this frame
			}

			const UINT64 lastCompletedFence = mFence->GetCompletedValue();
			mCurrentFrameIdx = (mCurrentFrameIdx + 1) % DX12ResourceProvider::BUFFER_FRAME_CNT;
			uint64_t cmdListFence = mCmdListFence[mCurrentFrameIdx];

//			PIX_SET_MARKER(queue, L"Testing for fence - last completed=%d, new frame idx=%d, frame fence=%d...", lastCompletedFence, mCurrentFrameIdx, cmdListFence);

			// Check if this frame resource is still in use by the GPU; wait on its associated fence if not available yet
			if (lastCompletedFence < cmdListFence)
			{
				PIX_BEGIN_EVENT(queue, L"Frame resources in use, waiting for fence...");
                static NotXPSupportedAPIs::CreateEventExFn createEventEx = GetNotXPSupportedAPIs()->createEventEx;
                HANDLE eventHandle = createEventEx ? createEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS) : NULL;
				if (eventHandle)
				{
					HRESULT hr = mFence->SetEventOnCompletion(cmdListFence, eventHandle);
					if (SUCCEEDED(hr))
					{
						DWORD timeoutInMS = 1000;
						DWORD result = WaitForSingleObject(eventHandle, timeoutInMS);
						if (result != WAIT_OBJECT_0)
						{
							CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 timeout waiting on fence (%d/%d) for queue=%p", lastCompletedFence, cmdListFence, queue));
						}
					}
					else
						IGOLogWarn("Failed to setup queue fence to signal of end of frame resource usage - hr=0x%08x", hr);

					CloseHandle(eventHandle);
				}

				else
					IGOLogWarn("Unable to create event to properly wait on frame resource completion!!");

				PIX_END_EVENT(queue);
			}

			// First we need to reset the command list/allocator now that the GPU is done using them
			bool success = false;
			HRESULT hr = mCmdAllocator[mCurrentFrameIdx]->Reset();
			if (SUCCEEDED(hr))
			{
				hr = DX12GraphicsCommandListResetFcn(mCmdList[mCurrentFrameIdx], mCmdAllocator[mCurrentFrameIdx], mBgPso);
				if (SUCCEEDED(hr))
				{
					// Prepare render target - TODO: this may not be good enough at all, but for now...
					SetResourceBarrier(mCmdList[mCurrentFrameIdx], mRenderTarget[backbufferIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

					// Setup root signature
					mCmdList[mCurrentFrameIdx]->SetGraphicsRootSignature(mRootSignature);

					// Setup viewport and scissor
					D3D12_VIEWPORT viewport;
					ZeroMemory(&viewport, sizeof(viewport));
					viewport.TopLeftX = 0;
					viewport.TopLeftY = 0;
					viewport.Width = static_cast<float>(width);
					viewport.Height = static_cast<float>(height);
					viewport.MaxDepth = 1.0f;
					viewport.MinDepth = 0.0f;
					mCmdList[mCurrentFrameIdx]->RSSetViewports(1, &viewport);

					D3D12_RECT scissorRect = { 0, 0, width, height };
					mCmdList[mCurrentFrameIdx]->RSSetScissorRects(1, &scissorRect);

					// Set topology
					mCmdList[mCurrentFrameIdx]->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

					// Set descriptor heaps
					ID3D12DescriptorHeap* heaps[2] = { mCbvSrvDescriptorHeap, mSamplerDescriptorHeap };
					mCmdList[mCurrentFrameIdx]->SetDescriptorHeaps(_countof(heaps), heaps);

					// Set vertex buffer
					mCmdList[mCurrentFrameIdx]->IASetVertexBuffers(0, 1, &mVertexBufferView);

					// Set render target
					CD3DX12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle(mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
					UINT descriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
					rtvCpuHandle.Offset(backbufferIdx, descriptorSize);

					D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptors[] = { rtvCpuHandle };
					mCmdList[mCurrentFrameIdx]->OMSetRenderTargets(1, rtvDescriptors, false, nullptr);

					// Set descriptor table for sampler/proj matrix
					float data[4];
					data[0] = projMatrix._11;
					data[1] = projMatrix._22;
					data[2] = projMatrix._33;
					data[3] = projMatrix._34;
					mCmdList[mCurrentFrameIdx]->SetGraphicsRoot32BitConstants(1, 4, data, 0);
					mCmdList[mCurrentFrameIdx]->SetGraphicsRootDescriptorTable(3, mSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

					// Update the state of our textures (ie are we done uploading next textures/ready to render them? is it time to destroy them?)
					for (int idx = 0; idx < DX12ResourceProvider::MAX_SUPPORTED_WINDOWS; ++idx)
					{
						if (mTextures[idx])
						{
							if (mTextures[idx]->UpdateState(cmdListFence))
							{
								delete mTextures[idx];
								mTextures[idx] = NULL;
							}
						}
					}

					success = true;
				}

				else
					IGOLogWarn("Failed to reset command list %d", mCurrentFrameIdx);
			}

			else
				IGOLogWarn("Failed to reset command allocator %d", mCurrentFrameIdx);

			return true;
		}

		void EndCommandList(ID3D12CommandQueue* queue, UINT backbufferIdx)
		{
			if (!IsValid())
				return;

			// Prepare render target for 'Present'
			SetResourceBarrier(mCmdList[mCurrentFrameIdx], mRenderTarget[backbufferIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

			// Close command list and send it over
			HRESULT hr = DX12GraphicsCommandListCloseFcn(mCmdList[mCurrentFrameIdx]);
			if (SUCCEEDED(hr))
			{
				// Send it over!
				ID3D12CommandList* commandLists[] = { mCmdList[mCurrentFrameIdx] };
				DX12CommandQueueExecuteCommandListsFcn(queue, _countof(commandLists), commandLists);

				// Prepare for next round
				mCmdListFence[mCurrentFrameIdx] = mCurrentFenceID;
				queue->Signal(mFence, mCurrentFenceID);
				++mCurrentFenceID;
			}

			else
			{
				CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to close command list (0x%08x)", hr));
			}
		}

		bool IsTextureInUse(int texID)
		{
			if (texID == DX12ResourceProvider::INVALID_TEXTURE_ID)
				return false;

			if (mTextures[texID])
				return true;

			return false;
		}

		void CreateTexture2D(int texID, uint32_t width, uint32_t height, const char* data, size_t dataSize)
		{
			PIX_SET_MARKER(mCmdList[mCurrentFrameIdx], L"CL %d:Reserve new texture %d x %d", mCurrentFrameIdx, width, height);

			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuViewHandle(mCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuViewHandle(mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			const UINT cbvSrvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			cpuViewHandle.Offset(MAX_CONSTANT_BUFFERS + texID * DX12ResourceProvider::BUFFER_FRAME_CNT, cbvSrvDescriptorSize);
			gpuViewHandle.Offset(MAX_CONSTANT_BUFFERS + texID * DX12ResourceProvider::BUFFER_FRAME_CNT, cbvSrvDescriptorSize);

			DX12Texture* texture = new DX12Texture(texID, mCurrentFenceID, cpuViewHandle, gpuViewHandle);
			texture->UpdateContent(mDevice, mNodeMask, mCmdList[mCurrentFrameIdx], mCurrentFenceID, width, height, data, dataSize);
			mTextures[texID] = texture;

			PIX_SET_MARKER(mCmdList[mCurrentFrameIdx], L"CL %d:Texture created ID=%d", mCurrentFrameIdx, texID);
		}

		void UpdateTexture2D(int texID, uint32_t width, uint32_t height, const char* data, size_t dataSize)
		{
			if (texID == DX12ResourceProvider::INVALID_TEXTURE_ID)
				return;

			if (!mTextures[texID] || !mTextures[texID]->IsValid())
				return;

			PIX_SET_MARKER(mCmdList[mCurrentFrameIdx], L"CL %d:Reserve new texture to update=%d, %d x %d, node=0x%08x", mCurrentFrameIdx, texID, width, height, mNodeMask);

			mTextures[texID]->UpdateContent(mDevice, mNodeMask, mCmdList[mCurrentFrameIdx], mCurrentFenceID, width, height, data, dataSize);

			PIX_SET_MARKER(mCmdList[mCurrentFrameIdx], L"CL %d:Texture update created ID=%d", mCurrentFrameIdx, texID);
		}

		void DeleteTexture2D(int texID)
		{
			if (texID == DX12ResourceProvider::INVALID_TEXTURE_ID)
				return;

			if (!mTextures[texID] || !mTextures[texID]->IsValid())
				return;

			mTextures[texID]->ReadyForDeletion();
		}

        void RenderQuad(int texID, float x, float y, float width, float height, float alpha, ShaderEffect effect, float effectParam)
		{
			if (texID == DX12ResourceProvider::INVALID_TEXTURE_ID)
				return;

			if (!mTextures[texID] || !mTextures[texID]->IsValid())
				return;

			D3D12_GPU_DESCRIPTOR_HANDLE handle = mTextures[texID]->View(mCurrentFenceID);

			//PIX_BEGIN_EVENT(mCmdList[mCurrentFrameIdx], L"CL %d:Create quad for texID=%d, handle=0x%016llx", mCurrentFrameIdx, texID, handle.ptr);

            mCmdList[mCurrentFrameIdx]->SetPipelineState(effect == ShaderEffect_BACKGROUND ? mBgPso : mPso);
			mCmdList[mCurrentFrameIdx]->SetGraphicsRootDescriptorTable(0, handle);

			float data[5];
			data[0] = x;
			data[1] = y;
			int dimWH = (int)width + ((int)height << 16);
			data[2] = *((float*)&dimWH);
			data[3] = alpha;
			data[4] = effectParam;
			mCmdList[mCurrentFrameIdx]->SetGraphicsRoot32BitConstants(2, 5, data, 0);

			mCmdList[mCurrentFrameIdx]->DrawInstanced(4, 1, 0, 0);

			PIX_END_EVENT(mCmdList[mCurrentFrameIdx]);
		}

		void ForceTDR()
		{
			for (int texID = 0; texID < DX12ResourceProvider::MAX_SUPPORTED_WINDOWS; ++texID)
			{
				if (mTextures[texID] && mTextures[texID]->IsValid())
				{
					mTextures[texID]->ForceTDR();
					break;
				}
			}
		}

	private:
		static ID3DBlob* RootSignature(PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignatureFcn)
		{
			static bool initialized = false;
			static CComPtr<ID3DBlob> signature;

			if (!initialized)
			{
				initialized = true;

				CD3DX12_DESCRIPTOR_RANGE descRange[4]; // Perfomance TIP: Order from most frequent to least frequent
				descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);		// 1 frequently changed diffuse - t0
				descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);		// 1 per frame constant buffer - b0 - proj xform
				descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 1);		// 1 per window constant buffer - b1 - object xform
				descRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);	// 1 static sampler - s0

				CD3DX12_ROOT_PARAMETER rootParameters[4];
				rootParameters[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
				rootParameters[1].InitAsConstants(4, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
				rootParameters[2].InitAsConstants(5, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
				rootParameters[3].InitAsDescriptorTable(1, &descRange[3], D3D12_SHADER_VISIBILITY_PIXEL);

				CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
				descRootSignature.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
				{
					CComPtr<ID3DBlob> errorBlob;
					HRESULT hr = serializeRootSignatureFcn(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBlob);
					if (FAILED(hr))
					{
						CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to serialize root signature (0x%08x)", hr));
					}
				}
			}

			return signature;
		}

		static D3D12_SHADER_BYTECODE VertexShaderInfo()
		{
			static bool initialized = false;
			static D3D12_SHADER_BYTECODE vertexShader;

			if (!initialized)
			{
				initialized = true;
                vertexShader = LoadShaderData(IDR_IGODX12VSFX, "DX12 Missing vertex shader resource");
			}

			return vertexShader;
		}

        static D3D12_SHADER_BYTECODE BackgroundVertexShaderInfo()
        {
            static bool initialized = false;
            static D3D12_SHADER_BYTECODE bgVertexShader;

            if (!initialized)
            {
                initialized = true;
                bgVertexShader = LoadShaderData(IDR_IGODX12VSBACKGROUNDFX, "DX12 Missing background vertex shader resource");
            }

            return bgVertexShader;
        }

		static D3D12_SHADER_BYTECODE PixelShaderInfo()
		{
			static bool initialized = false;
			static D3D12_SHADER_BYTECODE pixelShader;

			if (!initialized)
			{
				initialized = true;
                pixelShader = LoadShaderData(IDR_IGODX12PSFX, "DX12 Missing pixel shader resource");
			}

			return pixelShader;
		}

        static D3D12_SHADER_BYTECODE BackgroundPixelShaderInfo()
        {
            static bool initialized = false;
            static D3D12_SHADER_BYTECODE bgPixelShader;

            if (!initialized)
            {
                initialized = true;
                bgPixelShader = LoadShaderData(IDR_IGODX12PSBACKGROUNDFX, "DX12 Missing pixel shader resource");
            }

            return bgPixelShader;
        }

        static D3D12_SHADER_BYTECODE LoadShaderData(int resourceID, const char* errorMsg)
        {
            D3D12_SHADER_BYTECODE pixelShader;

            pixelShader.BytecodeLength = 0;
            pixelShader.pShaderBytecode = NULL;

            HRSRC resourceInfo = FindResource(gInstDLL, MAKEINTRESOURCE(resourceID), RT_RCDATA);
            if (resourceInfo)
            {
                HGLOBAL hRes = LoadResource(gInstDLL, resourceInfo);
                pixelShader.pShaderBytecode = LockResource(hRes);
                pixelShader.BytecodeLength = SizeofResource(gInstDLL, resourceInfo);
            }

            else
            {
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, errorMsg));
            }

            return pixelShader;
        }

	private:
		ID3D12Device* mDevice;
		UINT mBackbufferCnt;												// Swap chain buffer count

		UINT mNodeMask;														// which GPU we're talking about from a point of view of the current device owning the swapchain

		uint64_t mCurrentFenceID;
		CComPtr<ID3D12Fence> mFence;										// Our main fence object to detect when rendering is completed

		uint32_t mCurrentFrameIdx;											// Cycles through 3 sequential command queues
		uint64_t mCmdListFence[DX12ResourceProvider::BUFFER_FRAME_CNT];							// Current
		CComPtr<ID3D12GraphicsCommandList> mCmdList[DX12ResourceProvider::BUFFER_FRAME_CNT];
		CComPtr<ID3D12CommandAllocator> mCmdAllocator[DX12ResourceProvider::BUFFER_FRAME_CNT];

		static const uint32_t MAX_SUPPORTED_TEXTURES = DX12ResourceProvider::MAX_SUPPORTED_WINDOWS * DX12ResourceProvider::BUFFER_FRAME_CNT; // We can have up to BUFFER_FRAME_CNT textures for each window (in use + uploadings) 
		static const uint32_t MAX_CONSTANT_BUFFERS = 0;						// Right now using root signature constants - otherwise we would need (1 for proj matrix, 1 for each window xform) * BUFFER_FRAME_CNT
		static const uint32_t MAX_CBV_SRV_DESCRIPTOR_CNT = MAX_CONSTANT_BUFFERS + MAX_SUPPORTED_TEXTURES;

		CComPtr<ID3D12DescriptorHeap> mCbvSrvDescriptorHeap;				// Constant buffer view / shader resource view heap
		CComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;					// Render target view heap
		CComPtr<ID3D12DescriptorHeap> mSamplerDescriptorHeap;				// Sampler descriptor heap

		CComPtr<ID3D12PipelineState> mPso;									// Pipeline state object 
        CComPtr<ID3D12PipelineState> mBgPso;                                // Background pipeline state object
		CComPtr<ID3D12RootSignature> mRootSignature;						// Our command list root signature (ie shader entry points info)

		static const uint32_t MAX_RENDER_TARGET_CNT = 16;
		CComPtr<ID3D12Resource> mRenderTarget[MAX_RENDER_TARGET_CNT];		// The render target we're going to use
		CComPtr<ID3D12Resource> mVertexBuffer;								// GPU vertex data
		CComPtr<ID3D12Resource> mVertexBufferUploadTmp;						// Intermediate heap used to upload vertex data
		D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;							// View to vertex data for rendering

		DX12Texture* mTextures[DX12ResourceProvider::MAX_SUPPORTED_WINDOWS];// Set of managed textures

		bool mIsValid;
		bool mResourcesInitialized;											// We use our first render loop to upload the vertex data and all.

		friend class DX12Texture;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	DX12ResourceProvider::DX12ResourceProvider(PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignatureFcn)
		: mSerializeRootSignatureFcn(serializeRootSignatureFcn)
	{
		memset(&mCurrentTargetInfo, 0, sizeof(mCurrentTargetInfo));
		ZeroMemory(mTextureCache, sizeof(mTextureCache));
	}

	DX12ResourceProvider::~DX12ResourceProvider()
	{
		// Useful? Reset(true);
	}

	bool DX12ResourceProvider::Init(ID3D12Device* device, IDXGISwapChain3* swapChain, ID3D12CommandQueue* cmdQueue)
	{
#ifdef DEBUG
//		IGOLogInfo("Initialize resources for device=%p, swapChain=%p, cmdQueue=%p", device, swapChain, cmdQueue);
#endif

		// Make sure this is a graphics command queue - otherwise we're going to need to update the code (in case we have the same crazy
		// scenarios as Mantle, ie flushing the swap chain on a compute command queue)
		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(cmdQueue, &queueDevice, &nodeMask))
			return false;

		if (queueDevice != device)
		{
			IGOLogWarn("Queue is on different device than swapchain (%p/%p)!", device, queueDevice);
		}

		NodeResources* resources = NULL;
		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
		{
			IGOLogInfo("Creating new node resources for node=0x%08x", nodeMask);
			// Time to setup new rendering resources
			resources = new NodeResources(queueDevice, swapChain, cmdQueue, nodeMask, mSerializeRootSignatureFcn);

			RenderContext ctxt;
			ZeroMemory(&ctxt, sizeof(ctxt));
			ctxt.device = queueDevice;
			ctxt.nodeMask = nodeMask;
			ctxt.resources = resources;

			mRenderContexts.push_back(ctxt);
		}

		else
		{
#ifdef DEBUG
//			IGOLogInfo("Reusing node resources for node=0x%08x", nodeMask);
#endif
			resources = mRenderContexts[ctxtIdx].resources;
		}

		return resources->IsValid();
	}

	void DX12ResourceProvider::Reset(bool skipCommandCompletion)
	{
		IGOLogWarn("Resetting resources (SkipCommandCompletion=%d)", skipCommandCompletion);

		for (size_t idx = 0; idx < mRenderContexts.size(); ++idx)
		{
			// Should we wait for the completion of the command lists?
			if (!skipCommandCompletion)
			{
				ID3D12CommandQueue* queue = NULL;
				mRenderContexts[idx].resources->WaitForCompletion(queue);
			}

			delete mRenderContexts[idx].resources;

			for (int idx = 0; idx < MAX_SUPPORTED_WINDOWS; ++idx)
			{
				char* data = mTextureCache[idx].data;
				if (data)
					delete[] data;
			}

			ZeroMemory(mTextureCache, sizeof(mTextureCache));
		}

		mRenderContexts.clear();
	}

	bool DX12ResourceProvider::BeginCommandList(const D3DMATRIX& projMatrix, uint32_t width, uint32_t height, const TargetInfo& targetInfo)
	{
		mCurrentTargetInfo = targetInfo;

		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(targetInfo.cmdQueue, &queueDevice, &nodeMask))
			return false;

		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
			return false;

		NodeResources* resources = mRenderContexts[ctxtIdx].resources;
		return resources->BeginCommandList(targetInfo.cmdQueue, targetInfo.currentBackbufferIdx, projMatrix, width, height);
	}

	void DX12ResourceProvider::EndCommandList()
	{
		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(mCurrentTargetInfo.cmdQueue, &queueDevice, &nodeMask))
			return;

		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
			return;

		NodeResources* resources = mRenderContexts[ctxtIdx].resources;
		resources->EndCommandList(mCurrentTargetInfo.cmdQueue, mCurrentTargetInfo.currentBackbufferIdx);

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	int  DX12ResourceProvider::CreateTexture2D(uint32_t width, uint32_t height, const char* data, size_t dataSize)
	{
#ifdef DEBUG_TEXTURES
		IGOLogWarn("CreateTexture2D: %d x %d", width, height);
#endif

		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(mCurrentTargetInfo.cmdQueue, &queueDevice, &nodeMask))
			return INVALID_TEXTURE_ID;

		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
			return INVALID_TEXTURE_ID;

#ifdef DEBUG_TEXTURES
		IGOLogWarn("CreateTexture2D: device=%p, queue=%p, nodeMask=0x%08x, width=%d, height=%d, dataSize=%d", queueDevice, mCurrentTargetInfo.cmdQueue, nodeMask, width, height, dataSize);
#endif

#ifdef MULTIPLE_UPDATES_PER_FRAME_ARE_HANDLED_AT_THE_IPC_LEVEL
		RenderContext* ctxt = &mRenderContexts[ctxtIdx];
#endif

		// Lookup for the next texID to use
		int texID = 0;
		for (; texID < MAX_SUPPORTED_WINDOWS; ++texID)
		{
			if (!mTextureCache[texID].inUse)
			{
				bool readyForUse = true;

				// Although not "in use", a node may still be using it for the last time
				for (size_t idx = 0; idx < mRenderContexts.size(); ++idx)
				{
					if (mRenderContexts[idx].resources->IsTextureInUse(texID))
					{
						readyForUse = false;
						break;
					}
				}

				if (readyForUse)
					break;
			}
		}

		if (texID == MAX_SUPPORTED_WINDOWS)
		{
			CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 reached the limit of OIG windows (%d)", MAX_SUPPORTED_WINDOWS));
			return INVALID_TEXTURE_ID;
		}

		// Cache the data in case we need it on multiple nodes
		TextureCacheInfo* backup = &mTextureCache[texID];
		backup->data = new char[dataSize];
		memcpy(backup->data, data, dataSize);
		backup->dataSize = dataSize;
		backup->width = width;
		backup->height = height;
		backup->inUse = true;

		// Invalidate this texture for all nodes, EVEN the current one -> we're going to wait until we try to render the texture before creating/updating
		// the data - this is because we may have received multiple sequential requests during the same frame 
		for (RenderContexts::iterator iter = mRenderContexts.begin(); iter != mRenderContexts.end(); ++iter)
		{
			iter->textures[texID].dirty = true;
			iter->textures[texID].created = false;
		}

#ifdef MULTIPLE_UPDATES_PER_FRAME_ARE_HANDLED_AT_THE_IPC_LEVEL
		ctxt->textures[texID].dirty = false;
		ctxt->textures[texID].created = true;

		// Prepare upload
		ctxt->resources->CreateTexture2D(texID, width, height, data, dataSize);
#endif

#ifdef DEBUG_TEXTURES
		IGOLogWarn("New texture %d created", texID);
#endif
		return texID;
	}

	void DX12ResourceProvider::UpdateTexture2D(int texID, uint32_t width, uint32_t height, const char* data, size_t dataSize)
	{
		if (texID == INVALID_TEXTURE_ID || !mTextureCache[texID].inUse)
			return;

#ifdef DEBUG_TEXTURES
		IGOLogWarn("UpdateTexture2D texID=%d, %d x %d", texID, width, height);
#endif

		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(mCurrentTargetInfo.cmdQueue, &queueDevice, &nodeMask))
			return;

		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
			return;

#ifdef DEBUG_TEXTURES
		IGOLogWarn("UpdateTexture2D %d: device=%p, queue=%p, nodeMask=0x%08x, width=%d, height=%d, dataSize=%d", texID, queueDevice, mCurrentTargetInfo.cmdQueue, nodeMask, width, height, dataSize);
#endif

#ifdef MULTIPLE_UPDATES_PER_FRAME_ARE_HANDLED_AT_THE_IPC_LEVEL
		RenderContext* ctxt = &mRenderContexts[ctxtIdx];
#endif

		// Update local cache
		TextureCacheInfo* backup = &mTextureCache[texID];
		if (backup->width != width || backup->height != height)
		{
			delete[] backup->data;

			backup->data = new char[dataSize];
			memcpy(backup->data, data, dataSize);
			backup->dataSize = dataSize;
			backup->width = width;
			backup->height = height;
			backup->inUse = true;
		}

		else
			memcpy(backup->data, data, dataSize);

		// Invalidate this texture for all nodes, EVEN the current one -> we're going to wait until we try to render the texture before creating/updating
		// the data - this is because we may have received multiple sequential requests during the same frame 
		for (RenderContexts::iterator iter = mRenderContexts.begin(); iter != mRenderContexts.end(); ++iter)
			iter->textures[texID].dirty = true;

#ifdef MULTIPLE_UPDATES_PER_FRAME_ARE_HANDLED_AT_THE_IPC_LEVEL
		ctxt->textures[texID].dirty = false;

		// Do we need to create the texture or simply update it?
		if (!ctxt->textures[texID].created)
		{
			ctxt->textures[texID].created = true;
			ctxt->resources->CreateTexture2D(texID, width, height, data, dataSize);
		}

		else
			ctxt->resources->UpdateTexture2D(texID, width, height, data, dataSize);
#endif
	}

	void DX12ResourceProvider::DeleteTexture2D(int texID)
	{
		if (texID == INVALID_TEXTURE_ID || !mTextureCache[texID].inUse)
			return;

#ifdef DEBUG_TEXTURES
		IGOLogWarn("DeleteTexture2D texID=%d", texID);
#endif
		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(mCurrentTargetInfo.cmdQueue, &queueDevice, &nodeMask))
			return;

		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
			return;

#ifdef DEBUG_TEXTURES
		IGOLogWarn("DeleteTexture2D %d: device=%p, queue=%p, nodeMask=0x%08x", texID, queueDevice, mCurrentTargetInfo.cmdQueue, nodeMask);
#endif

		RenderContext* ctxt = &mRenderContexts[ctxtIdx];

		// Clean up our cache
		delete[] mTextureCache[texID].data;
		ZeroMemory(&(mTextureCache[texID]), sizeof(TextureCacheInfo));

		// Invalidate this texture for all nodes
		for (RenderContexts::iterator iter = mRenderContexts.begin(); iter != mRenderContexts.end(); ++iter)
		{
			iter->textures[texID].dirty = true;
			iter->textures[texID].created = false;
		}

		ctxt->resources->DeleteTexture2D(texID);
	}

    void DX12ResourceProvider::RenderQuad(int texID, float x, float y, float width, float height, float alpha, ShaderEffect effect, float effectParam)
	{
		if (texID == INVALID_TEXTURE_ID || !mTextureCache[texID].inUse)
			return;

#ifdef DEBUG_TEXTURES
		IGOLogWarn("RenderQuad texID=%d, xy=(%f, %f), %d x %d, alpha=%f, effect=%d, effectParam=%f", texID, x, y, width, height, alpha, effect, effectParam);
#endif

		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(mCurrentTargetInfo.cmdQueue, &queueDevice, &nodeMask))
			return;

		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
			return;

		RenderContext* ctxt = &mRenderContexts[ctxtIdx];

		// Is the texture data up-to-date on this node?
		if (ctxt->textures[texID].dirty)
		{
			ctxt->textures[texID].dirty = false;

			// Do we need to create the texture or simply update it?
			if (!ctxt->textures[texID].created)
			{
#ifdef DEBUG_TEXTURES
				IGOLogWarn("RenderQuad %d - create texture first on new node: device=%p, queue=%p, nodeMask=0x%08x", texID, queueDevice, mCurrentTargetInfo.cmdQueue, nodeMask);
#endif
				ctxt->textures[texID].created = true;
				ctxt->resources->CreateTexture2D(texID, mTextureCache[texID].width, mTextureCache[texID].height, mTextureCache[texID].data, mTextureCache[texID].dataSize);
			}

			else
			{
#ifdef DEBUG_TEXTURES
				IGOLogWarn("RenderQuad %d - updating texture first on new node: device=%p, queue=%p, nodeMask=0x%08x", texID, queueDevice, mCurrentTargetInfo.cmdQueue, nodeMask);
#endif
				ctxt->resources->UpdateTexture2D(texID, mTextureCache[texID].width, mTextureCache[texID].height, mTextureCache[texID].data, mTextureCache[texID].dataSize);
			}
		}

		ctxt->resources->RenderQuad(texID, x, y, width, height, alpha, effect, effectParam);

	}

	void DX12ResourceProvider::ForceTDR()
	{
		// Just pick first context
		if (mRenderContexts.size() > 0)
			mRenderContexts[0].resources->ForceTDR();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	int DX12ResourceProvider::FindRenderContext(ID3D12Device* device, UINT nodeMask)
	{
		for (size_t idx = 0; idx < mRenderContexts.size(); ++idx)
		{
			if (mRenderContexts[idx].device == device && mRenderContexts[idx].nodeMask == nodeMask)
				return static_cast<int>(idx);
		}

		return -1;
	}

} // OriginIGO
