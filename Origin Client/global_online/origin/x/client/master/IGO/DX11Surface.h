
///////////////////////////////////////////////////////////////////////////////
// DX11Surface.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef DX11SURFACE_H
#define DX11SURFACE_H

#include "IGOSurface.h"

#if defined(ORIGIN_PC)

#include <d3d11.h>
#include <d3dtypes.h>

namespace OriginIGO {

    class DX11SurfacePrivate;
    class IGOWindow;

    struct DX11CBNeverChanges
    {
        D3DMATRIX mViewProj;
    };

    struct DX11CBChangesEveryFrame
    {
        D3DMATRIX mWorld;
        float mColor[4];
    };

    class DX11Surface : public IGOSurface
    {
    public:
        DX11Surface(void *pDev, IGOWindow *pWindow);
        virtual ~DX11Surface();

		virtual void render(void *pDev);
		virtual void update(void *pDev, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags);

    private:
        IGOWindow *mWindow;
        DX11SurfacePrivate *mPrivate;
        int mWidth, mHeight;
    };
}
#endif

#endif 