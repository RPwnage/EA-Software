// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/StickyMessageChangedV1.h>
#include <eadp/foundation/internal/ProtobufWireFormat.h>
#include <eadp/foundation/internal/Utils.h>
#include <eadp/foundation/Hub.h>

namespace com
{

namespace ea
{

namespace eadp
{

namespace antelope
{

namespace rtm
{

namespace protocol
{

StickyMessageChangedV1::StickyMessageChangedV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.StickyMessageChangedV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_stickyMessageChangeType(1)
, m_stickyMessage(nullptr)
, m_index(0)
{
}

void StickyMessageChangedV1::clear()
{
    m_cachedByteSize_ = 0;
    m_stickyMessageChangeType = 1;
    m_stickyMessage.reset();
    m_index = 0;
}

size_t StickyMessageChangedV1::getByteSize()
{
    size_t total_size = 0;
    if (m_stickyMessageChangeType != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_stickyMessageChangeType);
    }
    if (m_stickyMessage != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_stickyMessage);
    }
    if (m_index != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_index);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* StickyMessageChangedV1::onSerialize(uint8_t* target) const
{
    if (m_stickyMessageChangeType != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(1, static_cast<int32_t>(getStickyMessageChangeType()), target);
    }
    if (m_stickyMessage != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_stickyMessage, target);
    }
    if (getIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getIndex(), target);
    }
    return target;
}

bool StickyMessageChangedV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
{
    clear();
    uint32_t tag;
    for (;;)
    {
        tag = input->readTag();
        if (tag > 127) goto handle_unusual;
        switch (::eadp::foundation::Internal::ProtobufWireFormat::getTagFieldNumber(tag))
        {
            case 1:
            {
                if (tag == 8)
                {
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setStickyMessageChangeType(static_cast<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangeType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_stickyMessage;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_stickyMessage:
                    if (!input->readMessage(getStickyMessage().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_index;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_index:
                    if (!input->readInt32(&m_index))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectAtEnd()) return true;
                break;
            }

            default: {
            handle_unusual:
                if (tag == 0)
                {
                    return true;
                }
                if(!input->skipField(tag)) return false;
                break;
            }
        }
    }
}
::eadp::foundation::String StickyMessageChangedV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_stickyMessageChangeType != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  stickyMessageChangeType: %d,\n", indent.c_str(), m_stickyMessageChangeType);
    }
    if (m_stickyMessage != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  stickyMessage: %s,\n", indent.c_str(), m_stickyMessage->toString(indent + "  ").c_str());
    }
    if (m_index != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  index: %d,\n", indent.c_str(), m_index);
    }
    result.append(indent);
    result.append("}");
    return result;
}

com::ea::eadp::antelope::rtm::protocol::StickyMessageChangeType StickyMessageChangedV1::getStickyMessageChangeType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangeType>(m_stickyMessageChangeType);
}
void StickyMessageChangedV1::setStickyMessageChangeType(com::ea::eadp::antelope::rtm::protocol::StickyMessageChangeType value)
{
    m_stickyMessageChangeType = static_cast<int>(value);
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageV1> StickyMessageChangedV1::getStickyMessage()
{
    if (m_stickyMessage == nullptr)
    {
        m_stickyMessage = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::StickyMessageV1>(m_allocator_);
    }
    return m_stickyMessage;
}

void StickyMessageChangedV1::setStickyMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageV1> val)
{
    m_stickyMessage = val;
}

bool StickyMessageChangedV1::hasStickyMessage()
{
    return m_stickyMessage != nullptr;
}

void StickyMessageChangedV1::releaseStickyMessage()
{
    m_stickyMessage.reset();
}


int32_t StickyMessageChangedV1::getIndex() const
{
    return m_index;
}

void StickyMessageChangedV1::setIndex(int32_t value)
{
    m_index = value;
}


}
}
}
}
}
}
