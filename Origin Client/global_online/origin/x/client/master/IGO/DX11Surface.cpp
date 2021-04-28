///////////////////////////////////////////////////////////////////////////////
// DX11Surface.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "DX11Surface.h"

#if defined(ORIGIN_PC)

#include "IGOIPC/IGOIPC.h"

#include "IGOWindow.h"
#include "IGOApplication.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

namespace OriginIGO {

    extern D3DMATRIX gMatOrtho;
    extern D3DMATRIX gMatWorld;
    
    extern ID3D11PixelShader *gPixelShader;
    extern ID3D11PixelShader *gBackgroundPixelShader;
    extern ID3D11VertexShader *gVertexShader;
    extern ID3D11VertexShader *gBackgroundVertexShader;
    extern ID3D11Buffer *gCBChangesEveryFrame;
    extern ID3D11Buffer *gCBNeverChanges;

    struct QuadVertex
    {
        float x, y, z;            //!< The transformed position for the vertex.
        float tu1, tv1;            //!< texture coordinates
    };

    class DX11SurfacePrivate
    {
    public:
        DX11SurfacePrivate() { }
        ~DX11SurfacePrivate() { }

        ID3D11Device *mDev;
        ID3D11Texture2D *mTexture;
        ID3D11Buffer *mVB;
        ID3D11ShaderResourceView *mShaderResourceView;
    };

    DX11Surface::DX11Surface(void *pDev, IGOWindow *pWindow) :
        mWindow(pWindow),
        mWidth(0), mHeight(0)
    {
        mPrivate = new DX11SurfacePrivate();
        mPrivate->mDev = (ID3D11Device *)pDev;
        mPrivate->mTexture = NULL;
        mPrivate->mVB = NULL;
        mPrivate->mShaderResourceView = NULL;
    }

    DX11Surface::~DX11Surface()
    {
        // don't delete the device
        RELEASE_IF(mPrivate->mShaderResourceView);
        RELEASE_IF(mPrivate->mTexture);
        RELEASE_IF(mPrivate->mVB);
        delete mPrivate;
    }

	void DX11Surface::render(void *pDevice)
    {
        ID3D11Device *pDev = (ID3D11Device *)pDevice;
        ID3D11DeviceContext *pDevContext = NULL;

        pDev->GetImmediateContext(&pDevContext);

        if (mPrivate && mPrivate->mVB && mPrivate->mShaderResourceView && pDevContext && gCBChangesEveryFrame && gCBNeverChanges)
        {
            if (mWindow->id() == BACKGROUND_WINDOW_ID)
            {
                pDevContext->VSSetShader(gBackgroundVertexShader, NULL, 0);
                pDevContext->PSSetShader(gBackgroundPixelShader, NULL, 0);
            }
            else
            {
                pDevContext->VSSetShader(gVertexShader, NULL, 0);
                pDevContext->PSSetShader(gPixelShader, NULL, 0);
            }

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
            gMatWorld._44 = 1.0f;

            // translate
            gMatWorld._14 = X;
            gMatWorld._24 = Y;

            DX11CBNeverChanges cbNeverChanges;
            cbNeverChanges.mViewProj = gMatOrtho;
            pDevContext->UpdateSubresource(gCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0);
            
            // set pos and color
            DX11CBChangesEveryFrame cb;
            cb.mWorld = gMatWorld;
            cb.mColor[0] = mWindow->customValue() / 10000.0f;
            cb.mColor[1] = cb.mColor[2] = 1.0f;
            cb.mColor[3] = mWindow->alpha()/255.0f;
            pDevContext->UpdateSubresource(gCBChangesEveryFrame, 0, NULL, &cb, 0, 0);

            // set vertex buffer
            UINT stride = sizeof(QuadVertex);
            UINT offset = 0;
            pDevContext->IASetVertexBuffers(0, 1, &mPrivate->mVB, &stride, &offset);

            // set the texture
            pDevContext->PSSetShaderResources(0, 1, &mPrivate->mShaderResourceView);

            // draw the quad
            pDevContext->Draw(4, 0);
        }

        RELEASE_IF(pDevContext)
    }

	void DX11Surface::update(void *device, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
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

            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.SampleDesc.Count = 1;
            desc.Format = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isSRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA dataRes;
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
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
                D3D11_TEXTURE2D_DESC desc;
                mPrivate->mTexture->GetDesc(&desc);

                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = desc.Format;
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = desc.MipLevels;
                srvDesc.Texture2D.MostDetailedMip = 0;

                // create the shader resource view
                HRESULT hr = mPrivate->mDev->CreateShaderResourceView(mPrivate->mTexture, &srvDesc, &mPrivate->mShaderResourceView);
                if (FAILED(hr))
                {
                    CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_DX11, "DX11Surface::CreateShaderResourceView() failed. error = %x", hr));
                }
            }
            else
            {
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_DX11, "DX11Surface::CreateTexture2D() failed. error = %x", hr));
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
            D3D11_BUFFER_DESC bd;
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.ByteWidth = sizeof(vtx);
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = 0;
            bd.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA InitData;
            ZeroMemory(&InitData, sizeof(InitData));
            InitData.pSysMem = vtx;
            HRESULT hr = mPrivate->mDev->CreateBuffer(&bd, &InitData, &mPrivate->mVB);
            if (FAILED(hr))
            {
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_DX11, "DX11Surface::CreateBuffer() failed.error = %x", hr));
            }
        }
    }
}
#endif // ORIGIN_PC
