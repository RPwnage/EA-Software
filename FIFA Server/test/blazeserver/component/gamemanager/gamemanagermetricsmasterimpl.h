/*************************************************************************************************/
/*!
    \file   gamemanagermetricsmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_GAMEMANAGERMETRICSMASTERIMPL_H
#define BLAZE_GAMEMANAGER_GAMEMANAGERMETRICSMASTERIMPL_H

/*** Include files *******************************************************************************/
#include "gamemanager/rpc/gamemanagermetricsmaster_stub.h"

namespace Blaze
{
namespace GameManager
{

class GameManagerMetricsMasterImpl : public GameManagerMetricsMasterStub
{
public:
    GameManagerMetricsMasterImpl() {};
    ~GameManagerMetricsMasterImpl() override {};
};

}
}

#endif // BLAZE_GAMEMANAGER_GAMEMANAGERMETRICSMASTERIMPL_H