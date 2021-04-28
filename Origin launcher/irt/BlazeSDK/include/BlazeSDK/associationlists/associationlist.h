/*************************************************************************************************/
/*!
    \file associationlist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_H
#define BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/memorypool.h"
#include "BlazeSDK/component/associationlists/tdf/associationlists.h"
#include "BlazeSDK/usergroup.h"
#include "associationlistmember.h"

#include "BlazeSDK/blaze_eastl/vector.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"

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
    class AssociationListAPI;
    
    struct BLAZESDK_API ListByBlazeObjectIdNode : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListByListTypeNode      : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListByListNameNode      : public eastl::intrusive_hash_node {};

    /*! **************************************************************************************************************/
    /*! 
        \class AssociationList

        \brief
        A AssociationList object represents the AssociationList the current user belongs to.
    ******************************************************************************************************************/
    class BLAZESDK_API AssociationList :
        public UserGroup,
        protected ListInfo,
        protected ListByBlazeObjectIdNode,
        protected ListByListTypeNode,
        protected ListByListNameNode
    {
    public:
        typedef Blaze::vector<const AssociationListMember *> MemberVector;

        /*! ****************************************************************************/
        /*! \brief Returns the AssociationListAPI object associated with this AssociationList.
        ********************************************************************************/
        AssociationListAPI* getAssociationListAPI() const { 
            return mAPI;  
        }

        /*! ****************************************************************************/
        /*! \brief Returns true if the Association List has not been backed by the server.

             A "local only" list is a list created by AssociationListAPI::createLocalList.
             If "fetchMembers" is called, then the list is no longer considered "local only".
             All modification methods can be called on a local-only list, but results that 
             depend on server information (like members or total size) will not return 
             correct information.
        ********************************************************************************/
        bool isLocalOnlyList() const {
            return mIsLocalOnly;
        }

        /*! ****************************************************************************/
        /*! \brief Returns the type of this list.
        ********************************************************************************/
        ListType getListType() const {
            return ListInfo::getId().getListType();
        }

        /*! ****************************************************************************/
        /*! \brief Returns the name of this list.
        ********************************************************************************/
        const char8_t *getListName() const {
            return ListInfo::getId().getListName();
        }

        /*! ****************************************************************************/
        /*! \brief Returns the EA::TDF::ObjectId of this list.
        ********************************************************************************/
        EA::TDF::ObjectId getBlazeObjectId() const {
            return ListInfo::getBlazeObjId();
        }

        /*! ****************************************************************************/
        /*! \brief Returns the maximum number of member this list can have, as configured on the server, or 0 for a local only list.
        ********************************************************************************/
        uint32_t getMaxSize() const {
            return ListInfo::getMaxSize();
        }

        /*! ****************************************************************************/
        /*! \brief Returns true if this list allows member rollover, as configured on the server, and false for a local only list.
        ********************************************************************************/
        bool isRollover() const {
            return ListInfo::getStatusFlags().getRollover();
        }

        /*! ****************************************************************************/
        /*! \brief Returns true if this list is currently subscribed.
        ********************************************************************************/
        bool isSubscribed() const {
            return ListInfo::getStatusFlags().getSubscribed();
        }

        /*! ****************************************************************************/
        /*! \brief Returns a AssociationListMember given a ListMemberId, by using the using the valid BlazeId, ExternalId, or persona.

            \param memberId The method uses the first valid BlazeId, ExternalId, or persona (in that order) to find the AssociationListMember.
                   For example, if you set a valid BlazeId and a valid ExternalId, then only the BlazeId will be used to find a member. If a valid
                   BlazeId is set, and no match is found, no further attempt is made to find a match even if a valid ExternalId was set.
            \return An AssociationListMember*, or nullptr if no match was made.
        ********************************************************************************/
        AssociationListMember* getMemberByMemberId(const ListMemberId &memberId) const;

        /*! ****************************************************************************/
        /*! \brief Returns a AssociationListMember given a BlazeId.

            \param id The Blaze ID of the user to look up in the member list.
            \return An AssociationListMember*, or nullptr if no match was made.
        ********************************************************************************/
        AssociationListMember *getMemberByBlazeId(BlazeId id) const;

        /*! ****************************************************************************/
        /*! \brief (DEPRECATED) Returns a AssociationListMember given an ExternalId which maps to the NATIVE ExternalSystemId type.

            \param id The native External ID of the user to look up in the member list.
            \return An AssociationListMember*, or nullptr if no match was made.
        ********************************************************************************/
        AssociationListMember *getMemberByNativeExternalId(ExternalId id) const;

        /*! ****************************************************************************/
        /*! \brief (DEPRECATED) Returns a AssociationListMember given an ExternalId.

            \param id The External ID of the user to look up in the member list.
            \return An AssociationListMember*, or nullptr if no match was made.
        ********************************************************************************/
        AssociationListMember *getMemberByExternalId(ExternalId id) const;

        /*! ****************************************************************************/
        /*! \brief  Returns a AssociationListMember given an PlatformInfo.

            \param platformInfo The platformInfo of the user to look up in the member list.
            \return An AssociationListMember*, or nullptr if no match was made.
        ********************************************************************************/
        AssociationListMember *getMemberByPlatformInfo(const PlatformInfo& platformInfo) const;
        AssociationListMember *getMemberByPsnAccountId(ExternalPsnAccountId psnId) const;
        AssociationListMember *getMemberByXblAccountId(ExternalXblAccountId xblId) const;
        AssociationListMember *getMemberBySteamAccountId(ExternalSteamAccountId steamId) const;
        AssociationListMember *getMemberByStadiaAccountId(ExternalStadiaAccountId stadiaId) const;
        AssociationListMember *getMemberBySwitchId(const char8_t* switchId) const;
        AssociationListMember *getMemberByNucleusAccountId(AccountId accountId) const;

        /*! ****************************************************************************/
        /*! \brief Returns a AssociationListMember given a persona name and personaNamespace.

            \param persona The persona name of the user to look up in the member list.
            \return An AssociationListMember*, or nullptr if no match was made.
        ********************************************************************************/
        AssociationListMember *getMemberByPersonaName(const char8_t *personaNamespace, const char8_t *persona) const;

        /*! ****************************************************************************/
        /*! \brief Returns the list of AssociationListMembers contained in this AssociationList.

            \return A list of AssociationListMembers.
        ********************************************************************************/
        const MemberVector &getMemberVector() const;

        /*! ****************************************************************************/
        /*! \brief Returns the member count of the list on server side. 

            \return Member count of the list on server side.
        ********************************************************************************/
        uint32_t getMemberCount() const;

        /*! ****************************************************************************/
        /*! \brief Returns the number of AssociationListMembers contained in this AssociationList.

            \return Count of AssociationListMembers.
        ********************************************************************************/
        uint32_t getLocalMemberCount() const;

        /*******************************************************************************  
            \brief If local list has complete members cached on SDK side, returns true, otherwise
                return false.  

            \return Whether local list has complete members cached on SDK side.  
        ********************************************************************************/ 
        bool hasCompleteCache() const; 

        /*! ****************************************************************************/
        /*! \brief Returns the page start position, works only when the local list on SDK
                   side is a page.

            \return Page start position.
        ********************************************************************************/
        uint32_t getPageOffset() const;
        

        typedef Functor3<const AssociationList *, BlazeError, JobId> FetchMembersCb;
        /*! ****************************************************************************/
        /*! \brief Fetches the list members for an association list.  

            On success the list is no longer considered "local-only" (if it was local only previously), and getMembers will work
            for the requested range of users.

            The callback can potentially return the following errors:
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with the local list's type or name is not valid

            \param[in] resultFunctor the functor dispatched upon success/failure
            \param[in] maxResultCount - the size limit for the returned list members
            \param[in] offset - the start position of the returned list members
            \return JobId of task.
        ********************************************************************************/
        JobId fetchListMembers(const FetchMembersCb &resultFunctor, const uint32_t maxResultCount = UINT32_MAX, const uint32_t offset = 0);


        typedef Functor4<const AssociationList *, const ListMemberInfoVector *, BlazeError, JobId> AddUsersToListCb;
        /*! ****************************************************************************/
        /*! \brief Add user(s) to the association list 

            On success the specified user(s) are added to the association list

            The callback returns association list with the added members, if successful,
            - There is no partial success, if one member to be added by the request fails, the add process is terminated
            - Note that external users that do not correspond to an EA user in the db will be dropped from the add request.
            - ASSOCIATIONLIST_ERR_DUPLICATE_USER_FOUND - duplicate members found in the request
            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND -  if a user with indicated blaze id is not found on server
            - ASSOCIATIONLIST_ERR_USER_CAN_NOT_ADD_SELF - if a user trying to add itself to the list
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file
            - ASSOCIATIONLIST_ERR_LIST_IS_FULL_OR_TOO_MANY_USERS - if the list is already full or will be oversize with members in the list
            - ASSOCIATIONLIST_MEMBER_ALREADY_IN_THE_LIST - member is already in the list
            - ASSOCIATIONLIST_ERR_INVALID_USER - neither blaze id nor external id for user is valid

            \param[in] listMemberIdVector a list of member to be added to the association list
            \param resultFunctor the functor dispatched upon success/failure
            \param[in] mask a bitfield mask utilized to determine which attribute fields are intended to be updated
            \return JobId of task.
        ********************************************************************************/
        JobId addUsersToList(const ListMemberIdVector &listMemberIdVector, const AddUsersToListCb &resultFunctor, const ListMemberAttributes &mask = ListMemberAttributes());

        /*Deprecated - DO NOT USE - This method is the same as calling the non-mutual version.  To be removed in a future release. */
        JobId addUsersToListMutual(const ListMemberIdVector &listMemberIdVector, const AddUsersToListCb &resultFunctor) { return addUsersToList(listMemberIdVector, resultFunctor); }

        typedef Functor4<const AssociationList *, const ListMemberInfoVector *, BlazeError, JobId> SetUsersAttributesInListCb;
        /*! ****************************************************************************/
        /*! \brief Set user(s) attributes in the association list 

            On success the specified user(s) attributes are updated.

            The callback returns association list with the updated members, if successful,
            - There is no partial success, if one member to be added by the request fails, the add process is terminated
            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND -  if a user with indicated blaze id is not found in the association list
            - ASSOCIATIONLIST_ERR_USER_CAN_NOT_ADD_SELF - if a user trying to add itself to the list
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file
            - ASSOCIATIONLIST_ERR_LIST_IS_FULL_OR_TOO_MANY_USERS - if the list is already full or will be oversize with members in the list
            - ASSOCIATIONLIST_ERR_INVALID_USER - neither blaze id nor external id for user is valid

            \param[in] listMemberIdVector a list of member to be added to the association list
            \param resultFunctor the functor dispatched upon success/failure
            \param[in] mask a bitfield mask utilized to determine which attribute fields are intended to be updated
            \return JobId of task.
        ********************************************************************************/
        JobId setUsersAttributesInList(const ListMemberIdVector &listMemberIdVector, const SetUsersAttributesInListCb &resultFunctor, const ListMemberAttributes& mask);

        typedef Functor4<const AssociationList *, const ListMemberIdVector *, BlazeError, JobId> RemoveUsersFromListCb;
        /*! ****************************************************************************/
        /*! \brief Remove user(s) from the association list 

            On success the specified user(s) are removed from the associations list

            The callback returns association list on success added,
            - There is no partial success, if one member to be removed in the requirement list failed to
            be removed from the association list, the whole process will fail
            - Note that external users that do not correspond to an EA user in the db will be dropped from the add request.
            - ASSOCIATIONLIST_ERR_DUPLICATE_USER_FOUND - duplicate members found in the request
            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND - if a user with indicated blaze id is not found on server
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file
            - ASSOCIATIONLIST_MEMBER_NOT_FOUND_IN_THE_LIST - if the member to be removed is not in the list

            \param[in] listMemberIdVector a list of member to be removed from the association list
            \param[out] resultFunctor the functor dispatched upon success/failure
            \param validateUsers validates the users before removing them.
            \return JobId of task.
        ********************************************************************************/
        JobId removeUsersFromList(const ListMemberIdVector &listMemberIdVector, const RemoveUsersFromListCb &resultFunctor, bool validateUsers = true);

        /*Deprecated - DO NOT USE - This method is the same as calling the non-mutual version.  To be removed in a future release. */
        JobId removeUsersFromListMutual(const ListMemberIdVector &listMemberIdVector, const RemoveUsersFromListCb &resultFunctor, bool validateUsers = true) { return removeUsersFromList(listMemberIdVector, resultFunctor, validateUsers); }

        typedef Functor2<BlazeError, JobId> ClearListCb;
        /*! ****************************************************************************/
        /*! \brief remove all members in the specified list

            - ASSOCIATIONLIST_ERR_INVALID_LIST_NAME - if the list name is invalid
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file


            \param[out] resultFunctor the functor dispatched upon success/failure
            \return JobId of task.
        ********************************************************************************/
        JobId clearList(const ClearListCb &resultFunctor);
        
        /*Deprecated - DO NOT USE - This method is the same as calling the non-mutual version.  To be removed in a future release. */
        JobId clearListMutual(const ClearListCb &resultFunctor) { return clearList(resultFunctor); }

        /*! ****************************************************************************/
        /*! \brief Clear the old list and add user(s) to association list 

            On success the local user add user(s) specified to it's association list

            The callback returns association list on success added,
            - There is no partial success, if one member to be added in the requirement list failed to
            be added to the association list, the whole process will fail
            - Note that external users that do not correspond to an EA user in the db will be dropped from the add request.
            - ASSOCIATIONLIST_ERR_DUPLICATE_USER_FOUND - duplicate members found in the request
            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND -  if a user with indicated blaze id is not found on server
            - ASSOCIATIONLIST_ERR_USER_CAN_NOT_ADD_SELF - if a user trying to add itself to the list
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file
            - ASSOCIATIONLIST_ERR_LIST_IS_FULL_OR_TOO_MANY_USERS - if the list is already full or will be oversize with members in the list
            - ASSOCIATIONLIST_ERR_INVALID_USER - neither blaze id nor external id for user is valid

            \param[in] listMemberIdVector a list of member to be added to the association list
            \param resultFunctor the functor dispatched upon success/failure
            \param[in] mask a bitfield mask utilized to determine which attribute fields are intended to be updated
            \param validateUsers validates the users before adding them.
            \return JobId of task.
        ********************************************************************************/
        JobId setUsersToList(const ListMemberIdVector &listMemberIdVector, const AddUsersToListCb &resultFunctor, bool validateUsers = true, bool ignoreHash = false, const ListMemberAttributes &mask = ListMemberAttributes());

        /*Deprecated - DO NOT USE - This method is the same as calling the non-mutual version.  To be removed in a future release. */
        JobId setUsersToListMutual(const ListMemberIdVector &listMemberIdVector, const AddUsersToListCb &resultFunctor, bool validateUsers = true) { return setUsersToList(listMemberIdVector, resultFunctor, validateUsers); }

        typedef Functor2<BlazeError, JobId> SubscribeToListCb;
        /*! ************************************************************************************************/
        /*! \brief Subscribe to all members in the specified association list.
            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND -  if a user with indicated blaze id is not found on server
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file

            \param[in] resultFunctor - the functor dispatched upon success/failure to retrieve the list.
            \return JobId of task.
        ***************************************************************************************************/
        JobId subscribeToList(const SubscribeToListCb &resultFunctor);

        typedef Functor2<BlazeError, JobId> UnsubscribeFromListCb;
        /*! ************************************************************************************************/
        /*! \brief Unsubscribe from all members in the specified association list(s).
            - ASSOCIATIONLIST_ERR_USER_NOT_FOUND -  if a user with indicated blaze id is not found on server
            - ASSOCIATIONLIST_ERR_LIST_NOT_FOUND - if the list with listName is not in config file            

            \param[in] resultFunctor   the functor dispatched upon success/failure to unsubscribe from lists
            \return JobId of task.
        ***************************************************************************************************/
        JobId unsubscribeFromList(const UnsubscribeFromListCb &resultFunctor);

        /*! ************************************************************************************************/
        /*! \brief Clear all the members from the local list on SDK side.
        ***************************************************************************************************/
        void clearLocalList();

        /*! ************************************************************************************************/
        /*! \brief Allows you to copy the lists information into a ListInfo instance.
        ***************************************************************************************************/
        void copyInto(ListInfo &listInfo) const {
            ListInfo::copyInto(listInfo);
        }

    protected:
        /*! ****************************************************************************/
        /*! \brief Constructor. 
            
            \param[in] api         pointer to alist api
            \param[in] listMembers A ListMembers TDF containing the ListMembers this AssociationList should be initialized with.
            \param[in] memGroupId  mem group to be used by this class to allocate memory  

            Private as all construction should happen via the factory method.
        ********************************************************************************/
       AssociationList(AssociationListAPI *api, const ListMembers &listMembers, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
       
        /*! ****************************************************************************/
        /*! \brief Local list constructor. 
            
            \param[in] api         pointer to alist api
            \param[in] listId      The identification of the list.
            \param[in] memGroupId  mem group to be used by this class to allocate memory  

            Private as all construction should happen via the factory method.
        ********************************************************************************/
       AssociationList(AssociationListAPI *api, const ListIdentification& listId, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
       virtual ~AssociationList();

       void initFromServer(const ListMembers &listMembers);

       void addMembers(const ListMemberInfoVector &listMembers);
       void removeMembers(const ListMemberIdVector &listMembers, bool triggerDispatch = true);

    private:
        friend class AssociationListAPI;

        NON_COPYABLE(AssociationList);

        void updateListMembersCommon(UpdateListMembersRequest& request, const ListMemberIdVector& listMemberIdVector) const;
        uint32_t removeDuplicateMembers(const ListMemberInfoVector &listMemberInfoVector);

        void addMemberToIndicies(AssociationListMember &listMember);
        void removeMemberFromIndicies(const AssociationListMember &listMember);

        void processUpdateListMembersResponse(const UpdateListMembersResponse *response);


        void fetchListMembersCb(const Lists *response, BlazeError err, JobId jobid, const FetchMembersCb resultCb);
        void waitForFetchedUsers(BlazeError err, JobId jobid, const FetchMembersCb resultCb);
        void checkMemberHashCb(const Blaze::Association::GetMemberHashResponse* response, BlazeError err, JobId jobid, UpdateListMembersRequestPtr memberRequest, const AddUsersToListCb resultCb);
        void updateListMembersCb(const UpdateListMembersResponse *response, BlazeError err, JobId jobid, const AddUsersToListCb resultCb);
        void removeUsersFromListCb(const UpdateListMembersResponse *response, BlazeError err, JobId jobid, const RemoveUsersFromListCb resultCb);
        void subscribeToListCb(BlazeError err, JobId jobid, const SubscribeToListCb resultCb);
        void unsubscribeFromListCb(BlazeError err, JobId jobid, const UnsubscribeFromListCb resultCb);
        void clearListCb(BlazeError err, JobId jobid, const ClearListCb resultCb);
        void waitForUsers(UpdateListMembersResponsePtr response, BlazeError err, JobId jobid, const AddUsersToListCb resultCb);



        void onListUpdated(const UpdateListMembersResponse& notification);
        AssociationListMember*  addMember(const ListMemberInfo& listMemberInfo);
        void removeMember(AssociationListMember* listMember);

    private:
        friend class MemPool<AssociationList>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListByBlazeObjectIdNode, EA::TDF::ObjectId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListByListTypeNode, Blaze::Association::ListType>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListByListNameNode, const char8_t *>;

        AssociationListAPI *mAPI;

        bool mIsLocalOnly;

        uint32_t mPageOffset;
        uint32_t mMemberTotalCount;

        MemberVector mMemberVector;

        typedef eastl::intrusive_hash_map<BlazeId, ListMemberByBlazeIdNode, 67> MemberBlazeIdMap;
        MemberBlazeIdMap mMemberBlazeIdMap;

        typedef eastl::intrusive_hash_map<ExternalPsnAccountId, ListMemberByExternalPsnIdNode, 67> MemberExternalPsnIdMap;
        typedef eastl::intrusive_hash_map<ExternalXblAccountId, ListMemberByExternalXblIdNode, 67> MemberExternalXblIdMap;
        typedef eastl::intrusive_hash_map<ExternalSteamAccountId, ListMemberByExternalSteamIdNode, 67> MemberExternalSteamIdMap;
        typedef eastl::intrusive_hash_map<ExternalStadiaAccountId, ListMemberByExternalStadiaIdNode, 67> MemberExternalStadiaIdMap;
        typedef eastl::intrusive_hash_map<ExternalSwitchId, ListMemberByExternalSwitchIdNode, 67, EA::TDF::TdfStringHash> MemberExternalSwitchIdMap;
        typedef eastl::intrusive_hash_map<AccountId, ListMemberByAccountIdNode, 67> MemberAccountIdMap;
        MemberExternalPsnIdMap mMemberExternalPsnIdMap;
        MemberExternalXblIdMap mMemberExternalXblIdMap;
        MemberExternalSteamIdMap mMemberExternalSteamIdMap;
        MemberExternalStadiaIdMap mMemberExternalStadiaIdMap;
        MemberExternalSwitchIdMap mMemberExternalSwitchIdMap;
        MemberAccountIdMap mMemberAccountIdMap;

        typedef eastl::intrusive_hash_map<ListMemberId, ListMemberByPersonaNameAndNamespaceNode, 67,
            PersonaAndNamespaceHash, PersonaAndNamespaceEqualTo> MemberPersonaNameMap;
        MemberPersonaNameMap mMemberPersonaNameAndNamespaceMap;

        MemPool<AssociationListMember> mAListMemberPool;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor

        bool mInitialSetComplete; //!< Whether or not "SET" has been called on this list.
    };
} //Association
} // namespace Blaze

namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::Association::ListByBlazeObjectIdNode, EA::TDF::ObjectId>
    {
        const EA::TDF::ObjectId operator()(const Blaze::Association::ListByBlazeObjectIdNode &x) const
        { 
            return static_cast<const Blaze::Association::AssociationList &>(x).getBlazeObjectId();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListByListTypeNode, Blaze::Association::ListType>
    {
        Blaze::Association::ListType operator()(const Blaze::Association::ListByListTypeNode &x) const
        { 
            return static_cast<const Blaze::Association::AssociationList &>(x).getId().getListType();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListByListNameNode, const char8_t *>
    {
        const char8_t * operator()(const Blaze::Association::ListByListNameNode &x) const
        { 
            return static_cast<const Blaze::Association::AssociationList &>(x).getId().getListName();
        }
    };
}
#endif // BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_H
