///////////////////////////////////////////////////////////////////////////////
// MantleSurface.cpp
// 
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "MantleSurface.h"
#include "MantleShader.h"
#include "MantleHook.h"

#if defined(ORIGIN_PC)

#include "mantle.h"
#include "MantleObjects.h"
#include "resource.h"

#include "IGOIPC/IGOIPC.h"
#include "IGOLogger.h"
#include "IGOTrace.h"
#include "IGOWindow.h"
#include "IGOApplication.h"

namespace OriginIGO {
	extern HINSTANCE gInstDLL;
	extern bool gWindowedMode;
	extern RenderPass *gRenderPass[GR_MAX_PHYSICAL_GPUS];
	extern ConstantBuffer *gNeverChangeConstants[GR_MAX_PHYSICAL_GPUS];
	extern RenderTarget *gRenderTarget[GR_MAX_PHYSICAL_GPUS];
	extern RenderState *gRenderState[GR_MAX_PHYSICAL_GPUS];
	extern GR_UINT32 gTextureHeap[GR_MAX_PHYSICAL_GPUS];
	extern Shader*  gShaderVs[GR_MAX_PHYSICAL_GPUS];
    extern Shader*	gShaderPs[GR_MAX_PHYSICAL_GPUS];
    extern Shader*  gShaderBgVs[GR_MAX_PHYSICAL_GPUS];
    extern Shader*	gShaderBgPs[GR_MAX_PHYSICAL_GPUS];


	struct QuadVertex
	{
		GR_FLOAT x, y, z, w;		//!< The transformed position for the vertex.
		GR_FLOAT tu1, tv1;			//!< texture coordinates
	};

	class MantleSurfacePrivate
	{
	public:
		MantleSurfacePrivate()
		{
			memset(&mDescriptorSet, 0, sizeof(mDescriptorSet));
			memset(&mSampler, 0, sizeof(mSampler));
			memset(&mTexture, 0, sizeof(mTexture));
			memset(&mMesh, 0, sizeof(mMesh));
			memset(&mPipeline, 0, sizeof(mPipeline));
			memset(&mPerPassConstants, 0, sizeof(mPerPassConstants));
			memset(&mDrawable, 0, sizeof(mDrawable));

		}
		~MantleSurfacePrivate() { }

		DescriptorSet*      mDescriptorSet[GR_MAX_PHYSICAL_GPUS];
		Sampler*            mSampler[GR_MAX_PHYSICAL_GPUS];
		Texture*            mTexture[GR_MAX_PHYSICAL_GPUS];
		Mesh*               mMesh[GR_MAX_PHYSICAL_GPUS];
		Pipeline*           mPipeline[GR_MAX_PHYSICAL_GPUS];
		ConstantBuffer*		mPerPassConstants[GR_MAX_PHYSICAL_GPUS];

		Drawable*			mDrawable[GR_MAX_PHYSICAL_GPUS];
	};

	MantleSurface::MantleSurface(void *pDev, IGOWindow *pWindow) :
		mWindow(pWindow),
		mAlpha(255),
		mBackupTexture(NULL),
		mBackupDevice(-1)

	{
		mPrivate = new MantleSurfacePrivate();
		memset(mDirtyTexture, 0, sizeof(mDirtyTexture));
		memset(mWidth, 0, sizeof(mWidth));
		memset(mHeight, 0, sizeof(mHeight));
	}

