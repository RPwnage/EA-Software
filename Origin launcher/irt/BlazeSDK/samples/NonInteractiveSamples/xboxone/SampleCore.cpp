

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)


#include "PyroSDK/pyrosdk.h"

#include "xboxone/SampleCore.h"

#include "EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h"
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#include "EAControllerUserPairing/Messages.h"

#include <d3dcompiler_x.h>


using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Activation;


namespace NonInteractiveSamples
{
    class NonInteractiveSamplesAllocator
        : public EA::Allocator::ICoreAllocator
    {
    public:
        virtual void *Alloc(size_t size, const char *name, unsigned int flags) { return malloc(size); }
        virtual void *Alloc(size_t size, const char *name, unsigned int flags, unsigned int align, unsigned int alignoffset)  { return malloc(size); }
        virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags)  { return malloc(size); }
        virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags, unsigned int align, unsigned int alignOffset)  { return malloc(size); }
        virtual void Free(void *ptr, size_t) { free(ptr); }
    };

    class NonInteractiveSamplesIEAMessageHandler : public EA::Messaging::IHandler
    {
    public:
        NonInteractiveSamplesIEAMessageHandler()
        {
            NetPrintf(("SampleCore: [EACUP event handler] initializing\n"));
            memset(aLocalUsers, 0, sizeof(aLocalUsers));
        }

        bool HandleMessage(EA::Messaging::MessageId messageId, void* pMessage)
        {
            bool bSuccess = true;
            const EA::User::IEAUser *pLocalUser;
            EA::Pairing::UserMessage *pUserMessage;

            switch (messageId)
            {
                case EA::Pairing::E_USER_ADDED:
                    NetPrintf(("SampleCore: [EACUP event handler] E_USER_ADDED\n"));
                    pUserMessage = (EA::Pairing::UserMessage *)pMessage;
                    pLocalUser = pUserMessage->GetUser();
                    pLocalUser->AddRef();
                    bSuccess = AddLocalUser(pLocalUser);
                    break;

                case EA::Pairing::E_USER_REMOVED:
                    NetPrintf(("SampleCore: [EACUP event handler] E_USER_REMOVED\n"));
                    pUserMessage = (EA::Pairing::UserMessage *)pMessage;
                    pLocalUser = pUserMessage->GetUser();
                    bSuccess = RemoveLocalUser(pLocalUser);
                    pLocalUser->Release();
                    break;

                default:
                    NetPrintf(("SampleCore: [EACUP event handler] unsupported event (%d)\n", messageId));
                    bSuccess = false;
                    break;
            }

            return(bSuccess);
        }

        bool AddLocalUser(const EA::User::IEAUser *pLocalUser)
        {
            bool bSuccess = false;
            int32_t iLocalUserIndex;

            for (iLocalUserIndex = 0; iLocalUserIndex < iMaxLocalUsers; iLocalUserIndex++)
            {
                if (aLocalUsers[iLocalUserIndex] == nullptr)
                {
                    aLocalUsers[iLocalUserIndex] = pLocalUser;
                    if (NetConnAddLocalUser(iLocalUserIndex, pLocalUser) == 0)
                    {
                        bSuccess = true;
                    }
                    break;
                }
            }

            if (!bSuccess)
            {
                NetPrintf(("SampleCore: E_USER_ADDED failed\n"));
            }

            return(bSuccess);
        }

        bool RemoveLocalUser(const EA::User::IEAUser * pLocalUser)
        {
            bool bSuccess = false;
            int32_t iLocalUserIndex;

            for (iLocalUserIndex = 0; iLocalUserIndex < iMaxLocalUsers; iLocalUserIndex++)
            {
                if (aLocalUsers[iLocalUserIndex] == pLocalUser)
                {
                    if (NetConnRemoveLocalUser(iLocalUserIndex, pLocalUser) == 0)
                    {
                        bSuccess = true;
                    }
                    aLocalUsers[iLocalUserIndex] = nullptr;
                    break;
                }
            }

            if (!bSuccess)
            {
                NetPrintf(("SampleCore: E_USER_REMOVED failed\n"));
            }

            return(bSuccess);
        }

    private:
        static const int32_t iMaxLocalUsers = 16;
        const EA::User::IEAUser *aLocalUsers[iMaxLocalUsers];
    };
    EA::Pairing::EAControllerUserPairingServer *gpEacup;
    EA::SEMD::SystemEventMessageDispatcher *gpEasmd;
    NonInteractiveSamplesIEAMessageHandler *gpEAmsghdlr;
    NonInteractiveSamplesAllocator gsAllocator;

