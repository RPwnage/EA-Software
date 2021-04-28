/*! ************************************************************************************************/
/*!
    \file getgameliststreaming_command.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEBROWSERSERVICE_V1_GETGAMELISTSTREAMING_COMMAND_H
#define BLAZE_GAMEBROWSERSERVICE_V1_GETGAMELISTSTREAMING_COMMAND_H

#include "component/gamebrowserservice/rpc/gamebrowserserviceslave/getgameliststreaming_stub.h"
#include "gamemanager/gamebrowser/gamebrowserlistowner.h"

#include "framework/system/timerqueue.h"
namespace Blaze
{
namespace GameManager
{
    class NotifyGameListUpdate;
}

namespace GameBrowserService
{
namespace v1
{

class GetGameListStreamingCommand : public GetGameListStreamingCommandStub
{
public:
    using GetGameListStreamingCommandStub::GetGameListStreamingCommandStub;
    
    ~GetGameListStreamingCommand();
    void onProcessRequest(const GameBrowserListRequest& request) override;
    void onCleanup(bool cancelled) override;

    void done();
    void onListUpdate(const Blaze::GameManager::NotifyGameListUpdate& listUpdate);
    void onListDestroy();
private:
    Blaze::GameManager::IGameBrowserListOwnerPtr mListOwner;
    Blaze::GameManager::GameBrowserListId mListId = 0;
    bool mGbrRequestInProgress = false;

    // We have too many different ways the list can get cleaned up as it's a race between different async systems
    // and quirkiness of the underlying gamebrowser api itself. Hence, the few booleans below to keep things safe.

    // mCleanedUp tracks when the grpc lib is done with this rpc. It can be due to client abort, server finish, premature
    // failure within Authentication etc. We should not be communicating with grpc library once we get this.
    bool mCleanedUp = false;
    // mSignaledDone tracks when the rpc layer is done (gracefully). Once signaled, we no longer need to communicate with 
    // the grpc lib. But if the underlying List is getting destroyed before the rpc layer signaled 'done', that is communicated as error.     
    bool mSignaledDone = false;
    // mListDestroyedByGbr tracks when the underlying GameBrowser System is done.
    bool mListDestroyedByGbr = false;

    TimerId mCleanupTimer = INVALID_TIMER_ID;

};

} // namespace v1
} // namespace GameBrowserService
} // namespace Blaze

#endif // BLAZE_GAMEBROWSERSERVICE_V1_GETGAMELISTSTREAMING_COMMAND_H
