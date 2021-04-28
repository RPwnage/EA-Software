/*************************************************************************************************/
/*!
\file associationlistapi.h


\attention
(c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_API_H
#define BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_API_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/memorypool.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/dispatcher.h"

#include "EASTL/functional.h"
#include "EASTL/hash_set.h"
#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "BlazeSDK/blaze_eastl/list.h"

#include "BlazeSDK/component/associationlists/tdf/associationlists.h"
#include "BlazeSDK/component/associationlistscomponent.h"

#include "associationlistmember.h"
#include "associationlist.h"

namespace Blaze
{
    class BlazeHub;

namespace UserManager
{
    class UserManager;
    class User;
}

namespace Association
{
    class AssociationListAPIListener;
    class AssociationListsComponent;
 
    /*! **************************************************************************************************************/
    /*!
        \class AssociationListAPI
        \ingroup _mod_apis

        \brief
        AssociationListAPI allows creation and browsing of Association.

        \details
        The API provides methods to create, browse and join Association using the Association Blaze component.   From this API
        the local user can also access the Association it's currently in.

    ******************************************************************************************************************/
    class BLAZESDK_API AssociationListAPI
        : public MultiAPI,
          public UserGroupProvider,
          public UserManager::UserEventListener
    {

    public:
        struct AssociationListApiParams
        {
            //! \brief Default struct constructor
            AssociationListApiParams()
                : mMaxLocalAssociationLists(0)
            {}

            AssociationListApiParams(uint32_t maxLocalAssociationLists)
                :mMaxLocalAssociationLists(maxLocalAssociationLists)
            {}

            uint32_t mMaxLocalAssociationLists;   //!< pre-allocation cap for the number of AssociationList the local user has.
        };

        /*! ****************************************************************************/
        /*! \brief Creates the AssociationListAPI.

            \param hub The hub to create the API on.
            \param params The settings to create the API with.
            \param allocator pointer to the optional allocator to be used for the component; will use default if not specified. 
        ********************************************************************************/
        static void createAPI(BlazeHub &hub, const AssociationListApiParams &params, EA::Allocator::ICoreAllocator *allocator = nullptr);
      
        /*! ****************************************************************************/
        /*! \name API methods
        ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Creates a local list for a given list type or name for the owner of the API if it does not already exist.

            This method can be used to access list modification functions without needing to 
            fetch an entire list from the server.  One use case would be to set a large friends
            list on the server using the AssocationList::setUsersToList command.  This can be done
            without ever needing to fetch the entire list from the server and set it to memory. 

            \param[in] listId - The identifying 
            \return The created local list, or an existing list.
        ********************************************************************************/
        AssociationList* createLocalList(const ListIdentification& listId);

        typedef Functor2<BlazeError, JobId> GetListsCb;
        /*! ****************************************************************************/
        /*! \brief Retrieve all(or partial) list information with specified association list(s) for
             the user and subscribe to user extended data for all members in the list if flag
             set to true.

             To retrieve all configured lists (on the server), you may pass an empty ListInfoVector

            Note:
            1) The function call will release all local members in the list for the user and create
               local list and members based on the response of the API call.
            2) If a list in the request does not exist for the user(which means the list is empty 
               for the use), we will not have it in the response.

            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND - if a cache entry for the user is not found on server

            \param[in] listInfoVector - a list of ListInfo objects outlining the desired lists
            \param[in] resultFunctor - the functor dispatched upon success/failure         
            \param[in] maxResultCount - the limit for the returned list members
            \param[in] offset - the start position of the returned list members
            \return JobId of task.
        ********************************************************************************/
        JobId getLists(const ListInfoVector &listInfoVector, const GetListsCb &resultFunctor, const uint32_t maxResultCount = UINT32_MAX, const uint32_t offset = 0);

        typedef AssociationListsComponent::GetListForUserCb GetListForUserCb;
        /*! ****************************************************************************/
        /*! \brief Retrieve an association list info for specified user 

            If the user is not online, we will try to retrieve information from db for the user    
            - This rpc need authority, PERMISSION_GET_USER_ASSOC_LIST permission is required
            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND - if a user with indicated blaze id is not found on server
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file

            \param[in] blazeId  - the user whose association list to be retrieved
            \param[in] listIdentification - the list name and or type of list to be retrieved
            \param[in] resultFunctor - the functor dispatched upon success/failure         
            \param[in] maxResultCount - the limit for the returned list members
            \param[in] offset - the start position of the returned list members
            \return JobId of task.
        ********************************************************************************/
        JobId getListForUser(const BlazeId blazeId, const ListIdentification &listIdentification, const GetListForUserCb &resultFunctor, const uint32_t maxResultCount = UINT32_MAX, const uint32_t offset = 0);
      
        /*! ****************************************************************************/
        /*! \name AssociationListAPIListener registration methods
        ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Adds a listener to receive async notifications from the AssociationListAPI.

            Once an object adds itself as a listener, AssociationListAPI will dispatch
            async events to the object via the methods in AssociationListAPIListener.

            \param listener The listener to add.
        ********************************************************************************/
        void addListener(AssociationListAPIListener *listener);

        /*! ****************************************************************************/
        /*! \brief Removes a listener from the AssociationListAPI, preventing any further async
        notifications from being dispatched to the listener.

        \param listener The listener to remove.
        ********************************************************************************/
        void removeListener(AssociationListAPIListener *listener);
   
        /*! ****************************************************************************/
        /*! \name Misc Helpers
        ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief return the low level AssociationList component associated with this API (not typically needed)

            You don't typically need to call low-level RPCs directly.
            \return the AssociationListsComponent associated with the AssociationListAPI's user
        ***************************************************************************************************/
        AssociationListsComponent &getAssociationListComponent() const { return mComponent; }

        /*! ************************************************************************************************/
        /*! \brief Returns the parameters use to initialize the API.

            \return Reference to the AssociationListAPIParams used during initialization.
        ***************************************************************************************************/
        const AssociationListApiParams &getApiParams() const { return mApiParams; }

        typedef Blaze::list<AssociationList *> AssociationListList;
        /*! ****************************************************************************/
        /*! \brief Return the local list of Associationlist objects.  This can be used to iterate through the available lists

            \return The internal AssociationListList object.
        ********************************************************************************/
        const AssociationListList &getAssociationLists() const { return mAssociationListList; }

        /*! ****************************************************************************/
        /*! \brief Return the local Association list object with the unique blaze object id

            \param  objId - the blaze object id of teh association list to get
            \return The AssociationList, or nullptr if none found.
        ********************************************************************************/
        AssociationList* getListByObjId(const EA::TDF::ObjectId& objId) const;

        /*! ************************************************************************************************/
        /*! \brief retrieve association list via list name

            \return association list with specific name if such list is there for user(list exist and not empty)
             otherwise, nullptr
        ***************************************************************************************************/
        AssociationList* getListByName(const char8_t* listName) const;

        /*! ************************************************************************************************/
        /*! \brief retrieve association list via list type

            \return association list with specific type if such list is there for user(list exists and not empty)
              otherwise, nullptr
        ***************************************************************************************************/
        AssociationList* getListByType(const ListType listType) const;

        /*! ****************************************************************************/
        /*! \brief Override this method in order to enable the other component to map 
            to a corresponding AssociationList object via blaze object id

            \param bobjId that identifies a particular AssociationList.
            \return AssociationList object that corresponds to the bobjId.
        ********************************************************************************/
        virtual AssociationList *getUserGroupById(const EA::TDF::ObjectId &bobjId) const;

        typedef AssociationListsComponent::GetConfigListsInfoCb GetConfigListsInfoCb;
        /*! ****************************************************************************/
        /*! \brief Retrieve all config association lists info 

            \param[in] resultFunctor - the functor dispatched upon success/failure 
            \return JobId of task.
        ********************************************************************************/
        JobId getConfigListsInfo(const GetConfigListsInfoCb &resultFunctor);
      
        /*! ************************************************************************************************/
        /*! \brief Notification that a local user has lost authentication (logged out) of the blaze server
            The provided user index indicates which local user  lost authentication.  If multiple local users
            loose authentication, this will trigger once per user.

            \param[in] userIndex - the user index of the local user that authenticated.
        ***************************************************************************************************/
        virtual void onDeAuthenticated(uint32_t userIndex);

        typedef Dispatcher<AssociationListAPIListener> AssociationListAPIDispatcher;
        AssociationListAPIDispatcher& getDispatcher() {
            return mDispatcher;
        }

        void addListToIndices(AssociationList& list);
        void removeListFromIndices(AssociationList& list);

    protected:
        virtual void onUserUpdated(const UserManager::User& user);

    private:
        NON_COPYABLE(AssociationListAPI);

        /*! ****************************************************************************/
        /*! \brief Constructor.

            \param[in] blazeHub    pointer to Blaze Hub
            \param[in] memGroupId  mem group to be used by this class to allocate memory
        
             Private as all construction should happen via the factory method.
        ********************************************************************************/
        AssociationListAPI(BlazeHub &blazeHub, const AssociationListApiParams& params, uint32_t userIndex, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
        virtual ~AssociationListAPI();

        void setNotificationHandlers();
        void clearNotificationHandlers();

        void releaseAssociationLists();

        void getListsCb(const Lists *response, BlazeError err, JobId jobid, const GetListsCb resultCb);
        void getListForUserCb(const ListMembers *response, BlazeError err, JobId jobid, const GetListForUserCb resultCb);
        void waitForAllUsers(BlazeError err, JobId jobid, const GetListsCb resultCb);
        void onUserUpdated(const UserStatus* userStatus);

        bool validateListIdentification(const ListIdentification &listIdentification) const;

        ListType getListType(const char8_t* listName);

        /*! ****************************************************************************/
        /*! \brief Logs any initialization parameters for this API to the log.
        ********************************************************************************/
        void logStartupParameters() const {};

        //  notification handlers
        void onNotifyUpdateListMembership(const Blaze::Association::UpdateListMembersResponse* notification, uint32_t userIndex);
        void onNotifyMembersRemoved(const UpdateListMembersRequest *listMembers, uint32_t userIndex);

    private:
        AssociationListsComponent&   mComponent;
        AssociationListApiParams     mApiParams;

        AssociationListList mAssociationListList;

        typedef eastl::intrusive_hash_map<EA::TDF::ObjectId, ListByBlazeObjectIdNode, 11> ListByBlazeObjectIdMap;
        ListByBlazeObjectIdMap mBlazeObjectIdMap;

        typedef eastl::intrusive_hash_map<ListType, ListByListTypeNode, 11> ListByListTypeMap;
        ListByListTypeMap mListTypeMap;

        typedef eastl::intrusive_hash_map<const char8_t *, ListByListNameNode, 11,
            eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > ListByListNameMap;
        ListByListNameMap mListNameMap;

        MemPool<AssociationList>  mAListMemPool;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor

        AssociationListAPIDispatcher    mDispatcher;
    };

    /*! ****************************************************************************/
    /*!
        \class AssociationListAPIListener

        \brief classes that wish to receive async notifications about AssociationListAPI
        events implement this interface, then add themselves as listeners.

        Listeners must add themselves to each AssociationListAPI that they wish to get notifications
        about.  See AssociationListAPI::addListener.
    ********************************************************************************/
    class BLAZESDK_API AssociationListAPIListener
    {
        public:
            virtual ~AssociationListAPIListener() {};

            /*! ****************************************************************************/
            /*! \brief Notifies when a user has been added to the local user's association list
                \param member - association list member added
                \param list - the association list 
            ********************************************************************************/
            virtual void onMemberAdded(Association::AssociationListMember& member, Association::AssociationList& list) {}

            /*! ****************************************************************************/
            /*! \brief Notifies you when the status of a list member is updated
                \param list - the association list 
                \param member - The AssociationListMember that has been updated
            ********************************************************************************/
            virtual void onMemberUpdated(Association::AssociationList* list, const Association::AssociationListMember *member) = 0;

            /*! ****************************************************************************/
            /*! \brief Notifies when a user has been removed from the local user's association list.            
                \param member - association list member removed
                \param list - the association list 
            ********************************************************************************/
            virtual void onMemberRemoved(Association::AssociationListMember& member, Association::AssociationList& list) {}

            /*! ****************************************************************************/
            /*! \brief Notifies before a list is freed from the local cache/storage (for example 
                    on disconnection.)
                \param list - the freed association list
            ********************************************************************************/
            virtual void onListRemoved(Association::AssociationList* list) = 0;

            /*! ****************************************************************************/
            /*! \brief DEPRECATED (use onMemberRemoved) Notifies when user(s) has been removed from an association list of current user
                \param list - the association list has members added, will be nullptr if the list
                               is empty
            ********************************************************************************/
            virtual void onMembersRemoved(Association::AssociationList* list) = 0;
    };

    BLAZESDK_API const char8_t *ListTypeToString(ListType listType);
    BLAZESDK_API ListType StringToListType(const char8_t *listType);

} //Association
} // namespace Blaze
#endif // BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_API_H
