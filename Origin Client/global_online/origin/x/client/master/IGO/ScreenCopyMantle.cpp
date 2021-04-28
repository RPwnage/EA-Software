///////////////////////////////////////////////////////////////////////////////
// ScreenCopyMantle.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2014 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ScreenCopyMantle.h"
#include "IGO.h"
#include "resource.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "twitchmanager.h"
#include "MantleShader.h"
#include "IGOLogger.h"
#include "IGOApplication.h"
#include "MantleHook.h"

namespace OriginIGO{
	EXTERN_NEXT_FUNC(GR_VOID, grCmdPrepareImagesHook, (GR_CMD_BUFFER cmdBuffer, GR_UINT transitionCount, const GR_IMAGE_STATE_TRANSITION* pStateTransitions));
	EXTERN_NEXT_FUNC(GR_VOID, grCmdCopyImageHook, (GR_CMD_BUFFER cmdBuffer, GR_IMAGE srcImage, GR_IMAGE destImage, GR_UINT regionCount, const GR_IMAGE_COPY* pRegions));
};

namespace OriginIGO {

    extern HMODULE gInstDLL;
	extern GR_UINT32 gCPUReadHeap[GR_MAX_PHYSICAL_GPUS];
	extern GR_UINT32 gGPUOnlyHeap[GR_MAX_PHYSICAL_GPUS];

	struct QuadVertex
	{
		GR_FLOAT x, y, z, w;		//!< The transformed position for the vertex.
		GR_FLOAT tu1, tv1;			//!< texture coordinates
	};
	
	
	ScreenCopyMantle::ScreenCopyMantle() :
	    mWidth(0),
	    mHeight(0),
	    mSourceWidth(0),
	    mSourceHeight(0),
        mFPS(0),
		mHardwareAccelerated(TwitchManager::TTV_IsOriginEncoder())
    {

        //MessageBox(NULL, L"Twitch ScreenCopyMantle", L"breakpoint", MB_OK);
		for (int i = 0; i < GR_MAX_PHYSICAL_GPUS; i++)
		{
			mCurrentOffscreenBuffer[i] = 0;
			mPrevOffscreenBuffer[i] = 0;
			mCapturedBufferFilled[i] = false;
				
			mDescriptorSet[i] = NULL;
			mSampler[i] = NULL;
			mMesh[i] = NULL;
			mPipeline[i] = NULL;
			mNeverChangeConstants[i] = NULL;
			mPerPassConstants[i] = NULL;

			mRenderPass2[i] = NULL;
			mRenderState[i] = NULL;
			mDrawable[i] = NULL;
			mShaderPs[i] = NULL;
			mShaderVs[i] = NULL;

			mRenderTarget[i] = NULL;
			for (int i2 = 0; i2 < NUM_OFFSCREEN_BUFFERS; i2++)
			{
				mRenderPass[i][i2] = NULL;
				mRenderTargetScaled[i][i2] = NULL;
				mBackBufferCopy[i][i2] = NULL;
				mBackBufferOffscreenCPU[i][i2] = NULL;
			}
		}
    }

    ScreenCopyMantle::~ScreenCopyMantle()
    {
    }

    #pragma region IScreenCopy Implementation