// Note: This code was derived from the Durango SimpleTriangle sample code.  Please see ViewProvider.cpp/.h for ref.

void XboxOneView::Initialize( Windows::ApplicationModel::Core::CoreApplicationView^ applicationView )
{
    // instantiate system event message dispatcher
    EA::SEMD::SystemEventMessageDispatcherSettings semdSettings;
    semdSettings.useCompanionHttpd = false;
    semdSettings.useCompanionUtil = false;
    semdSettings.useSystemService = true;
    semdSettings.useUserService = true;
    EA::SEMD::SystemEventMessageDispatcher::CreateInstance(semdSettings, &gsAllocator);

    // instantiate EACUP server
    gpEasmd = EA::SEMD::SystemEventMessageDispatcher::GetInstance();
    gpEacup = new EA::Pairing::EAControllerUserPairingServer(gpEasmd, &gsAllocator);
    gpEAmsghdlr = new NonInteractiveSamplesIEAMessageHandler();
    gpEacup->Init();
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED, false, 0);
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED, false, 0);
    EA::User::UserList eacupUserList = gpEacup->GetUsers();

    NetConnStartup("-servicename=ignition");

    // to be called after NetConnStartup()
    NetPrintf(("SampleCore: IFrameworkView::Initialize   tick = %d\n", NetTick()));

    // tell netconn about the first set of users
    EA::User::UserList::iterator i; 
    for(i = eacupUserList.begin(); i != eacupUserList.end(); i++)
    {
        const EA::User::IEAUser *pLocalUser = *i;
        pLocalUser->AddRef();
        gpEAmsghdlr->AddLocalUser(pLocalUser);
    }

    applicationView->Activated += ref new Windows::Foundation::TypedEventHandler< CoreApplicationView^, IActivatedEventArgs^ >( this, &XboxOneView::OnActivated );
    applicationView->Activated += ref new Windows::Foundation::TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>([](CoreApplicationView^, IActivatedEventArgs^ args) {
            Windows::UI::Core::CoreWindow::GetForCurrentThread()->Activate();
        });
    CoreApplication::Suspending += ref new Windows::Foundation::EventHandler< Windows::ApplicationModel::SuspendingEventArgs^ >( this, &XboxOneView::OnSuspending );
    CoreApplication::Resuming += ref new Windows::Foundation::EventHandler< Platform::Object^>( this, & XboxOneView::OnResuming ); 
}

void XboxOneView::OnActivated( CoreApplicationView^ applicationView, IActivatedEventArgs^ args )
{
    NetPrintf(("SampleCore: IFrameworkView::OnActivated   tick = %d\n", NetTick()));

#if defined(EA_PLATFORM_XBOXONE) && (_XDK_VER >= __XDK_VER_2015_MP) && 0
    if (args->Kind == Windows::ApplicationModel::Activation::ActivationKind::Protocol)
    {
        // parse the activation url
        IProtocolActivatedEventArgs^ proArgs = static_cast< IProtocolActivatedEventArgs^>(args);
        if (!NonInteractiveSamples::ProtocolActivationInfo::initProtocolActivation(proArgs))
        {
            NetPrintf(("SampleCore: XboxOneView::OnActivated unhandled/invalid protocol activation type for url %S.",
                (((proArgs != nullptr) && (proArgs->Uri != nullptr) && (proArgs->Uri->RawUri != nullptr))? proArgs->Uri->RawUri->Data() : L"")));
            return;
        }
        // process the activation if ready
        NonInteractiveSamples::ProtocolActivationInfo::checkProtocolActivation();
    }
    else
    {
        NetPrintf(("SampleCore: XboxOneView::OnActivated Activation kind %S unhandled", args->Kind.ToString()->Data()));
    }
#endif

    CoreWindow^ window = CoreWindow::GetForCurrentThread(); 

    window->VisibilityChanged += ref new Windows::Foundation::TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>( this, &XboxOneView::OnVisibilityChanged );
    window->Activated  += ref new Windows::Foundation::TypedEventHandler<CoreWindow^,WindowActivatedEventArgs^>( this, &XboxOneView::OnWindowActivatedChanged );

    window->Activate();
}

