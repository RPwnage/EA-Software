// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/GroupMembershipChange.h>
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

namespace protocol
{

GroupMembershipChange::GroupMembershipChange(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.GroupMembershipChange"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_targetPersonaId(m_allocator_.emptyString())
, m_targetDisplayName(m_allocator_.emptyString())
, m_initiatorPersonaId(m_allocator_.emptyString())
, m_initiatorDisplayName(m_allocator_.emptyString())
, m_type(1)
, m_targetPlayer(nullptr)
, m_initiatingPLayer(nullptr)
{
}

void GroupMembershipChange::clear()
{
    m_cachedByteSize_ = 0;
    m_targetPersonaId.clear();
    m_targetDisplayName.clear();
    m_initiatorPersonaId.clear();
    m_initiatorDisplayName.clear();
    m_type = 1;
    m_targetPlayer.reset();
    m_initiatingPLayer.reset();
}

size_t GroupMembershipChange::getByteSize()
{
    size_t total_size = 0;
    if (!m_targetPersonaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_targetPersonaId);
    }
    if (!m_targetDisplayName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_targetDisplayName);
    }
    if (!m_initiatorPersonaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_initiatorPersonaId);
    }
    if (!m_initiatorDisplayName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_initiatorDisplayName);
    }
    if (m_type != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_type);
    }
    if (m_targetPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_targetPlayer);
    }
    if (m_initiatingPLayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_initiatingPLayer);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* GroupMembershipChange::onSerialize(uint8_t* target) const
{
    if (!getTargetPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getTargetPersonaId(), target);
    }
    if (!getTargetDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getTargetDisplayName(), target);
    }
    if (!getInitiatorPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getInitiatorPersonaId(), target);
    }
    if (!getInitiatorDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getInitiatorDisplayName(), target);
    }
    if (m_type != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(5, static_cast<int32_t>(getType()), target);
    }
    if (m_targetPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(6, *m_targetPlayer, target);
    }
    if (m_initiatingPLayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(7, *m_initiatingPLayer, target);
    }
    return target;
}

bool GroupMembershipChange::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (tag == 10)
                {
                    if (!input->readString(&m_targetPersonaId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_targetDisplayName;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_targetDisplayName:
                    if (!input->readString(&m_targetDisplayName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_initiatorPersonaId;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_initiatorPersonaId:
                    if (!input->readString(&m_initiatorPersonaId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_initiatorDisplayName;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_initiatorDisplayName:
                    if (!input->readString(&m_initiatorDisplayName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(40)) goto parse_type;
                break;
            }

            case 5:
            {
                if (tag == 40)
                {
                    parse_type:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setType(static_cast<com::ea::eadp::antelope::protocol::GroupMembershipChangeType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_targetPlayer;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_targetPlayer:
                    if (!input->readMessage(getTargetPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_initiatingPLayer;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_initiatingPLayer:
                    if (!input->readMessage(getInitiatingPLayer().get())) return false;
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
::eadp::foundation::String GroupMembershipChange::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_targetPersonaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  targetPersonaId: \"%s\",\n", indent.c_str(), m_targetPersonaId.c_str());
    }
    if (!m_targetDisplayName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  targetDisplayName: \"%s\",\n", indent.c_str(), m_targetDisplayName.c_str());
    }
    if (!m_initiatorPersonaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  initiatorPersonaId: \"%s\",\n", indent.c_str(), m_initiatorPersonaId.c_str());
    }
    if (!m_initiatorDisplayName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  initiatorDisplayName: \"%s\",\n", indent.c_str(), m_initiatorDisplayName.c_str());
    }
    if (m_type != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  type: %d,\n", indent.c_str(), m_type);
    }
    if (m_targetPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  targetPlayer: %s,\n", indent.c_str(), m_targetPlayer->toString(indent + "  ").c_str());
    }
    if (m_initiatingPLayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  initiatingPLayer: %s,\n", indent.c_str(), m_initiatingPLayer->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& GroupMembershipChange::getTargetPersonaId() const
{
    return m_targetPersonaId;
}

void GroupMembershipChange::setTargetPersonaId(const ::eadp::foundation::String& value)
{
    m_targetPersonaId = value;
}

void GroupMembershipChange::setTargetPersonaId(const char8_t* value)
{
    m_targetPersonaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& GroupMembershipChange::getTargetDisplayName() const
{
    return m_targetDisplayName;
}

void GroupMembershipChange::setTargetDisplayName(const ::eadp::foundation::String& value)
{
    m_targetDisplayName = value;
}

void GroupMembershipChange::setTargetDisplayName(const char8_t* value)
{
    m_targetDisplayName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& GroupMembershipChange::getInitiatorPersonaId() const
{
    return m_initiatorPersonaId;
}

void GroupMembershipChange::setInitiatorPersonaId(const ::eadp::foundation::String& value)
{
    m_initiatorPersonaId = value;
}

void GroupMembershipChange::setInitiatorPersonaId(const char8_t* value)
{
    m_initiatorPersonaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& GroupMembershipChange::getInitiatorDisplayName() const
{
    return m_initiatorDisplayName;
}

void GroupMembershipChange::setInitiatorDisplayName(const ::eadp::foundation::String& value)
{
    m_initiatorDisplayName = value;
}

void GroupMembershipChange::setInitiatorDisplayName(const char8_t* value)
{
    m_initiatorDisplayName = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::protocol::GroupMembershipChangeType GroupMembershipChange::getType() const
{
    return static_cast<com::ea::eadp::antelope::protocol::GroupMembershipChangeType>(m_type);
}
void GroupMembershipChange::setType(com::ea::eadp::antelope::protocol::GroupMembershipChangeType value)
{
    m_type = static_cast<int>(value);
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> GroupMembershipChange::getTargetPlayer()
{
    if (m_targetPlayer == nullptr)
    {
        m_targetPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::Player>(m_allocator_);
    }
    return m_targetPlayer;
}

void GroupMembershipChange::setTargetPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> val)
{
    m_targetPlayer = val;
}

bool GroupMembershipChange::hasTargetPlayer()
{
    return m_targetPlayer != nullptr;
}

void GroupMembershipChange::releaseTargetPlayer()
{
    m_targetPlayer.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> GroupMembershipChange::getInitiatingPLayer()
{
    if (m_initiatingPLayer == nullptr)
    {
        m_initiatingPLayer = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::Player>(m_allocator_);
    }
    return m_initiatingPLayer;
}

void GroupMembershipChange::setInitiatingPLayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> val)
{
    m_initiatingPLayer = val;
}

bool GroupMembershipChange::hasInitiatingPLayer()
{
    return m_initiatingPLayer != nullptr;
}

void GroupMembershipChange::releaseInitiatingPLayer()
{
    m_initiatingPLayer.reset();
}


}
}
}
}
}
