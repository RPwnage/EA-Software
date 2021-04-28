/*************************************************************************************************/
/*!
    \file httpconnmanagertest.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/framework/httpconnmanagertest.h"
#include "test/mock/mocklogger.h"
#include "test/framework/testhttpconnectionutil.h"
#include "framework/protocol/restprotocolutil.h" // for RestOutboundHttpResult in basicGET()

using namespace BlazeServerTest::Framework;

//DISABLED_: see TODO in sendTestRequestFiber
TEST_F(HttpConnManagerTest, DISABLED_basicGET)
{
    TestHttpConnectionUtil* t = BLAZE_NEW TestHttpConnectionUtil("google.com", "maps", "GET");
    t->run();
    const Blaze::RestOutboundHttpResult& result = t->getHttpResult();
    auto httpcode = const_cast<Blaze::RestOutboundHttpResult&>(result).getHttpStatusCode();
    ASSERT_EQ(httpcode, (Blaze::HttpStatusCode)0);
}
//DISABLED_: see TODO in sendTestRequestFiber
TEST_F(HttpConnManagerTest, DISABLED_initializeHttpConnManager)
{
    // case: basic non secure: size_t poolSize = 256; bool secure = false;
    Blaze::Authentication::HttpConnectionManagerPtr newConnMgr = nullptr;
    TestHttpConnectionUtil::getNewConnMgr(newConnMgr, "google.com", 256, false);
    return_if_testerr();
    ASSERT_TRUE(newConnMgr != nullptr);

    // case: change secure = true, with same settings above. For sanity, also tests reset()'ing the HttpConnectionManagerPtr
    TestHttpConnectionUtil::getNewConnMgr(newConnMgr, "google.com", 256, true);
    return_if_testerr();
    ASSERT_TRUE(newConnMgr != nullptr);

    // case: change conn pool to 1 and secure = false, with same settings above. For sanity, also tests reset()'ing the HttpConnectionManagerPtr
    TestHttpConnectionUtil::getNewConnMgr(newConnMgr, "google.com", 1, false);
    return_if_testerr();
    ASSERT_TRUE(newConnMgr != nullptr);
}
