/*************************************************************************************************/
/*!
    \file associationlistmember.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_MEMBER_H
#define BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_MEMBER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/component/associationlists/tdf/associationlists.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/usercontainer.h"


namespace Blaze
{
    class BlazeHub;


namespace Association
{   
    class AssociationList;

    struct BLAZESDK_API ListMemberNode              : public eastl::intrusive_list_node {};
    struct BLAZESDK_API ListMemberByBlazeIdNode     : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListMemberByExternalPsnIdNode : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListMemberByExternalXblIdNode : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListMemberByExternalSteamIdNode : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListMemberByExternalStadiaIdNode : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListMemberByExternalSwitchIdNode : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListMemberByAccountIdNode : public eastl::intrusive_hash_node {};
    struct BLAZESDK_API ListMemberByPersonaNameAndNamespaceNode : public eastl::intrusive_hash_node {};
    

    /*! **************************************************************************************************************/
    /*! 
        \class AssociationListMember

        \brief
        A AssociationListMember object represents a member of a AssociationList.

        \nosubgrouping
    ******************************************************************************************************************/
    class BLAZESDK_API AssociationListMember :
        protected ListMemberInfo,
        protected ListMemberNode,
        protected ListMemberByBlazeIdNode,
        protected ListMemberByExternalPsnIdNode,
        protected ListMemberByExternalXblIdNode,
        protected ListMemberByExternalSteamIdNode,
        protected ListMemberByExternalStadiaIdNode,
        protected ListMemberByExternalSwitchIdNode,
        protected ListMemberByAccountIdNode,
        protected ListMemberByPersonaNameAndNamespaceNode
    {        
    public:

        /*! ****************************************************************************/
        /*! \brief Returns the User object for this AssociationListMember.
            
            \return The User object.
        ********************************************************************************/
        const UserManager::User* getUser() const;

        /*! ****************************************************************************/
        /*! \brief Returns the AssociationList object associated with this AssociationListMember.
            
            \return The AssociationList object that owns this member.
        ********************************************************************************/
        const AssociationList &getAssociationList() const { 
            return mAssociaionList;
        }

        /*! ****************************************************************************/
        /*! \brief Returns the BlazeId (user id) of this member.  If this is an external member (not a Blaze user), then this will return INVALID_BLAZE_ID

            \return The BlazeId (user id) of this member.
        ********************************************************************************/
        BlazeId getBlazeId() const { 
            return ListMemberInfo::getListMemberId().getUser().getBlazeId();
        }

        /*! ****************************************************************************/
        /*! \brief Returns the ClientPlatformType of this member, provide the context for ExternalId.

            \return ClientPlatformType
        ********************************************************************************/
        ClientPlatformType getExternalSystemId() const { 
            return ListMemberInfo::getListMemberId().getExternalSystemId();
        }

        /*! ****************************************************************************/
        /*! \brief DEPRECATED - Returns the ExternalId (XUID or PSNID) of this member.  On PC, this value will always be 0

            \return The ExternalId (XUID or PSNID) of this member.
        ********************************************************************************/
        ExternalId getExternalId() const;

        /*! ****************************************************************************/
       /*! \brief return the platform info for the user.  (ex. XblId, PsnId, OriginId, etc.)
       ********************************************************************************/
        const PlatformInfo& getPlatformInfo() const;

        /*! ****************************************************************************/
        /*! \brief return the Xbl id for the user.  (INVALID_XBL_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalXblAccountId getXblAccountId() const { return getPlatformInfo().getExternalIds().getXblAccountId(); }

        /*! ****************************************************************************/
        /*! \brief return the Psn id for the user.  (INVALID_PSN_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalPsnAccountId getPsnAccountId() const { return getPlatformInfo().getExternalIds().getPsnAccountId(); }
        
        /*! ****************************************************************************/
        /*! \brief return the steam id for the user.  (INVALID_STEAM_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalSteamAccountId getSteamAccountId() const { return getPlatformInfo().getExternalIds().getSteamAccountId(); }

        /*! ****************************************************************************/
        /*! \brief return the stadia id for the user.  (INVALID_STADIA_ACCOUNT_ID otherwise)
        ********************************************************************************/
        ExternalStadiaAccountId getStadiaAccountId() const { return getPlatformInfo().getExternalIds().getStadiaAccountId(); }

        /*! *********************************************************************************************/
        /*! \brief return the Switch id (NSA id with environment prefix) for the user.  (empty string otherwise)
        *************************************************************************************************/
        ExternalSwitchId getSwitchId() const { return getPlatformInfo().getExternalIds().getSwitchIdAsTdfString(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Nucleus Account Id, which is the primary id used for Origin operations (not the Origin Persona Id).
        \return    Returns the accountId value.
        **************************************************************************************************************/
        AccountId getNucleusAccountId() const { return getPlatformInfo().getEaIds().getNucleusAccountId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Origin Persona Id.
        \return    Returns the personaid for this user's origin account.
        **************************************************************************************************************/
        OriginPersonaId getOriginPersonaId() const { return getPlatformInfo().getEaIds().getOriginPersonaId(); }

        /*! **********************************************************************************************************/
        /*! \brief Returns the Origin Persona Name.
        \return    Returns the PersonaName for this user's origin account.
        **************************************************************************************************************/
        const char8_t* getOriginPersonaName() const { return getPlatformInfo().getEaIds().getOriginPersonaName(); }

        /*! ****************************************************************************/
        /*! \brief Returns the persona of this member.  This can be used for is an external member (not a Blaze user) to get the name as set when the user was added

            \return The persona of this member.
        ********************************************************************************/
        const char8_t *getPersonaName() const { 
            return ListMemberInfo::getListMemberId().getUser().getName();
        }

        /*! ****************************************************************************/
        /*! \brief Returns the persona namespace of this member.

            \return The persona of this member.
        ********************************************************************************/
        const char8_t *getPersonaNamespace() const { 
            return ListMemberInfo::getListMemberId().getUser().getPersonaNamespace();
        }

        /*! ****************************************************************************/
        /*! \brief Returns the client platform type of this member.

            \return The persona of this member.
        ********************************************************************************/
        ClientPlatformType getClientPlatform() const {
            return ListMemberInfo::getListMemberId().getUser().getPlatformInfo().getClientPlatform();
        }

        /*! ****************************************************************************/
        /*! \brief Returns the time the member added to the association list

            Note the smaller the value, the longer time the member is in the list
            
            \return Added time since association list creation in seconds.
        ********************************************************************************/
        int64_t getAddedTime() const { 
            return mTimeAdded;
        }

        /*! ****************************************************************************/
        /*! \brief Returns the member's attributes
            
            \return Bitfield representation of the member's attributes
        ********************************************************************************/
        const ListMemberAttributes &getAttributes() const { 
            return ListMemberInfo::getListMemberId().getAttributes();
        }
        
        /*! ************************************************************************************************/
        /*! \brief Allows you to copy the list members information into a ListMemberInfo instance.
        ***************************************************************************************************/
        void copyInto(ListMemberInfo &memberInfo) const {
            ListMemberInfo::copyInto(memberInfo);
        }

    private:
        friend class AssociationList;
        friend class AssociationListAPI;
        friend class MemPool<AssociationListMember>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByBlazeIdNode, Blaze::BlazeId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByExternalPsnIdNode, Blaze::ExternalPsnAccountId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByExternalXblIdNode, Blaze::ExternalXblAccountId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByExternalSteamIdNode, Blaze::ExternalSteamAccountId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByExternalStadiaIdNode, Blaze::ExternalStadiaAccountId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByExternalSwitchIdNode, Blaze::ExternalSwitchId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByAccountIdNode, Blaze::AccountId>;
        friend struct eastl::use_intrusive_key<Blaze::Association::ListMemberByPersonaNameAndNamespaceNode, Blaze::Association::ListMemberId>;

        NON_COPYABLE(AssociationListMember);

        /*! ****************************************************************************/
        /*! \brief Constructor. 
        
            \param[in] aList       pointer to association list object
            \param[in] memberInfo  pointer to association list member info
            \param[in] memGroupId  mem group to be used by this class to allocate memory  

            Private as all construction should happen via the factory method.
        ********************************************************************************/
        AssociationListMember(const AssociationList &list, const ListMemberInfo &memberInfo, MemoryGroupId memGroupId = Blaze::MEM_GROUP_FRAMEWORK_TEMP);
        virtual ~AssociationListMember();

    private:
        const AssociationList& mAssociaionList;
        const UserManager::User* mUser;
    };

    struct PersonaAndNamespaceHash
    {
        size_t operator()(const ListMemberId& listMemberId) const;
    };

    struct PersonaAndNamespaceEqualTo
    {
        bool operator()(const ListMemberId& listMemberId1, const ListMemberId& listMemberId2) const;
    };

} //Association
} // namespace Blaze

namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByBlazeIdNode, Blaze::BlazeId>
    {
        Blaze::BlazeId operator()(const Blaze::Association::ListMemberByBlazeIdNode &x) const
        { 
            return static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId().getUser().getBlazeId();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByExternalPsnIdNode, Blaze::ExternalPsnAccountId>
    {
        Blaze::ExternalPsnAccountId operator()(const Blaze::Association::ListMemberByExternalPsnIdNode &x) const
        { 
            return static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId().getUser().getPlatformInfo().getExternalIds().getPsnAccountId();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByExternalXblIdNode, Blaze::ExternalXblAccountId>
    {
        Blaze::ExternalXblAccountId operator()(const Blaze::Association::ListMemberByExternalXblIdNode &x) const
        {
            return static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId().getUser().getPlatformInfo().getExternalIds().getXblAccountId();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByExternalSteamIdNode, Blaze::ExternalSteamAccountId>
    {
        Blaze::ExternalSteamAccountId operator()(const Blaze::Association::ListMemberByExternalSteamIdNode &x) const
        {
            return static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId().getUser().getPlatformInfo().getExternalIds().getSteamAccountId();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByExternalStadiaIdNode, Blaze::ExternalStadiaAccountId>
    {
        Blaze::ExternalStadiaAccountId operator()(const Blaze::Association::ListMemberByExternalStadiaIdNode &x) const
        {
            return static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId().getUser().getPlatformInfo().getExternalIds().getStadiaAccountId();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByExternalSwitchIdNode, Blaze::ExternalSwitchId>
    {
        Blaze::ExternalSwitchId operator()(const Blaze::Association::ListMemberByExternalSwitchIdNode &x) const
        {
            return static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId().getUser().getPlatformInfo().getExternalIds().getSwitchIdAsTdfString();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByAccountIdNode, Blaze::AccountId>
    {
        Blaze::AccountId operator()(const Blaze::Association::ListMemberByAccountIdNode &x) const
        {
            return static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId().getUser().getPlatformInfo().getEaIds().getNucleusAccountId();
        }
    };

    template <>
    struct use_intrusive_key<Blaze::Association::ListMemberByPersonaNameAndNamespaceNode, Blaze::Association::ListMemberId>
    {
        const Blaze::Association::ListMemberId& operator()(const Blaze::Association::ListMemberByPersonaNameAndNamespaceNode &x) const
        {             
            return (static_cast<const Blaze::Association::AssociationListMember &>(x).getListMemberId());
        }
    };

}

#endif //  BLAZE_ASSOCIATIONLISTS_ASSOCIATION_LIST_MEMBER
