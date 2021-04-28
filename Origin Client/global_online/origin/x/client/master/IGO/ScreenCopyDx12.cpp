///////////////////////////////////////////////////////////////////////////////
// ScreenCopyDX12.cpp
// 
// Created by Frederic Meraud
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ScreenCopyDx12.h"
#include "tchar.h"
#include "strsafe.h"
#include "IGO.h"
#include "resource.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "twitchmanager.h"
#include "HookAPI.h"
#include "Helpers.h"
#include "IGOLogger.h"
#include "IGOApplication.h"
#include "IGOSharedStructs.h"
#include "DX12ResourceProvider.h"
#include "IGOTelemetry.h"

namespace OriginIGO {

	extern HMODULE gInstDLL;

	// DX12 APIs to use
#if 0 // In case we go back to hooking command queue/command list ops
	EXTERN_NEXT_FUNC(void, DX12CommandQueueExecuteCommandListsHook, (ID3D12CommandQueue* This, UINT Count, ID3D12CommandList *const *ppCommandLists))
	EXTERN_NEXT_FUNC(HRESULT, DX12GraphicsCommandListCloseHook, (ID3D12GraphicsCommandList* This))
	EXTERN_NEXT_FUNC(HRESULT, DX12GraphicsCommandListResetHook, (ID3D12GraphicsCommandList* This, ID3D12CommandAllocator *pAllocator, ID3D12PipelineState *pInitialState))

