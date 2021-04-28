/*! ************************************************************************************************/
/*!
\file setuserscenariovant_slavecommand.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/setuserscenariovariant_stub.h"
#include "gamemanagerslaveimpl.h"

namespace Blaze
{
    namespace GameManager
    {
        /*! ************************************************************************************************/
        /*! \brief Update a game's external session image.
        ***************************************************************************************************/
        class SetUserScenarioVariantCommand : public SetUserScenarioVariantCommandStub
        {
        public:
            SetUserScenarioVariantCommand(Message* message, SetUserScenarioVariantRequest *request, GameManagerSlaveImpl* componentImpl)
                : SetUserScenarioVariantCommandStub(message, request), mComponent(*componentImpl)
            {
            }
            ~SetUserScenarioVariantCommand() override
            {
            }

        private:

            SetUserScenarioVariantCommandStub::Errors execute() override
            {
                return commandErrorFromBlazeError(mComponent.setUserScenarioVariant(mRequest));
            }

        private:
            GameManagerSlaveImpl &mComponent;
        };

        DEFINE_SETUSERSCENARIOVARIANT_CREATE()

    } // namespace GameManager
} // namespace Blaze
