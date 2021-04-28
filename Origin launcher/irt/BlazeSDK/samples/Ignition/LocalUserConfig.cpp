#include "Ignition/LocalUserConfig.h"
#include "EAStdC/EASprintf.h"

namespace Ignition
{
LocalUserConfig::LocalUserConfig():
    BlazeHubUiBuilder("Local User Config"),
    mSelectedIndices(Blaze::MEM_GROUP_DEFAULT, MEM_NAME(Blaze::MEM_GROUP_DEFAULT, "Ignition::mSelectedIndices"))
{
    gBlazeHub->addUserStateEventHandler(this);

    setVisibility(Pyro::ItemVisibility::ADVANCED);

    getActions().add(&getActionUpdateLocalUserIndices());
}

LocalUserConfig::~LocalUserConfig()
{
    gBlazeHub->removeUserStateEventHandler(this);

    mSelectedIndices.clear();
}

void LocalUserConfig::onAuthenticated(uint32_t userIndex)
{
    initActionUpdateLocalUserIndices(getActionUpdateLocalUserIndices());
}

void LocalUserConfig::onDeAuthenticated(uint32_t userIndex)
{
    initActionUpdateLocalUserIndices(getActionUpdateLocalUserIndices());
}

void LocalUserConfig::onAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(true);
}

void LocalUserConfig::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    getWindows().clear();
}

void LocalUserConfig::initActionUpdateLocalUserIndices(Pyro::UiNodeAction &action)
{
    action.setText("Update Local User Group");
    action.setDescription("Join a subset of logged-in local users into a local user group");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);

    Pyro::UiNodeParameterStruct &parameters = action.getParameters();
    parameters.resetToDefault();

    for (uint32_t index = 0; index < gBlazeHub->getNumUsers(); ++index)
    {
        const Blaze::UserManager::LocalUser* localUser = gBlazeHub->getUserManager()->getLocalUser(index);
        if (localUser != nullptr)
        {
            Pyro::UiNodeParameter& param = parameters.addBool(localUser->getName(), mConnUserIndexUpdateSet.find(index) != mConnUserIndexUpdateSet.end(),
                localUser->getName(), "Join this user into Local User Group");
            param.setIncludePreviousValues(false);
        }
    }
}

void LocalUserConfig::actionUpdateLocalUserIndices(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    mSelectedIndices.clear();

    for (uint32_t index = 0; index < gBlazeHub->getNumUsers(); ++index)
    {
        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(index);
        if (localUser != nullptr)
        {
            if (parameters[localUser->getName()])
            {
                mSelectedIndices.push_back(index);
            }
        }
    }

    Blaze::UserManager::LocalUserGroup::UpdateLUGroupCb cb(this, &LocalUserConfig::updateLUGroupCb);
    gBlazeHub->getUserManager()->getLocalUserGroup().updateLocalUserGroup(mSelectedIndices, cb);
}

void LocalUserConfig::updateLUGroupCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::UpdateLocalUserGroupResponse *response)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    mConnUserIndexUpdateSet.clear();

    Blaze::ConnectionUserIndexList::const_iterator itr, end;
    for (itr = response->getConnUserIndexList().begin(), end = response->getConnUserIndexList().end(); itr != end; ++itr)
    {
        mConnUserIndexUpdateSet.insert(*itr);
    }

    initActionUpdateLocalUserIndices(getActionUpdateLocalUserIndices());

    REPORT_BLAZE_ERROR(blazeError);
}

}
