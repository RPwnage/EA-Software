// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include <TestConfig.h>
#include <EadpSdkTester.h>

#include "../../source/presence/PresenceUpdateJob.h"
#include "../../source/presence/PresenceSubscribeJob.h"
#include "../../source/presence/PresenceUnsubscribeJob.h"

constexpr auto TEST_CALLBACK_GROUP = 1001;
constexpr auto TEST_PRESENCE_STATUS = "TEST STATUS";
constexpr auto TEST_PERSONA_ID_1 = "12345";
constexpr auto TEST_PERSONA_ID_2 = "67890";
constexpr auto TEST_PERSONA_ID_3 = "44114";
constexpr auto TEST_PLAYER_ID_1 = "33333";
constexpr auto TEST_PRODUCT_ID_1 = "00000";

using namespace eadp::foundation;
using namespace eadp::foundation::Internal;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope::rtm;
using namespace testing;

class PresenceJobTests : public StandardSdkTester
{
protected:
    PresenceJobTests() = default;

    EA_NON_COPYABLE(PresenceJobTests);

    bool m_callbackInvoked; 
    SharedPtr<ConnectionService> m_connectionService; 
    SharedPtr<protocol::PresenceUpdateV1> m_updateStatusRequest;
    SharedPtr<protocol::PresenceSubscribeV1> m_subscribeRequest;
    SharedPtr<protocol::PresenceUnsubscribeV1> m_unsubscribeRequest;

    virtual void SetUp()
    {
        m_callbackInvoked = false; 
        m_connectionService = ConnectionService::createService(m_hub);

        auto allocator = getHub()->getAllocator(); 
        m_updateStatusRequest = allocator.makeShared<protocol::PresenceUpdateV1>(allocator);
        m_updateStatusRequest->setStatus(TEST_PRESENCE_STATUS);

        m_subscribeRequest = allocator.makeShared<protocol::PresenceSubscribeV1>(allocator);
        m_subscribeRequest->addPersonaId(TEST_PERSONA_ID_1);
        m_subscribeRequest->addPersonaId(TEST_PERSONA_ID_2);
        auto player = allocator.makeShared<protocol::Player>(allocator);
        player->setPlayerId(TEST_PLAYER_ID_1);
        player->setProductId(TEST_PRODUCT_ID_1);
        m_subscribeRequest->getPlayers().push_back(player);
        
        m_unsubscribeRequest = allocator.makeShared<protocol::PresenceUnsubscribeV1>(allocator);
        m_unsubscribeRequest->addPersonaId(TEST_PERSONA_ID_3); 
        m_unsubscribeRequest->getPlayers().push_back(player); 

        EXPECT_CALL(*m_hub, mockAddResponse(_));
    }

    virtual void TearDown()
    {
    }

};

TEST_F(PresenceJobTests, TestUpdateJobConstructor)
{
    auto cb = [this](const eadp::foundation::ErrorPtr error)
    {
        ASSERT_NE(error, nullptr);
        EXPECT_FALSE(error->getReason().empty());
        m_callbackInvoked = true;
    };

    PresenceUpdateJob job(getHub(), m_connectionService->getRTMService(), m_updateStatusRequest, PresenceService::ErrorCallback(cb, m_callbackPersistObj, TEST_CALLBACK_GROUP));
    job.execute();
    EXPECT_TRUE(m_callbackInvoked);
}

TEST_F(PresenceJobTests, TestSubscribeJobConstructor)
{
    auto cb = [this](const eadp::foundation::ErrorPtr error)
    {
        ASSERT_NE(error, nullptr);
        EXPECT_FALSE(error->getReason().empty());
        m_callbackInvoked = true;
    };

    PresenceSubscribeJob job(getHub(), m_connectionService->getRTMService(), m_subscribeRequest, PresenceService::ErrorCallback(cb, m_callbackPersistObj, TEST_CALLBACK_GROUP));
    job.execute();
    EXPECT_TRUE(m_callbackInvoked);
}

TEST_F(PresenceJobTests, TestUnsubscribeJobConstructor)
{
    auto cb = [this](const eadp::foundation::ErrorPtr error)
    {
        ASSERT_NE(error, nullptr);
        EXPECT_FALSE(error->getReason().empty());
        m_callbackInvoked = true;
    };

    PresenceUnsubscribeJob job(getHub(), m_connectionService->getRTMService(), m_unsubscribeRequest, PresenceService::ErrorCallback(cb, m_callbackPersistObj, TEST_CALLBACK_GROUP));
    job.execute();
    EXPECT_TRUE(m_callbackInvoked);
}