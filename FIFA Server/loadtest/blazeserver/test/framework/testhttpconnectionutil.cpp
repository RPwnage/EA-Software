/*************************************************************************************************/
/*!
\file testhttpconnectionutil.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/framework/testhttpconnectionutil.h"
#include "test/mock/mocklogger.h"
#include "framework/protocol/restprotocolutil.h" // for RestOutboundHttpResult in TestHttpConnectionUtil

namespace BlazeServerTest
{
namespace Framework
{
    TestHttpConnectionUtil::TestHttpConnectionUtil(const char8_t* baseUrl, const char8_t* urlSuffix, const char8_t* method)
        :
        mIsSending(false),
        mHttpResult(BLAZE_NEW Blaze::RestOutboundHttpResult(nullptr, nullptr, nullptr, nullptr, nullptr)),
        mBaseUrl(baseUrl),
        mUrlSuffix(urlSuffix),
        mHttpMethod(method)
    {
    }

    TestHttpConnectionUtil::~TestHttpConnectionUtil()
    {
        if (mHttpResult != nullptr)
            delete mHttpResult;
        mHttpResult = nullptr;
    }

    void TestHttpConnectionUtil::run()
    {
        if (mIsSending)
        {
            return;
        }
        mIsSending = true;

        getNewConnMgr(mConnMgr, mBaseUrl, 256, false);
        EXPECT_TRUE(mConnMgr != nullptr);

        Blaze::Fiber::CreateParams params;
        params.blocking = true;
        params.blocksCaller = true;
        params.stackSize = Blaze::Fiber::STACK_MEDIUM;

        // need to wrap in a fiber, since top fiber isn't allowed to call getAndWait as its non-blocking
        auto sendResult = Blaze::gSelector->scheduleFiberCall<TestHttpConnectionUtil, Blaze::RestOutboundHttpResult&, Blaze::Authentication::HttpConnectionManagerPtr, eastl::string, eastl::string>(
            this, &TestHttpConnectionUtil::sendReqAndWait, *mHttpResult, mConnMgr, mHttpMethod, mUrlSuffix, "sendReqAndWait", params);
        EXPECT_TRUE(sendResult);
    }
    

    // send request, blocks caller
    void TestHttpConnectionUtil::sendReqAndWait(Blaze::RestOutboundHttpResult& httpResult,
        Blaze::Authentication::HttpConnectionManagerPtr connMgr, eastl::string httpMethod, eastl::string url)
    {
        // send request
        Blaze::Fiber::CreateParams params;
        params.blocking = true;
        params.blocksCaller = true;
        params.stackSize = Blaze::Fiber::STACK_MEDIUM;
        //no one else should be waiting on our handle
        if (mUpdateEventHandle.isValid())
        {
            Blaze::Fiber::signal(mUpdateEventHandle, Blaze::ERR_SYSTEM);
            return;
        }
        mUpdateEventHandle = Blaze::Fiber::getNextEventHandle();

        sendTestRequestFiber(httpResult, connMgr, httpMethod, url);

        //Note: the below attempt to wait for response won't work, as the non-main fiber will just return to caller right away, causing test to just continue. Future test enhancements would need to update so the main test fiber can block
        //// wait for all the results here
        //while (isSending() && mUpdateEventHandle.isValid())
        //{
        //    //no one else should be waiting on our handle
        //    if (mUpdateEventHandle.isValid())
        //    {
        //        testerr("Prior test op using mUpdateEventHandle, cannot continue.");
        //        Blaze::Fiber::signal(mUpdateEventHandle, Blaze::ERR_SYSTEM);
        //        return;
        //    }
        //    if (Blaze::Fiber::getAndWait(mUpdateEventHandle, "sendTestRequestFiber") != Blaze::ERR_OK)
        //    {
        //        testerr("Failed to wait for send request fiber. check test");
        //    }
        //}
    }

    void TestHttpConnectionUtil::sendTestRequestFiber(Blaze::RestOutboundHttpResult& httpResult,
        Blaze::Authentication::HttpConnectionManagerPtr connMgr,
        eastl::string httpMethod, eastl::string url)
    {
        Blaze::BlazeRpcError requestError = Blaze::ERR_OK;
        Blaze::RestRequestBuilder::HeaderVector headerVector;
        Blaze::RestRequestBuilder::HttpParamVector httpParams;
        const char8_t** httpHeaders = nullptr;
        Blaze::OutboundHttpConnection::ContentPartList contentList;
        Blaze::RpcCallOptions callOpts;
        callOpts.ignoreReply = true;
        callOpts.timeoutOverride = 10000 * 1000;
        
        // DISABLED TODO: instead of real http call, needs to internally swap in w/ *mock object* response:
        requestError = connMgr->sendRequest(Blaze::HttpProtocolUtil::getMethodType(httpMethod.c_str()), url.c_str(),
            &httpParams[0], httpParams.size(), httpHeaders, headerVector.size(),
            &httpResult, &contentList, Blaze::Log::HTTP, nullptr, nullptr, 10, callOpts);
        /*
        // Note: the below attempt to wait for response won't work, as the non-main fiber will just return to caller right away, causing test to just continue. Future test enhancements would need to update so the main test fiber can block
        if (has_testerr())
        {
            mIsSending = false;
            Blaze::Fiber::signal(mUpdateEventHandle, Blaze::ERR_SYSTEM);
            return;
        }
        if (requestError == ERR_OK)
            requestError = httpResult.getBlazeErrorCode();
        if (requestError != ERR_OK)
        {
            EXPECT_EQ(Blaze::ERR_OK, requestError);
            testerr("Blaze error sending request");
            mIsSending = false;
            Blaze::Fiber::signal(mUpdateEventHandle, requestError);
            return;
        }
        mIsSending = false;
        Blaze::Fiber::signal(mUpdateEventHandle, requestError);*/
    }

    void TestHttpConnectionUtil::getNewConnMgr(Blaze::Authentication::HttpConnectionManagerPtr& connMgr, const eastl::string& baseUrl,
        size_t poolSize, bool secure)
    {
        const char8_t* hostname = nullptr;
        Blaze::HttpProtocolUtil::getHostnameFromConfig(baseUrl.c_str(), hostname, secure);
        Blaze::OutboundHttpConnectionManager* newConnMgr = BLAZE_NEW Blaze::OutboundHttpConnectionManager(hostname);
        newConnMgr->initialize(Blaze::InetAddress(hostname), poolSize, secure, true,
            Blaze::OutboundHttpConnectionManager::SSLVERSION_DEFAULT, "application/JSON", nullptr, 10000, 10000, 10000, "test");

        if (connMgr != nullptr)
        {
            ((Blaze::Authentication::HttpConnectionManagerPtr)connMgr).reset();
        }
        connMgr = Blaze::Authentication::HttpConnectionManagerPtr(newConnMgr);
    }


}//ns
}//ns