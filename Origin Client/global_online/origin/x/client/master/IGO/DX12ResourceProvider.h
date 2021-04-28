///////////////////////////////////////////////////////////////////////////////
// DX12RESOURCEPROVIDER_H.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef DX12RESOURCEPROVIDER_H
#define DX12RESOURCEPROVIDER_H

#include <windowsx.h>
#include <atlbase.h>
//#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <d3dtypes.h>
#include <EASTL/vector.h>
#include "IGOLogger.h"


namespace OriginIGO
{
#if _DEBUG
#include <tchar.h>
#include <strsafe.h>

#define DX12_NAME_OBJECT(obj, fmt, ...) \
	{ \
		WCHAR fullName[128]; \
		StringCbPrintfW(fullName, sizeof(fullName), L"%S(%d)_" ## fmt, __FUNCTION__, __LINE__, __VA_ARGS__); \
		(obj)->SetName(fullName); \
	}
#else
#define DX12_NAME_OBJECT(obj, fmt, ...)
#endif

//#define USE_PIX_MARKERS

#ifdef _DEBUG

#ifdef USE_PIX_MARKERS

#define PIX_SET_MARKER(cmdListOrQueue, fmt, ...) \
		{ \
			WCHAR msg[512]; \
			StringCbPrintfW(msg, sizeof(msg), L"%S(%d) - " ## fmt, __FUNCTION__, __LINE__, __VA_ARGS__); \
			PIXSetMarker(cmdListOrQueue, 0, msg); \
		}
#define PIX_BEGIN_EVENT(cmdListOrQueue, fmt, ...) \
		{ \
			WCHAR msg[512]; \
			StringCbPrintfW(msg, sizeof(msg), L"%S(%d) - " ## fmt, __FUNCTION__, __LINE__, __VA_ARGS__); \
			PIXBeginEvent(cmdListOrQueue, 0, msg); \
		}
#define PIX_END_EVENT(cmdListOrQueue) \
		PIXEndEvent(cmdListOrQueue);

#else // USE_PIX_MARKERS

#define PIX_SET_MARKER(cmdListOrQueue, fmt, ...) \
		{ \
			WCHAR msg[512]; \
			StringCbPrintfW(msg, sizeof(msg), fmt, __VA_ARGS__); \
			\
			char cmsg[1024] = { 0 }; \
			WideCharToMultiByte(CP_UTF8, 0, msg, -1, cmsg, sizeof(cmsg), NULL, NULL); \
			IGOLogger::instance()->warn(__FILE__, __LINE__, false, cmsg); \
		}
#define PIX_BEGIN_EVENT(cmdListOrQueue, fmt, ...) \
		{ \
			WCHAR msg[512]; \
			StringCbPrintfW(msg, sizeof(msg), fmt, __VA_ARGS__); \
			\
			char cmsg[1024] = { 0 }; \
			WideCharToMultiByte(CP_UTF8, 0, msg, -1, cmsg, sizeof(cmsg), NULL, NULL); \
			IGOLogger::instance()->warn(__FILE__, __LINE__, false, cmsg); \
		}
#define PIX_END_EVENT(cmdListOrQueue)

#endif // USE_PIX_MARKERS

#else // _DEBUG
#define PIX_SET_MARKER(cmdListOrQueue, fmt, ...)
#define PIX_BEGIN_EVENT(cmdListOrQueue, fmt, ...)
#define PIX_END_EVENT(cmdListOrQueue)
#endif // _DEBUG


	class DX12ResourceProvider
	{
		// Fwd decls
		class NodeResources;

		struct CBPerFrame;
		struct CBPerWindow;


	public:
		static const int BUFFER_FRAME_CNT = 3;
		static const int INVALID_TEXTURE_ID = -1;
		static const uint32_t MAX_SUPPORTED_WINDOWS = 32;	// We can have up to 32 windows/tooltip open at once in OIG

	public:
		DX12ResourceProvider(PFN_D3D12_SERIALIZE_ROOT_SIGNATURE serializeRootSignatureFcn);
		~DX12ResourceProvider();

		bool Init(ID3D12Device* device, IDXGISwapChain3* swapChain, ID3D12CommandQueue* cmdQueue);
		void Reset(bool skipCommandCompletion);

		struct TargetInfo
		{
			ID3D12CommandQueue* cmdQueue;
			IDXGISwapChain* swapChain;
			UINT currentBackbufferIdx;
		};
		bool BeginCommandList(const D3DMATRIX& projMatrix, uint32_t width, uint32_t height, const TargetInfo& targetInfo);
		void EndCommandList();

		int  CreateTexture2D(uint32_t width, uint32_t height, const char* data, size_t dataSize);
		void UpdateTexture2D(int texID, uint32_t width, uint32_t height, const char* data, size_t dataSize);
		void DeleteTexture2D(int texID);

        enum ShaderEffect
        {
            ShaderEffect_NONE,
            ShaderEffect_BACKGROUND
        };
		void RenderQuad(int texID, float x, float y, float width, float height, float alpha, ShaderEffect effect, float effectParam);

		void ForceTDR(); // for debugging

	private:
		int FindRenderContext(ID3D12Device* device, UINT nodeIdx);


		PFN_D3D12_SERIALIZE_ROOT_SIGNATURE mSerializeRootSignatureFcn;		// Pointer to D3D12 method used to serialize command list root signatures

		TargetInfo mCurrentTargetInfo;										// Current render target setup extracted from hooking side

		// List of resources on a per node (a node = hardware GPU under the same adapter) basis
		struct RenderContext
		{
			ID3D12Device* device;
			NodeResources* resources;
			UINT nodeMask;

			struct TextureState
			{
				bool dirty;			// the texture data is invalid and needs to be updated before the texture can actually be rendered on this node
				bool created;		// the texture object already exists for this node
			} textures[MAX_SUPPORTED_WINDOWS];
		};
		typedef eastl::vector<RenderContext> RenderContexts;
		RenderContexts mRenderContexts;

		// Cache the texture data in case we create/update the data on one node, but later
		// use it on another node
		struct TextureCacheInfo
		{
			char* data;
			size_t dataSize;

			uint32_t width;
			uint32_t height;

			bool inUse;

		};
		TextureCacheInfo mTextureCache[MAX_SUPPORTED_WINDOWS];
	};

	///////////////////////////////////////////////// Helpers ////////////////////////////////////////////////////////////

	void SetResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
	bool ExtractDeviceAndNodeMask(ID3D12CommandQueue* queue, ID3D12Device** device, UINT* nodeMask);
	void WaitForCompletion(ID3D12CommandQueue* queue, ID3D12Fence* fence, uint64_t currentFenceID);

} // OriginIGO

#endif