	void MantleSurface::initSurface(void *context)
	{
		int renderDevice = ((OriginIGO::MantleHook::DevContext*) context)->deviceIndex;
		CommandBuffer *cmdBuffer = ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer;
		tRenderTarget *rt = ((OriginIGO::MantleHook::DevContext*) context)->rt;

		IGO_ASSERT(rt);
		IGO_ASSERT(cmdBuffer);

		// setup the Scene
		GR_RESULT result = GR_SUCCESS;
		if (result == GR_SUCCESS)
		{
			// create pipeline
			GR_GRAPHICS_PIPELINE_CREATE_INFO pi = gPipelineInfo;
			pi.cbState.target->format = rt->mImageCreateInfo.format;
			IGO_ASSERT(mPrivate->mPipeline[renderDevice] == NULL);
			mPrivate->mPipeline[renderDevice] = new Pipeline();
            Shader* vsShader = gShaderVs[renderDevice];
            Shader* psShader = gShaderPs[renderDevice];
            if (mWindow->id() == BACKGROUND_WINDOW_ID)
            {
                vsShader = gShaderBgVs[renderDevice];
                psShader = gShaderBgPs[renderDevice];
            }
			result = mPrivate->mPipeline[renderDevice]->init(pi, vsShader, NULL, NULL, NULL, psShader, renderDevice);

			IGO_ASSERT(mPrivate->mSampler[renderDevice] == NULL);
			mPrivate->mSampler[renderDevice] = new Sampler();
			mPrivate->mSampler[renderDevice]->init(renderDevice, true);

			QuadVertex vtx[] = {
				{ 0, 0, 1, 1, 0.0f, 0.0f },
				{ 1, 0, 1, 1, 1.0f, 0.0f },
				{ 0, -1, 1, 1, 0.0f, 1.0f },
				{ 1, -1, 1, 1, 1.0f, 1.0f },
			};

			IGO_ASSERT(mPrivate->mMesh[renderDevice] == NULL);
			mPrivate->mMesh[renderDevice] = new Mesh();
			mPrivate->mMesh[renderDevice]->init(cmdBuffer, sizeof(vtx) / sizeof(QuadVertex), sizeof(QuadVertex), vtx, 0, 0, NULL, renderDevice);

			const unsigned char txt[] = { 0, 255, 0, 255 };	// dummy texture data
			IGO_ASSERT(mPrivate->mTexture[renderDevice] == NULL);
			mPrivate->mTexture[renderDevice] = new Texture(renderDevice);
			mPrivate->mTexture[renderDevice]->createTextureFromMemARGB8(gTextureHeap[renderDevice], cmdBuffer, (const unsigned char*)&txt[0], 1, 1);

			// no other GPU created this one yet
			if (mBackupDevice == -1)
			{
				EA::Thread::AutoFutex lock(mMemoryBarrier);
				DELETE_ARRAY_IF(mBackupTexture);
				mBackupTexture = new unsigned char[1 * 1 * 4];
				memcpy(mBackupTexture, (const unsigned char*)&txt[0], 1 * 1 * 4);
				mWidth[renderDevice] = 1;
				mHeight[renderDevice] = 1;
				mBackupDevice = renderDevice;
			}

			IGO_ASSERT(mPrivate->mDescriptorSet[renderDevice] == NULL);
			mPrivate->mDescriptorSet[renderDevice] = mPrivate->mPipeline[renderDevice]->createDescriptorSet();
			GR_MEMORY_VIEW_ATTACH_INFO memView = mPrivate->mMesh[renderDevice]->vbView();
			mPrivate->mDescriptorSet[renderDevice]->BindMemory(VERTEX_SHADER, DXST_VERTEXBUFFER, 0, 1, &memView);
			memView = gRenderPass[renderDevice]->getConstantsView();
			mPrivate->mDescriptorSet[renderDevice]->BindMemory(VERTEX_SHADER, DXST_CONSTANTS, 0, 1, &memView);
			GR_IMAGE_VIEW_ATTACH_INFO imageView = mPrivate->mTexture[renderDevice]->imageView();
			mPrivate->mDescriptorSet[renderDevice]->BindImages(PIXEL_SHADER, DXST_TEXTURE, 0, 1, &mPrivate->mTexture[renderDevice]->memory(), &imageView);
			mPrivate->mDescriptorSet[renderDevice]->BindSamplers(PIXEL_SHADER, DXST_SAMPLER, 0, 1, &mPrivate->mSampler[renderDevice]->sampler());

			IGO_ASSERT(mPrivate->mDrawable[renderDevice] == NULL);
			mPrivate->mDrawable[renderDevice] = new Drawable();
			mPrivate->mDrawable[renderDevice]->init(cmdBuffer, mPrivate->mMesh[renderDevice], mPrivate->mPipeline[renderDevice]);
			mPrivate->mDrawable[renderDevice]->setDescriptorSet(mPrivate->mDescriptorSet[renderDevice]);

			IGO_ASSERT(mPrivate->mPerPassConstants[renderDevice] == NULL);
			mPrivate->mPerPassConstants[renderDevice] = new ConstantBuffer();
			mPrivate->mPerPassConstants[renderDevice]->init(cmdBuffer, 4 * sizeof(float), 20 * sizeof(float), NULL, renderDevice);

			memView = mPrivate->mPerPassConstants[renderDevice]->view();
			mPrivate->mDrawable[renderDevice]->setConstantBuffer(&memView);

			gRenderPass[renderDevice]->add(mPrivate->mDrawable[renderDevice]);

		}
		else
		{
			IGO_ASSERT(0);
		}
	}

