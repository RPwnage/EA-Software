/*! ************************************************************************************************/
/*!
    \file getgamelistsnapshot_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/getgamelistsnapshot_stub.h"
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
    class GetGameListSnapshotCommand : public GetGameListSnapshotCommandStub
    {
    public:
        GetGameListSnapshotCommand(Message* message, GetGameListRequest* request, GameManagerSlaveImpl* componentImpl)
            :   GetGameListSnapshotCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~GetGameListSnapshotCommand() override {}

    private:
        
        GetGameListSnapshotCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.getGameBrowser().processCreateGameList(mRequest, GameManager::GAME_BROWSER_LIST_SNAPSHOT, mResponse, mErrorResponse, getMsgNum());
            return commandErrorFromBlazeError(err);
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETGAMELISTSNAPSHOT_CREATE()

} // namespace GameManager
} // namespace Blaze
