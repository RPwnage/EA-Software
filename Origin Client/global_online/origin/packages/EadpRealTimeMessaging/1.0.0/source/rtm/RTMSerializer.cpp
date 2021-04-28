// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "RTMSerializer.h"
#include <eadp/foundation/LoggingService.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

constexpr int32_t kDelimiterSize = 4;

RTMSerializer::RTMSerializer(IHub* hub) 
: m_hub(hub)
{
}

eadp::foundation::ErrorPtr RTMSerializer::serializeWithDelimiter(eadp::foundation::SharedPtr<CommunicationWrapper> commWrapper, eadp::foundation::Internal::RawBuffer& buffer)
{
    // Each payload contains a 4 byte delimiter specifying the length of the communication data, followed by the communication data itself
    int32_t communicationSize = 0;
    if (commWrapper->hasRtmCommunication())
    {
        communicationSize = static_cast<int32_t>(commWrapper->getRtmCommunication()->getByteSize());
    }
    else
    {
        communicationSize = static_cast<int32_t>(commWrapper->getSocialCommunication()->getByteSize());
    }

    buffer.allocate(kDelimiterSize + communicationSize);
    buffer[0] = (uint8_t)((communicationSize & 0xff000000) >> 24);
    buffer[1] = (uint8_t)((communicationSize & 0x00ff0000) >> 16);
    buffer[2] = (uint8_t)((communicationSize & 0x0000ff00) >> 8);
    buffer[3] = (uint8_t)((communicationSize & 0x000000ff));

    if (commWrapper->hasRtmCommunication())
    {
        if (commWrapper->getRtmCommunication()->serialize(buffer.getBytes() + kDelimiterSize, communicationSize))
        {
            return nullptr;
        }
        else
        {
            return FoundationError::create(m_hub->getAllocator(), FoundationError::Code::SYSTEM_UNEXPECTED, "Unable to serialize request to buffer.");
        }
    }
    else
    {
        if (commWrapper->getSocialCommunication()->serialize(buffer.getBytes() + kDelimiterSize, communicationSize))
        {
            return nullptr;
        }
        else
        {
            return FoundationError::create(m_hub->getAllocator(), FoundationError::Code::SYSTEM_UNEXPECTED, "Unable to serialize request to buffer.");
        }
    }
}

eadp::foundation::ErrorPtr RTMSerializer::parseDataIntoCommunication(uint8_t* buffer, uint32_t length, eadp::foundation::SharedPtr<CommunicationWrapper>& commWrapper, int32_t& bytesRead)
{
    if (length < kDelimiterSize)
    {
        bytesRead = 0;
        return nullptr;
    }

    int32_t msgSize = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];

    int32_t payloadLength = msgSize + kDelimiterSize;
    if (length < static_cast<uint32_t>(payloadLength))
    {
        // The data does not contain the full serialized Communication - do not read any bytes, just wait until we receive more data
        bytesRead = 0;
        return nullptr;
    }

    bytesRead = payloadLength;

    // Deserialize Message
    uint8_t* msgStart = buffer + kDelimiterSize;
    SharedPtr<protocol::Communication> socialCommunication = m_hub->getAllocator().makeShared<protocol::Communication>(m_hub->getAllocator());

    if (socialCommunication->deserialize(msgStart, msgSize) && socialCommunication->getBodyCase() != protocol::Communication::BodyCase::kNotSet)
    {

        commWrapper = m_hub->getAllocator().makeShared<CommunicationWrapper>(socialCommunication);
    }
    else
    {
        SharedPtr<rtm::protocol::Communication> rtmCommunication = m_hub->getAllocator().makeShared<rtm::protocol::Communication>(m_hub->getAllocator());

        if (rtmCommunication->deserialize(msgStart, msgSize))
        {
            commWrapper = m_hub->getAllocator().makeShared<CommunicationWrapper>(rtmCommunication);
        }
        else
        {
            return FoundationError::create(m_hub->getAllocator(), FoundationError::Code::SYSTEM_UNEXPECTED, "Unable to decode incoming response.");
        }
    }
    return nullptr;
}
