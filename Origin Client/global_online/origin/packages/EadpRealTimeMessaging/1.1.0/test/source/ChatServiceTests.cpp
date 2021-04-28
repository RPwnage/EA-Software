// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include <TestConfig.h>
#include <EadpSdkTester.h>

#include "../../source/chat/ChatServiceImpl.h"
#include "../../source/rtm/RTMServiceImpl.h"
#include <mock/MockConnectionService.h>
#include <mock/MockRTMService.h>

constexpr auto kTestChannelId = "1234";
constexpr auto kTestPersonaId = "123456789";
constexpr auto kTestUnixTimestamp = "1576004104";
constexpr auto kTestCustomProperties = "test custom properties";
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
    : m_mockConnectionService(getAllocator().makeShared<MockConnectionService>(getHub()))
    , m_mockRTMService(getAllocator().makeShared<eadp::realtimemessaging::Internal::MockRTMService>())
    , m_chatServiceImpl(getAllocator().makeShared<ChatServiceImpl>(m_hub))
    , m_persistenceObject(getAllocator().makeShared<Callback::Persistence>())
    {}

    void SetUp() override
    {
        EXPECT_CALL(*m_mockConnectionService, getRTMService()).WillRepeatedly(Return(m_mockRTMService));
#if EA_DEBUG
        EXPECT_CALL(*m_mockRTMService, addRTMCommunicationCallback(_)).Times(1);
#endif
        EXPECT_CALL(*m_hub, mockAddJob(_)).Times(1);
        m_chatServiceImpl->initialize(m_mockConnectionService);
    }

protected:
    SharedPtr<MockConnectionService> m_mockConnectionService;
    SharedPtr<eadp::realtimemessaging::Internal::MockRTMService> m_mockRTMService;
    SharedPtr<ChatService> m_chatServiceImpl;
    SharedPtr<Callback::Persistence> m_persistenceObject; 
};

TEST_F(ChatServiceTests, TestSetChatInviteCallback)
{
    EXPECT_CALL(*m_mockRTMService, addRTMCommunicationCallback(_)).Times(1);
    auto callback = [](SharedPtr<rtm::protocol::NotificationV1> chatInvite) {};
    m_chatServiceImpl->setChatInviteCallback(ChatService::ChatInviteCallback(callback, m_persistenceObject, RTM_GROUP_ID));
}

TEST_F(ChatServiceTests, TestSetChannelMessageCallback)
{
    EXPECT_CALL(*m_mockRTMService, addRTMCommunicationCallback(_)).Times(1);
    auto callback = [](SharedPtr<protocol::ChannelMessage> message) {};
    m_chatServiceImpl->setChannelMessageCallback(ChatService::ChannelMessageCallback(callback, m_persistenceObject, RTM_GROUP_ID));
}

TEST_F(ChatServiceTests, TestSetDirectMessageCallback)
{
    EXPECT_CALL(*m_mockRTMService, addRTMCommunicationCallback(_)).Times(1);
    auto callback = [](SharedPtr<rtm::protocol::PointToPointMessageV1> message) {};
    m_chatServiceImpl->setDirectMessageCallback(ChatService::DirectMessageCallback(callback, m_persistenceObject, RTM_GROUP_ID));
}