void XboxOneView::OnResuming( Platform::Object^ sender, Platform::Object^ args)
{
    NetPrintf(("SampleCore: IFrameworkView::OnResuming triggered   tick = %d\n", NetTick()));

    mRenderer.pD3DXboxPerfContext->Resume();
}

//--------------------------------------------------------------------------------------
// This event handler is called by when title window visibility is changed. 
//--------------------------------------------------------------------------------------
void XboxOneView::OnVisibilityChanged( CoreWindow^ window, VisibilityChangedEventArgs^ args )
{
    try
    {
        if( args->Visible )
        {
            NetPrintf(("SampleCore: IFrameworkView::OnVisibilityChanged - Window gained visibility   tick = %d\n", NetTick()));
        }
        else
        {
            NetPrintf(("SampleCore: IFrameworkView::OnVisibilityChanged - Window lost visibility   tick = %d\n", NetTick()));
        }
    }
    catch( Platform::Exception^ ex )
    {
        NetPrintf(("SampleCore: IFrameworkView::OnVisibilityChanged - Exception thrown by VisibilityChangedEventArgs::Visible; Error code = 0x%x, Message = %s\n", ex->HResult, ex->Message));
    }
}

//--------------------------------------------------------------------------------------
// This event handler is called by when title window focus state is changed. 
//--------------------------------------------------------------------------------------
void XboxOneView::OnWindowActivatedChanged( CoreWindow^ window, WindowActivatedEventArgs^ args )
{
    try
    {
        if( args->WindowActivationState == CoreWindowActivationState::CodeActivated )
        {
            NetPrintf(("SampleCore: IFrameworkView::OnWindowActivatedChanged - Window gained focus, CodeActivated   tick = %d\n", NetTick()));
        }
        else if ( args->WindowActivationState ==  CoreWindowActivationState::Deactivated )
        {
            NetPrintf(("SampleCore: IFrameworkView::OnWindowActivatedChanged - Window lost focus, Deactivated   tick = %d\n", NetTick()));
        }
        else if ( args->WindowActivationState ==  CoreWindowActivationState::PointerActivated )
        {
            NetPrintf(("SampleCore: IFrameworkView::OnWindowActivatedChanged - Window gained focus, PointerActivated   tick = %d\n", NetTick()));
        }
    }
    catch( Platform::Exception^ ex )
    {
        NetPrintf(("SampleCore: IFrameworkView::OnWindowActivatedChanged- Exception thrown by WindowActivatedEventArgs::WindowActivationState; Error code = 0x%x, Message = %s\n", ex->HResult, ex->Message));
    }

}

void XboxOneView::OnSuspending( Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args)
{
    NetPrintf(("SampleCore: IFrameworkView::Suspending   tick = %d\n", NetTick()));

    // calling the suspend API
    // omitting to call this will result in the sample being terminated because of an incomplete suspend.
    mRenderer.pD3DXboxPerfContext->Suspend(0);
}

void XboxOneView::OnResourceAvailabilityChanged( Platform::Object^ sender, Platform::Object^ args)
{
    if( Windows::ApplicationModel::Core::CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Full )
    {
        NetPrintf(("SampleCore: IFrameworkView::OnResourceAvailabilityChanged - entered the 'Full Running' state   tick = %d\n", NetTick()));
    }
    else
    {
        NetPrintf(("SampleCore: IFrameworkView::OnResourceAvailabilityChanged - entered the 'Constrained' state   tick = %d\n", NetTick()));
    }
}

