
#include "Ignition/AssociationLists.h"
#include "DirtySDK/dirtysock/dirtyuser.h"
#include "Ignition/GameManagement.h"

namespace Ignition
{

AssociationLists::AssociationLists(uint32_t userIndex)
    : LocalUserUiBuilder("Association Lists", userIndex),
      mGetAssociationListParams(nullptr)
{
    getActions().add(&getActionGetAssociationList());
    getActions().add(&getActionCreateLocalAssociationList());
    getActions().add(&getActionGetAssociationListForUser());
}

AssociationLists::~AssociationLists()
{
}


void AssociationLists::onAuthenticated()
{
    setIsVisible(true);
}

void AssociationLists::onDeAuthenticated()
{
    setIsVisible(false);

    getWindows().clear();
}


AssociationListWindow &AssociationLists::getAssociationListWindow(Blaze::Association::AssociationList *associationList)
{
    AssociationListWindow *window = static_cast<AssociationListWindow *>(getWindows().findById_va(associationList->getListName()));
    if (window == nullptr)
    {
        window = new AssociationListWindow(getUserIndex(), associationList);
        window->UserClosing += Pyro::UiNodeWindow::UserClosingEventHandler(this, &AssociationLists::AssociationListWindowClosing);

        getWindows().add(window);
    }

    return *window;
}

void AssociationLists::AssociationListWindowClosing(Pyro::UiNodeWindow *sender)
{
    AssociationListWindow *associationListWindow = (AssociationListWindow *)sender;
    getWindows().remove(associationListWindow);
}

void AssociationLists::initActionGetAssociationListForUser(Pyro::UiNodeAction &action)
{
    action.setText("Get List For User");
    action.setDescription("Gets another users association list");

    action.getParameters().addInt64("blazeId", 0, "Blaze ID", "", "The BlazeId of the user who owns the list you would like to get");
    action.getParameters().addString("listName", "friendList", "List Name", "friendList,recentPlayerList,persistentMuteList,communicationBlockList", "");
}

void AssociationLists::actionGetAssociationListForUser(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Association::ListIdentification listIdent;
    listIdent.setListName(parameters["listName"]);

    vLOG(gBlazeHub->getAssociationListAPI(gBlazeHub->getPrimaryLocalUserIndex())->getListForUser(parameters["blazeId"], listIdent, Blaze::Association::AssociationListsComponent::GetListForUserCb(this, &AssociationLists::GetListForUserCb)));
}

void AssociationLists::GetListForUserCb(const Blaze::Association::ListMembers *listMembers, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    //Blaze::Messaging::SendMessageParameters msg;

    //Blaze::Messaging::ClientMessage clientMessage;
    //clientMessage.setTargetType(Blaze::ENTITY_TYPE_USER);
    //clientMessage.getTargetIds().push_back(listMembers->getInfo().getBlazeObjId().id);

    //gBlazeHub->getComponentManager(0)->getMessagingComponent()->sendMessage(clientMessage,
    //    Blaze::Messaging::MessagingComponent::SendMessageCb(this, &AssociationLists::sendMessageCb));

    REPORT_FUNCTOR_CALLBACK("Callback from Get Lists For User");
}

//void AssociationLists::sendMessageCb(const Blaze::Messaging::SendMessageResponse *response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
//{
//}

void AssociationLists::initActionGetAssociationList(Pyro::UiNodeAction &action)
{
    action.setText("Get Association List");
    action.setDescription("Requests information about an association list");

    action.getParameters().addString("listName", "friendList", "List Name", "friendList,recentPlayerList,persistentMuteList,communicationBlockList", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addBool("subscribeToList", false, "Subscribe to List", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addUInt32("maxResultCount", UINT_MAX, "Max Result Count", "0, 100, 10000", "How many users to get back", Pyro::ItemVisibility::GAME);
    action.getParameters().addUInt32("offset", 0, "Result Offset", "0, 10, 100", "What offset to use for the users you get back", Pyro::ItemVisibility::GAME);
}

void AssociationLists::actionGetAssociationList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Association::ListInfoVector listInfoVector;
    Blaze::Association::ListInfo *listInfo;

    mGetAssociationListParams = parameters;

    listInfo = listInfoVector.pull_back();
    listInfo->getId().setListName(parameters["listName"]);
    listInfo->getStatusFlags().setBits(0);
    if (parameters["subscribeToList"].getAsBool())
    {
        listInfo->getStatusFlags().setSubscribed();
    }

    vLOG(gBlazeHub->getAssociationListAPI(getUserIndex())->getLists(listInfoVector, Blaze::Association::AssociationListAPI::GetListsCb(this, &AssociationLists::GetListsCb), 
                                          parameters["maxResultCount"].getAsUInt32(), parameters["offset"].getAsUInt32()));
}

void AssociationLists::GetListsCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("Get Lists Callback");
    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Association::AssociationListAPI::AssociationListList::const_iterator it = gBlazeHub->getAssociationListAPI(getUserIndex())->getAssociationLists().begin();
        Blaze::Association::AssociationListAPI::AssociationListList::const_iterator end = gBlazeHub->getAssociationListAPI(getUserIndex())->getAssociationLists().end();
        for ( ; it != end; ++it)
        {
            if ((mGetAssociationListParams == nullptr) ||
                (mGetAssociationListParams["listName"].getAsString()[0] == '\0') ||
                (strcmp(mGetAssociationListParams["listName"], (*it)->getListName()) == 0))
            {
                getAssociationListWindow(*it).setIsVisible(true);
            }
        }
    }

