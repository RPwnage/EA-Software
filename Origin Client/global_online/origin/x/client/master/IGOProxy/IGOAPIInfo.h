#ifndef IGO_API_INFO_DOT_H
#define IGO_API_INFO_DOT_H

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)

struct IGOAPIInfo
{
	static const uint32_t CurrentVersion = 2;

	uint32_t version;
};

typedef HRESULT(WINAPI *IGOAPIDXCreateSwapChainFcn)(IDXGIFactory* This, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

typedef ULONG (WINAPI *IGOAPIDXReleaseFcn)(IDXGISwapChain* pSwapChain);
typedef HRESULT (WINAPI *IGOAPIDXPresentFcn)(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT (WINAPI *IGOAPIDXResizeFcn)(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef HRESULT(WINAPI *IGOAPIDXResizeBuffersFcn)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags); // ResizeBuffers for DX12 = Resize for DX10/DX11
typedef HRESULT(WINAPI *IGOAPIDXResizeBuffers1Fcn)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue);

struct IGOAPIInfo_V1 : public IGOAPIInfo
{
	IGOAPIInfo_V1() 
	{ 
        ZeroMemory(this, sizeof(*this));

        version = CurrentVersion;
        ;
    }

    union
    {
        struct
        {
            union
            {
                IGOAPIDXReleaseFcn swapChainReleaseAddr;
                uint64_t swapChainReleaseOffset;
            };
            union
            {
                IGOAPIDXPresentFcn swapChainPresentAddr;
                uint64_t swapChainPresentOffset;
            };
            union
            {
                IGOAPIDXResizeFcn swapChainResizeAddr;
                uint64_t swapChainResizeOffset;
            };
        } DX10;

        struct
        {
            union
            {
                IGOAPIDXReleaseFcn swapChainReleaseAddr;
                uint64_t swapChainReleaseOffset;
            };
            union
            {
                IGOAPIDXPresentFcn swapChainPresentAddr;
                uint64_t swapChainPresentOffset;
            };
            union
            {
                IGOAPIDXResizeFcn swapChainResizeAddr;
                uint64_t swapChainResizeOffset;
            };
        } DX11;
        
		struct
		{
			union
			{
				IGOAPIDXCreateSwapChainFcn factoryCreateSwapChainAddr;
				uint64_t factoryCreateSwapChainOffset;
			};
			union
			{
				IGOAPIDXReleaseFcn swapChainReleaseAddr;
				uint64_t swapChainReleaseOffset;
			};
			union
			{
				IGOAPIDXPresentFcn swapChainPresentAddr;
				uint64_t swapChainPresentOffset;
			};
			union
			{
				IGOAPIDXResizeBuffersFcn swapChainResizeBuffersAddr; // ResizeBuffers for DX12 = Resize for DX10/DX11
				uint64_t swapChainResizeBuffersOffset;
			};
			union
			{
				IGOAPIDXResizeBuffers1Fcn swapChainResizeBuffers1Addr;
				uint64_t swapChainResizeBuffers1Offset;
			};
		} DX12;

    };
};

#pragma pack(pop)

#endif // IGO_API_INFO_DOT_H