	bool ScreenCopyMantle::CreateSurfaces(void *context)
    {
		CommandBuffer *cmdBuffer = ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer;
		tRenderTarget *renderTarget = ((OriginIGO::MantleHook::DevContext*) context)->rt;
		int renderDevice = ((OriginIGO::MantleHook::DevContext*) context)->deviceIndex;
#ifdef	DEBUG_MANTLE_RENDERING
		GR_IMAGE_STATE oldState = ((OriginIGO::MantleHook::DevContext*) context)->oldState;
		GR_IMAGE_STATE newState = ((OriginIGO::MantleHook::DevContext*) context)->newState;
#endif
		
		ReleaseSurfaces(cmdBuffer->cmdBuffer(), renderDevice);

		{

			{
				GR_RESULT result = GR_SUCCESS;
				GR_FORMAT format = { GR_CH_FMT_R8G8B8A8, GR_NUM_FMT_UNORM };

				for (int i = 0; i < NUM_OFFSCREEN_BUFFERS; i++)
				{
                    IGO_ASSERT(mSourceHeight == renderTarget->mImageCreateInfo.extent.height);
                    IGO_ASSERT(mSourceWidth == renderTarget->mImageCreateInfo.extent.width);
                    
                    mBackBufferCopy[renderDevice][i] = new Texture(renderDevice);
					GR_RESULT success = mBackBufferCopy[renderDevice][i]->init(gGPUOnlyHeap[renderDevice], renderTarget->mImageCreateInfo.extent.width, renderTarget->mImageCreateInfo.extent.height, renderTarget->mImageCreateInfo.format, GR_IMAGE_USAGE_SHADER_ACCESS_READ, 1, false/*no CPU access*/);
                    if (success != GR_SUCCESS)
                    {
                        IGOLogWarn("Unable to initialize backbuffer copy texture (%d/%d) - error %d", i, NUM_OFFSCREEN_BUFFERS, success);
                        result = success;
                    }

					mBackBufferOffscreenCPU[renderDevice][i] = new Texture(renderDevice);
					success = mBackBufferOffscreenCPU[renderDevice][i]->init(gCPUReadHeap[renderDevice], mWidth, mHeight, format, GR_IMAGE_USAGE_COLOR_TARGET, 1, true/*CPU access*/);
                    if (success != GR_SUCCESS)
                    {
                        IGOLogWarn("Unable to initialize backbuffer offscren texture (%d/%d) - error %d", i, NUM_OFFSCREEN_BUFFERS, success);
                        result = success;
                    }

					mRenderTargetScaled[renderDevice][i] = new RenderTarget(mWidth, mHeight, format, GR_IMAGE_USAGE_COLOR_TARGET, renderDevice);
				
                    // make it usable for rendering
                    mRenderTargetScaled[renderDevice][i]->prepare(cmdBuffer, GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL);

                }

#ifdef	DEBUG_MANTLE_RENDERING
				mRenderTarget[renderDevice] = new RenderTarget(oldState, newState, renderTarget->mViewCreateInfo.image, renderTarget->mMemory, renderTarget->mView, renderTarget->mImageCreateInfo.format, renderDevice);
#endif


				// setup the Scene
				if (result == GR_SUCCESS)
				{
					// create renderstate
					mRenderState[renderDevice] = new RenderState();
					mRenderState[renderDevice]->init(false, (GR_FLOAT)mWidth, (GR_FLOAT)mHeight, renderDevice);

					for (int i = 0; i < NUM_OFFSCREEN_BUFFERS; i++)
					{	// create RenderPass to offscreen
						mRenderPass[renderDevice][i] = new RenderPass();
						mRenderPass[renderDevice][i]->init(cmdBuffer, mRenderState[renderDevice], 1, &mRenderTargetScaled[renderDevice][i], NULL, renderDevice);

						// make it usable for the pixel shader
						mBackBufferCopy[renderDevice][i]->prepare(cmdBuffer, GR_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY);
					}
#ifdef	DEBUG_MANTLE_RENDERING
					// create RenderPass to screen (for debugging)
					mRenderPass2[renderDevice] = new RenderPass();
					mRenderPass2[renderDevice]->init(cmdBuffer, mRenderState[renderDevice], 1, &mRenderTarget[renderDevice], NULL, renderDevice);
#endif
					// Load Compiled Vertex & Pixel Shader from DLL resource
					HRSRC hRes = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOVSQUADFX), RT_RCDATA);
					IGO_ASSERT(hRes != NULL);
					const char * pIGOVS;
					DWORD dwIGOVSSize = 0;
					HGLOBAL hIGOVS = LoadResource(gInstDLL, hRes);
					pIGOVS = (const char *)LockResource(hIGOVS);
					dwIGOVSSize = SizeofResource(gInstDLL, hRes);

					// if any of these fail, the resouce is not properly compiled into the dll
					IGO_ASSERT(hIGOVS != NULL);
					IGO_ASSERT(pIGOVS != NULL);
					IGO_ASSERT(dwIGOVSSize > 0);

					// setup YUV shader if needed
					if (mHardwareAccelerated)
					{
						hRes = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOPSQUADFX_YUV), RT_RCDATA);
					}
					else
					{
						hRes = FindResource(gInstDLL, MAKEINTRESOURCE(IDR_IGOPSQUADFX), RT_RCDATA);
					}
					IGO_ASSERT(hRes != NULL);
					const char * pIGOPS;
					DWORD dwIGOPSSize = 0;
					HGLOBAL hIGOPS = LoadResource(gInstDLL, hRes);
					pIGOPS = (const char *)LockResource(hIGOPS);
					dwIGOPSSize = SizeofResource(gInstDLL, hRes);