    REPORT_BLAZE_ERROR(blazeError);
}


void AssociationLists::initActionCreateLocalAssociationList(Pyro::UiNodeAction &action)
{
    action.setText("Create Local Association List");
    action.setDescription("Creates a local association list for list manipulation");

    action.getParameters().addString("listName", "friendList", "List Name", "friendList,recentPlayerList,persistentMuteList,communicationBlockList", "", Pyro::ItemVisibility::GAME);
}

void AssociationLists::actionCreateLocalAssociationList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Association::ListIdentification id;

    id.setListName(parameters["listName"]);

    vLOG(Blaze::Association::AssociationList* list = gBlazeHub->getAssociationListAPI(getUserIndex())->createLocalList(id));
    getAssociationListWindow(list).setIsVisible(true);
}

AssociationListWindow::AssociationListWindow(uint32_t userIndex, Blaze::Association::AssociationList *associationList)
    : Pyro::UiNodeWindow(associationList->getListName()),
      mUserIndex(userIndex),
      mAssociationList(associationList)
{
    mFriendsOffset = 0;
    mTotalFriendsRequested = 0;

    getActionGroup().getActions().add(&getActionFetchMembers());    
    getActionGroup().getActions().add(&getActionAddMembersToList());
    getActionGroup().getActions().add(&getActionAddExternalMembersToList());
#if !defined(EA_PLATFORM_XBOXONE)
    getActionGroup().getActions().add(&getActionFavoriteMembersInList());
#endif
    getActionGroup().getActions().add(&getActionSetMembersToList());
    getActionGroup().getActions().add(&getActionRefresh());
    getActionGroup().getActions().add(&getActionSubscribe());
    getActionGroup().getActions().add(&getActionUnsubscribe());
    getActionGroup().getActions().add(&getActionClear());

    refreshList();
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4)
    mUserListApiRef = UserListApiCreate(1000);
#endif

    gBlazeHub->getAssociationListAPI(mUserIndex)->addListener(this);

    if (mAssociationList->getListType() == Blaze::Association::LIST_TYPE_FRIEND)
    {
        getActionGroup().getActions().add(&getActionJoinGameWithReservedExternalPlayers());

        gBlazeHub->addIdler(this, "AssociationLists");
    }
}

AssociationListWindow::~AssociationListWindow()
{
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4)
    // cancels all pending transactions
    UserListApiCancelAll(mUserListApiRef);
    UserListApiDestroy(mUserListApiRef);
#endif

    if (mAssociationList->getListType() == Blaze::Association::LIST_TYPE_FRIEND)
    {
        gBlazeHub->removeIdler(this);
    }

    gBlazeHub->getAssociationListAPI(mUserIndex)->removeListener(this);
}

void AssociationListWindow::refreshList()
{
    Pyro::UiNodeTable &tableNode = getTable("Association List");

    tableNode.getColumn("blazeId").setText("Blaze Id");
    tableNode.getColumn("name").setText("Name");
    tableNode.getColumn("status").setText("Status");
    tableNode.getColumn("externalId").setText("External Id");
    tableNode.getColumn("externalSystemId").setText("External System Id");
    tableNode.getColumn("favorite").setText("Favorite");

    tableNode.getActions().insert(&getActionRemoveUserFromList());

    tableNode.getRows().clear();

    Blaze::Association::AssociationList::MemberVector::const_iterator it = mAssociationList->getMemberVector().begin();
    Blaze::Association::AssociationList::MemberVector::const_iterator end = mAssociationList->getMemberVector().end();
    for ( ; it != end; ++it)
    {
        const Blaze::Association::AssociationListMember *member = *it;

        Pyro::UiNodeTableRow &row = tableNode.getRow(member->getPersonaName());
        row.setIconImage(Pyro::IconImage::USER);

        Pyro::UiNodeTableRowFieldContainer &fields = row.getFields();

        const Blaze::UserManager::User *user = member->getUser();
        if (user != nullptr)
        {
            fields["blazeId"] = user->getId();
            fields["name"] = user->getName();
            fields["status"] = user->getOnlineStatus();
        }
        else
        {
            fields["blazeId"] = "n/a";
            fields["name"] = member->getPersonaName();
            fields["status"] = "n/a";
        }

        fields["externalId"] = member->getExternalId();
        fields["externalSystemId"] = member->getExternalSystemId();
        fields["favorite"] = (bool)member->getAttributes().getFavorite();
    }
}

