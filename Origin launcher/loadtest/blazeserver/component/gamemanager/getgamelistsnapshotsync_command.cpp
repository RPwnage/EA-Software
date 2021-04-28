/*! ************************************************************************************************/
/*!
    \file getgamelistsnapshotsync_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/getgamelistsnapshotsync_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"


namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief Get a list of games matching the supplied criteria.  Games are returned synchronously.
    ***************************************************************************************************/
    class GetGameListSnapshotSyncCommand : public GetGameListSnapshotSyncCommandStub
    {
    public:
        GetGameListSnapshotSyncCommand(Message* message, GetGameListRequest* request, GameManagerSlaveImpl* componentImpl)
            :   GetGameListSnapshotSyncCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~GetGameListSnapshotSyncCommand() override {}

    private:
        
        GetGameListSnapshotSyncCommandStub::Errors execute() override
        {
            MatchmakingCriteriaError error;
            uint32_t listCapacity = mRequest.getListCapacity();
            if (listCapacity != GAME_LIST_CAPACITY_UNLIMITED && listCapacity > mComponent.getGameBrowser().getMaxGameListSyncSize())
                return GAMEBROWSER_ERR_EXCEED_MAX_SYNC_SIZE;

            return commandErrorFromBlazeError(mComponent.getGameBrowser().processGetGameListSnapshotSync(mRequest, mResponse, error));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_GETGAMELISTSNAPSHOTSYNC_CREATE()

} // namespace GameManager
} // namespace Blaze
