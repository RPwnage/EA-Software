///////////////////////////////////////////////////////////////////////////////
// MantleHook.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "MantleHook.h"

#if defined(ORIGIN_PC)

#include <windows.h>
#include "MantleShader.h"
#include "MantleObjects.h"

#include "HookAPI.h"
#include "IGOApplication.h"
#include "resource.h"

#include "EASTL/hash_map.h"
#include "IGOTrace.h"
#include "InputHook.h"
#include "IGOLogger.h"

#include "ScreenCopyMantle.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

#include "MantleFunctions.h"

#undef _MANTLE_DIAGNOSE_
#undef _ENABLE_DEVICE_VALIDATION_

#undef _SIMULATE_NON_CROSS_DISPLAY_SUPPORT_
#undef TEST_FOR_ENGINE_OR_GAME_SPECIFIC_MANTLE_HACKS

HWND getProcMainWindow(DWORD PID);

namespace OriginIGO {

	void purgeIGOMemory(GR_CMD_BUFFER cmd);

	unsigned int gLastFrostbiteSubmit = 0;
	GR_DEVICE gCurrentMantleDevice = NULL;
	tMantleRenderDeviceList gRenderDevices;

	EA::Thread::Futex gObjectMutex;
	tCmdBufferObjectMap gCmdObjects;
	EA::Thread::Futex gMemMutex;
	tCmdBufferMemMap gCmdMem;



	EA::Thread::RWMutex gRenderDevicesMutex;
	EA::Thread::RWMutex gTwitchObjectCreation;
	RenderPass* gRenderPass[GR_MAX_PHYSICAL_GPUS] = {};
	ConstantBuffer* gNeverChangeConstants[GR_MAX_PHYSICAL_GPUS] = {};
	RenderTarget* gRenderTarget[GR_MAX_PHYSICAL_GPUS] = {};
	RenderState* gRenderState[GR_MAX_PHYSICAL_GPUS] = {};
	GR_UINT32 gTextureHeap[GR_MAX_PHYSICAL_GPUS] = {};
	GR_UINT32 gCPUReadHeap[GR_MAX_PHYSICAL_GPUS] = {};
	GR_UINT32 gGPUOnlyHeap[GR_MAX_PHYSICAL_GPUS] = {};
	Shader* gShaderVs[GR_MAX_PHYSICAL_GPUS] = {};
    Shader*	gShaderPs[GR_MAX_PHYSICAL_GPUS] = {};
    Shader* gShaderBgVs[GR_MAX_PHYSICAL_GPUS] = {};
    Shader* gShaderBgPs[GR_MAX_PHYSICAL_GPUS] = {};


	EA::Thread::Futex MantleHook::mInstanceHookMutex;
	HANDLE MantleHook::mIGOMantleHookDestroyThreadHandle = NULL;

	bool MantleHook::isHooked = false;
    static bool gMultiGPURenderingEnabled = false;
	extern DWORD gPresentHookThreadId;
	extern volatile DWORD gPresentHookCalled;
	extern volatile bool gTestCooperativeLevelHook_or_ResetHook_Called;

	extern HINSTANCE gInstDLL;
	extern bool gInputHooked;
	extern volatile bool gQuitting;

	static void CreateScreenCopyMANTLE(void *context)
	{
		EA::Thread::AutoRWMutex gTwitchMutexInstance(gTwitchObjectCreation, EA::Thread::RWMutex::kLockTypeWrite);
		if (gScreenCopy == NULL)
		{
			gScreenCopy = new ScreenCopyMantle();
			gScreenCopy->Create(context);
		}
	}

	static void DestroyScreenCopyMANTLE(void *context)
	{
		EA::Thread::AutoRWMutex gTwitchMutexInstance(gTwitchObjectCreation, EA::Thread::RWMutex::kLockTypeWrite);
		if (gScreenCopy)
		{
			gScreenCopy->Destroy(context);
			delete gScreenCopy;
			gScreenCopy = NULL;
		}
	}


	static bool saveRenderState()
	{
		return true;
	}

	static void restoreRenderState()
	{

	}

	static void ReleaseMantleObjects(void *context)
	{
		MANTLE_OBJECTS_AUTO
		GR_CMD_BUFFER cmd = context != NULL ? ((OriginIGO::MantleHook::DevContext*) context)->cmdBuffer->cmdBuffer() : NULL;
		// cmd buffer & render pass have to be released first!
		DestroyScreenCopyMANTLE(cmd);

		for (unsigned int i = 0; i < gRenderDevices.size(); ++i)
		{
			IGOLogDebug("gRenderPass[%i] deleted", i);
			DELETE_MANTLE_IF(gRenderPass[i], cmd);
			DELETE_MANTLE_IF(gNeverChangeConstants[i], cmd);
			DELETE_MANTLE_IF(gRenderState[i], cmd);
			DELETE_MANTLE_IF(gRenderTarget[i], cmd);
		}
	}

	void Init()
	{
		IGOLogInfo("Initialize Mantle");

		if (!IGOApplication::instance())
			IGOApplication::createInstance(RendererMantle, "Mantle can have multiple renderer devices!", &ReleaseMantleObjects);

	}


	bool isCMDBufferOnUniversalQueue(GR_CMD_BUFFER cmdBuffer)
	{
		EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

		bool result = false;
		for (size_t i = 0; i < gRenderDevices.size(); i++)
		{
			tCmdBufferMap::iterator iter = gRenderDevices[i]->mCommandBuffers.find(cmdBuffer);
			if (iter != gRenderDevices[i]->mCommandBuffers.end())
			{
				if (iter->second.queueType == GR_QUEUE_UNIVERSAL)
				{
					return true;
				}
			}
		}

		return result;
	}

    DEFINE_HOOK_SAFE(GR_RESULT, grOpenSharedMemoryHook, (
        GR_DEVICE                  device,
        const GR_MEMORY_OPEN_INFO* pOpenInfo,
        GR_GPU_MEMORY*             pMem))
    

        GR_RESULT result = grOpenSharedMemoryHookNext(device,
            pOpenInfo,
            pMem);


        if (result == GR_SUCCESS)
        {
            EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

            int parentDevice = -1;
            int currentDevice = -1;
            tRenderTarget* rt = NULL;

            for (size_t i = 0; i < gRenderDevices.size() && parentDevice == -1; i++)
            {
                tMantleRenderTargetList::const_iterator rtiter;
                for (rtiter = gRenderDevices[i]->mRenderTargets.begin(); rtiter != gRenderDevices[i]->mRenderTargets.end() && parentDevice == -1; ++rtiter)
                {
                    if ((*rtiter)->mMemory == pOpenInfo->sharedMem)
                    {
                        parentDevice = static_cast<int>(i);
                        rt = (*rtiter);
                        break;
                    }
                }
            }

            IGOLogDebug("grOpenSharedMemoryHook (mem %p) gpu %i mem %p", pOpenInfo->sharedMem, parentDevice, *pMem);

            for (size_t i = 0; i < gRenderDevices.size(); i++)
            {
                if (gRenderDevices[i]->mDevice == device)
                {
                    currentDevice = static_cast<int>(i);
                    break;
                }
            }

            if (rt && currentDevice >= 0)
            {
                /*
                OriginIGO::tRenderTarget *renderTarget = new OriginIGO::tRenderTarget();

                renderTarget->mViewCreateInfo = rt->mViewCreateInfo;
                renderTarget->mImageCreateInfo = rt->mImageCreateInfo;
                renderTarget->mViewCreateInfo.image = *pImage;
                renderTarget->mMemory = *pMem;
                renderTarget->mPresent = true;

                gRenderDevices[i]->mRenderTargets.push_back(renderTarget);
                */
                IGOLogDebug("grOpenSharedMemoryHook gpu %i im %p (mem %p) -> gpu %i (mem %p)", parentDevice, rt->mViewCreateInfo.image, rt->mMemory, currentDevice, *pMem);
            }
        }


