
///////////////////////////////////////////////////////////////////////////////
// MantleSurface.h
// 
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MantleSURFACE_H
#define MantleSURFACE_H

#include "IGOSurface.h"

#if defined(ORIGIN_PC)

#include <mantle.h>
#include "MantleObjects.h"
#include "eathread/eathread_futex.h"

namespace OriginIGO {

    class MantleSurfacePrivate;
    class IGOWindow;

    class MantleSurface : public IGOSurface
    {
    public:
	    MantleSurface(void *context, IGOWindow *pWindow);
	    virtual ~MantleSurface();

		virtual void dealloc(void *context);

	    virtual void render(void *context);
		virtual void unrender(void *context); 
		virtual void update(void *context, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags);

    private:
		void initSurface(void *context);
	    
		IGOWindow *mWindow;
	    MantleSurfacePrivate *mPrivate;
		int mWidth[GR_MAX_PHYSICAL_GPUS], mHeight[GR_MAX_PHYSICAL_GPUS];
	    int mAlpha;
		GR_QUEUE mRenderQueue;
		bool mDirtyTexture[GR_MAX_PHYSICAL_GPUS];
		unsigned char *mBackupTexture;
		int mBackupDevice;
		GR_UINT32 mHeap;

		EA::Thread::Futex mMemoryBarrier;
	};
}
#endif

#endif 