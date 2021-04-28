
#include "Ignition/PlatformFeatures.h"
#include "Ignition/GameManagement.h"
#include "DirtySDK/dirtysock/dirtyuser.h"

#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
#include <np/np_npid.h>
#endif

namespace Ignition
{


PlatformFeatures::PlatformFeatures(uint32_t userIndex)
    : LocalUserUiBuilder("PlatformFeatures", userIndex)
{
    #if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        getActions().add(&getActionAcceptInvite());
        getActions().add(&getActionShowReceiveDialog());
        mJoinGame = false;
        mFetchMetadata = false;
        mFetchInvites = false;
    #endif
}

PlatformFeatures::~PlatformFeatures()
{
}

void PlatformFeatures::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    #if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    //check that we have a valid session manager that has zero pending request count 'gprc'
    if ((gConnApiAdapter != nullptr) && (gConnApiAdapter->getDirtySessionManagerRefT() != nullptr) && (DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gprc', nullptr, 0) == 0))
    {
        char strAcceptedInviteID[64];
        char strAcceptedSessionID[64];
        DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gsii', strAcceptedInviteID, sizeof(strAcceptedInviteID));
        DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gssi', strAcceptedSessionID, sizeof(strAcceptedSessionID));

        //check to see if the invite list is out of date
        if ((DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'giud', getUserIndex(), 0, 0, nullptr, 0) == FALSE))
        {
            //get a new list of invites
            mFetchInvites = true;
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'ginv', getUserIndex(), 0, nullptr);
        }

        //if we're getting a new list and we're done getting it, get the meta data for each item in the list
        else if (mFetchInvites)
        {
            mFetchInvites = false;
            int32_t iNumInvites = DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'ginv', getUserIndex(), 0, 0, nullptr, 0);

            //we have invites and we want to get the extra meta data before displaying them
            for (int32_t i = 0; i < iNumInvites; i++)
            {
                DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gimd', getUserIndex(), i, nullptr);
            }
            mFetchMetadata = true;
        }

        //we have finished receiving the meta data about our invites, generate a display so the user can select one to join
        else if (mFetchMetadata)
        {
            mFetchMetadata = false;
            char strMessage[512];
            char strFromUser[32];
            char strSessionId[64];
            char strAcceptButtonText[64];
            DirtyUserT user;
            Blaze::string strInvitesList;

            strFromUser[0] = 0;
            getActionAcceptInvite().getParameters().clear();
            int32_t iNumInvites = DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'ginv', getUserIndex(), 0, 0, nullptr, 0);

            for (int32_t i = 0; i < iNumInvites; i++)
            {
                DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'gims', getUserIndex(), i, 0, strMessage, sizeof(strMessage));
                DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'gisi', getUserIndex(), i, 0, strSessionId, sizeof(strSessionId));
                DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'giun', getUserIndex(), i, 0, &user, sizeof(user));

                SceNpId npid;
                DirtyUserToNativeUser(&npid, sizeof(npid), &user);
                ds_strnzcpy(strFromUser, npid.handle.data, sizeof(strFromUser));
                strInvitesList.append_sprintf("%d - %s - %s ,", i, strFromUser, strMessage);
            }

            //new parameters to accept invite, make selection easier 
            Pyro::UiNodeParameter &inviteParam = getActionAcceptInvite().getParameters().addString("Invite", "", "Select Invite", strInvitesList.c_str(), "Select Invite to join.");
            inviteParam.setIncludePreviousValues(false);

            //new accept invite button text
            blaze_snzprintf(strAcceptButtonText, sizeof(strAcceptButtonText), "Accept Invite (%d)", iNumInvites);
            getActionAcceptInvite().setText(strAcceptButtonText);
        }

        //detect that the session receive dialog has been used and an invite selected
        else if (strAcceptedInviteID[0] != '\0')
        {
            char strInviteId[64];
            char strSessionId[64];

            //look for the invite in our invite list
            int32_t iNumInvites = DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'ginv', getUserIndex(), 0, 0, nullptr, 0);

            for (int32_t iInviteIndex = 0; iInviteIndex < iNumInvites; iInviteIndex++)
            {
                DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'giii', getUserIndex(), iInviteIndex, 0, strInviteId, sizeof(strInviteId));
                if (strcmp(strInviteId, strAcceptedInviteID) == 0)
                {
                    //now that we've consumed the selection clear it
                    DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'cssi', 0, 0, nullptr);
            
                    //this is the invite the user decided to accept
                    //join the session

                    //get the session id for the selected invite
                    DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'gisi', getUserIndex(), iInviteIndex, 0, strSessionId, sizeof(strSessionId));
                    
                    // the accepted session id should be the same as the one in the invite
                    BlazeAssert(strcmp(strSessionId, strAcceptedSessionID) == 0);

                    //request data about the session, looking for the GameId eventually obtained via 'gpli', and game type obtained via 'gpgt'.
                    DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gses', getUserIndex(), 0, strAcceptedSessionID);
                    DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gdat', getUserIndex(), 0, nullptr);
                    //Similarly request changeable data for the game mode, later obtained via 'gpgm'
                    DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gchg', getUserIndex(), 0, nullptr);

                    //mark the invite as used
                    DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'uinv', getUserIndex(), iInviteIndex, nullptr);
        
                    //tell the idler to join the blaze game once the session join has completed
                    mJoinGame = true;
                    
                    break;
                }
            }
            
        }

        //detect that the session is selected through a different manner, boot event etc
        else if (strAcceptedSessionID[0] != '\0')
        {
            //now that we've consumed the selection clear it
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'cssi', 0, 0, nullptr);

            //request data about the session, looking for the GameId eventually obtained via 'gpli'
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gses', getUserIndex(), 0, strAcceptedSessionID);
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gdat', getUserIndex(), 0, nullptr);
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gchg', getUserIndex(), 0, nullptr);

            //tell the idler to join the blaze game once the session join has completed
            mJoinGame = true;
        }

        //we want to join the blaze game
        else if (mJoinGame)
        {
            mJoinGame = false;
            int64_t blazeGameId;
            int64_t gameType;
            char8_t strGameMode[Blaze::Collections::MAX_ATTRIBUTEVALUE_LEN];
            memset(strGameMode, 0, sizeof(strGameMode));

            DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gpli', &blazeGameId, sizeof(blazeGameId));
            DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gpgt', &gameType, sizeof(gameType));
            DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gpgm', strGameMode, sizeof(strGameMode));
            
            //set join params that may be based on game mode, type
            Blaze::Collections::AttributeMap playerAttribs;
            Blaze::GameManager::RoleNameToPlayerMap joiningRoles;
            Ignition::GameManagement::setDefaultRole(joiningRoles, GameManagement::getInitialPlayerRole(strGameMode, (Blaze::GameManager::GameType)gameType));
            playerAttribs["type"] = getInitialPlayerAttribute(strGameMode, (Blaze::GameManager::GameType)gameType);

            gBlazeHubUiDriver->getGameManagement().runJoinGameByIdWithInvite(
                blazeGameId,
                nullptr,
                Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
                Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, &joiningRoles,
                Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, &playerAttribs);
        }
    }
    #elif defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    int32_t iSceResult;

    // if the dialog not running return
    if ((scePlayerInvitationDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_RUNNING) || (scePlayerInvitationDialogUpdateStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED))
    {
        return;
    }

    // get the result
    ScePlayerInvitationDialogResult Result;
    ds_memclr(&Result, sizeof(Result));
    if ((iSceResult = scePlayerInvitationDialogGetResult(&Result)) != SCE_OK)
    {
        gBlazeHubUiDriver->showMessage_va("scePlayerInvitationDialogGetResult failed (err=%s)", DirtyErrGetName(iSceResult));
    }

    #endif
}