	#define DX12CommandQueueExecuteCommandListsFcn(This, Count, ppCommandLists) DX12CommandQueueExecuteCommandListsHookNext(This, Count, ppCommandLists)
	#define DX12GraphicsCommandListCloseFcn(This) DX12GraphicsCommandListCloseHookNext(This)
	#define DX12GraphicsCommandListResetFcn(This, pAllocator, pInitialState) DX12GraphicsCommandListResetHookNext(This, pAllocator, pInitialState)
#else
	#define DX12CommandQueueExecuteCommandListsFcn(This, Count, ppCommandLists) This->ExecuteCommandLists(Count, ppCommandLists)
	#define DX12GraphicsCommandListCloseFcn(This) This->Close()
	#define DX12GraphicsCommandListResetFcn(This, pAllocator, pInitialState) This->Reset(pAllocator, pInitialState)
#endif

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	static const int MAX_SEQUENTIAL_SCREEN_CAPTURES = 32;
	static int ScreenCaptureIdx = MAX_SEQUENTIAL_SCREEN_CAPTURES;	// Don't start screen captures until ResetScreenCaptures() is called

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ScreenCopyDx12::NodeResources
	{
		struct QuadVertex
		{
			struct
			{
				float x;
				float y;
				float z;
			} pos;

			struct
			{
				float u;
				float v;
			} tex;
		};


	public:
		NodeResources(const ScreenCopyDx12::Config& config, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, UINT nodeMask, PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignatureFcn)
		{
			IGOLogInfo("Creating new Twitch NodeResources: device=%p, swapchain=%p, cmdQueue=%p, nodeMask=%d", device, config.swapChain, cmdQueue, nodeMask);

			mIsValid = false;
			mConfig = config;
			mDevice = device;
			mBackbufferCnt = 0;
			mCurrentFenceID = 0;
			mCurrentFrameIdx = 0;
			mResourcesInitialized = false;

			DXGI_SWAP_CHAIN_DESC swDesc;
			HRESULT hr = mConfig.swapChain->GetDesc(&swDesc);
			if (FAILED(hr))
			{
				CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to query swap chain desc (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
				return;
			}

			for (int idx = 0; idx < BUFFER_FRAME_CNT; ++idx)
			{
				mCmdListFence[idx] = 0;

				// A command allocator
				hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdAllocator[idx]));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 unable to create cmd allocator (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mCmdAllocator[idx], L"TwitchCommandAllocator%d", idx);

				hr = device->CreateCommandList(nodeMask, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocator[idx], mPso, IID_PPV_ARGS(&mCmdList[idx]));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create initial command list (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mCmdList[idx], L"TwitchCommandList%d", idx);

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

				DX12_NAME_OBJECT(mRootSignature, L"TwitchRootSignature");
			}

			// Descriptor heaps
			{
				// - For render target view
				mBackbufferCnt = swDesc.BufferCount;
				if (mBackbufferCnt == 0 || mBackbufferCnt > MAX_BACKBUFFER_CNT)
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 Too many back buffers to support (%d/%d) - device=%p, queue=%p, nodeMask=%d", mBackbufferCnt, MAX_BACKBUFFER_CNT, device, cmdQueue, nodeMask));
					return;
				}

				D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
				descHeap.NumDescriptors = BUFFER_FRAME_CNT; // Our intermediate texture views
				descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				descHeap.NodeMask = nodeMask;
				hr = device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&mRtvDescriptorHeap));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create rtv descriptor heap (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mRtvDescriptorHeap, L"Twitch_RTVHeap");

				// - For shader resource views
				D3D12_DESCRIPTOR_HEAP_DESC descHeapCbvSrv = {};
				descHeapCbvSrv.NumDescriptors = mBackbufferCnt; // Our source texture views, ie the swapchain backbuffers
				descHeapCbvSrv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				descHeapCbvSrv.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				descHeapCbvSrv.NodeMask = nodeMask;
				hr = device->CreateDescriptorHeap(&descHeapCbvSrv, IID_PPV_ARGS(&mCbvSrvDescriptorHeap));
				if (FAILED(hr))
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create cbv/srv descriptor heap (0x%08x) - device=%p, queue=%p, nodeMask=%d", hr, device, cmdQueue, nodeMask));
					return;
				}

				DX12_NAME_OBJECT(mRtvDescriptorHeap, L"Twitch_CBV_SRVHeap");

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

				DX12_NAME_OBJECT(mSamplerDescriptorHeap, L"Twitch_SamplerHeap");
			}

			// setup sampler
			{
				// get a handle to the start of the descriptor heap
				D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = mSamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

				// create the wrapping sampler
				D3D12_SAMPLER_DESC sampDesc;
				ZeroMemory(&sampDesc, sizeof(sampDesc));
				sampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
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
				blendDesc.RenderTarget[0].BlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
				blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
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
				psoDesc.PS = mConfig.hardwareAccelerated ? YUVPixelShaderInfo() : PixelShaderInfo();
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

				DX12_NAME_OBJECT(mPso, L"TwitchPSO");
			}

			// Setup backbuffers as shader sources
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
				shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

				CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (UINT idx = 0; idx < mBackbufferCnt; ++idx)
				{
					hr = mConfig.swapChain->GetBuffer(idx, IID_PPV_ARGS(&mBackbufferSource[idx]));
					if (FAILED(hr))
					{
						CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to access swap chain buffer %d (0x%08x) - device=%p, queue=%p, nodeMask=%d", idx, hr, device, cmdQueue, nodeMask));
						return;
					}

					device->CreateShaderResourceView(mBackbufferSource[idx], &shaderResourceViewDesc, cpuHandle);
					cpuHandle.Offset(1, descriptorSize);
				}
			}

			// Setup render targets
			{
				CD3DX12_HEAP_PROPERTIES texHeap(D3D12_HEAP_TYPE_DEFAULT, nodeMask, nodeMask);
				D3D12_HEAP_FLAGS texHeapFlags = D3D12_HEAP_FLAG_NONE;

				UINT16 arraySize = 1;
				UINT16 mipLevels = 1;
				DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
				UINT sampleCount = 1;
				UINT sampleQuality = 0;
				D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, mConfig.width, mConfig.height, arraySize, mipLevels, sampleCount, sampleQuality, flags);

				D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
				renderTargetViewDesc.Format = format;
				renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				renderTargetViewDesc.Texture2D.MipSlice = 0;
				renderTargetViewDesc.Texture2D.PlaneSlice = 0;

				CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				for (UINT idx = 0; idx < BUFFER_FRAME_CNT; ++idx)
				{
					HRESULT hr = device->CreateCommittedResource(&texHeap, texHeapFlags, &texDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mTargetTextures[idx]));
					if (FAILED(hr))
					{
						CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create texture GPU resource for screen copy (0x%08x)", hr));
						return;
					}

					device->CreateRenderTargetView(mTargetTextures[idx], &renderTargetViewDesc, cpuHandle);
					cpuHandle.Offset(1, descriptorSize);

					DX12_NAME_OBJECT(mTargetTextures[idx], L"TwitchTargetTexture%d", idx);
				}

				// Store info we're going to need when copyign to from render texture to readback buffer
				device->GetCopyableFootprints(&texDesc, 0, 1, 0, &mReadbackFootprint, &mReadbackNumRows, &mReadbackRowSizesInBytes, &mReadbackBufferSize);
				IGOLogInfo("Readback footprint width=%u, height=%u, rowPitch=%u - numRows=%u, rowSizeinBytes=%llu, bufferSize=%llu", mReadbackFootprint.Footprint.Width, mReadbackFootprint.Footprint.Height, mReadbackFootprint.Footprint.RowPitch, mReadbackNumRows, mReadbackRowSizesInBytes, mReadbackBufferSize);
			}

			// Readback buffers
			{
				for (UINT idx = 0; idx < BUFFER_FRAME_CNT; ++idx)
				{
					CD3DX12_HEAP_PROPERTIES texReadbackHeap(D3D12_HEAP_TYPE_READBACK, nodeMask, nodeMask);
					D3D12_HEAP_FLAGS texReadbackHeapFlags = D3D12_HEAP_FLAG_NONE;
					CD3DX12_RESOURCE_DESC texReadbackDesc = CD3DX12_RESOURCE_DESC::Buffer(mReadbackBufferSize);

					HRESULT hr = device->CreateCommittedResource(&texReadbackHeap, texReadbackHeapFlags, &texReadbackDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mBackupBuffers[idx]));
					if (FAILED(hr))
					{
						CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 failed to create texture readback resource (0x%08x)", hr));
						return;
					}

					DX12_NAME_OBJECT(mBackupBuffers[idx], L"TwitchReadbackBuffer%d", idx);
				}
			}

			// Create command list for initial GPU setup
			{
				// create the vertex buffer
				{
					QuadVertex vtx[4];
					ComputeVertexBuffer(vtx);

					size_t vertexDataSize = sizeof(vtx);
					CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, nodeMask, nodeMask);
					CD3DX12_RESOURCE_DESC vertexDataDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexDataSize);
					hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &vertexDataDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mVertexBuffer));

					// schedule a copy to get data in the buffer
					if (SUCCEEDED(hr))
					{
						DX12_NAME_OBJECT(mVertexBuffer, L"TwitchCommittedVB");

						heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, nodeMask, nodeMask);
						hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &vertexDataDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexBufferUploadTmp));

						if (SUCCEEDED(hr))
						{
							DX12_NAME_OBJECT(mVertexBufferUploadTmp, L"TwitchUploadVB");

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

				DX12_NAME_OBJECT(mFence, L"TwitchFence");
			}

			mIsValid = true;
		}

		~NodeResources()
		{
			for (UINT idx = 0; idx < mBackbufferCnt; ++idx)
				mBackbufferSource[idx].Release();

			mFence.Release();

			for (int idx = 0; idx < BUFFER_FRAME_CNT; ++idx)
			{
				mCmdList[idx].Release();
				mCmdAllocator[idx].Release();
				mTargetTextures[idx].Release();
				mBackupBuffers[idx].Release();
			}

			mCbvSrvDescriptorHeap.Release();
			mRtvDescriptorHeap.Release();
			mSamplerDescriptorHeap.Release();

			mPso.Release();
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

		bool CaptureBackbufferAndUpdateTwitch(ID3D12CommandQueue* queue, UINT backbufferIdx, bool recordData)
		{
			if (!IsValid())
				return false;

			if (mResourcesInitialized)
			{
				// Make sure to wait until our next set of resources aren't in use by the GPU
				const UINT64 lastCompletedFence = mFence->GetCompletedValue();
				mCurrentFrameIdx = (mCurrentFrameIdx + 1) % DX12ResourceProvider::BUFFER_FRAME_CNT;
				uint64_t cmdListFence = mCmdListFence[mCurrentFrameIdx];

				PIX_SET_MARKER(queue, L"Testing for fence - last completed=%d, new frame idx=%d, frame fence=%d...", lastCompletedFence, mCurrentFrameIdx, cmdListFence);

				// Check if this frame resource is still in use by the GPU; wait on its associated fence if not available yet
				if (lastCompletedFence < cmdListFence)
				{
					PIX_BEGIN_EVENT(queue, L"Frame resources in use, waiting for fence...");

                    // We can remove this once we're done with support for XP/can switch to Windows 10 SDK... or newer!
                    static NotXPSupportedAPIs::CreateEventExFn createEventEx = GetNotXPSupportedAPIs()->createEventEx;
					HANDLE eventHandle = createEventEx ? createEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS) : createEventEx;
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
				HRESULT hr = mCmdAllocator[mCurrentFrameIdx]->Reset();
				if (FAILED(hr))
				{
					IGOLogWarn("Failed to reset command allocator %d", mCurrentFrameIdx);
					return false;
				}

				hr = DX12GraphicsCommandListResetFcn(mCmdList[mCurrentFrameIdx], mCmdAllocator[mCurrentFrameIdx], mPso);
				if (FAILED(hr))
				{
					IGOLogWarn("Failed to reset command list %d", mCurrentFrameIdx);
					return false;
				}

				// Prepare our texture render target
				SetResourceBarrier(mCmdList[mCurrentFrameIdx], mTargetTextures[mCurrentFrameIdx], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			else
			{
				mResourcesInitialized = true;

				// Prepare our texture render target
				SetResourceBarrier(mCmdList[mCurrentFrameIdx], mTargetTextures[mCurrentFrameIdx], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			// First, can we copy over the previous data to Twitch?
			if (recordData && mCurrentFenceID > BUFFER_FRAME_CNT)
			{
				unsigned char* data = NULL;
				HRESULT hr = mBackupBuffers[mCurrentFrameIdx]->Map(0, NULL, reinterpret_cast<void**>(&data));
				if (SUCCEEDED(hr))
				{
//#define DUMP_SNAPSHOTS
//#ifdef DUMP_SNAPSHOTS
					if (ScreenCaptureIdx < MAX_SEQUENTIAL_SCREEN_CAPTURES)
					{
						UINT64 snapshotSize = mReadbackRowSizesInBytes * mReadbackNumRows;
						unsigned char* snapshot = new unsigned char[static_cast<unsigned int>(snapshotSize)];
						
						D3D12_SUBRESOURCE_DATA srcData = { data, mReadbackFootprint.Footprint.RowPitch, 0 };
						D3D12_MEMCPY_DEST dstData = { snapshot, static_cast<SIZE_T>(mReadbackRowSizesInBytes), 0 };
						MemcpySubresource(&dstData, &srcData, static_cast<SIZE_T>(mReadbackRowSizesInBytes), mReadbackNumRows, 1);

						static WCHAR folderName[MAX_PATH] = { 0 };
						if (!folderName[0])
						{
							WCHAR rootPath[MAX_PATH] = { 0 };
							if (GetCommonDataPath(rootPath))
							{
								StringCchPrintf(folderName, _countof(folderName), L"%s\\Origin", rootPath);
								if (!DirectoryExists(folderName))
								{
									if (CreateDirectory(folderName, NULL) == 0)
									{
										DWORD error = GetLastError();
										IGOLogWarn("Unable to create folder '%S' (%d)", folderName, error);
									}
								}

								StringCchPrintf(folderName, _countof(folderName), L"%s\\Origin\\DX12Snapshots", rootPath);
								if (!DirectoryExists(folderName))
								{
									if (CreateDirectory(folderName, NULL) == 0)
									{
										DWORD error = GetLastError();
										IGOLogWarn("Unable to create folder '%S' (%d)", folderName, error);
									}
								}
							}
						}
						WCHAR fileName[MAX_PATH];
						StringCchPrintf(fileName, _countof(fileName), L"%s\\Snapshot%03d.tga", folderName, ScreenCaptureIdx);
						SaveRawRGBAToTGA(fileName, static_cast<size_t>(mReadbackRowSizesInBytes / 4), mReadbackNumRows, snapshot, true);

						delete[]snapshot;
						++ScreenCaptureIdx;
					}

					else
//#else
					{
						TwitchManager::tPixelBuffer *pixelBuffer = TwitchManager::GetUnusedPixelBuffer();
						if (pixelBuffer)
						{
							D3D12_SUBRESOURCE_DATA srcData = { data, mReadbackFootprint.Footprint.RowPitch, 0 };
							D3D12_MEMCPY_DEST dstData = { pixelBuffer->data, static_cast<SIZE_T>(mReadbackRowSizesInBytes), 0 };
							MemcpySubresource(&dstData, &srcData, static_cast<SIZE_T>(mReadbackRowSizesInBytes), mReadbackNumRows, 1);
							TwitchManager::TTV_SubmitVideoFrame(pixelBuffer->data, pixelBuffer);
						}
					}
//#endif
					mBackupBuffers[mCurrentFrameIdx]->Unmap(0, NULL);
				}

				else
					IGOLogWarn("Failed to map readback buffer %d (hr=0x%08x)", mCurrentFrameIdx, hr);
			}

			// Make the current backbuffer our shader texture source
			SetResourceBarrier(mCmdList[mCurrentFrameIdx], mBackbufferSource[backbufferIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			// Setup root signature
			mCmdList[mCurrentFrameIdx]->SetGraphicsRootSignature(mRootSignature);

			// Setup viewport and scissor
			D3D12_VIEWPORT viewport;
			ZeroMemory(&viewport, sizeof(viewport));
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = static_cast<float>(mConfig.width);
			viewport.Height = static_cast<float>(mConfig.height);
			viewport.MaxDepth = 1.0f;
			viewport.MinDepth = 0.0f;
			mCmdList[mCurrentFrameIdx]->RSSetViewports(1, &viewport);

			D3D12_RECT scissorRect = { 0, 0, mConfig.width, mConfig.height };
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
			rtvCpuHandle.Offset(mCurrentFrameIdx, descriptorSize);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptors[] = { rtvCpuHandle };
			mCmdList[mCurrentFrameIdx]->OMSetRenderTargets(1, rtvDescriptors, false, nullptr);

			// Set root entries
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle(mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			cbvSrvGpuHandle.Offset(backbufferIdx, mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

			mCmdList[mCurrentFrameIdx]->SetGraphicsRootDescriptorTable(0, cbvSrvGpuHandle);
			mCmdList[mCurrentFrameIdx]->SetGraphicsRootDescriptorTable(1, mSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

///////////////////////////////////

			// TODO: do I need to perform ResolvedSubResource to go from multi-sample to single-sample?

			mCmdList[mCurrentFrameIdx]->DrawInstanced(4, 1, 0, 0);

			// Copy the data to our readback buffer
			SetResourceBarrier(mCmdList[mCurrentFrameIdx], mTargetTextures[mCurrentFrameIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

			CD3DX12_TEXTURE_COPY_LOCATION src(mTargetTextures[mCurrentFrameIdx], 0);
			CD3DX12_TEXTURE_COPY_LOCATION dst(mBackupBuffers[mCurrentFrameIdx], mReadbackFootprint);
			mCmdList[mCurrentFrameIdx]->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

///////////////////////////////////

			// Restore backbuffer for 'Present' and our intermediate texture as render target
			SetResourceBarrier(mCmdList[mCurrentFrameIdx], mBackbufferSource[backbufferIdx], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PRESENT);

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

			return true;
		}

	private:
		static ID3DBlob* RootSignature(PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignatureFcn)
		{
			static bool initialized = false;
			static CComPtr<ID3DBlob> signature;

			if (!initialized)
			{
				initialized = true;

				CD3DX12_DESCRIPTOR_RANGE descRange[2]; // Perfomance TIP: Order from most frequent to least frequent
				descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);		// 1 frequently changed diffuse - t0
				descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);	// 1 static sampler - s0

				CD3DX12_ROOT_PARAMETER rootParameters[2];
				rootParameters[0].InitAsDescriptorTable(1, &descRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
				rootParameters[1].InitAsDescriptorTable(1, &descRange[1], D3D12_SHADER_VISIBILITY_PIXEL);

				CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
				descRootSignature.Init(2, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
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
				vertexShader.BytecodeLength = 0;
				vertexShader.pShaderBytecode = NULL;

				HRSRC resourceInfo = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOVSQUADFX), RT_RCDATA);
				if (resourceInfo)
				{
					HGLOBAL hRes = LoadResource(gInstDLL, resourceInfo);
					vertexShader.pShaderBytecode = LockResource(hRes);
					vertexShader.BytecodeLength = SizeofResource(gInstDLL, resourceInfo);
				}

				else
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 Missing vertex shader resource"));
				}
			}

			return vertexShader;
		}

		static D3D12_SHADER_BYTECODE PixelShaderInfo()
		{
			static bool initialized = false;
			static D3D12_SHADER_BYTECODE pixelShader;

			if (!initialized)
			{
				initialized = true;
				pixelShader.BytecodeLength = 0;
				pixelShader.pShaderBytecode = NULL;

				HRSRC resourceInfo = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOPSQUADFX), RT_RCDATA);
				if (resourceInfo)
				{
					HGLOBAL hRes = LoadResource(gInstDLL, resourceInfo);
					pixelShader.pShaderBytecode = LockResource(hRes);
					pixelShader.BytecodeLength = SizeofResource(gInstDLL, resourceInfo);
				}

				else
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 Missing pixel shader resource"));
				}
			}

			return pixelShader;
		}

		static D3D12_SHADER_BYTECODE YUVPixelShaderInfo()
		{
			static bool initialized = false;
			static D3D12_SHADER_BYTECODE pixelShader;

			if (!initialized)
			{
				initialized = true;
				pixelShader.BytecodeLength = 0;
				pixelShader.pShaderBytecode = NULL;

				HRSRC resourceInfo = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOPSQUADFX_YUV), RT_RCDATA);
				if (resourceInfo)
				{
					HGLOBAL hRes = LoadResource(gInstDLL, resourceInfo);
					pixelShader.pShaderBytecode = LockResource(hRes);
					pixelShader.BytecodeLength = SizeofResource(gInstDLL, resourceInfo);
				}

				else
				{
					CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "DX12 Missing YUV pixel shader resource"));
				}
			}

			return pixelShader;
		}

		void ComputeVertexBuffer(QuadVertex vtx[4])
		{
			// calculate aspect ratio and scaling values - game screen vs video resolution
			int gameHeight = (int)(((float)((float)mConfig.sourceHeight / (float)mConfig.sourceWidth)) * mConfig.width);
			int gameWidth = mConfig.width;

			if (gameHeight > mConfig.height)
			{
				float scale = (((float)mConfig.height) / ((float)gameHeight));
				gameHeight  = mConfig.height;
				gameWidth   = (int)(gameWidth * scale);
			}

			if (gameWidth > mConfig.width)
			{
				float scale = (((float)mConfig.width) / ((float)gameWidth));
				gameWidth   = mConfig.width;
				gameHeight  = (int)(gameHeight * scale);
			}


			float scaledRect_left;
			float scaledRect_right;
			float scaledRect_top;
			float scaledRect_bottom;

			scaledRect_left   = ((mConfig.width - gameWidth) * 0.5f);
			scaledRect_right  = (gameWidth + (mConfig.width - gameWidth) * 0.5f);
			scaledRect_top    = ((mConfig.height - gameHeight) * 0.5f);
			scaledRect_bottom = (gameHeight + (mConfig.height - gameHeight) * 0.5f);

			// translate
			scaledRect_left   -= mConfig.width * 0.5f;
			scaledRect_right  -= mConfig.width * 0.5f;
			scaledRect_top    -= mConfig.height * 0.5f;
			scaledRect_bottom -= mConfig.height * 0.5f;

			// scale
			scaledRect_left   *= 2.0f / mConfig.width;
			scaledRect_right  *= 2.0f / mConfig.width;
			scaledRect_top    *= 2.0f / mConfig.height;
			scaledRect_bottom *= 2.0f / mConfig.height;

			vtx[0].pos = { scaledRect_left, scaledRect_bottom, 0.5f };
			vtx[0].tex = { 0.0f, 0.0f };

			vtx[1].pos = { scaledRect_right, scaledRect_bottom, 0.5f };
			vtx[1].tex = { 1.0f, 0.0f };

			vtx[2].pos = { scaledRect_left, scaledRect_top, 0.5f };
			vtx[2].tex = { 0.0f, 1.0f };

			vtx[3].pos = { scaledRect_right, scaledRect_top, 0.5f };
			vtx[3].tex = { 1.0f, 1.0f };
		}


	private:
		static const int BUFFER_FRAME_CNT = 3;
		static const int MAX_CBV_SRV_DESCRIPTOR_CNT = BUFFER_FRAME_CNT;

		ScreenCopyDx12::Config mConfig;										// Twitch config: source dims, twitch dims, fps to respect, ...

		ID3D12Device* mDevice;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT mReadbackFootprint;				// Info to use for copying to readback buffer
		UINT64 mReadbackBufferSize;											
		UINT64 mReadbackRowSizesInBytes;
		UINT mReadbackNumRows;

		UINT mBackbufferCnt;												// Swap chain buffer count

		uint64_t mCurrentFenceID;
		CComPtr<ID3D12Fence> mFence;										// Our main fence object to detect when rendering is completed

		uint32_t mCurrentFrameIdx;											// Cycles through 3 sequential command queues
		uint64_t mCmdListFence[BUFFER_FRAME_CNT];							// Current
		CComPtr<ID3D12GraphicsCommandList> mCmdList[BUFFER_FRAME_CNT];
		CComPtr<ID3D12CommandAllocator> mCmdAllocator[BUFFER_FRAME_CNT];

		CComPtr<ID3D12DescriptorHeap> mCbvSrvDescriptorHeap;				// Constant buffer view / shader resource view heap
		CComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;					// Render target view heap
		CComPtr<ID3D12DescriptorHeap> mSamplerDescriptorHeap;				// Sampler descriptor heap

		CComPtr<ID3D12PipelineState> mPso;									// Pipeline state object 
		CComPtr<ID3D12RootSignature> mRootSignature;						// Our command list root signature (ie shader entry points info)

		static const uint32_t MAX_BACKBUFFER_CNT = 16;
		CComPtr<ID3D12Resource> mBackbufferSource[MAX_BACKBUFFER_CNT];		// The swap chain backbuffers used as shader texture sources
		CComPtr<ID3D12Resource> mTargetTextures[BUFFER_FRAME_CNT];			// Intermediate textures used to store single sample version of backbuffer
		CComPtr<ID3D12Resource> mBackupBuffers[BUFFER_FRAME_CNT];			// Actual readback resources 

		CComPtr<ID3D12Resource> mVertexBuffer;								// GPU vertex data
		CComPtr<ID3D12Resource> mVertexBufferUploadTmp;						// Intermediate heap used to upload vertex data
		D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;							// View to vertex data for rendering

		bool mIsValid;
		bool mResourcesInitialized;

		friend class DX12Texture;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	ScreenCopyDx12::ScreenCopyDx12()
	{
		ZeroMemory(&mConfig, sizeof(mConfig));
	}

	ScreenCopyDx12::~ScreenCopyDx12()
	{
	}

	int ScreenCopyDx12::FindRenderContext(ID3D12Device* device, UINT nodeMask)
	{
		for (size_t idx = 0; idx < mRenderContexts.size(); ++idx)
		{
			if (mRenderContexts[idx].device == device && mRenderContexts[idx].nodeMask == nodeMask)
				return static_cast<int>(idx);
		}

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region IScreenCopy Implementation

	bool ScreenCopyDx12::Create(void *pDevice, void *pSwapChain, int widthOverride, int heightOverride)
	{
		mConfig.device = static_cast<ID3D12Device*>(pDevice);
		mConfig.swapChain = static_cast<IDXGISwapChain*>(pSwapChain);

		mConfig.sourceWidth = widthOverride;
		mConfig.sourceHeight = heightOverride;

		return TTV_SUCCEEDED(TwitchManager::InitializeForStreaming(mConfig.sourceWidth, mConfig.sourceHeight, mConfig.width, mConfig.height, mConfig.fps));
	}

	int ScreenCopyDx12::GetObjectCount()
	{
		return 0;
	}

	bool ScreenCopyDx12::Reset(void *pDevice)
	{
		return true;
	}

	bool ScreenCopyDx12::Lost()
	{
		return true;
	}

	bool ScreenCopyDx12::Destroy(void *userData)
	{
		bool forceReset = false;
		if (userData)
			forceReset = *static_cast<bool*>(userData);

		IGOLogWarn("Destroy (forceReset=%d)", forceReset);

		if (TwitchManager::IsTTVDLLReady())
		{
			TwitchManager::TTV_PauseVideo();
		}

		for (size_t idx = 0; idx < mRenderContexts.size(); ++idx)
		{
			// Should we wait for the completion of the command lists?
			if (!forceReset)
			{
				ID3D12CommandQueue* queue = NULL;
				mRenderContexts[idx].resources->WaitForCompletion(queue);
			}

			delete mRenderContexts[idx].resources;
		}

		mRenderContexts.clear();
		return true;
	}

	bool ScreenCopyDx12::Render(void *context, void *)
	{
		// Are we in stable condition?
		if (!IGOApplication::instance())
		{
			TwitchManager::TTV_PauseVideo();
			return true;
		}

		// Have we saturated Twitch?
		static LARGE_INTEGER last_update = { 0, 0 };
		static LARGE_INTEGER freq = { 0, 0 };

		if (freq.QuadPart == 0)
		{
			QueryPerformanceFrequency(&freq);
		}

		LARGE_INTEGER current_update;
		QueryPerformanceCounter(&current_update);

		// Feed the encoder with 1.25x the frame rate to prevent stuttering!
		bool recordData = true;
		if (TwitchManager::GetUsedPixelBufferCount() > 16 /*too many unencoded frames 1920*1080 -> 16 frames ~ 125MB*/
			|| ((current_update.QuadPart - last_update.QuadPart) * (mConfig.fps * 1.25f)) / freq.QuadPart == 0)
			recordData = false;

		last_update = current_update;

		// Find our render context
		DX12ResourceProvider::TargetInfo* targetInfo = static_cast<DX12ResourceProvider::TargetInfo*>(context);
		if (!targetInfo)
			return false;

		if (targetInfo->swapChain != mConfig.swapChain)
		{
			IGOLogWarn("Multiple swapchains in use (%p/%p)", targetInfo->swapChain, mConfig.swapChain);
			return false;
		}

		// Make sure we have this context already setup
		UINT nodeMask = 0;
		ID3D12Device* queueDevice = NULL;
		if (!ExtractDeviceAndNodeMask(targetInfo->cmdQueue, &queueDevice, &nodeMask))
			return false;

		NodeResources* resources = NULL;
		int ctxtIdx = FindRenderContext(queueDevice, nodeMask);
		if (ctxtIdx < 0)
		{
			// Time to setup new rendering resources
			HMODULE hD3D12 = GetModuleHandle(L"d3d12.dll");
			if (hD3D12)
			{
				PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(hD3D12, "D3D12SerializeRootSignature"));
				if (serializeRootSignature)
				{
					resources = new NodeResources(mConfig, queueDevice, targetInfo->cmdQueue, nodeMask, serializeRootSignature);

					RenderContext ctxt;
					ZeroMemory(&ctxt, sizeof(ctxt));
					ctxt.device = queueDevice;
					ctxt.nodeMask = nodeMask;
					ctxt.resources = resources;

					mRenderContexts.push_back(ctxt);
				}
			}
		}

		else
			resources = mRenderContexts[ctxtIdx].resources;

		if (resources)
			return resources->CaptureBackbufferAndUpdateTwitch(targetInfo->cmdQueue, targetInfo->currentBackbufferIdx, recordData);

		return false;
	}

	bool ScreenCopyDx12::CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth)
	{
		return true;
	}

	bool ScreenCopyDx12::CopyFromMemory(void *pDevice, void *pSource, int width, int heigth)
	{
		return false;
	}

	bool ScreenCopyDx12::isSRGB_format(DXGI_FORMAT f)
	{
		switch (f)
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

	void ScreenCopyDx12::ResetScreenCaptures()
	{
		ScreenCaptureIdx = 0;
	}

#pragma endregion
}
