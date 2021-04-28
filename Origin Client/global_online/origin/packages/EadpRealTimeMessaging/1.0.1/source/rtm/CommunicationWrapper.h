// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <com/ea/eadp/antelope/protocol/Communication.h>
#include <com/ea/eadp/antelope/rtm/protocol/Communication.h>

namespace eadp
{
namespace realtimemessaging
{
class CommunicationWrapper
{
public:
    CommunicationWrapper(eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Communication> socialCommunication) :
        m_socialCommunication(socialCommunication) {}
    CommunicationWrapper(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Communication> rtmCommunication) :
        m_rtmCommunication(rtmCommunication) {};

    bool hasSocialCommunication() const { return m_socialCommunication != nullptr; }
    eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Communication> getSocialCommunication() { return m_socialCommunication; }

    bool hasRtmCommunication() const { return m_rtmCommunication != nullptr; }
    eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Communication> getRtmCommunication() { return m_rtmCommunication; }

private:
    eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Communication> m_socialCommunication;
    eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Communication> m_rtmCommunication;
};

}
}

