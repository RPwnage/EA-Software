///////////////////////////////////////////////////////////////////////////////
// DX10Surface.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "DX10Surface.h"

#if defined(ORIGIN_PC)

#include <d3d10.h>
#include <d3dtypes.h>

#include "IGOIPC/IGOIPC.h"

#include "IGOWindow.h"
#include "IGOApplication.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

namespace OriginIGO {

    extern D3DMATRIX gMatWorld;
    extern D3DMATRIX gMatOrtho;
    extern D3DMATRIX gMatIdentity;
    extern ID3D10EffectTechnique *gEffectTechnique;
    extern ID3D10EffectVectorVariable* gColorVariable;
    extern ID3D10EffectShaderResourceVariable* gTextureVariable;
    extern ID3D10EffectMatrixVariable* gWorldVariable;
    extern ID3D10EffectMatrixVariable* gViewProjectionVariable;
    
    struct QuadVertex
    {
        float x, y, z;            //!< The transformed position for the vertex.
        float tu1, tv1;            //!< texture coordinates
    };

    class DX10SurfacePrivate
    {
    public:
        DX10SurfacePrivate() { }
        ~DX10SurfacePrivate() { }

        ID3D10Device *mDev;
        ID3D10Texture2D *mTexture;
        ID3D10Buffer *mVB;
        ID3D10ShaderResourceView *mShaderResourceView;
    };

    DX10Surface::DX10Surface(void *pDev, IGOWindow *pWindow) :
        mWindow(pWindow),
        mWidth(0), mHeight(0)
    {
        mPrivate = new DX10SurfacePrivate();
        mPrivate->mDev = (ID3D10Device *)pDev;
        mPrivate->mTexture = NULL;
        mPrivate->mVB = NULL;
        mPrivate->mShaderResourceView = NULL;
    }

    DX10Surface::~DX10Surface()
    {
        // don't delete the device
        RELEASE_IF(mPrivate->mShaderResourceView);
        RELEASE_IF(mPrivate->mTexture);
        RELEASE_IF(mPrivate->mVB);
        delete mPrivate;
    }

	void DX10Surface::render(void *pDevice)
    {
        ID3D10Device *pDev = (ID3D10Device *)pDevice;
        if (mPrivate && mPrivate->mShaderResourceView &&pDev && gWorldVariable && gViewProjectionVariable && gEffectTechnique && gColorVariable && gTextureVariable)
        {
            mWindow->checkDirty();

            float X;
            float Y;
    
            //Get coordinates
            X = mWindow->x() - (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth) / 2;
            Y = -mWindow->y() + (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight) / 2; 

            ZeroMemory(&gMatWorld, sizeof(gMatWorld));

            // scale
            gMatWorld._11 = (FLOAT)mWindow->width();
            gMatWorld._22 = (FLOAT)mWindow->height();
            gMatWorld._33 = 1.0f;

            // translate
            gMatWorld._41 = X;
            gMatWorld._42 = Y;
            gMatWorld._44 = 1.0f;

            gWorldVariable->SetMatrix((float *)&gMatWorld);
            gViewProjectionVariable->SetMatrix((float*)&gMatOrtho);

            // set vertex buffer
            UINT stride = sizeof(QuadVertex);
            UINT offset = 0;
            if (mPrivate->mVB)
                pDev->IASetVertexBuffers(0, 1, &mPrivate->mVB, &stride, &offset);

            // set alpha color
            float color[4];
            color[0] = mWindow->customValue() / 10000.0f;
            color[1] = 1.0f;
            color[2] = 1.0f;
            color[3] = mWindow->alpha() / 255.0f;
            gColorVariable->SetFloatVector(color);
        
            gTextureVariable->SetResource(mPrivate->mShaderResourceView);

            // draw the quad
            // Tech pass 0 -> regular window
            // Tech pass 1 -> background
            UINT passIdx = mWindow->id() == BACKGROUND_WINDOW_ID ? 1 : 0;
            ID3D10EffectPass *pass = gEffectTechnique->GetPassByIndex(passIdx);
            if (pass)
            {
                pass->Apply(0);
                pDev->Draw(4, 0);
            }
        }
    }

	void DX10Surface::update(void *device, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
    {
        if (!(flags&IGOIPC::WINDOW_UPDATE_WIDTH))
            width = mWidth;

        if (!(flags&IGOIPC::WINDOW_UPDATE_HEIGHT))
            height = mHeight;

        if (mPrivate->mTexture == NULL || 
            (mWidth != width || mHeight != height) ||
            (flags&IGOIPC::WINDOW_UPDATE_BITS))
        {
            //TODO (Thomas): this call might be inefficient if we only have flags&IGOIPC::WINDOW_UPDATE_BITS and the texture dimensions are not changed

            RELEASE_IF(mPrivate->mShaderResourceView);
            RELEASE_IF(mPrivate->mTexture);
            D3D10_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.SampleDesc.Count = 1;
            desc.Format = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isSRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.Usage = D3D10_USAGE_DEFAULT;
            desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            D3D10_SUBRESOURCE_DATA dataRes;
            ZeroMemory(&dataRes, sizeof(dataRes));
            // fix for crash bug: https://developer.origin.com/support/browse/EBIBUGS-21939 where sometimes the size is too small for the width and height
            if (data && (size != (width * height * 4)))
                data = NULL;
            
            dataRes.pSysMem = data;
            dataRes.SysMemPitch = 4 * width;
            if (!data)
                dataRes.SysMemPitch = 0;

            HRESULT hr = mPrivate->mDev->CreateTexture2D(&desc, &dataRes, &mPrivate->mTexture);
            if (SUCCEEDED(hr))
            {
                D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
                D3D10_TEXTURE2D_DESC desc;
                mPrivate->mTexture->GetDesc(&desc);

                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = desc.Format;
                srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = desc.MipLevels;
                srvDesc.Texture2D.MostDetailedMip = 0;

                // create the shader resource view
                HRESULT hr = mPrivate->mDev->CreateShaderResourceView(mPrivate->mTexture, &srvDesc, &mPrivate->mShaderResourceView);
                if (FAILED(hr))
                {
                    CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_DX10, "DX10Surface::CreateShaderResourceView() failed. error = %x", hr));
                }
            }
            else
            {
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_DX10, "DX10Surface::CreateTexture2D() failed. error = %x", hr));
            }
        }

        static const QuadVertex vtx[] =  {
            { 0,    0,        1,    0.0f, 0.0f },
            { 1,    0,        1,    1.0f, 0.0f },
            { 0,    -1,        1,    0.0f, 1.0f },
            { 1,    -1,        1,    1.0f, 1.0f },
        };

        // we need to only create the vertex buffer once
        if (mPrivate->mVB == NULL)
        {
            D3D10_BUFFER_DESC bd;
            bd.Usage = D3D10_USAGE_DEFAULT;
            bd.ByteWidth = sizeof(vtx);
            bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = 0;
            bd.MiscFlags = 0;

            D3D10_SUBRESOURCE_DATA InitData;
            ZeroMemory(&InitData, sizeof(InitData));
            InitData.pSysMem = vtx;
            HRESULT hr = mPrivate->mDev->CreateBuffer(&bd, &InitData, &mPrivate->mVB);
            if (FAILED(hr))
            {
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_DX10, "DX10Surface::CreateBuffer() failed. error = %x", hr));
            }
        }
    }
}
#endif // ORIGIN_PC