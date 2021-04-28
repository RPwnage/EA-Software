/*************************************************************************************************/
/*!
    \file associationlistmember.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/associationlists/associationlistmember.h"
#include "BlazeSDK/associationlists/associationlist.h"
#include "BlazeSDK/associationlists/associationlistapi.h"

#include "BlazeSDK/component/associationlistscomponent.h"   

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "DirtySDK/dirtysock/dirtynames.h"

namespace Blaze
{
namespace Association
{

AssociationListMember::AssociationListMember(const AssociationList &list, const ListMemberInfo &memberInfo, MemoryGroupId memGroupId)
    : mAssociaionList(list),
      mUser(nullptr)
{
    memberInfo.copyInto(*this);
}

AssociationListMember::~AssociationListMember()
{
}

ExternalId AssociationListMember::getExternalId() const 
{
    return mAssociaionList.getAssociationListAPI()->getBlazeHub()->getExternalIdFromPlatformInfo(getPlatformInfo());
}

const PlatformInfo& AssociationListMember::getPlatformInfo() const
{
    return ListMemberInfo::getListMemberId().getUser().getPlatformInfo();
}


const UserManager::User* AssociationListMember::getUser() const
{
    AssociationListMember *member = const_cast<AssociationListMember *>(this);
    if (mUser == nullptr)
    {
        AssociationListAPI *api = mAssociaionList.getAssociationListAPI();

        if (mListMemberId.getUser().getBlazeId() != INVALID_BLAZE_ID)
        {
            member->mUser = api->getBlazeHub()->getUserManager()->getUserById(mListMemberId.getUser().getBlazeId());
        }
        else
        {
            member->mUser = api->getBlazeHub()->getUserManager()->getUserByPlatformInfo(mListMemberId.getUser().getPlatformInfo());
        }
    }

    return mUser;
}

size_t Blaze::Association::PersonaAndNamespaceHash::operator()(const ListMemberId& listMemberId) const
{
    CaseInsensitiveStringHash npResult;
    return static_cast<size_t>(DirtyUsernameHash(listMemberId.getUser().getName())) * npResult(listMemberId.getUser().getPersonaNamespace());
}

bool Blaze::Association::PersonaAndNamespaceEqualTo::operator()(const ListMemberId& listMemberId1, const ListMemberId& listMemberId2) const
{
    CaseInsensitiveStringEqualTo npResult;
    return (DirtyUsernameCompare(listMemberId1.getUser().getName(), listMemberId2.getUser().getName()) == 0) && npResult(listMemberId1.getUser().getPersonaNamespace(), listMemberId2.getUser().getPersonaNamespace());
}

} // namespace Association
} // namespace Blaze