void AssociationListWindow::onMembersRemoved(Blaze::Association::AssociationList *aList)
{
    if (mAssociationList != aList)
        return;

    refreshList();
}

void AssociationListWindow::onMemberUpdated(Blaze::Association::AssociationList* aList,
    const Blaze::Association::AssociationListMember *member)
{
    if (mAssociationList != aList)
        return;

    refreshList();
}

void AssociationListWindow::onListRemoved(Blaze::Association::AssociationList *aList)
{
    if (mAssociationList != aList)
        return;
}

void AssociationListWindow::onMemberAdded(Blaze::Association::AssociationListMember& member, Blaze::Association::AssociationList& list)
{
    if (mAssociationList != &list)
        return;

   refreshList();
}


void AssociationListWindow::onMemberRemoved(Blaze::Association::AssociationListMember& member, Blaze::Association::AssociationList& list)
{
    if (mAssociationList != &list)
        return;

    refreshList();
}


void AssociationListWindow::initActionFetchMembers(Pyro::UiNodeAction &action)
{
    action.setText("Fetch Members");
}

void AssociationListWindow::actionFetchMembers(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(mAssociationList->fetchListMembers(Blaze::MakeFunctor(this, &AssociationListWindow::fetchMembersCb)));
}

void AssociationListWindow::fetchMembersCb(const Blaze::Association::AssociationList*, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("Fetch Members Cb");
    refreshList();
}

void AssociationListWindow::initActionAddMembersToList(Pyro::UiNodeAction &action)
{
    action.setText("Add Members");
    action.setDescription("");

    action.getParameters().addString("memberName1", "", "Member Name 1", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId1", Blaze::INVALID_BLAZE_ID, "Blaze Id 1", "", "", Pyro::ItemVisibility::GAME);
#if !defined(EA_PLATFORM_XBOXONE)
    action.getParameters().addBool("memberFavorite1", false, "Favorite", "", Pyro::ItemVisibility::GAME);
#endif
    action.getParameters().addString("memberName2", "", "Member Name 2", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId2", Blaze::INVALID_BLAZE_ID, "Blaze Id 2", "", "", Pyro::ItemVisibility::GAME);
#if !defined(EA_PLATFORM_XBOXONE)
    action.getParameters().addBool("memberFavorite2", false, "Favorite", "", Pyro::ItemVisibility::GAME);
#endif
    action.getParameters().addString("memberName3", "", "Member Name 3", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId3", Blaze::INVALID_BLAZE_ID, "Blaze Id 3", "", "", Pyro::ItemVisibility::GAME);
#if !defined(EA_PLATFORM_XBOXONE)
    action.getParameters().addBool("memberFavorite3", false, "Favorite", "", Pyro::ItemVisibility::GAME);
#endif
}

void AssociationListWindow::actionAddMembersToList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Association::ListMemberIdVector listMemberIdVector;
    Blaze::Association::ListMemberId *listMemberId;

    const char8_t *memberName = parameters["memberName1"];
    Blaze::BlazeId memberBlazeId = parameters["memberBlazeId1"];
#if !defined(EA_PLATFORM_XBOXONE)
    bool isFavorite = parameters["memberFavorite1"];
#endif
    if ((memberName[0] != '\0') || (memberBlazeId != Blaze::INVALID_BLAZE_ID))
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberName[0] != '\0')
        {
            listMemberId->getUser().setName(memberName);
        }
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
#if !defined(EA_PLATFORM_XBOXONE)
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
#endif
    }

    memberName = parameters["memberName2"];
    memberBlazeId = parameters["memberBlazeId2"];
#if !defined(EA_PLATFORM_XBOXONE)
    isFavorite = parameters["memberFavorite2"];
#endif
    if ((memberName[0] != '\0') || (memberBlazeId != Blaze::INVALID_BLAZE_ID))
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberName[0] != '\0')
        {
            listMemberId->getUser().setName(memberName);
        }
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
#if !defined(EA_PLATFORM_XBOXONE)
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
#endif
    }

    memberName = parameters["memberName3"];
    memberBlazeId = parameters["memberBlazeId3"];
