// GENERATED CODE (Source: rtm_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/UserType.h>

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

class EADPGENERATED_API LoginRequestV2 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit LoginRequestV2(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    const ::eadp::foundation::String& getToken() const;
    void setToken(const ::eadp::foundation::String& value);
    void setToken(const char8_t* value);
    /**@}*/

    /**@{
     */
    bool getReconnect() const;
    void setReconnect(bool value);
    /**@}*/

    /**@{
     */
    bool getHeartbeat() const;
    void setHeartbeat(bool value);
    /**@}*/

    /**@{
     */
    com::ea::eadp::antelope::rtm::protocol::UserType getUserType() const;
    void setUserType(com::ea::eadp::antelope::rtm::protocol::UserType value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getProductId() const;
    void setProductId(const ::eadp::foundation::String& value);
    void setProductId(const char8_t* value);
    /**@}*/

    /**@{
     */
    bool getSingleSessionForceLogout() const;
    void setSingleSessionForceLogout(bool value);
    /**@}*/

    /*!
     * Serializes the message contents into a buffer.
    Called by serialize.
     */
    uint8_t* onSerialize(uint8_t* target) const override;

    /*!
     * Deserializes the message contents from an input stream.
    Called by deserialize.
     */
    bool onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input) override;

    /*!
     * Returns a previously calculated serialized size of the message in protobuf wire format.
     */
    size_t getCachedSize() const override
    {
        return m_cachedByteSize_;
    }

    /*!
     * Calculates and returns the serialized size of the message in protobuf wire format.
     */
    size_t getByteSize() override;

    /*!
     * Returns a string representing the contents of the message for debugging and logging.
     */
    ::eadp::foundation::String toString(const ::eadp::foundation::String& indent) const override;

private:
    ::eadp::foundation::Allocator m_allocator_;
    size_t m_cachedByteSize_;
    ::eadp::foundation::String m_token;
    bool m_reconnect;
    bool m_heartbeat;
    int m_userType;
    ::eadp::foundation::String m_productId;
    bool m_singleSessionForceLogout;

};

}
}
}
}
}
}
