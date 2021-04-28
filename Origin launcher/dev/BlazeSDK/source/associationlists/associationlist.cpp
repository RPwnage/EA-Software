/*************************************************************************************************/
/*!
    \file associationlist.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/associationlists/associationlist.h"

#include "BlazeSDK/associationlists/associationlistapi.h"
#include "BlazeSDK/associationlists/associationlistmember.h"
#include "BlazeSDK/component/associationlistscomponent.h"

#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

#include "BlazeSDK/blazesdkdefs.h"

#include "EAStdC/EAHashCRC.h"

namespace Blaze
{
namespace Association
{
    
AssociationList::AssociationList(AssociationListAPI *api, const ListIdentification& listId, MemoryGroupId memGroupId)
    : mAPI(api),
    mIsLocalOnly(true),
    mPageOffset(0),
    mMemberTotalCount(0),
    mMemberVector(MEM_GROUP_ASSOCIATIONLIST, "AssociationListMemberVector"),
    mAListMemberPool(memGroupId),
    mMemGroup(memGroupId),
    mInitialSetComplete(false)
{
    listId.copyInto(ListInfo::getId());

    // TODO: Remove the mInitialSetComplete value once the non-mutual association list fix is in. 

    // Mutual lists never need to send an initial set
    mInitialSetComplete = ListInfo::getStatusFlags().getMutualAction();

    // If the server disables the initial set, then it's not needed
    int32_t skipInitialSet = 0;
    if (!mInitialSetComplete && mAPI->getBlazeHub()->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_ASSOCIATIONLIST_SKIP_INITIAL_SET, &skipInitialSet))
    {
        mInitialSetComplete = (skipInitialSet != 0);
    }



    if (listId.getListType() != 0)
    {
        //we can do the object id too
        Blaze::UserManager::LocalUser *user = mAPI->getBlazeHub()->getUserManager()->getLocalUser(mAPI->getUserIndex());
        if (user != nullptr)
        {
            ListInfo::setBlazeObjId(EA::TDF::ObjectId(ASSOCIATIONLISTSCOMPONENT_COMPONENT_ID, listId.getListType(), user->getId()));
        }
    }
}

AssociationList::~AssociationList()
{
    //  cancel all jobs associated with this association list
    mAPI->getBlazeHub()->getScheduler()->removeByAssociatedObject(this);

    clearLocalList();
}

void AssociationList::initFromServer(const ListMembers &listMembers)
{
    clearLocalList();

    mIsLocalOnly = false;

    //bounce us out of the indices
    mAPI->removeListFromIndices(*this);

    listMembers.getInfo().copyInto(*this);
    addMembers(listMembers.getListMemberInfoVector());

    //put us back in the indices
    mAPI->addListToIndices(*this);

    mPageOffset = listMembers.getOffset();
    if (listMembers.getTotalCount() > 0)
        mMemberTotalCount = listMembers.getTotalCount();
    else
        mMemberTotalCount = static_cast<uint32_t>(listMembers.getListMemberInfoVector().size());
}

void AssociationList::addMembers(const ListMemberInfoVector &listMembers)
{
    ListMemberInfoVector::const_iterator it = listMembers.begin();
    ListMemberInfoVector::const_iterator itend = listMembers.end();
    for ( ; it != itend; it++)
    {
        const ListMemberInfo *memberInfo = *it;
        addMember(*memberInfo);
    }
}

AssociationListMember* AssociationList::addMember(const ListMemberInfo& listMemberInfo)
{
    AssociationListMember* listMember =  new (mAListMemberPool.alloc()) AssociationListMember(*this, listMemberInfo);
    addMemberToIndicies(*listMember);
    return listMember;
}

void AssociationList::removeMembers(const ListMemberIdVector &listMembers, bool triggerDispatch)
{
    ListMemberIdVector::const_iterator it = listMembers.begin();
    ListMemberIdVector::const_iterator itend = listMembers.end();
    for ( ; it != itend; it++)
    {
        const ListMemberId *listMemberId = *it;
        AssociationListMember *listMember = getMemberByMemberId(*listMemberId);
        if (listMember != nullptr)
            removeMember(listMember);
    }

    if (!listMembers.empty() && triggerDispatch)
    {
        mAPI->getDispatcher().dispatch("onMembersRemoved", &AssociationListAPIListener::onMembersRemoved, this);
    }
}

void AssociationList::removeMember(AssociationListMember* listMember)
{
    removeMemberFromIndicies(*listMember);
    mAListMemberPool.free(listMember);
}


void AssociationList::processUpdateListMembersResponse(const UpdateListMembersResponse *response)
{
    bool triggerRemovedDispatch = false;
    ListMemberIdVector::const_iterator it = response->getRemovedListMemberIdVector().begin();
    ListMemberIdVector::const_iterator itend = response->getRemovedListMemberIdVector().end();
    for ( ; it != itend; it++)
    {
        const ListMemberId *memberId = *it;

        ListMemberInfoVector::const_iterator infoIt = response->getListMemberInfoVector().begin();
        ListMemberInfoVector::const_iterator infoItend = response->getListMemberInfoVector().end();
        for ( ; infoIt != infoItend; infoIt++)
        {
            const ListMemberInfo *memberInfo = *infoIt;
            if (memberId->getUser().getBlazeId() == memberInfo->getListMemberId().getUser().getBlazeId())
            {
                break;
            }
        }

        if (infoIt == infoItend)
        {
            triggerRemovedDispatch = true;
            break;
        }
    }

    removeMembers(response->getRemovedListMemberIdVector(), triggerRemovedDispatch);
    addMembers(response->getListMemberInfoVector());
}

uint32_t AssociationList::removeDuplicateMembers(const ListMemberInfoVector &listMemberInfoVector)
{
    uint32_t updateOnlyCount = 0;
    ListMemberInfoVector::const_iterator infoIt = listMemberInfoVector.begin();
    ListMemberInfoVector::const_iterator infoItend = listMemberInfoVector.end();
    for ( ; infoIt != infoItend; infoIt++)
    {
        const ListMemberInfo *memberInfo = *infoIt;
        // Remove List Member if duplicate exists. This avoids duplicate values.
        AssociationListMember *listMember = getMemberByMemberId(memberInfo->getListMemberId());
        if (listMember != nullptr)
        {
            updateOnlyCount++;
            removeMember(listMember);
        }
    }
    return updateOnlyCount;
}

JobId AssociationList::fetchListMembers(const FetchMembersCb &resultFunctor, const uint32_t maxResultCount /* = UINT32_MAX */, const uint32_t offset /* = 0 */)
{
    GetListsRequest request;

    Association::ListInfo *listInfo = request.getListInfoVector().pull_back();
    copyInto(*listInfo);
    request.setMaxResultCount(maxResultCount);
    request.setOffset(offset);

    JobId jobId = mAPI->getAssociationListComponent().getLists(request, MakeFunctor(this, &AssociationList::fetchListMembersCb), resultFunctor);
    Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);

    return jobId;
}

