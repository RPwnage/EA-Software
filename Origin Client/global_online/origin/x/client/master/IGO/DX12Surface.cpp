///////////////////////////////////////////////////////////////////////////////
// DX12Surface.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "DX12Surface.h"

#if defined(ORIGIN_PC)

#include <d3d12.h>
#include <d3dtypes.h>

#include "IGOIPC/IGOIPC.h"

#include "IGOWindow.h"
#include "IGOApplication.h"
#include "IGOSharedStructs.h"
#include "DX12ResourceProvider.h"

namespace OriginIGO {

	class DX12SurfaceImpl
	{
	public:
		DX12SurfaceImpl(DX12ResourceProvider* provider, IGOWindow* window)
			: mProvider(provider), mWindow(window), mWidth(0), mHeight(0), mTextureID(DX12ResourceProvider::INVALID_TEXTURE_ID)
		{
		}

		~DX12SurfaceImpl() 
		{ 
			mProvider->DeleteTexture2D(mTextureID);
		}

		void DX12SurfaceImpl::render()
		{
			if (mTextureID != DX12ResourceProvider::INVALID_TEXTURE_ID)
			{
				mWindow->checkDirty();

				//Get coordinates
				float X = mWindow->x() - (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth) / 2;
				float Y = -mWindow->y() + (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight) / 2;

				float width = (float)mWindow->width();
				float height = (float)mWindow->height();
				float alpha = (float)mWindow->alpha() / 255.0f;
                float effectParam = (float)mWindow->customValue() / 10000.0f;

                DX12ResourceProvider::ShaderEffect effect = (mWindow->id() == BACKGROUND_WINDOW_ID) ? DX12ResourceProvider::ShaderEffect_BACKGROUND : DX12ResourceProvider::ShaderEffect_NONE;
                mProvider->RenderQuad(mTextureID, X, Y, width, height, alpha, effect, effectParam);
			}
		}

		void DX12SurfaceImpl::update(uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
		{
			if (!(flags & IGOIPC::WINDOW_UPDATE_WIDTH))
				width = mWidth;

			if (!(flags & IGOIPC::WINDOW_UPDATE_HEIGHT))
				height = mHeight;

			if (mTextureID == DX12ResourceProvider::INVALID_TEXTURE_ID)
				mTextureID = mProvider->CreateTexture2D(width, height, data, size);

			else
			if (mWidth != width || mHeight != height || (flags & IGOIPC::WINDOW_UPDATE_BITS))
				mProvider->UpdateTexture2D(mTextureID, width, height, data, size);
		}

	private:
		DX12ResourceProvider* mProvider;
		IGOWindow* mWindow;

		int mWidth;
		int mHeight;
		int mTextureID;
	};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

	DX12Surface::DX12Surface(void* context, IGOWindow* window)
	{
		mImpl = new DX12SurfaceImpl(reinterpret_cast<DX12ResourceProvider*>(context), window);
	}

	DX12Surface::~DX12Surface()
	{
		delete mImpl;
	}

	void DX12Surface::render(void* context)
	{
		mImpl->render();
	}

	void DX12Surface::update(void* context, uint8_t alpha, int32_t width, int32_t height, const char* data, int size, uint32_t flags)
	{
		mImpl->update(alpha, width, height, data, size, flags);
	}

}

#endif // ORIGIN_PC