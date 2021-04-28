/*! ************************************************************************************************/
/*!
    \file getusersetgamelistsubscription_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "gamemanager/rpc/gamemanagerslave/getusersetgamelistsubscription_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"


namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Start a matchmaking session; on success we register for async notifications from the session.
    ***************************************************************************************************/
    class GetUserSetGameListSubscriptionCommand : public GetUserSetGameListSubscriptionCommandStub
    {
    public:
        GetUserSetGameListSubscriptionCommand(Message* message, GetUserSetGameListSubscriptionRequest* request, GameManagerSlaveImpl* componentImpl)
            :   GetUserSetGameListSubscriptionCommandStub(message, request), mComponent(*componentImpl), mMessage(message)
        {
        }

        ~GetUserSetGameListSubscriptionCommand() override {}

    private:
        
        GetUserSetGameListSubscriptionCommandStub::Errors execute() override
        {
            MatchmakingCriteriaError error;
            BlazeRpcError blazeError = mComponent.getGameBrowser().processGetUserSetGameListSubscription(mRequest, mResponse, error, mMessage);
            return commandErrorFromBlazeError(blazeError);
        }

    private:
        GameManagerSlaveImpl &mComponent;
        Message* mMessage;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETUSERSETGAMELISTSUBSCRIPTION_CREATE()

} // namespace GameManager
} // namespace Blaze
