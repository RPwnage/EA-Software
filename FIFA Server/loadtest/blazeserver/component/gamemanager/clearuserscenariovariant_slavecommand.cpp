/*! ************************************************************************************************/
/*!
\file clearuserscenariovariant_slavecommand.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/clearuserscenariovariant_stub.h"
#include "gamemanagerslaveimpl.h"

namespace Blaze
{
    namespace GameManager
    {
        /*! ************************************************************************************************/
        /*! \brief Update a game's external session image.
        ***************************************************************************************************/
        class ClearUserScenarioVariantCommand : public ClearUserScenarioVariantCommandStub
        {
        public:
            ClearUserScenarioVariantCommand(Message* message, ClearUserScenarioVariantRequest *request, GameManagerSlaveImpl* componentImpl)
                : ClearUserScenarioVariantCommandStub(message, request), mComponent(*componentImpl)
            {
            }
            ~ClearUserScenarioVariantCommand() override
            {
            }

        private:

            ClearUserScenarioVariantCommandStub::Errors execute() override
            {
                return commandErrorFromBlazeError(mComponent.clearUserScenarioVariant(mRequest));
            }

        private:
            GameManagerSlaveImpl &mComponent;
        };

        DEFINE_CLEARUSERSCENARIOVARIANT_CREATE()

    } // namespace GameManager
} // namespace Blaze