	MantleSurface::~MantleSurface()
	{
		if (mPrivate)
		{
			for (int renderDevice = 0; renderDevice < GR_MAX_PHYSICAL_GPUS; renderDevice++)
			{
				IGO_ASSERT(NULL == mPrivate->mDescriptorSet[renderDevice]);
				IGO_ASSERT(NULL == mPrivate->mSampler[renderDevice]);
				IGO_ASSERT(NULL == mPrivate->mTexture[renderDevice]);
				IGO_ASSERT(NULL == mPrivate->mMesh[renderDevice]);
				IGO_ASSERT(NULL == mPrivate->mPipeline[renderDevice]);
				IGO_ASSERT(NULL == mPrivate->mPerPassConstants[renderDevice]);
				IGO_ASSERT(NULL == mPrivate->mDrawable[renderDevice]);
			}
		}
	}

	void MantleSurface::dealloc(void *context)
	{
		GR_CMD_BUFFER cmdBuffer = context != NULL ? ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer->cmdBuffer() : NULL;
		
		mBackupDevice = -1;

		for (int renderDevice = 0; renderDevice < GR_MAX_PHYSICAL_GPUS; renderDevice++)
		{
			if (gRenderPass[renderDevice] != NULL && mPrivate->mDrawable[renderDevice] != NULL)
				gRenderPass[renderDevice]->remove(mPrivate->mDrawable[renderDevice]);

			//DELETE_MANTLE_IF(gRenderPass[renderDevice], cmdBuffer);

			DELETE_MANTLE_IF(mPrivate->mDrawable[renderDevice], cmdBuffer);
			DELETE_MANTLE_IF(mPrivate->mPerPassConstants[renderDevice], cmdBuffer);
			DELETE_MANTLE_IF(mPrivate->mPipeline[renderDevice], cmdBuffer);
			DELETE_MANTLE_IF(mPrivate->mTexture[renderDevice], cmdBuffer);
			DELETE_MANTLE_IF(mPrivate->mMesh[renderDevice], cmdBuffer);
			DELETE_MANTLE_IF(mPrivate->mSampler[renderDevice], cmdBuffer);
			DELETE_MANTLE_IF(mPrivate->mDescriptorSet[renderDevice], cmdBuffer);
		}
		DELETE_IF(mPrivate);
		EA::Thread::AutoFutex lock(mMemoryBarrier);
		DELETE_ARRAY_IF(mBackupTexture);
	}

	/* only needed if we use a cmd buffer that is not cleared after a submit*/
	void MantleSurface::unrender(void *mantleDeviceIndex)
	{
		unsigned int renderDevice = 0;
		if (mantleDeviceIndex)
		{
			renderDevice = *((unsigned int*)mantleDeviceIndex);
		}

		gRenderPass[renderDevice]->remove(mPrivate->mDrawable[renderDevice]);
	}

