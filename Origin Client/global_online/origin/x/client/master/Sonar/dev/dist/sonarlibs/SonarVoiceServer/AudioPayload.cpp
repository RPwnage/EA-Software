#include "AudioPayload.h"
#include <SonarVoiceServer/Server.h>

namespace sonar
{
    void* AudioPayload::operator new (size_t size)
    {
        return Server::getInstance().m_payloadAllocator.alloc();
    }

    void AudioPayload::operator delete (void *p)
    {
        Server::getInstance().m_payloadAllocator.free( (AudioPayload *) p);
    }
}
