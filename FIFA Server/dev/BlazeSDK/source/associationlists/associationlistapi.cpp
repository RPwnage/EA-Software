/*************************************************************************************************/
/*!
    \file associationlistapi.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "framework/util/shared/collections.h"

#include "BlazeSDK/associationlists/associationlistapi.h"
#include "BlazeSDK/component/associationlistscomponent.h"

#include "BlazeSDK/component/associationlists/tdf/associationlists.h"

namespace Blaze
{
namespace Association
{

void AssociationListAPI::createAPI(BlazeHub &hub, const AssociationListApiParams& params, EA::Allocator::ICoreAllocator* allocator)
{
    if (hub.getAssociationListAPI(0) != nullptr)
    {
        BLAZE_SDK_DEBUGF("[AssociationListAPI] Warning: Ignoring attempt to recreate API\n");
        return;
    }

    if (Blaze::Allocator::getAllocator(MEM_GROUP_ASSOCIATIONLIST) == nullptr)
        Blaze::Allocator::setAllocator(MEM_GROUP_ASSOCIATIONLIST, allocator!=nullptr? allocator : Blaze::Allocator::getAllocator());
    Association::AssociationListsComponent::createComponent(&hub);

    APIPtrVector *apiVector = BLAZE_NEW(MEM_GROUP_ASSOCIATIONLIST, "AssocationListAPIArray")
        APIPtrVector(MEM_GROUP_ASSOCIATIONLIST, hub.getNumUsers(), MEM_NAME(MEM_GROUP_ASSOCIATIONLIST, "AssocationListAPIArray")); 
    // create a separate instance for each userIndex
    for(uint32_t i=0; i < hub.getNumUsers(); i++)
    {
        AssociationListAPI *api = BLAZE_NEW(MEM_GROUP_ASSOCIATIONLIST, "AssocationListAPI") 
            AssociationListAPI(hub, params, i, MEM_GROUP_ASSOCIATIONLIST);
        (*apiVector)[i] = api; // Note: the Blaze framework will delete the API(s)
    }

    hub.createAPI(ASSOCIATIONLIST_API, apiVector);
}

AssociationListAPI::AssociationListAPI(BlazeHub &blazeHub, const AssociationListApiParams &params, uint32_t userIndex, MemoryGroupId memGroupId)
    : MultiAPI(blazeHub, userIndex),
      mComponent(*blazeHub.getComponentManager(userIndex)->getAssociationListsComponent()),
      mApiParams(params),
      mAssociationListList(MEM_GROUP_ASSOCIATIONLIST, "AssociationListAPIAssociationListList" ),
      mAListMemPool(memGroupId),
      mMemGroup(memGroupId)
{
    // create all memory pool objs we need for the component
    mAListMemPool.reserve(mApiParams.mMaxLocalAssociationLists, "ALAPI::AssociationListPool");

    getBlazeHub()->addUserGroupProvider(AssociationListsComponent::ASSOCIATIONLISTSCOMPONENT_COMPONENT_ID, this);

    setNotificationHandlers();

    getBlazeHub()->getUserManager()->addListener(this);
}


AssociationListAPI::~AssociationListAPI()
{
    getBlazeHub()->getUserManager()->removeListener(this);

    getBlazeHub()->getScheduler()->removeByAssociatedObject(this);

    releaseAssociationLists();

    clearNotificationHandlers();

    getBlazeHub()->removeUserGroupProvider(AssociationListsComponent::ASSOCIATIONLISTSCOMPONENT_COMPONENT_ID, this);
}

///////////////////////////////////////////////////////////////////////////////
void AssociationListAPI::setNotificationHandlers()
{
    // Setup notification handlers.
    mComponent.setNotifyUpdateListMembershipHandler(AssociationListsComponent::NotifyUpdateListMembershipCb(this, &AssociationListAPI::onNotifyUpdateListMembership));
}

void AssociationListAPI::clearNotificationHandlers()
{
    // Remove notification handlers.
    mComponent.clearNotifyUpdateListMembershipHandler();
}

void AssociationListAPI::releaseAssociationLists()
{
    while (!mAssociationListList.empty())
    {
        AssociationList *list = mAssociationListList.front();

        mBlazeObjectIdMap.erase(list->getBlazeObjectId());
        mListNameMap.erase(list->getId().getListName());
        mListTypeMap.erase(list->getId().getListType());
        mAssociationListList.remove(list);

        mDispatcher.dispatch("onListRemoved", &AssociationListAPIListener::onListRemoved, list);

        mAListMemPool.free(list);
    }
}


void AssociationListAPI::onDeAuthenticated(uint32_t userIndex)
{
    if (userIndex == getUserIndex())
    {
        BLAZE_SDK_DEBUGF("[AssociationListAPI:%p].onDeAuthenticated : User index %u lost authentication\n", this, userIndex);
        releaseAssociationLists();
    }
}

void AssociationListAPI::onNotifyUpdateListMembership(const Blaze::Association::UpdateListMembersResponse* notification, uint32_t userIndex)
{
    AssociationList* list = getListByType(notification->getListType());
    if (list == nullptr)
    {
        BLAZE_SDK_DEBUGF("[AssociationListAPI].onNotifyUpdateListMembership : notification for user's list is not loaded into memory (listType=%d)\n", notification->getListType());
        return;
    }
    list->onListUpdated(*notification);

}  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

////////////////////////////////////////////////////////////////////////////////////////

AssociationList *AssociationListAPI::getUserGroupById(const EA::TDF::ObjectId &bobjId) const
{
    if (bobjId.type.component == AssociationListsComponent::ASSOCIATIONLISTSCOMPONENT_COMPONENT_ID)
    {
        return getListByObjId(bobjId);
    }
    return nullptr;
}


AssociationList* AssociationListAPI::createLocalList(const ListIdentification& listId)
{
    //If a list already exists, we use that, we don't create another one.
    AssociationList* list = nullptr;
   if (listId.getListType() != 0)
   {
       list = getListByType(listId.getListType());
   }
   else if (listId.getListName() != nullptr && *listId.getListName() != '\0')
   {
       list = getListByName(listId.getListName());
   }
   else
   {
       //bad list id
       return nullptr;
   }

   if (list == nullptr)
   {
        list = new (mAListMemPool.alloc()) AssociationList(this, listId);
        mAssociationListList.push_back(list);
        addListToIndices(*list);      
   }

   return list;
}

void AssociationListAPI::addListToIndices(AssociationList& list)
{
    if (list.getListType() != 0)
    {
        mListTypeMap.insert(list);
    }

    if (list.getBlazeObjectId() != EA::TDF::OBJECT_ID_INVALID)            
    {
        mBlazeObjectIdMap.insert(list);
    }

    if (list.getListName() != nullptr && *list.getListName() != '\0')
    {
        mListNameMap.insert(list);
    }        
}

void AssociationListAPI::removeListFromIndices(AssociationList& list)
{
    if (list.getListType() != 0)
    {
        mListTypeMap.erase(list.getListType());
    }

    if (list.getBlazeObjectId() != EA::TDF::OBJECT_ID_INVALID)            
    {
        mBlazeObjectIdMap.erase(list.getBlazeObjectId());
    }

    if (list.getListName() != nullptr && *list.getListName() != '\0')
    {
        mListNameMap.erase(list.getListName());
    }        
}


///////////////////////////////////////////////////////////////////////////////
//  Create AssociationList RPC -> Response -> execute result functor to caller
JobId AssociationListAPI::getLists(const ListInfoVector &listInfoVector, const GetListsCb &resultFunctor, const uint32_t maxResultCount/* = UINT32_MAX*/, const uint32_t offset/* = 0*/)
{
    GetListsRequest request;

    // Reserve space to prevent mem leak when doing the copyInto on the pull'd element.
    request.getListInfoVector().reserve(listInfoVector.size());
    ListInfoVector::const_iterator it = listInfoVector.begin();
    ListInfoVector::const_iterator itend = listInfoVector.end();
    for ( ; it != itend; ++it)
    {
        Association::ListInfo *listInfo = request.getListInfoVector().pull_back();
        (*it)->copyInto(*listInfo);
    }

    request.setMaxResultCount(maxResultCount);
    request.setOffset(offset);

    JobId jobId = mComponent.getLists(request, MakeFunctor(this, &AssociationListAPI::getListsCb), resultFunctor);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultFunctor);

    // clear the vector on stack in requirement
    request.getListInfoVector().clear();

    return jobId;
}

