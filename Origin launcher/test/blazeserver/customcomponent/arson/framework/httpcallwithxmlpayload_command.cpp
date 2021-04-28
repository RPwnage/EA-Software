
/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"


// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/httpcallwithxmlpayload_stub.h"

namespace Blaze
{
namespace Arson
{

class HttpCallResult : public OutboundHttpResult
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
class HttpCallWithXMLpayloadCommand : public HttpCallWithXMLpayloadCommandStub
{
public:
    HttpCallWithXMLpayloadCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : HttpCallWithXMLpayloadCommandStub(message),
        mComponent(componentImpl)
    {
        
    }

    ~HttpCallWithXMLpayloadCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    HttpCallWithXMLpayloadCommandStub::Errors execute() override
    {
       
        const char8_t* host = "nucleus.integration.ea.com:80";
        InetAddress *addr = BLAZE_NEW InetAddress(host);
        OutboundHttpConnectionManager connMgr(host);
        connMgr.initialize(*addr, 10, false);

        char8_t strURI[256];
        memset(strURI, 0, sizeof(strURI));
        blaze_snzprintf(strURI, sizeof(strURI), "1.0/authtoken");
        
        const uint8_t inputHeaderSize = 1;
        const char8_t* inputHeaders[] = { 
            "Nucleus-RequestorId: GOS-Blaze"
        };

        const uint8_t httpParamsSize = 3;
        HttpParam httpParams[httpParamsSize];
        
        httpParams[0].name = "duration";
        httpParams[0].value = "15";
        httpParams[1].name = "email";
        httpParams[1].value = "nexus001@ea.com";
        httpParams[2].name = "password";
        httpParams[2].value = "t3st3r";

        HttpCallResult result;
        BlazeRpcError error = connMgr.sendRequest(HttpProtocolUtil::HTTP_POST, strURI, httpParams, httpParamsSize, inputHeaders, inputHeaderSize, &result, Log::HTTP, true); 

        delete addr; 
        
        if(!error)
            return ERR_OK;
        else 
            return ERR_SYSTEM;
        
    }
};

DEFINE_HTTPCALLWITHXMLPAYLOAD_CREATE()

} //Arson
} //Blaze
