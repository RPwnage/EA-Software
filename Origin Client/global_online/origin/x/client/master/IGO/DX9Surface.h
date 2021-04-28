///////////////////////////////////////////////////////////////////////////////
// DX9Surface.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGODX9SURFACE_H
#define IGODX9SURFACE_H

#include "IGOSurface.h"

#if defined(ORIGIN_PC)

namespace OriginIGO {

    class DX9SurfacePrivate;
    class IGOWindow;

    class DX9Surface : public IGOSurface
    {
    public:
        DX9Surface(void *pDev, IGOWindow *pWindow);
        virtual ~DX9Surface();

        virtual void render(void *context);
        virtual void update(void *context, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags);
        //virtual void updateRects(void *context, const eastl::vector<IGORECT> &rects, const char *data, int size);

    private:
        IGOWindow *mWindow;
        DX9SurfacePrivate *mPrivate;
        int mWidth, mHeight;
        int mAlpha;
    };
}
#endif

#endif 