        return result;
    }

    

	DEFINE_HOOK_SAFE(GR_RESULT, grOpenPeerMemoryHook, (
		GR_DEVICE                       device,
		const GR_PEER_MEMORY_OPEN_INFO* pOpenInfo,
		GR_GPU_MEMORY*                  pMem))



		GR_RESULT result = grOpenPeerMemoryHookNext(device,
		pOpenInfo,
		pMem);

		if (result == GR_SUCCESS)
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

      		int parentDevice = -1;
    		int currentDevice = -1;
	    	tRenderTarget* rt = NULL;

			for (size_t i = 0; i < gRenderDevices.size() && parentDevice == -1; i++)
			{
				tMantleRenderTargetList::const_iterator rtiter;
				for (rtiter = gRenderDevices[i]->mRenderTargets.begin(); rtiter != gRenderDevices[i]->mRenderTargets.end() && parentDevice == -1; ++rtiter)
				{
					if ((*rtiter)->mMemory == pOpenInfo->originalMem)
					{
						parentDevice = static_cast<int>(i);
						rt = (*rtiter);
						break;
					}
				}
			}

			IGOLogDebug("grOpenPeerMemoryHook (mem %p) gpu %i mem %p", pOpenInfo->originalMem, parentDevice, *pMem);

			for (size_t i = 0; i < gRenderDevices.size(); i++)
			{
				if (gRenderDevices[i]->mDevice == device)
				{
					currentDevice = static_cast<int>(i);
					break;
				}
			}

			if (rt && currentDevice >= 0)
			{
				/*
				OriginIGO::tRenderTarget *renderTarget = new OriginIGO::tRenderTarget();

				renderTarget->mViewCreateInfo = rt->mViewCreateInfo;
				renderTarget->mImageCreateInfo = rt->mImageCreateInfo;
				renderTarget->mViewCreateInfo.image = *pImage;
				renderTarget->mMemory = *pMem;
				renderTarget->mPresent = true;

				gRenderDevices[i]->mRenderTargets.push_back(renderTarget);
				*/
				IGOLogDebug("grOpenPeerMemoryHook gpu %i im %p (mem %p) -> gpu %i (mem %p)", parentDevice, rt->mViewCreateInfo.image, rt->mMemory, currentDevice, *pMem);
			}
		}


		return result;
	}


	DEFINE_HOOK_SAFE(GR_RESULT, grOpenPeerImageHook, (
		GR_DEVICE                      device,
		const GR_PEER_IMAGE_OPEN_INFO* pOpenInfo,
		GR_IMAGE*                      pImage,
		GR_GPU_MEMORY*                 pMem))



		GR_RESULT result = grOpenPeerImageHookNext(device,
												pOpenInfo,
												pImage,
												pMem);


		if (result == GR_SUCCESS)
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

        	int parentDevice = -1;
        	int currentDevice = -1;
		    tRenderTarget* rt = NULL;

			for (size_t i = 0; i < gRenderDevices.size() && parentDevice == -1; i++)
			{
				tMantleRenderTargetList::const_iterator rtiter;
				for (rtiter = gRenderDevices[i]->mRenderTargets.begin(); rtiter != gRenderDevices[i]->mRenderTargets.end() && parentDevice == -1; ++rtiter)
				{
					if ((*rtiter)->mViewCreateInfo.image == pOpenInfo->originalImage)
					{
						parentDevice = static_cast<int>(i);
						rt = (*rtiter);
						break;
					}
				}
			}

			for (size_t i = 0; i < gRenderDevices.size(); i++)
			{
				if (gRenderDevices[i]->mDevice == device)
				{
					currentDevice = static_cast<int>(i);
					break;
				}
			}

			if (rt && currentDevice >= 0)
			{
				OriginIGO::tRenderTarget *renderTarget = new OriginIGO::tRenderTarget();

				renderTarget->mViewCreateInfo = rt->mViewCreateInfo;
				renderTarget->mImageCreateInfo = rt->mImageCreateInfo;
				renderTarget->mViewCreateInfo.image = *pImage;
				renderTarget->mMemory = *pMem;
				renderTarget->mPresent = true;
				renderTarget->mParentImage = pOpenInfo->originalImage;

				gRenderDevices[currentDevice]->mRenderTargets.push_back(renderTarget);

				IGOLogDebug("grOpenPeerImageHook added rendertarget %p from gpu %i im %p (mem %p) -> gpu %i im %p (mem %p)", renderTarget, parentDevice, rt->mViewCreateInfo.image, rt->mMemory, currentDevice, *pImage, *pMem);
			}
		}
		return result;
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grEndCommandBufferHook, (
		GR_CMD_BUFFER cmdBuffer))
		EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

		for (size_t i = 0; i < gRenderDevices.size(); i++)
		{
			if (gRenderDevices[i]->mCommandBuffers.find(cmdBuffer) != gRenderDevices[i]->mCommandBuffers.end())
			{
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered = true;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].started = false;
				IGOLogDebug("grEndCommandBufferHook %p %s used %i", cmdBuffer, gRenderDevices[i]->mCommandBuffers[cmdBuffer].queueType == GR_QUEUE_UNIVERSAL ? "(universal)" : "(non universal)", gRenderDevices[i]->mCommandBuffers[cmdBuffer].used);

                if (gRenderDevices[i]->mCommandBuffers[cmdBuffer].used)
                {
                    gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered = false;
                    return grEndCommandBufferHookNext(cmdBuffer);
                }
                else
				    return GR_SUCCESS;
			}
		}
	
		return grEndCommandBufferHookNext(cmdBuffer);
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grBeginCommandBufferHook, (
		GR_CMD_BUFFER cmdBuffer,
		GR_FLAGS      flags))

		EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

		for (size_t i = 0; i < gRenderDevices.size(); i++)
		{
			if (gRenderDevices[i]->mCommandBuffers.find(cmdBuffer) != gRenderDevices[i]->mCommandBuffers.end())
			{
				IGOLogDebug("grBeginCommandBufferHook %p %s flags %i used %i", cmdBuffer, gRenderDevices[i]->mCommandBuffers[cmdBuffer].queueType == GR_QUEUE_UNIVERSAL ? "(universal)" : "(non universal)", flags, gRenderDevices[i]->mCommandBuffers[cmdBuffer].used);

                if (gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered || gRenderDevices[i]->mCommandBuffers[cmdBuffer].started)
                {
					GR_RESULT result = grEndCommandBufferHookNext(cmdBuffer);
                    IGO_UNUSED(result)
					IGO_ASSERT(result == GR_SUCCESS);
				}

				// reset cmd buffer data
                if (gRenderDevices[i]->mCommandBuffers[cmdBuffer].cmd)
                    gRenderDevices[i]->mCommandBuffers[cmdBuffer].cmd->clearMemRefs(false);
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].memRefCount = 0;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].pMemRefs = 0;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered = false;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].used = false;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].started = true;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].flags = flags;
			}
		}

	return grBeginCommandBufferHookNext(cmdBuffer, flags);
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grResetCommandBufferHook, (
		GR_CMD_BUFFER cmdBuffer))

		//	IGOLogDebug("grResetCommandBufferHook cmdBuffer %p", cmdBuffer);

		EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

		for (size_t i = 0; i < gRenderDevices.size(); i++)
		{
			if (gRenderDevices[i]->mCommandBuffers.find(cmdBuffer) != gRenderDevices[i]->mCommandBuffers.end())
			{				
				IGOLogDebug("grResetCommandBufferHook %p %s used %i", cmdBuffer, gRenderDevices[i]->mCommandBuffers[cmdBuffer].queueType == GR_QUEUE_UNIVERSAL ? "(universal)" : "(non universal)", gRenderDevices[i]->mCommandBuffers[cmdBuffer].used);

				if (gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered || gRenderDevices[i]->mCommandBuffers[cmdBuffer].started)
				{
					IGOLogDebug("closing CMD buffer");
					GR_RESULT result = grEndCommandBufferHookNext(cmdBuffer);
                    IGO_UNUSED(result)
					IGO_ASSERT(result == GR_SUCCESS);
				}

				// reset cmd buffer data
                if (gRenderDevices[i]->mCommandBuffers[cmdBuffer].cmd)
                    gRenderDevices[i]->mCommandBuffers[cmdBuffer].cmd->clearMemRefs(false);
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].memRefCount = 0;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].pMemRefs = 0;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].used = false;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered = false;
				gRenderDevices[i]->mCommandBuffers[cmdBuffer].started = false;
			}
		}

	return grResetCommandBufferHookNext(cmdBuffer);

	}



	void doIGO(bool runTwitch, CommandBuffer *m_cmdBuffer, GR_UINT32 in_memRefCount, GR_MEMORY_REF* in_pMemRefs, GR_UINT32 *out_memRefCount, GR_MEMORY_REF**out_pMemRefs, tRenderTarget *rt, tRenderDevice *dev, int renderDeviceIndex, GR_IMAGE_STATE currentState = GR_IMAGE_STATE_UNINITIALIZED_TARGET, GR_IMAGE_STATE stateAfterRendering = GR_IMAGE_STATE_UNINITIALIZED_TARGET)
	{

		IGOLogDebug("doIGO %i----", renderDeviceIndex);

		GR_DEVICE renderDeviceHandle = dev->mDevice;
		gCurrentMantleDevice = renderDeviceHandle;
		gPresentHookCalled = GetTickCount();

		IGO_ASSERT(rt);
		IGO_ASSERT(m_cmdBuffer);
		IGO_ASSERT(dev);

		{
			SAFE_CALL_LOCK_AUTO
				if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererMantle))
				{
					MantleHook::CleanupLater();
					IGOLogDebug("Invalid Mantle device from SwapChain. Could be different renderer instead.");
					return;
				}
		}

		if (!gInputHooked && !gQuitting)
			InputHook::TryHookLater(&gInputHooked);

		if (IGOApplication::isPendingDeletion())
		{

			OriginIGO::MantleHook::DevContext context;
			context.deviceIndex = renderDeviceIndex;
			context.cmdBuffer = m_cmdBuffer;
			context.fullscreen = gWindowedMode == false;
			context.rt = rt;
			context.oldState = rt->mState[m_cmdBuffer->cmdBuffer()];
			context.newState = context.oldState;

			IGOApplication::deleteInstance(&context);
			//purgeIGOMemory((GR_CMD_BUFFER)-1);
			return;
		}

		if (!IGOApplication::instance())
		{
			// first time. we initialize the IGO
			{
				if (gCurrentMantleDevice)
					Init();
				else
					return;

				gPresentHookThreadId = GetCurrentThreadId();
				// hook windows message
				if (!gQuitting && gRenderingWindow)
				{
					InputHook::HookWindow(gRenderingWindow);
				}
			}
		}
		SAFE_CALL_LOCK_ENTER
		SAFE_CALL(IGOApplication::instance(), &IGOApplication::startTime);
		SAFE_CALL_LOCK_LEAVE

		{
			// hook windows message
			if (!gQuitting && gRenderingWindow)
			{
				InputHook::HookWindow(gRenderingWindow);
			}

			//SAFE_CALL_LOCK_ENTER
			//TODO: add SRGB support
			//SAFE_CALL(IGOApplication::instance(), &IGOApplication::setSRGB, ScreenCopyMantle::isSRGB_format(desc.BufferDesc.Format));
			//SAFE_CALL_LOCK_LEAVE
		}

		// check for resize & render

		static uint32_t w = 0;
		static uint32_t h = 0;
		static bool resizeEvent = false;

		if (w != (uint32_t)rt->mImageCreateInfo.extent.width || h != (uint32_t)rt->mImageCreateInfo.extent.height)
		{

			w = (uint32_t)rt->mImageCreateInfo.extent.width;
			h = (uint32_t)rt->mImageCreateInfo.extent.height;
			resizeEvent = true;

		}

		if (resizeEvent)
		{
			// resize event
			resizeEvent = false;

			OriginIGO::MantleHook::DevContext context;
			context.deviceIndex = renderDeviceIndex;
			context.cmdBuffer = m_cmdBuffer;
			context.fullscreen = gWindowedMode == false;
			context.rt = rt;
			context.oldState = rt->mState[m_cmdBuffer->cmdBuffer()];
			context.newState = context.oldState;

			IGOApplication::deleteInstance(&context);
			return;
		}

		IGOLogDebug("doIGO cmd %p device %i %i %i (im %p) bound %i present %i", m_cmdBuffer->cmdBuffer(), renderDeviceIndex, w, h, rt->mViewCreateInfo.image, rt->mBound[m_cmdBuffer->cmdBuffer()], rt->mPresent);

		if (saveRenderState() && w > 0 && h > 0)
		{
			// use cmd!!!
			MANTLE_OBJECTS_AUTO
			{
				{
					// sync object creation
						if (gRenderPass[renderDeviceIndex] == NULL)
						{
							IGOLogDebug("gRenderPass[%i] created", renderDeviceIndex);
							IGO_ASSERT(rt->mMemory != NULL);

							gRenderPass[renderDeviceIndex] = new RenderPass();
							gRenderTarget[renderDeviceIndex] = new RenderTarget(rt->mState[m_cmdBuffer->cmdBuffer()], rt->mStateAfterPresent, rt->mViewCreateInfo.image, rt->mMemory, rt->mView, rt->mImageCreateInfo.format, renderDeviceIndex);
							gRenderState[renderDeviceIndex] = new RenderState();
							gRenderState[renderDeviceIndex]->init(true, (GR_FLOAT)w, (GR_FLOAT)h, renderDeviceIndex);
						}

						gRenderPass[renderDeviceIndex]->init(m_cmdBuffer, gRenderState[renderDeviceIndex], 1, &gRenderTarget[renderDeviceIndex], NULL, renderDeviceIndex);
						gRenderTarget[renderDeviceIndex]->update(rt->mState[m_cmdBuffer->cmdBuffer()], rt->mStateAfterPresent, rt->mViewCreateInfo.image, rt->mMemory, rt->mView, rt->mImageCreateInfo.format);

					if (gNeverChangeConstants[renderDeviceIndex] == NULL)
					{
						gNeverChangeConstants[renderDeviceIndex] = new ConstantBuffer();
					}

					// viewprojection
					GR_FLOAT viewProjection[16] =
					{
						1, 0, 0, 0,
						0, 1, 0, 0,
						0, 0, 1, 0,
						0, 0, 0, 1
					};

					viewProjection[0] = 2.0f / (w == 0 ? 1 : w);
					viewProjection[5] = 2.0f / (h == 0 ? 1 : h);
					viewProjection[10] = -2.0f / (1 - 10);
					viewProjection[11] = 2.0f / (1 - 10);

					gNeverChangeConstants[renderDeviceIndex]->init(m_cmdBuffer, 4 * sizeof(float), 16 * sizeof(float), &viewProjection, renderDeviceIndex);
					gRenderPass[renderDeviceIndex]->setConstants(gNeverChangeConstants[renderDeviceIndex]);
				}

				SAFE_CALL_LOCK_ENTER
					SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, w, h);
				/*
				bool windowedMode = (pPresentInfos->presentMode == GR_WSI_WIN_PRESENT_MODE_BLT);
				static RECT rect = { 0 };
				if (gRenderingWindow)
					GetClientRect(gRenderingWindow, &rect);
				UINT width = rect.right - rect.left;
				UINT height = rect.bottom - rect.top;
				if (windowedMode != gWindowedMode || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenWidth) != width || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenHeight) != height)
				{
					gWindowedMode = windowedMode;
					SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
					SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, width, height);
				}
				*/
				SAFE_CALL_LOCK_LEAVE
					
				OriginIGO::MantleHook::DevContext context;
				context.deviceIndex = renderDeviceIndex;
				context.cmdBuffer = m_cmdBuffer;
				context.fullscreen = gWindowedMode == false;
				context.rt = rt;
				context.oldState = rt->mState[m_cmdBuffer->cmdBuffer()];
				context.newState = context.oldState;

				if (GR_IMAGE_STATE_UNINITIALIZED_TARGET != currentState)
				{
					GR_IMAGE_STATE tmpState = rt->mState[m_cmdBuffer->cmdBuffer()];
					IGO_UNUSED(tmpState)
					IGO_ASSERT(tmpState == currentState);
				}

				context.queue = GR_NULL_HANDLE;
				
				if (runTwitch)
				{
					if (TwitchManager::IsBroadCastingInitiated())
						CreateScreenCopyMANTLE(&context);
					else
						DestroyScreenCopyMANTLE(&context);

					if (TwitchManager::IsBroadCastingInitiated())
					{
						TwitchManager::TTV_PollTasks();
						if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
						{
							bool s = false;
							{
								EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gTwitchObjectCreation, EA::Thread::RWMutex::kLockTypeRead);
								s = SAFE_CALL(gScreenCopy, &IScreenCopy::Render, &context, NULL);
							}

							if (!s)
								DestroyScreenCopyMANTLE(&context);
						}
					}
				}
				
				// render IGO
				{
					SAFE_CALL_LOCK_ENTER

					SAFE_CALL(IGOApplication::instance(), &IGOApplication::render, &context);

					if (gRenderPass[renderDeviceIndex])
					{
						// add exisiting references - typically from the game
						m_cmdBuffer->addMemReferences(in_memRefCount, in_pMemRefs);

						//add rendering commands to the cmd buffer
						gRenderPass[renderDeviceIndex]->bakeCmdBuffer(m_cmdBuffer, currentState, stateAfterRendering, out_pMemRefs, out_memRefCount);
					}

					SAFE_CALL_LOCK_LEAVE
				}
			}
			restoreRenderState();
		}
		SAFE_CALL_LOCK_ENTER
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::stopTime);
		SAFE_CALL_LOCK_LEAVE

		IGOLogDebug(" ---- ");
	}

	DEFINE_HOOK_SAFE_VOID(grCmdCopyImageHook, (
		GR_CMD_BUFFER        cmdBuffer,
		GR_IMAGE             srcImage,
		GR_IMAGE             destImage,
		GR_UINT              regionCount,
		const GR_IMAGE_COPY* pRegions))

		if (!isCMDBufferOnUniversalQueue(cmdBuffer))
		{

			//IGOLogDebug("grCmdCopyImageHook not on universal queue");
			return grCmdCopyImageHookNext(cmdBuffer,
				srcImage,
				destImage,
				regionCount,
				pRegions);
		}

		{ // lock

			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			for (size_t i = 0; i < gRenderDevices.size(); i++)
			{

				// present
				{
					tMantleRenderTargetList::iterator rtiter;
					for (rtiter = gRenderDevices[i]->mRenderTargets.begin(); rtiter != gRenderDevices[i]->mRenderTargets.end(); ++rtiter)
					{
						if ((*rtiter)->mViewCreateInfo.image == srcImage)
						{
							IGOLogDebug("grCmdCopyImageHook device %i cmdBuffer %p rendertarget(%p) (state %x) src im %p -> dest im %p bound %i present %i present state %i", i, cmdBuffer, (*rtiter), (*rtiter)->mState[cmdBuffer], srcImage, destImage, (*rtiter)->mBound[cmdBuffer], (*rtiter)->mPresent, (*rtiter)->mPresentState);
							if (/*(*rtiter)->mPresent || */(*rtiter)->mBound[cmdBuffer])
							{
								IGOLogDebug("*grCmdCopyImageHook device %i cmdBuffer %p rendertarget(%p) (state %x) src im %p -> dest im %p bound %i present %i present state %i", i, cmdBuffer, (*rtiter), (*rtiter)->mState[cmdBuffer], srcImage, destImage, (*rtiter)->mBound[cmdBuffer], (*rtiter)->mPresent, (*rtiter)->mPresentState);

								tCmdBufferMap::iterator iter = gRenderDevices[i]->mCommandBuffers.find(cmdBuffer);
								if (iter != gRenderDevices[i]->mCommandBuffers.end())
								{
									if (!((*iter).second.started && !(*iter).second.defered))
										IGOLogDebug("command buffer not accessible");

									IGOLogDebug("cmd flags %i queueType %s", iter->second.flags, iter->second.queueType == GR_QUEUE_UNIVERSAL ? "(universal)" : "(non universal)");
                                    // Currently (07/16/2015) not used by any FB games
                                    /*
									if ((*iter).second.used == false)
									{
										// cmd buffer interception 										
										doIGO(true, (*iter).second.cmd, 0, (GR_MEMORY_REF*)0, &((*iter).second.memRefCount), &((*iter).second.pMemRefs), (*rtiter), gRenderDevices[i], static_cast<int>(i), GR_IMAGE_STATE_UNINITIALIZED_TARGET, (*rtiter)->mState[cmdBuffer]);
										(*iter).second.used = true;
										
										IGOLogDebug("*cmd buffer %p used %i", (*iter).second.cmd->cmdBuffer(), (*iter).second.used);
									}
									else
									{
										IGOLogDebug("multiple cmd buffer interception attempts");
									}
                                    */
								}
								else
								{
									IGOLogDebug("command buffer not found");
								}
							}
							else
								IGOLogDebug("no rt bound");
						}
                        else
                        {
                            /*
                            if ((*rtiter)->mViewCreateInfo.image == destImage)
                            {
                                IGOLogDebug("grCmdCopyImageHook device %i cmdBuffer %p rendertarget(%p) (state %x)  dest im %p <- src im %p bound %i present %i present state %i", i, cmdBuffer, (*rtiter), (*rtiter)->mState[cmdBuffer], destImage, srcImage, (*rtiter)->mBound[cmdBuffer], (*rtiter)->mPresent, (*rtiter)->mPresentState);
                            }
                            */
                        }
					}
				}
			}
		}
	
		grCmdCopyImageHookNext(cmdBuffer,
			srcImage,
			destImage,
			regionCount,
			pRegions);
	}

	DEFINE_HOOK_SAFE_VOID(grCmdCopyMemoryToImageHook, (
		GR_CMD_BUFFER               cmdBuffer,
		GR_GPU_MEMORY               srcMem,
		GR_IMAGE                    destImage,
		GR_UINT                     regionCount,
		const GR_MEMORY_IMAGE_COPY* pRegions))
	
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			if (!isCMDBufferOnUniversalQueue(cmdBuffer))
			{
				return grCmdCopyMemoryToImageHookNext(cmdBuffer,
					srcMem,
					destImage,
					regionCount,
					pRegions);
			}

			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
			{
				tMantleRenderTargetList::iterator rtiter;
				for (rtiter = (*iter)->mRenderTargets.begin(); rtiter != (*iter)->mRenderTargets.end(); ++rtiter)
				{
					{
						if ((*rtiter)->mMemory == srcMem || (*rtiter)->mViewCreateInfo.image == destImage)
						{
							IGOLogDebug("grCmdCopyMemoryToImageHook rendertarget s %p d %p", (*rtiter), srcMem, destImage);
						}
					}
				}
			}
		}
		grCmdCopyMemoryToImageHookNext(cmdBuffer,
			srcMem,
			destImage,
			regionCount,
			pRegions);
	}


	DEFINE_HOOK_SAFE_VOID(grCmdCopyImageToMemoryHook, (
		GR_CMD_BUFFER               cmdBuffer,
		GR_IMAGE                    srcImage,
		GR_GPU_MEMORY               destMem,
		GR_UINT                     regionCount,
		const GR_MEMORY_IMAGE_COPY* pRegions))

		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			if (!isCMDBufferOnUniversalQueue(cmdBuffer))
			{
				return grCmdCopyImageToMemoryHookNext(cmdBuffer,
					srcImage,
					destMem,
					regionCount,
					pRegions);
			}

			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
			{
				tMantleRenderTargetList::iterator rtiter;
				for (rtiter = (*iter)->mRenderTargets.begin(); rtiter != (*iter)->mRenderTargets.end(); ++rtiter)
				{
					{
						if ((*rtiter)->mViewCreateInfo.image == srcImage || (*rtiter)->mMemory == destMem)
						{
							IGOLogDebug("grCmdCopyImageToMemoryHook rendertarget s %p d %p", (*rtiter), srcImage, destMem);
						}
					}
				}
			}
		}
	
		grCmdCopyImageToMemoryHookNext(cmdBuffer,
		srcImage,
		destMem,
		regionCount,
		pRegions);
	}


	DEFINE_HOOK_SAFE(GR_RESULT, grSignalQueueSemaphoreHook, (
		GR_QUEUE queue,
		GR_QUEUE_SEMAPHORE semaphore))

		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			for (size_t i = 0; i < gRenderDevices.size(); i++)
			{
				if (gRenderDevices[i]->mDMAQueue == queue)
				{
					IGOLogDebug("grSignalQueueSemaphoreHook DMA %p %p device %i", queue, semaphore, i);
                    break;
				}
				else
					if (gRenderDevices[i]->mComputeOnlyQueue == queue)
					{
						IGOLogDebug("grSignalQueueSemaphoreHook Compute %p %p device %i", queue, semaphore, i);
                        break;
                    }
                    else
                        if (gRenderDevices[i]->mUniversalQueue == queue)
                        {
                            IGOLogDebug("grSignalQueueSemaphoreHook Universal %p %p device %i", queue, semaphore, i);
                            break;
                        }
                        else
                            if (gRenderDevices[i]->mTimerQueue == queue)
                            {
                                IGOLogDebug("grSignalQueueSemaphoreHook Timer %p %p device %i", queue, semaphore, i);
                                break;
                            }
            }
		}

    GR_RESULT result = grSignalQueueSemaphoreHookNext(queue, semaphore);
		return result;
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grOpenSharedQueueSemaphoreHook, (
		GR_DEVICE                           device,
		const GR_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
		GR_QUEUE_SEMAPHORE*                 pSemaphore))
		

		GR_RESULT result = grOpenSharedQueueSemaphoreHookNext(device, pOpenInfo, pSemaphore);
	
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			for (size_t i = 0; i < gRenderDevices.size(); i++)
			{
				if (gRenderDevices[i]->mDevice == device)
				{
					IGOLogDebug("grOpenSharedQueueSemaphoreHook sem. %p -> new sem. %p device %i", pOpenInfo->sharedSemaphore, *pSemaphore, i);
				}
			}
		}
		return result;
	}


	DEFINE_HOOK_SAFE(GR_RESULT, grWaitQueueSemaphoreHook, (
		GR_QUEUE queue,
		GR_QUEUE_SEMAPHORE semaphore))

		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			for (size_t i = 0; i < gRenderDevices.size(); i++)
			{
				if (gRenderDevices[i]->mDMAQueue == queue)
				{
					IGOLogDebug("grWaitQueueSemaphoreHook DMA %p %p device %i", queue, semaphore, i);
                    break;
				}
				else
					if (gRenderDevices[i]->mComputeOnlyQueue == queue)
					{
						IGOLogDebug("grWaitQueueSemaphoreHook Compute %p %p device %i", queue, semaphore, i);
                        break;
                    }
					else
						if (gRenderDevices[i]->mUniversalQueue == queue)
						{
							IGOLogDebug("grWaitQueueSemaphoreHook Universal %p %p device %i", queue, semaphore, i);
                            break;
                        }
                        else
                            if (gRenderDevices[i]->mTimerQueue == queue)
                            {
                                IGOLogDebug("grWaitQueueSemaphoreHook Timer %p %p device %i", queue, semaphore, i);
                                break;
                            }
            }
		}
		GR_RESULT result = grWaitQueueSemaphoreHookNext(queue, semaphore);
		return result;
	}


	DEFINE_HOOK_SAFE_VOID(grCmdCopyMemoryHook, (
		GR_CMD_BUFFER         cmdBuffer,
		GR_GPU_MEMORY         srcMem,
		GR_GPU_MEMORY         destMem,
		GR_UINT               regionCount,
		const GR_MEMORY_COPY* pRegions))

		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			if (!isCMDBufferOnUniversalQueue(cmdBuffer))
			{
				return grCmdCopyMemoryHookNext(cmdBuffer,
					srcMem,
					destMem,
					regionCount,
					pRegions);
			}


			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
			{
				tMantleRenderTargetList::iterator rtiter;
				for (rtiter = (*iter)->mRenderTargets.begin(); rtiter != (*iter)->mRenderTargets.end(); ++rtiter)
				{
					{
						if ((*rtiter)->mMemory == srcMem || (*rtiter)->mMemory == destMem)
						{
							IGOLogDebug("grCmdCopyMemoryHook rendertarget s %p d %p", (*rtiter), srcMem, destMem);
						}
					}
				}
			}
		}

		grCmdCopyMemoryHookNext(cmdBuffer,
		srcMem,
		destMem,
		regionCount,
		pRegions);
	}

	DEFINE_HOOK_SAFE_VOID(grCmdCloneImageDataHook, (
		GR_CMD_BUFFER  cmdBuffer,
		GR_IMAGE       srcImage,
		GR_ENUM        srcImageState,       // GR_IMAGE_STATE
		GR_IMAGE       destImage,
		GR_ENUM        destImageState))     // GR_IMAGE_STATE


		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			if (!isCMDBufferOnUniversalQueue(cmdBuffer))
			{
				return grCmdCloneImageDataHookNext(cmdBuffer,
					srcImage,
					srcImageState,       // GR_IMAGE_STATE
					destImage,
					destImageState);
			}


			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
			{
				tMantleRenderTargetList::iterator rtiter;
				for (rtiter = (*iter)->mRenderTargets.begin(); rtiter != (*iter)->mRenderTargets.end(); ++rtiter)
				{
					{
						if ((*rtiter)->mViewCreateInfo.image == srcImage || (*rtiter)->mViewCreateInfo.image == destImage)
						{
							IGOLogDebug("grCmdCloneImageDataHook rendertarget s %p d %p  ss %x ds %x", (*rtiter), srcImage, destImage, srcImageState, destImageState);
						}
					}
				}
			}
		}

		grCmdCloneImageDataHookNext(cmdBuffer,
		srcImage,
		srcImageState,       // GR_IMAGE_STATE
		destImage,
		destImageState);
	}




	DEFINE_HOOK_SAFE_VOID(grCmdBindTargetsHook,(
		GR_CMD_BUFFER                     cmdBuffer,
		GR_UINT                           colorTargetCount,
		const GR_COLOR_TARGET_BIND_INFO*  pColorTargets,
		const GR_DEPTH_STENCIL_BIND_INFO* pDepthTarget))


		{		
			if (!isCMDBufferOnUniversalQueue(cmdBuffer))
			{
				return grCmdBindTargetsHookNext(
					cmdBuffer,
					colorTargetCount,
					pColorTargets,
					pDepthTarget);
			}

			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite/*Read*/);

			for (size_t i2 = 0; i2 < gRenderDevices.size(); i2++)
			{
				if (gRenderDevices[i2]->mCommandBuffers.find(cmdBuffer) != gRenderDevices[i2]->mCommandBuffers.end())
				{
					tMantleRenderTargetList::iterator rtiter;
					for (rtiter = gRenderDevices[i2]->mRenderTargets.begin(); rtiter != gRenderDevices[i2]->mRenderTargets.end(); ++rtiter)
					{
						for (GR_UINT i = 0; i < colorTargetCount; i++)
						{
							(*rtiter)->mBound[cmdBuffer] = false; // unbind RT
							if ((*rtiter)->mView == pColorTargets[i].view)
							{
								IGOLogDebug("*grCmdBindTargetsHook(%i) device %i cmdBuffer %p rendertarget %p im %p (parent %p) state %x color target state %x", i, i2, cmdBuffer, (*rtiter), (*rtiter)->mViewCreateInfo.image, (*rtiter)->mParentImage, (*rtiter)->mState[cmdBuffer], pColorTargets[i].colorTargetState);
								(*rtiter)->mBound[cmdBuffer] = true;
								(*rtiter)->mState[cmdBuffer] = (GR_IMAGE_STATE)pColorTargets[i].colorTargetState;	// sync states
							}
						}
					}
				}
			}
		}


		grCmdBindTargetsHookNext(
		cmdBuffer,
		colorTargetCount,
		pColorTargets,
		pDepthTarget);
	}


       
    DEFINE_HOOK_SAFE_VOID(grCmdPrepareImagesHook, (
	GR_CMD_BUFFER                    cmdBuffer,
	GR_UINT                          transitionCount,
	const GR_IMAGE_STATE_TRANSITION* pStateTransitions))

        {
			if (!isCMDBufferOnUniversalQueue(cmdBuffer))
			{
				return grCmdPrepareImagesHookNext(cmdBuffer, transitionCount, pStateTransitions);
			}
			
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			for (size_t i2 = 0; i2 < gRenderDevices.size(); i2++)
			{
                if (gRenderDevices[i2]->mPresentDevice) /* exclude device 0, the presenting device, otherwise IGO breaks in BFH*/
                {
                    continue;
                }

                tMantleRenderTargetList::iterator rtiter;
				for (rtiter = gRenderDevices[i2]->mRenderTargets.begin(); rtiter != gRenderDevices[i2]->mRenderTargets.end(); ++rtiter)
				{
					for (GR_UINT i = 0; i < transitionCount; i++)
					{
						if ((*rtiter)->mViewCreateInfo.image == pStateTransitions[i].image)
						{
                            IGOLogDebug("grCmdPrepareImages device %i cmdBuffer %p rendertarget %p im %p (parent %p) os %x ns %x as %x arsi %i barsli %i bmil %i milvs %i", i2, cmdBuffer, (*rtiter), pStateTransitions[i].image, (*rtiter)->mParentImage, pStateTransitions[i].oldState, pStateTransitions[i].newState, pStateTransitions[i].subresourceRange.aspect, pStateTransitions[i].subresourceRange.arraySize, pStateTransitions[i].subresourceRange.baseArraySlice, pStateTransitions[i].subresourceRange.baseMipLevel, pStateTransitions[i].subresourceRange.mipLevels);
                            
                            tCmdBufferMap::iterator iter = gRenderDevices[i2]->mCommandBuffers.find(cmdBuffer);
							if (iter != gRenderDevices[i2]->mCommandBuffers.end())
							{
								IGOLogDebug("cmd %p cmd flags %i queueType %s", cmdBuffer, iter->second.flags, iter->second.queueType == GR_QUEUE_UNIVERSAL ? "(universal)" : "(non universal)");
                                IGOLogDebug("*grCmdPrepareImages device %i cmdBuffer %p rendertarget %p im %p (parent %p) present %i bound %i (storedstate %x) os %x ns %x as %x arsi %i barsli %i bmil %i milvs %i", i2, cmdBuffer, (*rtiter), pStateTransitions[i].image, (*rtiter)->mParentImage, (*rtiter)->mPresent, (*rtiter)->mBound[cmdBuffer], (*rtiter)->mState[cmdBuffer], pStateTransitions[i].oldState, pStateTransitions[i].newState, pStateTransitions[i].subresourceRange.aspect, pStateTransitions[i].subresourceRange.arraySize, pStateTransitions[i].subresourceRange.baseArraySlice, pStateTransitions[i].subresourceRange.baseMipLevel, pStateTransitions[i].subresourceRange.mipLevels);

                                if (pStateTransitions[i].oldState == GR_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL && pStateTransitions[i].newState == GR_IMAGE_STATE_DATA_TRANSFER)
                                {
                                    // multi-GPU windowed mode needs this
                                    if ((*iter).second.used == false && ((*iter).second.started || (*iter).second.defered) && (*rtiter)->mState[cmdBuffer] == (GR_IMAGE_STATE)pStateTransitions[i].oldState)
									{
                                        // cmd buffer interception 									
                                        doIGO(true, (*iter).second.cmd, 0, (GR_MEMORY_REF*)0, &((*iter).second.memRefCount), &((*iter).second.pMemRefs), (*rtiter), gRenderDevices[i2], i2, (*rtiter)->mState[cmdBuffer], (*rtiter)->mState[cmdBuffer]);
                                        (*iter).second.used = true;
                                        IGOLogDebug("*cmd buffer %p used %i", (*iter).second.cmd->cmdBuffer(), (*iter).second.used);                                       
									}
									else
									{
										IGOLogDebug("multiple cmd buffer interception attempts");
									}
								}
								else
									IGOLogDebug("no IGO injected, wrong states");
                                
							}
							else
							{
								IGOLogDebug("command buffer not found");
							}

                            (*rtiter)->mState[cmdBuffer] = (GR_IMAGE_STATE)pStateTransitions[i].newState;	// sync states
						}
					}
				}
			}
		}

		grCmdPrepareImagesHookNext(cmdBuffer, transitionCount, pStateTransitions);
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grCreateCommandBufferHook, (
		GR_DEVICE                        device,
		const GR_CMD_BUFFER_CREATE_INFO* pCreateInfo,
		GR_CMD_BUFFER*                   pCmdBuffer))

		GR_RESULT result = grCreateCommandBufferHookNext(device, pCreateInfo, pCmdBuffer);
		
		if (result == GR_SUCCESS)
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			for (size_t i = 0; i < gRenderDevices.size(); i++)
			{
				if (gRenderDevices[i]->mDevice == device && pCreateInfo->queueType == GR_QUEUE_UNIVERSAL)
				{
                    if (i > 0)
                    {
                        gMultiGPURenderingEnabled = true;
                        IGOLogWarn("multi GPU rendering detected");
                    }

					if (pCreateInfo->queueType == GR_QUEUE_UNIVERSAL)
						IGOLogDebug("grCreateCommandBufferHook (universal) device %i cmdBuffer %p flags %x", i, *pCmdBuffer, pCreateInfo->flags);
					else
						IGOLogDebug("grCreateCommandBufferHook (non universal) device %i cmdBuffer %p flags %x", i, *pCmdBuffer, pCreateInfo->flags);

                    DELETE_MANTLE_IF(gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].cmd, *pCmdBuffer);
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].memRefCount = 0;
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].pMemRefs = 0;
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].used = false;
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].defered = false;
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].started = false;
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].queueType = (GR_QUEUE_TYPE)pCreateInfo->queueType;
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].flags = 0;
					gRenderDevices[i]->mCommandBuffers[*pCmdBuffer].cmd = new CommandBuffer(*pCmdBuffer);
				}
			}
		}
		return result;
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grQueueSubmitHook, (
		GR_QUEUE             queue,
		GR_UINT              cmdBufferCount,
		const GR_CMD_BUFFER* pCmdBuffers,
		GR_UINT              memRefCount,
		const GR_MEMORY_REF* pMemRefs,
		GR_FENCE             fence))
	

        //EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);
        gRenderDevicesMutex.Lock(EA::Thread::RWMutex::kLockTypeWrite);
 
        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_HARDWARE, TelemetryRenderer_MANTLE, "GPUs: %i", gRenderDevices.size()));

		int devIndex = -1;
        {
            for (size_t i = 0; i < gRenderDevices.size(); i++)
            {
                if (gRenderDevices[i]->mUniversalQueue == queue
#ifdef	_MANTLE_DIAGNOSE_
                    || gRenderDevices[i]->mDMAQueue == queue
                    || gRenderDevices[i]->mComputeOnlyQueue == queue
                    || gRenderDevices[i]->mTimerQueue == queue
#endif
                    )
                {
                    devIndex = static_cast<int>(i);
                    break;
                }
            }
        }


