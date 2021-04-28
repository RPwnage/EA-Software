#pragma once
#include <SonarConnection/Protocol.h>
#include <SonarCommon/Common.h>

#include <assert.h>
#include <string.h>
#include <cstddef>

namespace sonar
{

class Message
{
public:

    struct Buffer
    {
        size_t size;
        size_t offset;
        char data[protocol::MESSAGE_BUFFER_MAX_SIZE];
    };

    
    Message (const Message &message);
    Message (long long timestamp = 0);
    Message (int msg, int source, bool reliable, long long timestamp = 0, bool isClientOnlyJitterBufferAware = true);

    void reset();
    void reset (int msg, int source, bool reliable, bool isClientOnlyJitterBufferAware);

    const protocol::MessageHeader &getHeader(void) const;
    protocol::MessageHeader &getHeader(void);

    void *getBufferPtr(void);
    const void *getBufferPtr(void) const;
    size_t getMaxSize(void); 
    size_t getBufferSize() const;
    bool isReliable(void) const;

    long long timestamp() const { return m_timestamp; }
    void setTimestamp(long long ts) { m_timestamp = ts; }

    bool pullHeader(void);
    void updateSequence(protocol::SEQUENCE sequence);
    void writeHeader(protocol::PacketType type, int cmd, int source, bool isClientOnlyJitterBufferAware);
    unsigned long getSequence(void) const;

    void update(size_t bytes, const sockaddr_in &addr);

    void writeUnsafeByte(unsigned char);
    void writeUnsafeShort(unsigned short);
    void writeUnsafeLong(unsigned long);
    void writeUnsafeULongLong(unsigned long long);
    void writeUnsafeSmallBlob(const void *blob, size_t cbSize);
    void writeUnsafeSmallString(CString &str);
    void writeUnsafeBigString(CString &str);
    void writeUnsafe (const void *data, size_t cbData);
    char *writeUnsafeReserve(size_t cbSize);

    unsigned char readUnsafeByte (void);
    unsigned short readUnsafeShort(void);
    unsigned long readUnsafeLong (void);
    unsigned long long readUnsafeULongLong (void);
    CString readSmallString(void);
    CString readBigString(void);
    const char *readSmallBlob (size_t *outSize);
    const char *readUnsafe (size_t cbSize);

    bool readSafeByte (unsigned char & data);
    bool readSafeShort (unsigned short & data);
    bool readSafeLong (unsigned long & data);
    bool readSafeULongLong (unsigned long long & data);
    const char *readSafe (size_t cbSize);

    size_t getBytesLeft(void) const;

    const sockaddr_in &getAddress(void) const;

    CString& blockingInfo() { return m_blockingInfo; }
    void setBlockingInfo(CString& blockingInfo) { m_blockingInfo = blockingInfo; }

private:
    protocol::PacketType getType() const;
    void *getPayload(void) const;
    size_t getHeaderSize(void) const;
    void writeUnsafeBigBlob(const void *blob, size_t cbSize);

    const char *readBigBlob (size_t *outSize);

    int getCommand() const;

private:

