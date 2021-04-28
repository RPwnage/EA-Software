// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/MutedSetV1.h>
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

MutedSetV1::MutedSetV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.MutedSetV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_mutedBy(m_allocator_.make<::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::MutedByV1>>())
{
}

void MutedSetV1::clear()
{
    m_cachedByteSize_ = 0;
    m_mutedBy.clear();
}

size_t MutedSetV1::getByteSize()
{
    size_t total_size = 0;
    {
        int dataSize = 0;
        for (EADPSDK_SIZE_T i = 0; i < m_mutedBy.size(); i++)
        {
            dataSize += ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(static_cast<int32_t>(m_mutedBy[i]));
        }
        if (dataSize > 0)
        {
            total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(dataSize);
        }
        m_mutedBy__cachedByteSize = dataSize;
        total_size += dataSize;
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* MutedSetV1::onSerialize(uint8_t* target) const
{
    if (m_mutedBy.size() > 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeTag(1, ::eadp::foundation::Internal::ProtobufWireFormat::WireType::LENGTH_DELIMITED, target);
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(m_mutedBy__cachedByteSize, target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_mutedBy.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(static_cast<int32_t>(m_mutedBy[i]), target);
    }
    return target;
}

bool MutedSetV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    parse_mutedBy:
                    int elem;
                    if (!input->readEnum(&elem))
                    {
                        return false;
                    }
                    m_mutedBy.push_back(static_cast<com::ea::eadp::antelope::rtm::protocol::MutedByV1>(elem));
                } else if (tag == 10) {
                    if (!input->readPackedEnum<com::ea::eadp::antelope::rtm::protocol::MutedByV1>(&m_mutedBy)){
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(8)) goto parse_mutedBy;
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
::eadp::foundation::String MutedSetV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_mutedBy.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedBy: [\n", indent.c_str());
        for (auto iter = m_mutedBy.begin(); iter != m_mutedBy.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %d,\n", indent.c_str(), *iter);
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::MutedByV1>& MutedSetV1::getMutedBy()
{
    return m_mutedBy;
}

}
}
}
}
}
}
