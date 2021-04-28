/*************************************************************************************************/
/*!
    \file   greeter_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class greeter_command

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "greeter/rpc/greeterslave/greet_stub.h"

#include <string>
namespace studio
{
namespace title
{
namespace greeter
{
namespace v1alpha
{

// Command with both request and response
class GreetCommand : public GreetCommandStub
{
public:
    using GreetCommandStub::GreetCommandStub;

    // This is the entry point for a Unary gRPC request.
    GreetError::Error onProcessRequest(const GreetRequest& request, GreetResponse& response, GreetErrorResponse& errorResponse) override
    {
        eastl::string userName(request.getUserName());
        if (userName.empty())
        {
            errorResponse.setGreeting("Hi user, please specify your username.");
            return GreetError::GREETER_ERR_INVALID_REQUEST;
        }

        eastl::string greeting("Hello "+ userName + ". Your persona id is " + std::to_string(getIdentityContext() != nullptr ? getIdentityContext()->getPersonaId() : 0).c_str());
        response.setGreeting(greeting.c_str());
      
        return ERR_OK;
    }

    // This is the finalization point for a Unary gRPC request.
    void onCleanup(bool cancelled) override
    {
        // We're not obligated to do anything here.  If we had allocated memory, or schedule some
        // job to run against 'this', we could free/cancel those things here.
    }
};

DEFINE_GREET_CREATE()

} // namespace v1alpha
} // namespace greeter
} // namespace title
} // namespace studio
