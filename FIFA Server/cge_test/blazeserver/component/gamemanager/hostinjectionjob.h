/*! ************************************************************************************************/
/*!
    \file   hostinjectionjob.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_HOSTINJECTIONJOB_H
#define BLAZE_GAMEMANAGER_HOSTINJECTIONJOB_H

#include "gamemanager/tdf/gamemanager.h"

#include "gamemanager/matchmaker/matchmaker.h"

namespace Blaze
{
namespace GameManager
{
    class GameSessionMaster;
    class GameManagerMasterImpl;

class HostInjectionJob
{
    NON_COPYABLE(HostInjectionJob);

public:

    // schedules job timeout after saving off the info
    HostInjectionJob(GameId gameId, const ChooseHostForInjectionRequest &hostInjectionRequest);

    // cancel timer, if valid
     ~HostInjectionJob();
    
    void start();

private:
    
    void attemptHostInjection();
    void terminateHostInjectionJob(BlazeRpcError joinError);

    GameId mGameId;
    ChooseHostForInjectionRequest mChooseHostForInjectionRequest;
    Fiber::FiberHandle mFiberHandle;
    static const float EXACT_MATCH_THRESHOLD;
};

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_HOSTINJECTIONJOB_H
