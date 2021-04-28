///////////////////////////////////////////////////////////////////////////////
// MantleShader.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef MANTLESHADER_H
#define MANTLESHADER_H

#ifdef ORIGIN_PC

#include <mantle.h>
#include <mantleExt.h>
#include <mantleWsiWinExt.h>
#include "MantleObjects.h"

#include "EAThread/eathread_rwmutex.h"
#include "EASTL/vector.h"
#include "EASTL/algorithm.h"
#include "EASTL/hash_map.h"
#include "EASTL/string.h"

#include "../../HookAPI.h"

namespace OriginIGO {
	
	EXTERN_NEXT_FUNC(GR_RESULT, grFreeMemoryHook, (GR_GPU_MEMORY mem));
	EXTERN_NEXT_FUNC(GR_RESULT, grDestroyObjectHook, (GR_OBJECT object));


	typedef class tCmdBuffer{
	public:
		GR_UINT32 memRefCount;
		GR_MEMORY_REF *pMemRefs;
		CommandBuffer *cmd;
		bool defered;
		bool started;
		bool used;
		GR_FLAGS flags;
		GR_QUEUE_TYPE queueType;

		tCmdBuffer()
		{
			memRefCount = 0;
			pMemRefs = NULL;
			cmd = NULL;
			defered = 0;
			started = 0;
			used = 0;
			flags = 0;
			queueType = GR_QUEUE_UNIVERSAL;
		}
	}tCmdBuffer;

	typedef eastl::vector<GR_OBJECT> tObjectList;
	typedef eastl::vector<GR_GPU_MEMORY> tMemList;

	typedef eastl::hash_map<GR_CMD_BUFFER, tCmdBuffer> tCmdBufferMap;
	typedef eastl::hash_map<GR_CMD_BUFFER, tObjectList> tCmdBufferObjectMap;
	typedef eastl::hash_map<GR_CMD_BUFFER, tMemList> tCmdBufferMemMap;

	class tRenderTarget
	{
	public:
		tRenderTarget()
		{
			clearAll();
		}

		void clearImage()
		{

			GR_FORMAT fmt =
			{
				GR_NUM_FMT_UNDEFINED,
				GR_NUM_FMT_UNDEFINED
			};

			mImageCreateInfo.format = fmt;
			mImageCreateInfo.usage = 0;
			mImageCreateInfo.extent.height = 0;
			mImageCreateInfo.extent.width = 0;
			mImageCreateInfo.display = 0;
			mImageCreateInfo.flags = 0;
		}

		void clearView()
		{
			mViewCreateInfo.arraySize = 0;
			mViewCreateInfo.baseArraySlice = 0;
			mViewCreateInfo.image = GR_NULL_HANDLE;
			mViewCreateInfo.mipLevel = 0;
		}

		void clearAll()
		{
			clearImage();
			clearView();
			
			mStateAfterPresent = GR_IMAGE_STATE_UNINITIALIZED_TARGET;
			mMemory = GR_NULL_HANDLE;
			mView = GR_NULL_HANDLE;
			mBound.clear();
			mState.clear();
			mPresent = false;
			mPresentState = false;
			mParentImage = GR_NULL_HANDLE;
		}


		GR_WSI_WIN_PRESENTABLE_IMAGE_CREATE_INFO  mImageCreateInfo;
		GR_COLOR_TARGET_VIEW_CREATE_INFO  mViewCreateInfo;

		eastl::hash_map<GR_CMD_BUFFER, GR_IMAGE_STATE> mState;
		GR_IMAGE_STATE			mStateAfterPresent;
		GR_GPU_MEMORY			mMemory;
		GR_COLOR_TARGET_VIEW	mView;
		GR_IMAGE				mParentImage;

		eastl::hash_map<GR_CMD_BUFFER, bool> mBound;
		bool					mPresent;
		bool					mPresentState;
	};

	typedef eastl::vector<tRenderTarget*> tMantleRenderTargetList;

	class tRenderDevice
	{
	public:
		tRenderDevice()
		{
            mPresentDevice = false;
			mDevice = GR_NULL_HANDLE;
            mDeviceIndexExternal = 0;
			mComputeOnlyQueue = GR_NULL_HANDLE;
			mUniversalQueue = GR_NULL_HANDLE;
            mDMAQueue = GR_NULL_HANDLE;
            mTimerQueue = GR_NULL_HANDLE;
            mFence = GR_NULL_HANDLE;
			mCommandBuffers.clear();
		}

		~tRenderDevice()
		{
			tMantleRenderTargetList::iterator rtiter = mRenderTargets.begin();
			while (rtiter != mRenderTargets.end())
			{
				(*rtiter)->clearAll();
				delete (*rtiter);
				rtiter = mRenderTargets.erase(rtiter);
			}

			tCmdBufferMap::iterator iterCmd = mCommandBuffers.begin();
			while (iterCmd != mCommandBuffers.end())
			{
				if (iterCmd->second.defered == true)
				{
					IGO_ASSERT(0);
				}

				if (iterCmd->second.cmd)
				{
					iterCmd->second.cmd->dealloc(0);
					delete iterCmd->second.cmd;
				}

				iterCmd = mCommandBuffers.erase(iterCmd);
			}
		}

		GR_DEVICE           mDevice;
        BOOL                mPresentDevice;
		GR_QUEUE			mComputeOnlyQueue;
		GR_QUEUE            mUniversalQueue;
        GR_QUEUE			mDMAQueue;
        GR_QUEUE			mTimerQueue;
        uint16_t            mDeviceIndexExternal;

		GR_FENCE            mFence;
		tMantleRenderTargetList mRenderTargets;
		tCmdBufferMap		mCommandBuffers;

	};

	typedef eastl::vector<tRenderDevice*> tMantleRenderDeviceList;

	extern GR_PIPELINE_SHADER gEmptyShader;
	extern GR_GRAPHICS_PIPELINE_CREATE_INFO gPipelineInfo;
	extern ILC_VS_INPUT_SLOT gVsInputSlots[2];
	extern ILC_VS_INPUT_LAYOUT gVbLayout;


}	// namespace OriginIGO

#endif // PC

#endif
