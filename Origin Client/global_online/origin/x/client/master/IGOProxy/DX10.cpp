#include "Helpers.h"

#include <windows.h>
#include <d3d10.h>
#include <d3d10misc.h>
#include <dxgi.h>
#include <Psapi.h>

#include <string>

#include "IGOAPIInfo.h"

namespace IGOProxy
{
	int StoreDX10OffsetInfo(IDXGISwapChain* swapChain)
	{
		// Create the reference name from the API + DLLs we depend upon. This may be too much, or not enough, but we need a way to uniquely identify the condition
		// + need to be able to use that data from OIG while in the loader thread (ie too dangerous to use APIs like GetFileVersionInfo, etc...)
		char dx10DllMd5[33] = { 0 };
		HMODULE dx10Handle = GetModuleHandle(TEXT("d3d10.dll"));
		if (!ComputeFileMd5(dx10Handle, dx10DllMd5))
			return -1;

		char dxgiDllMd5[33] = { 0 };
		HMODULE dxgiHandle = GetModuleHandle(TEXT("dxgi.dll"));
		if (!ComputeFileMd5(dxgiHandle, dxgiDllMd5))
			return -1;

		char fileName[MAX_PATH] = { 0 };
		StringCchPrintfA(fileName, _countof(fileName), "DX10_%s_%s.igo", dx10DllMd5, dxgiDllMd5);

		// Time to fill the information we want to convey
		IGOAPIInfo_V1 apiInfo;
        apiInfo.DX10.swapChainReleaseOffset = (LONG_PTR)GetInterfaceMethod(swapChain, 2) - (LONG_PTR)dxgiHandle;
		apiInfo.DX10.swapChainPresentOffset = (LONG_PTR)GetInterfaceMethod(swapChain, 8) - (LONG_PTR)dxgiHandle;
        apiInfo.DX10.swapChainResizeOffset  = (LONG_PTR)GetInterfaceMethod(swapChain, 13) - (LONG_PTR)dxgiHandle;

#ifdef DEBUG
        IGOProxyLogInfo("DX10 SwapChain Release Offset = 0x%08xL", apiInfo.DX10.swapChainReleaseOffset);
        IGOProxyLogInfo("DX10 SwapChain Present Offset = 0x%08xL", apiInfo.DX10.swapChainPresentOffset);
        IGOProxyLogInfo("DX10 SwapChain Resize Offset = 0x%08xL", apiInfo.DX10.swapChainResizeOffset);
#endif

		int retVal = StoreLookupData(fileName, &apiInfo, sizeof(apiInfo));

        return retVal;
	}

	int LookupDX10Offsets()
	{
		int retVal = -1;

		IGOProxyLogInfo("Looking up DX10 Offsets");

		HMODULE dx10Handle = LoadLibrary(TEXT("d3d10.dll"));
		if (dx10Handle)
		{
            typedef HRESULT (WINAPI *PFN_D3D10_CREATE_DEVICE_AND_SWAP_CHAIN)(IDXGIAdapter* pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, UINT SDKVersion, DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, IDXGISwapChain** ppSwapChain, ID3D10Device** ppDevice);
			PFN_D3D10_CREATE_DEVICE_AND_SWAP_CHAIN createDevice = reinterpret_cast<PFN_D3D10_CREATE_DEVICE_AND_SWAP_CHAIN>(GetProcAddress(dx10Handle, "D3D10CreateDeviceAndSwapChain"));
			if (createDevice)
			{
				HWND hWnd = createRenderWindow(L"DX10");
				if (hWnd)
				{
					ID3D10Device* device = NULL;
                    IDXGISwapChain* dxgiSwapChain = NULL;

   					DXGI_SWAP_CHAIN_DESC swapChainDesc;
					ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
					swapChainDesc.BufferCount = 1;
					swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
					swapChainDesc.OutputWindow = hWnd;
					swapChainDesc.SampleDesc.Count = 1;
                    swapChainDesc.SampleDesc.Quality = 0;
					swapChainDesc.Windowed = TRUE;

                    HRESULT hr = createDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &dxgiSwapChain, &device);
                    if (SUCCEEDED(hr))
                    {
						retVal = StoreDX10OffsetInfo(dxgiSwapChain);

          				if (device)
					        device->Release();

				        if (dxgiSwapChain)
					        dxgiSwapChain->Release();
                    }

                    else
                        IGOProxyLogError("Unable to create device/swapchain - hr=0x%08x", hr);

                    DestroyWindow(hWnd);

                } // hWnd

                FreeLibrary(dx10Handle);
            }
            else
				IGOProxyLogError("Unable to find D3D11CreateDeviceAndSwapChain");
		}

		else
			IGOProxyLogError("Unable to load DX11 dll");

		return retVal;
	}
}
