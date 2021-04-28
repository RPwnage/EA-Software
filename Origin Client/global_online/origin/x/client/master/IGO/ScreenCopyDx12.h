///////////////////////////////////////////////////////////////////////////////
// ScreenCopyDX12.h
// 
// Created by Frederic Meraud
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef __SCREEN_COPY_DX12_H__
#define __SCREEN_COPY_DX12_H__

#include "IScreenCopy.h"

#include <d3d12.h>
#include <d3dtypes.h>
#include <EASTL/vector.h>

namespace OriginIGO {

	class ScreenCopyDx12 : public IScreenCopy
	{
	public:

		//////////////////////////////////////////////////////////
		/// \brief ScreenCopy Constructor.
		ScreenCopyDx12();

		//////////////////////////////////////////////////////////
		/// \brief ScreenCopy Destructor.
		virtual ~ScreenCopyDx12();


	public: // implementation of IScreenCopy

#pragma region IScreenCopy
		//////////////////////////////////////////////////////////
		/// \brief Call resources that will 'survive' a device lost scenario.
		/// \param pDevice The device to create all the resources on.
		/// \return returns true if all resources are allocated.
		virtual bool Create(void *pDevice, void *pSwapChain = 0, int widthOverride = 0, int heightOverride = 0);

		//////////////////////////////////////////////////////////
		/// \brief Create all the resources that will need to be released when a device is lost.
		/// \param pDevice The device to create the resources on.
		/// \return Returns true if all resources are allocated.
		virtual bool Reset(void *pDevice);

		//////////////////////////////////////////////////////////
		/// \brief Release all resources that have to be released when a device is lost.
		/// \param pDevice The device to release the resources on.
		/// \return Returns true if all resources are properly released.
		virtual bool Lost();

		//////////////////////////////////////////////////////////
		/// \brief Release all resources we created on this device.
		virtual bool Destroy(void *userData = NULL);

		//////////////////////////////////////////////////////////
		/// \brief Perform the render operation.
		/// \param pDevice The device to perform the render operation on.
		/// \return returns true if the render operation succeeded.
		virtual bool Render(void *pDevice, void *pSwapChain = 0);

		//////////////////////////////////////////////////////////
		/// \brief Copy the texture from device texture to memory
		/// \param [in] pDevice the device to copy from.
		/// \param [in] pDestination The system memory to copy the texture to.
		/// \param [in,out] The buffer size of the result. Pass in the available size of the buffer. If the buffer is too small size will return the necessary amount needed. 
		/// \param [out] width The horizontal size of the image.
		/// \param [out] height The vertical size of the image.
		virtual bool CopyToMemory(void *pDevice, void *pDestination, int &size, int &width, int &heigth);

		//////////////////////////////////////////////////////////
		/// \brief Copy the memory texture to the device to be rendered in the next render call.
		///    \param [in] pDevice The device to render the texture to.
		/// \param [in] pSource The memory to copy the texture contents from.
		/// \param [in] width The width of the texture
		/// \param [in] height The height of the texture
		virtual bool CopyFromMemory(void *pDevice, void *pSource, int width, int heigth);

		//////////////////////////////////////////////////////////
		/// \brief Returns the DirectX object count for propper release hook calculations
		virtual int GetObjectCount();

		//////////////////////////////////////////////////////////
		/// \brief Returns true if we have a SRGB colour format
		static bool isSRGB_format(DXGI_FORMAT f);
#pragma endregion


	public:
		struct Config
		{
			int width;						///< The width of the output image
			int height;						///< The height of the output image
			int sourceWidth;				///< The width of the source image
			int sourceHeight;				///< The height of the source image
			int fps;						///< The number of frames per second
			bool hardwareAccelerated;

			ID3D12Device* device;			// Device associated with swapchain currently in use
			IDXGISwapChain* swapChain;		// Current swap chain we're capturing
		};

		static void ResetScreenCaptures();	// Save the screen as recorded for Twitch to disc for debugging

	private:
		int FindRenderContext(ID3D12Device* device, UINT nodeMask);


		Config mConfig;

		// List of resources on a per node (a node = hardware GPU under the same adapter) basis
		class NodeResources;
		struct RenderContext
		{
			ID3D12Device* device;
			NodeResources* resources;
			UINT nodeMask;
		};
		typedef eastl::vector<RenderContext> RenderContexts;
		RenderContexts mRenderContexts;
	};

}

#endif //__SCREEN_COPY_DX11_H__