					// if any of these fail, the resouce is not properly compiled into the dll
					IGO_ASSERT(hIGOPS != NULL);
					IGO_ASSERT(pIGOPS != NULL);
					IGO_ASSERT(dwIGOPSSize > 0);

					ILC_VS_INPUT_LAYOUT emptyInputLayout = { 0 };

					mShaderVs[renderDevice] = new Shader();
					mShaderVs[renderDevice]->initFromBinaryHLSL(pIGOVS, dwIGOVSSize, gVbLayout, NULL, renderDevice);

					mShaderPs[renderDevice] = new Shader();
					mShaderPs[renderDevice]->initFromBinaryHLSL(pIGOPS, dwIGOPSSize, emptyInputLayout, NULL, renderDevice);

					// create pipeline
					GR_GRAPHICS_PIPELINE_CREATE_INFO pi = gPipelineInfo;
					pi.cbState.target->blendEnable = GR_FALSE;
					pi.cbState.target->format = renderTarget->mImageCreateInfo.format;

					mPipeline[renderDevice] = new Pipeline();
					result = mPipeline[renderDevice]->init(pi, mShaderVs[renderDevice], NULL, NULL, NULL, mShaderPs[renderDevice], renderDevice);

					mSampler[renderDevice] = new Sampler();
					mSampler[renderDevice]->init(renderDevice, false);

					// Create a screen quad for all render to texture operations

					// calculate aspect ratio and scaling values - game screen vs video resolution
					int gameHeight = (int)(((float)((float)mSourceHeight / (float)mSourceWidth))*mWidth);
					int gameWidth = mWidth;

					if (gameHeight > mHeight)
					{
						float scale = (((float)mHeight) / ((float)gameHeight));
						gameHeight = mHeight;
						gameWidth = (int)(gameWidth * scale);
					}

					if (gameWidth > mWidth)
					{
						float scale = (((float)mWidth) / ((float)gameWidth));
						gameWidth = mWidth;
						gameHeight = (int)(gameHeight * scale);
					}


					float scaledRect_left;
					float scaledRect_right;
					float scaledRect_top;
					float scaledRect_bottom;

					scaledRect_left = ((mWidth - gameWidth)*0.5f);
					scaledRect_right = (gameWidth + (mWidth - gameWidth)*0.5f);
					scaledRect_top = ((mHeight - gameHeight)*0.5f);
					scaledRect_bottom = (gameHeight + (mHeight - gameHeight)*0.5f);

					// translate
					scaledRect_left -= mWidth*0.5f;
					scaledRect_right -= mWidth*0.5f;
					scaledRect_top -= mHeight*0.5f;
					scaledRect_bottom -= mHeight*0.5f;

					// scale
					scaledRect_left *= 2.0f / mWidth;
					scaledRect_right *= 2.0f / mWidth;
					scaledRect_top *= 2.0f / mHeight;
					scaledRect_bottom *= 2.0f / mHeight;

