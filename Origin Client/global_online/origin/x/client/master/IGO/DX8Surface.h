///////////////////////////////////////////////////////////////////////////////
// DX8Surface.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DX8SURFACE_H
#define DX8SURFACE_H

#ifndef _WIN64

#include "IGOSurface.h"

#if defined(ORIGIN_PC)

namespace OriginIGO {

    class DX8SurfacePrivate;
    class IGOWindow;

    class DX8Surface : public IGOSurface
    {
    public:
        DX8Surface(void *pDev, IGOWindow *pWindow);
        virtual ~DX8Surface();

		virtual void render(void *pDev);
		virtual void update(void *pDev, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags);

    private:
        IGOWindow *mWindow;
        DX8SurfacePrivate *mPrivate;
        int mWidth, mHeight;
    };
}
#endif

#endif // _WIN64

#endif 