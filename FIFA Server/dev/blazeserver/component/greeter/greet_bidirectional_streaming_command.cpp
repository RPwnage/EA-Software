/*************************************************************************************************/
/*!
\file   greetbidirectionalstreaming_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class GreetBidirectionalStreamingCommand

\note
 The following example is a command handler meant specifically to handle gRPC BidirectionalStreaming requests.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include

#include "framework/blaze.h"
#include "greeter/rpc/greeterslave/greetbidirectionalstreaming_stub.h"

#include "framework/connection/selector.h"
#include "EATDF/time.h"

namespace studio
{
namespace title
{
namespace greeter
{
namespace v1alpha
{

class GreetBidirectionalStreamingCommand : public GreetBidirectionalStreamingCommandStub
{
public:
    using GreetBidirectionalStreamingCommandStub::GreetBidirectionalStreamingCommandStub;

    // This is the entry point for a BidirectionalStreaming gRPC request.
    void onProcessRequest(const GreetRequest& request) override
    {
        mNumberOfResponsesToSend += request.getNum();
        if (mNumberOfResponsesToSend == 0)
        {
            finishWithError(grpc::Status(grpc::INVALID_ARGUMENT, "Hello, no responses expected."));
            return;
        }

        if (mResponseTimerId == INVALID_TIMER_ID) // Only create a new timer if one isn't pending.
        {
            mResponseTimerId = Blaze::gSelector->scheduleFiberTimerCall(EA::TDF::TimeValue::getTimeOfDay() + (1 * 1000 * 1000),
                this, &GreetBidirectionalStreamingCommand::doSendResponse, "GreetBidirectionalStreamingCommand::doSendResponse");
        }
    }

    // This is called when the client has told gRPC that it has finished sending all requests/messages.
    void onClientFinished() override
    {
        mClientFinished = true;
    }

    // This is the finalization point for a BidirectionalStreaming gRPC request.
    void onCleanup(bool cancelled) override
    {
        Blaze::gSelector->cancelTimer(mResponseTimerId);
    }

    void doSendResponse()
    {
        if (mNumberOfResponsesToSend > 0) // if have something to send
        {
            GreetResponse response;
            response.setGreeting("Hello!");
            sendResponse(response);

            --mNumberOfResponsesToSend;
        }
        // If the client is finished and we don't have any more responses to send, finish the rpc.
        // We only want to signal done in this demo implementation after the client is finished. See comment inside BidirectionalStreamingRpc::sendResponseImpl.
        // In a real implementation, you can still finish a bidi streaming before the client by cancelling the rpc.
        if (mClientFinished && mNumberOfResponsesToSend == 0)
        {
            signalDone();
        }
        else
        {
            mResponseTimerId = Blaze::gSelector->scheduleFiberTimerCall(EA::TDF::TimeValue::getTimeOfDay() + (1 * 1000 * 1000),
                this, &GreetBidirectionalStreamingCommand::doSendResponse, "GreetBidirectionalStreamingCommand::doSendResponse");
        }
    }

private:
    int32_t mNumberOfResponsesToSend = 0;
    TimerId mResponseTimerId = INVALID_TIMER_ID;
    bool mClientFinished = false;
};


// static factory method impl
DEFINE_GREETBIDIRECTIONALSTREAMING_CREATE();

} // namespace v1alpha
} // namespace greeter
} // namespace title
} // namespace studio