void XboxOneView::SetWindow( Windows::UI::Core::CoreWindow^ window)
{
    NetPrintf(("SampleCore: IFrameworkView::SetWindow  tick = %d\n", NetTick()));
}

void XboxOneView::Load( Platform::String^ entryPoint)
{
    NetPrintf(("SampleCore: IFrameworkView::Load  tick = %d\n", NetTick()));
}

void XboxOneView::Run()
{
    // initialize Renderer
    mRenderer.Initialize();

    Pyro::PyroFramework pyro(&mProxyServerListener);
    pyro.startup(false);

    while (1)
    {
        Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);

        pyro.idleServer();

        gpEasmd->Tick();
        gpEacup->Tick();

        NetConnSleep(16);

        // render
        mRenderer.Render();
    }
}


void XboxOneView::Uninitialize()
{
    NetPrintf(("SampleCore: IFrameworkView::Uninitialize  tick = %d\n", NetTick()));

    // free EACUP server
    gpEacup->RemoveMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED);
    gpEacup->RemoveMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED);
    delete gpEAmsghdlr;
    gpEacup->Shutdown();
    delete gpEacup;
    gpEasmd->DestroyInstance();

    while (NetConnShutdown(0) == -1)
    {
        NetConnSleep(16);
    }
}



Windows::ApplicationModel::Core::IFrameworkView^ XboxOneViewFactory::CreateView()
{
    return ref new XboxOneView(mProxyServerListener);
}

}




/*F********************************************************************************/
/*!
    \Function SampleRenderer::Initialize

    \Description
        Initialize D3D Deivce and Swap Chain

    \Version 03/03/2014 (tcho)
*/
/********************************************************************************F*/
void NonInteractiveSamples::SampleRenderer::Initialize()
{
    //Get the current window handle
    CoreWindow^ pWindow = CoreWindow::GetForCurrentThread();
    uHeight = (UINT)pWindow->Bounds.Height;
    uWidth  = (UINT)pWindow->Bounds.Width;

    //Use to convert the back to DX11.1 versions
    ComPtr<ID3D11Device> pTempDevice;
    ComPtr<ID3D11DeviceContext> pTempDeviceContext;

    // Create the device and device context objects
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &pTempDevice, nullptr, &pTempDeviceContext);

    //Convert Device and Context back to DX11.1
    pTempDevice.As(&pD3DDevice);
    pTempDeviceContext.As(&pD3DDeviceContext);

    pD3DDeviceContext.As(&pD3DXboxPerfContext);

    //On Xone you need the dxgi factory to create the swap chain
    ComPtr<IDXGIDevice2> pDxgiDevice;
    ComPtr<IDXGIAdapter> pDxgiAdapter;
    ComPtr<IDXGIFactory2> pDxgiFactory;

    pD3DDevice.As(&pDxgiDevice);
    pDxgiDevice->GetAdapter(&pDxgiAdapter);
    pDxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &pDxgiFactory);

    //Setup swap chain description (came from MS samples)
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    memset(&swapChainDesc, 0, sizeof(swapChainDesc));
    swapChainDesc.Height = uHeight;
    swapChainDesc.Width = uWidth;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SampleDesc.Count = 1;

    //Create the Swap Chain
    pDxgiFactory->CreateSwapChainForCoreWindow(pD3DDevice.Get(), reinterpret_cast<IUnknown *>(pWindow), &swapChainDesc, nullptr, &pSwapChain);

    //Get Point to Back Buffer 
    ComPtr<ID3D11Texture2D> pBackbuffer;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackbuffer);

    // create a render target pointing to the back buffer
    pD3DDevice->CreateRenderTargetView(pBackbuffer.Get(), nullptr, &pRenderTargetView);

    //Create a ViewPort
     D3D11_VIEWPORT viewPort;
     memset(&viewPort, 0, sizeof(viewPort));
     viewPort.TopLeftX = 0;
     viewPort.TopLeftY = 0;
     viewPort.MinDepth = 0.0f;
     viewPort.MaxDepth = 1.0f;
     viewPort.Height = pWindow->Bounds.Height;
     viewPort.Width = pWindow->Bounds.Width;

     pD3DDeviceContext->RSSetViewports(1, &viewPort);

     //Animation
     InitializePipeline();
     InitializeGraphics();
}

