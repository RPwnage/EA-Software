/*! ************************************************************************************************/
/*!
    \file getgamelistsubscription_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/getgamelistsubscription_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"


namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! 
        \brief "Get a list of games matching the supplied criteria.  
        Games are returned via async NotifyGameListUpdate notifications
    */
    /*! ***********************************************************************************************/
    class GetGameListSubscriptionCommand : public GetGameListSubscriptionCommandStub
    {
    public:
        GetGameListSubscriptionCommand(Message* message, GetGameListRequest* request, GameManagerSlaveImpl* componentImpl)
            :   GetGameListSubscriptionCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~GetGameListSubscriptionCommand() override {}

    private:
        
        GetGameListSubscriptionCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.getGameBrowser().processCreateGameList(mRequest, GAME_BROWSER_LIST_SUBSCRIPTION, mResponse, mErrorResponse, 0, nullptr); 
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETGAMELISTSUBSCRIPTION_CREATE()

} // namespace GameManager
} // namespace Blaze
