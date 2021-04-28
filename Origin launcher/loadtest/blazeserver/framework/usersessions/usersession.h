/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USER_SESSION_H
#define BLAZE_USER_SESSION_H

/*** Include files *******************************************************************************/
#include "framework/blazedefines.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/rpc/usersessionsmaster.h"
#include "framework/usersessions/authorization.h"
#include "framework/usersessions/userinfo.h"

#include "EASTL/intrusive_list.h"
#include "EASTL/set.h"

#include "openssl/sha.h" // for SHA256

#define COMPONENT_ID_FROM_UED_KEY(key) ((uint16_t)((key >> 16) & 0xFFFF))
#define DATA_ID_FROM_UED_KEY(key) ((uint16_t)(key & 0xFFFF))
#define UED_KEY_FROM_IDS(compId, dataId) ((uint32_t)(compId << 16) | dataId)

namespace Blaze
{

class UserSessionData;
class ReplicatedConnectionGroup;
class UserSessionManager;
class UpdateBlazeObjectIdInfo;


// forward dec for friend class needed to access debug method
namespace Arson
{
    class ForceLostConnectionCommand;
}

class UserSession
{
    NON_COPYABLE(UserSession);
public:
    static const UserSessionId INVALID_SESSION_ID = INVALID_USER_SESSION_ID; // BuildSliverKey(INVALID_SLIVER_ID, 0);

    static bool isValidUserSessionId(UserSessionId userSessionId) { return (userSessionId != INVALID_SESSION_ID); }

    UserSession(UserSessionExistenceData* existenceData);
    virtual ~UserSession();

    inline UserSessionId getUserSessionId() const { return mExistenceData->getUserSessionId(); }
    inline UserSessionType getUserSessionType() const { return mExistenceData->getUserSessionType(); }
    inline BlazeId getBlazeId() const { return mExistenceData->getBlazeId(); }
    inline PlatformInfo& getPlatformInfo() { return mExistenceData->getPlatformInfo(); }
    inline const PlatformInfo& getPlatformInfo() const { return mExistenceData->getPlatformInfo(); }
    inline AccountId getAccountId() const { return mExistenceData->getPlatformInfo().getEaIds().getNucleusAccountId(); }
    inline BlazeId getUserId() const { return getBlazeId(); }  // DEPRECATED
    inline const char8_t* getPersonaName() const { return mExistenceData->getPersonaName(); }
    inline const char8_t* getPersonaNamespace() const { return mExistenceData->getPersonaNamespace(); }
    inline const char8_t* getUniqueDeviceId() const { return mExistenceData->getUniqueDeviceId(); }
    inline DeviceLocality getDeviceLocality() const { return mExistenceData->getDeviceLocality(); }
    inline ClientType getClientType() const { return mExistenceData->getClientType(); }
    inline ClientPlatformType getClientPlatform() const { return mExistenceData->getPlatformInfo().getClientPlatform(); }
    inline const char8_t* getClientVersion() const { return mExistenceData->getClientVersion(); }
    inline const char8_t* getBlazeSDKVersion() const { return mExistenceData->getBlazeSDKVersion(); }
    inline const char8_t* getProtoTunnelVer() const { return mExistenceData->getProtoTunnelVer(); }
    inline uint32_t getSessionLocale() const { return mExistenceData->getSessionLocale(); }
    inline void setSessionLocale(uint32_t locale) const { return mExistenceData->setSessionLocale(locale); }
    inline uint32_t getSessionCountry() const { return mExistenceData->getSessionCountry(); }
    inline uint32_t getAccountLocale() const { return mExistenceData->getAccountLocale(); }
    inline uint32_t getAccountCountry() const { return mExistenceData->getAccountCountry(); }
    inline uint32_t getPreviousAccountCountry() const { return mExistenceData->getPreviousAccountCountry(); }
    inline const char8_t* getServiceName() const { return mExistenceData->getServiceName(); }
    inline const char8_t* getProductName() const { return mExistenceData->getProductName(); }
	inline const EA::TDF::TimeValue& getCreationTime() const{ return mExistenceData->getCreationTime(); }