#if !defined(EA_PLATFORM_XBOXONE)
    isFavorite = parameters["memberFavorite3"];
#endif
    if ((memberName[0] != '\0') || (memberBlazeId != Blaze::INVALID_BLAZE_ID))
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberName[0] != '\0')
        {
            listMemberId->getUser().setName(memberName);
        }
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
#if !defined(EA_PLATFORM_XBOXONE)
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
#endif
    }

    Blaze::Association::ListMemberAttributes mask;
#if !defined(EA_PLATFORM_XBOXONE)
    mask.setFavorite();
#endif
    
    vLOG( mAssociationList->addUsersToList(listMemberIdVector, Blaze::Association::AssociationList::AddUsersToListCb(this, &AssociationListWindow::AddUsersToListCb), mask));

}
#if !defined(EA_PLATFORM_XBOXONE)
void AssociationListWindow::initActionFavoriteMembersInList(Pyro::UiNodeAction &action)
{
    action.setText("Favorite Members");
    action.setDescription("");

    action.getParameters().addInt64("memberBlazeId1", Blaze::INVALID_BLAZE_ID, "Blaze Id 1", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addBool("memberFavorite1", false, "Favorite Mamber 1", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId2", Blaze::INVALID_BLAZE_ID, "Blaze Id 2", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addBool("memberFavorite2", false, "Favorite Mamber 2", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId3", Blaze::INVALID_BLAZE_ID, "Blaze Id 3", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addBool("memberFavorite3", false, "Favorite Mamber 3", "", Pyro::ItemVisibility::GAME);
}

void AssociationListWindow::actionFavoriteMembersInList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Association::ListMemberIdVector listMemberIdVector;
    Blaze::Association::ListMemberId *listMemberId;

    Blaze::BlazeId memberBlazeId = parameters["memberBlazeId1"];
    bool isFavorite = parameters["memberFavorite1"];
    if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
    }

    memberBlazeId = parameters["memberBlazeId2"];
    isFavorite = parameters["memberFavorite2"];
    if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
    }

    memberBlazeId = parameters["memberBlazeId3"];
    isFavorite = parameters["memberFavorite3"];
    if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
    }

    Blaze::Association::ListMemberAttributes mask;
    mask.setFavorite();

    vLOG(mAssociationList->setUsersAttributesInList(listMemberIdVector, Blaze::Association::AssociationList::SetUsersAttributesInListCb(this, &AssociationListWindow::FavoriteUsersInListCb), mask));

}

void AssociationListWindow::FavoriteUsersInListCb(const Blaze::Association::AssociationList *associationList,
    const Blaze::Association::ListMemberInfoVector *listMemberInfoVector, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("Favorite Users Callback");

    if (blazeError == Blaze::ERR_OK)
    {
        refreshList();
    }

    REPORT_BLAZE_ERROR(blazeError);
}
#endif

void AssociationListWindow::initActionAddExternalMembersToList(Pyro::UiNodeAction &action)
{
    action.setText("Add External Members");

    //newAction->getParameters().addEnum("memberName1", Blaze::, "Member Name 1", Blaze::, "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberExternalSystemId1", Blaze::NATIVE, "Member External System Id 1", "NONE,MOBILE,LEGACYPROFILEID,VERIZON,FACEBOOK,FACEBOOK_EACOM,BEBO,FRIENDSTER,NATIVE", "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberExternalId1", Blaze::, "Member External Id 1", Blaze::, "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberName2", Blaze::, "Member Name 2", Blaze::, "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberExternalSystemId2", Blaze::NATIVE, "Member External System Id 2", "NONE,MOBILE,LEGACYPROFILEID,VERIZON,FACEBOOK,FACEBOOK_EACOM,BEBO,FRIENDSTER,NATIVE", "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberExternalId2", Blaze::, "Member External Id 2", Blaze::, "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberName3", Blaze::, "Member Name 3", Blaze::, "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberExternalSystemId3", Blaze::NATIVE, "Member External System Id 3", "NONE,MOBILE,LEGACYPROFILEID,VERIZON,FACEBOOK,FACEBOOK_EACOM,BEBO,FRIENDSTER,NATIVE", "", Pyro::ItemVisibility::GAME);
    //newAction->getParameters().addEnum("memberExternalId3", Blaze::, "Member External Id 3", Blaze::, "", Pyro::ItemVisibility::GAME);
}

