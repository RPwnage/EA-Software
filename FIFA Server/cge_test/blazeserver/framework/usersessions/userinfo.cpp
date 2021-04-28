/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/userinfo.h"
#include "framework/tdf/userdefines.h"
#include "framework/rpc/usersessions_defines.h"
#include "framework/rpc/usersessionsslave.h"
#include "framework/component/blazerpc.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{

UserInfo::UserInfo(EA::Allocator::ICoreAllocator& allocator, const char8_t* debugName) :
    UserInfoData(allocator, debugName),
    mIsIndexed(false)
{
}

UserInfo::~UserInfo()
{
    if (mIsIndexed)
        gUserSessionManager->removeUserInfoFromIndices(this);
}

EA::TDF::ObjectId UserInfo::getBlazeObjectId() const
{
    return EA::TDF::ObjectId(Blaze::ENTITY_TYPE_USER, getId());
}

void UserInfo::filloutUserIdentification(const UserInfoData& userInfoData, UserIdentification &identification)
{
    identification.setBlazeId(userInfoData.getId());
    userInfoData.getPlatformInfo().copyInto(identification.getPlatformInfo());
    identification.setName(userInfoData.getPersonaName());
    identification.setAccountId(identification.getPlatformInfo().getEaIds().getNucleusAccountId());
    identification.setAccountLocale(userInfoData.getAccountLocale());
    identification.setAccountCountry(userInfoData.getAccountCountry());
    identification.setOriginPersonaId(userInfoData.getOriginPersonaId());
    identification.setPidId(userInfoData.getPidId());
    identification.setPersonaNamespace(userInfoData.getPersonaNamespace());
    
    identification.setIsPrimaryPersona(userInfoData.getIsPrimaryPersona());
}

void UserInfo::filloutUserCoreIdentification(const UserInfoData& userInfoData, CoreIdentification &identification)
{
    userInfoData.getPlatformInfo().copyInto(identification.getPlatformInfo());
    identification.setName(userInfoData.getPersonaName());
    identification.setPersonaNamespace(userInfoData.getPersonaNamespace());
    identification.setBlazeId(userInfoData.getId());
}

bool UserInfo::isUserIdentificationValid(const UserIdentification& identification)
{
    return (identification.getBlazeId() != INVALID_BLAZE_ID ||
            has1stPartyPlatformInfo(identification.getPlatformInfo()) ||
            (identification.getPlatformInfo().getClientPlatform() != INVALID &&
            (*identification.getName() != '\0' ||
            identification.getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID ||
            identification.getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID))
        );
}

}
