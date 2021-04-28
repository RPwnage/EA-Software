//  *************************************************************************************************
//
//   File:    playerinstance.h
//
//   Author:  systest (put login = false in the config file)
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef PLAYERINSTANCE_H
#define PLAYERINSTANCE_H

#include "EABase/eabase.h"
#include "framework/connection/inetaddress.h"
#include "framework/logger.h"
#include "tools/stress/stressmodule.h"
#include "tools/stress/stressinstance.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "EAStdC/EAString.h"

//   Authentication
#include "authentication/tdf/authentication.h"

#include "playermodule.h"
//#include "player.h"
#include "reference.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class StressPlayerInfo;
class Player;

//   Verify if any async functions need to be registered.
class PlayerInstance: public StressInstance, public AsyncHandler {
        NON_COPYABLE(PlayerInstance);

    public:
        PlayerInstance(StressModule* owner, StressConnection* connection, Login* login, bool isSpartaSignalMock = false);
        virtual                     ~PlayerInstance();

        //  Player will be selected from random distribution
        Player*             mPlayer;
        void                createPlayerData();

    protected:
        //   Virtual methods
        virtual BlazeRpcError       execute();
        virtual void                onDisconnected();
        virtual void                onAsync(uint16_t component, uint16_t type, RawBuffer* payload);
        void                        reconnectHandler();
        Blaze::Stress::StressPlayerInfo*  mPlayerData;
        bool                        mIsSpartaSignalMock;
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   PLAYERINSTANCE_H
