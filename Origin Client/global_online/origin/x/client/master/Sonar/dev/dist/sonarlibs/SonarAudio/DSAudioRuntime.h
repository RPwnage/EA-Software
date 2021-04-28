#pragma once 

#include <Mmdeviceapi.h>

namespace sonar
{
    class DSAudioRuntime
    {
    public:

        GUID *lookupRealGuid (const GUID *_pDeviceGUID, GUID *_buffer);
    };
};