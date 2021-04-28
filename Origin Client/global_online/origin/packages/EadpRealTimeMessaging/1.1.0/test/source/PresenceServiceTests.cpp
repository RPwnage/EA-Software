// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include <TestConfig.h>
#include <EadpSdkTester.h>

#include "../../source/presence/PresenceServiceImpl.h"
#include "../../source/rtm/RTMServiceImpl.h"
#include <mock/MockConnectionService.h>
#include <mock/MockRTMService.h>

constexpr auto kTestPersonaId = "123456789";
constexpr auto RTM_GROUP_ID = 1337;

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;
using namespace testing;

class PresenceServiceTests : public StandardSdkTester
{
protected:
    PresenceServiceTests()
        : m_mockConnectionService(getAllocator().makeShared<MockConnectionService>(getHub()))
        , m_mockRTMService(getAllocator().makeShared<eadp::realtimemessaging::Internal::MockRTMService>())
        , m_presenceServiceImpl(getAllocator().makeShared<PresenceServiceImpl>(m_hub))
        , m_persistenceObject(getAllocator().makeShared<Callback::Persistence>())
    {}

    void SetUp() override
    {
        EXPECT_CALL(*m_mockConnectionService, getRTMService()).WillRepeatedly(Return(m_mockRTMService));
        EXPECT_CALL(*m_mockRTMService, addRTMCommunicationCallback(_)).Times(3);
        auto updateCallback = [](const SharedPtr<rtm::protocol::PresenceV1> notificationUpdate) {};
        auto subscribeErrorCallback = [](const SharedPtr<rtm::protocol::PresenceSubscriptionErrorV1> notificationUpdate) {};
        auto updateErrorCallback = [](const SharedPtr<rtm::protocol::PresenceUpdateErrorV1> notificationUpdate) {};
        m_presenceServiceImpl->initialize(m_mockConnectionService,
                                          PresenceService::PresenceUpdateCallback(updateCallback, m_persistenceObject, RTM_GROUP_ID),
                                          PresenceService::PresenceSubscriptionErrorCallback(subscribeErrorCallback, m_persistenceObject, RTM_GROUP_ID),
                                          PresenceService::PresenceUpdateErrorCallback(updateErrorCallback, m_persistenceObject, RTM_GROUP_ID));
    }

protected:
    SharedPtr<MockConnectionService> m_mockConnectionService;
    SharedPtr<eadp::realtimemessaging::Internal::MockRTMService> m_mockRTMService;
    SharedPtr<PresenceServiceImpl> m_presenceServiceImpl;
    SharedPtr<Callback::Persistence> m_persistenceObject;
};

//////////////////////////////////////////////////////////////////////////
////// Update Status Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(PresenceServiceTests, TestUpdateStatus_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::PresenceUpdateV1>(getAllocator());
    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->updateStatus(EADPSDK_MOVE(request), PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(PresenceServiceTests, TestUpdateStatus_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->updateStatus(nullptr, PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(PresenceServiceTests, TestUpdateStatus_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::PresenceUpdateV1>(getAllocator());
    request->setStatus("Test Status");
    request->setUserDefinedPresence("Test User Defined Presence"); 
    request->setBasicPresenceType(rtm::protocol::BasicPresenceType::ONLINE);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->updateStatus(EADPSDK_MOVE(request), PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Subscribe Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(PresenceServiceTests, TestSubscribe_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::PresenceSubscribeV1>(getAllocator());
    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->subscribe(EADPSDK_MOVE(request), PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(PresenceServiceTests, TestSubscribe_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->subscribe(nullptr, PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(PresenceServiceTests, TestSubscribe_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::PresenceSubscribeV1>(getAllocator());
    request->addPersonaId(kTestPersonaId);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->subscribe(EADPSDK_MOVE(request), PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

//////////////////////////////////////////////////////////////////////////
////// Unsubscribe Tests
//////////////////////////////////////////////////////////////////////////

TEST_F(PresenceServiceTests, TestUnsubscribe_EmptyRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::PresenceUnsubscribeV1>(getAllocator());
    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->unsubscribe(EADPSDK_MOVE(request), PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(PresenceServiceTests, TestUnsubscribe_NullRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->unsubscribe(nullptr, PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}

TEST_F(PresenceServiceTests, TestUnsubscribe_ValidRequest)
{
    EXPECT_CALL(*m_mockRTMService, sendCommunication(_, _)).Times(1);

    int jobAddedCount = 0;
    auto jobAddedCallback = [&jobAddedCount](UniquePtr<IJob>& job)
    {
        ++jobAddedCount;
        job->execute();
    };
    EXPECT_CALL(*m_hub, mockAddJob(_)).WillOnce(Invoke(jobAddedCallback));

    auto request = getAllocator().makeUnique<rtm::protocol::PresenceUnsubscribeV1>(getAllocator());
    request->addPersonaId(kTestPersonaId);

    auto errorCallback = [](const ErrorPtr& error) {};
    m_presenceServiceImpl->unsubscribe(EADPSDK_MOVE(request), PresenceService::ErrorCallback(errorCallback, m_persistenceObject, RTM_GROUP_ID));
    EXPECT_EQ(jobAddedCount, 1);
}