    inline const char8_t* getAuthGroupName() const { return mExistenceData->getAuthGroupName(); }
    inline const SessionFlags& getSessionFlags() const { return mExistenceData->getSessionFlags(); }
    inline const InstanceIdList& getInstanceIds() const { return mExistenceData->getInstanceIds(); }
    inline const UserSessionExistenceData& getExistenceData() const { return *mExistenceData; }

    // Deprecated accessor functions.
    inline UserSessionId getId() const { return getUserSessionId(); }
    inline UserSessionId getSessionId() const { return getUserSessionId(); }

    // "Shortcut" functions that just provide easier access to other public data members.
    bool isLocal() const;
    const ClientTypeFlags& getClientTypeFlags() const;
    bool isPrimarySession() const       { return getClientTypeFlags().getPrimarySession(); }
    bool isGameplayUser() const         { return getClientTypeFlags().getAllowAsGamePlayer(); }
    bool getEnforceSingleLogin() const  { return getClientTypeFlags().getEnforceSingleLogin(); }
    bool canBeInLocalConnGroup() const  { return getClientTypeFlags().getAllowAsLocalConnGroupMember(); }

    bool isGuestSession() const                 { return (getUserSessionType() == USER_SESSION_GUEST); }
    bool canBeParentToGuestSession() const      { return (getUserSessionType() == USER_SESSION_NORMAL) && isGameplayUser(); }
    bool canBeParentToWalCloneSession() const   { return (getUserSessionType() == USER_SESSION_NORMAL); }

    static bool isGameplayUser(UserSessionId userSessionId);

    /*! \brief Returns true if specified id belongs to a session owned by the current core slave instance. */
    static bool isOwnedByThisInstance(UserSessionId userSessionId);
    static InstanceId getOwnerInstanceId(UserSessionId userSessionId);

    SliverId getSliverId() const { return GetSliverIdFromSliverKey(getUserSessionId()); }
    SliverIdentity getSliverIdentity() const { return MakeSliverIdentity(UserSessionsMaster::COMPONENT_ID, getSliverId()); }
    static SliverIdentity makeSliverIdentity(UserSessionId userSessionId);

    /*! \brief Returns true if this session has the necessary permission. */
    bool isAuthorized(const Authorization::Permission& permission, bool suppressWarnLog = false) const;
    static bool isAuthorized(UserSessionId userSessionId, const Authorization::Permission& permission, bool suppressWarnLog = false);

    /*! \brief Returns true if gCurrentUserSession session has the necessary permission, or if running in the superuser context. */
    static bool isCurrentContextAuthorized(const Authorization::Permission& permission, bool suppressWarnLog = false);

    static bool getDataValue(const UserExtendedDataMap& extendedDataMap, const UserExtendedDataName& name, UserExtendedDataValue &value);
    static bool getDataValue(const UserExtendedDataMap& extendedDataMap, UserExtendedDataKey& key, UserExtendedDataValue &value);
    static bool getDataValue(const UserExtendedDataMap& extendedDataMap, ComponentId componentId, uint16_t key, UserExtendedDataValue &value);

    /*! \brief Add/Remove EA::TDF::ObjectId or ObjectType from the ObjectIdList inside UserSessionData. */
    static void updateBlazeObjectId(UserSessionData& session, const Blaze::UpdateBlazeObjectIdInfo &info);

    /**************************************************************************************************************/
    // TODO: Add diagnostic/debug helpers to the implementations of these methods.  Bugs in and around UserSession subscriptions have
    //       historically been very difficult to debug.  Having access to data/tools/information to help diagnose these bugs will
    //       save time in the future.
    static void updateUsers(UserSessionId targetId, const BlazeIdList& blazeIds, SubscriptionAction action);
    static void updateUsers(const UserSessionIdList& targetIds, BlazeId blazeId, SubscriptionAction action);
    static void updateUsers(const UserSessionIdList& targetIds, const BlazeIdList& blazeIds, SubscriptionAction action);