void AssociationListAPI::getListsCb(const Lists *response, BlazeError err, JobId jobid, const GetListsCb resultCb)
{
    if (err == ERR_OK)
    {
        const ListMembersVector &lists = response->getListMembersVector();

        ListMembersVector::const_iterator it = lists.begin();
        ListMembersVector::const_iterator itend = lists.end();
        for ( ; it != itend; ++it)
        {
            const ListMembers *listMembers = (*it);
            AssociationList *list = nullptr;

            ListByListTypeMap::iterator itr = mListTypeMap.find(listMembers->getInfo().getId().getListType());
            if (itr != mListTypeMap.end())
            {
                list = static_cast<AssociationList *>(&(*itr));
            }
            else
            {
                list = createLocalList(listMembers->getInfo().getId());
            }

            list->initFromServer(*listMembers);                
        }

        waitForAllUsers(err, jobid, resultCb);
    }
    else
    {
        resultCb(err, jobid);
    }


}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void AssociationListAPI::waitForAllUsers(BlazeError err, JobId jobid, const GetListsCb resultCb)
{
    AssociationListList::const_iterator it = getAssociationLists().begin();
    AssociationListList::const_iterator itend = getAssociationLists().end();
    for ( ; it != itend; it++)
    {
        AssociationList *list = *it;
        AssociationList::MemberVector::const_iterator itr = list->getMemberVector().begin();
        AssociationList::MemberVector::const_iterator itrEnd = list->getMemberVector().end();
        for ( ; itr != itrEnd; itr++)
        {
            const AssociationListMember *listMember = *itr;
            if ((listMember->getListMemberId().getUser().getBlazeId() != INVALID_BLAZE_ID) && (*(listMember->getListMemberId().getUser().getName()) != '\0') && (listMember->getUser() == nullptr))
            {
                JobId jobId = getBlazeHub()->getScheduler()->scheduleFunctor("waitForAllUsersCb",
                    MakeFunctor(this, &AssociationListAPI::waitForAllUsers),
                    err, jobid, resultCb);
                Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultCb);
                return;
            }
        }
    }

    resultCb(err, jobid);
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

