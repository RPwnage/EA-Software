
#ifndef ASSOCIATION_LISTS_H
#define ASSOCIATION_LISTS_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/associationlists/associationlistapi.h"
#include "BlazeSDK/messaging/messagingapi.h"
#include "DirtySDK/misc/userlistapi.h"

namespace Ignition
{

class AssociationListWindow;

class AssociationLists :
    public LocalUserUiBuilder
{
    public:
        AssociationLists(uint32_t userIndex);
        virtual ~AssociationLists();

        virtual void onAuthenticated();
        virtual void onDeAuthenticated();

        AssociationListWindow &getAssociationListWindow(Blaze::Association::AssociationList *associationList);

    private:
        Pyro::UiNodeParameterStructPtr mGetAssociationListParams;

        PYRO_ACTION(AssociationLists, GetAssociationList);
        PYRO_ACTION(AssociationLists, CreateLocalAssociationList);
        PYRO_ACTION(AssociationLists, GetAssociationListForUser);

        void GetListsCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void GetListForUserCb(const Blaze::Association::ListMembers *listMembers, Blaze::BlazeError blazeError, Blaze::JobId jobId);

        void AssociationListWindowClosing(Pyro::UiNodeWindow *sender);
};

class AssociationListWindow :
    public Pyro::UiNodeWindow,
    protected Blaze::Association::AssociationListAPIListener,
    protected Blaze::Idler
{
    friend class AssociationLists;

    public:
        AssociationListWindow(uint32_t userIndex, Blaze::Association::AssociationList *associationList);
        virtual ~AssociationListWindow();

        AssociationLists *getParent() { return (AssociationLists *)Pyro::UiNodeWindow::getParent(); }

    protected:        
        virtual void onMembersRemoved(Blaze::Association::AssociationList *aList);
        virtual void onMemberUpdated(Blaze::Association::AssociationList* aList, const Blaze::Association::AssociationListMember *member);
        virtual void onListRemoved(Blaze::Association::AssociationList *aList);

        virtual void onMemberAdded(Blaze::Association::AssociationListMember& member, Blaze::Association::AssociationList& list);
        virtual void onMemberRemoved(Blaze::Association::AssociationListMember& member, Blaze::Association::AssociationList& list);

    private:
        uint32_t mUserIndex;
        Blaze::Association::AssociationList *mAssociationList;

        void refreshList();

        PYRO_ACTION(AssociationListWindow, FetchMembers);
        PYRO_ACTION(AssociationListWindow, AddMembersToList);
#if !defined(EA_PLATFORM_XBOXONE)
        PYRO_ACTION(AssociationListWindow, FavoriteMembersInList)
#endif
        PYRO_ACTION(AssociationListWindow, AddExternalMembersToList);
        PYRO_ACTION(AssociationListWindow, SetMembersToList);
        PYRO_ACTION(AssociationListWindow, Refresh);
        PYRO_ACTION(AssociationListWindow, Subscribe);
        PYRO_ACTION(AssociationListWindow, Unsubscribe);
        PYRO_ACTION(AssociationListWindow, Clear);

        PYRO_ACTION(AssociationListWindow, RemoveUserFromList);

        PYRO_ACTION(AssociationListWindow, SyncFirstParty);
        PYRO_ACTION(AssociationListWindow, JoinGameWithReservedExternalPlayers);

        void fetchMembersCb(const Blaze::Association::AssociationList*, Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void sendMessageCb(const Blaze::Messaging::SendMessageResponse *response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void AddUsersToListCb(const Blaze::Association::AssociationList *associationList,
            const Blaze::Association::ListMemberInfoVector *listMemberInfoVector, Blaze::BlazeError blazeError, Blaze::JobId jobId);
#if !defined(EA_PLATFORM_XBOXONE)
        void FavoriteUsersInListCb(const Blaze::Association::AssociationList *associationList,
            const Blaze::Association::ListMemberInfoVector *listMemberInfoVector, Blaze::BlazeError blazeError, Blaze::JobId jobId);
#endif
        void SetUsersToListCb(const Blaze::Association::AssociationList *associationList,
            const Blaze::Association::ListMemberInfoVector *listMemberInfoVector, Blaze::BlazeError blazeError, Blaze::JobId jobId);
        
        void RemoveUsersFromListCb(const Blaze::Association::AssociationList *associationList,
            const Blaze::Association::ListMemberIdVector *removedUsers, Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void SubscribeToListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void UnsubscribeFromListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void ClearListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);

        virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);

        UserListApiRefT* mUserListApiRef;
        Blaze::Association::ListMemberIdVector mFriendsMemberList;
        int32_t mFriendsOffset;
        int32_t mTotalFriendsRequested;

        void requestMoreFirstPartyFriends();
public:
        void userApiListCallback(UserListApiReturnTypeE eResponseType, UserListApiEventDataT *pUserApiEventData);
private:
        void synchronizeFirstPartyList();
        void synchronizeFirstPartyFriendsList();
};

}

#endif