#ifdef	_MANTLE_DIAGNOSE_
        if (devIndex != -1)
        {
            if (gRenderDevices[devIndex]->mDMAQueue == queue)
            {
                IGOLogDebug("grQueueSubmitHook DMA submit ----");
                IGOLogDebug("device %i", devIndex);
                IGOLogDebug("fence %p", fence);
                IGOLogDebug("cmdBufferCount %i", cmdBufferCount);
                for (GR_UINT c = 0; c < cmdBufferCount; c++)
                    IGOLogDebug("pCmdBuffers %p", pCmdBuffers[c]);
                IGOLogDebug("memRefCount %i", memRefCount);

            }
            else
                if (gRenderDevices[devIndex]->mComputeOnlyQueue == queue)
                {
                    IGOLogDebug("grQueueSubmitHook Compute submit ----");
                    IGOLogDebug("device %i", devIndex);
                    IGOLogDebug("fence %p", fence);
                    IGOLogDebug("cmdBufferCount %i", cmdBufferCount);
                    for (GR_UINT c = 0; c < cmdBufferCount; c++)
                        IGOLogDebug("pCmdBuffers %p", pCmdBuffers[c]);
                    IGOLogDebug("memRefCount %i", memRefCount);

                }
                else
                    if (gRenderDevices[devIndex]->mUniversalQueue == queue)
                    {

                        IGOLogDebug("grQueueSubmitHook Universal submit----");
                        IGOLogDebug("device %i", devIndex);
                        IGOLogDebug("fence %p", fence);
                        IGOLogDebug("cmdBufferCount %i", cmdBufferCount);
                        for (GR_UINT c = 0; c < cmdBufferCount; c++)
                            IGOLogDebug("pCmdBuffers %p", pCmdBuffers[c]);
                        IGOLogDebug("memRefCount %i", memRefCount);

                    }
                    else
                        if (gRenderDevices[devIndex]->mTimerQueue == queue)
                        {

                            IGOLogDebug("grQueueSubmitHook Timer submit----");
                            IGOLogDebug("device %i", devIndex);
                            IGOLogDebug("fence %p", fence);
                            IGOLogDebug("cmdBufferCount %i", cmdBufferCount);
                            for (GR_UINT c = 0; c < cmdBufferCount; c++)
                                IGOLogDebug("pCmdBuffers %p", pCmdBuffers[c]);
                            IGOLogDebug("memRefCount %i", memRefCount);

                        }
                        else
                        {
                            IGOLogDebug("grQueueSubmitHook Unknown(u %p c %p d %p t %p) submit----", gRenderDevices[devIndex]->mUniversalQueue, gRenderDevices[devIndex]->mComputeOnlyQueue, gRenderDevices[devIndex]->mDMAQueue, gRenderDevices[devIndex]->mTimerQueue);
                            IGOLogDebug("device %i", devIndex);
                            IGOLogDebug("fence %p", fence);
                            IGOLogDebug("cmdBufferCount %i", cmdBufferCount);
                            for (GR_UINT c = 0; c < cmdBufferCount; c++)
                                IGOLogDebug("pCmdBuffers %p", pCmdBuffers[c]);
                            IGOLogDebug("memRefCount %i", memRefCount);

                        }

            // available rt's
            for (GR_UINT c = 0; c < cmdBufferCount; c++)
            {
                if (gRenderDevices[devIndex]->mUniversalQueue == queue)
                {
                    // check for render target marked "bound" and "present"
                    for (GR_UINT i2 = 0; i2 < memRefCount; i2++)
                    {
                        tMantleRenderTargetList::const_iterator rtiter;
                        for (rtiter = gRenderDevices[devIndex]->mRenderTargets.begin(); rtiter != gRenderDevices[devIndex]->mRenderTargets.end(); ++rtiter)
                        {
                            if (pMemRefs[i2].mem == (*rtiter)->mMemory) // found a RT
                            {
                                IGOLogDebug("renderTarget & cmd buffer combinations: device %i %p (state %x) im %p (parent %p) bound %i present %i present state %i", devIndex, (*rtiter), (*rtiter)->mState[pCmdBuffers[c]], (*rtiter)->mViewCreateInfo.image, (*rtiter)->mParentImage, (*rtiter)->mBound[pCmdBuffers[c]], (*rtiter)->mPresent, (*rtiter)->mPresentState);
                            }
                        }
                    }
                }
            }
        }
