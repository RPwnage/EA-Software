#include "framework/blaze.h"
#include "arson/rpc/arsonslave/runcommandfromanothercommand_stub.h"
#include "arson/rpc/arsonslave/commandtorunfromanothercommand_stub.h"
#include "arsonslaveimpl.h"

namespace Blaze
{

namespace Arson
{
class RunCommandFromAnotherCommandCommand : public RunCommandFromAnotherCommandCommandStub
{
public:
    RunCommandFromAnotherCommandCommand(Message* message, ArsonSlave* componentImpl)
        : RunCommandFromAnotherCommandCommandStub(message), mComponent(componentImpl)
    {
    }

    ~RunCommandFromAnotherCommandCommand() override { }

    RunCommandFromAnotherCommandCommandStub::Errors execute() override
    {
        ASSERT(gCurrentUserSession != nullptr);

        INFO_LOG("RunCommandFromAnotherCommand: Sleeping for 9s before calling another command...");
        BlazeRpcError err = gSelector->sleep(9000000);

        err = mComponent->commandToRunFromAnotherCommand();

        return commandErrorFromBlazeError(err);
    }

private:
    ArsonSlave* mComponent;
};

// static
DEFINE_RUNCOMMANDFROMANOTHERCOMMAND_CREATE();


class CommandToRunFromAnotherCommandCommand: public CommandToRunFromAnotherCommandCommandStub
{
public:
    CommandToRunFromAnotherCommandCommand(Message* message, ArsonSlaveImpl* componentImp)
        : CommandToRunFromAnotherCommandCommandStub(message)
    {
    }

    ~CommandToRunFromAnotherCommandCommand() override { }

    CommandToRunFromAnotherCommandCommandStub::Errors execute() override
    {
        ASSERT(gCurrentUserSession != nullptr);
        return ERR_OK;
    }
};

// static
DEFINE_COMMANDTORUNFROMANOTHERCOMMAND_CREATE();

} // namespace Arson
} // namespace Blaze

