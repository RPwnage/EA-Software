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
 * @brief RTM request job that does not expect a response or time out. 
 */
class RTMCommunicationJob : public eadp::foundation::Internal::JobBase
{
public:
    virtual ~RTMCommunicationJob() = default;

    virtual void onSendError(const eadp::foundation::ErrorPtr& error) = 0;
    virtual void setupOutgoingCommunication() = 0;

    void execute() override;

    eadp::foundation::SharedPtr<CommunicationWrapper> getRequestCommunication();
    eadp::foundation::IHub* getHub();

protected:
    RTMCommunicationJob(eadp::foundation::IHub* hub, eadp::foundation::SharedPtr<Internal::RTMService> rtmService);

    eadp::foundation::SharedPtr<CommunicationWrapper> m_communication;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;

private:
    eadp::foundation::IHub* m_hub;
    eadp::foundation::Callback::PersistencePtr m_persistObj;
    bool m_executed; 
};
}
}

