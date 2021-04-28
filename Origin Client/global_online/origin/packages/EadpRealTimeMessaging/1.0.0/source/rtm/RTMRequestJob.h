// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include "rtm/CommunicationWrapper.h"
#include "rtm/RTMService.h"
#include <eadp/foundation/Error.h>
#include <eadp/foundation/internal/JobBase.h>

namespace eadp
{
namespace realtimemessaging
{

/*!
 * @brief RTM request job that expects a response and can time out. 
 */
class RTMRequestJob : public eadp::foundation::Internal::JobBase
{
public:
    static const uint32_t kDefaultTimeout = 30;

    virtual ~RTMRequestJob() = default;

    virtual void onTimeout() = 0;
    virtual void onSendError(const eadp::foundation::ErrorPtr& error) = 0;
    virtual void onComplete(eadp::foundation::SharedPtr<CommunicationWrapper> responseCommunication) = 0;
    virtual void setupOutgoingCommunication(const eadp::foundation::String& requestId) = 0;

    virtual eadp::foundation::Timestamp getExpirationTime();
    void execute() override;

    eadp::foundation::IHub* getHub();
    eadp::foundation::String getRequestId();
    eadp::foundation::SharedPtr<CommunicationWrapper> getRequestCommunication();

protected:
    RTMRequestJob(eadp::foundation::IHub* hub, eadp::foundation::SharedPtr<Internal::RTMService> rtmService);

    /*!
     * @brief Internal use only. Skip connectivity check. Used for logging in.
     */
    void skipConnectivityCheck();

    eadp::foundation::SharedPtr<CommunicationWrapper> m_communication;

private:
    enum class State
    {
        PRE_SETUP,
        AWAITING_RESPONSE
    };

    eadp::foundation::IHub* m_hub;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;
    eadp::foundation::Callback::PersistencePtr m_persistObj;
    eadp::foundation::Timestamp m_expiration;
    eadp::foundation::String m_requestId;
    State m_state;
    uint32_t m_timeout;
    bool m_skipConnectivityCheck;

};
}
}