void AssociationListWindow::actionAddExternalMembersToList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //nexus001 387921949
    //nexus002 384248447
    //nexus003 386488463
    //nexus004 372309152

    Blaze::Association::ListMemberIdVector listMemberIdVector;
    Blaze::Association::ListMemberId *listMemberId;

    const char8_t *memberName = parameters["memberName1"];
    if (memberName[0] != '\0')
    {
        listMemberId = listMemberIdVector.pull_back();
        listMemberId->getUser().setName(memberName);

        Blaze::ClientPlatformType externalSystemId = Blaze::INVALID;
        Blaze::ParseClientPlatformType(parameters["memberExternalSystemId1"], externalSystemId);
        listMemberId->setExternalSystemId(externalSystemId);

        gBlazeHub->setExternalStringOrIdIntoPlatformInfo(listMemberId->getUser().getPlatformInfo(), parameters["memberExternalId1"], parameters["memberExternalString1"]);
    }

    memberName = parameters["memberName2"];
    if (memberName[0] != '\0')
    {
        listMemberId = listMemberIdVector.pull_back();
        listMemberId->getUser().setName(memberName);

        Blaze::ClientPlatformType externalSystemId = Blaze::INVALID;
        Blaze::ParseClientPlatformType(parameters["memberExternalSystemId2"], externalSystemId);
        listMemberId->setExternalSystemId(externalSystemId);

        gBlazeHub->setExternalStringOrIdIntoPlatformInfo(listMemberId->getUser().getPlatformInfo(), parameters["memberExternalId2"], parameters["memberExternalString2"]);
    }

    memberName = parameters["memberName3"];
    if (memberName[0] != '\0')
    {
        listMemberId = listMemberIdVector.pull_back();
        listMemberId->getUser().setName(memberName);

        Blaze::ClientPlatformType externalSystemId = Blaze::INVALID;
        Blaze::ParseClientPlatformType(parameters["memberExternalSystemId3"], externalSystemId);
        listMemberId->setExternalSystemId(externalSystemId);

        gBlazeHub->setExternalStringOrIdIntoPlatformInfo(listMemberId->getUser().getPlatformInfo(), parameters["memberExternalId3"], parameters["memberExternalString3"]);
    }

    vLOG(mAssociationList->addUsersToList(listMemberIdVector, Blaze::Association::AssociationList::AddUsersToListCb(this, &AssociationListWindow::AddUsersToListCb)));
}

void AssociationListWindow::AddUsersToListCb(const Blaze::Association::AssociationList *associationList,
    const Blaze::Association::ListMemberInfoVector *listMemberInfoVector, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("Add Users Callback");

    if (blazeError == Blaze::ERR_OK)
    {
        refreshList();
    }

    REPORT_BLAZE_ERROR(blazeError);
}


