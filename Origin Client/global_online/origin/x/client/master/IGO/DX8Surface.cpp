///////////////////////////////////////////////////////////////////////////////
// DX8Surface.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _WIN64

#include "IGO.h"
#include "DX8Surface.h"

#if defined(ORIGIN_PC)

// Now that we support from XP to Windows 10, we can't include the DirectX 8 path directly in the build file, otherwise we'll get #define conflicts
//#include <d3d8.h>
#include "../../../../DirectX/8.1/include/d3d8.h"

#include "IGOIPC/IGOIPC.h"

#include "IGOWindow.h"
#include "IGOApplication.h"

namespace OriginIGO {

    struct QuadVertex
    {
        float x, y, z;            //!< The transformed position for the vertex.
        DWORD diffuse;            //!< colour of the vertex
        float tu1, tv1;            //!< texture coordinates
    };

    class DX8SurfacePrivate
    {
    public:
        DX8SurfacePrivate() { }
        ~DX8SurfacePrivate() { }

        IDirect3DDevice8 *mDev;
        IDirect3DTexture8 *mTexture;
        IDirect3DVertexBuffer8 *mVB;
    };


    DX8Surface::DX8Surface(void *pDev, IGOWindow *pWindow) :
        mWindow(pWindow),
        mWidth(0), mHeight(0)
    {
        mPrivate = new DX8SurfacePrivate();
        mPrivate->mDev = (IDirect3DDevice8 *)pDev;
        mPrivate->mTexture = NULL;
        mPrivate->mVB = NULL;
    }

    DX8Surface::~DX8Surface()
    {
        // don't delete the device
        RELEASE_IF(mPrivate->mTexture);
        RELEASE_IF(mPrivate->mVB);
        delete mPrivate;
    }

	void DX8Surface::render(void *pDevice)
    {
        IDirect3DDevice8 *pDev = (IDirect3DDevice8 *)pDevice;
        if (mPrivate && mPrivate->mTexture && mPrivate->mVB)
        {
            mWindow->checkDirty();

            float X;
            float Y;
            D3DMATRIX matTransform;
    
            //Get coordinates
            X = mWindow->x() - (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth) / 2;
            Y = -mWindow->y() + (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight) / 2; 

            ZeroMemory(&matTransform, sizeof(matTransform));

            // scale
            matTransform._11 = (FLOAT)mWindow->width() - 0.5f;
            matTransform._22 = (FLOAT)mWindow->height() - 0.5f;
            matTransform._33 = 1.0f;

            // translate
            matTransform._41 = X;
            matTransform._42 = Y;
            matTransform._44 = 1.0f;


            pDev->SetTransform (D3DTS_WORLD, &matTransform);
            pDev->SetStreamSource(0, mPrivate->mVB, sizeof(QuadVertex));
            pDev->SetTexture(0, mPrivate->mTexture);
            pDev->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
        }
    }

    void DX8Surface::update(void  *device, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
    {
        bool updateVerts = false;
        bool updateSize = false;

        if ((flags&IGOIPC::WINDOW_UPDATE_WIDTH) && (mWidth != width))
        {
            mWidth = width;
            updateSize = true;
        }

        if ((flags&IGOIPC::WINDOW_UPDATE_HEIGHT) && (mHeight != height))
        {
            mHeight = height;
            updateSize = true;
        }

        uint32_t color = 0xffffffff;
        if (flags&IGOIPC::WINDOW_UPDATE_ALPHA)
        {
            color = D3DCOLOR_ARGB(alpha, 0xff, 0xff, 0xff);
            updateVerts = true;
        }

        if (mPrivate->mTexture == NULL || updateSize)
        {
            RELEASE_IF(mPrivate->mTexture);
            mPrivate->mDev->CreateTexture(mWidth, mHeight, 1, 0, 
                D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &mPrivate->mTexture);
        }

        if (flags&IGOIPC::WINDOW_UPDATE_BITS)
        {
            D3DLOCKED_RECT rect;
            if (mPrivate->mTexture &&SUCCEEDED(mPrivate->mTexture->LockRect(0, &rect, NULL, 0)))
            {
                copySurfaceData(data, rect.pBits, mWidth, mHeight, rect.Pitch);
                mPrivate->mTexture->UnlockRect(0);
            }
        }

        // we need to only create the vertex buffer once
        if (mPrivate->mVB == NULL)
        {
            mPrivate->mDev->CreateVertexBuffer(sizeof(QuadVertex)*4, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 
                D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1, D3DPOOL_DEFAULT, &mPrivate->mVB);
            updateVerts = true;
        }

        if (updateVerts)
        {
            QuadVertex*    pVertex;
            if (SUCCEEDED(mPrivate->mVB->Lock(0, 0, (BYTE**)&pVertex, D3DLOCK_DISCARD)))
            {
                QuadVertex vtx[4] =  {
                    { 0,    0,        1,    color, 0.0f, 0.0f },
                    { 1,    0,        1,    color, 1.0f, 0.0f },
                    { 1,    -1,        1,    color, 1.0f, 1.0f },
                    { 0,    -1,        1,    color, 0.0f, 1.0f },
                };
                memcpy(pVertex, vtx, sizeof(QuadVertex)*4);

                mPrivate->mVB->Unlock();
            }
        }
    }
}
#endif // ORIGIN_PC

#endif // _WIN64