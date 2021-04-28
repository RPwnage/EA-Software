// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "HeartbeatJob.h"
#include <DirtySDK/platform.h>
#include <eadp/foundation/internal/Timestamp.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace EA::StdC;

/*!
 * @brief Time between heartbeat requests in seconds
 */
constexpr uint32_t kHeartbeatRate = 25;

HeartbeatJob::HeartbeatJob(WeakPtr<eadp::realtimemessaging::Internal::RTMService> rtmService)
: m_rtmService(EADPSDK_MOVE(rtmService))
, m_lastRefreshTime(0)
{
}

HeartbeatJob::~HeartbeatJob()
{
}

void HeartbeatJob::execute()
{
    uint64_t currentTime = ds_timeinsecs();
    if (kHeartbeatRate > (currentTime - m_lastRefreshTime))
    {
        return;
    }

    // Heartbeat job not needed if the RTM service is destroyed
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService = m_rtmService.lock();
    if (!rtmService)
    {
        m_done = true;
        return;
    }

    // Refresh will fail if the RTM service is not connected
    if (rtmService->getState() != eadp::realtimemessaging::Internal::RTMService::ConnectionState::CONNECTED)
    {
        return;
    }

    rtmService->refresh();
    m_lastRefreshTime = currentTime;
}