					QuadVertex vtx[] = {
						{ scaledRect_left, scaledRect_bottom, 0.5f, 1.0f, 0.0f, 0.0f },
						{ scaledRect_right, scaledRect_bottom, 0.5f, 1.0f, 1.0f, 0.0f },
						{ scaledRect_left, scaledRect_top, 0.5f, 1.0f, 0.0f, 1.0f },
						{ scaledRect_right, scaledRect_top, 0.5f, 1.0f, 1.0f, 1.0f }
					};

					mMesh[renderDevice] = new Mesh();
					mMesh[renderDevice]->init(cmdBuffer, sizeof(vtx) / sizeof(QuadVertex), sizeof(QuadVertex), vtx, 0, 0, NULL, renderDevice);

					mDescriptorSet[renderDevice] = mPipeline[renderDevice]->createDescriptorSet();

					mDrawable[renderDevice] = new Drawable();
					mDrawable[renderDevice]->init(cmdBuffer, mMesh[renderDevice], mPipeline[renderDevice]);

					for (int i = 0; i < NUM_OFFSCREEN_BUFFERS; i++)
					{
						GR_MEMORY_VIEW_ATTACH_INFO memView = mMesh[renderDevice]->vbView();
						mDescriptorSet[renderDevice]->BindMemory(VERTEX_SHADER, DXST_VERTEXBUFFER, 0, 1, &memView);
						GR_IMAGE_VIEW_ATTACH_INFO imageView = mBackBufferCopy[renderDevice][i]->imageView();
						mDescriptorSet[renderDevice]->BindImages(PIXEL_SHADER, DXST_TEXTURE, 0, 1, &mBackBufferCopy[renderDevice][i]->memory(), &imageView);
						mDescriptorSet[renderDevice]->BindSamplers(PIXEL_SHADER, DXST_SAMPLER, 0, 1, &mSampler[renderDevice]->sampler());

						mDrawable[renderDevice]->setDescriptorSet(mDescriptorSet[renderDevice]);

						mRenderPass[renderDevice][i]->add(mDrawable[renderDevice]);
					}
#ifdef DEBUG_MANTLE_RENDERING				
					mRenderPass2[renderDevice]->add(mDrawable[renderDevice]);
#endif
				}

				GR_UINT32 out_memRefCount;
				GR_MEMORY_REF *out_pMemRefs;

				// prepare the render pass command buffer
                for (int i = 0; i < NUM_OFFSCREEN_BUFFERS; i++)
                {
                    if (mHardwareAccelerated)
                    {
                        float ClearColor_Black[4] = { 0.0625f, 0.5f, 0.0f, 0.5f }; // YUxV
                        mRenderPass[renderDevice][i]->setClearColor(ClearColor_Black);
                    }
                    else
                    {
                        float ClearColor_Black[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // RGBA
                        mRenderPass[renderDevice][i]->setClearColor(ClearColor_Black);
                    }

                    mRenderPass[renderDevice][i]->bakeCmdBuffer(cmdBuffer, GR_IMAGE_STATE_UNINITIALIZED_TARGET, GR_IMAGE_STATE_UNINITIALIZED_TARGET, &out_pMemRefs, &out_memRefCount, true);
                }
#ifdef DEBUG_MANTLE_RENDERING
				mRenderPass2[renderDevice]->bakeCmdBuffer(cmdBuffer, GR_IMAGE_STATE_UNINITIALIZED_TARGET, GR_IMAGE_STATE_UNINITIALIZED_TARGET, &out_pMemRefs, &out_memRefCount, true);
#endif
			}
		}

        return true;
    }


	void ScreenCopyMantle::ReleaseSurfaces(GR_CMD_BUFFER cmd, int deviceIndex)
    {
        if (TwitchManager::IsTTVDLLReady())
        {
            TwitchManager::TTV_PauseVideo();
        }


		for (int i = (deviceIndex == -1 ? 0 : deviceIndex); i <= (deviceIndex == -1 ? (GR_MAX_PHYSICAL_GPUS-1) : deviceIndex); i++)
		{
			mCurrentOffscreenBuffer[i] = 0;
			mPrevOffscreenBuffer[i] = 0;
			mCapturedBufferFilled[i] = false;

			DELETE_MANTLE_IF(mDescriptorSet[i], cmd);
			DELETE_MANTLE_IF(mSampler[i], cmd);
			DELETE_MANTLE_IF(mMesh[i], cmd);
			DELETE_MANTLE_IF(mPipeline[i], cmd);
			DELETE_MANTLE_IF(mNeverChangeConstants[i], cmd);
			DELETE_MANTLE_IF(mPerPassConstants[i], cmd);

			DELETE_MANTLE_IF(mRenderState[i], cmd);
			DELETE_MANTLE_IF(mRenderPass2[i], cmd);
			DELETE_MANTLE_IF(mDrawable[i], cmd);
			DELETE_MANTLE_IF(mShaderPs[i], cmd);
			DELETE_MANTLE_IF(mShaderVs[i], cmd);

			DELETE_MANTLE_IF(mRenderTarget[i], cmd);
			for (int i2 = 0; i2 < NUM_OFFSCREEN_BUFFERS; i2++)
			{
				DELETE_MANTLE_IF(mRenderTargetScaled[i][i2], cmd);
				DELETE_MANTLE_IF(mBackBufferCopy[i][i2], cmd);
				DELETE_MANTLE_IF(mBackBufferOffscreenCPU[i][i2], cmd);
				DELETE_MANTLE_IF(mRenderPass[i][i2], cmd);
			}
		}
    }


	bool ScreenCopyMantle::Create(void *context, void *, int, int )
    {
        if (context)
        {
            tRenderTarget *renderTarget = ((OriginIGO::MantleHook::DevContext*) context)->rt;

            if (renderTarget)
            {
                mSourceWidth = renderTarget->mImageCreateInfo.extent.width;
                mSourceHeight = renderTarget->mImageCreateInfo.extent.height;

                return TTV_SUCCEEDED(TwitchManager::InitializeForStreaming(mSourceWidth, mSourceHeight, mWidth, mHeight, mFPS));
            }
        }

        return false;
    }

    int ScreenCopyMantle::GetObjectCount()
    {
        int objects = 0;
			
        return objects;
    }


    bool ScreenCopyMantle::Reset(void *deviceQueue)
    {
        return true;
    }
    
    bool ScreenCopyMantle::Lost()
    {
	    return true;
    }

    bool ScreenCopyMantle::Destroy(void *context)
    {
		CommandBuffer *cmdBuffer = context != NULL ? ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer : NULL;
		ReleaseSurfaces(cmdBuffer ? cmdBuffer->cmdBuffer() : NULL);
	    return true;
    }

	bool ScreenCopyMantle::Render(void *context, void *)
	{
		IGO_ASSERT(context);

		unsigned int renderDevice = ((OriginIGO::MantleHook::DevContext*) context)->deviceIndex;
		CommandBuffer *cmdBuffer = ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer;
		OriginIGO::tRenderTarget *renderTarget = ((OriginIGO::MantleHook::DevContext*) context)->rt;

		GR_IMAGE_STATE oldState = ((OriginIGO::MantleHook::DevContext*) context)->oldState;
		GR_IMAGE_STATE newState = ((OriginIGO::MantleHook::DevContext*) context)->newState;

		IGO_ASSERT(cmdBuffer);
		IGO_ASSERT(renderTarget);

		if (renderTarget == GR_NULL_HANDLE || renderTarget->mMemory == GR_NULL_HANDLE || cmdBuffer == NULL)
			return false;
		
		// validate our resources
		if (mRenderState[renderDevice] == GR_NULL_HANDLE)
			if (!CreateSurfaces(context))
				return false;


		static LARGE_INTEGER last_update = { 0, 0 };
		static LARGE_INTEGER freq = { 0, 0 };
		GR_RESULT result = GR_SUCCESS;

		if (freq.QuadPart == 0)
		{
			QueryPerformanceFrequency(&freq);
		}

		LARGE_INTEGER current_update;
		QueryPerformanceCounter(&current_update);

		// Feed the encoder with 1.25x the frame rate to prevent stuttering!
		if (TwitchManager::GetUsedPixelBufferCount() > 16 /*too many unencoded frames 1920*1080 -> 16 frames ~ 125MB*/ || ((current_update.QuadPart - last_update.QuadPart) * (mFPS*1.25f)) / freq.QuadPart == 0)
			return true;
		
		last_update = current_update;
        
		mCurrentOffscreenBuffer[renderDevice]++;
		if (mCurrentOffscreenBuffer[renderDevice] >= NUM_OFFSCREEN_BUFFERS)
			mCurrentOffscreenBuffer[renderDevice] = 0;

		mPrevOffscreenBuffer[renderDevice]++;
		// we reached the last frame in our buffer, now start the actual data transfer into the offscreen buffer
		// cur.  osb   0 1 2 0 1 2 0 1 2 ...
		// prev. osb   - - 0 1 2 0 1 ...
		if (mCurrentOffscreenBuffer[renderDevice] == (NUM_OFFSCREEN_BUFFERS - 1))
		{
			mPrevOffscreenBuffer[renderDevice] = 0;
			mCapturedBufferFilled[renderDevice] = true;
		}
        
		//TODO: add MSAA resolve support if ever needed

		// copy render target to texture ....		
		result = GR_SUCCESS;
		if (result == GR_SUCCESS)
		{
			
			{
				GR_IMAGE_STATE_TRANSITION transition =
				{
					renderTarget->mViewCreateInfo.image,
					oldState,
					//GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
					GR_IMAGE_STATE_DATA_TRANSFER,
					GR_IMAGE_ASPECT_COLOR,
					0,
					1,
					0,
					1
				};

				grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);
				

				GR_MEMORY_REF memRef;
				memRef.mem = renderTarget->mMemory;
				memRef.flags = 0;
                OriginIGO::IGOLogDebug("renderTarget->mMemory  %p", memRef.mem);
                cmdBuffer->addMemReferences(1, &memRef);
			}


			{
			GR_IMAGE_STATE_TRANSITION transition =
			{
				mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(),
				GR_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY,
				GR_IMAGE_STATE_DATA_TRANSFER,
				GR_IMAGE_ASPECT_COLOR,
				0,
				1,
				0,
				1
			};

			grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);

			GR_MEMORY_REF memRef;
			memRef.mem = mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory();
			memRef.flags = 0;
            OriginIGO::IGOLogDebug("mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory() %p", memRef.mem);
            cmdBuffer->addMemReferences(1, &memRef);
		}


			{
				GR_IMAGE_COPY region = { 0 };

                IGO_ASSERT(mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->height() == (GR_UINT32)renderTarget->mImageCreateInfo.extent.height);
                IGO_ASSERT(mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->width() == (GR_UINT32)renderTarget->mImageCreateInfo.extent.width);
                IGO_ASSERT(mSourceHeight == renderTarget->mImageCreateInfo.extent.height);
                IGO_ASSERT(mSourceWidth == renderTarget->mImageCreateInfo.extent.width);
                IGO_ASSERT(mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->format().channelFormat == (GR_UINT32)renderTarget->mImageCreateInfo.format.channelFormat);
                IGO_ASSERT(mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->format().numericFormat == (GR_UINT32)renderTarget->mImageCreateInfo.format.numericFormat);

				region.extent.height = renderTarget->mImageCreateInfo.extent.height,
				region.extent.width = renderTarget->mImageCreateInfo.extent.width;
				region.extent.depth = 1;
				region.srcSubresource.arraySlice = 0;
				region.srcSubresource.aspect = GR_IMAGE_ASPECT_COLOR;
				region.srcSubresource.mipLevel = 0;
				region.destSubresource.arraySlice = 0;
				region.destSubresource.aspect = GR_IMAGE_ASPECT_COLOR;
				region.destSubresource.mipLevel = 0;

				grCmdCopyImageHookNext(cmdBuffer->cmdBuffer(), renderTarget->mViewCreateInfo.image, mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(), 1, &region);
			}

			// reset txt state
			{
				{
					GR_IMAGE_STATE_TRANSITION transition =
					{
						mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(),
						GR_IMAGE_STATE_DATA_TRANSFER,
						GR_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY,
						GR_IMAGE_ASPECT_COLOR,
						0,
						1,
						0,
						1
					};

					grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);

					GR_MEMORY_REF memRef;
					memRef.mem = mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory();
					memRef.flags = 0;
                    OriginIGO::IGOLogDebug("mBackBufferCopy[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory() %p", memRef.mem);
                    cmdBuffer->addMemReferences(1, &memRef);
				}
			}

			// reset rt state
			{
				GR_IMAGE_STATE_TRANSITION transition =
				{
					renderTarget->mViewCreateInfo.image,
					GR_IMAGE_STATE_DATA_TRANSFER,
					newState,//GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
					GR_IMAGE_ASPECT_COLOR,
					0,
					1,
					0,
					1
				};

				grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);

				GR_MEMORY_REF memRef;
				memRef.mem = renderTarget->mMemory;
				memRef.flags = 0;
                OriginIGO::IGOLogDebug("renderTarget->mMemory %p", memRef.mem);
                cmdBuffer->addMemReferences(1, &memRef);
			}

			GR_UINT32 out_memRefCount;
			GR_MEMORY_REF *out_pMemRefs;

            // scaling & YUV conversion
            mRenderPass[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->bakeCmdBuffer(cmdBuffer, GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL, GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL, &out_pMemRefs, &out_memRefCount, true);
            
            // copy scaled output to CPU accesssible surface
			if (result == GR_SUCCESS)
			{
				{
					GR_IMAGE_STATE_TRANSITION transition =
					{
						mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(),
                        GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,//GR_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY,
						GR_IMAGE_STATE_DATA_TRANSFER,
						GR_IMAGE_ASPECT_COLOR,
						0,
						1,
						0,
						1
					};

					grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);

					GR_MEMORY_REF memRef;
					memRef.mem = mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory();
					memRef.flags = 0;
                    OriginIGO::IGOLogDebug("mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory() %p", memRef.mem);
                    cmdBuffer->addMemReferences(1, &memRef);
				}


				{
				GR_IMAGE_STATE_TRANSITION transition =
				{
					mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(),
					GR_IMAGE_STATE_UNINITIALIZED_TARGET,
					GR_IMAGE_STATE_DATA_TRANSFER,
					GR_IMAGE_ASPECT_COLOR,
					0,
					1,
					0,
					1
				};

				grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);

				GR_MEMORY_REF memRef;
				memRef.mem = mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory();
				memRef.flags = 0;
                OriginIGO::IGOLogDebug("mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory() %p", memRef.mem);
                cmdBuffer->addMemReferences(1, &memRef);
			}



				{
					IGO_ASSERT(mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->height() == static_cast<GR_UINT32>(mHeight));
					IGO_ASSERT(mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->width() == static_cast<GR_UINT32>(mWidth));
					IGO_ASSERT(mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->format().channelFormat == mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->format().channelFormat);
					IGO_ASSERT(mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->format().numericFormat == mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->format().numericFormat);
					IGO_ASSERT(mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->height() == mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->height());
					IGO_ASSERT(mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->width() == mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->width());


					GR_IMAGE_COPY region = { 0 };
					region.extent.height = mHeight,
					region.extent.width = mWidth;
					region.extent.depth = 1;
					region.srcSubresource.arraySlice = 0;
					region.srcSubresource.aspect = GR_IMAGE_ASPECT_COLOR;
					region.srcSubresource.mipLevel = 0;
					region.destSubresource.arraySlice = 0;
					region.destSubresource.aspect = GR_IMAGE_ASPECT_COLOR;
					region.destSubresource.mipLevel = 0;

					grCmdCopyImageHookNext(cmdBuffer->cmdBuffer(), mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(), mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(), 1, &region);
				}

				// reset txt state
				{
					{
						GR_IMAGE_STATE_TRANSITION transition =
						{
							mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(),
							GR_IMAGE_STATE_DATA_TRANSFER,
							GR_IMAGE_STATE_UNINITIALIZED_TARGET,
							GR_IMAGE_ASPECT_COLOR,
							0,
							1,
							0,
							1
						};

						grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);

						GR_MEMORY_REF memRef;
						memRef.mem = mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory();
						memRef.flags = 0;
                        OriginIGO::IGOLogDebug("mBackBufferOffscreenCPU[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory() %p", memRef.mem);
                        cmdBuffer->addMemReferences(1, &memRef);
					}
				}


                // reset rt state
				{
					GR_IMAGE_STATE_TRANSITION transition =
					{
						mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->image(),
                        GR_IMAGE_STATE_DATA_TRANSFER,
                        GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,//GR_IMAGE_STATE_UNINITIALIZED_TARGET,
						GR_IMAGE_ASPECT_COLOR,
						0,
						1,
						0,
						1
					};

					grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);

					GR_MEMORY_REF memRef;
					memRef.mem = mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory();
					memRef.flags = 0;
                    OriginIGO::IGOLogDebug("mRenderTargetScaled[renderDevice][mCurrentOffscreenBuffer[renderDevice]]->memory() %p", memRef.mem);
                    cmdBuffer->addMemReferences(1, &memRef);
				}

			}
			else
				IGO_ASSERT(0);
		}
		else
			IGO_ASSERT(0);

