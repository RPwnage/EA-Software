// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include <TestConfig.h>
#include <EadpSdkTester.h>

#include "../../source/chat/ChatServiceImpl.h"
#include "../../source/rtm/RTMServiceImpl.h"

constexpr auto TEST_CHANNEL_ID = "1234";
constexpr auto TEST_PRODUCT_ID = "origin";
constexpr auto RTM_GROUP_ID = 1337; 

using namespace eadp::foundation;
using namespace eadp::foundation::Internal;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;
using namespace testing;

class ChatServiceTests : public StandardSdkTester
{
protected:
    ChatServiceTests()
        : m_connectionService(ConnectionService::createService(m_hub))
        , m_chatServiceImpl(getAllocator().makeShared<ChatServiceImpl>(m_hub))
        , m_rtmService(getAllocator().makeShared<eadp::realtimemessaging::Internal::RTMServiceImpl>(getHub(), TEST_PRODUCT_ID))
        , m_persistenceObject(getAllocator().makeShared<Callback::Persistence>())
    {
    }

    EA_NON_COPYABLE(ChatServiceTests);

    virtual void SetUp()
    {
        m_callbackInvoked = false; 
    }

    virtual void TearDown()
    {
    }

    UniquePtr<rtm::protocol::ChatTypingEventRequestV1> createTypingIndicatorRequest()
    {
        auto allocator = getHub()->getAllocator();
        auto request = allocator.makeUnique<rtm::protocol::ChatTypingEventRequestV1>(allocator);
        request->setChannelId(TEST_CHANNEL_ID);
        request->setEvent(rtm::protocol::TypingEventType::COMPOSING);
        return request; 
    }

protected:
    SharedPtr<ConnectionService> m_connectionService;
    SharedPtr<ChatServiceImpl> m_chatServiceImpl; 
    SharedPtr<eadp::realtimemessaging::Internal::RTMServiceImpl> m_rtmService;
    SharedPtr<Callback::Persistence> m_persistenceObject; 
    bool m_callbackInvoked;
};

TEST_F(ChatServiceTests, TestSendTypingIndicator_NullRequest)
{
    auto errorCallback = [this](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr); 
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        m_callbackInvoked = true;
    };

    EXPECT_CALL(*m_hub, mockAddResponse(_));
    m_chatServiceImpl->sendTypingIndicator(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_TRUE(m_callbackInvoked); 
}

TEST_F(ChatServiceTests, TestSetComposingTimeInterval)
{
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), 3u);
    
    m_chatServiceImpl->setComposingTimeInterval(UINT32_MAX);
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), UINT32_MAX);
    
    m_chatServiceImpl->setComposingTimeInterval(0);
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), 3u);
}