TEST_F(ChatServiceTests, TestFetchPreferences)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto callback = [](SharedPtr<rtm::protocol::GetPreferenceResponseV1> preferenceResponse, const ErrorPtr& error) {};
    m_chatServiceImpl->fetchPreferences(ChatService::PreferenceCallback(callback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestFetchChannelListV1)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto callback = [](SharedPtr<rtm::protocol::ChatChannelsV1> chatChannels, const ErrorPtr& error) {};
    m_chatServiceImpl->fetchChannelList(ChatService::ChatChannelsCallback(callback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestFetchChannelListV2)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatChannelsRequestV2>(getAllocator());
    auto callback = [](SharedPtr<rtm::protocol::ChatChannelsV1> chatChannels, const ErrorPtr& error) {};
    m_chatServiceImpl->fetchChannelList(EADPSDK_MOVE(request), ChatService::ChatChannelsCallback(callback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestFetchMembers_ValidChannelId)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatMembersRequestV1>(getAllocator());
    request->addChannelId(kTestChannelId);
    request->setChannelType(rtm::protocol::ChannelType::GAME);

    auto callback = [](SharedPtr<rtm::protocol::ChatMembersV1> chatMembers, const ErrorPtr& error) {};
    m_chatServiceImpl->fetchMembers(EADPSDK_MOVE(request), ChatService::ChatMembersCallback(callback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Fetch Message History Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestFetchMessageHistory_EmptyChannelId)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const String& channelId,
                                                 UniquePtr<Vector<SharedPtr<protocol::ChannelMessage>>> channelMessages,
                                                 const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->fetchMessageHistory("", 100, ChatService::FetchMessagesCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestFetchMessageHistory_ValidChannelId)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto callback = [](const String& channelId,
                       UniquePtr<Vector<SharedPtr<protocol::ChannelMessage>>> channelMessages,
                       const ErrorPtr& error)
    {};

    m_chatServiceImpl->fetchMessageHistory(kTestChannelId, 100, ChatService::FetchMessagesCallback(callback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Initiate Chat Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestInitiateChat_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto errorCallback = [](SharedPtr<rtm::protocol::ChatInitiateSuccessV1> chatInitiateSuccess, const ErrorPtr& error) {};
    auto request = getAllocator().makeUnique<rtm::protocol::ChatInitiateV1>(getAllocator());
    m_chatServiceImpl->initiateChat(EADPSDK_MOVE(request), ChatService::InitiateChatCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestInitiateChat_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](SharedPtr<rtm::protocol::ChatInitiateSuccessV1> chatInitiateSuccess, const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->initiateChat(nullptr, ChatService::InitiateChatCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestInitiateChat_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatInitiateV1>(getAllocator());
    request->addPersonaIds(kTestPersonaId); 

    auto errorCallback = [](SharedPtr<rtm::protocol::ChatInitiateSuccessV1> chatInitiateSuccess, const ErrorPtr& error) {};
    m_chatServiceImpl->initiateChat(EADPSDK_MOVE(request), ChatService::InitiateChatCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Update Translation Preferences Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestUpdateTranslationPreferences_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto errorCallback = [](const ErrorPtr& error) {};
    auto request = getAllocator().makeUnique<rtm::protocol::TranslationPreferenceV1>(getAllocator());
    m_chatServiceImpl->updateTranslationPreference(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestUpdateTranslationPreferences_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->updateTranslationPreference(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestUpdateTranslationPreferences_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::TranslationPreferenceV1>(getAllocator());
    request->setAutoTranslate(true);
    request->setLanguage("en");

    auto errorCallback = [](const ErrorPtr& error) {};
    m_chatServiceImpl->updateTranslationPreference(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Subscribe To Channel Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestSubscribeToChannel_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    auto request = getAllocator().makeUnique<protocol::SubscribeRequest>(getAllocator());
    m_chatServiceImpl->subscribeToChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSubscribeToChannel_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->subscribeToChannel(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSubscribeToChannel_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<protocol::SubscribeRequest>(getAllocator());
    request->setChannelId(kTestChannelId);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_chatServiceImpl->subscribeToChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Unsubscribe To Channel Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestUnsubscribeToChannel_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    auto request = getAllocator().makeUnique<protocol::UnsubscribeRequest>(getAllocator());
    m_chatServiceImpl->unsubscribeToChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestUnsubscribeToChannel_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->unsubscribeToChannel(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestUnsubscribeToChannel_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<protocol::UnsubscribeRequest>(getAllocator());
    request->setChannelId(kTestChannelId);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_chatServiceImpl->unsubscribeToChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Send Message To Channel Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestSendMessageToChannel_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    auto request = getAllocator().makeUnique<protocol::PublishTextRequest>(getAllocator());
    m_chatServiceImpl->sendMessageToChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSendMessageToChannel_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->sendMessageToChannel(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSendMessageToChannel_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<protocol::PublishTextRequest>(getAllocator());
    request->setChannelId(kTestChannelId);
    request->setText("Test Message"); 
    request->setMessageType(rtm::protocol::MessageType::REGULAR_MESSAGE);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_chatServiceImpl->sendMessageToChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Send Message Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestSendMessage_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));
    auto errorCallback = [](const ErrorPtr& error) {};
    auto request = getAllocator().makeUnique<rtm::protocol::PointToPointMessageV1>(getAllocator());
    m_chatServiceImpl->sendMessage(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestSendMessage_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->sendMessage(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSendMessage_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));
   
    auto request = getAllocator().makeUnique<rtm::protocol::PointToPointMessageV1>(getAllocator());
    auto customMessage = getAllocator().makeShared<rtm::protocol::CustomMessage>(getAllocator()); 
    customMessage->setPayload("test payload");
    customMessage->setPayloadType("test_payload");
    customMessage->setType("test_type");
    request->setCustomMessage(customMessage);
    auto from = getAllocator().makeShared<rtm::protocol::AddressV1>(getAllocator());
    from->setDomain(rtm::protocol::Domain::USER);
    from->setId("test_from_id");
    request->setFrom(from);
    auto to = getAllocator().makeShared<rtm::protocol::AddressV1>(getAllocator());
    to->setDomain(rtm::protocol::Domain::USER);
    to->setId("test_to_id");
    request->setFrom(to);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_chatServiceImpl->sendMessage(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Leave Channel Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestLeaveChannel_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1); 
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1); 
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute(); 
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));
    auto errorCallback = [](const ErrorPtr& error) {};
    auto request = getAllocator().makeUnique<rtm::protocol::ChatLeaveV1>(getAllocator());
    m_chatServiceImpl->leaveChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestLeaveChannel_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_)).Times(1);

    m_chatServiceImpl->leaveChannel(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestLeaveChannel_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, generateRequestId()).Times(1);
    EXPECT_CALL(*m_mockRTMService, sendRequest(_, _, _, _, _)).Times(1);
    
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatLeaveV1>(getAllocator());
    request->setChannelId(kTestChannelId);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_chatServiceImpl->leaveChannel(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Typing Indicator Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestSendTypingIndicator_EmptyRequest)
{
    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatTypingEventRequestV1>(getAllocator());
    m_chatServiceImpl->sendTypingIndicator(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSendTypingIndicator_NullRequest)
{
    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr); 
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };

    EXPECT_CALL(*m_hub, mockAddResponse(_));
    m_chatServiceImpl->sendTypingIndicator(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSendTypingIndicator_ValidRequest)
{
    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        ++callbackInvokedCount;
    };
    int jobAddedCount = 0; 
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatTypingEventRequestV1>(getAllocator());
    request->setChannelId(kTestChannelId); 
    request->setEvent(rtm::protocol::TypingEventType::COMPOSING);
    m_chatServiceImpl->sendTypingIndicator(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(callbackInvokedCount, 0);
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestSetComposingTimeInterval)
{
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), 3);
    
    m_chatServiceImpl->setComposingTimeInterval(UINT32_MAX);
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), UINT32_MAX);
    
    m_chatServiceImpl->setComposingTimeInterval(0);
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), 3);

    m_chatServiceImpl->setComposingTimeInterval(-1);
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), 3);
}

