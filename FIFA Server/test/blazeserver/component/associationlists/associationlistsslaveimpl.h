/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ASSOCIATIONLISTS_SLAVEIMPL_H
#define BLAZE_ASSOCIATIONLISTS_SLAVEIMPL_H

/*** Include files ******************************************************************************/
#include "framework/usersessions/usersessionsubscriber.h"
#include "framework/userset/userset.h"
#include "associationlists/rpc/associationlistsslave_stub.h"
#include "associationlists/tdf/associationlists_server.h"
#include "associationlisthelper.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
    class UserSession;

namespace Association
{



class AssociationListsSlaveImpl :
    public AssociationListsSlaveStub,
    public UserSetProvider,
    private UserSessionSubscriber,
    private LocalUserSessionSubscriber
                                  
{
public:
    AssociationListsSlaveImpl();
    ~AssociationListsSlaveImpl() override;
  
    const ListInfoPtrVector& getTemplates() const { return mListTemplates; }

    ListInfoPtr getTemplate(const ListIdentification& id) const;
    ListInfoPtr getTemplate(const char8_t *listName) const;
    ListInfoPtr getTemplate(ListType listType) const;

    AssociationListRef getListByObjectId(const EA::TDF::ObjectId& id);
    AssociationListRef getList(ListType type, BlazeId blazeId) { return getListByObjectId(EA::TDF::ObjectId(COMPONENT_ID, type, blazeId)); }
    AssociationListRef getList(const ListIdentification& listId, BlazeId blazeId);

    BlazeRpcError loadLists(const ListIdentificationVector& listIds, BlazeId ownerId, AssociationListVector& lists, const uint32_t maxResultCount = UINT32_MAX, const uint32_t offset = 0);
    AssociationListRef findOrCreateList(const ListIdentification &listIdentification, BlazeId ownerId) { return findOrCreateList(getTemplate(listIdentification), ownerId); }
    AssociationListRef findOrCreateList(ListInfoPtr temp, BlazeId ownerId);

    // Note: if ownerId is provided to validateListMemberIdVector, then ASSOCIATIONLIST_ERR_USER_NOT_FOUND is returned if the list owner can't be looked up,
    // and ASSOCIATIONLIST_ERR_INVALID_USER is returned if any list member is on a different platform from the list owner
    static BlazeRpcError validateListMemberIdVector(const ListMemberIdVector &listMemberIdVector, UserInfoAttributesVector& results, BlazeId ownerId = INVALID_BLAZE_ID, ListMemberIdVector* extMemberIdVector = nullptr);
    static BlazeRpcError checkUserEditPermission(const BlazeId blazeId);

    uint16_t getDbSchemaVersion() const override { return 3; }
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }

    BlazeRpcError checkListMembership(const ListIdentification& listIdentification, const BlazeIdList& ownersBlazeIds, const BlazeId memberBlazeId, BlazeIdList& existingOwnersBlazeIds) const;
    BlazeRpcError checkListContainsMembers(const ListIdentification& listIdentification, const BlazeId ownerBlazeId, const BlazeIdList& membersBlazeIds, BlazeIdList& existingMemberBlazeIds) const;

    void addPendingSubscribe(AssociationListMember& listMember);
    void removePendingSubscribe(AssociationListMember& listMember);

private:
    friend class AssociationList;

    GetConfigListsInfoError::Error processGetConfigListsInfo(ConfigLists& response,const Blaze::Message * message) override;
    
    ///////////////////////////////////////////////////////////////////////////
    // UserSetProvider interface functions
    //
    /* \brief Find all UserSessions matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids) override;

    /* \brief Find all BlazeIds matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results) override;

    /* \brief Find all UserIdentifications matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& results) override;

    /* \brief Count all UserSessions matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;

    /* \brief Count all Users matching EA::TDF::ObjectId (fiber) */
    BlazeRpcError countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count) override;

    RoutingOptions getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId) override;

    /* \brief Gets the member info vector. AssociationListRef is used if the data was already held locally. */
    BlazeRpcError loadListByObjectId(const EA::TDF::ObjectId& bobjId, AssociationListRef& ref);

    ///////////////////////////////////////////////////////////////////////////
    // UserSessionSubscriber interface functions
    //    
    void onUserSessionExistence(const UserSession& userSession) override;
    void onUserSessionExtinction(const UserSession& userSession) override;

    ///////////////////////////////////////////////////////////////////////////
    // LocalUserSessionSubscriber interface functions
    //    
    void onLocalUserSessionLogin(const UserSessionMaster& userSession) override;
    void onLocalUserSessionExported(const UserSessionMaster& userSession) override;

    ///////////////////////////////////////////////////////////////////////////
    // AssociationListsSlaveListener interface functions
    //
    void onNotifyUpdateListMembershipSlave(const Blaze::Association::UpdateListMembersResponse& data,Blaze::UserSession *) override;
    void handleUpdateListMembership(Blaze::Association::UpdateListMembersResponsePtr data);
    
    SlaveSession* getHostingSlaveSession(UserSessionId sessionId) const { return mSlaveList.getSlaveSessionByInstanceId(UserSession::getOwnerInstanceId(sessionId)); }

    bool onActivate() override;
    bool onConfigure() override;
    bool onResolve() override;
    bool onReconfigure() override;
    bool configureCommon();
    void onShutdown() override;

    bool onValidateConfig(AssociationlistsConfig& config, const AssociationlistsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    void validateConfigurePairLists(const ListDataVector& assocLists, ConfigureValidationErrors& validationErrors) const;

    void createOrCheckTables(BlazeRpcError& result);
    void onUserFirstLogin(BlazeId blazeId);
    
private:
    bool createOrReplaceTrigger(DbConnPtr conn, eastl::map<eastl::string, eastl::string>& activeTriggers, BlazeRpcError& result,
                                const char8_t* triggerName, const char8_t* triggerCondition, const char8_t* triggerAction, bool useVerbatimTriggerActionString = false);


    ///////////////////////////////////////////////////////////////////////////
    // Association list keyed by it's id, cached from configuration file
    // AssociationListTemplate obj created/deleted by slaveimpl, 
    // Objects created on heap, need to clear out upon destruction

    // Due to functional behavior in AssociationListMember dtor (namely, unsubscribe()), mPendingSubscribeMap must be decl'd
    // before mUserSessionToListsMap to prevent accessing deleted memory during this class's dtor (via AssociationListMember dtor)
    typedef eastl::hash_multimap<BlazeId, AssociationListMember*> ListMemberByBlazeId;
    typedef eastl::pair<ListMemberByBlazeId::iterator, ListMemberByBlazeId::iterator> ListMemberByBlazeIdRange;
    ListMemberByBlazeId mPendingSubscribeMap;

    typedef eastl::hash_map<eastl::string, ListInfoPtr, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ListNameTemplateMap;
    ListNameTemplateMap mListNameToListTemplateMap;

    typedef eastl::hash_map<ListType, ListInfoPtr> ListTypeTemplateMap;
    ListTypeTemplateMap mListTypeToListTemplateMap;

    ListInfoPtrVector mListTemplates;

    typedef eastl::hash_multimap<UserSessionId, AssociationListRef> UserSessionIdToAssociationListMap;
    UserSessionIdToAssociationListMap mUserSessionToListsMap;

    typedef eastl::hash_map<EA::TDF::ObjectId, AssociationListRef> BlazeObjectIdToAssociationListMap;
    BlazeObjectIdToAssociationListMap mBlazeObjectIdToAssociationListMap;
};

} // Association
} // Blaze

#endif // BLAZE_ASSOCIATIONLISTS_SLAVEIMPL_H

