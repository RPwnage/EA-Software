#include "Ignition/Friends.h"
#if !defined(EA_PLATFORM_NX)
#include "DirtySDK/voip/voipblocklist.h"
#endif

namespace Ignition
{

const char8_t* Friends::DEFAULT_FRIENDS_APPLICATION_KEY = "WEBSDKSAMPLEKEY";
const char8_t* Friends::DEFAULT_FRIENDS_API_VERSION = "2";
const char8_t* Friends::DEFAULT_FRIENDS_HOSTNAME = "integration.friends.gs.ea.com";
const int      Friends::DEFAULT_FRIENDS_HOST_PORT = 443;
const bool     Friends::DEFAULT_FRIENDS_SECURE = true;

Friends::Friends(uint32_t userIndex)
    : LocalUserUiBuilder("Friends", userIndex)
{
    //Initialize the Friends component
    
    Blaze::BlazeSender::ServerConnectionInfo serverInfo(DEFAULT_FRIENDS_HOSTNAME, DEFAULT_FRIENDS_HOST_PORT, DEFAULT_FRIENDS_SECURE);
    gBlazeHub->getComponentManager()->getFriendsComponent()->createComponent(gBlazeHub, serverInfo, Blaze::Encoder::JSON);

    getActions().add(&getActionMuteUser());
    getActions().add(&getActionUnmuteUser());
    getActions().add(&getActionCheckMuteStatus());
    getActions().add(&getActionSetAccessToken());
}

Friends::~Friends()
{
}


void Friends::onAuthenticated()
{
    setIsVisible(true);
}

void Friends::onDeAuthenticated()
{
    setIsVisible(false);
    getWindows().clear();


#if !defined(EA_PLATFORM_NX)
    // clear the mute list in DirtySDK since the local user is gone
    if (VoipGetRef() != nullptr)
        VoipBlockListClear(VoipGetRef(), getUserIndex());
#endif

}

void Friends::initActionSetAccessToken(Pyro::UiNodeAction &action)
{
    action.setText("Nucleus Access Token");
    action.setDescription("Cache the nucleus token which is valid for 60 minutes.");
    action.getParameters().addString("accessToken", "Access Token", "Access Token", "Nucleus access token");
}

void Friends::actionSetAccessToken(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    getAccessToken().clear();
    eastl::string token((const char8_t *)parameters["accessToken"]);
    setAccessToken("Bearer " + token);
}

void Friends::initActionCheckMuteStatus(Pyro::UiNodeAction &action)
{
    action.setText("Check Mute Status");
    action.setDescription("This call will return the requesting user's list of muted users.");
    action.getParameters().addInt32("start", 0, "Start", "starting record index (used if no ID list is specified).");
    action.getParameters().addInt32("count", 100, "Count", "maximum number of records to return (used if no ID list is specified).");
    action.getParameters().addBool("names", false, "Names", "return persona names with records (used if no ID list is specified).");
}

void Friends::actionCheckMuteStatus(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if (localUser == nullptr)
    {
        if (getUiDriver() != nullptr)
            getUiDriver()->showMessage_va(Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr");
        return;
    }

    Blaze::Social::Friends::CheckMuteStatusRequest checkMuteStatusRequest;

    checkMuteStatusRequest.getAuthCredentials().setApiVersion(DEFAULT_FRIENDS_API_VERSION);
    checkMuteStatusRequest.getAuthCredentials().setApplicationKey(DEFAULT_FRIENDS_APPLICATION_KEY);
    checkMuteStatusRequest.getAuthCredentials().setAuthorization(getAccessToken().c_str());
    checkMuteStatusRequest.setNucleusId(localUser->getNucleusAccountId());
    checkMuteStatusRequest.setCount(parameters["count"]);
    checkMuteStatusRequest.setStart(parameters["start"]);
    checkMuteStatusRequest.setNames(parameters["names"]);

    Blaze::Social::Friends::FriendsComponent * component = gBlazeHub->getComponentManager()->getFriendsComponent();
    component->checkMuteStatus(checkMuteStatusRequest, Blaze::MakeFunctor(this, &Friends::CheckMuteStatusCb));
}

void Friends::CheckMuteStatusCb(const Blaze::Social::Friends::PagedListUser *response, Blaze::BlazeError error, Blaze::JobId jobId)
{

#if !defined(EA_PLATFORM_NX)
    if (error == Blaze::ERR_OK)
    {

        // clear the mute list in DirtySDK since the list is about to be replaced
        VoipBlockListClear(VoipGetRef(), getUserIndex());


        Pyro::UiNodeWindow &window = getWindow("CheckMuteStatus");
        window.setDockingState(Pyro::DockingState::DOCK_TOP);

        Pyro::UiNodeTable &table = window.getTable("Check Mute Status Results");

        table.getColumn("displayName").setText("Display Name");
        table.getColumn("timestamp").setText("Timestamp");
        table.getColumn("userId").setText("User Id");
        table.getColumn("source").setText("Source");
        table.getColumn("personaId").setText("Persona Id");
        table.getColumn("friendType").setText("Friend Type");
        table.getColumn("favorite").setText("Favorite");
        table.getColumn("dateTime").setText("DateTime");

        table.getRows().clear();
        
        for (auto const& entry : response->getEntries()) {

            eastl::string userId = eastl::to_string(entry->getUserId());
            Pyro::UiNodeTableRowFieldContainer &fields = table.getRow(userId.c_str()).getFields();
            
            fields["displayName"] = entry->getDisplayName();
            fields["timestamp"] = entry->getTimestamp();
            fields["userId"] = entry->getUserId();
            fields["source"] = entry->getSource();
            fields["personaId"] = entry->getPersonaId();
            fields["friendType"] = entry->getFriendType();
            fields["favorite"] = entry->getFavorite();
            fields["dateTime"] = entry->getDateTime();


            //CV to add DS Api Calls
            

            // tell DirtySDK about the muted status of this user
            VoipBlockListAdd(VoipGetRef(), getUserIndex(), entry->getUserId());

        }
    }
#endif

    REPORT_BLAZE_ERROR(error);
}

void Friends::initActionMuteUser(Pyro::UiNodeAction &action)
{
    action.setText("Mute User");
    action.setDescription("Mute the target user and un-friend them (if friends), preventing communication to or from them");
    action.getParameters().addInt64("friendId", 0, "Friend ID", "nucleus user ID of the target user.");
}

void Friends::actionMuteUser(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if (localUser == nullptr)
    {
        if (getUiDriver() != nullptr)
            getUiDriver()->showMessage_va(Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr");
        return;
    }

    int64_t friendId = parameters["friendId"];
    Blaze::Social::Friends::MuteUserRequest request;
    request.setNucleusId(localUser->getNucleusAccountId());
    request.setFriendId(friendId);
    request.getAuthCredentials().setApiVersion(DEFAULT_FRIENDS_API_VERSION);
    request.getAuthCredentials().setApplicationKey(DEFAULT_FRIENDS_APPLICATION_KEY);
    request.getAuthCredentials().setAuthorization(getAccessToken().c_str());
    
    Blaze::Social::Friends::FriendsComponent * component = gBlazeHub->getComponentManager()->getFriendsComponent();
    component->muteUser(request, Blaze::MakeFunctor(this, &Friends::MuteUserActionCb), friendId);

#if !defined(EA_PLATFORM_NX)
    // mute the targeted user regardless if the call to social is successful
    VoipBlockListAdd(VoipGetRef(), getUserIndex(), friendId);
#endif
}

void Friends::MuteUserActionCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, int64_t friendId)
{
    if (blazeError == Blaze::ERR_OK)
    {
        //CV friendId user muted
        NetPrintf(("Friends: user index %d, added user %x to mute list.\n", getUserIndex(), friendId));
    }
    else
    {
        NetPrintf(("Friends: user index %d, FAILED to add user %x to mute list.\n", getUserIndex(), friendId));
        REPORT_BLAZE_ERROR(blazeError);
    }
    REPORT_BLAZE_ERROR(blazeError);    
}

void Friends::initActionUnmuteUser(Pyro::UiNodeAction &action)
{
    action.setText("Un-mute User");
    action.setDescription("Un-mute the target user, restoring the ability to communicate with them.");
    action.getParameters().addInt64("friendId", 0, "Friend ID", "nucleus user ID of the target user.");
}

void Friends::actionUnmuteUser(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if (localUser == nullptr)
    {
        if (getUiDriver() != nullptr)
            getUiDriver()->showMessage_va(Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr");
        return;
    }

    int64_t friendId = parameters["friendId"];
    Blaze::Social::Friends::MuteUserRequest request;
    request.setNucleusId(localUser->getNucleusAccountId());
    request.setFriendId(friendId);
    request.getAuthCredentials().setApiVersion(DEFAULT_FRIENDS_API_VERSION);
    request.getAuthCredentials().setApplicationKey(DEFAULT_FRIENDS_APPLICATION_KEY);
    request.getAuthCredentials().setAuthorization(getAccessToken().c_str());
    
    Blaze::Social::Friends::FriendsComponent * component = gBlazeHub->getComponentManager()->getFriendsComponent();
    component->unmuteUser(request, Blaze::MakeFunctor(this, &Friends::UnMuteUserActionCb), friendId);
 
#if !defined(EA_PLATFORM_NX)
    // un-mute the targeted user regardless if the call to social is successful
    VoipBlockListRemove(VoipGetRef(), getUserIndex(), friendId);
#endif
}

void Friends::UnMuteUserActionCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, int64_t friendId)
{
    if (blazeError == Blaze::ERR_OK)
    {
        //CV friendId user unmuted
        NetPrintf(("Friends: user index %d, removed user %x from mute list.\n", getUserIndex(), friendId));
    }
    else
    {
        NetPrintf(("Friends: user index %d, FAILED to remove user %x from mute list.\n", getUserIndex(), friendId));
        REPORT_BLAZE_ERROR(blazeError);
    }
    
    REPORT_BLAZE_ERROR(blazeError);
}

}
