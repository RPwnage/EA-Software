/*! ************************************************************************************************/
/*!
\file getscenariovariants_slavecommand.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/getscenariovariants_stub.h"
#include "gamemanagerslaveimpl.h"

namespace Blaze
{
    namespace GameManager
    {
        /*! ************************************************************************************************/
        /*! \brief Update a game's external session image.
        ***************************************************************************************************/
        class GetScenarioVariantsCommand : public GetScenarioVariantsCommandStub
        {
        public:
            GetScenarioVariantsCommand(Message* message, GetScenarioVariantsRequest *request, GameManagerSlaveImpl* componentImpl)
                : GetScenarioVariantsCommandStub(message, request), mComponent(*componentImpl)
            {
            }
            ~GetScenarioVariantsCommand() override
            {
            }

        private:

            GetScenarioVariantsCommandStub::Errors execute() override
            {
                return commandErrorFromBlazeError(mComponent.getScenarioManager().getScenarioVariants(mRequest, mResponse));
            }

        private:
            GameManagerSlaveImpl &mComponent;
        };

        DEFINE_GETSCENARIOVARIANTS_CREATE()

    } // namespace GameManager
} // namespace Blaze