#ifdef	DEBUG_MANTLE_RENDERING

			
		mRenderTarget[renderDevice]->update(oldState, newState, renderTarget->mViewCreateInfo.image, renderTarget->mMemory, renderTarget->mView, renderTarget->mImageCreateInfo.format);

		GR_UINT32 out_memRefCount;
		GR_MEMORY_REF *out_pMemRefs;

		mRenderPass2[renderDevice]->bakeCmdBuffer(cmdBuffer, GR_IMAGE_STATE_UNINITIALIZED_TARGET, GR_IMAGE_STATE_UNINITIALIZED_TARGET, &out_pMemRefs, &out_memRefCount, true);
#endif

		
		if (mCapturedBufferFilled[renderDevice] && mBackBufferOffscreenCPU[renderDevice][mPrevOffscreenBuffer[renderDevice]] != NULL)
		{
			GR_SUBRESOURCE_LAYOUT srLayout = {};
			BYTE*    pData = reinterpret_cast<BYTE*>(mBackBufferOffscreenCPU[renderDevice][mPrevOffscreenBuffer[renderDevice]]->Map(0, 0, &srLayout));
			if (pData != NULL)
			{
				TwitchManager::tPixelBuffer *pixelBuffer = TwitchManager::GetUnusedPixelBuffer();
				if (pixelBuffer)
				{
					CopySurfaceData(pData, pixelBuffer->data, mWidth, mHeight, (uint32_t)srLayout.rowPitch);
					mBackBufferOffscreenCPU[renderDevice][mPrevOffscreenBuffer[renderDevice]]->Unmap();
					TwitchManager::TTV_SubmitVideoFrame(pixelBuffer->data, pixelBuffer);
					//TwitchManager::MarkUnusedPixelBuffer(pixelBuffer);
				}
				else
        			mBackBufferOffscreenCPU[renderDevice][mPrevOffscreenBuffer[renderDevice]]->Unmap();

                return true;
			}

		}
		else
		if (!mCapturedBufferFilled[renderDevice])
			return true;
	
		
		
		TwitchManager::SetBroadcastError(result, TwitchManager::ERROR_CATEGORY_VIDEO_SETTINGS);
		ReleaseSurfaces(cmdBuffer->cmdBuffer());

	    return false;
    }

    bool ScreenCopyMantle::CopyToMemory(void *deviceQueue, void *pDestination, int &size, int &width, int &heigth)
    {
	
	    return true;
    }

    bool ScreenCopyMantle::CopyFromMemory(void *deviceQueue, void *pSource, int width, int heigth)
    {
	    return false;
    }
#pragma endregion
}