void AssociationListWindow::initActionSetMembersToList(Pyro::UiNodeAction &action)
{
    action.setText("Set Members");
    action.setDescription("");

    action.getParameters().addBool("ignoreHash", false, "Ignore Hash", "Forces submit of the send regardless of hash.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("memberName1", "", "Member Name 1", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId1", Blaze::INVALID_BLAZE_ID, "Blaze Id 1", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addUInt64("memberExternalId1", 0, "External Id 1", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addString("memberExternalString1", 0, "External String 1", "", "", Pyro::ItemVisibility::GAME);
#if !defined(EA_PLATFORM_XBOXONE)
    action.getParameters().addBool("memberFavorite1", false, "Favorite", "", Pyro::ItemVisibility::GAME);
#endif
    action.getParameters().addString("memberName2", "", "Member Name 2", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId2", Blaze::INVALID_BLAZE_ID, "Blaze Id 2", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addUInt64("memberExternalId2", 0, "External Id 2", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addString("memberExternalString2", 0, "External String 2", "", "", Pyro::ItemVisibility::GAME);
#if !defined(EA_PLATFORM_XBOXONE)
    action.getParameters().addBool("memberFavorite2", false, "Favorite", "", Pyro::ItemVisibility::GAME);
#endif
    action.getParameters().addString("memberName3", "", "Member Name 3", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addInt64("memberBlazeId3", Blaze::INVALID_BLAZE_ID, "Blaze Id 3", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addUInt64("memberExternalId3", 0, "External Id 3", "", "", Pyro::ItemVisibility::GAME);
    action.getParameters().addString("memberExternalString3", 0, "External String 3", "", "", Pyro::ItemVisibility::GAME);
#if !defined(EA_PLATFORM_XBOXONE)
    action.getParameters().addBool("memberFavorite3", false, "Favorite", "", Pyro::ItemVisibility::GAME);
#endif
}

void AssociationListWindow::actionSetMembersToList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    bool ignoreHash = parameters["ignoreHash"].getAsBool();

    Blaze::Association::ListMemberIdVector listMemberIdVector;
    Blaze::Association::ListMemberId *listMemberId;

    const char8_t *memberName = parameters["memberName1"];
    Blaze::BlazeId memberBlazeId = parameters["memberBlazeId1"];
    uint64_t extId = parameters["memberExternalId1"];
    const char8_t* extString = parameters["memberExternalString1"];
#if !defined(EA_PLATFORM_XBOXONE)
    bool isFavorite = parameters["memberFavorite1"];
#endif
    if ((memberName[0] != '\0') || (memberBlazeId != Blaze::INVALID_BLAZE_ID) || (extId != 0))
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberName[0] != '\0')
        {
            listMemberId->getUser().setName(memberName);
        }
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
        if (extId != 0 || (extString != nullptr && extString[0] != '\0'))
        {
            gBlazeHub->setExternalStringOrIdIntoPlatformInfo(listMemberId->getUser().getPlatformInfo(), extId, extString);
        }
#if !defined(EA_PLATFORM_XBOXONE)
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
#endif
    }

    memberName = parameters["memberName2"];
    memberBlazeId = parameters["memberBlazeId2"];
    extId = parameters["memberExternalId2"];
    extString = parameters["memberExternalString2"];
#if !defined(EA_PLATFORM_XBOXONE)
    isFavorite = parameters["memberFavorite2"];
#endif
    if ((memberName[0] != '\0') || (memberBlazeId != Blaze::INVALID_BLAZE_ID) || (extId != 0))
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberName[0] != '\0')
        {
            listMemberId->getUser().setName(memberName);
        }
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
        if (extId != 0 || (extString != nullptr && extString[0] != '\0'))
        {
            gBlazeHub->setExternalStringOrIdIntoPlatformInfo(listMemberId->getUser().getPlatformInfo(), extId, extString);
        }
#if !defined(EA_PLATFORM_XBOXONE)
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
#endif
    }

    memberName = parameters["memberName3"];
    memberBlazeId = parameters["memberBlazeId3"];
    extId = parameters["memberExternalId3"];
    extString = parameters["memberExternalString3"];
#if !defined(EA_PLATFORM_XBOXONE)
    isFavorite = parameters["memberFavorite3"];
#endif
    if ((memberName[0] != '\0') || (memberBlazeId != Blaze::INVALID_BLAZE_ID) || (extId != 0))
    {
        listMemberId = listMemberIdVector.pull_back();
        if (memberName[0] != '\0')
        {
            listMemberId->getUser().setName(memberName);
        }
        if (memberBlazeId != Blaze::INVALID_BLAZE_ID)
        {
            listMemberId->getUser().setBlazeId(memberBlazeId);
        }
        if (extId != 0 || (extString != nullptr && extString[0] != '\0'))
        {
            gBlazeHub->setExternalStringOrIdIntoPlatformInfo(listMemberId->getUser().getPlatformInfo(), extId, extString);
        }
#if !defined(EA_PLATFORM_XBOXONE)
        if (isFavorite)
            listMemberId->getAttributes().setFavorite();
#endif
    }

    Blaze::Association::ListMemberAttributes mask;
#if !defined(EA_PLATFORM_XBOXONE)
    mask.setFavorite();
#endif


    vLOG( mAssociationList->setUsersToList(listMemberIdVector, Blaze::Association::AssociationList::AddUsersToListCb(this, &AssociationListWindow::SetUsersToListCb), true, ignoreHash, mask));

}


void AssociationListWindow::SetUsersToListCb(const Blaze::Association::AssociationList *associationList,
    const Blaze::Association::ListMemberInfoVector *listMemberInfoVector, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("Set Users Callback");

    if (blazeError == Blaze::ERR_OK)
    {
        refreshList();
    }

    REPORT_BLAZE_ERROR(blazeError);
}



void AssociationListWindow::initActionSubscribe(Pyro::UiNodeAction &action)
{
    action.setText("Subscribe");
    action.setDescription("Subscribe for updates to members in this association list");
}

void AssociationListWindow::actionSubscribe(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(mAssociationList->subscribeToList(Blaze::Association::AssociationList::SubscribeToListCb(this, &AssociationListWindow::SubscribeToListCb)));
}

void AssociationListWindow::SubscribeToListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("List Subscribe Callback");
}

void AssociationListWindow::initActionUnsubscribe(Pyro::UiNodeAction &action)
{
    action.setText("Unsubscribe");
    action.setDescription("Unsubscribe from updates to members in this association list");
}

