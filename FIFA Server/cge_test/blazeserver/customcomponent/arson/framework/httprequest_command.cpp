#include "framework/blaze.h"
#include "arson/rpc/arsonslave/httprequest_stub.h"
#include "arson/tdf/arson.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/outboundhttpresult.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/usersessions/usersession.h"

#include "arsonslaveimpl.h"

namespace Blaze
{

namespace Arson
{

class ArsonHttpResult : public OutboundHttpResult
{
public:
    void setHttpError(BlazeRpcError err = ERR_SYSTEM) override
    {
    }

    void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes,
            size_t attributeCount) override
    {
    }

    void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override
    {
    }

    bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
            size_t attrCount) override
    {
        return false;
    }

};

class HttpRequestCommand : public HttpRequestCommandStub
{
public:
    HttpRequestCommand(Message* message, HttpRequest *req, ArsonSlaveImpl* componentImp)
        : HttpRequestCommandStub(message, req)
    {
    }

    ~HttpRequestCommand() override { }

    HttpRequestCommandStub::Errors execute() override
    {
        Errors rc = ERR_OK;
        
        TRACE_LOG("[HttpRequestCommand].start: starting http " << mRequest.getCount() << " request(s) to " 
                  << mRequest.getAddress() << "; uri=" << mRequest.getUri());

        InetAddress addr(mRequest.getAddress());
        OutboundHttpConnectionManager mgr(mRequest.getAddress());
        mgr.initialize(addr, 1);

        for(int32_t idx = 0; idx < mRequest.getCount(); ++idx)
        {
            ArsonHttpResult result;
            BlazeRpcError err = mgr.sendRequest(
                    HttpProtocolUtil::HTTP_GET, mRequest.getUri(), nullptr, 0, nullptr, 0, &result);
            if (err != Blaze::ERR_OK)
            {
                WARN_LOG("[HttpRequestCommand].start: error fetching URL.");
            }
        }

        TRACE_LOG("[HttpRequestCommand].start: completed request.");

        return rc;
    }
};

DEFINE_HTTPREQUEST_CREATE()

} // namespace Arson
} // namespace Blaze

