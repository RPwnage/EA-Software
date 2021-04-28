/*************************************************************************************************/
/*!
    \file testhttpconnectionutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_MOCK_HTTP_CONNECTION_H
#define BLAZE_SERVER_TEST_MOCK_HTTP_CONNECTION_H

#include "gtest/gtest.h"
#include "framework/blaze.h" // for avoiding Blaze compile errors NON_COPYABLE macro etc?
#include "framework/connection/outboundhttpconnectionmanager.h" // for HttpConnectionManagerPtr
#include "framework/system/fiber.h" //for Fiber::EventHandle mUpdateEventHandle

namespace Blaze
{
    class RestOutboundHttpResult;
    namespace Authentication { typedef eastl::intrusive_ptr<OutboundHttpConnectionManager> HttpConnectionManagerPtr; }
}


namespace BlazeServerTest
{
namespace Framework
{
    class TestHttpConnectionUtil
    {
    public:
        TestHttpConnectionUtil(const char8_t* baseUrl, const char8_t* urlSuffix, const char8_t* method);
        virtual ~TestHttpConnectionUtil();

        void run();

        const Blaze::RestOutboundHttpResult& getHttpResult() const { return *mHttpResult; }

        static void getNewConnMgr(Blaze::Authentication::HttpConnectionManagerPtr& connMgr, const eastl::string& baseUrl,
            size_t poolSize, bool secure);

    private:
        void sendReqAndWait(Blaze::RestOutboundHttpResult& httpResult,
            Blaze::Authentication::HttpConnectionManagerPtr connMgr, eastl::string httpMethod, eastl::string url);

        void sendTestRequestFiber(Blaze::RestOutboundHttpResult& httpResult, Blaze::Authentication::HttpConnectionManagerPtr connMgr,
            eastl::string httpMethod, eastl::string url);

    private:
        bool mIsSending;
        Blaze::RestOutboundHttpResult* mHttpResult;
        eastl::string mBaseUrl;
        eastl::string mUrlSuffix;
        eastl::string mHttpMethod;
        Blaze::Fiber::EventHandle mUpdateEventHandle;
        Blaze::Authentication::HttpConnectionManagerPtr mConnMgr;
    };

}//ns
}//ns

#endif