void AssociationList::fetchListMembersCb(const Lists *response, BlazeError err, JobId jobid, const FetchMembersCb resultCb)
{
    if (err == ERR_OK && !response->getListMembersVector().empty())
    {
        const ListMembers *listMembers = response->getListMembersVector().front();
        initFromServer(*listMembers);                

        waitForFetchedUsers(err, jobid, resultCb);
    }
    else
    {
        resultCb(this, err, jobid);
    }


}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void AssociationList::waitForFetchedUsers(BlazeError err, JobId jobid, const FetchMembersCb resultCb)
{      
    for (AssociationList::MemberVector::const_iterator it = mMemberVector.begin(), itend = mMemberVector.end(); it != itend; it++)
    {
        const AssociationListMember *listMember = *it;
        if ((listMember->getListMemberId().getUser().getBlazeId() != INVALID_BLAZE_ID) && (*(listMember->getListMemberId().getUser().getName()) != '\0') && (listMember->getUser() == nullptr))
        {
            JobId jobId = mAPI->getBlazeHub()->getScheduler()->scheduleFunctor("waitForFetchedUsersCb", MakeFunctor(this, &AssociationList::waitForFetchedUsers), err, jobid, resultCb);
            Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultCb);
            return;
        }
    }

    resultCb(this, err, jobid);
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/



