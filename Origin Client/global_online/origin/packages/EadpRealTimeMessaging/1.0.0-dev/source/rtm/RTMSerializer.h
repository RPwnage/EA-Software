// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include "rtm/CommunicationWrapper.h"
#include <eadp/foundation/internal/Serialization.h>

namespace eadp
{
namespace realtimemessaging
{
class RTMSerializer
{
public:
    explicit RTMSerializer(eadp::foundation::IHub* hub);

    eadp::foundation::ErrorPtr serializeWithDelimiter(eadp::foundation::SharedPtr<CommunicationWrapper> commWrapper, eadp::foundation::Internal::RawBuffer& bufferLength);
    eadp::foundation::ErrorPtr parseDataIntoCommunication(uint8_t* buffer, uint32_t length, eadp::foundation::SharedPtr<CommunicationWrapper>& commWrapper, int32_t& bytesRead);

private:
    eadp::foundation::IHub* m_hub;
};
}
}
