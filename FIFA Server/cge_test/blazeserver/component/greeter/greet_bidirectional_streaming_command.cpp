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
            GreetErrorResponse errorResponse;
            errorResponse.setGreeting("Hello, no responses expected.");
            sendErrorResponse(errorResponse, GREETER_ERR_INVALID_REQUEST);

            signalDone();
            return;
        }

        mResponseTimerId = Blaze::gSelector->scheduleFiberTimerCall(EA::TDF::TimeValue::getTimeOfDay() + (1 * 1000 * 1000),
            this, &GreetBidirectionalStreamingCommand::doSendResponse, "GreetBidirectionalStreamingCommand::doSendResponse");
    }

    // This is called when the client has told gRPC that it has finished sending all requests/messages.
    void onClientFinished() override
    {
    }

    // This is the finalization point for a BidirectionalStreaming gRPC request.
    void onCleanup(bool cancelled) override
    {
        Blaze::gSelector->cancelTimer(mResponseTimerId);
    }

    void doSendResponse()
    {
        GreetResponse response;
        response.setGreeting("Hello!");
        sendResponse(response);

        mNumberOfResponsesToSend -= 1;
        if (mNumberOfResponsesToSend == 0)
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
};


// static factory method impl
DEFINE_GREETBIDIRECTIONALSTREAMING_CREATE();

} // namespace v1alpha
} // namespace greeter
} // namespace title
} // namespace studio


