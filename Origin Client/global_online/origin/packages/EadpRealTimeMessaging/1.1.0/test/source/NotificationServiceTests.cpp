// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include <TestConfig.h>
#include <EadpSdkTester.h>

#include "../../source/notification/NotificationServiceImpl.h"
#include "../../source/rtm/RTMServiceImpl.h"
#include <mock/MockConnectionService.h>
#include <mock/MockRTMService.h>

constexpr auto RTM_GROUP_ID = 1337;

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;
using namespace testing;

class NotificationServiceTests : public StandardSdkTester
{
protected:
    NotificationServiceTests()
    : m_mockConnectionService(getAllocator().makeShared<MockConnectionService>(getHub()))
    , m_mockRTMService(getAllocator().makeShared<eadp::realtimemessaging::Internal::MockRTMService>())
    , m_notificationServiceImpl(getAllocator().makeShared<NotificationServiceImpl>(m_hub))
    , m_persistenceObject(getAllocator().makeShared<Callback::Persistence>())
    {}

    void SetUp() override
    {
        EXPECT_CALL(*m_mockConnectionService, getRTMService()).WillRepeatedly(Return(m_mockRTMService));
    }

protected:
    SharedPtr<MockConnectionService> m_mockConnectionService;
    SharedPtr<eadp::realtimemessaging::Internal::MockRTMService> m_mockRTMService;
    SharedPtr<NotificationServiceImpl> m_notificationServiceImpl;
    SharedPtr<Callback::Persistence> m_persistenceObject;
};

TEST_F(NotificationServiceTests, TestInitialize)
{
    EXPECT_CALL(*m_mockRTMService, addRTMCommunicationCallback(_)).Times(1);
    auto callback = [](const SharedPtr<rtm::protocol::NotificationV1> notificationUpdate) {};
    m_notificationServiceImpl->initialize(m_mockConnectionService, NotificationService::NotificationCallback(callback, m_persistenceObject, RTM_GROUP_ID));
}