JobId AssociationList::addUsersToList(const ListMemberIdVector &listMemberIdVector, const AddUsersToListCb &resultFunctor, const ListMemberAttributes &mask /*= ListMemberAttributes()*/)
{
    UpdateListMembersRequest request;

    updateListMembersCommon(request, listMemberIdVector);
    request.getAttributesMask().setBits(mask.getBits());

    JobId jobId = mAPI->getAssociationListComponent().addUsersToList(request, MakeFunctor(this, &AssociationList::updateListMembersCb), resultFunctor);
    Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);
    
    return jobId;
}

JobId AssociationList::setUsersAttributesInList(const ListMemberIdVector &listMemberIdVector, const SetUsersAttributesInListCb &resultFunctor, const ListMemberAttributes &mask)
{
    UpdateListMembersRequest request;
    updateListMembersCommon(request, listMemberIdVector);

    request.getAttributesMask().setBits(mask.getBits());

    JobId jobId = mAPI->getAssociationListComponent().setUsersAttributesInList(request, MakeFunctor(this, &AssociationList::updateListMembersCb), resultFunctor);
    Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);
    
    return jobId;
}

void AssociationList::updateListMembersCommon(UpdateListMembersRequest& request, const ListMemberIdVector& listMemberIdVector) const
{
    mId.copyInto(request.getListIdentification());

    ListMemberIdVector::const_iterator it = listMemberIdVector.begin();
    ListMemberIdVector::const_iterator itend = listMemberIdVector.end();
    for ( ; it != itend; ++it)
    {
        Association::ListMemberId *listMemberId = request.getListMemberIdVector().pull_back();
        (*it)->copyInto(*listMemberId);
    }
}

