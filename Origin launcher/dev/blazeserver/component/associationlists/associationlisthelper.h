/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef ASSOCIATIONLISTHELPER_H
#define ASSOCIATIONLISTHELPER_H

#include <EASTL/map.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include "blazerpcerrors.h"
#include "associationlists/tdf/associationlists.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/util/safe_ptr_ext.h"
#include "framework/database/dbscheduler.h"
#include "EASTL/intrusive_ptr.h"

namespace Blaze
{
namespace Association
{

typedef eastl::vector<ListInfoPtr> ListInfoPtrVector;

typedef struct
{
    UserInfoPtr userInfo;
    ListMemberAttributes userAttributes;
} UserInfoAttributes;

typedef eastl::vector<UserInfoAttributes> UserInfoAttributesVector;


///////////////////////////////////////////////////////////////////////////
// forward declaration
//
class AssociationListsSlaveImpl;
class AssociationList;

///////////////////////////////////////////////////////////////////////////
// typedef
//

///////////////////////////////////////////////////////////////////////////
// A class used to hold an association list member in a specified association 
// list for a specified user. The member could be a local player (with blaze id)
// or an external member from first party
//
class AssociationListMember :
    public eastl::safe_object
{
    NON_COPYABLE(AssociationListMember);
public:
    AssociationListMember(AssociationList &ownerList, const UserInfoPtr& user, const TimeValue& timeAdded, const ListMemberAttributes& attributes);
    virtual ~AssociationListMember();

    void subscribe();
    void unsubscribe();

    AssociationList* getOwner() { return &mOwnerList; }
    const UserInfoPtr& getUserInfo() const { return mUser; }

    BlazeId getBlazeId() const { return mUser->getId(); }
    const TimeValue& getTimeAdded() const { return mTimeAdded; }
    void setTimeAdded(const TimeValue& value) { mTimeAdded = value; }

    void fillOutListMemberId(ListMemberId& listId, ClientPlatformType requestorPlatform) const;
    static void fillOutListMemberId(const UserInfoPtr& userPtr, const ListMemberAttributes& attr, ListMemberId& listId, ClientPlatformType requestorPlatform);
    void fillOutListMemberInfo(ListMemberInfo& info, ClientPlatformType requestorPlatform) const;

    void deleteIfUnreferenced(bool clearRefs);

    ListMemberAttributes getAttributes() { return mAttributes; }
    void setAttributes(ListMemberAttributes attr) { mAttributes.setBits(attr.getBits()); }

    void handleExistenceChanged(const BlazeId blazeId);

private:
    AssociationList &mOwnerList;
    UserInfoPtr mUser;    
    bool mSubscribed;
    TimeValue mTimeAdded;
    ListMemberAttributes mAttributes; 
};

typedef safe_ptr_ext_owner<AssociationListMember, class AssociationList> AssociationListMemberRef;
typedef eastl::vector<AssociationListMemberRef> AssociationListMemberRefVector;

///////////////////////////////////////////////////////////////////////////
// A class used to hold a specified association list for a user. Each 
// AssociationList owns a unique EA::TDF::ObjectId (used by UserSet) and has a list
// of AssociListMembers 
//
class AssociationList : 
    public eastl::safe_object
{
    NON_COPYABLE(AssociationList);

    public:        
        AssociationList(AssociationListsSlaveImpl& component, const ListInfo &listTemplate, BlazeId owner);
        virtual ~AssociationList();

        //DB operations        
        BlazeRpcError DEFINE_ASYNC_RET(loadFromDb(bool doSubscriptionCheck, const uint32_t maxResultCount = UINT32_MAX, const uint32_t offset = 0));
        BlazeRpcError DEFINE_ASYNC_RET(addMembers(UserInfoAttributesVector &users,  const ListMemberAttributes& mask, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform));
        BlazeRpcError DEFINE_ASYNC_RET(setMembersAttributes(UserInfoAttributesVector &users, const ListMemberAttributes& mask,  UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform));
        BlazeRpcError DEFINE_ASYNC_RET(setMembers(const UserInfoAttributesVector& users, const ListMemberIdVector& extMembers, MemberHash hashValue, const ListMemberAttributes& mask, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform));
        BlazeRpcError DEFINE_ASYNC_RET(setExternalMembers(const ListMemberIdVector& extMembers, DbConnPtr& dbCon));
        BlazeRpcError DEFINE_ASYNC_RET(removeMembers(const UserInfoAttributesVector& users, bool validateDeletion, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform));
        BlazeRpcError DEFINE_ASYNC_RET(clearMembers(ClientPlatformType requestorPlatform, UpdateListMembersResponse* updateResponse = nullptr, MemberHash hashValue = INVALID_MEMBER_HASH));
        BlazeRpcError DEFINE_ASYNC_RET(getMemberHash(MemberHash& hashValue));

        //Cache operations
        AssociationListMemberRef findMember(BlazeId blazeId) const;
        size_t getCachedMemberCount() const { return mMemberList.size(); }
        size_t getTotalCount() const { return mFullyLoaded ? mMemberList.size() : mTotalSize; }
        size_t getCurrentOffset() const { return mCurrentOffset; }

        void handleMemberUpdateNotification(const UpdateListMembersResponse &notification);

        void subscribe();
        void unsubscribe();

        void deleteIfUnreferenced(bool clearRefs = false); 
        
        const ListInfo& getInfo() const { return mInfo; }
        ListType getType() const { return mInfo.getId().getListType(); }                
        const char8_t* getName() const { return mInfo.getId().getListName(); }

        bool isMutual() const { return mInfo.getStatusFlags().getMutualAction(); }
        bool isInPair() const { return mInfo.getPairId() != LIST_TYPE_UNKNOWN; }
    
        BlazeId getOwnerBlazeId() const { return mInfo.getBlazeObjId().id; }
        AssociationListsSlaveImpl& getComponent() const { return mComponent; }
        UserSessionPtr getOwnerPrimarySession() const { return mOwnerPrimarySession; }
        void setOwnerPrimarySession(UserSessionPtr session) { mOwnerPrimarySession = session; }

        BlazeRpcError DEFINE_ASYNC_RET(fillOutMemberInfoVector(ListMemberInfoVector& vector, ClientPlatformType requestorPlatform, uint32_t offset = 0, uint32_t count = UINT32_MAX));

    private:
        typedef safe_ptr_ext<AssociationListMember> MemberPtr; 
        typedef eastl::vector<MemberPtr> AssociationListMemberVector;
        friend class AssociationListsSlaveImpl;

        void addMemberToCache(UserInfoPtr user, const TimeValue& timeAdded, bool doSubscriptionCheck, const ListMemberAttributes& attr, bool isPairedList);
        void removeMemberFromCache(BlazeId memberId);
        void clearMemberCache();


        static BlazeRpcError lockAndGetSize(DbConnPtr conn, BlazeId blazeId, ListType listType, size_t& listSize, bool cancelOnEmpty);
        BlazeRpcError finishRemove(DbResultPtr deletedResult, UpdateListMembersResponse& updateResponse, BlazeIdToUserInfoMap& idsToUsers, ClientPlatformType requestorPlatform);
        BlazeRpcError finishAddRemove(DbResultPtr deletedResult, DbResultPtr deletedResult2, DbResultPtr addedResult, UpdateListMembersResponse& upateResponse, ClientPlatformType requestorPlatform);
        void sendUpdateNotifications(const UpdateListMembersResponse& notification);
        BlazeRpcError getUsersForDbResult(const DbResultPtr& result, DbResultPtr dbResult2, BlazeIdToUserInfoMap& idsToUsers) const;

        const AssociationListMemberVector& getMembers() const  { return mMemberList;  }

    private:
        AssociationListsSlaveImpl& mComponent;
        UserSessionPtr mOwnerPrimarySession;
        ListInfo mInfo;
        
        AssociationListMemberVector mMemberList;

        typedef eastl::hash_map<BlazeId, MemberPtr> ListMemberByBlazeIdMap;
        ListMemberByBlazeIdMap mListMemberByBlazeIdMap;

        bool mFullyLoaded;
        bool mPartiallyLoaded;
        uint32_t mCurrentOffset;
        uint32_t mTotalSize;
};

typedef safe_ptr_ext<class AssociationList> AssociationListRef;
typedef eastl::vector<AssociationListRef> AssociationListVector;


} //Association
} //Blaze

#endif //ASSOCIATIONLISTHELPER_H
