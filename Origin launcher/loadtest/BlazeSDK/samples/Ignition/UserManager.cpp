
#include "Ignition/UserManager.h"


namespace Ignition
{


UserManager::UserManager(uint32_t userIndex)
    : LocalUserUiBuilder("UserManager", userIndex)
{
    getActions().add(&getActionClaimPrimaryLocalUser());

}

UserManager::~UserManager()
{
}


void UserManager::onAuthenticated()
{
    setIsVisible(true);
}

void UserManager::onDeAuthenticated()
{
    setIsVisible(false);

    getWindows().clear();
}

void UserManager::initActionClaimPrimaryLocalUser(Pyro::UiNodeAction &action)
{
    action.setText("Claim Primary Local User");
    action.setDescription("Sets this user as the PrimaryLocalUser.");
}

void UserManager::actionClaimPrimaryLocalUser(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
     gBlazeHub->getUserManager()->setPrimaryLocalUser(getUserIndex());
}

}
