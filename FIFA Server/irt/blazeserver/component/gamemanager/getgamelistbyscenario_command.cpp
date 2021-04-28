/*! ************************************************************************************************/
/*!
\file getgamelistbyscenario_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/getgamelistbyscenario_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getgamebrowserattributesconfig_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{
    namespace GameManager
    {
        /*! ************************************************************************************************/
        /*! \brief Get a list of games matching the supplied criteria.
        Games are returned via async NotifyGameListUpdate notifications
        */
        /*! ***********************************************************************************************/
        class GetGameListByScenarioCommand : public GetGameListByScenarioCommandStub
        {
        public:
            GetGameListByScenarioCommand(Message* message, GetGameListScenarioRequest* request, GameManagerSlaveImpl* componentImpl)
                : GetGameListByScenarioCommandStub(message, request), mComponent(*componentImpl)
            {
            }
            virtual ~GetGameListByScenarioCommand() {}
        private:
            GetGameListByScenarioCommandStub::Errors execute()
            {
                BlazeRpcError err = mComponent.getGameBrowser().processCreateGameListForScenario(mRequest, mResponse, mErrorResponse, getMsgNum());
                return commandErrorFromBlazeError(err);
            }
        private:
            GameManagerSlaveImpl &mComponent;
        };
        //! \brief static creation factory method for command (from stub).
        DEFINE_GETGAMELISTBYSCENARIO_CREATE()

            /*! ************************************************************************************************/
            /*! \brief Get information about the game browser scenario config;
            ***************************************************************************************************/
            class GetGameBrowserAttributesConfigCommand : public GetGameBrowserAttributesConfigCommandStub
        {
        public:
            GetGameBrowserAttributesConfigCommand(Message* message, GameManagerSlaveImpl* componentImpl)
                : GetGameBrowserAttributesConfigCommandStub(message), mComponent(*componentImpl)
            {
            }

            virtual ~GetGameBrowserAttributesConfigCommand() {}

        private:

            GetGameBrowserAttributesConfigCommand::Errors execute()
            {
                BlazeRpcError err = mComponent.getGameBrowser().getGameBrowserScenarioAttributesConfig(mResponse);
                return commandErrorFromBlazeError(err);
            }

        private:
            GameManagerSlaveImpl &mComponent;
        };

        //! \brief static creation factory method for command (from stub)
        DEFINE_GETGAMEBROWSERATTRIBUTESCONFIG_CREATE()

    } // namespace GameManager
} // namespace Blaze