/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USER_INFO_H
#define BLAZE_USER_INFO_H

/*** Include files *******************************************************************************/
#include "framework/tdf/userinfotypes_server.h"
#include "EASTL/intrusive_ptr.h"

namespace Blaze
{

class UserIdentification;

class UserInfo :
    public UserInfoData
{
public: 
    /*! \brief Constructor */
    UserInfo(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR), const char8_t* debugName = "");
    ~UserInfo() override;

    bool getIsIndexed() const { return mIsIndexed; }

    //Identity functions
    virtual EA::TDF::ObjectId getBlazeObjectId() const;
    virtual const char8_t *getIdentityName() const { return getPersonaName(); }

    void filloutUserIdentification(UserIdentification &identification) const { UserInfo::filloutUserIdentification(*this, identification); }
    void filloutUserCoreIdentification(CoreIdentification &identification) const { UserInfo::filloutUserCoreIdentification(*this, identification); }

    static void filloutUserIdentification(const UserInfoData& userInfoData, UserIdentification &identification);
    static void filloutUserCoreIdentification(const UserInfoData& userInfoData, CoreIdentification &identification);

    //void filloutUserIdentification(UserIdentification &identification) const;
    //void filloutUserCoreIdentification(CoreIdentification &identification) const;
    static bool isUserIdentificationValid(const UserIdentification& identification);

private:
    friend class UserSessionManager;
    friend class UserSessionMaster;

    bool mIsIndexed;
    void setIsIndexed(bool isIndexed) { mIsIndexed = isIndexed; }

    friend void intrusive_ptr_add_ref(UserInfo*);
    friend void intrusive_ptr_release(UserInfo*);
    virtual void AddRef() const { UserInfoData::AddRef(); }
    virtual void Release() const { UserInfoData::Release(); }
};

typedef eastl::intrusive_ptr<UserInfo> UserInfoPtr;

inline void intrusive_ptr_add_ref(UserInfo* ptr) { ptr->AddRef(); }
inline void intrusive_ptr_release(UserInfo* ptr) { ptr->Release(); }

typedef eastl::vector<BlazeId> BlazeIdVector;
typedef eastl::vector_set<BlazeId> BlazeIdVectorSet;
typedef eastl::vector_map<BlazeId, UserInfoPtr> BlazeIdToUserInfoMap;
typedef eastl::vector<const char8_t*> PersonaNameVector;
typedef eastl::vector_map<const char8_t*, UserInfoPtr> PersonaNameToUserInfoMap;
typedef eastl::hash_map<const char8_t*, PersonaNameToUserInfoMap> NamespaceToPersonaUserInfoMap;
typedef eastl::vector<PlatformInfo> PlatformInfoVector;
typedef eastl::vector_map<ExternalId, UserInfoPtr> ExternalIdToUserInfoMap;
typedef eastl::vector_map<OriginPersonaId, UserInfoPtr> OriginPersonaIdToUserInfoMap;
typedef eastl::vector<AccountId> AccountIdVector;
typedef eastl::hash_set<AccountId> AccountIdSet;
typedef eastl::vector<OriginPersonaId> OriginPersonaIdVector;
typedef eastl::hash_set<BlazeId> BlazeIdSet;
typedef eastl::vector<UserInfoPtr> UserInfoPtrList;

typedef eastl::map<ClientPlatformType, UserInfo*> UserInfoByPlatform;
typedef eastl::vector_map<const char8_t*, UserInfoPtrList> UserInfoListByPersonaNameMap;
typedef eastl::hash_map<const char8_t*, UserInfoListByPersonaNameMap> UserInfoListByPersonaNameByNamespaceMap;
typedef eastl::map<ClientPlatformType, BlazeIdVector> BlazeIdsByPlatformMap;
typedef eastl::vector_map<ClientPlatformType, PlatformInfoVector> PlatformInfoMap;
typedef eastl::hash_map<AccountId, PlatformInfo> PlatformInfoByAccountIdMap;
typedef eastl::map<ClientPlatformType, uint64_t> CountByPlatformMap;

}

#endif //BLAZE_USER_INFO_H
