
#if defined(EA_PLATFORM_XBOX_GDK)

#ifndef __SAMPLE_CORE_GDK_H__
#define __SAMPLE_CORE_GDK_H__

#include "IEAController/IEAController.h"
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#include "EAControllerUserPairing/Messages.h"

#include "samplecode/DeviceResources.h"
#include "samplecode/StepTimer.h"

namespace EA { namespace Pairing { class EAControllerUserPairingServer; } }

namespace Ignition
{
    extern EA::Pairing::EAControllerUserPairingServer *gpEacup;  // Extern Value so we can lookup user directly

    struct Vertex
    {
        XMFLOAT3 pos;
        XMFLOAT3 normal;
    };

    class IgnitionIEAMessageHandler : public EA::Messaging::IHandler
    {
    public:
        IgnitionIEAMessageHandler()
        {
            NetPrintf(("SampleCore: [EACUP event handler] initializing\n"));
            memset(aLocalUsers, 0, sizeof(aLocalUsers));
        }

        bool HandleMessage(EA::Messaging::MessageId messageId, void* pMessage) override
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

    //--------------------------------------------------------------------------------------
    // Name: Sample
    // Desc: A basic sample implementation that creates a D3D12 device and
    //       provides the update & render loop
    // Note: Using DX12, taken from Microsoft provided samples
    //--------------------------------------------------------------------------------------

    class Sample
    {
    public:

        Sample(Pyro::ProxyServerListener &proxyServerListener);
        ~Sample();

        // Initialization and management
        bool Initialize(HINSTANCE inst, int32_t iShow);
        void Run();

        // Messages
        void OnSuspending();
        void OnResuming();

        void Uninitialize();

    private:

        struct ConstantBuffer
        {
            DirectX::XMMATRIX WorldMatrix;
            DirectX::XMMATRIX ViewMatrix;
            DirectX::XMMATRIX ProjectionMatrix;
            DirectX::XMVECTOR aLightDir[2];
            DirectX::XMVECTOR aLightColor[2];
            DirectX::XMVECTOR OutputColorT;
        };

        // We'll allocate space for several of these and they will need to be padded for alignment.
        static_assert(sizeof(ConstantBuffer) == 272, "Checking the size here.");

        // D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT < 272 < 2 * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
        // Create a union with the correct size and enough room for one ConstantBuffer
        union PaddedConstantBuffer
        {
            ConstantBuffer Constants;
            uint8_t aBytes[2 * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
        };

        // Check the exact size of the PaddedConstantBuffer to make sure it will align properly
        static_assert(sizeof(PaddedConstantBuffer) == 2 * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, "PaddedConstantBuffer is not aligned properly");

        void Sample::Update(const DX::StepTimer &timer);
        void Render();

        // Basic render loop
        void Tick();

        void Clear();

        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();

        // Device resources.
        std::unique_ptr<DX::DeviceResources>        pDeviceResources;

        // Rendering loop timer.
        uint64_t                                    uFrame;
        DX::StepTimer                               m_timer;

        // DirectXTK objects.
        Microsoft::WRL::ComPtr<ID3D12RootSignature>  cpRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>  cpLambertPipelineState;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>  cpSolidColorPipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource>       cpVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>       cpIndexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>       cpPerFrameConstants;
        PaddedConstantBuffer*                        pMappedConstantData;
        D3D12_GPU_VIRTUAL_ADDRESS                    ConstantDataGpuAddrT;
        D3D12_VERTEX_BUFFER_VIEW                     VertexBufferView;
        D3D12_INDEX_BUFFER_VIEW                      IndexBufferView;

        // In this simple sample, we know that there are three draw calls
        // and we will update the scene constants for each draw call.
        static const uint32_t                        uNumDrawCalls = 3;

        // A synchronization fence and an event. These members will be used
        // to synchronize the CPU with the GPU so that there will be no
        // contention for the constant buffers. 
        Microsoft::WRL::ComPtr<ID3D12Fence>          cpFence;
        Microsoft::WRL::Wrappers::Event              m_fenceEvent;

        // Index in the root parameter table
        static const uint32_t                        uRootParameterCB = 0;

        // Scene constants, updated per-frame
        float                                        fCurRotationAngleRad;

        // These computed values will be loaded into a ConstantBuffer
        // during Render
        DirectX::XMFLOAT4X4                          WorldMatrix;
        DirectX::XMFLOAT4X4                          ViewMatrix;
        DirectX::XMFLOAT4X4                          ProjectionMatrix;
        DirectX::XMFLOAT4                            aLightDirs[2];
        DirectX::XMFLOAT4                            aLightColors[2];
        DirectX::XMFLOAT4                            OutputColor;

        Pyro::ProxyServerListener                    &mProxyServerListener;
    };
}

#endif // __SAMPLE_CORE_GDK_H__

#endif // EA_PLATFORM_XBOX_GDK