	void MantleSurface::render(void *context)
	{
		unsigned int renderDevice = ((OriginIGO::MantleHook::DevContext*) context)->deviceIndex;
		CommandBuffer *cmdBuffer = ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer;
		tRenderTarget *rt = ((OriginIGO::MantleHook::DevContext*) context)->rt;

        IGO_UNUSED(rt)
		IGO_ASSERT(rt);
		IGO_ASSERT(cmdBuffer);
		

		if (mPrivate)
		{
			float X;
			float Y;

			IGO_ASSERT(rt->mMemory != GR_NULL_HANDLE);

			//Get coordinates
			X = mWindow->x() - (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth) / 2.0f;
			Y = -mWindow->y() + (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight) / 2.0f;

			float matWorldAndColor[20] = { (float)mWindow->width(), 0, 0, X,
				0, (float)mWindow->height(), 0, Y,
				0, 0, 1, 0,
				0, 0, 0, 1,
                mWindow->customValue() / 10000.0f, 1, 1, mAlpha / 255.0f };

			if (mBackupDevice == -1)	// no valid texture data yet
				IGO_ASSERT(0);

			// update texture data between GPUs
			{
				EA::Thread::AutoFutex lock(mMemoryBarrier);
				if ((/*mBackupDevice != renderDevice || */mDirtyTexture[renderDevice]) && ((mBackupDevice != -1 /* valid ?*/) && mPrivate->mTexture[mBackupDevice] /* alive ? */))
					update(context, static_cast<uint8_t>(mAlpha), mPrivate->mTexture[mBackupDevice]->width(), mPrivate->mTexture[mBackupDevice]->height(), (const char*)mBackupTexture, mPrivate->mTexture[mBackupDevice]->width() * mPrivate->mTexture[mBackupDevice]->height() * 4, IGOIPC::WINDOW_UPDATE_MGPU);
			}


			mPrivate->mPerPassConstants[renderDevice]->Prepare(cmdBuffer, GR_MEMORY_STATE_DATA_TRANSFER);
			_grCmdUpdateMemory(cmdBuffer->cmdBuffer(), mPrivate->mPerPassConstants[renderDevice]->memory(), mPrivate->mPerPassConstants[renderDevice]->offset(), 20 * sizeof(float), reinterpret_cast<GR_UINT32*>(&matWorldAndColor[0]));
			mPrivate->mPerPassConstants[renderDevice]->Prepare(cmdBuffer, GR_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY);

			// add the actual draw commands
			gRenderPass[renderDevice]->add(mPrivate->mDrawable[renderDevice]);
	    }
    }