/*F********************************************************************************/
/*!
    \Function SampleRenderer::InitializePipeline

    \Description
        Initialize Vertex and Pixel Shaders

    \Version 03/03/2014 (tcho)
*/
/********************************************************************************F*/
void NonInteractiveSamples::SampleRenderer::InitializePipeline()
{
    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC depthStencilDesc;
    memset(&depthStencilDesc, 0, sizeof(depthStencilDesc));
    depthStencilDesc.Width = uWidth;
    depthStencilDesc.Height = uHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;
    
    pD3DDevice->CreateTexture2D(&depthStencilDesc, nullptr, &pDepthStencil);

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    memset(&depthStencilViewDesc, 0, sizeof(depthStencilViewDesc));
    depthStencilViewDesc.Format = depthStencilDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    pD3DDevice->CreateDepthStencilView(pDepthStencil.Get(), &depthStencilViewDesc, &pDepthStencilView);

    //Compile and Create pixel shader
    ComPtr<ID3DBlob> pPixelShaderBlob;
    D3DCompileFromFile(L"Tutorial05_PS.hlsl", nullptr, nullptr, "PS", "ps_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pPixelShaderBlob, nullptr);
    pD3DDevice->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr, &pPixelShader);

    //Compile and Create vertex shader
    ComPtr<ID3DBlob> pVertexShaderBlob;
    D3DCompileFromFile(L"Tutorial05_VS.hlsl", nullptr, nullptr, "VS", "vs_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVertexShaderBlob, nullptr);
    pD3DDevice->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), nullptr, &pVertexShader);

    //Define and Create Input Layout
    D3D11_INPUT_ELEMENT_DESC aLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT uElements = ARRAYSIZE(aLayout);
    pD3DDevice->CreateInputLayout(aLayout, uElements, pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), &pVertexLayout);
}

/*F********************************************************************************/
/*!
    \Function SampleRenderer::InitializeGraphics

    \Description
        Initialize Cube

    \Version 03/03/2014 (tcho)
*/
/********************************************************************************F*/
void NonInteractiveSamples::SampleRenderer::InitializeGraphics()
{
     // Create vertex buffer
    SimpleVertex vertices[] =
    {
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 1.0f, 0.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT4( 0.0f, 1.0f, 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f ) },
    };

    D3D11_BUFFER_DESC bufferDesc;
    memset(&bufferDesc, 0, sizeof(bufferDesc));
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof( SimpleVertex ) * 8;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData;
    memset(&initData, 0, sizeof(initData));
    initData.pSysMem = vertices;

    pD3DDevice->CreateBuffer(&bufferDesc, &initData, &pVertexBuffer);

    //Create Index Buffer (cube)
    WORD indices[] =
    {
        3,1,0,
        2,1,3,

        0,5,4,
        1,5,0,

        3,4,7,
        0,4,3,

        1,6,5,
        2,6,1,

        2,7,6,
        3,7,2,

        6,4,5,
        7,4,6,
    };

    //Reuse buffer descriptor
    memset(&bufferDesc, 0, sizeof(bufferDesc));
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof( WORD ) * 36;        // 36 vertices needed for 12 triangles in a triangle list
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    initData.pSysMem = indices;

    pD3DDevice->CreateBuffer(&bufferDesc, &initData, &pIndexBuffer);

    //Set Index Buffer and primitive topology
    pD3DDeviceContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Create Constant Buffer
     memset(&bufferDesc, 0, sizeof(bufferDesc));
     bufferDesc.Usage = D3D11_USAGE_DEFAULT;
     bufferDesc.ByteWidth = sizeof(ConstantBuffer);
     bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
     bufferDesc.CPUAccessFlags = 0;

     pD3DDevice->CreateBuffer(&bufferDesc, nullptr, &pConstantBuffer);

     //Initalize world matrix
     mWorld1 = XMMatrixIdentity();
     mWorld2 = XMMatrixIdentity();

     //Initialize View
     XMVECTOR Eye = XMVectorSet( 0.0f, 1.0f, -6.0f, 0.0f );
     XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
     XMVECTOR Up = XMVectorSet( 0.0f, -0.2f, 0.0f, 0.0f );
     mView = XMMatrixLookAtLH( Eye, At, Up );

     //Initialize Projection
     mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV2, uWidth/(FLOAT)uHeight, 0.01f, 100.0f);
}

