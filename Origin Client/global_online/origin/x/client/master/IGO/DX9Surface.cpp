///////////////////////////////////////////////////////////////////////////////
// DX9Surface.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "DX9Surface.h"

#if defined(ORIGIN_PC)

#include <d3d9.h>
#include <d3dx9effect.h>

#include "IGOIPC/IGOIPC.h"

#include "DX9Hook.h"
#include "IGOWindow.h"
#include "IGOApplication.h"

namespace OriginIGO {

    extern D3DXMATRIX gDX9MatWorld;
    extern ID3DXEffect* gDX9Effect;
    extern D3DXHANDLE gDX9EffectTechnique;
    extern D3DXHANDLE gDX9ColorVariable;
    extern D3DXHANDLE gDX9TextureVariable;
    extern D3DXHANDLE gDX9WorldVariable;

    struct QuadVertex
    {
        float x, y, z;            //!< The transformed position for the vertex.
        DWORD diffuse;            //!< colour of the vertex
        float tu1, tv1;            //!< texture coordinates
    };

    struct ShaderQuadVertex
    {
        float x, y, z;            //!< The transformed position for the vertex.
        float tu1, tv1;            //!< texture coordinates
    };

    class DX9SurfacePrivate
    {
    public:
        DX9SurfacePrivate() { }
        ~DX9SurfacePrivate() { }

        IDirect3DDevice9 *mDev;
        IDirect3DTexture9 *mTexture;
        IDirect3DVertexBuffer9 *mVB;
    };


    DX9Surface::DX9Surface(void *pDev, IGOWindow *pWindow) :
        mWindow(pWindow),
        mWidth(0), mHeight(0),
        mAlpha(255)
    {
        mPrivate = new DX9SurfacePrivate();
        mPrivate->mDev = (IDirect3DDevice9 *)pDev;
        mPrivate->mTexture = NULL;
        mPrivate->mVB = NULL;
    }

    DX9Surface::~DX9Surface()
    {
        // don't delete the device
        RELEASE_IF(mPrivate->mTexture);
        RELEASE_IF(mPrivate->mVB);
        delete mPrivate;
    }

	void DX9Surface::render(void *pDevice)
    {
        IDirect3DDevice9 *pDev = (IDirect3DDevice9 *)pDevice;
        if (mPrivate && mPrivate->mTexture && mPrivate->mVB)
        {
            mWindow->checkDirty();

            // TODO: use a shader for DX9 too!
            {
                int alpha = mWindow->alpha();
                if (alpha != mAlpha)
                {
                    mAlpha = alpha;
                    update(pDevice, static_cast<uint8_t>(alpha), 0, 0, NULL, 0, IGOIPC::WINDOW_UPDATE_ALPHA);
                }
            }

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

            if (DX9Hook::ShaderSupport() == DX9Hook::ShaderSupport_FULL) // Using shader
            {
                gDX9MatWorld = D3DXMATRIX(matTransform);
                gDX9Effect->SetMatrix(gDX9WorldVariable, &gDX9MatWorld);

                // set vertex buffer
                UINT stride = sizeof(ShaderQuadVertex);
                UINT offset = 0;
                if (mPrivate->mVB)
                {
                    //pDev->SetVertexDeclaration();
                    pDev->SetStreamSource(0, mPrivate->mVB, offset, stride);

                    // set alpha color
                    float color[4];
                    color[0] = mWindow->customValue() / 10000.0f;
                    color[1] = 1.0f;
                    color[2] = 1.0f;
                    color[3] = mWindow->alpha() / 255.0f;
                    gDX9Effect->SetFloatArray(gDX9ColorVariable, color, 4);

                    gDX9Effect->SetTexture(gDX9TextureVariable, mPrivate->mTexture);

                    // draw the quad
                    gDX9Effect->SetTechnique(gDX9EffectTechnique);

                    UINT passCnt = 0;
                    UINT passIdx = mWindow->id() == BACKGROUND_WINDOW_ID ? 1 : 0;
                    HRESULT hr = 0;
                    if (SUCCEEDED(hr = gDX9Effect->Begin(&passCnt, 0)))
                    {
                        if (SUCCEEDED(hr = gDX9Effect->BeginPass(passIdx)))
                        {
                            pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

                            if (FAILED(hr = gDX9Effect->EndPass()))
                                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to end effect pass (%d) - hr=0x%08x", passIdx, hr));
                        }

                        else
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to begin effect pass (%d) - hr=0x%08x", passIdx, hr));

                        if (FAILED(hr = gDX9Effect->End()))
                            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to end effect - hr=0x%08x", hr));
                    }

                    else
                        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to access effect = hr=0x%08x", hr));
                }
            }

            else
            {
                pDev->SetTransform(D3DTS_WORLD, &matTransform);
                pDev->SetStreamSource(0, mPrivate->mVB, 0, sizeof(QuadVertex));
                pDev->SetTexture(0, mPrivate->mTexture);
                pDev->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
            }

        }
    }

