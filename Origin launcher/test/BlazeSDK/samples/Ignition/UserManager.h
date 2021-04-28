
#ifndef USERMANAGER_H
#define USERMANAGER_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/usermanager/usermanager.h"

namespace Ignition
{

class UserManager :
    public LocalUserUiBuilder
{
    public:
        

        UserManager(uint32_t userIndex);
        virtual ~UserManager();

        virtual void onAuthenticated();
        virtual void onDeAuthenticated();

    private:

        PYRO_ACTION(UserManager, ClaimPrimaryLocalUser);

};

}

#endif //USERMANAGER_H
