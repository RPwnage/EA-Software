// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "FetchChannelHistoryRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using FetchChannelHistoryResponse = ResponseT<const String&,
                                              UniquePtr<Vector<SharedPtr<protocol::ChannelMessage>>>,
                                              const ErrorPtr&>;
constexpr char kFetchChannelHistoryRequestJobName[] = "FetchChannelHistoryRequestJob";
}

FetchChannelHistoryRequestJob::FetchChannelHistoryRequestJob(IHub* hub,
                                                             SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                                             const String& channelId,
                                                             uint32_t count,
                                                             SharedPtr<Timestamp> timestamp,
                                                             const String& primaryFilterType,
                                                             const String& secondaryFilterType,
                                                             ChatService::FetchMessagesCallback callback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(callback)
, m_channelId(channelId)
, m_count(count)
, m_timestamp(timestamp)
, m_primaryFilterType(primaryFilterType)
, m_secondaryFilterType(secondaryFilterType)
{
}

void FetchChannelHistoryRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (m_callback.isEmpty())
    {
        m_done = true;      // Mark current job done

        return;
    }

    if (m_channelId.empty())
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("ChannelId cannot be empty");
        EADPSDK_LOGE(getHub(), kFetchChannelHistoryRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        FetchChannelHistoryResponse::post(getHub(), kFetchChannelHistoryRequestJobName, m_callback, m_channelId, nullptr, error);
        return;
    }

    SharedPtr <protocol::HistoryRequest> historyRequest = getHub()->getAllocator().makeShared<protocol::HistoryRequest>(getHub()->getAllocator());
    historyRequest->setChannelId(m_channelId);
    historyRequest->setRecordCount(m_count);

    if (!m_primaryFilterType.empty())
    {
        historyRequest->setPrimaryFilterType(m_primaryFilterType);
        if (!m_secondaryFilterType.empty())
        {
            historyRequest->setSecondaryFilterType(m_secondaryFilterType);
        }
    }
    
    if (m_timestamp && m_timestamp->GetSeconds() > 0)
    {
        time_t t = static_cast<time_t>(EA::StdC::DateTimeSecondsToTimeTSeconds(m_timestamp->GetSeconds()));
        tm* pTM = gmtime(&t);
        if (pTM)
        {
            char result[32] = { 0 };
            EA::StdC::Strftime(result, sizeof(result), "%Y-%m-%dT%H:%M:%SZ", pTM);
            historyRequest->setTimestamp(getHub()->getAllocator().make<String>(result));
        }
    }

    SharedPtr<protocol::Header> header = getHub()->getAllocator().makeShared<protocol::Header>(getHub()->getAllocator());
    header->setRequestId(requestId);
    header->setType(protocol::CommunicationType::HISTORY_REQUEST);

    SharedPtr<protocol::Communication> communication = getHub()->getAllocator().makeShared<protocol::Communication>(getHub()->getAllocator());
    communication->setHeader(header);
    communication->setHistoryRequest(historyRequest);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void FetchChannelHistoryRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Fetch Channel History Request timed out.");
    EADPSDK_LOGE(getHub(), kFetchChannelHistoryRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    FetchChannelHistoryResponse::post(getHub(), kFetchChannelHistoryRequestJobName, m_callback, m_channelId, nullptr, error);
}

void FetchChannelHistoryRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto socialCommunication = communicationWrapper->getSocialCommunication();
    if (!socialCommunication
        || !socialCommunication->hasHistoryResponse())
    {
        String message = getHub()->getAllocator().make<String>("Fetch Channel History Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kFetchChannelHistoryRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        FetchChannelHistoryResponse::post(getHub(), kFetchChannelHistoryRequestJobName, m_callback, m_channelId, nullptr, error);
        return;
    }

    auto historyResponse = socialCommunication->getHistoryResponse();
    if (historyResponse->getSuccess())
    {
        EADPSDK_LOGV(getHub(), kFetchChannelHistoryRequestJobName, "Fetch Channel History successful");
        FetchChannelHistoryResponse::post(getHub(), kFetchChannelHistoryRequestJobName, m_callback, m_channelId, getHub()->getAllocator().makeUnique<Vector<SharedPtr<protocol::ChannelMessage>>>(historyResponse->getMessages()), nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Fetch Channel History request.");
        if (historyResponse->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", historyResponse->getErrorCode().c_str());
        }
        if (historyResponse->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", historyResponse->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kFetchChannelHistoryRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), historyResponse->getErrorCode(), message);
        FetchChannelHistoryResponse::post(getHub(), kFetchChannelHistoryRequestJobName, m_callback, m_channelId, nullptr, error);
    }
}

void FetchChannelHistoryRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    FetchChannelHistoryResponse::post(getHub(), kFetchChannelHistoryRequestJobName, m_callback, m_channelId, nullptr, error);
}