/*F********************************************************************************/
/*!
    \Function SampleRenderer::Render

    \Description
        Renders a single frame

    \Version 03/03/2014 (tcho)
*/
/********************************************************************************F*/
void NonInteractiveSamples::SampleRenderer::Render()
{
    //Prep Deivce Context for Rendering 
    pD3DDeviceContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressOf(), pDepthStencilView.Get());
    pD3DDeviceContext->IASetInputLayout(pVertexLayout.Get());

    UINT uStride = sizeof(SimpleVertex);
    UINT uOffset = 0;
    pD3DDeviceContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &uStride , &uOffset);

    pD3DDeviceContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    //Update time
    static float t = 0.0f;
    static ULONGLONG uTimeStart = 0;
    ULONGLONG uTimeCurrent = GetTickCount64();

    if ( uTimeStart == 0)
    {
        uTimeStart = uTimeCurrent;
    }

    t = (uTimeCurrent - uTimeStart)/1000.0f;

    //Rotate 1st cube around origin
    mWorld1 = XMMatrixRotationY(t);

    //Rotate 2nd cube around origin and rotate it around the first cube.
    XMMATRIX mSpin = XMMatrixRotationZ(-t);
    XMMATRIX mOrbit = XMMatrixRotationY(-t * 2.0f);
    XMMATRIX mTranslate = XMMatrixTranslation(-4.0f, 0.0f, 0.0f);
    XMMATRIX mScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);

    mWorld2 = mScale * mSpin * mTranslate * mOrbit;

    // Clear Background (Set Background Color)
    float color[4] = {0.0f, 0.8f, 0.4f, 1.0f};
    pD3DDeviceContext->ClearRenderTargetView(pRenderTargetView.Get(), color);

    //Clear depth buffer
    pD3DDeviceContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    //Setup Rendering Pipeline
    pD3DDeviceContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
    pD3DDeviceContext->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());
    pD3DDeviceContext->PSSetShader(pPixelShader.Get(), nullptr, 0);

    //Setup Variables for the 1st Cube
    ConstantBuffer constantBuffer1;
    constantBuffer1.mWorld = XMMatrixTranspose(mWorld1);
    constantBuffer1.mView = XMMatrixTranspose(mView);
    constantBuffer1.mProjection = XMMatrixTranspose(mProjection);
    pD3DDeviceContext->UpdateSubresource(pConstantBuffer.Get(), 0, nullptr, &constantBuffer1, 0, 0);

    //Render 1st cube
    pD3DDeviceContext->DrawIndexed(36, 0, 0);

    //Setup Variables for the 2nd Cube
    ConstantBuffer constantBuffer2;
    constantBuffer2.mWorld = XMMatrixTranspose(mWorld2);
    constantBuffer2.mView = XMMatrixTranspose(mView);
    constantBuffer2.mProjection = XMMatrixTranspose(mProjection);
    pD3DDeviceContext->UpdateSubresource(pConstantBuffer.Get(), 0, nullptr, &constantBuffer2, 0, 0);

    //Render 2nd cube
    pD3DDeviceContext->DrawIndexed(36, 0, 0);

    //Present
    pSwapChain->Present(1, 0);

}



#endif // EA_PLATFORM_XBOXONE