JobId AssociationList::setUsersToList(const ListMemberIdVector &listMemberIdVector, const AddUsersToListCb &resultFunctor, 
    bool validateUsers /*= true*/, bool ignoreHash /*= false*/, const ListMemberAttributes &mask /*= ListMemberAttributes()*/)
{
    MemberHash hash = Association::INVALID_MEMBER_HASH;

    if (!listMemberIdVector.empty())
    {
        hash = EA::StdC::kCRC32InitialValue;
        for (ListMemberIdVector::const_iterator itr = listMemberIdVector.begin(), end = listMemberIdVector.end(); itr != end; )
        {
            const ListMemberId& memberId = (**itr++);
            bool finalize = itr !=end;
            if (memberId.getUser().getBlazeId() != INVALID_BLAZE_ID)
            {
                BlazeId id = memberId.getUser().getBlazeId();
                hash = EA::StdC::CRC32(&id, sizeof(BlazeId), hash, finalize);
            }

            // Since we're not adding crossplay support to AssociationLists, we just hash the 1st-party id for the caller's platform 
#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
            if (memberId.getUser().getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
            {
                ExternalId id = memberId.getUser().getPlatformInfo().getExternalIds().getXblAccountId();
                hash = EA::StdC::CRC32(&id, sizeof(ExternalId), hash, finalize);
            }
#elif defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
            if (memberId.getUser().getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
            {
                ExternalId id = memberId.getUser().getPlatformInfo().getExternalIds().getPsnAccountId();
                hash = EA::StdC::CRC32(&id, sizeof(ExternalId), hash, finalize);
            }
#elif defined(EA_PLATFORM_NX)
            if (!memberId.getUser().getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
            {
                hash = EA::StdC::CRC32(memberId.getUser().getPlatformInfo().getExternalIds().getSwitchId(), memberId.getUser().getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().length(), hash, finalize);
            }
#elif defined(EA_PLATFORM_STADIA)
            if (memberId.getUser().getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
            {
                ExternalId id = memberId.getUser().getPlatformInfo().getExternalIds().getStadiaAccountId();
                hash = EA::StdC::CRC32(&id, sizeof(ExternalId), hash, finalize);
            }
#elif defined(EA_PLATFORM_WINDOWS)
            if (memberId.getUser().getPlatformInfo().getClientPlatform() == ClientPlatformType::steam
                && memberId.getUser().getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
            {
                ExternalId id = memberId.getUser().getPlatformInfo().getExternalIds().getSteamAccountId();
                hash = EA::StdC::CRC32(&id, sizeof(ExternalId), hash, finalize);
            }
#endif

            if (memberId.getUser().getName() != nullptr && *(memberId.getUser().getName()) != '\0')
            {
                hash = EA::StdC::CRC32(memberId.getUser().getName(), strlen(memberId.getUser().getName()), hash, finalize);
            }
            
            uint32_t attrBits = memberId.getAttributes().getBits();
            hash = EA::StdC::CRC32(&attrBits, sizeof(attrBits), hash, finalize);
        }
    }


    UpdateListMembersRequestPtr memberRequest = BLAZE_NEW(mMemGroup, "TemporaryUpdateListMembersRequest") UpdateListMembersRequest();
    updateListMembersCommon(*memberRequest, listMemberIdVector);
    memberRequest->setMemberHash(hash);
    memberRequest->getAttributesMask().setBits(mask.getBits());

    //If this isn't our first time updating, or we haven't been told to explicitly ignore the hash, first try checking the hash
    //against what's on the server.  If identical, we don't bother updating.
    JobId jobId = INVALID_JOB_ID;
    if (mInitialSetComplete && !ignoreHash)
    {
        GetMemberHashRequest request;
        mId.copyInto(request.getListIdentification());

        jobId = mAPI->getAssociationListComponent().getMemberHash(request, MakeFunctor(this, &AssociationList::checkMemberHashCb), memberRequest, resultFunctor);
        Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);
    }
    else
    {
        jobId = mAPI->getAssociationListComponent().setUsersToList(*memberRequest, MakeFunctor(this, &AssociationList::updateListMembersCb), resultFunctor);
        Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);    
    }

    return jobId;
}

void AssociationList::checkMemberHashCb(const Blaze::Association::GetMemberHashResponse* response, BlazeError err, JobId jobid, UpdateListMembersRequestPtr memberRequest, const AddUsersToListCb resultCb)
{
    if (err == ERR_OK && (response->getMemberHash() == INVALID_MEMBER_HASH || response->getMemberHash() != memberRequest->getMemberHash()))
    {
        mAPI->getAssociationListComponent().setUsersToList(*memberRequest, MakeFunctor(this, &AssociationList::updateListMembersCb), resultCb, jobid);
        Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobid, resultCb);
    }
    else
    {
        resultCb(this, nullptr, err, jobid);
    }
}

void AssociationList::updateListMembersCb(const UpdateListMembersResponse *response, BlazeError err, JobId jobid, const AddUsersToListCb resultCb)
{
    if (err == ERR_OK)
    {
        mInitialSetComplete = true;
        uint32_t updateOnlyCount = removeDuplicateMembers(response->getListMemberInfoVector());
        processUpdateListMembersResponse(response);
        mMemberTotalCount += static_cast<uint32_t>(response->getListMemberInfoVector().size() - updateOnlyCount - response->getRemovedListMemberIdVector().size());
        UpdateListMembersResponse *cachedResponse = BLAZE_NEW(mMemGroup, "TemporaryUpdateListMembersResponse") UpdateListMembersResponse;
        response->copyInto(*cachedResponse);
        waitForUsers(cachedResponse, err, jobid, resultCb);
    }
    else
    {
        resultCb(this, &response->getListMemberInfoVector(), err, jobid);
    }
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void AssociationList::waitForUsers(UpdateListMembersResponsePtr response, BlazeError err, JobId jobid, const AddUsersToListCb resultCb)
{
    MemberVector::iterator it = mMemberVector.begin();
    MemberVector::iterator itend = mMemberVector.end();
    for ( ; it != itend; it++)
    {
        const AssociationListMember *listMember = *it;
        if ((listMember->getListMemberId().getUser().getBlazeId() != INVALID_BLAZE_ID) && (*(listMember->getListMemberId().getUser().getName()) != '\0') && (listMember->getUser() == nullptr))
        {
            JobId jobId = mAPI->getBlazeHub()->getScheduler()->scheduleFunctor("waitForUsersCb",
                MakeFunctor(this, &AssociationList::waitForUsers),
                response, err, jobid, resultCb);
            Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultCb);
            return;
        }
    }

    resultCb(this, &response->getListMemberInfoVector(), err, jobid);

}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

JobId AssociationList::removeUsersFromList(const ListMemberIdVector &listMemberIdVector, const RemoveUsersFromListCb &resultFunctor, bool validateUsers /*= true*/)
{
    UpdateListMembersRequest request;

    request.setValidateDelete(validateUsers);

    updateListMembersCommon(request, listMemberIdVector);

    JobId jobId = mAPI->getAssociationListComponent().removeUsersFromList(request, MakeFunctor(this, &AssociationList::removeUsersFromListCb), resultFunctor);
    Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);

    return jobId;
}


void AssociationList::removeUsersFromListCb(const UpdateListMembersResponse *response, BlazeError err, JobId jobid, const RemoveUsersFromListCb resultCb)
{
    if (err == ERR_OK)
    {
        processUpdateListMembersResponse(response);
        mMemberTotalCount -= static_cast<uint32_t>(response->getRemovedListMemberIdVector().size());
    }

    resultCb(this, &response->getRemovedListMemberIdVector(), err, jobid);
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

JobId AssociationList::clearList(const ClearListCb& resultFunctor)
{
    UpdateListsRequest request;

    ListIdentification *listIdent = request.getListIdentificationVector().pull_back();
    mId.copyInto(*listIdent);

    JobId jobId = mAPI->getAssociationListComponent().clearLists(request, MakeFunctor(this, &AssociationList::clearListCb), resultFunctor);
    Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);
    return jobId;
}


void AssociationList::clearListCb(BlazeError err, JobId jobid, const ClearListCb resultCb)
{
    if (err == ERR_OK)
    {
        mAPI->getDispatcher().dispatch<AssociationList *>("onMembersRemoved", &AssociationListAPIListener::onMembersRemoved, this);

        clearLocalList();
    }

    resultCb(err, jobid);
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

JobId AssociationList::subscribeToList(const SubscribeToListCb& resultFunctor)
{
    UpdateListsRequest request;

    request.getListIdentificationVector().push_back(&mId);

    AssociationListsComponent& assocListComponent = mAPI->getAssociationListComponent();

    JobId jobId = assocListComponent.subscribeToLists(request, MakeFunctor(this, &AssociationList::subscribeToListCb), resultFunctor);
    Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);

    request.getListIdentificationVector().clear();

    return jobId;
}

void AssociationList::subscribeToListCb(BlazeError err, JobId jobid, const SubscribeToListCb resultCb)
{
    if (err == ERR_OK)
    {
        mStatusFlags.setSubscribed();
    }

    resultCb(err, jobid);
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

JobId AssociationList::unsubscribeFromList(const UnsubscribeFromListCb& resultFunctor)
{
    UpdateListsRequest request;

    request.getListIdentificationVector().push_back(&mId);

    AssociationListsComponent& assocListComponent = mAPI->getAssociationListComponent();

    JobId jobId = assocListComponent.unsubscribeFromLists(request, MakeFunctor(this, &AssociationList::unsubscribeFromListCb), resultFunctor);
    Job::addTitleCbAssociatedObject(mAPI->getBlazeHub()->getScheduler(), jobId, resultFunctor);

    request.getListIdentificationVector().clear();

    return jobId;
}

void AssociationList::unsubscribeFromListCb(BlazeError err, JobId jobid, const UnsubscribeFromListCb resultCb)
{
    if (err == ERR_OK)
    {
        mStatusFlags.clearSubscribed();
    }

    resultCb(err, jobid);
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void AssociationList::addMemberToIndicies(AssociationListMember &listMember)
{
    mMemberVector.push_back(&listMember);
    if (listMember.getListMemberId().getUser().getBlazeId() != INVALID_BLAZE_ID)
    {
        mMemberBlazeIdMap.insert(listMember);
    }

    if (listMember.getListMemberId().getUser().getName()[0] != '\0')
    {
        mMemberPersonaNameAndNamespaceMap.insert(listMember);
    }

    if (listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID)
    {
        mMemberExternalPsnIdMap.insert(listMember);
    }
    if (listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID)
    {
        mMemberExternalXblIdMap.insert(listMember);
    }
    if (listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID)
    {
        mMemberExternalSteamIdMap.insert(listMember);
    }
    if (listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID)
    {
        mMemberExternalStadiaIdMap.insert(listMember);
    }
    if (!listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty())
    {
        mMemberExternalSwitchIdMap.insert(listMember);
    }
    if (listMember.getListMemberId().getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
    {
        mMemberAccountIdMap.insert(listMember);
    }
}

void AssociationList::removeMemberFromIndicies(const AssociationListMember &listMember)
{
    if (listMember.getListMemberId().getUser().getName()[0] != '\0')
    {
        mMemberPersonaNameAndNamespaceMap.erase(listMember.getListMemberId());
    }

    if (listMember.getListMemberId().getUser().getBlazeId() != INVALID_BLAZE_ID)
    {
        mMemberBlazeIdMap.erase(listMember.getListMemberId().getUser().getBlazeId());
    }

    ExternalPsnAccountId extPsnId = listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getPsnAccountId();
    if (extPsnId != INVALID_PSN_ACCOUNT_ID)
    {
        mMemberExternalPsnIdMap.erase(extPsnId);
    }
    ExternalXblAccountId extXblId = listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getXblAccountId();
    if (extXblId != INVALID_XBL_ACCOUNT_ID)
    {
        mMemberExternalXblIdMap.erase(extXblId);
    }
    ExternalSteamAccountId extSteamId = listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getSteamAccountId();
    if (extSteamId != INVALID_STEAM_ACCOUNT_ID)
    {
        mMemberExternalSteamIdMap.erase(extSteamId);
    }
    ExternalStadiaAccountId extStadiaId = listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getStadiaAccountId();
    if (extStadiaId != INVALID_STADIA_ACCOUNT_ID)
    {
        mMemberExternalStadiaIdMap.erase(extStadiaId);
    }
    ExternalSwitchId extSwitchId = listMember.getListMemberId().getUser().getPlatformInfo().getExternalIds().getSwitchIdAsTdfString();
    if (!extSwitchId.empty())
    {
        mMemberExternalSwitchIdMap.erase(extSwitchId);
    }
    AccountId accountId = listMember.getListMemberId().getUser().getPlatformInfo().getEaIds().getNucleusAccountId();
    if (accountId != INVALID_ACCOUNT_ID)
    {
        mMemberAccountIdMap.erase(accountId);
    }

    MemberVector::iterator it = mMemberVector.begin();
    MemberVector::iterator itend = mMemberVector.end();
    for ( ; it != itend; ++it)
    {
        if (&listMember == *it)
        {
            mMemberVector.erase(it);
            break;
        }
    }
}

void AssociationList::clearLocalList()
{
    while (!mMemberVector.empty())
    {
        const AssociationListMember *listMember = mMemberVector.front();
        removeMemberFromIndicies(*listMember);

        mAListMemberPool.free(const_cast<AssociationListMember *>(listMember));
    }

    mMemberTotalCount = 0;
}

AssociationListMember *AssociationList::getMemberByMemberId(const ListMemberId &memberId) const
{
    AssociationListMember *listMember = nullptr;

    if (memberId.getUser().getBlazeId() != INVALID_BLAZE_ID)
    {
        listMember = getMemberByBlazeId(memberId.getUser().getBlazeId());
    }
    else if (memberId.getUser().getName()[0] != '\0' && memberId.getUser().getPersonaNamespace()[0] != '\0')
    {
        listMember = getMemberByPersonaName(memberId.getUser().getPersonaNamespace(), memberId.getUser().getName());
    }
    else
    {
        listMember = getMemberByPlatformInfo(memberId.getUser().getPlatformInfo());
    }

    return listMember;
}

AssociationListMember *AssociationList::getMemberByBlazeId(BlazeId id) const
{
    MemberBlazeIdMap::const_iterator it = mMemberBlazeIdMap.find(id);
    if (it != mMemberBlazeIdMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }

    return nullptr;
}

AssociationListMember *AssociationList::getMemberByNativeExternalId(ExternalId id) const
{
    return getMemberByExternalId(id);
}

AssociationListMember *AssociationList::getMemberByExternalId(ExternalId id) const
{
    switch (mAPI->getBlazeHub()->getClientPlatformType())
    {
    case ps4:
    case ps5:
        return getMemberByPsnAccountId((ExternalPsnAccountId)id);
    case xone:
    case xbsx:
        return getMemberByXblAccountId((ExternalXblAccountId)id);
    case steam:
        return getMemberBySteamAccountId((ExternalSteamAccountId)id);
    case stadia:
        return getMemberByStadiaAccountId((ExternalStadiaAccountId)id);
    case pc:
        return getMemberByNucleusAccountId((AccountId)id);
    default:
        break;
    }
    return nullptr;
}

AssociationListMember *AssociationList::getMemberByPlatformInfo(const PlatformInfo& platformInfo) const
{
    AssociationListMember * outMember = nullptr;
    if (outMember == nullptr) outMember = getMemberByXblAccountId(platformInfo.getExternalIds().getXblAccountId());
    if (outMember == nullptr) outMember = getMemberByPsnAccountId(platformInfo.getExternalIds().getPsnAccountId());
    if (outMember == nullptr) outMember = getMemberBySwitchId(platformInfo.getExternalIds().getSwitchId());
    if (outMember == nullptr) outMember = getMemberByNucleusAccountId(platformInfo.getEaIds().getNucleusAccountId());

    return outMember;
}

AssociationListMember *AssociationList::getMemberByPsnAccountId(ExternalPsnAccountId psnId) const
{
    if (psnId == INVALID_PSN_ACCOUNT_ID)
        return nullptr;

    MemberExternalPsnIdMap::const_iterator it = mMemberExternalPsnIdMap.find(psnId);
    if (it != mMemberExternalPsnIdMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }
    return nullptr;
}
Blaze::Association::AssociationListMember * AssociationList::getMemberByXblAccountId(ExternalXblAccountId xblId) const
{
    if (xblId == INVALID_XBL_ACCOUNT_ID)
        return nullptr;

    MemberExternalXblIdMap::const_iterator it = mMemberExternalXblIdMap.find(xblId);
    if (it != mMemberExternalXblIdMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }
    return nullptr;
}
Blaze::Association::AssociationListMember * AssociationList::getMemberBySteamAccountId(ExternalSteamAccountId steamId) const
{
    if (steamId == INVALID_STEAM_ACCOUNT_ID)
        return nullptr;

    MemberExternalSteamIdMap::const_iterator it = mMemberExternalSteamIdMap.find(steamId);
    if (it != mMemberExternalSteamIdMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }
    return nullptr;
}
Blaze::Association::AssociationListMember * AssociationList::getMemberByStadiaAccountId(ExternalStadiaAccountId stadiaId) const
{
    if (stadiaId == INVALID_STADIA_ACCOUNT_ID)
        return nullptr;

    MemberExternalStadiaIdMap::const_iterator it = mMemberExternalStadiaIdMap.find(stadiaId);
    if (it != mMemberExternalStadiaIdMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }
    return nullptr;
}

AssociationListMember *AssociationList::getMemberBySwitchId(const char8_t* switchId) const
{
    if (switchId == nullptr || switchId[0] == '\0')
        return nullptr;

    MemberExternalSwitchIdMap::const_iterator it = mMemberExternalSwitchIdMap.find(switchId);
    if (it != mMemberExternalSwitchIdMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }
    return nullptr;
}
AssociationListMember *AssociationList::getMemberByNucleusAccountId(AccountId accountId) const
{
    if (accountId == INVALID_ACCOUNT_ID)
        return nullptr;

    MemberAccountIdMap::const_iterator it = mMemberAccountIdMap.find(accountId);
    if (it != mMemberAccountIdMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }
    return nullptr;
}


AssociationListMember *AssociationList::getMemberByPersonaName(const char8_t *personaNamespace, const char8_t *persona) const
{
    Blaze::Association::ListMemberId listMemId;
    listMemId.getUser().setName(persona);
    listMemId.getUser().setPersonaNamespace(personaNamespace);
    MemberPersonaNameMap::const_iterator it = mMemberPersonaNameAndNamespaceMap.find(listMemId);
    if (it != mMemberPersonaNameAndNamespaceMap.end())
    {
        return (AssociationListMember *)(&(*it));
    }

    return nullptr;
}

const AssociationList::MemberVector &AssociationList::getMemberVector() const
{
    return mMemberVector;
}

uint32_t AssociationList::getLocalMemberCount() const
{
    return static_cast<uint32_t>(mMemberVector.size());
}

uint32_t AssociationList::getMemberCount() const
{
    return mMemberTotalCount;
}

bool AssociationList::hasCompleteCache() const
{
    if (getLocalMemberCount() < getMemberCount())
        return false;
    else
        return true;
}

uint32_t AssociationList::getPageOffset() const
{
    return mPageOffset;
}

void AssociationList::onListUpdated(const UpdateListMembersResponse& notification)
{
    for (ListMemberIdVector::const_iterator itr = notification.getRemovedListMemberIdVector().begin(), end =  notification.getRemovedListMemberIdVector().end(); itr != end; ++itr)
    {
        AssociationListMember *member = getMemberByBlazeId((*itr)->getUser().getBlazeId());
        if (member != nullptr)
        {
            removeMemberFromIndicies(*member);
            --mMemberTotalCount;
            mAPI->getDispatcher().dispatch<AssociationListMember&,AssociationList&>("onMemberRemoved", &AssociationListAPIListener::onMemberRemoved, *member, *this);
            mAListMemberPool.free(member);
        }       
    }

    for (ListMemberInfoVector::const_iterator itr =  notification.getListMemberInfoVector().begin(), end =  notification.getListMemberInfoVector().end(); itr != end; ++itr)
    {
        AssociationListMember *member = getMemberByBlazeId((*itr)->getListMemberId().getUser().getBlazeId());
        if (member == nullptr)
        {
            member = addMember(**itr);
            if (member != nullptr)
            {
                ++mMemberTotalCount;
                mAPI->getDispatcher().dispatch<AssociationListMember&,AssociationList&>("onMemberAdded", &AssociationListAPIListener::onMemberAdded, *member, *this);
            }
        }
    }    
}

}
}
