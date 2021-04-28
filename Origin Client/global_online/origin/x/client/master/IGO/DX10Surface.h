///////////////////////////////////////////////////////////////////////////////
// DX10Surface.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DX10SURFACE_H
#define DX10SURFACE_H

#include "IGOSurface.h"

#if defined(ORIGIN_PC)

namespace OriginIGO {

    class DX10SurfacePrivate;
    class IGOWindow;

    class DX10Surface : public IGOSurface
    {
    public:
        DX10Surface(void *pDev, IGOWindow *pWindow);
        virtual ~DX10Surface();

		virtual void render(void *pDev);
		virtual void update(void *pDev, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags);

    private:
        IGOWindow *mWindow;
        DX10SurfacePrivate *mPrivate;
        int mWidth, mHeight;
    };
}
#endif

#endif 