#ifdef EA_PLATFORM_NX

#ifndef __SAMPLE_CORE_NX_H__
#define __SAMPLE_CORE_NX_H__

namespace EA { namespace Pairing { class EAControllerUserPairingServer; } }
namespace Ignition
{
    extern EA::Pairing::EAControllerUserPairingServer *gpEacup;  // Extern Value so we can lookup user directly
}


#endif // __SAMPLE_CORE_NX_H__

#endif // EA_PLATFORM_NX
