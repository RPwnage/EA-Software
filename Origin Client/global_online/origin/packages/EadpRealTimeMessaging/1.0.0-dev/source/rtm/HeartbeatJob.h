// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include "rtm/RTMService.h"
#include <eadp/foundation/internal/JobBase.h>

namespace eadp
{
namespace realtimemessaging
{
class HeartbeatJob : public eadp::foundation::Internal::JobBase
{
public:
    HeartbeatJob(eadp::foundation::WeakPtr<Internal::RTMService> rtmService);
    ~HeartbeatJob();

    void execute() override;
private:
    eadp::foundation::WeakPtr<Internal::RTMService> m_rtmService;
    uint64_t m_lastRefreshTime;

};
}
}