//////////////////////////////////////////////////////////////////////////
////// Channel Update Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(ChatServiceTests, TestSendChannelUpdate_EmptyRequest)
{
    int callbackInvokedCount = 0; 
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };
    EXPECT_CALL(*m_hub, mockAddResponse(_));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatChannelUpdateV1>(getAllocator()); 
    m_chatServiceImpl->sendChannelUpdate(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSendChannelUpdate_NullRequest)
{
    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        EXPECT_NE(error, nullptr);
        EXPECT_EQ(error->getCode(), static_cast<uint32_t>(FoundationError::Code::INVALID_ARGUMENT));
        ++callbackInvokedCount;
    };

    EXPECT_CALL(*m_hub, mockAddResponse(_));
    m_chatServiceImpl->sendChannelUpdate(nullptr, ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(callbackInvokedCount, 1);
}

TEST_F(ChatServiceTests, TestSendChannelUpdate_ValidRequest)
{
    int callbackInvokedCount = 0;
    auto errorCallback = [&callbackInvokedCount](const ErrorPtr& error)
    {
        ++callbackInvokedCount;
    };
    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::ChatChannelUpdateV1>(getAllocator());
    request->setChannelId(kTestChannelId);
    request->setLastReadTimestamp(kTestUnixTimestamp);
    request->setCustomProperties(kTestCustomProperties);
    m_chatServiceImpl->sendChannelUpdate(EADPSDK_MOVE(request), ChatService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(callbackInvokedCount, 0);
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(ChatServiceTests, TestSetChannelUpdateInterval)
{
    EXPECT_EQ(m_chatServiceImpl->getChannelUpdateInterval(), 3);

    m_chatServiceImpl->setChannelUpdateInterval(UINT32_MAX);
    EXPECT_EQ(m_chatServiceImpl->getChannelUpdateInterval(), UINT32_MAX);

    m_chatServiceImpl->setChannelUpdateInterval(0);
    EXPECT_EQ(m_chatServiceImpl->getChannelUpdateInterval(), 3);

    m_chatServiceImpl->setComposingTimeInterval(-1);
    EXPECT_EQ(m_chatServiceImpl->getComposingTimeInterval(), 3);
}