const char8_t* PlatformFeatures::getInitialPlayerAttribute(const char8_t* strGameMode, Blaze::GameManager::GameType gameType)
{
    if (gameType == Blaze::GameManager::GAME_TYPE_GROUP)
    {
        return "LobbyUser";
    }
    else if (strcmp(strGameMode, "Heroes"))
    {
        return "Hero";
    }
    else if (strcmp(strGameMode, "Hero Hunt"))
    {
        return "Hunter";
    }
    return "";
}


void PlatformFeatures::onAuthenticated()
{
    setIsVisible(true);
    gBlazeHub->addIdler(this, "PlatformFeatures");
}

void PlatformFeatures::onDeAuthenticated()
{
    setIsVisible(false);
    getWindows().clear();
    gBlazeHub->removeIdler(this);
}

#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
void PlatformFeatures::initActionAcceptInvite(Pyro::UiNodeAction &action)
{
    action.setText("Accept Invite (0)");
    action.setDescription("View and join an invite.");
}

void PlatformFeatures::actionAcceptInvite(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if ((gConnApiAdapter != nullptr) && (gConnApiAdapter->getDirtySessionManagerRefT() != nullptr))
    {
        int32_t iNumInvites = DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'ginv', getUserIndex(), 0, 0, nullptr, 0);

        if (iNumInvites > 0)
        {
            char strSessionId[64];
            char strInviteIndex[8];
            strInviteIndex[0] = parameters["Invite"].getAsString()[0];
            strInviteIndex[1] = '\0';
            int32_t iInviteIndex = atoi(strInviteIndex);

            //get the session id for the selected invite
            DirtySessionManagerStatus2(gConnApiAdapter->getDirtySessionManagerRefT(), 'gisi', getUserIndex(), iInviteIndex, 0, strSessionId, sizeof(strSessionId));

            //request data about the session, looking for the GameId eventually obtained via 'gpli'
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gses', getUserIndex(), 0, strSessionId);
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'gdat', getUserIndex(), 0, strSessionId);

            //mark the invite as used
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'uinv', getUserIndex(), iInviteIndex, nullptr);
        
            //tell the idler to join the blaze game once the session join has completed
            mJoinGame = true;
        }
    }
}

void PlatformFeatures::initActionShowReceiveDialog(Pyro::UiNodeAction &action)
{
    action.setText("Recieve Invite Dialog");
    action.setDescription("Open Session Receive Dialog");
}

void PlatformFeatures::actionShowReceiveDialog(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if ((gConnApiAdapter != nullptr) && (gConnApiAdapter->getDirtySessionManagerRefT() != nullptr))
    {
        //show the recieve invite dialog, the user's selection can later be detected with gsii or gssi
        DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'osrd', getUserIndex(), 0, nullptr);
    }
}
#endif
}