void AssociationListWindow::actionUnsubscribe(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(mAssociationList->unsubscribeFromList(Blaze::Association::AssociationList::UnsubscribeFromListCb(this, &AssociationListWindow::UnsubscribeFromListCb)));
}

void AssociationListWindow::UnsubscribeFromListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("List Unsubscribe Callback");
}

void AssociationListWindow::initActionClear(Pyro::UiNodeAction &action)
{
    action.setText("Clear");
    action.setDescription("Remove all members from this association list");
}

void AssociationListWindow::actionClear(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(mAssociationList->clearList(Blaze::Association::AssociationList::ClearListCb(this, &AssociationListWindow::ClearListCb)));
}

void AssociationListWindow::ClearListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("Clear List Callback");
    refreshList();
    
}

void AssociationListWindow::initActionRefresh(Pyro::UiNodeAction &action)
{
    action.setText("Refresh");
    action.setDescription("Refresh the member list");
}

void AssociationListWindow::actionRefresh(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    refreshList();
}

void AssociationListWindow::initActionRemoveUserFromList(Pyro::UiNodeAction &action)
{
    action.setText("Remove User");
}

void AssociationListWindow::actionRemoveUserFromList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Association::ListMemberIdVector listMemberIdVector;
    Blaze::Association::ListMemberId *listMemberId;

    listMemberId = listMemberIdVector.pull_back();
    listMemberId->getUser().setBlazeId(parameters["blazeId"]);

    vLOG(mAssociationList->removeUsersFromList(listMemberIdVector, Blaze::Association::AssociationList::RemoveUsersFromListCb(this, &AssociationListWindow::RemoveUsersFromListCb)));
}

void AssociationListWindow::RemoveUsersFromListCb(const Blaze::Association::AssociationList *associationList,
    const Blaze::Association::ListMemberIdVector *removedUsers, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    if (blazeError == Blaze::ERR_OK)
    {
        REPORT_FUNCTOR_CALLBACK("Remove Users From List Callback");

        refreshList();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void AssociationListWindow::initActionSyncFirstParty(Pyro::UiNodeAction &action)
{
    action.setText("Synchronize");
    action.setDescription("Synchronizes the Blaze Association List with the first-party friends list");
}

void AssociationListWindow::actionSyncFirstParty(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    synchronizeFirstPartyList();
}

void AssociationListWindow::initActionJoinGameWithReservedExternalPlayers(Pyro::UiNodeAction &action)
{
    action.setText("Join Association List to Game, With Reserved External Players");
    action.setDescription("Bring this association List into a game by id, adding offline members as reserved external players.");

    action.getParameters().addUInt64("gameId", 0, "Game Id", "", "The Id of the game you would like to join.");
    action.getParameters().addEnum("gameEntryType", Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT, "Game Entry", "", Pyro::ItemVisibility::ADVANCED);
}

void AssociationListWindow::actionJoinGameWithReservedExternalPlayers(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserIdentificationList reservedExternalIdentifications;

    // add assoc list members who aren't online, as reserved external players
    Blaze::Association::AssociationList::MemberVector::const_iterator it = mAssociationList->getMemberVector().begin();
    Blaze::Association::AssociationList::MemberVector::const_iterator end = mAssociationList->getMemberVector().end();
    for ( ; it != end; ++it)
    {
        const Blaze::Association::AssociationListMember *member = *it;
        const Blaze::UserManager::User *user = member->getUser();
        if ((user != nullptr) && (user->getOnlineStatus() != Blaze::UserManager::User::ONLINE))
        {
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
            reservedExternalIdentifications.pull_back()->getPlatformInfo().getExternalIds().setPsnAccountId(member->getPsnAccountId());
#elif defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
            reservedExternalIdentifications.pull_back()->getPlatformInfo().getExternalIds().setXblAccountId(member->getXblAccountId());
#elif defined(EA_PLATFORM_NX)
            reservedExternalIdentifications.pull_back()->getPlatformInfo().getExternalIds().setSwitchId(member->getSwitchId());
#elif defined(EA_PLATFORM_WINDOWS)
            if(gBlazeHub->getClientPlatformType() == Blaze::ClientPlatformType::steam) // Needed for steam platform only
                reservedExternalIdentifications.pull_back()->getPlatformInfo().getExternalIds().setSteamAccountId(member->getSteamAccountId());
#elif defined(EA_PLATFORM_STADIA)
            reservedExternalIdentifications.pull_back()->getPlatformInfo().getExternalIds().setStadiaAccountId(member->getStadiaAccountId());
#endif
        }
    }

    //note: for mAssociationList userGroup arg here, blaze will only join as active online list members,
    //while any of those in reservedExternalIdentifications will be reserved
    gBlazeHubUiDriver->getGameManagement().runJoinGameById(
        parameters["gameId"],
        mAssociationList,
        Blaze::GameManager::UNSPECIFIED_TEAM_INDEX,
        parameters["gameEntryType"], nullptr, Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT, &reservedExternalIdentifications);
}

void AssociationListWindow::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4)
    // give module time to perform periodic processing
    UserListApiUpdate(mUserListApiRef);
#endif
}
void AssociationListWindow::synchronizeFirstPartyList()
{
    switch (mAssociationList->getListType())
    {
        case Blaze::Association::LIST_TYPE_FRIEND:
        {
#if defined(EA_PLATFORM_CONSOLE)
            synchronizeFirstPartyFriendsList();
#endif
            break;
        }
        default:
        {
            // do nothing
            break;
        }
    }
}