#endif

        if (devIndex == -1) // wrong queue or no queue -> early exit!
        {
            for (size_t i = 0; i < gRenderDevices.size(); i++)
            {
                for (GR_UINT c = 0; c < cmdBufferCount; c++)
                {
                    tCmdBufferMap::iterator iter = gRenderDevices[i]->mCommandBuffers.find(pCmdBuffers[c]);
                    if (iter != gRenderDevices[i]->mCommandBuffers.end())
                    {
                        if (iter->second.defered == true || iter->second.started == true)
                        {
                            IGOLogDebug("closing cmd buffers %p", pCmdBuffers[c]);
                            GR_RESULT result = grEndCommandBufferHookNext(pCmdBuffers[c]);
                            IGO_UNUSED(result)
                            IGO_ASSERT(result == GR_SUCCESS);
                            IGO_ASSERT(iter->second.used == false);
                        }

                        iter->second.defered = false;
                        iter->second.started = false;
                    }
                }
            }

            GR_RESULT result = GR_SUCCESS;

            result = grQueueSubmitHookNext(queue,
                cmdBufferCount,
                pCmdBuffers,
                memRefCount,
                pMemRefs,
                fence);


            if (result != GR_SUCCESS)
            {
                IGOLogDebug("*Queue submit failed, IGO destroyed.");
                IGOApplication::deleteInstance();
            }

            IGO_ASSERT(result == GR_SUCCESS);

            for (size_t i = 0; i < gRenderDevices.size(); i++)
            {
                // clear our command buffer
                for (GR_UINT c = 0; c < cmdBufferCount; c++)
                {
                    tCmdBufferMap::iterator iter = gRenderDevices[i]->mCommandBuffers.find(pCmdBuffers[c]);
                    if (iter != gRenderDevices[i]->mCommandBuffers.end())
                    {
                        if (iter->second.defered == true || iter->second.started == true)
                        {
                            IGOLogDebug("Cmd buffer not closed %p", pCmdBuffers[c]);
                            IGO_ASSERT(0);
                            result = grEndCommandBufferHookNext(pCmdBuffers[c]);
                        }

                        iter->second.defered = false;
                        iter->second.started = false;
                        if (iter->second.cmd)
                            iter->second.cmd->clearMemRefs(false);
                        iter->second.memRefCount = 0;
                        iter->second.pMemRefs = 0;
                    }
                }
            }

            gRenderDevicesMutex.Unlock();

            for (GR_UINT c = 0; c < cmdBufferCount; c++)
                purgeIGOMemory(pCmdBuffers[c]);

            purgeIGOMemory(0);

            return result;
        }


        if (queue)
		{
			tRenderDevice *dev = 0;
			tRenderTarget *rt = 0;
			GR_CMD_BUFFER cmdBuffer = 0;
			
			GR_UINT32 out_memRefCount = 0;
			GR_MEMORY_REF *out_pMemRefs = 0;

			GR_IMAGE_STATE state = GR_IMAGE_STATE_UNINITIALIZED_TARGET;
			{
				// present && bound
				bool searchDone = false;
				for (GR_UINT c = 0; c < cmdBufferCount && !searchDone; c++)
				{
					for (size_t i = 0; i < gRenderDevices.size() && !searchDone; i++)
					{
#ifdef	_MANTLE_DIAGNOSE_
                        if (gRenderDevices[i]->mUniversalQueue == queue)
						{
#endif
							// check for render target marked "bound" and "present"
                            if (fence != GR_NULL_HANDLE)    // only if we have a fence (usually device 0 on multi GPU systems)!
                            {
                                tMantleRenderTargetList::const_iterator rtiter;
                                for (rtiter = gRenderDevices[i]->mRenderTargets.begin(); rtiter != gRenderDevices[i]->mRenderTargets.end() && !searchDone; ++rtiter)
                                {
                                    IGOLogDebug("*(with fence - probing) renderTarget device %i %p (state %x) im %p (parent %p) bound %i present %i present state %i", i, (*rtiter), (*rtiter)->mState[pCmdBuffers[c]], (*rtiter)->mViewCreateInfo.image, (*rtiter)->mParentImage, (*rtiter)->mBound[pCmdBuffers[c]], (*rtiter)->mPresent, (*rtiter)->mPresentState);

                                    if ((*rtiter)->mPresent && ((*rtiter)->mBound[pCmdBuffers[c]]))
                                    {
                                        IGOLogDebug("*(with fence) renderTarget device %i %p (state %x) im %p (parent %p) bound %i present %i present state %i", i, (*rtiter), (*rtiter)->mState[pCmdBuffers[c]], (*rtiter)->mViewCreateInfo.image, (*rtiter)->mParentImage, (*rtiter)->mBound[pCmdBuffers[c]], (*rtiter)->mPresent, (*rtiter)->mPresentState);
                                        dev = gRenderDevices[i];
                                        rt = (*rtiter);
                                        state = (*rtiter)->mState[pCmdBuffers[c]];
                                        cmdBuffer = pCmdBuffers[c];
                                        searchDone = true;
                                    }
                                }
                            }
                            else
                            {
                                //Currently (07/16/2015) not used by any FB games
                                //old way, without using a fence - check for render target marked "bound" and "present"
                                /*
                                for (GR_UINT i2 = 0; i2 < memRefCount && !searchDone; i2++)
                                {
                                    tMantleRenderTargetList::const_iterator rtiter;
                                    for (rtiter = gRenderDevices[i]->mRenderTargets.begin(); rtiter != gRenderDevices[i]->mRenderTargets.end() && !searchDone; ++rtiter)
                                    {
                                        if ((pMemRefs[i2].mem == (*rtiter)->mMemory)) // found a RT
                                        {
                                            IGOLogDebug("*(without fence - probing) renderTarget device %i %p (state %x) im %p (parent %p) bound %i present %i present state %i", i, (*rtiter), (*rtiter)->mState[pCmdBuffers[c]], (*rtiter)->mViewCreateInfo.image, (*rtiter)->mParentImage, (*rtiter)->mBound[pCmdBuffers[c]], (*rtiter)->mPresent, (*rtiter)->mPresentState);

                                            if ((*rtiter)->mPresent && ((*rtiter)->mBound[pCmdBuffers[c]]))
                                            {
                                                IGOLogDebug("*(without fence) renderTarget device %i %p (state %x) im %p (parent %p) bound %i present %i present state %i", i, (*rtiter), (*rtiter)->mState[pCmdBuffers[c]], (*rtiter)->mViewCreateInfo.image, (*rtiter)->mParentImage, (*rtiter)->mBound[pCmdBuffers[c]], (*rtiter)->mPresent, (*rtiter)->mPresentState);
                                                dev = gRenderDevices[i];
                                                rt = (*rtiter);
                                                state = (*rtiter)->mState[pCmdBuffers[c]];
                                                cmdBuffer = pCmdBuffers[c];
                                                searchDone = true;
                                            }
                                        }
                                    }
                                }
                                */
                            }
#ifdef	_MANTLE_DIAGNOSE_
                        }
#endif
					}
				}
			}
			
			GR_RESULT result = GR_SUCCESS;
			
			{
                bool doneIGO = false;
				for (GR_UINT c = 0; c < cmdBufferCount && !doneIGO; c++)
				{
					tCmdBufferMap::iterator iter = gRenderDevices[devIndex]->mCommandBuffers.find(pCmdBuffers[c]);
					if (iter != gRenderDevices[devIndex]->mCommandBuffers.end())
					{
                        if (rt && dev && iter->second.cmd && cmdBuffer == iter->second.cmd->cmdBuffer() && iter->second.used)
                            IGOForceLogWarn("IGO execution conflict - two possible paths, using fence to decide which one to use!");


                        if (iter->second.used)
						{

                            {
#ifdef _DEBUG
                                bool validCMDBuffer = (iter->second.defered == false && iter->second.started == false) && iter->second.used == true;
#endif
                                IGO_ASSERT(gRenderDevices[devIndex]->mCommandBuffers[pCmdBuffers[c]].cmd == iter->second.cmd);
                                IGO_ASSERT(validCMDBuffer == true);
                                IGO_ASSERT(out_memRefCount == 0);

                                // add existing memory references				
                                if (memRefCount && pMemRefs)
                                    iter->second.cmd->addMemReferences(memRefCount, (GR_MEMORY_REF*)pMemRefs);
                                iter->second.cmd->getMemRefs(&out_pMemRefs, &out_memRefCount, true);

                                IGOLogDebug("IGO executed (path 1 - device %i) on cmdbuffer %p - %i (+%i) queueType %s ", devIndex, pCmdBuffers[c], memRefCount, out_memRefCount, iter->second.queueType == GR_QUEUE_UNIVERSAL ? "(universal)" : "(non universal)");

                                iter->second.used = false;
                                if (rt)
                                {
                                    rt->mBound[pCmdBuffers[c]] = false;
                                    IGOLogDebug("rt bound -> reset");
                                }
                                doneIGO = true;
                            }

						}
						else
						{
							//IGO_ASSERT(iter->second.used == false);
                            //dev 0 -> presenting device only rendering
                            if (rt && dev && iter->second.cmd && cmdBuffer == iter->second.cmd->cmdBuffer() && fence != GR_NULL_HANDLE)
							{
								IGO_ASSERT(iter->second.defered == true || iter->second.started == true);
								IGO_ASSERT(out_memRefCount == 0);
								IGO_ASSERT(iter->second.used == false);

								doIGO(true, iter->second.cmd, memRefCount, (GR_MEMORY_REF*)pMemRefs, &out_memRefCount, &out_pMemRefs, rt, dev, devIndex, GR_IMAGE_STATE_UNINITIALIZED_TARGET, state);

								IGOLogDebug("IGO executed (path 2 - device %i) on cmdbuffer %p - %i (+%i) flags %i queueType %s ", devIndex, pCmdBuffers[c], memRefCount, out_memRefCount, iter->second.flags, iter->second.queueType == GR_QUEUE_UNIVERSAL ? "(universal)" : "(non universal)");
								iter->second.used = false;
								rt->mBound[pCmdBuffers[c]] = false;
								doneIGO = true;
							}
						}
					}
				}

				for (GR_UINT c = 0; c < cmdBufferCount; c++)
				{
					tCmdBufferMap::iterator iter = gRenderDevices[devIndex]->mCommandBuffers.find(pCmdBuffers[c]);
					if (iter != gRenderDevices[devIndex]->mCommandBuffers.end())
					{
                        if (iter->second.defered == true || iter->second.started == true)
							result = grEndCommandBufferHookNext(pCmdBuffers[c]);

						iter->second.defered = false;
						iter->second.started = false;
							
						IGO_ASSERT(result == GR_SUCCESS);
					}
				}

				if (out_memRefCount > memRefCount)
				{
                    
                    if (!IGOApplication::instance() || IGOApplication::instance()->isPendingDeletion())
                    {
                        IGOLogWarn("IGO is shutting down, this may break the cmd buffer.");

                        // skip cmd buffer submission
                        /*
                        result = grQueueSubmitHookNext(queue,
                            cmdBufferCount,
                            pCmdBuffers,
                            memRefCount,
                            pMemRefs,
                            fence);
                        */
                    }
                    else
                    {
                        // altered cmd buffer
                        result = grQueueSubmitHookNext(queue,
                            cmdBufferCount,
                            pCmdBuffers,
                            out_memRefCount,
                            out_pMemRefs,
                            fence);
                    }

					if (result != GR_SUCCESS)
					{
						IGOLogDebug("*Queue submit failed, IGO destroyed.");
						IGOApplication::deleteInstance();
					}

					IGO_ASSERT(result == GR_SUCCESS);
				}
				else
				{
					result = grQueueSubmitHookNext(queue,
						cmdBufferCount,
						pCmdBuffers,
						memRefCount,
						pMemRefs,
						fence);

					if (result != GR_SUCCESS)
					{
						IGOLogDebug("*Queue submit failed, IGO destroyed.");
						IGOApplication::deleteInstance();
					}

					IGO_ASSERT(result == GR_SUCCESS);
				}
				
			}
			
			// clear our command buffer
			for (GR_UINT c = 0; c < cmdBufferCount; c++)
			{
				tCmdBufferMap::iterator iter = gRenderDevices[devIndex]->mCommandBuffers.find(pCmdBuffers[c]);
				if (iter != gRenderDevices[devIndex]->mCommandBuffers.end())
				{
                    if (iter->second.defered == true || iter->second.started == true)
                    {
                        IGOLogDebug("Cmd buffer not closed %p", pCmdBuffers[c]);
                        IGO_ASSERT(0);
                        result = grEndCommandBufferHookNext(pCmdBuffers[c]);
                    }

                    iter->second.defered = false;
					iter->second.started = false;
                    if (iter->second.cmd)
                        iter->second.cmd->clearMemRefs(false);
					iter->second.memRefCount = 0;
					iter->second.pMemRefs = 0;
				}
			}
			
            gRenderDevicesMutex.Unlock();

			for (GR_UINT c = 0; c < cmdBufferCount; c++)
				purgeIGOMemory(pCmdBuffers[c]);

			purgeIGOMemory(0);
			
			return result;

		}

		// close(end)the cmd buffer in our list
		for (GR_UINT c = 0; c < cmdBufferCount; c++)
		{
			tCmdBufferMap::iterator iter = gRenderDevices[devIndex]->mCommandBuffers.find(pCmdBuffers[c]);
			if (iter != gRenderDevices[devIndex]->mCommandBuffers.end())
			{
				GR_RESULT result = GR_SUCCESS;
                if (iter->second.defered == true || iter->second.started == true)
					result = grEndCommandBufferHookNext(pCmdBuffers[c]);

				iter->second.defered = false;
				iter->second.started = false;
                if (iter->second.cmd)
                    iter->second.cmd->clearMemRefs(false);
				iter->second.memRefCount = 0;
				iter->second.pMemRefs = 0;

				IGO_ASSERT(result == GR_SUCCESS);
			}
		}
		
		IGO_ASSERT(grQueueSubmitHookNext != NULL);


		GR_RESULT result = grQueueSubmitHookNext(queue,
												cmdBufferCount,
												pCmdBuffers,
												memRefCount,
												pMemRefs,
												fence);
		
		if (result != GR_SUCCESS)
		{
			IGOLogDebug("*Queue submit failed, IGO destroyed.");
			IGOApplication::deleteInstance();
		}

        gRenderDevicesMutex.Unlock();

		for (GR_UINT c = 0; c < cmdBufferCount; c++)
			purgeIGOMemory(pCmdBuffers[c]);

		purgeIGOMemory(0);
		
		return result;
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grGetDeviceQueueHook, (GR_DEVICE device, GR_ENUM queueType, GR_UINT queueIndex, GR_QUEUE* pQueue))

	IGOLogDebug("grGetDeviceQueueHook ----");
	IGOLogDebug("device %p", device);
	IGOLogDebug("queueType %d", queueType);
	IGOLogDebug("queueIndex %d", queueIndex);
	GR_RESULT hr = grGetDeviceQueueHookNext(device, queueType, queueIndex, pQueue);
	IGOLogDebug("queue %p", *pQueue);
	IGOLogDebug("----");

		if (hr == GR_SUCCESS)
		{
			switch (queueType)
			{
				case GR_QUEUE_UNIVERSAL:
				{
					EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

					tMantleRenderDeviceList::iterator iter;
					for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
					{
						if ((*iter)->mDevice == device)
						{
							(*iter)->mUniversalQueue = *pQueue;
							break;
						}
					}
				}
				break;
				
				case GR_QUEUE_COMPUTE_ONLY:
				{
					EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

					tMantleRenderDeviceList::iterator iter;
					for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
					{
						if ((*iter)->mDevice == device)
						{
							(*iter)->mComputeOnlyQueue = *pQueue;
							break;
						}
					}
				}
				break;

				case GR_EXT_QUEUE_DMA:
				{
					EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

					tMantleRenderDeviceList::iterator iter;
					for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
					{
						if ((*iter)->mDevice == device)
						{
							(*iter)->mDMAQueue = *pQueue;
							break;
						}
					}
				}
				break;

                case GR_EXT_QUEUE_TIMER:
                {
                    EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

                    tMantleRenderDeviceList::iterator iter;
                    for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
                    {
                        if ((*iter)->mDevice == device)
                        {
                            (*iter)->mTimerQueue = *pQueue;
                            break;
                        }
                    }
                }
                break;                
			}
		}

		return hr;
	}

	DEFINE_HOOK_SAFE(GR_RESULT, grInitAndEnumerateGpusHook, (
		const GR_APPLICATION_INFO* pAppInfo,
		const GR_ALLOC_CALLBACKS*  pAllocCb,
		GR_UINT*                   pGpuCount,
		GR_PHYSICAL_GPU            gpus[GR_MAX_PHYSICAL_GPUS]))

		if (pAppInfo)
		{
#ifdef TEST_FOR_ENGINE_OR_GAME_SPECIFIC_MANTLE_HACKS
			((GR_APPLICATION_INFO*)pAppInfo)->appVersion = 0;
			((GR_APPLICATION_INFO*)pAppInfo)->engineVersion = 0;
			((GR_APPLICATION_INFO*)pAppInfo)->pAppName = "test";
			((GR_APPLICATION_INFO*)pAppInfo)->pEngineName = "test";
#endif
			IGOLogWarn("grInitAndEnumerateGpu : api version %i app version %i engine version %i name %s engine %s", pAppInfo->apiVersion, pAppInfo->appVersion, pAppInfo->engineVersion, pAppInfo->pAppName, pAppInfo->pEngineName);
		}

		GR_RESULT result = grInitAndEnumerateGpusHookNext(pAppInfo, pAllocCb, pGpuCount, gpus);

		return result;

	}


	DEFINE_HOOK_SAFE(GR_RESULT, grCreateDeviceHook, (GR_PHYSICAL_GPU gpu, const GR_DEVICE_CREATE_INFO* pCreateInfo, GR_DEVICE* pDevice))

		IGOLogDebug("grCreateDeviceHook ----");
		if (!pCreateInfo)
			return GR_ERROR_INITIALIZATION_FAILED; 