	void DX9Surface::update(void *device, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
    {
        bool updateVerts = false;
        bool updateSize = false;
        enum DX9Hook::ShaderSupport shaderSupport = DX9Hook::ShaderSupport();

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
            mAlpha = alpha;
            color = D3DCOLOR_ARGB(alpha, 0xff, 0xff, 0xff);

            if (shaderSupport == DX9Hook::ShaderSupport_FIXED)
                updateVerts = true;
        }

        if (mPrivate->mTexture == NULL || updateSize)
        {
            RELEASE_IF(mPrivate->mTexture);
            // 0x0 dimensions make the d3d9 debug runtime print out a warning
            mPrivate->mDev->CreateTexture(mWidth == 0 ? 1 : mWidth, mHeight == 0 ? 1 : mHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &mPrivate->mTexture, NULL);
        }

        if (flags&IGOIPC::WINDOW_UPDATE_BITS)
        {
            D3DLOCKED_RECT rect;
            if (mPrivate->mTexture && SUCCEEDED(mPrivate->mTexture->LockRect(0, &rect, NULL, 0)))
            {
                D3DSURFACE_DESC desc;
                memset(&desc, 0, sizeof(desc));
                mPrivate->mTexture->GetLevelDesc(0, &desc);
                if (desc.Height != (uint32_t)mHeight || desc.Width != (uint32_t)mWidth)
                {
                    // this should never happen, but we see an error in SimCityNext, so keep this code for safety!!!
                    IGO_ASSERT(0);

                    mHeight = desc.Height;
                    mWidth = desc.Width;
                }

                copySurfaceData(data, rect.pBits, mWidth, mHeight, rect.Pitch);
                mPrivate->mTexture->UnlockRect(0);
            }
        }

        // Don't create the VB until we know whether to use the fixed pipeline/shaders

        if (shaderSupport != DX9Hook::ShaderSupport_UNKNOWN)
        {
            if (mPrivate->mVB == NULL)
            {
                HRESULT hr = 0;
                if (shaderSupport == DX9Hook::ShaderSupport_FULL)
                    hr = mPrivate->mDev->CreateVertexBuffer(sizeof(ShaderQuadVertex) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &mPrivate->mVB, NULL);

                else
                    hr = mPrivate->mDev->CreateVertexBuffer(sizeof(QuadVertex) * 4, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &mPrivate->mVB, NULL);

                if (SUCCEEDED(hr))
                    updateVerts = true;

                else
                    CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to create vertex buffer - hr=0x%08x", hr));
            }

            if (mPrivate->mVB && updateVerts)
            {
                if (shaderSupport == DX9Hook::ShaderSupport_FULL)
                {
                    static const ShaderQuadVertex vtx[] = {
                        { 0, 0, 1, 0.0f, 0.0f },
                        { 1, 0, 1, 1.0f, 0.0f },
                        { 0, -1, 1, 0.0f, 1.0f },
                        { 1, -1, 1, 1.0f, 1.0f },
                    };

                    // we need to only create the vertex buffer once
                    ShaderQuadVertex* pVertex;
                    if (SUCCEEDED(mPrivate->mVB->Lock(0, 0, (void**)&pVertex, 0)))
                    {
                        memcpy(pVertex, vtx, sizeof(ShaderQuadVertex) * 4);
                        mPrivate->mVB->Unlock();
                    }
                }

                else
                {
                    QuadVertex* pVertex;
                    if (SUCCEEDED(mPrivate->mVB->Lock(0, 0, (void**)&pVertex, D3DLOCK_DISCARD)))
                    {
                        QuadVertex vtx[4] = {
                            { 0, 0, 1, color, 0.0f, 0.0f },
                            { 1, 0, 1, color, 1.0f, 0.0f },
                            { 1, -1, 1, color, 1.0f, 1.0f },
                            { 0, -1, 1, color, 0.0f, 1.0f },
                        };
                        memcpy(pVertex, vtx, sizeof(QuadVertex) * 4);
                        mPrivate->mVB->Unlock();
                    }
                }
            } // ShaderSupport defined
        }
    }

    /* // currently not used
    void DX9Surface::updateRects(void *context, const eastl::vector<IGORECT> &rects, const char *data, int size)
    {
        if (!mPrivate->mTexture)
            return;

        eastl::vector<IGORECT>::const_iterator iter;
        const char *ptr = data;
        for (iter = rects.begin(); iter != rects.end(); ++iter)
        {
            IGORECT igorect = (*iter);
            int width = igorect.right - igorect.left;
            int height = igorect.bottom - igorect.top;
            RECT rect;
            memcpy(&rect, &igorect, sizeof(rect));

            D3DLOCKED_RECT d3drect;
            if (SUCCEEDED(mPrivate->mTexture->LockRect(0, &d3drect, &rect, 0)))
            {
                copySurfaceData(ptr, d3drect.pBits, width, height, d3drect.Pitch);
                mPrivate->mTexture->UnlockRect(0);
            }

            ptr += width*height*sizeof(uint32_t);
        }
    }*/
}
#endif // ORIGIN_PC