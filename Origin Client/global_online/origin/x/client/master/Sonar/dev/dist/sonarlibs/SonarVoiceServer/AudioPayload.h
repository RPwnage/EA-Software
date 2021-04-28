#pragma once

#include <SonarConnection/Protocol.h>
#include <assert.h>
#include <string.h>

namespace sonar
{

class AudioPayload
{
public:
    void* operator new (size_t size);
    void operator delete (void *p);

    AudioPayload (const void *payload, size_t cbSize, unsigned char take, uint32_t frameCounter, uint32_t sourceVersion);
    AudioPayload (unsigned char take);

    size_t getSize(void) const;
    const void *getPtr(void) const;
    bool isStopFrame(void) const;
    unsigned char getTake(void) const;
    UINT32 getFrameCounter(void) const;
    uint32_t getSourceVersion() const;

private:

    unsigned char m_take;
    bool m_stopFrame;
    char m_buffer[sonar::protocol::PAYLOAD_MAX_SIZE];
    size_t m_size;
    UINT32 m_frameCounter;
    uint32_t m_sourceVersion;
};

inline AudioPayload::AudioPayload (const void *payload, size_t cbSize, unsigned char take, UINT32 frameCounter, uint32_t sourceVersion)
    : m_take(take)
    , m_stopFrame(false)
    , m_size(cbSize)
    , m_frameCounter(frameCounter)
    , m_sourceVersion(sourceVersion)
{
    memcpy(m_buffer, payload, cbSize);
}

inline AudioPayload::AudioPayload (unsigned char take)
    : m_take(take)
    , m_stopFrame(true)
    , m_size(0)
    , m_sourceVersion(protocol::PROTOCOL_VERSION)
{
}

inline unsigned char AudioPayload::getTake(void) const
{
    return m_take;
}

inline size_t AudioPayload::getSize(void) const
{
    assert (!m_stopFrame);
    return m_size;
}

inline const void *AudioPayload::getPtr(void) const
{
    assert (!m_stopFrame);
    return m_buffer;
}

inline bool AudioPayload::isStopFrame(void) const
{
    return m_stopFrame;
}

inline UINT32 AudioPayload::getFrameCounter(void) const
{
    return m_frameCounter;
}

inline uint32_t AudioPayload::getSourceVersion() const {
    return m_sourceVersion;
}
}