	void MantleSurface::update(void *context, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
	{
		bool updateSize = false;
		unsigned int renderDevice = ((OriginIGO::MantleHook::DevContext*) context)->deviceIndex;
		CommandBuffer *cmdBuffer = ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer;

		// create our surface, if it does not exist yet
		if (mPrivate->mPipeline[renderDevice] == NULL)
		{
			initSurface(context);
		}

		if (((flags&IGOIPC::WINDOW_UPDATE_WIDTH) || (flags&IGOIPC::WINDOW_UPDATE_MGPU)) && (mWidth[renderDevice] != width))
		{
			mWidth[renderDevice] = width;
			updateSize = true;
		}

		if (((flags&IGOIPC::WINDOW_UPDATE_HEIGHT) || (flags&IGOIPC::WINDOW_UPDATE_MGPU)) && (mHeight[renderDevice] != height))
		{
			mHeight[renderDevice] = height;
			updateSize = true;
		}

		if (flags&IGOIPC::WINDOW_UPDATE_ALPHA)
		{
			mAlpha = alpha;
		}

		// fix for crash bug: https://developer.origin.com/support/browse/EBIBUGS-21939 where sometimes the size is too small for the width and height
		if ((data && (size != (width * height * 4))) || data == NULL)
		{
			size = 0;
		}

		// create a backup in CPU memory...
		if (!(flags&IGOIPC::WINDOW_UPDATE_MGPU) && size)
		{
			EA::Thread::AutoFutex lock(mMemoryBarrier);
			DELETE_ARRAY_IF(mBackupTexture);
			mBackupDevice = renderDevice;
			mBackupTexture = new unsigned char[mWidth[mBackupDevice] * mHeight[mBackupDevice] * 4];
			memcpy(mBackupTexture, data, mWidth[mBackupDevice] * mHeight[mBackupDevice] * 4);

			IGO_ASSERT(mWidth[renderDevice] == width);
			IGO_ASSERT(mHeight[renderDevice] == height);
		}



		{	
			// mark texture as dirty on all other GPU's and create a local copy
			if (!(flags&IGOIPC::WINDOW_UPDATE_MGPU))
			{
				for (int i = 0; i < GR_MAX_PHYSICAL_GPUS; i++)
					mDirtyTexture[i] = true;
			}


			if (mPrivate->mTexture[renderDevice] == NULL || updateSize)
			{
				IGO_ASSERT(mWidth[renderDevice] == width);
				IGO_ASSERT(mHeight[renderDevice] == height);
												
				if (size)
				{
                    DELETE_MANTLE_IF(mPrivate->mTexture[renderDevice], cmdBuffer->cmdBuffer());

					mPrivate->mTexture[renderDevice] = new Texture(renderDevice);
					mPrivate->mTexture[renderDevice]->createTextureFromMemARGB8(gTextureHeap[renderDevice], cmdBuffer, (const unsigned char*)data, mWidth[renderDevice], mHeight[renderDevice]);
					GR_IMAGE_VIEW_ATTACH_INFO imageView = mPrivate->mTexture[renderDevice]->imageView();
					mPrivate->mDescriptorSet[renderDevice]->BindImages(PIXEL_SHADER, DXST_TEXTURE, 0, 1, &mPrivate->mTexture[renderDevice]->memory(), &imageView);
					mPrivate->mDrawable[renderDevice]->setDescriptorSet(mPrivate->mDescriptorSet[renderDevice]);
					mDirtyTexture[renderDevice] = false;

				}
				else
					IGO_ASSERT(0);
			}
			else
			{
				if ((flags&IGOIPC::WINDOW_UPDATE_BITS) || (flags&IGOIPC::WINDOW_UPDATE_MGPU))
				{
					IGO_ASSERT(mWidth[renderDevice] == width);
					IGO_ASSERT(mHeight[renderDevice] == height);
							
					if (size)
					{
						IGO_ASSERT(mPrivate->mTexture[renderDevice]);
						IGO_ASSERT(mBackupDevice >= 0);

						if ((flags&IGOIPC::WINDOW_UPDATE_MGPU) && mPrivate->mTexture[renderDevice])
						{
							IGO_ASSERT(mPrivate->mTexture[renderDevice]->width() == static_cast<GR_UINT32>(mWidth[mBackupDevice]));
							IGO_ASSERT(mPrivate->mTexture[renderDevice]->height() == static_cast<GR_UINT32>(mHeight[mBackupDevice]));
							IGO_ASSERT(mPrivate->mTexture[renderDevice]->width() == static_cast<GR_UINT32>(width));
							IGO_ASSERT(mPrivate->mTexture[renderDevice]->height() == static_cast<GR_UINT32>(height));
							IGO_ASSERT(mWidth[mBackupDevice] == width);
							IGO_ASSERT(mHeight[mBackupDevice] == height);
						}

						if (!(flags&IGOIPC::WINDOW_UPDATE_MGPU) && mPrivate->mTexture[renderDevice])
						{
							IGO_ASSERT(mPrivate->mTexture[renderDevice]->width() == static_cast<GR_UINT32>(width));
							IGO_ASSERT(mPrivate->mTexture[renderDevice]->height() == static_cast<GR_UINT32>(height));
						}

						mPrivate->mTexture[renderDevice]->updateTextureFromMemARGB8((const unsigned char*)data);

						mDirtyTexture[renderDevice] = false;
					}
				}
			}
			
		}
    }
}
#endif // ORIGIN_PC