    struct sockaddr_in m_addr;
    Buffer m_buffer;
    int m_source;
    String m_blockingInfo;
    long long m_timestamp;
    unsigned long* m_frameCounterSlot;
};

//

inline Message::Message (const Message &other)
    : m_source(other.m_source)
    , m_timestamp(other.m_timestamp)
{
    m_buffer.size = other.m_buffer.size;
    m_buffer.offset = other.m_buffer.offset;
    assert (m_buffer.offset <= m_buffer.size);
    memcpy(m_buffer.data, other.m_buffer.data, m_buffer.size);
    if (other.m_frameCounterSlot) {
        ptrdiff_t frameCounterOffset = reinterpret_cast<char const*>(other.m_frameCounterSlot) - reinterpret_cast<char const*>(&other);
        m_frameCounterSlot = reinterpret_cast<unsigned long*>(reinterpret_cast<char*>(this) + frameCounterOffset);
    } else {
        m_frameCounterSlot = NULL;
    }
}

inline Message::Message (long long timestamp)
    : m_source(0)
    , m_timestamp(timestamp)
    , m_frameCounterSlot(NULL)
{
    m_buffer.size = sizeof (m_buffer.data);
    m_buffer.offset = 0;
}

inline Message::Message (int msg, int source, bool reliable, long long timestamp, bool isClientOnlyJitterBufferAware)
    : m_timestamp(timestamp)
    , m_frameCounterSlot(NULL)
{
    reset(msg, source, reliable, isClientOnlyJitterBufferAware);
}

inline void Message::reset()
{
    m_buffer.size = sizeof (m_buffer.data);
    m_buffer.offset = 0;
    m_source = 0;
    m_frameCounterSlot = NULL;
}

inline void Message::reset(int msg, int source, bool reliable, bool isClientOnlyJitterBufferAware)
{
    m_buffer.size = sizeof (m_buffer.data);
    m_buffer.offset = 0;
    m_source = source;

    protocol::PacketType type = (reliable) ? protocol::PT_SEGREL : protocol::PT_SEGUNREL;
    writeHeader(type, msg, source, isClientOnlyJitterBufferAware);
}

inline void Message::writeUnsafeByte(unsigned char value)
{
    UINT8 val = value;
    writeUnsafe(&val, sizeof(val));
}

inline void Message::writeUnsafeShort(unsigned short value)
{
    UINT16 val = value;
    writeUnsafe(&val, sizeof(val));
}

inline void Message::writeUnsafeLong(unsigned long value)
{
    UINT32 val = value;
    writeUnsafe(&val, sizeof(val));
}

inline void Message::writeUnsafeULongLong(unsigned long long value)
{
    writeUnsafe(&value, sizeof(value));
}

inline void Message::writeUnsafeSmallBlob(const void *blob, size_t cbSize)
{
    assert(cbSize < 255);
    writeUnsafeByte( (unsigned char) cbSize);

    if (cbSize == 0)
        return;

    writeUnsafe(blob, cbSize);
}

inline void Message::writeUnsafeBigBlob(const void *blob, size_t cbSize)
{
    assert(cbSize < 65535);
    writeUnsafeShort( (unsigned short) cbSize);

    if (cbSize == 0)
        return;

    writeUnsafe(blob, cbSize);
}

inline void Message::writeUnsafeSmallString(CString &str)
{
    writeUnsafeSmallBlob(str.c_str(), str.size ());
}

inline void Message::writeUnsafeBigString(CString &str)
{
    writeUnsafeBigBlob(str.c_str(), str.size ());
}

inline const char *Message::readUnsafe (size_t cbSize)
{
    assert (m_buffer.offset + cbSize <= m_buffer.size);
    const char *ret = m_buffer.data + m_buffer.offset;
    m_buffer.offset += cbSize;
    return ret;
}

inline char *Message::writeUnsafeReserve(size_t cbSize)
{
    assert (m_buffer.offset + cbSize <= m_buffer.size);
    char *ret = m_buffer.data + m_buffer.offset;
    m_buffer.offset += cbSize;
    return ret;
}

inline void Message::writeUnsafe (const void *data, size_t cbData)
{
    char *dst = writeUnsafeReserve(cbData);

    if (cbData < 16)
    {
        switch (cbData)
        {
        case 1: *((UINT8 *)(dst)) = *((UINT8 *) data); break;
        case 2: *((UINT16 *)(dst)) = *((UINT16 *) data); break;
        case 4: *((UINT32 *)(dst)) = *((UINT32 *) data); break;
        case 8: *((UINT64 *)(dst)) = *((UINT64 *) data); break;

        default:
            {
                char *src = (char *) data;
                char *end = src + cbData;
                while (src < end)
                    (*dst++) = (*src++);
                break;
            }
        }
    }
    else
    {
        memcpy (dst, data, cbData);
    }
}

inline unsigned char Message::readUnsafeByte (void)
{
    return *((UINT8 *) readUnsafe(1));
}

inline unsigned short Message::readUnsafeShort(void) 
{
    return *((UINT16 *) readUnsafe(2));
}

inline unsigned long Message::readUnsafeLong (void) 
{
    return *((UINT32 *) readUnsafe(4));
}

inline unsigned long long Message::readUnsafeULongLong (void) 
{
    return *((UINT64 *) readUnsafe(8));
}

inline bool Message::readSafeByte (unsigned char & data)
{
    bool ok = false;
    const char * ptr = readSafe(1);
    if( ptr )
    {
        data = *((UINT8 *)ptr);
        ok = true;
    }

    return ok;
}

inline bool Message::readSafeShort (unsigned short & data)
{
    bool ok = false;
    const char * ptr = readSafe(2);
    if( ptr )
    {
        data = *((UINT16 *)ptr);
        ok = true;
    }

    return ok;
}

inline bool Message::readSafeLong (unsigned long & data)
{
    bool ok = false;
    const char * ptr = readSafe(4);
    if( ptr )
    {
        data = *((UINT32 *)ptr);
        ok = true;
    }

    return ok;
}

inline bool Message::readSafeULongLong (unsigned long long & data)
{
    bool ok = false;
    const char * ptr = readSafe(8);
    if( ptr )
    {
        data = *((UINT64 *)ptr);
        ok = true;
    }

    return ok;
}

inline const char *Message::readSafe (size_t cbSize)
{
    //assert (m_buffer.offset + cbSize <= m_buffer.size);
    if (m_buffer.offset + cbSize <= m_buffer.size)
    {
        const char *ret = m_buffer.data + m_buffer.offset;
        m_buffer.offset += cbSize;
        return ret;
    }

    return NULL;
}

inline const char *Message::readSmallBlob (size_t *outSize) 
{
    if (getBytesLeft() < 1)
    {
        if (outSize)
            *outSize = 0;
        return NULL;
    }

    size_t len = readUnsafeByte();

    if (getBytesLeft() < len || len == 0)
    {
        if (outSize)
            *outSize = 0;
        return NULL;
    }

    const char *ret = readUnsafe(len);
    *outSize = len;
    return ret;
}

inline CString Message::readSmallString (void)
{
    size_t size;
    const char *pstr = readSmallBlob(&size);

    if (pstr == NULL)
        return "";

    return String(pstr, size);
}

inline CString Message::readBigString(void)
{
    size_t size;
    const char *pstr = readBigBlob(&size);

    if (pstr == NULL)
        return "";

    return String(pstr, size);
}

inline const char *Message::readBigBlob (size_t *outSize) 
{
    if (getBytesLeft() < 2)
    {
        return NULL;
    }

    size_t len = readUnsafeShort();

    if (getBytesLeft() < len)
    {
        return NULL;
    }

    const char *ret = readUnsafe(len);
    *outSize = len;
    return ret;
}

inline bool Message::isReliable(void) const
{
    return getType() == protocol::PT_SEGREL;
}

inline int Message::getCommand() const
{
    return ((protocol::MessageHeader *) m_buffer.data)->cmd;
}

inline bool Message::pullHeader(void)
{
    if (getBytesLeft() < sizeof(protocol::MessageHeader))
    {
        return false;
    }

    readUnsafe(sizeof(protocol::MessageHeader));
    return true;
}

inline unsigned long Message::getSequence(void) const
{
    return getHeader().seq;
}

inline size_t Message::getBufferSize() const
{
    return m_buffer.offset;
}

inline size_t Message::getMaxSize(void)
{
    return m_buffer.size;
}

inline const protocol::MessageHeader &Message::getHeader(void) const
{
    return *( (protocol::MessageHeader *) &m_buffer.data[0]);
}

inline protocol::MessageHeader &Message::getHeader(void)
{
    return *( (protocol::MessageHeader *) &m_buffer.data[0]);
}

inline size_t Message::getBytesLeft(void) const
{
    return m_buffer.size - m_buffer.offset;
}

inline size_t Message::getHeaderSize() const
{
    size_t cbSize = sizeof(protocol::MessageHeader);

    if (getType() == protocol::PT_SEGUNREL)
    {
        cbSize -= sizeof(protocol::SEQUENCE);
    }

    return cbSize;
}

inline void *Message::getPayload() const
{
    return (void *) (&m_buffer.data[0] + getHeaderSize());
}

inline void *Message::getBufferPtr(void)
{
    return m_buffer.data;
}

inline const void *Message::getBufferPtr(void) const
{
    return m_buffer.data;
}


inline void Message::update(size_t bytes, const sockaddr_in &addr)
{
    m_buffer.size = bytes;
    m_addr = addr;
}

inline protocol::PacketType Message::getType() const
{
    return (protocol::PacketType) ((getHeader().flags >> protocol::PACKETTYPE_SHIFT) & protocol::PACKETTYPE_MASK);
}

inline void Message::writeHeader(protocol::PacketType type, int cmd, int source, bool isClientOnlyJitterBufferAware)
{
    protocol::MessageHeader &header = getHeader();

    header.cmd = (UINT8) cmd;
    header.source = (UINT16) source;
    header.flags = 0;
    header.flags |= ( ((int) type) & protocol::PACKETTYPE_MASK) << protocol::PACKETTYPE_SHIFT;

    m_buffer.offset += sizeof(protocol::MessageHeader);
    assert (m_buffer.offset <= m_buffer.size);
}

inline void Message::updateSequence(protocol::SEQUENCE sequence)
{
    getHeader().seq = sequence;
}

inline const sockaddr_in &Message::getAddress(void) const
{
    return m_addr;
}

}