bool AssociationListAPI::validateListIdentification(const ListIdentification &listIdentification) const
{
    if ((listIdentification.getListType() == LIST_TYPE_UNKNOWN) &&
        (listIdentification.getListName()[0] == '\0'))
    {
        return false;
    }
    
    return true;
}

JobId AssociationListAPI::getListForUser(const BlazeId blazeId, const ListIdentification &listIdentification, const GetListForUserCb &resultFunctor, const uint32_t maxResultCount/*=UINT32_MAX*/, const uint32_t offset/*=0*/)
{
    if ( ! validateListIdentification(listIdentification))
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("getListForUserCb", resultFunctor, (const ListMembers *)nullptr, ASSOCIATIONLIST_ERR_LIST_NOT_FOUND, jobId);
        Job::addTitleCbAssociatedObject(jobScheduler , jobId, resultFunctor);
        return jobId;
    }

    GetListForUserRequest request;
    request.setBlazeId(blazeId);
    request.setMaxResultCount(maxResultCount);
    request.setOffset(offset);
    listIdentification.copyInto(request.getListIdentification());

    JobId jobId = mComponent.getListForUser(request, MakeFunctor(this, &AssociationListAPI::getListForUserCb), resultFunctor);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultFunctor);
    return jobId;
}

void AssociationListAPI::getListForUserCb(const ListMembers *response, BlazeError err, JobId jobid, const GetListForUserCb resultCb)
{
    resultCb(response, err, jobid);
}  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

