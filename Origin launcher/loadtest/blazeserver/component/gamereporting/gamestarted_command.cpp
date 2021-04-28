/*************************************************************************************************/
/*!
    \file   gamestarted_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamereporting/rpc/gamereportingslave/gamestarted_stub.h"
#include "gamereporting/tdf/gamereportingevents_server.h"
#include "gamereportingslaveimpl.h"

namespace Blaze
{
namespace GameReporting
{

/*************************************************************************************************/
/*!
    \class GameStartedCommand

    Create a game report to track a started game.
*/
/*************************************************************************************************/

class GameStartedCommand : public GameStartedCommandStub
{
public:
    GameStartedCommand(Message *message, Blaze::GameManager::StartedGameInfo *request, GameReportingSlaveImpl *componentImpl)
    :   GameStartedCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GameStartedCommand() override
    {
    }

private:
    GameStartedCommandStub::Errors execute() override;

private:
    GameReportingSlaveImpl *mComponent;
};
DEFINE_GAMESTARTED_CREATE()

GameStartedCommandStub::Errors GameStartedCommand::execute()
{
    if (mRequest.getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID)
    {
        WARN_LOG("[GameStartedCommand]: ignoring game report ("
            << mRequest.getGameReportName() << ") with invalid id");
        return ERR_SYSTEM;
    }

    // total number of games started (on this slave)
    mComponent->getMetrics().mGames.increment();

    TRACE_LOG("[GameStartedCommand:]: find/create in REDIS game reports for game report ("
        << mRequest.getGameReportName() << ") with id " << mRequest.getGameReportingId());

    OwnedSliverRef sliverRef = mComponent->getReportCollateDataTable().getSliverOwner().getOwnedSliver(GetSliverIdFromSliverKey(mRequest.getGameReportingId()));
    if (sliverRef == nullptr)
    {
        // This should never happen, SliverManager ensures that creation RPCs never get routed to an owner that has no ready slivers
        ERR_LOG("GameStartedCommand.execute: failed to obtain owned sliver ref.");
        return ERR_SYSTEM;
    }

    Sliver::AccessRef sliverAccess;
    if (!sliverRef->getPrioritySliverAccess(sliverAccess))
    {
        ERR_LOG("GameStartedCommand.execute: Could not get priority sliver access for SliverId(" << sliverRef->getSliverId() << ")");
        return ERR_SYSTEM;
    }

    GameReportMasterPtrByIdMap::insert_return_type inserted = mComponent->mGameReportMasterPtrByIdMap.insert(mRequest.getGameReportingId());
    if (!inserted.second)
    {
        // unexpected -- log an error, ignore this request and carry on
        ERR_LOG("[GameStartedCommand]: ignoring game report (" << mRequest.getGameReportName() << ") because game report ("
            << inserted.first->second->getData()->getStartedGameInfo().getGameReportName() << ") already exists with id " << mRequest.getGameReportingId());
        return ERR_OK;
    }
    GameReportMasterPtr gameReportMaster = BLAZE_NEW GameReportMaster(mRequest.getGameReportingId(), sliverRef);
    inserted.first->second = gameReportMaster;

    // Initialize the game report with some game info
    mRequest.copyInto(gameReportMaster->getData()->getStartedGameInfo());

    // When this is set, it will trigger the GameReportMasterPtr intrusive_ptr release function to commit the changes.
    gameReportMaster->setAutoCommitOnRelease();

    // the custom report collator will be created/destroyed on demand as reports are received/consumed

    TRACE_LOG("[GameStartedCommand]: created game report ("
        << mRequest.getGameReportName() << ") with id " << mRequest.getGameReportingId());

    return ERR_OK;
}

} // namespace GameReporting
} // namespace Blaze
