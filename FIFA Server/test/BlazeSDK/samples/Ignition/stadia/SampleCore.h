#ifdef EA_PLATFORM_STADIA

#ifndef __SAMPLE_CORE_STADIA_H__
#define __SAMPLE_CORE_STADIA_H__

namespace Pyro
{
    class ProxyServerListener;
}

namespace EA { namespace Pairing { class EAControllerUserPairingServer; } } 

namespace Ignition
{
    void Initialize();
    void Run(Pyro::ProxyServerListener *pProxyServerListener);
    void Uninitialize();

    extern EA::Pairing::EAControllerUserPairingServer *gpEacup;  // Extern Value so we can lookup user directly
}

#endif
#endif