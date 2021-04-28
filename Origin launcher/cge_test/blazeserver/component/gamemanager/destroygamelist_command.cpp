/*! ************************************************************************************************/
/*!
    \file destroygamelist_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/destroygamelist_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"


namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Register a dynamic server creator
    ***************************************************************************************************/
    class DestroyGameListCommand : public DestroyGameListCommandStub
    {
    public:
        DestroyGameListCommand(Message* message, DestroyGameListRequest* request, GameManagerSlaveImpl* componentImpl)
            :   DestroyGameListCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~DestroyGameListCommand() override {}

    private:
        
        DestroyGameListCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.getGameBrowser().processDestroyGameList(mRequest);
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_DESTROYGAMELIST_CREATE()

} // namespace GameManager
} // namespace Blaze
