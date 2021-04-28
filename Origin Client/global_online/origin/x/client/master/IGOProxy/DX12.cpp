#include "Helpers.h"

#include <windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include "../IGO/d3dx12.h"
#include <dxgi1_4.h>
#include <Psapi.h>

#include <string>

#include "IGOAPIInfo.h"

namespace IGOProxy
{
	int StoreOffsetInfo(IDXGIFactory4* dxgiFactory4, IDXGISwapChain3* swapChain3)
	{
		// Create the reference name from the API + DLLs we depend upon. This may be too much, or not enough, but we need a way to uniquely identify the condition
		// + need to be able to use that data from OIG while in the loader thread (ie too dangerous to use APIs like GetFileVersionInfo, etc...)
		char dx12DllMd5[33] = { 0 };
		HMODULE dx12Handle = GetModuleHandle(TEXT("d3d12.dll"));
		if (!ComputeFileMd5(dx12Handle, dx12DllMd5))
			return -1;

		char dxgiDllMd5[33] = { 0 };
		HMODULE dxgiHandle = GetModuleHandle(TEXT("dxgi.dll"));
		if (!ComputeFileMd5(dxgiHandle, dxgiDllMd5))
			return -1;

		char fileName[MAX_PATH] = { 0 };
		_snprintf(fileName, sizeof(fileName), "DX12_%s_%s.igo", dx12DllMd5, dxgiDllMd5);

		// Time to fill the information we want to convey
		IGOAPIInfo_V1 apiInfo;
		apiInfo.DX12.factoryCreateSwapChainOffset = (LONG_PTR)GetInterfaceMethod(dxgiFactory4, 10) - (LONG_PTR)dxgiHandle;

		apiInfo.DX12.swapChainReleaseOffset = (LONG_PTR)GetInterfaceMethod(swapChain3, 2) - (LONG_PTR)dxgiHandle;
		apiInfo.DX12.swapChainPresentOffset = (LONG_PTR)GetInterfaceMethod(swapChain3, 8) - (LONG_PTR)dxgiHandle;
		apiInfo.DX12.swapChainResizeBuffersOffset = (LONG_PTR)GetInterfaceMethod(swapChain3, 13) - (LONG_PTR)dxgiHandle; // ResizeBuffers for DX12 = Resize for DX10/DX11
		apiInfo.DX12.swapChainResizeBuffers1Offset = (LONG_PTR)GetInterfaceMethod(swapChain3, 39) - (LONG_PTR)dxgiHandle;

		return StoreLookupData(fileName, &apiInfo, sizeof(apiInfo));
	}

	int LookupDX12Offsets()
	{
		int retVal = -1;

		IGOProxyLogInfo("Looking up DX12 Offsets");

		HMODULE dx12Handle = LoadLibrary(TEXT("d3d12.dll"));
		if (dx12Handle)
		{
			PFN_D3D12_CREATE_DEVICE createDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(dx12Handle, "D3D12CreateDevice"));
			if (createDevice)
			{
				HWND hWnd = createRenderWindow(L"DX12");
				if (hWnd)
				{
#ifdef _DEBUG
					// Enable the D3D12 debug layer.
					{
						PFN_D3D12_GET_DEBUG_INTERFACE IDebug = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(GetProcAddress(dx12Handle, "D3D12GetDebugInterface"));
						if (IDebug)
						{
							ID3D12Debug* debugController = NULL;
							HRESULT hr = IDebug(IID_PPV_ARGS(&debugController));
							if (SUCCEEDED(hr))
							{
								debugController->EnableDebugLayer();
								debugController->Release();
							}

							else
								IGOProxyLogError("Unable to support debug mode - hr=0x%08x", hr);
						}
					}
#endif

					ID3D12Device* device = NULL;
					IDXGIFactory4* dxgiFactory4 = NULL;
					IDXGISwapChain* dxgiSwapChain = NULL;
					ID3D12CommandQueue* queue = NULL;

					HRESULT hr = createDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
					if (SUCCEEDED(hr))
					{
						D3D12_COMMAND_QUEUE_DESC queueDesc;
						ZeroMemory(&queueDesc, sizeof(queueDesc));
						queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
						queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
						hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
						if (SUCCEEDED(hr))
						{
							// At this point we should have loaded dxgi.dll
							typedef HRESULT (WINAPI *PFN_CREATE_DXGI_FACTORY1)(REFIID riid, _COM_Outptr_ void **ppFactory);
							PFN_CREATE_DXGI_FACTORY1 createDxgiFactory1 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(GetModuleHandle(TEXT("dxgi.dll")), "CreateDXGIFactory1"));
							hr = createDxgiFactory1(IID_PPV_ARGS(&dxgiFactory4));
							if (SUCCEEDED(hr))
							{
								DXGI_SWAP_CHAIN_DESC swapChainDesc;
								ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
								swapChainDesc.BufferCount = 2;
								swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
								swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
								swapChainDesc.OutputWindow = hWnd;
								swapChainDesc.SampleDesc.Count = 1;
								swapChainDesc.Windowed = TRUE;
								swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
								swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

								hr = dxgiFactory4->CreateSwapChain(queue, &swapChainDesc, &dxgiSwapChain);
								if (SUCCEEDED(hr))
								{
									IDXGISwapChain3* dxgiSwapChain3 = NULL;
									HRESULT hr = dxgiSwapChain->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
									if (SUCCEEDED(hr))
									{
										retVal = StoreOffsetInfo(dxgiFactory4, dxgiSwapChain3);
										dxgiSwapChain3->Release();
									}

									else
										IGOProxyLogError("Unable to query IDXGISwapChain3 interface - hr=0x%08x", hr);
								}
								else
									IGOProxyLogError("Unable to create dxgi swap chain - hr=0x%08x", hr);
							}
							else
								IGOProxyLogError("Unable to create dxgi factory4 - hr=0x%08x", hr);

						}
						else
							IGOProxyLogError("Unable to create command queue - hr=0x%08x", hr);
					}

					else
						IGOProxyLogError("Unable to create device HARDWARE/NONE/LEVEL_11_0 - hr=0x%08x", hr);

					if (device)
						device->Release();

					if (dxgiFactory4)
						dxgiFactory4->Release();

					if (dxgiSwapChain)
						dxgiSwapChain->Release();

					if (queue)
						queue->Release();

				} // if (hWnd)
			}
			else
				IGOProxyLogError("Unable to find D3D12CreateDevice");

			FreeLibrary(dx12Handle);
		}

		else
			IGOProxyLogError("Unable to load DX12 dll");

		return retVal;
	}
}