void UserApiListCallbackGlobal(UserListApiRefT *pRef, UserListApiReturnTypeE eResponseType, UserListApiEventDataT *pUserApiEventData, void *pUserData)
{
    ((AssociationListWindow*)pUserData)->userApiListCallback(eResponseType, pUserApiEventData);
}

void AssociationListWindow::userApiListCallback(UserListApiReturnTypeE eResponseType, UserListApiEventDataT *pUserApiEventData)
{
    switch (eResponseType)
    {
    case TYPE_LIST_END:
        {
            // If we got all the friends we requested, there may still be more out there:
            if (mTotalFriendsRequested == static_cast<int32_t>(mFriendsMemberList.size()))
            {
                requestMoreFirstPartyFriends();
            }
            else
            {
                // Otherwise, Send the message off:
                Blaze::Association::ListMemberAttributes mask;
#if defined(EA_PLATFORM_XBOXONE)
                mask.setFavorite();
#endif
                vLOG(mAssociationList->setUsersToList(mFriendsMemberList, Blaze::Association::AssociationList::AddUsersToListCb(this, &AssociationListWindow::AddUsersToListCb), true, false, mask));
            }
            break;
        }
    case TYPE_USER_DATA:
        {
            Blaze::Association::ListMemberId *memberId = mFriendsMemberList.pull_back();

#if defined(EA_PLATFORM_XBOXONE) 
            Blaze::ExternalXblAccountId xuid;
            DirtyUserToNativeUser(&xuid, sizeof(xuid), &pUserApiEventData->UserData.DirtyUser);
            memberId->getUser().getPlatformInfo().getExternalIds().setXblAccountId(xuid);
            if (pUserApiEventData->UserData.bIsFavorite == 1)
                memberId->getAttributes().setFavorite();
#elif defined(EA_PLATFORM_PS4)
            Blaze::ExternalPsnAccountId psnId;
            DirtyUserToNativeUser(&psnId, sizeof(psnId), &pUserApiEventData->UserData.DirtyUser);
            memberId->getUser().getPlatformInfo().getExternalIds().setPsnAccountId(psnId);
#else
            // No other platforms supported currently:
#endif
            memberId->setExternalSystemId(gBlazeHub->getConnectionManager()->getClientPlatformType());
            break;
        }
    default:
        break;
    }
}

void AssociationListWindow::requestMoreFirstPartyFriends()
{
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4)
    // This value could go upto 500 max (PS4 limit), but we will still have to call this multiple times
    const int32_t cMaxFriendsToRequestAtOnce = 100;

    UserListApiGetListAsync(mUserListApiRef, gBlazeHub->getLoginManager(mUserIndex)->getDirtySockUserIndex(), USERLISTAPI_TYPE_FRIENDS, cMaxFriendsToRequestAtOnce,  mFriendsOffset, nullptr, UserApiListCallbackGlobal, 0, (void*)this);
    mFriendsOffset += cMaxFriendsToRequestAtOnce;
    mTotalFriendsRequested += cMaxFriendsToRequestAtOnce;
#endif
}
void AssociationListWindow::synchronizeFirstPartyFriendsList()
{
    mFriendsMemberList.clear();

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS4)
    // Clear & Reset the UserListApi requests:
    mFriendsOffset = 0;
    mTotalFriendsRequested = 0;
    UserListApiCancelAll(mUserListApiRef);

    requestMoreFirstPartyFriends();
    vLOG(mAssociationList->setUsersToList(mFriendsMemberList, Blaze::Association::AssociationList::AddUsersToListCb(this, &AssociationListWindow::AddUsersToListCb)));
#endif
}
}