#ifdef _DEBUG
		Sleep(5000);
		
#ifdef _ENABLE_DEVICE_VALIDATION_
		((GR_DEVICE_CREATE_INFO*)pCreateInfo)->flags |= GR_DEVICE_CREATE_VALIDATION;
		((GR_DEVICE_CREATE_INFO*)pCreateInfo)->maxValidationLevel = GR_VALIDATION_LEVEL_2;
		
		DWORD off = 0;
        DWORD on = 1;
        IGO_ASSERT(GR_SUCCESS == _grDbgSetGlobalOption(GR_DBG_OPTION_BREAK_ON_ERROR, 4, &off));
        on = 1;
        off = 0;
        IGO_ASSERT(GR_SUCCESS == _grDbgSetGlobalOption(GR_DBG_OPTION_BREAK_ON_WARNING, 4, &off));	
        on = 1;
        off = 0;
        IGO_ASSERT(GR_SUCCESS == _grDbgSetGlobalOption(GR_DBG_OPTION_DEBUG_ECHO_ENABLE, 4, &on));
        on = 1;
        off = 0;
#endif
#endif		

		IGOLogDebug("GPU %p", gpu);
		IGOLogDebug("extensionCount %d", pCreateInfo->extensionCount);
		IGOLogDebug("flags %d", pCreateInfo->flags);
		IGOLogDebug("maxValidationLevel %d", pCreateInfo->maxValidationLevel);
		IGOLogDebug("queueRecordCount %d", pCreateInfo->queueRecordCount);

		for (GR_UINT i = 0; i < pCreateInfo->extensionCount; i++)
		{
			IGOLogDebug("mantle extensions: %s", pCreateInfo->ppEnabledExtensionNames[i]);
		}

		for (GR_UINT i = 0; i < pCreateInfo->queueRecordCount; i++)
		{
			IGOLogDebug("queueCount %d", pCreateInfo->pRequestedQueues[i].queueCount);
			IGOLogDebug("queueType %d", pCreateInfo->pRequestedQueues[i].queueType);
		}
		

		IGOLogDebug("----");

		GR_RESULT hr = grCreateDeviceHookNext(gpu, pCreateInfo, pDevice);

		if (hr == GR_SUCCESS)
		{
			{	// short lock
				OriginIGO::tRenderDevice *renderDeviceIndex = new OriginIGO::tRenderDevice();

                renderDeviceIndex->mDevice = *pDevice;

                EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);
                renderDeviceIndex->mDeviceIndexExternal = static_cast<uint16_t>(gRenderDevices.size());    // external index in gRenderDevices
				gRenderDevices.push_back(renderDeviceIndex);
            }

			// create device specific objects

			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeRead);

			GpuMemMgr::Instance((GR_UINT)gRenderDevices.size() - 1)->init((GR_UINT)gRenderDevices.size() - 1);
			{
				GR_UINT32 heaps[GR_MAX_MEMORY_HEAPS] = { 0, 1, 2, 3, 4, 5, 6, 7 };
				GR_UINT32 numHeaps = GpuMemMgr::Instance((GR_UINT)gRenderDevices.size() - 1)->getHeapCount();
				GpuMemMgr::Instance((GR_UINT)gRenderDevices.size() - 1)->filterHeapsBySpeed(&numHeaps, heaps, 2/*GPU read*/, 0, 0, 1/*CPU Write*/);
				gTextureHeap[(GR_UINT)gRenderDevices.size() - 1] = heaps[0];
			}
			{
				GR_UINT32 heaps[GR_MAX_MEMORY_HEAPS] = { 0, 1, 2, 3, 4, 5, 6, 7 };
				GR_UINT32 numHeaps = GpuMemMgr::Instance((GR_UINT)gRenderDevices.size() - 1)->getHeapCount();
				GpuMemMgr::Instance((GR_UINT)gRenderDevices.size() - 1)->filterHeapsBySpeed(&numHeaps, heaps, 0, 0, 1/*CPU read*/, 0);
				gCPUReadHeap[(GR_UINT)gRenderDevices.size() - 1] = heaps[0];
			}
			{
				GR_UINT32 heaps[GR_MAX_MEMORY_HEAPS] = { 0, 1, 2, 3, 4, 5, 6, 7 };
				GR_UINT32 numHeaps = GpuMemMgr::Instance((GR_UINT)gRenderDevices.size() - 1)->getHeapCount();
				GpuMemMgr::Instance((GR_UINT)gRenderDevices.size() - 1)->filterHeapsBySpeed(&numHeaps, heaps, 2/*GPU read*/, 1/*GPU write*/, 0, 0);
				gGPUOnlyHeap[(GR_UINT)gRenderDevices.size() - 1] = heaps[0];
			}



			// Load Compiled Vertex & Pixel Shader from DLL resource
            ILC_VS_INPUT_LAYOUT emptyInputLayout = { 0 };
            DX_SLOT_INFO dynamic = { DXST_CONSTANTS, 1 };

            DWORD shaderCodeSize = 0;
            const char* shaderCode = NULL;
            if (LoadEmbeddedResource(IDR_IGOVSFX, &shaderCode, &shaderCodeSize))
            {
                gShaderVs[(GR_UINT)gRenderDevices.size() - 1] = new Shader();
                gShaderVs[(GR_UINT)gRenderDevices.size() - 1]->initFromBinaryHLSL(shaderCode, shaderCodeSize, gVbLayout, &dynamic, (GR_UINT)gRenderDevices.size() - 1);

            }

            else
            {
                gShaderVs[(GR_UINT)gRenderDevices.size() - 1] = NULL;
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_MANTLE, "Failed to access VS resource"))
                IGO_ASSERT(0);
            }

            if (LoadEmbeddedResource(IDR_IGOPSFX, &shaderCode, &shaderCodeSize))
            {
                gShaderPs[(GR_UINT)gRenderDevices.size() - 1] = new Shader();
                gShaderPs[(GR_UINT)gRenderDevices.size() - 1]->initFromBinaryHLSL(shaderCode, shaderCodeSize, emptyInputLayout, &dynamic, (GR_UINT)gRenderDevices.size() - 1);
            }
            else
            {
                gShaderPs[(GR_UINT)gRenderDevices.size() - 1] = NULL;
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_MANTLE, "Failed to access PS resource"))
                IGO_ASSERT(0);
            }

            if (LoadEmbeddedResource(IDR_IGOVSBACKGROUNDFX, &shaderCode, &shaderCodeSize))
            {
                gShaderBgVs[(GR_UINT)gRenderDevices.size() - 1] = new Shader();
                gShaderBgVs[(GR_UINT)gRenderDevices.size() - 1]->initFromBinaryHLSL(shaderCode, shaderCodeSize, gVbLayout, &dynamic, (GR_UINT)gRenderDevices.size() - 1);
            }
            else
            {
                gShaderBgVs[(GR_UINT)gRenderDevices.size() - 1] = NULL;
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_MANTLE, "Failed to access background VS resource"))
                IGO_ASSERT(0);
            }

            if (LoadEmbeddedResource(IDR_IGOPSBACKGROUNDFX, &shaderCode, &shaderCodeSize))
            {
                gShaderBgPs[(GR_UINT)gRenderDevices.size() - 1] = new Shader();
                gShaderBgPs[(GR_UINT)gRenderDevices.size() - 1]->initFromBinaryHLSL(shaderCode, shaderCodeSize, emptyInputLayout, &dynamic, (GR_UINT)gRenderDevices.size() - 1);
            }
            else
            {
                gShaderBgPs[(GR_UINT)gRenderDevices.size() - 1] = NULL;
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_MANTLE, "Failed to access background PS resource"))
                IGO_ASSERT(0);
            }

		}
		return hr;
    }

	DEFINE_HOOK_SAFE(GR_RESULT, grWsiWinGetDisplayModeListHook, (
		GR_WSI_WIN_DISPLAY       display,
		GR_UINT*                 pNumDisplayModes,
		GR_WSI_WIN_DISPLAY_MODE* pDisplayModeList))

		IGOLogDebug("grWsiWinGetDisplayModeListHook.");
		GR_RESULT result = grWsiWinGetDisplayModeListHookNext(display, pNumDisplayModes, pDisplayModeList);

		if (result == GR_SUCCESS && pNumDisplayModes != NULL && pDisplayModeList != NULL)
		{
			for (GR_UINT i = 0; i < *pNumDisplayModes; i++)
			{
				IGOLogDebug("mode[%i] %ix%i chn: %i fmt: %i hz: %i crossDisplay: %i stereo: %i", i, pDisplayModeList[i].extent.width, pDisplayModeList[i].extent.height, pDisplayModeList[i].format.channelFormat, pDisplayModeList[i].format.numericFormat, pDisplayModeList[i].refreshRate, pDisplayModeList[i].crossDisplayPresent, pDisplayModeList[i].stereo);
#ifdef _SIMULATE_NON_CROSS_DISPLAY_SUPPORT_	// hardware image composition with multiple GPU's
				pDisplayModeList[i].crossDisplayPresent = 0;
#endif
			}
		}

		return result;
	}
	
	DEFINE_HOOK_SAFE(GR_RESULT, grBindObjectMemoryHook, (GR_OBJECT object, GR_GPU_MEMORY mem, GR_GPU_SIZE offset))

		GR_RESULT result = grBindObjectMemoryHookNext(object, mem, offset);
		if (result == GR_SUCCESS)
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
			{
				tMantleRenderTargetList::iterator rtiter;
				for (rtiter = (*iter)->mRenderTargets.begin(); rtiter != (*iter)->mRenderTargets.end(); ++rtiter)
				{
					if ((*rtiter)->mViewCreateInfo.image == object)
					{
                        IGO_ASSERT((*rtiter)->mMemory == GR_NULL_HANDLE || mem == GR_NULL_HANDLE);

						(*rtiter)->mMemory = mem;
						break;
					}
				}
			}
		}

		return result;
	}


	DEFINE_HOOK_SAFE(GR_RESULT, grDestroyObjectHook, (GR_OBJECT object))

		IGOLogDebug("grDestroyObjectHook. %p", object);
		EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);
		
		bool deleteInstance = false;
		{
			
			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end() && !deleteInstance; ++iter)
			{
				tMantleRenderTargetList::iterator rtiter = (*iter)->mRenderTargets.begin();
				while (rtiter != (*iter)->mRenderTargets.end() && !deleteInstance)
				{
					if ((*rtiter)->mViewCreateInfo.image == object || (*rtiter)->mView == object)
					{
						deleteInstance = true;
						break;
					}
					++rtiter;
				}

				// see if it's our command buffer
				{
					tCmdBufferMap::iterator iterCmd = (*iter)->mCommandBuffers.find((GR_CMD_BUFFER)object);
					if (iterCmd != (*iter)->mCommandBuffers.end() && !deleteInstance)
					{
						deleteInstance = true;
						break;
					}
				}
			}
			
		}

		if (deleteInstance)// delete outside of the mutex!!!
		{
			IGOApplication::deleteInstance();
			purgeIGOMemory((GR_CMD_BUFFER)-1);
		}
		
		{
			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
			{
				tMantleRenderTargetList::iterator rtiter = (*iter)->mRenderTargets.begin();
				while (rtiter != (*iter)->mRenderTargets.end())
				{
					if ((*rtiter)->mParentImage == object)
						(*rtiter)->mParentImage = NULL;

					if ((*rtiter)->mViewCreateInfo.image == object || (*rtiter)->mView == object)
					{
						IGOLogDebug("grDestroyObjectHook. %p (%i %i)", object, (*rtiter)->mViewCreateInfo.image == object, (*rtiter)->mView == object);

						(*rtiter)->clearAll();
						delete (*rtiter);
						rtiter = (*iter)->mRenderTargets.erase(rtiter);
					}
					else
						++rtiter;
				}

                // check if any presentable RT's are left on this device
                rtiter = (*iter)->mRenderTargets.begin();
                bool found = false;
                while ((rtiter != (*iter)->mRenderTargets.end()) && !found)
                {
                    if ((*rtiter)->mPresent && (*iter)->mPresentDevice)
                        found = true;

                    ++rtiter;
                }
                if (!found)
                    (*iter)->mPresentDevice = false;    // if we have no presentable RT's anymore, set the mPresentDevice to false


				// see if it's our command buffer
				{
					tCmdBufferMap::iterator iterCmd = (*iter)->mCommandBuffers.find((GR_CMD_BUFFER)object);
					if (iterCmd != (*iter)->mCommandBuffers.end())
					{
						IGOLogDebug("grDestroyObjectHook. %p cmd buffer", object);

                        if (iterCmd->second.defered == true || iterCmd->second.started == true)
						{
							grEndCommandBufferHookNext((GR_CMD_BUFFER)object);
						}

						iterCmd->second.defered = false;
						DELETE_MANTLE_IF(iterCmd->second.cmd, 0);

						(*iter)->mCommandBuffers.erase(iterCmd);
					}
				}
			}
		}


		IGO_ASSERT(grDestroyObjectHookNext);
		GR_RESULT result = grDestroyObjectHookNext(object);
		IGOLogDebug("grDestroyObjectHook. %p succeeded", object);

		return result;
    }

    DEFINE_HOOK_SAFE(GR_RESULT, grFreeMemoryHook, (GR_GPU_MEMORY mem))

        IGOLogDebug("grFreeMemoryHook %p.", mem);
        bool deleteInstance = false;
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end() && !deleteInstance; ++iter)
			{
				tMantleRenderTargetList::iterator rtiter;
				for (rtiter = (*iter)->mRenderTargets.begin(); rtiter != (*iter)->mRenderTargets.end() && !deleteInstance; ++rtiter)
				{
					if ((*rtiter)->mMemory == mem)
					{
						deleteInstance = true;
						break;
					}
				}
			}
		}

		if (deleteInstance)	// delete outside of the mutex!!!
		{
			IGOApplication::deleteInstance();
			purgeIGOMemory((GR_CMD_BUFFER)-1);
		}

		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			tMantleRenderDeviceList::const_iterator iter;
			for (iter = gRenderDevices.begin(); iter != gRenderDevices.end(); ++iter)
			{
				tMantleRenderTargetList::iterator rtiter = (*iter)->mRenderTargets.begin();
				while (rtiter != (*iter)->mRenderTargets.end())
				{
					if ((*rtiter)->mMemory == mem)
					{
						delete (*rtiter);
						rtiter = (*iter)->mRenderTargets.erase(rtiter);
					}
					else
						++rtiter;
				}
			}
		}


		IGO_ASSERT(grFreeMemoryHookNext);
		return grFreeMemoryHookNext(mem);
    }



	DEFINE_HOOK_SAFE(GR_RESULT, grDestroyDeviceHook, (GR_DEVICE device))

        IGOLogDebug("grDestroyDeviceHook %p.", device);
		bool deleteInstance = false;
		// find current device
		int deviceIndex = -1;
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			for (unsigned int i = 0; i < gRenderDevices.size(); ++i)
			{
				if (gRenderDevices[i]->mDevice == device)
				{
                    // do not use 'i' as the deviceIndex here, it will we wrong, if we have multiple devices and each is destroyed via a grDestroyDeviceHook(..) call!
                    deviceIndex = gRenderDevices[i]->mDeviceIndexExternal;
					deleteInstance = true;
					break;
				}
			}
		}
		if (deleteInstance)	// delete outside of the mutex!!!
		{
			IGOApplication::deleteInstance();

			if (deviceIndex != -1)
			{
				MANTLE_OBJECTS_AUTO

				IGOLogDebug("gRenderPass[%i] deleted", deviceIndex);
				DELETE_MANTLE_IF(gRenderPass[deviceIndex], 0);
                DELETE_MANTLE_IF(gShaderVs[deviceIndex], 0);
                DELETE_MANTLE_IF(gShaderPs[deviceIndex], 0);
                DELETE_MANTLE_IF(gShaderBgVs[deviceIndex], 0);
                DELETE_MANTLE_IF(gShaderBgPs[deviceIndex], 0);
				GpuMemMgr::Instance(deviceIndex)->uninit();
			}

			purgeIGOMemory((GR_CMD_BUFFER)-1);

			gCurrentMantleDevice = NULL;
		}
		// delete our structure
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

            tMantleRenderDeviceList::iterator iter = gRenderDevices.begin();
			while (iter != gRenderDevices.end())
			{
                if ((*iter)->mDevice == device)
                {
                    delete (*iter);
                    iter = gRenderDevices.erase(iter);
                }
                else
                    ++iter;
			}
		}
		return grDestroyDeviceHookNext(device);
	}


    DEFINE_HOOK_SAFE(GR_RESULT, grWsiWinCreatePresentableImageHook, (GR_DEVICE device, const GR_WSI_WIN_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, GR_IMAGE* pImage, GR_GPU_MEMORY* pMem))
		IGOLogDebug("grWsiWinCreatePresentableImageHook ----");
		IGOLogDebug("gpu %p", device);
		IGOLogDebug("display %p", pCreateInfo->display);
		IGOLogDebug("width %d", pCreateInfo->extent.width);
		IGOLogDebug("height %d", pCreateInfo->extent.height);
		IGOLogDebug("flags %d", pCreateInfo->flags);
		IGOLogDebug("format chn %d", pCreateInfo->format.channelFormat);
		IGOLogDebug("format fmt %d", pCreateInfo->format.numericFormat);
		IGOLogDebug("usage %d", pCreateInfo->usage);
		
		GR_RESULT hr = grWsiWinCreatePresentableImageHookNext(device, pCreateInfo, pImage, pMem);

		if (hr == GR_SUCCESS)
		{
			OriginIGO::tRenderTarget *renderTarget = new OriginIGO::tRenderTarget();

			renderTarget->mImageCreateInfo = *pCreateInfo;
			renderTarget->mViewCreateInfo.image = *pImage;
			renderTarget->mMemory = *pMem;
			renderTarget->mPresent = true;	// true presentable image, not just a RT!!!


			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			for (unsigned int i = 0; i < gRenderDevices.size(); ++i)
			{
                if (gRenderDevices[i]->mDevice == device)
				{
                    gRenderDevices[i]->mPresentDevice = true;;  // set presentable state
					gRenderDevices[i]->mRenderTargets.push_back(renderTarget);
					IGOLogDebug("added rendertarget %p for gpu %i im %p", renderTarget, i, renderTarget->mViewCreateInfo.image);

					break;
				}
			}
			
			IGOLogDebug("----");
					
		}

		return hr;
    }

		DEFINE_HOOK_SAFE(GR_RESULT, grCreateImageHook, (
			GR_DEVICE                   device,
			const GR_IMAGE_CREATE_INFO* pCreateInfo,
			GR_IMAGE*                   pImage))


			GR_RESULT hr = grCreateImageHookNext(device, pCreateInfo, pImage);

			if (hr == GR_SUCCESS)
			{
				// the last render target "mRenderTargets.size()-1" is safer than the first "0", because BF4 creates new render targets before releasing old ones on screen resize events
				if (gRenderDevices.size() > 0 && gRenderDevices[0]->mRenderTargets.size() > 0 &&					
					gRenderDevices[0]->mRenderTargets[gRenderDevices[0]->mRenderTargets.size() - 1]->mImageCreateInfo.format.channelFormat == pCreateInfo->format.channelFormat &&
					gRenderDevices[0]->mRenderTargets[gRenderDevices[0]->mRenderTargets.size() - 1]->mImageCreateInfo.format.numericFormat == pCreateInfo->format.numericFormat &&
					gRenderDevices[0]->mRenderTargets[gRenderDevices[0]->mRenderTargets.size() - 1]->mImageCreateInfo.extent.width == pCreateInfo->extent.width &&
					gRenderDevices[0]->mRenderTargets[gRenderDevices[0]->mRenderTargets.size() - 1]->mImageCreateInfo.extent.height == pCreateInfo->extent.height &&
					pCreateInfo->samples == 1 && pCreateInfo->mipLevels == 1 && pCreateInfo->imageType == GR_IMAGE_2D && (pCreateInfo->usage & GR_IMAGE_USAGE_COLOR_TARGET))
				{
					IGOLogDebug("grCreateImageHookNext ----");
					IGOLogDebug("gpu %p", device);
					IGOLogDebug("width %d", pCreateInfo->extent.width);
					IGOLogDebug("height %d", pCreateInfo->extent.height);
					IGOLogDebug("flags %d", pCreateInfo->flags);
					IGOLogDebug("format chn %d", pCreateInfo->format.channelFormat);
					IGOLogDebug("format fmt %d", pCreateInfo->format.numericFormat);
					IGOLogDebug("usage %d", pCreateInfo->usage);
					IGOLogDebug("im %p", *pImage);

					// add check for existing image???

					OriginIGO::tRenderTarget *renderTarget = new OriginIGO::tRenderTarget();
					
					renderTarget->mImageCreateInfo.format.channelFormat = pCreateInfo->format.channelFormat;
					renderTarget->mImageCreateInfo.format.numericFormat = pCreateInfo->format.numericFormat;
	
					renderTarget->mImageCreateInfo.usage = pCreateInfo->usage;

					renderTarget->mImageCreateInfo.extent.width = pCreateInfo->extent.width;
					renderTarget->mImageCreateInfo.extent.height = pCreateInfo->extent.height;

					renderTarget->mImageCreateInfo.display = 0;
					renderTarget->mImageCreateInfo.flags = pCreateInfo->flags;
					
					renderTarget->mViewCreateInfo.image = *pImage;
					renderTarget->mMemory = GR_NULL_HANDLE; // memory will be assigned later via grBindObjectMemory(object, *memory, *offset);

					EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

					for (unsigned int i = 0; i < gRenderDevices.size(); ++i)
					{
						if (gRenderDevices[i]->mDevice == device)
						{
							gRenderDevices[i]->mRenderTargets.push_back(renderTarget);
							IGOLogDebug("added rendertarget %p for gpu %i im %p", renderTarget, i, renderTarget->mViewCreateInfo.image);

							break;
						}
					}

					IGOLogDebug("----");
				}
				else
				{
#ifdef _MANTLE_DIAGNOSE_
                    IGOLogDebug("grCreateImageHookNext (info only)----");
					IGOLogDebug("gpu %p", device);
					IGOLogDebug("width %d", pCreateInfo->extent.width);
					IGOLogDebug("height %d", pCreateInfo->extent.height);
					IGOLogDebug("flags %d", pCreateInfo->flags);
					IGOLogDebug("format chn %d", pCreateInfo->format.channelFormat);
					IGOLogDebug("format fmt %d", pCreateInfo->format.numericFormat);
					IGOLogDebug("usage %d", pCreateInfo->usage);
					IGOLogDebug("im %p", *pImage);
#endif
				}
			}
			return hr;

		}


		
		DEFINE_HOOK_SAFE(GR_RESULT, grCreateColorTargetViewHook, (GR_DEVICE device, const GR_COLOR_TARGET_VIEW_CREATE_INFO* pCreateInfo, GR_COLOR_TARGET_VIEW* pView))

		IGOLogDebug("grCreateColorTargetViewHook ----");
		GR_RESULT hr = grCreateColorTargetViewHookNext(device, pCreateInfo, pView);
		IGOLogDebug("gpu %p", device);

		if (hr == GR_SUCCESS)
		{
			EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

			for (unsigned int i = 0; i < gRenderDevices.size(); ++i)
			{
				if (gRenderDevices[i]->mDevice == device)
				{
					tMantleRenderTargetList::const_iterator rtiter;
					for (rtiter = gRenderDevices[i]->mRenderTargets.begin(); rtiter != gRenderDevices[i]->mRenderTargets.end(); ++rtiter)
					{
						if ((*rtiter)->mViewCreateInfo.image == pCreateInfo->image)
						{
							(*rtiter)->mView = *pView;
							IGOLogDebug("added rendertarget %p view %p for gpu %i im %p", (*rtiter), pView, i, (*rtiter)->mViewCreateInfo.image);
						}
					}
				}
			}
			IGOLogDebug(" ----");
		}

		return hr;
    }

    DEFINE_HOOK_SAFE(GR_RESULT, grWsiWinTakeFullscreenOwnershipHook, (GR_WSI_WIN_DISPLAY display, GR_IMAGE image))
    
		IGOLogDebug("grWsiWinTakeFullscreenOwnershipHook.");

		GR_RESULT result = grWsiWinTakeFullscreenOwnershipHookNext(display, image);
		bool windowedMode = false;

		gRenderingWindow = getProcMainWindow(GetCurrentProcessId());

		RECT rect = { 0 };
		if (gRenderingWindow)
			GetClientRect(gRenderingWindow, &rect);
		UINT width = rect.right - rect.left;
		UINT height = rect.bottom - rect.top;
		SAFE_CALL_LOCK_AUTO
		if (windowedMode != gWindowedMode || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenWidth) != width || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenHeight) != height)
		{
			gWindowedMode = windowedMode;
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, width, height);
		}

		return result;
    }

	DEFINE_HOOK_SAFE(GR_RESULT, grWsiWinReleaseFullscreenOwnershipHook, (GR_WSI_WIN_DISPLAY display))

		IGOLogDebug("grWsiWinReleaseFullscreenOwnershipHook.");

		GR_RESULT result = grWsiWinReleaseFullscreenOwnershipHookNext(display);
		bool windowedMode = true;

		gRenderingWindow = getProcMainWindow(GetCurrentProcessId());

		RECT rect = { 0 };
		if (gRenderingWindow)
			GetClientRect(gRenderingWindow, &rect);
		UINT width = rect.right - rect.left;
		UINT height = rect.bottom - rect.top;
		SAFE_CALL_LOCK_AUTO
		if (windowedMode != gWindowedMode || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenWidth) != width || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenHeight) != height)
		{
			gWindowedMode = windowedMode;
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
			SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, width, height);
		}

		return result;
	}

    // Present Hook
	DEFINE_HOOK_SAFE(GR_RESULT, grWsiWinQueuePresentHook, (GR_QUEUE queue, const GR_WSI_WIN_PRESENT_INFO* pPresentInfos))

		return grWsiWinQueuePresentHookNext(queue, pPresentInfos);
    }




    MantleHook::MantleHook()
    {
        isHooked = false;
    }


    MantleHook::~MantleHook()
    {
    }



    static bool MantleInit(HMODULE hMantle)
    {
		// just a safety precaution...
        UNHOOK_SAFE(grWsiWinQueuePresentHook);
        UNHOOK_SAFE(grWsiWinReleaseFullscreenOwnershipHook);
        UNHOOK_SAFE(grWsiWinTakeFullscreenOwnershipHook);
        UNHOOK_SAFE(grCreateColorTargetViewHook);
        UNHOOK_SAFE(grWsiWinCreatePresentableImageHook);
        UNHOOK_SAFE(grFreeMemoryHook);
        UNHOOK_SAFE(grDestroyObjectHook);
        UNHOOK_SAFE(grDestroyDeviceHook);
		
		UNHOOK_SAFE(grInitAndEnumerateGpusHook);

		UNHOOK_SAFE(grCreateDeviceHook);
		UNHOOK_SAFE(grGetDeviceQueueHook);
		UNHOOK_SAFE(grWsiWinGetDisplayModeListHook);
		UNHOOK_SAFE(grQueueSubmitHook);
		UNHOOK_SAFE(grCmdPrepareImagesHook);

		UNHOOK_SAFE(grCmdBindTargetsHook);

		UNHOOK_SAFE(grBindObjectMemoryHook);
		UNHOOK_SAFE(grCreateImageHook);

		UNHOOK_SAFE(grOpenPeerMemoryHook);
		UNHOOK_SAFE(grOpenPeerImageHook);
		UNHOOK_SAFE(grCmdCopyImageHook);
		UNHOOK_SAFE(grCmdCopyMemoryToImageHook);
		UNHOOK_SAFE(grCmdCopyImageToMemoryHook);
		UNHOOK_SAFE(grCmdCloneImageDataHook);
		UNHOOK_SAFE(grCmdCopyMemoryHook);

		UNHOOK_SAFE(grSignalQueueSemaphoreHook);
		UNHOOK_SAFE(grWaitQueueSemaphoreHook);
		UNHOOK_SAFE(grOpenSharedQueueSemaphoreHook);

		UNHOOK_SAFE(grCreateCommandBufferHook);
		UNHOOK_SAFE(grBeginCommandBufferHook);
		UNHOOK_SAFE(grEndCommandBufferHook);
		UNHOOK_SAFE(grResetCommandBufferHook);


#ifdef _WIN64
        const LPCSTR dllName = "mantle64.dll";
#else
        const LPCSTR dllName = "mantle32.dll";
#endif

        HOOKAPI_SAFE(dllName, "grInitAndEnumerateGpus", grInitAndEnumerateGpusHook);
		HOOKAPI_SAFE(dllName, "grCreateDevice", grCreateDeviceHook);
		HOOKAPI_SAFE(dllName, "grCreateCommandBuffer", grCreateCommandBufferHook);


#ifdef _MANTLE_DIAGNOSE_
		// API hooks that are just for diagnosing & debugging purpose !!!
		HOOKAPI_SAFE(dllName, "grWsiWinQueuePresent", grWsiWinQueuePresentHook);
        HOOKAPI_SAFE(dllName, "grOpenSharedMemory", grOpenSharedMemoryHook);
        HOOKAPI_SAFE(dllName, "grOpenPeerMemory", grOpenPeerMemoryHook);
		HOOKAPI_SAFE(dllName, "grCmdCopyMemoryToImage", grCmdCopyMemoryToImageHook);
		HOOKAPI_SAFE(dllName, "grCmdCopyImageToMemory", grCmdCopyImageToMemoryHook);
		HOOKAPI_SAFE(dllName, "grCmdCloneImageData", grCmdCloneImageDataHook);
		HOOKAPI_SAFE(dllName, "grCmdCopyMemory", grCmdCopyMemoryHook);
		HOOKAPI_SAFE(dllName, "grSignalQueueSemaphore", grSignalQueueSemaphoreHook);
		HOOKAPI_SAFE(dllName, "grWaitQueueSemaphore", grWaitQueueSemaphoreHook);
		HOOKAPI_SAFE(dllName, "grOpenSharedQueueSemaphore", grOpenSharedQueueSemaphoreHook);
#endif

		HOOKAPI_SAFE(dllName, "grWsiWinReleaseFullscreenOwnership", grWsiWinReleaseFullscreenOwnershipHook);
        HOOKAPI_SAFE(dllName, "grWsiWinTakeFullscreenOwnership", grWsiWinTakeFullscreenOwnershipHook);
        HOOKAPI_SAFE(dllName, "grCreateColorTargetView", grCreateColorTargetViewHook);
        HOOKAPI_SAFE(dllName, "grWsiWinCreatePresentableImage", grWsiWinCreatePresentableImageHook);
        HOOKAPI_SAFE(dllName, "grFreeMemory", grFreeMemoryHook);

		HOOKAPI_SAFE(dllName, "grBindObjectMemory", grBindObjectMemoryHook);

		HOOKAPI_SAFE(dllName, "grDestroyObject", grDestroyObjectHook);
        HOOKAPI_SAFE(dllName, "grDestroyDevice", grDestroyDeviceHook);
		HOOKAPI_SAFE(dllName, "grGetDeviceQueue", grGetDeviceQueueHook);
		HOOKAPI_SAFE(dllName, "grWsiWinGetDisplayModeList", grWsiWinGetDisplayModeListHook);
		HOOKAPI_SAFE(dllName, "grQueueSubmit", grQueueSubmitHook);
		HOOKAPI_SAFE(dllName, "grCmdPrepareImages", grCmdPrepareImagesHook);
		HOOKAPI_SAFE(dllName, "grCmdBindTargets", grCmdBindTargetsHook);

		HOOKAPI_SAFE(dllName, "grCreateImage", grCreateImageHook);

		HOOKAPI_SAFE(dllName, "grOpenPeerImage", grOpenPeerImageHook);
		HOOKAPI_SAFE(dllName, "grCmdCopyImage", grCmdCopyImageHook);
		
		

		HOOKAPI_SAFE(dllName, "grBeginCommandBuffer", grBeginCommandBufferHook);
		HOOKAPI_SAFE(dllName, "grEndCommandBuffer", grEndCommandBufferHook);
		HOOKAPI_SAFE(dllName, "grResetCommandBuffer", grResetCommandBufferHook);
		
		bool ret = true;
	    IGOLogInfo("MantleInit() called");

		// bind DLL functions

		BIND_MANTLE_FN(grInitAndEnumerateGpus, hMantle)
		BIND_MANTLE_FN(grGetGpuInfo, hMantle)
		BIND_MANTLE_FN(grCreateDevice, hMantle)
		BIND_MANTLE_FN(grDestroyDevice, hMantle)
		BIND_MANTLE_FN(grGetExtensionSupport, hMantle)
		BIND_MANTLE_FN(grGetDeviceQueue, hMantle)
		BIND_MANTLE_FN(grQueueSubmit, hMantle)
		BIND_MANTLE_FN(grQueueSetGlobalMemReferences, hMantle)
		BIND_MANTLE_FN(grQueueWaitIdle, hMantle)
		BIND_MANTLE_FN(grDeviceWaitIdle, hMantle)
		BIND_MANTLE_FN(grGetMemoryHeapCount, hMantle)
		BIND_MANTLE_FN(grGetMemoryHeapInfo, hMantle)
		BIND_MANTLE_FN(grAllocMemory, hMantle)
		BIND_MANTLE_FN(grFreeMemory, hMantle)
		BIND_MANTLE_FN(grSetMemoryPriority, hMantle)
		BIND_MANTLE_FN(grMapMemory, hMantle)
		BIND_MANTLE_FN(grUnmapMemory, hMantle)
		BIND_MANTLE_FN(grPinSystemMemory, hMantle)
		BIND_MANTLE_FN(grRemapVirtualMemoryPages, hMantle)
		BIND_MANTLE_FN(grGetMultiGpuCompatibility, hMantle)
		BIND_MANTLE_FN(grOpenSharedMemory, hMantle)
		BIND_MANTLE_FN(grOpenSharedQueueSemaphore, hMantle)
		BIND_MANTLE_FN(grOpenPeerMemory, hMantle)
		BIND_MANTLE_FN(grOpenPeerImage, hMantle)
		BIND_MANTLE_FN(grDestroyObject, hMantle)
		BIND_MANTLE_FN(grGetObjectInfo, hMantle)
		BIND_MANTLE_FN(grBindObjectMemory, hMantle)
		BIND_MANTLE_FN(grCreateFence, hMantle)
		BIND_MANTLE_FN(grGetFenceStatus, hMantle)
		BIND_MANTLE_FN(grWaitForFences, hMantle)
		BIND_MANTLE_FN(grCreateQueueSemaphore, hMantle)
		BIND_MANTLE_FN(grSignalQueueSemaphore, hMantle)
		BIND_MANTLE_FN(grWaitQueueSemaphore, hMantle)
		BIND_MANTLE_FN(grCreateEvent, hMantle)
		BIND_MANTLE_FN(grGetEventStatus, hMantle)
		BIND_MANTLE_FN(grSetEvent, hMantle)
		BIND_MANTLE_FN(grResetEvent, hMantle)
		BIND_MANTLE_FN(grCreateQueryPool, hMantle)
		BIND_MANTLE_FN(grGetQueryPoolResults, hMantle)
		BIND_MANTLE_FN(grGetFormatInfo, hMantle)
		BIND_MANTLE_FN(grCreateImage, hMantle)
		BIND_MANTLE_FN(grGetImageSubresourceInfo, hMantle)
		BIND_MANTLE_FN(grCreateImageView, hMantle)
		BIND_MANTLE_FN(grCreateColorTargetView, hMantle)
		BIND_MANTLE_FN(grCreateDepthStencilView, hMantle)
		BIND_MANTLE_FN(grCreateShader, hMantle)
		BIND_MANTLE_FN(grCreateGraphicsPipeline, hMantle)
		BIND_MANTLE_FN(grCreateComputePipeline, hMantle)
		BIND_MANTLE_FN(grStorePipeline, hMantle)
		BIND_MANTLE_FN(grLoadPipeline, hMantle)
		BIND_MANTLE_FN(grCreateSampler, hMantle)
		BIND_MANTLE_FN(grCreateDescriptorSet, hMantle)
		BIND_MANTLE_FN(grBeginDescriptorSetUpdate, hMantle)
		BIND_MANTLE_FN(grEndDescriptorSetUpdate, hMantle)
		BIND_MANTLE_FN(grAttachSamplerDescriptors, hMantle)
		BIND_MANTLE_FN(grAttachImageViewDescriptors, hMantle)
		BIND_MANTLE_FN(grAttachMemoryViewDescriptors, hMantle)
		BIND_MANTLE_FN(grAttachNestedDescriptors, hMantle)
		BIND_MANTLE_FN(grClearDescriptorSetSlots, hMantle)
		BIND_MANTLE_FN(grCreateViewportState, hMantle)
		BIND_MANTLE_FN(grCreateRasterState, hMantle)
		BIND_MANTLE_FN(grCreateMsaaState, hMantle)
		BIND_MANTLE_FN(grCreateColorBlendState, hMantle)
		BIND_MANTLE_FN(grCreateDepthStencilState, hMantle)
		BIND_MANTLE_FN(grCreateCommandBuffer, hMantle)
		BIND_MANTLE_FN(grBeginCommandBuffer, hMantle)
		BIND_MANTLE_FN(grEndCommandBuffer, hMantle)
		BIND_MANTLE_FN(grResetCommandBuffer, hMantle)
		BIND_MANTLE_FN(grCmdBindPipeline, hMantle)
		BIND_MANTLE_FN(grCmdBindStateObject, hMantle)
		BIND_MANTLE_FN(grCmdBindDescriptorSet, hMantle)
		BIND_MANTLE_FN(grCmdBindDynamicMemoryView, hMantle)
		BIND_MANTLE_FN(grCmdBindIndexData, hMantle)
		BIND_MANTLE_FN(grCmdBindTargets, hMantle)
		BIND_MANTLE_FN(grCmdPrepareMemoryRegions, hMantle)
		BIND_MANTLE_FN(grCmdPrepareImages, hMantle)
		BIND_MANTLE_FN(grCmdDraw, hMantle)
		BIND_MANTLE_FN(grCmdDrawIndexed, hMantle)
		BIND_MANTLE_FN(grCmdDrawIndirect, hMantle)
		BIND_MANTLE_FN(grCmdDrawIndexedIndirect, hMantle)
		BIND_MANTLE_FN(grCmdDispatch, hMantle)
		BIND_MANTLE_FN(grCmdDispatchIndirect, hMantle)
		BIND_MANTLE_FN(grCmdCopyMemory, hMantle)
		BIND_MANTLE_FN(grCmdCopyImage, hMantle)
		BIND_MANTLE_FN(grCmdCopyMemoryToImage, hMantle)
		BIND_MANTLE_FN(grCmdCopyImageToMemory, hMantle)
		BIND_MANTLE_FN(grCmdCloneImageData, hMantle)
		BIND_MANTLE_FN(grCmdUpdateMemory, hMantle)
		BIND_MANTLE_FN(grCmdFillMemory, hMantle)
		BIND_MANTLE_FN(grCmdClearColorImage, hMantle)
		BIND_MANTLE_FN(grCmdClearColorImageRaw, hMantle)
		BIND_MANTLE_FN(grCmdClearDepthStencil, hMantle)
		BIND_MANTLE_FN(grCmdResolveImage, hMantle)
		BIND_MANTLE_FN(grCmdSetEvent, hMantle)
		BIND_MANTLE_FN(grCmdResetEvent, hMantle)
		BIND_MANTLE_FN(grCmdMemoryAtomic, hMantle)
		BIND_MANTLE_FN(grCmdBeginQuery, hMantle)
		BIND_MANTLE_FN(grCmdEndQuery, hMantle)
		BIND_MANTLE_FN(grCmdResetQueryPool, hMantle)
		BIND_MANTLE_FN(grCmdWriteTimestamp, hMantle)
		BIND_MANTLE_FN(grCmdInitAtomicCounters, hMantle)
		BIND_MANTLE_FN(grCmdLoadAtomicCounters, hMantle)
		BIND_MANTLE_FN(grCmdSaveAtomicCounters, hMantle)
		BIND_MANTLE_FN(grCmdDbgMarkerBegin, hMantle)
		BIND_MANTLE_FN(grCmdDbgMarkerEnd, hMantle)
		BIND_MANTLE_FN(grDbgSetValidationLevel, hMantle)
		BIND_MANTLE_FN(grDbgRegisterMsgCallback, hMantle)
		BIND_MANTLE_FN(grDbgUnregisterMsgCallback, hMantle)
		BIND_MANTLE_FN(grDbgSetMessageFilter, hMantle)
		BIND_MANTLE_FN(grDbgSetObjectTag, hMantle)
		BIND_MANTLE_FN(grDbgSetGlobalOption, hMantle)
		BIND_MANTLE_FN(grDbgSetDeviceOption, hMantle)


		return ret;
    }


	// functions for the mantle helpers

	GR_DEVICE getMantleDevice(unsigned int gpu)
	{
#ifdef _DEBUG
		bool safe = gRenderDevicesMutex.GetLockCount(EA::Thread::RWMutex::kLockTypeRead) > 0 || gRenderDevicesMutex.GetLockCount(EA::Thread::RWMutex::kLockTypeWrite) > 0;
#endif
		IGO_ASSERT(safe);
		IGO_ASSERT(gRenderDevices.size() > gpu);
		
		if (gRenderDevices.size() > gpu)
			return gRenderDevices[gpu]->mDevice;
		else
		{
			IGO_ASSERT(0);
			return NULL;
		}
	}

	bool getMantleDeviceFullscreen(unsigned int gpu /*not yet used*/)
	{
		return gWindowedMode == false;
	}

    bool MantleHook::TryHook()
    {
        if (MantleHook::mInstanceHookMutex.TryLock())
        {
            // do not hook another GFX api, if we already have IGO running in this process!!!
            {
                SAFE_CALL_LOCK_AUTO
	            if (IGOApplication::instance()!=NULL && (IGOApplication::instance()->rendererType() != RendererMantle))
                {
                    MantleHook::mInstanceHookMutex.Unlock();
		            return false;
                }
            }

			// force a mantle hook (until we monitor dll loading properly) - it does no harm
	        HMODULE hMantle = NULL;
#ifdef _WIN64
			hMantle = GetModuleHandle(L"mantle64.dll");
#else
			hMantle = GetModuleHandle(L"mantle32.dll");
#endif
            if (hMantle && !isHooked)
	        {
		        isHooked = MantleInit(hMantle);
                if (isHooked)
                    CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_MANTLE, "Hooked at least once"))
	        }
            MantleHook::mInstanceHookMutex.Unlock();
        }
	    return isHooked;
    }

    static DWORD WINAPI IGOMantleHookDestroyThread(LPVOID *lpParam)
    {
        MantleHook::Cleanup();
        
        CloseHandle(MantleHook::mIGOMantleHookDestroyThreadHandle);
        MantleHook::mIGOMantleHookDestroyThreadHandle = NULL;
	    return 0;
    }


    void MantleHook::CleanupLater()
    {
        if (MantleHook::mIGOMantleHookDestroyThreadHandle==NULL)
            MantleHook::mIGOMantleHookDestroyThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOMantleHookDestroyThread, NULL, 0, NULL);
    }


    void MantleHook::Cleanup()
    {
        MantleHook::mInstanceHookMutex.Lock();
		
		IGOApplication::deleteInstance();
		purgeIGOMemory((GR_CMD_BUFFER)-1);

        UNHOOK_SAFE(grWsiWinQueuePresentHook);
        UNHOOK_SAFE(grWsiWinReleaseFullscreenOwnershipHook);
        UNHOOK_SAFE(grWsiWinTakeFullscreenOwnershipHook);
        UNHOOK_SAFE(grCreateColorTargetViewHook);
        UNHOOK_SAFE(grWsiWinCreatePresentableImageHook);
        UNHOOK_SAFE(grFreeMemoryHook);
        UNHOOK_SAFE(grDestroyObjectHook);
        UNHOOK_SAFE(grDestroyDeviceHook);

		UNHOOK_SAFE(grInitAndEnumerateGpusHook);

		UNHOOK_SAFE(grCreateDeviceHook);

		UNHOOK_SAFE(grGetDeviceQueueHook);
		UNHOOK_SAFE(grWsiWinGetDisplayModeListHook);
		UNHOOK_SAFE(grQueueSubmitHook);
		UNHOOK_SAFE(grCmdPrepareImagesHook);

		UNHOOK_SAFE(grCmdBindTargetsHook);

		UNHOOK_SAFE(grBindObjectMemoryHook);
		UNHOOK_SAFE(grCreateImageHook);

        UNHOOK_SAFE(grOpenSharedMemoryHook);
        UNHOOK_SAFE(grOpenPeerMemoryHook);
		UNHOOK_SAFE(grOpenPeerImageHook);
		UNHOOK_SAFE(grCmdCopyImageHook);
		UNHOOK_SAFE(grCmdCopyMemoryToImageHook);
		UNHOOK_SAFE(grCmdCopyImageToMemoryHook);
		UNHOOK_SAFE(grCmdCloneImageDataHook);
		UNHOOK_SAFE(grCmdCopyMemoryHook);

		UNHOOK_SAFE(grSignalQueueSemaphoreHook);
		UNHOOK_SAFE(grWaitQueueSemaphoreHook);
		UNHOOK_SAFE(grOpenSharedQueueSemaphoreHook);

		UNHOOK_SAFE(grCreateCommandBufferHook);
		UNHOOK_SAFE(grBeginCommandBufferHook);
		UNHOOK_SAFE(grEndCommandBufferHook);
		UNHOOK_SAFE(grResetCommandBufferHook);

        isHooked = false;
        MantleHook::mInstanceHookMutex.Unlock();

    }

	void getPurgeIGOMemoryInformation(int &mem, int &obj)
	{
		mem = 0;
		obj = 0;
	
		{
			EA::Thread::AutoFutex objMutexInstance(OriginIGO::gObjectMutex);
			for (tCmdBufferObjectMap::iterator iter = gCmdObjects.begin(); iter != gCmdObjects.end(); ++iter)
			{
				obj += (int)iter->second.size();
			}
		}

		{
			EA::Thread::AutoFutex memMutexInstance(OriginIGO::gMemMutex);
			for (tCmdBufferMemMap::iterator iter = gCmdMem.begin(); iter != gCmdMem.end(); ++iter)
			{
				mem += (int)iter->second.size();
			}
		}

	}

	void purgeIGOMemory(GR_CMD_BUFFER cmdBuffer)
	{
		IGOLogDebug("purgeIGOMemory %p", cmdBuffer);

		if (cmdBuffer == (GR_CMD_BUFFER)-1)
		{
			{
				EA::Thread::AutoFutex objMutex(OriginIGO::gObjectMutex);
				for (tCmdBufferObjectMap::iterator iter = gCmdObjects.begin(); iter != gCmdObjects.end(); ++iter)
				{
					for (size_t i = 0; i < iter->second.size(); i++)
						grDestroyObjectHookNext(iter->second[i]);

					iter->second.clear();
				}
				gCmdObjects.clear();
			}

			{
				EA::Thread::AutoFutex memMutex(OriginIGO::gMemMutex);
				for (tCmdBufferMemMap::iterator iter = gCmdMem.begin(); iter != gCmdMem.end(); ++iter)
				{
					for (size_t i = 0; i < iter->second.size(); i++)
						grFreeMemoryHookNext(iter->second[i]);

					iter->second.clear();
				}
				gCmdMem.clear();
			}
		}
		else
		{
            {
                EA::Thread::AutoRWMutex gRenderDevicesMutexInstance(gRenderDevicesMutex, EA::Thread::RWMutex::kLockTypeWrite);

                for (size_t i = 0; i < gRenderDevices.size(); i++)
                {
                    if (gRenderDevices[i]->mCommandBuffers.find(cmdBuffer) != gRenderDevices[i]->mCommandBuffers.end())
                    {
                        if (gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered || gRenderDevices[i]->mCommandBuffers[cmdBuffer].started)
                        {
                            gRenderDevices[i]->mCommandBuffers[cmdBuffer].defered = false;
                            gRenderDevices[i]->mCommandBuffers[cmdBuffer].started = false;
                            IGO_ASSERT(grEndCommandBufferHookNext);
                            GR_RESULT hresult = grEndCommandBufferHookNext(cmdBuffer);
                            IGOLogWarn("*purgeIGOMemory while cmdbuffer is in use %p %x", cmdBuffer, hresult);
                        }
                    }
                }
            }

            IGOLogDebug("---");

			{
				EA::Thread::AutoFutex objMutex(OriginIGO::gObjectMutex);
				tCmdBufferObjectMap::iterator iter = gCmdObjects.find(cmdBuffer);
				if (iter != gCmdObjects.end())
				{
					for (size_t i = 0; i < iter->second.size(); i++)
						grDestroyObjectHookNext(iter->second[i]);

					iter->second.clear();
					gCmdObjects.erase(iter);
				}
			}
			
			{
				EA::Thread::AutoFutex memMutex(OriginIGO::gMemMutex);
				tCmdBufferMemMap::iterator iter = gCmdMem.find(cmdBuffer);
				if (iter != gCmdMem.end())
				{
					for (size_t i = 0; i < iter->second.size(); i++)
						grFreeMemoryHookNext(iter->second[i]);

					iter->second.clear();
					gCmdMem.erase(iter);
				}
			}
		}
	}
}	// OriginIGO namespace



GR_RESULT GR_STDCALL _grDestroyObjectInternal(GR_OBJECT object, GR_CMD_BUFFER cmdBuffer)
{
	EA::Thread::AutoFutex objMutex(OriginIGO::gObjectMutex);
	OriginIGO::gCmdObjects[cmdBuffer].push_back(object);
	
	return GR_SUCCESS;
}

GR_RESULT GR_STDCALL _grFreeMemoryInternal(GR_GPU_MEMORY mem, GR_CMD_BUFFER cmdBuffer)
{
	EA::Thread::AutoFutex memMutex(OriginIGO::gMemMutex);
	OriginIGO::gCmdMem[cmdBuffer].push_back(mem);

	return GR_SUCCESS;
}
#endif // ORIGIN_PC
