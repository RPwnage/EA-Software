
#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_GEMINI)

#ifndef __SAMPLE_CORE_XBOXONE_H__
#define __SAMPLE_CORE_XBOXONE_H__


#include <wrl\client.h>
#include <d3d12_x.h>
#include <DirectXColors.h>
#include <xdk.h> // for _XDK_VER in OnActivated

using namespace Microsoft::WRL;
using namespace DirectX;

namespace NonInteractiveSamples
{

//--------------------------------------------------------------------------------------
// Name: SampleRenderer
// Desc: A class that holds the Rendering Functionality
// Note: Using DX11.1
//--------------------------------------------------------------------------------------
class SampleRenderer
{
    public:
        struct SimpleVertex
        {
            XMFLOAT3 Pos;
            XMFLOAT4 Color;
        };

        struct ConstantBuffer
        {
            XMMATRIX mWorld;
            XMMATRIX mView;
            XMMATRIX mProjection;
        };

    //Public Functions
    public:        
        void Initialize();
        void Render();
        void InitializePipeline();
        void InitializeGraphics();
    //Public Members
    public:
        ComPtr<ID3D11DeviceX> pD3DDevice;
        ComPtr<ID3D11DeviceContextX> pD3DDeviceContext;
        ComPtr<ID3DXboxPerformanceContext> pD3DXboxPerfContext;
        ComPtr<IDXGISwapChain1> pSwapChain;
        ComPtr<ID3D11RenderTargetView> pRenderTargetView;

        ComPtr<ID3D11Texture2D> pDepthStencil;
        ComPtr<ID3D11DepthStencilView> pDepthStencilView;
        ComPtr<ID3D11PixelShader> pPixelShader;
        ComPtr<ID3D11VertexShader> pVertexShader;
        ComPtr<ID3D11InputLayout> pVertexLayout;

        ComPtr<ID3D11Buffer> pVertexBuffer;
        ComPtr<ID3D11Buffer> pIndexBuffer;
        ComPtr<ID3D11Buffer> pConstantBuffer;

        XMMATRIX mWorld1;
        XMMATRIX mWorld2;
        XMMATRIX mView;
        XMMATRIX mProjection;

        UINT uHeight;
        UINT uWidth;
};



ref class XboxOneView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
internal:
    XboxOneView(Pyro::ProxyServerListener &proxyServerListener) : mProxyServerListener(proxyServerListener) {}

public:
    virtual ~XboxOneView() {}

    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
    virtual void Load(Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

protected:

    // PLM Event handlers
    void OnActivated( Windows::ApplicationModel::Core::CoreApplicationView^ applicationViewProvider, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args );
    void OnResuming( Platform::Object^ sender, Platform::Object^ args );
    void OnSuspending( Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args );
    void OnResourceAvailabilityChanged( Platform::Object^ sender, Platform::Object^ args );
    void OnVisibilityChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::VisibilityChangedEventArgs^ args );
    void OnWindowActivatedChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::WindowActivatedEventArgs^ args );

private:
    Pyro::ProxyServerListener &mProxyServerListener;
    SampleRenderer mRenderer;
};

ref class XboxOneViewFactory sealed : Windows::ApplicationModel::Core::IFrameworkViewSource 
{
internal:
    XboxOneViewFactory(Pyro::ProxyServerListener &proxyServerListener) : mProxyServerListener(proxyServerListener) {};

public:
    virtual ~XboxOneViewFactory() {};

    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();

private:
    Pyro::ProxyServerListener &mProxyServerListener;
};

}

#endif // __SAMPLE_CORE_XBOXONE_H__

#endif // EA_PLATFORM_XBOXONE