    static void updateNotifiers(UserSessionId targetId, const UserSessionIdList& notifierIds, SubscriptionAction action);
    static void updateNotifiers(const UserSessionIdList& targetIds, UserSessionId notifierId, SubscriptionAction action);
    static void updateNotifiers(const UserSessionIdList& targetIds, const UserSessionIdList& notifierIds, SubscriptionAction action);

    static void mutualSubscribe(UserSessionId userSessionId, const UserSessionIdList& userSessionIds, SubscriptionAction action);

    template<typename InputIterator, typename UserSessionIdFetchFunction>
    static void fillUserSessionIdList(UserSessionIdList& result, InputIterator idsBegin, InputIterator idsEnd, UserSessionIdFetchFunction fetchFunction)
    {
        result.clear();
        for (; idsBegin != idsEnd; ++idsBegin)
            result.push_back(fetchFunction(*idsBegin));
    }
    
    /**************************************************************************************************************/


    /*! \brief Increments the super user privilege counter.  If the counter is > 0, the privelege is true */
    static void pushSuperUserPrivilege();

    /*! \brief Decrements the super user privilege counter. If the counter == 0, the privilege is false.  */
    static void popSuperUserPrivilege();

    /*! \brief Clears the super user privilege counter regardless of the setting.  Should not be in general use.  */
    static void clearSuperUserPrivilege() { msSuperUserPrivilegeCounter = 0; }

    /*! \brief Returns whether or not the super user privilege is set. */
    static bool hasSuperUserPrivilege() { return msSuperUserPrivilegeCounter > 0; }

    class SuperUserPermissionAutoPtr
    {
    public:
        SuperUserPermissionAutoPtr(bool enable) : mEnable(enable)
        {
            if (mEnable)
                UserSession::pushSuperUserPrivilege();
        }

        ~SuperUserPermissionAutoPtr()
        {
            if (mEnable)
                UserSession::popSuperUserPrivilege();
        }

    private:  
        bool mEnable;
    };

    static void setCurrentUserSessionId(UserSessionId userSessionId);
    static UserSessionId getCurrentUserSessionId();
    static BlazeId getCurrentUserBlazeId(); 
protected:
    bool isAuxLocal() const;
    uint32_t mRefCount;
    UserSessionExistenceDataPtr mExistenceData;

private:
    friend class UserSessionManager;
    friend class UserSessionsMasterImpl;
    friend void intrusive_ptr_add_ref(UserSession* ptr);
    friend void intrusive_ptr_release(UserSession* ptr);

    static FiberLocalVar<UserSessionId> msCurrentUserSessionId;
    static FiberLocalVar<uint32_t> msSuperUserPrivilegeCounter;
};

struct UserSessionIdIdentityFunc
{
    UserSessionIdIdentityFunc(UserSessionId skipId = INVALID_USER_SESSION_ID) : mSkipId(skipId) {}
    UserSessionId operator()(UserSessionId userSessionId) { return (userSessionId != mSkipId ? userSessionId : INVALID_USER_SESSION_ID); }
private:
    UserSessionId mSkipId;
};

struct SessionIdSetIdentityFunc
{
    SessionIdSetIdentityFunc()
    {
    }

    void addSessionId(UserSessionId s) { mSkipSessionIdSet.insert(s); }