AssociationList* AssociationListAPI::getListByObjId(const EA::TDF::ObjectId& objId) const
{
    ListByBlazeObjectIdMap::const_iterator it = mBlazeObjectIdMap.find(objId);
    if (it != mBlazeObjectIdMap.end())
    {
        return (AssociationList *)(&(*it));
    }

    return nullptr;
}

AssociationList* AssociationListAPI::getListByName(const char8_t* listName) const
{
    ListByListNameMap::const_iterator it = mListNameMap.find(listName);
    if (it != mListNameMap.end())
    {
        return (AssociationList *)(&(*it));
    }

    return nullptr;
}

AssociationList* AssociationListAPI::getListByType(const ListType listType) const
{
    ListByListTypeMap::const_iterator it = mListTypeMap.find(listType);
    if (it != mListTypeMap.end())
    {
        return (AssociationList *)(&(*it));
    }

    return nullptr;
}

void AssociationListAPI::addListener(AssociationListAPIListener *listener)
{
    mDispatcher.addDispatchee(listener);
}

void AssociationListAPI::removeListener(AssociationListAPIListener *listener)
{
    mDispatcher.removeDispatchee(listener);
}

void AssociationListAPI::onUserUpdated(const UserManager::User& user)
{
    BlazeId blazeId = user.getId();
    if (blazeId != INVALID_BLAZE_ID)
    {
        AssociationListList::iterator it = mAssociationListList.begin();
        AssociationListList::iterator itend = mAssociationListList.end();
        for ( ; it != itend; ++it)
        {
            AssociationList *list = *it;
            AssociationListMember *listMember = list->getMemberByBlazeId(blazeId);

            if (listMember == nullptr)
                listMember = list->getMemberByNativeExternalId((ExternalId)blazeId);

            if (listMember == nullptr)
                listMember = list->getMemberByPlatformInfo(user.getPlatformInfo());

            if (listMember != nullptr)
            {
                if (listMember->getBlazeId() != blazeId)
                {
                    list->removeMemberFromIndicies(*listMember);
                    listMember->getListMemberId().getUser().setBlazeId(blazeId);
                    list->addMemberToIndicies(*listMember);
                }

                mDispatcher.dispatch<AssociationList *, const AssociationListMember *>("onMemberUpdated", &AssociationListAPIListener::onMemberUpdated, list, listMember);
            }
        }
    }
}

const char8_t *ListTypeToString(ListType listType)
{
    #define __LIST_TYPE_CASE(lt) case lt: return #lt
    switch (listType)
    {
        __LIST_TYPE_CASE(LIST_TYPE_UNKNOWN);
        __LIST_TYPE_CASE(LIST_TYPE_FRIEND);
        __LIST_TYPE_CASE(LIST_TYPE_RECENTPLAYER);
        __LIST_TYPE_CASE(LIST_TYPE_MUTE);
        __LIST_TYPE_CASE(LIST_TYPE_BLOCK);
        __LIST_TYPE_CASE(LIST_TYPE_FIRST_CUSTOM);
        default: return "";
    }
    #undef __LIST_TYPE_CASE
}

ListType StringToListType(const char8_t *listType)
{
    #define __LIST_TYPE_CASE(lt) else if (blaze_stricmp(listType, #lt) == 0) return lt
    if (listType == nullptr) return LIST_TYPE_UNKNOWN;
    __LIST_TYPE_CASE(LIST_TYPE_UNKNOWN);
    __LIST_TYPE_CASE(LIST_TYPE_FRIEND);
    __LIST_TYPE_CASE(LIST_TYPE_RECENTPLAYER);
    __LIST_TYPE_CASE(LIST_TYPE_MUTE);
    __LIST_TYPE_CASE(LIST_TYPE_BLOCK);
    else return LIST_TYPE_FIRST_CUSTOM;
    #undef __LIST_TYPE_CASE
}

JobId AssociationListAPI::getConfigListsInfo(const GetConfigListsInfoCb &resultFunctor)
{
    JobId jobId = mComponent.getConfigListsInfo(resultFunctor);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultFunctor);
    return jobId;
}

}
}
