/*************************************************************************************************/
/*!
\file   greet_server_streaming_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class GreetServerStreamingCommand


\note
 The following example is a command handler meant specifically to handle gRPC ServerStreaming requests.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include

#include "framework/blaze.h"
#include "greeter/rpc/greeterslave/greetserverstreaming_stub.h"

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

class GreetServerStreamingCommand : public GreetServerStreamingCommandStub
{
public:
    using GreetServerStreamingCommandStub::GreetServerStreamingCommandStub;

    // This is the entry point for a ServerStreaming gRPC request.
    void onProcessRequest(const GreetRequest& request) override
    {
        mNumberOfResponsesToSend = request.getNum();
        if (mNumberOfResponsesToSend < 0)
        {
            GreetErrorResponse errorResponse;
            errorResponse.setGreeting("Hi user, Num must be >= 0");
            sendErrorResponse(errorResponse, GreetServerStreamingError::GREETER_ERR_INVALID_REQUEST);
            signalDone();
        }
        else
        {
            mUserName.assign(request.getUserName());
            mResponseTimerId = Blaze::gSelector->scheduleFiberTimerCall(EA::TDF::TimeValue::getTimeOfDay() + (1 * 1000 * 1000),
                this, &GreetServerStreamingCommand::doSendResponse, "GreetServerStreamingCommand::doSendResponse");
        }
    }

    // This is the finalization point for a ServerStreaming gRPC request.
    void onCleanup(bool cancelled) override
    {
        Blaze::gSelector->cancelTimer(mResponseTimerId);
    }

    void doSendResponse()
    {

        // It is intentional that we can go into negative values here if the caller specified 0 for Num.
        // You may notice that will lead to signalDone() *never* getting called.  This is just to show an example
        // of a never ending periodic server streaming response.
        mNumberOfResponsesToSend -= 1;

        eastl::string msg(eastl::string::CtorSprintf(), "Hello %s, Number of responses remaining: %" PRIi32, mUserName.c_str(), mNumberOfResponsesToSend);

        GreetResponse response;
        response.setGreeting(msg.c_str());

        sendResponse(response);

        if (mNumberOfResponsesToSend == 0)
        {
            signalDone();
        }
        else
        {
            mResponseTimerId = Blaze::gSelector->scheduleFiberTimerCall(EA::TDF::TimeValue::getTimeOfDay() + (1 * 1000 * 1000),
                this, &GreetServerStreamingCommand::doSendResponse, "GreetServerStreamingCommand::doSendResponse");
        }
    }

private:
    int32_t mNumberOfResponsesToSend = 0;
    TimerId mResponseTimerId = INVALID_TIMER_ID;
    eastl::string mUserName;
};


// static factory method impl
DEFINE_GREETSERVERSTREAMING_CREATE();

} // namespace v1alpha
} // namespace greeter
} // namespace title
} // namespace studio


