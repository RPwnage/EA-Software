/*************************************************************************************************/
/*!
\file   greet_client_streaming_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class GreetClientStreamingCommand

\note
 The following example is a command handler meant specifically to handle gRPC ClientStreaming requests.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include

#include "framework/blaze.h"
#include "greeter/rpc/greeterslave/greetclientstreaming_stub.h"

#include "framework/util/shared/stringbuilder.h"
#include "EATDF/time.h"

// global includes

// example includes

namespace studio
{
namespace title
{
namespace greeter
{
namespace v1alpha
{

class GreetClientStreamingCommand : public GreetClientStreamingCommandStub
{
public:
    using GreetClientStreamingCommandStub::GreetClientStreamingCommandStub;

    // This is the entry point for a ClientStreaming gRPC request.
    void onProcessRequest(const GreetRequest& request) override
    {
        mSumOfAllReceivedNums += request.getNum();
        mAggregateOfAllReceivedMessages << request.getUserName() << "\n";

        // simulate a blocking call just to show that blocking client streamed requests are processed in a serial manner.
        Blaze::Fiber::sleep(1 * 1000 * 1000, "GreetClientStreamingCommand::onProcessRequest");

        if (request.getNum() < 0)
        {
            // In this example, we just show how to quickly but gracefully tell gRPC we don't want any more requests from the client.
            // See comments for the signalCancelled() method.
            signalCancelled();
        }
    }

    // This is called when the client has told gRPC that it has finished sending all requests/messages. A response from the server can now be sent.
    GreetClientStreamingError::Error onClientFinished(GreetResponse& response, GreetErrorResponse& errorResponse) override
    {
        mAggregateOfAllReceivedMessages << "The sum of all received Nums is: " << mSumOfAllReceivedNums;
        if (mSumOfAllReceivedNums > 0)
        {
            eastl::string msg(eastl::string::CtorSprintf(), "Hello users, you are: %s", mAggregateOfAllReceivedMessages.get());
            response.setGreeting(msg.c_str());
            return ERR_OK;
        }
        else
        {
            errorResponse.setGreeting(mAggregateOfAllReceivedMessages.get());
            return GREETER_ERR_INVALID_REQUEST;
        }
    }

    // This is the finalization point for a ClientStreaming gRPC request.
    void onCleanup(bool cancelled) override
    {
        // We're not obligated to do anything here.  If we had allocated memory, or schedule some
        // job to run against 'this', we could free/cancel those things here.
    }

private:
    int32_t mSumOfAllReceivedNums = 0;
    Blaze::StringBuilder mAggregateOfAllReceivedMessages;
};


// static factory method impl
DEFINE_GREETCLIENTSTREAMING_CREATE();

} // namespace v1alpha
} // namespace greeter
} // namespace title
} // namespace studio


