#include "Helpers.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <Psapi.h>

#include <string>

#include "IGOAPIInfo.h"

namespace IGOProxy
{
	int StoreDX11OffsetInfo(IDXGISwapChain* swapChain)
	{
		// Create the reference name from the API + DLLs we depend upon. This may be too much, or not enough, but we need a way to uniquely identify the condition
		// + need to be able to use that data from OIG while in the loader thread (ie too dangerous to use APIs like GetFileVersionInfo, etc...)
		char dx11DllMd5[33] = { 0 };
		HMODULE dx11Handle = GetModuleHandle(TEXT("d3d11.dll"));
		if (!ComputeFileMd5(dx11Handle, dx11DllMd5))
			return -1;

		char dxgiDllMd5[33] = { 0 };
		HMODULE dxgiHandle = GetModuleHandle(TEXT("dxgi.dll"));
		if (!ComputeFileMd5(dxgiHandle, dxgiDllMd5))
			return -1;

		char fileName[MAX_PATH] = { 0 };
		StringCchPrintfA(fileName, _countof(fileName), "DX11_%s_%s.igo", dx11DllMd5, dxgiDllMd5);

		// Time to fill the information we want to convey
		IGOAPIInfo_V1 apiInfo;
        apiInfo.DX11.swapChainReleaseOffset = (LONG_PTR)GetInterfaceMethod(swapChain, 2) - (LONG_PTR)dxgiHandle;
		apiInfo.DX11.swapChainPresentOffset = (LONG_PTR)GetInterfaceMethod(swapChain, 8) - (LONG_PTR)dxgiHandle;
        apiInfo.DX11.swapChainResizeOffset  = (LONG_PTR)GetInterfaceMethod(swapChain, 13) - (LONG_PTR)dxgiHandle;

#ifdef DEBUG
        IGOProxyLogInfo("DX11 SwapChain Release Offset = 0x%08xL", apiInfo.DX11.swapChainReleaseOffset);
        IGOProxyLogInfo("DX11 SwapChain Present Offset = 0x%08xL", apiInfo.DX11.swapChainPresentOffset);
        IGOProxyLogInfo("DX11 SwapChain Resize Offset = 0x%08xL", apiInfo.DX11.swapChainResizeOffset);
#endif

		int retVal = StoreLookupData(fileName, &apiInfo, sizeof(apiInfo));

        return retVal;
	}

	int LookupDX11Offsets()
	{
		int retVal = -1;

 		IGOProxyLogInfo("Looking up DX11 Offsets");

		HMODULE dx11Handle = LoadLibrary(TEXT("d3d11.dll"));
		if (dx11Handle)
		{
			PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN createDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN>(GetProcAddress(dx11Handle, "D3D11CreateDeviceAndSwapChain"));
			if (createDevice)
			{
				HWND hWnd = createRenderWindow(L"DX11");
				if (hWnd)
				{
					ID3D11Device* device = NULL;
                    ID3D11DeviceContext* deviceCtxt = NULL;
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

                    D3D_FEATURE_LEVEL featureLevels[] =
                    {
                        (D3D_FEATURE_LEVEL)0xb100, /*D3D_FEATURE_LEVEL_11_1*/
                        D3D_FEATURE_LEVEL_11_0,
                        D3D_FEATURE_LEVEL_10_0,
                    };

                    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

                    HRESULT hr = createDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swapChainDesc, &dxgiSwapChain, &device, NULL, &deviceCtxt);
                    if (FAILED(hr))
                    {
                        IGOProxyLogError("Failed to create device/swap chain while using 11_1 feature level. Trying again by skipping it... - hr=0x%08x", hr);

                        hr = createDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &(featureLevels[1]), numFeatureLevels - 1, D3D11_SDK_VERSION, &swapChainDesc, &dxgiSwapChain, &device, NULL, &deviceCtxt);
                        if (FAILED(hr))
                            IGOProxyLogError("Failed to create device/swap chain - hr=0x%08x", hr); 
                    }

                    if (SUCCEEDED(hr))
                    {
						retVal = StoreDX11OffsetInfo(dxgiSwapChain);

          				if (device)
					        device->Release();

				        if (deviceCtxt)
					        deviceCtxt->Release();

				        if (dxgiSwapChain)
					        dxgiSwapChain->Release();
                    }

                    DestroyWindow(hWnd);

                } // hWnd

                FreeLibrary(dx11Handle);
            }
            else
				IGOProxyLogError("Unable to find D3D11CreateDeviceAndSwapChain");
		}

		else
			IGOProxyLogError("Unable to load DX11 dll");

		return retVal;
	}
}