    UserSessionId operator()(UserSessionId s) { return mSkipSessionIdSet.find(s) == mSkipSessionIdSet.end() ? s : INVALID_USER_SESSION_ID ;}

private:
    eastl::set<UserSessionId> mSkipSessionIdSet; 
};


//These are helper structs for the user session wrapper

struct BlazeIdListNode : public eastl::intrusive_list_node {};
struct AccountIdListNode : public eastl::intrusive_list_node {};
struct ExternalPsnAccountIdListNode : public eastl::intrusive_list_node {};
struct ExternalXblAccountIdListNode : public eastl::intrusive_list_node {};
struct ExternalSwitchIdListNode : public eastl::intrusive_list_node {};
struct ExternalSteamAccountIdListNode : public eastl::intrusive_list_node {};
struct ExternalStadiaAccountIdListNode : public eastl::intrusive_list_node {};
struct PrimarySessionListNode : public eastl::intrusive_list_node {};

typedef eastl::intrusive_list<BlazeIdListNode> BlazeIdExistenceList;
typedef eastl::intrusive_list<AccountIdListNode> AccountIdExistenceList;
typedef eastl::intrusive_list<ExternalPsnAccountIdListNode> ExternalPsnAccountIdExistenceList;
typedef eastl::intrusive_list<ExternalXblAccountIdListNode> ExternalXblAccountIdExistenceList;
typedef eastl::intrusive_list<ExternalSwitchIdListNode> ExternalSwitchIdExistenceList;
typedef eastl::intrusive_list<ExternalSteamAccountIdListNode> ExternalSteamAccountIdExistenceList;
typedef eastl::intrusive_list<ExternalStadiaAccountIdListNode> ExternalStadiaAccountIdExistenceList;
typedef eastl::intrusive_list<PrimarySessionListNode> PrimarySessionExistenceList;

class UserSessionExistence :
    public UserSession,
    public BlazeIdListNode,
    public AccountIdListNode,
    public ExternalPsnAccountIdListNode,
    public ExternalXblAccountIdListNode,
    public ExternalSwitchIdListNode,
    public ExternalSteamAccountIdListNode,
    public ExternalStadiaAccountIdListNode,
    public PrimarySessionListNode
{
    NON_COPYABLE(UserSessionExistence);
public:
    UserSessionExistence();
    ~UserSessionExistence() override;

    BlazeRpcError fetchSessionData(UserSessionDataPtr& sessionData) const;
    BlazeRpcError fetchUserInfo(UserInfoPtr& userInfo) const;

protected:
    void removeFromAllLists();

    friend void intrusive_ptr_add_ref(UserSessionExistence* ptr);
    friend void intrusive_ptr_release(UserSessionExistence* ptr);
private:
    friend class UserSessionManager;
    friend class UserSessionsMasterImpl;
};

typedef eastl::intrusive_ptr<UserSessionExistence> UserSessionExistencePtr;
typedef UserSessionExistencePtr UserSessionPtr; // Backward compatibility

extern FiberLocalWrappedPtr<UserSessionExistencePtr, UserSessionExistence> gCurrentUserSession;

void intrusive_ptr_add_ref(UserSessionExistence* ptr);
void intrusive_ptr_release(UserSessionExistence* ptr);

BlazeRpcError convertToPlatformInfo(CoreIdentification& coreIdent);
BlazeRpcError convertToPlatformInfo(UserIdentification& userIdent);
BlazeRpcError convertToPlatformInfo(PlatformInfo& platformInfo, ExternalId extId, const char8_t* extString, AccountId accountId, ClientPlatformType platform);
BlazeRpcError convertToPlatformInfo(PlatformInfo& platformInfo, ExternalId extId, const char8_t* extString, AccountId accountId, OriginPersonaId originPersonaId, const char8_t* originPersonaName, ClientPlatformType platform);
BlazeRpcError convertFromPlatformInfo(const PlatformInfo& platformInfo, ExternalId* extId, eastl::string* extString, AccountId* accountId);
bool has1stPartyPlatformInfo(const PlatformInfo& platformInfo);
const char8_t* platformInfoToString(const PlatformInfo& platformInfo, eastl::string& str);
ExternalId getExternalIdFromPlatformInfo(const PlatformInfo& platformInfo);


} //namespace Blaze

#endif  //BLAZE_USER_SESSION_H
