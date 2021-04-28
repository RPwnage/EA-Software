// global includes
#include "framework/blaze.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/pokepermissiontest_stub.h"

namespace Blaze
{
    namespace Arson
    {

        class PokePermissionTestCommand : public PokePermissionTestCommandStub
        {
        public:
            PokePermissionTestCommand(Message* message, ArsonSlave* componentImpl)
                : PokePermissionTestCommandStub(message), mComponent(componentImpl)
            {
            }

            ~PokePermissionTestCommand() override { }

            PokePermissionTestCommandStub::Errors execute() override
            {
                if (UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_ARSON_POKE_PERMISSION_TEST) == false)
                {
                    return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
                }

                char8_t msg[1024] = "User has permission to call this RPC.";
                mResponse.setMessage(msg);
                return ERR_OK;
            }

        private:
            ArsonSlave* mComponent;
        };

        // static
        DEFINE_POKEPERMISSIONTEST_CREATE();

    } // namespace Arson
} // namespace Blaze

