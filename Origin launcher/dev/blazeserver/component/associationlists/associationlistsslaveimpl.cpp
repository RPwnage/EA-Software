/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"
#include "framework/controller/processcontroller.h"
#include "framework/util/locales.h"
#include "framework/slivers/slivermanager.h"
#include "framework/taskscheduler/taskschedulerslaveimpl.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmaster.h"
#include "associationlistsslaveimpl.h"

// associationlist includes

namespace Blaze
{
namespace Association
{

// static
AssociationListsSlave* AssociationListsSlave::createImpl()
{
    return BLAZE_NEW_NAMED("AssociationListsSlaveImpl") AssociationListsSlaveImpl();
}

///////////////////////////////////////////////////////////////////////////////
// constructor/destructor
//
AssociationListsSlaveImpl::AssociationListsSlaveImpl()
{
}

AssociationListsSlaveImpl::~AssociationListsSlaveImpl()
{
}


ListInfoPtr AssociationListsSlaveImpl::getTemplate(const ListIdentification& id) const
{
    if (id.getListType() != LIST_TYPE_UNKNOWN)
    {
        return getTemplate(id.getListType());
    }
    else if (id.getListName()[0] != '\0')
    {
        return getTemplate(id.getListName());
    }    

    return ListInfoPtr();
}

ListInfoPtr AssociationListsSlaveImpl::getTemplate(const char8_t *listName) const
{
    ListNameTemplateMap::const_iterator it = mListNameToListTemplateMap.find(listName);
    if (it != mListNameToListTemplateMap.end())
    {
        return it->second;
    }

    return ListInfoPtr();
}

ListInfoPtr AssociationListsSlaveImpl::getTemplate(ListType listType) const
{
    ListTypeTemplateMap::const_iterator it = mListTypeToListTemplateMap.find(listType);
    if (it != mListTypeToListTemplateMap.end())
    {
        return it->second;
    }

    return ListInfoPtr();
}


AssociationListRef AssociationListsSlaveImpl::getListByObjectId(const EA::TDF::ObjectId& id)
{
    BlazeObjectIdToAssociationListMap::iterator it = mBlazeObjectIdToAssociationListMap.find(id);
    if (it != mBlazeObjectIdToAssociationListMap.end())
    {
        return it->second;
    }

    return AssociationListRef(nullptr);
}


AssociationListRef AssociationListsSlaveImpl::getList(const ListIdentification& listId, BlazeId blazeId)
{
    AssociationListRef list;
    if (listId.getListType() != LIST_TYPE_UNKNOWN)
    {
        list = getList(listId.getListType(), blazeId);
    }
    else if (listId.getListName()[0] != '\0')
    {
        ListInfoPtr temp = getTemplate(listId.getListName());
        if (temp != nullptr)
        {
            list = getList(temp->getId().getListType(), blazeId);
        }
    }

    return list;
}


BlazeRpcError AssociationListsSlaveImpl::loadLists(const ListIdentificationVector& listIds, BlazeId ownerId, AssociationListVector& lists, const uint32_t maxResultCount, const uint32_t offset)
{
    BlazeRpcError result = ERR_OK;

    if (!listIds.empty())
    {
        for (ListIdentificationVector::const_iterator itr = listIds.begin(), end = listIds.end(); result == ERR_OK && itr != end; ++itr)
        {
            AssociationListRef list = findOrCreateList(**itr, ownerId);
            if (list != nullptr)
            {
                if ((result = list->loadFromDb(true, maxResultCount, offset)) == ERR_OK)
                {
                    lists.push_back(list);
                }
            }
            else
            {
                result = ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
            }
        }
    }
    else
    {
        for (ListInfoPtrVector::iterator itr = mListTemplates.begin(), end = mListTemplates.end(); result == ERR_OK && itr != end; ++itr)
        {
            AssociationListRef list = findOrCreateList(*itr, ownerId);
            if (list != nullptr)
            {
                if ((result = list->loadFromDb(true, maxResultCount, offset)) == ERR_OK)
                {
                    lists.push_back(list);
                }
            }
            else
            {
                result = ERR_SYSTEM;
            }
        }
    }


    return result;
}

AssociationListRef AssociationListsSlaveImpl::findOrCreateList(ListInfoPtr temp, BlazeId ownerId)
{
    AssociationListRef list;

    if (temp != nullptr)
    {
        list = getList(temp->getId().getListType(), ownerId); 

        if (list == nullptr)
        {
            list = BLAZE_NEW AssociationList(*this, *temp, ownerId);
        }

        // If no owner was set, and the requester is the user who owns the list, then cache the list:
        if (list->getOwnerPrimarySession() == nullptr && gCurrentUserSession != nullptr && gCurrentUserSession->getBlazeId() == ownerId)
        {
            // We only need to track the association list when the user is associated with this coreSlave. (Sliver)
            InstanceId id = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(gCurrentUserSession->getUserSessionId())); 
            if (id == gController->getInstanceId())
            {
                list->setOwnerPrimarySession(gCurrentUserSession.get());
                mBlazeObjectIdToAssociationListMap[list->getInfo().getBlazeObjId()] = list;
                mUserSessionToListsMap.insert(UserSessionIdToAssociationListMap::value_type(list->getOwnerPrimarySession()->getId(), list));
            }
        }
    }

    return list;
}


BlazeRpcError AssociationListsSlaveImpl::validateListMemberIdVector(const ListMemberIdVector &listMemberIdVector, UserInfoAttributesVector& results, BlazeId ownerId, ListMemberIdVector* extMemberIdVector)
{
    BlazeRpcError result = ERR_OK;
    ClientPlatformType ownerPlatform = INVALID;
    const char8_t* ownerNamespace = "";

    if (!gController->isSharedCluster())
    {
        ownerPlatform = gController->getDefaultClientPlatform();
        ownerNamespace = gController->getNamespaceFromPlatform(ownerPlatform);
        ownerId = INVALID_BLAZE_ID; // Skip platform checks on single-platform deployments
    }
    else if (ownerId != INVALID_BLAZE_ID)
    {
        UserInfoPtr ownerInfo;
        result = gUserSessionManager->lookupUserInfoByBlazeId(ownerId, ownerInfo);
        if (result != ERR_OK)
        {
            ERR_LOG("[AssociationListsSlaveImpl].validateListMemberIdVector() Failed to look up list owner with BlazeId [" << ownerId << "]; error [" << (ErrorHelp::getErrorName(result)) << "]");
            return ASSOCIATIONLIST_ERR_USER_NOT_FOUND;
        }
        ownerPlatform = ownerInfo->getPlatformInfo().getClientPlatform();
        ownerNamespace = ownerInfo->getPersonaNamespace();
    }
    else if (gCurrentUserSession != nullptr)
    {
        ownerPlatform = gCurrentUserSession->getClientPlatform();
        ownerNamespace = gCurrentUserSession->getPersonaNamespace();
    }

    const ListMemberId *memberId = nullptr;
    for (ListMemberIdVector::const_iterator it = listMemberIdVector.begin(), itend = listMemberIdVector.end(); (result == ERR_OK) && (it != itend); ++it)
    {
        bool requireUserToExist = false; 
        memberId = *it;
        UserInfoPtr userInfo;

        // if a blaze id is provided...
        if (memberId->getUser().getBlazeId() != INVALID_BLAZE_ID)
        {
            requireUserToExist = true; //if passed by blaze id, these guys have to exist or we throw an error 
            result = gUserSessionManager->lookupUserInfoByBlazeId(memberId->getUser().getBlazeId(), userInfo);
            if (result == ERR_OK && ownerId != INVALID_BLAZE_ID && userInfo->getPlatformInfo().getClientPlatform() != ownerPlatform)
            {
                ERR_LOG("[AssociationListsSlaveImpl].validateListMemberIdVector() List owner with BlazeId(" << ownerId << ") on platform '" << ClientPlatformTypeToString(ownerPlatform) << "' cannot be in an association list with users on other platforms (attempted to add user with BlazeId " << memberId->getUser().getBlazeId() << " on platform '" << ClientPlatformTypeToString(userInfo->getPlatformInfo().getClientPlatform()) << "').");
                return ASSOCIATIONLIST_ERR_INVALID_USER;
            }
        }
        else if (has1stPartyPlatformInfo(memberId->getUser().getPlatformInfo()))
        {
            if (ownerId != INVALID_BLAZE_ID && memberId->getUser().getPlatformInfo().getClientPlatform() != ownerPlatform)
            {
                eastl::string platformInfoStr;
                ERR_LOG("[AssociationListsSlaveImpl].validateListMemberIdVector() List owner with BlazeId(" << ownerId << ") on platform '" << ClientPlatformTypeToString(ownerPlatform) << "' cannot be in an association list with users on other platforms (attempted to add user with platformInfo: " << platformInfoToString(memberId->getUser().getPlatformInfo(), platformInfoStr) << ").");
                return ASSOCIATIONLIST_ERR_INVALID_USER;
            }
            result = gUserSessionManager->lookupUserInfoByPlatformInfo(memberId->getUser().getPlatformInfo(), userInfo, false /*ignoreStatus*/);
        }
        else if (memberId->getUser().getName()[0] != '\0')
        {
            result = gUserSessionManager->lookupUserInfoByPersonaName(memberId->getUser().getName(), ownerPlatform, userInfo, ownerNamespace);
        }
        else 
        {
            result = ASSOCIATIONLIST_ERR_INVALID_USER;
        }
        
        if (result == ERR_OK)
        {
            if (userInfo != nullptr && gCurrentUserSession != nullptr && userInfo->getId() == gCurrentUserSession->getBlazeId())
            {
                // then, make sure it is not the id of the current user...
                result = ASSOCIATIONLIST_ERR_CANNOT_INCLUDE_SELF;
            }
            else
            {
                //Push to the back of the finished list if no dupes found
                for (UserInfoAttributesVector::const_iterator uitr = results.begin(), uend = results.end(); uitr != uend; ++uitr)
                {
                    if ((*uitr).userInfo->getId() == userInfo->getId())
                    {
                        result = ASSOCIATIONLIST_ERR_DUPLICATE_USER_FOUND;
                        break;
                    }
                }

                if (result == ERR_OK)
                {
                    //finally no errors, push this guy on the back of the results list
                    UserInfoAttributes& resultsMember = results.push_back();
                    resultsMember.userInfo = userInfo;
                    resultsMember.userAttributes.setBits(memberId->getAttributes().getBits());
                }
                
            }
        }

        if (result != ERR_OK && extMemberIdVector)
        {
            ListMemberId* extMemberId = extMemberIdVector->pull_back();
            memberId->copyInto(*extMemberId);
        }

        if (result == USER_ERR_USER_NOT_FOUND)
        {
            result = requireUserToExist ? ASSOCIATIONLIST_ERR_USER_NOT_FOUND : ERR_OK; //we just silently drop external users that don't live in our DB.
        }
    }

    if (result != ERR_OK)
    {
        results.clear();

        if (memberId != nullptr)
        {
            eastl::string platformInfoStr;
            TRACE_LOG("[AssociationListsSlaveImpl].validateListMemberIdVector() A validation attempt on a ListMemberId failed with error [" 
                << (ErrorHelp::getErrorName(result)) << "]: BlazeId [" << memberId->getUser().getBlazeId() << "] PlatformInfo [" << platformInfoToString(memberId->getUser().getPlatformInfo(), platformInfoStr)
                << "] PersonaName [" << memberId->getUser().getName() << "]");
        }
    }

    return result;
}

BlazeRpcError AssociationListsSlaveImpl::checkUserEditPermission(const BlazeId blazeId)
{
    bool isCurrentUser = (blazeId == INVALID_BLAZE_ID || (gCurrentUserSession != nullptr && blazeId == gCurrentUserSession->getBlazeId()));

    // check if the current user has the permission to modify another user's list
    if (!isCurrentUser && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_ASSOCIATIONLIST_ANY_USER_EDIT))
    {
        ERR_LOG("[AssociationListsSlaveImpl].checkUserEditPermission: User[" << (gCurrentUserSession != nullptr ? gCurrentUserSession->getBlazeId() : -1) << "] does not have permission to modify user[" << blazeId << "]'s lists!");
        return ERR_AUTHORIZATION_REQUIRED;
    }
    return Blaze::ERR_OK;
}


///////////////////////////////////////////////////////////////////////////
// Private Methods
//


GetConfigListsInfoError::Error AssociationListsSlaveImpl::processGetConfigListsInfo(ConfigLists& response,const Blaze::Message * message)
{
    getConfig().getLists().copyInto(response.getListsInfo());
    return GetConfigListsInfoError::ERR_OK;
}

///////////////////////////////////////////////////////////////////////////
// UserSetProvider interface functions
//
BlazeRpcError AssociationListsSlaveImpl::getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids)
{
    AssociationListRef ref(nullptr);
    BlazeRpcError result = loadListByObjectId(bobjId, ref);

    if (result == ERR_OK)
    {
        ids.reserve(ref->getMembers().size());
        for (AssociationList::AssociationListMemberVector::const_iterator cit = ref->getMembers().begin(),
            citEnd = ref->getMembers().end(); cit != citEnd; ++cit)
        {
            UserSessionId sessionId = gUserSessionManager->getPrimarySessionId((*cit)->getBlazeId());
            if (sessionId != INVALID_USER_SESSION_ID)
                ids.push_back(sessionId);
        }
    }
    
    return result;
}

BlazeRpcError AssociationListsSlaveImpl::getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& ids)
{
    AssociationListRef ref(nullptr);
    BlazeRpcError result = loadListByObjectId(bobjId, ref);

    if (result == ERR_OK)
    {
        ids.reserve(ref->getMembers().size());
        for (AssociationList::AssociationListMemberVector::const_iterator cit = ref->getMembers().begin(),
            citEnd = ref->getMembers().end(); cit != citEnd; ++cit)
        {
            ids.push_back((*cit)->getBlazeId());
        }
    }

    return result;
}

BlazeRpcError AssociationListsSlaveImpl::getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids)
{
    AssociationListRef ref(nullptr);
    BlazeRpcError result = loadListByObjectId(bobjId, ref);

    if (result == ERR_OK)
    {
        ids.reserve(ref->getMembers().size());
        for (AssociationList::AssociationListMemberVector::const_iterator cit = ref->getMembers().begin(),
            citEnd = ref->getMembers().end(); cit != citEnd; ++cit)
        {
            UserIdentificationPtr userIdent = ids.pull_back();
            userIdent->setBlazeId((*cit)->getBlazeId());
            (*cit)->getUserInfo()->getPlatformInfo().copyInto(userIdent->getPlatformInfo());
            userIdent->setName((*cit)->getUserInfo()->getPersonaName());
            userIdent->setAccountId(userIdent->getPlatformInfo().getEaIds().getNucleusAccountId());
            userIdent->setAccountLocale((*cit)->getUserInfo()->getAccountLocale());
            userIdent->setAccountCountry((*cit)->getUserInfo()->getAccountCountry());
            userIdent->setOriginPersonaId((*cit)->getUserInfo()->getOriginPersonaId());
            userIdent->setPidId((*cit)->getUserInfo()->getPidId());
            userIdent->setPersonaNamespace((*cit)->getUserInfo()->getPersonaNamespace());
        }
    }

    return result;
}

BlazeRpcError AssociationListsSlaveImpl::countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    count = 0;
    
    AssociationListRef ref(nullptr);
    BlazeRpcError result = loadListByObjectId(bobjId, ref);

    if (result == ERR_OK)
    {
        for (AssociationList::AssociationListMemberVector::const_iterator cit = ref->getMembers().begin(),
            citEnd = ref->getMembers().end(); cit != citEnd; ++cit)
        {
            UserSessionId sessionId = gUserSessionManager->getPrimarySessionId((*cit)->getBlazeId());
            if (sessionId != INVALID_USER_SESSION_ID)
                ++count;
        }
    }

    return result;
}

BlazeRpcError AssociationListsSlaveImpl::countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    AssociationListRef ref(nullptr);
    BlazeRpcError result = loadListByObjectId(bobjId, ref);

    if (result == ERR_OK)
    {
        count = ref->getMembers().size();
    }

    return result;
}

RoutingOptions AssociationListsSlaveImpl::getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId)
{
    // All association lists use the object id of a user as their id, and expect to use that user's sliver as the primary slave. 
    RoutingOptions routeTo;
    UserSessionId userSessionId = gUserSessionManager->getPrimarySessionId(bobjId.id);
    if (userSessionId == INVALID_USER_SESSION_ID)
    {
        UserSessionIdList sessionIds;
        gUserSessionManager->getSessionIds(bobjId.id, sessionIds);
        if (!sessionIds.empty())
            userSessionId = sessionIds.front();
    }

    if (userSessionId != INVALID_USER_SESSION_ID)
        routeTo.setSliverIdentityFromKey(UserSessionsMaster::COMPONENT_ID, userSessionId);
    else
        routeTo.setInstanceId(getLocalInstanceId());

    return routeTo;
}

BlazeRpcError AssociationListsSlaveImpl::loadListByObjectId(const EA::TDF::ObjectId& bobjId, AssociationListRef& ref)
{
    if (bobjId.type.component != COMPONENT_ID)
    {
        WARN_LOG("[AssociationListsSlaveImpl].getMemberInfoVector: The indicated object (type - " << bobjId.type.toString().c_str() 
                 << ") is not an association list");
        return ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
    }

    //If the user doesn't exist, there's no chance for a list to be valid (even if its an empty list)
    UserInfoPtr user;
    if (gUserSessionManager->lookupUserInfoByBlazeId(bobjId.id, user) != ERR_OK)
    {
        //bail 
        return ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
    }
    
    ref = findOrCreateList(getTemplate(bobjId.type.type), bobjId.id);

    return ref != nullptr ? ref->loadFromDb(false) : ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;    
}


//Function to check against the provided list owners if a specific member is contained in their list
//Eg. Am I existing in these people's list?
BlazeRpcError AssociationListsSlaveImpl::checkListMembership(const ListIdentification& listIdentification, const BlazeIdList& ownersBlazeIds, const BlazeId memberBlazeId, BlazeIdList& existingOwnersBlazeIds) const
{
    existingOwnersBlazeIds.clearVector();

    if (ownersBlazeIds.size() < 1)  //ownersList has to have at least 1 owner to find
    {
        return ASSOCIATIONLIST_ERR_INVALID_USER;    //no owners included in existingOwnersBlazeIds vector
    }

    //early out if one owner is self
    for (BlazeIdVector::const_iterator findOwnersItr = ownersBlazeIds.begin(), findOwnersEnd = ownersBlazeIds.end(); findOwnersItr!=findOwnersEnd; findOwnersItr++)
    {
        if (*findOwnersItr == memberBlazeId)
        {
            return ASSOCIATIONLIST_ERR_CANNOT_INCLUDE_SELF;
        }
    }

    ListInfoPtr listInfoPtr = getTemplate(listIdentification);
    if (listInfoPtr == nullptr)
    {
        return ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
    }

    DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].loadFromDb: Unable to obtain a database connection.");
        return ERR_DB_SYSTEM;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    //depending if the list is mutual, we treat the list columns differently
    if (!listInfoPtr->getStatusFlags().getMutualAction())
    {
        //Non-mutual lists you’re actually asking the opposite question – is this user a member of any of the following users’s list?  
        //Ie. need to view from the target user's list, and see if there is a match for memberblazeid=ownerblazeid
        BlazeIdVector::const_iterator findOwnersItr = ownersBlazeIds.begin();

        query->append("SELECT * FROM `user_association_list_$s` WHERE blazeid IN ($D", listInfoPtr->getId().getListName(), *findOwnersItr++);
        for (; findOwnersItr != ownersBlazeIds.end(); findOwnersItr++)
        {
            query->append(", $D", *findOwnersItr);
        }
        query->append(") AND memberblazeid=$D;", memberBlazeId);
    }
    else
    {
        //Mutual lists are done as a single table with the entries in either column based on value.  
        //So we need to consider both columns. 
        //A slight optimization here since for mutual list during insertion, the first column's blazeid is always < second column's, 
        //we can further filter the GMs for the "IN" query which will exist in their respective column
        query->append("SELECT * FROM `user_association_list_$s` WHERE ", listInfoPtr->getId().getListName());               

        uint32_t counter1 = 0;
        for (BlazeIdVector::const_iterator findOwnersItr = ownersBlazeIds.begin(), findOwnersEnd = ownersBlazeIds.end(); findOwnersItr != findOwnersEnd; findOwnersItr++)
        {                
            if (*findOwnersItr < memberBlazeId)
            {
                if (counter1 == 0)
                {
                    query->append("(blazeid IN ($D", *findOwnersItr);    
                }
                else 
                {
                    query->append(", $D", *findOwnersItr);
                }

                counter1++;
            }
        }
        if (counter1 > 0)
        {
            query->append(") AND memberblazeid=$D)", memberBlazeId);
            query->append((counter1 < ownersBlazeIds.size() ? " OR " : ""));
        }
        
        if (counter1 < ownersBlazeIds.size())  //no need to eval this block when all owners blazeids have been added
        {
            uint32_t counter2 = 0;
            for (BlazeIdVector::const_iterator findOwnersItr = ownersBlazeIds.begin(), findOwnersEnd = ownersBlazeIds.end(); findOwnersItr != findOwnersEnd; findOwnersItr++)
            {
                if (*findOwnersItr > memberBlazeId)
                {
                    if (counter2 == 0)
                    {
                        query->append("(blazeid=$D AND memberblazeid IN ($D", memberBlazeId, *findOwnersItr);                    
                    }
                    else
                    {           
                        query->append(", $D", *findOwnersItr);
                    } 

                    counter2++;
                }
            }
            query->append("))");
        }
        
        query->append(";");
    }

    //Execute the query
    DbResultPtr dbResult;
    BlazeRpcError dbErr;
    if ((dbErr = conn->executeQuery(query, dbResult)) != ERR_OK)
    {
        ERR_LOG("[AssociationList].checkListMembership: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
        return dbErr;
    }     

    //Fill in the existingMembers list with non-self
    for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
    {
        BlazeId id1 = (*itr)->getUInt64((uint32_t) 0), id2 = (*itr)->getUInt64(1);
        existingOwnersBlazeIds.push_back((id1 != memberBlazeId) ? id1 : id2);
    }

    return existingOwnersBlazeIds.empty() ? ASSOCIATIONLIST_ERR_USER_NOT_FOUND : ERR_OK;
}


//Function to check against a specific user's list if the members are contained in it
//Eg. Is the provided list of people existing in my list?
BlazeRpcError AssociationListsSlaveImpl::checkListContainsMembers(const ListIdentification& listIdentification, const BlazeId ownerBlazeId, const BlazeIdList& membersBlazeIds, BlazeIdList& existingMembersBlazeIds) const
{
    existingMembersBlazeIds.clearVector();

    ListInfoPtr listInfoPtr = getTemplate(listIdentification);
    if (listInfoPtr == nullptr)
    {
        return ASSOCIATIONLIST_ERR_LIST_NOT_FOUND;
    }
    
    //depending if the list is mutual, we treat the list columns differently
    if (listInfoPtr->getStatusFlags().getMutualAction())
    {
        //for mutual list, it's the same as asking if the owner is contained in any of the provided members' list (eg. need to look at both columns)
        //simply call and return the result from checkListMembership()
        return checkListMembership(listIdentification, membersBlazeIds, ownerBlazeId, existingMembersBlazeIds);     
    }

    if (membersBlazeIds.size() < 1)  //memberlist has to have at least 1 member to find
    {
        return ASSOCIATIONLIST_ERR_INVALID_USER;    //no member included in existingMemberBlazeIds vector
    }

    //early out if one member is self
    for (BlazeIdVector::const_iterator findMembersItr = membersBlazeIds.begin(), findMembersEnd = membersBlazeIds.end(); findMembersItr!=findMembersEnd; findMembersItr++)
    {
        if (*findMembersItr == ownerBlazeId)
        {
            return ASSOCIATIONLIST_ERR_CANNOT_INCLUDE_SELF;
        }
    }

    DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].loadFromDb: Unable to obtain a database connection.");
        return ERR_DB_SYSTEM;
    }
            
    //For non-mutual lists, we are asking "Are these members existing in my list?"  
    BlazeIdVector::const_iterator findMembersItr = membersBlazeIds.begin();

    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SELECT * FROM `user_association_list_$s` WHERE memberblazeid IN ($D", listInfoPtr->getId().getListName(), *findMembersItr++);
    for (; findMembersItr != membersBlazeIds.end(); findMembersItr++)
    {
        query->append(", $D", *findMembersItr);
    }
    query->append(") AND blazeid=$D;", ownerBlazeId);

    //Execute the query    
    DbResultPtr dbResult;
    BlazeRpcError dbErr;
    if ((dbErr = conn->executeQuery(query, dbResult)) != ERR_OK)
    {
        ERR_LOG("[AssociationList].checkListContainsMembers: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
        return dbErr;
    }     

    //Fill in the existingMembers
    for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
    {
        BlazeId memberId = (*itr)->getUInt64(1);
        existingMembersBlazeIds.push_back(memberId);
    }

    return existingMembersBlazeIds.empty() ? ASSOCIATIONLIST_ERR_USER_NOT_FOUND : ERR_OK;
}


///////////////////////////////////////////////////////////////////////////
// UserSessionSubscriber interface functions
//    
/* \brief A user was removed from the manager  (global event) */
void AssociationListsSlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
    if (userSession.isPrimarySession())
    {
        ListMemberByBlazeIdRange range = mPendingSubscribeMap.equal_range(userSession.getBlazeId());
        for (; range.first != range.second; ++range.first)
        {
            AssociationListMember* listMember = range.first->second;
            if (listMember != nullptr)
            {
                listMember->handleExistenceChanged(userSession.getBlazeId());
            }
            else
            {
                ERR_LOG("[AssociationListsSlaveImpl].onUserSessionExistence: usersession=" << userSession.getId() << ", for user=" << userSession.getBlazeId()
                    << ":(" << userSession.getPersonaName() << ")" << userSession.getPersonaName() << " on platform=" << userSession.getClientPlatform() << "'s BlazeId referenced a nullptr in mPendingSubscribeMap.");

            }
        }
    }
}

void AssociationListsSlaveImpl::onUserSessionExtinction(const UserSession& userSession)
{
    if (userSession.isPrimarySession())
    {
        ListMemberByBlazeIdRange range = mPendingSubscribeMap.equal_range(userSession.getBlazeId());
        for (; range.first != range.second; ++range.first)
        {
            AssociationListMember* listMember = range.first->second;
            if (listMember != nullptr)
            {
                listMember->handleExistenceChanged(userSession.getBlazeId());
            }
            else
            {
                ERR_LOG("[AssociationListsSlaveImpl].onUserSessionExtinction: usersession=" << userSession.getId() << ", for user=" << userSession.getBlazeId()
                    << ":(" << userSession.getPersonaName() << ")" << userSession.getPersonaName() << " on platform=" << userSession.getClientPlatform() << "'s BlazeId referenced a nullptr in mPendingSubscribeMap.");

            }
            
        }
    }

    eastl::pair<UserSessionIdToAssociationListMap::iterator, UserSessionIdToAssociationListMap::iterator> result = mUserSessionToListsMap.equal_range(userSession.getUserSessionId());
    for (UserSessionIdToAssociationListMap::iterator itr = result.first; itr != result.second; ++itr)
    {
        mBlazeObjectIdToAssociationListMap.erase(itr->second->getInfo().getBlazeObjId());
    }
    mUserSessionToListsMap.erase(result.first, result.second);
}

void AssociationListsSlaveImpl::onNotifyUpdateListMembershipSlave(const Blaze::Association::UpdateListMembersResponse& data, Blaze::UserSession*)
{
    //Need to go to a fiber b/c of updates
    Fiber::CreateParams params(Fiber::STACK_SMALL);
    gSelector->scheduleFiberCall(this, &AssociationListsSlaveImpl::handleUpdateListMembership, UpdateListMembersResponsePtr(data.clone()), "AssociationListsSlaveImpl::notifyUsersOfListMembershipChange", params);
}

void AssociationListsSlaveImpl::handleUpdateListMembership(Blaze::Association::UpdateListMembersResponsePtr data)
{  
    //if this user lives here, operate on them.
    AssociationListRef list = getList(data->getListType(), data->getOwnerId());
    if (list != nullptr)
    {
        list->handleMemberUpdateNotification(*data);
    }
}



///////////////////////////////////////////////////////////////////////////////
// Component methods
//
bool AssociationListsSlaveImpl::onValidateConfig(AssociationlistsConfig& config, const AssociationlistsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    const ListDataVector& assocLists = config.getLists();
    for (ListDataVector::const_iterator it = assocLists.begin(); it != assocLists.end(); ++it)
    {
        const ListData* listData = *it;
        // list name
        if (listData->getName()[0] == '\0')
        {
            eastl::string msg;
            msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list name [%s] is invalid.",
                listData->getName());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            for (ListDataVector::const_iterator nameIt = assocLists.begin(); nameIt != it; ++nameIt)
            {
                if (blaze_strcmp(listData->getName(), (*nameIt)->getName()) == 0)
                {
                    eastl::string msg;
                    msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list name [%s] is duplicated.",
                        listData->getName());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    break;
                }
            }
        }

        // list type
        if (listData->getId() == LIST_TYPE_UNKNOWN)
        {
            eastl::string msg;
            msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list type [%u] for list [%s] is invalid.",
                listData->getId(), listData->getName());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            for (ListDataVector::const_iterator typeIt = assocLists.begin(); typeIt != it; ++typeIt)
            {
                if (listData->getId() == (*typeIt)->getId())
                {
                    eastl::string msg;
                    msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list type [%u] for list [%s] is duplicated.",
                        listData->getId(), listData->getName());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    break;
                }
            }
        }

        // maxsize
        if (listData->getMaxSize() == 0)
        {
            eastl::string msg;
            msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list max size [%u] for list [%s] is invalid.",
                listData->getMaxSize(), listData->getName());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        // mutaul action can't combine with rollover
        if (listData->getMutualAction() && listData->getRollover())
        {
            eastl::string msg;
            msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list '%s' supporting rollover can not support mutual actions.  Aborting configuration.",
                listData->getName());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        // pair action can't combine with rollover and mutual action
        if (listData->getPairId() > 0)
        {
            if (listData->getRollover())
            {
                eastl::string msg;
                msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list '%s' supporting rollover can not support paired actions.  Aborting configuration.",
                    listData->getName());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            if (listData->getMutualAction())
            {
                eastl::string msg;
                msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list '%s' supporting mutual actions can not support paired actions.  Aborting configuration.",
                    listData->getName());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }
    validateConfigurePairLists(assocLists, validationErrors);

    if (referenceConfigPtr != nullptr)
    {
        // new lists in reconfigurations should contain all the old lists
        const ListDataVector& referAssocLists = referenceConfigPtr->getLists();
        for (ListDataVector::const_iterator referIt = referAssocLists.begin(); referIt != referAssocLists.end(); ++referIt)
        {
            ListDataVector::const_iterator it = assocLists.begin();
            for (; it != assocLists.end(); ++it)
            {
                if ((*referIt)->getId() == (*it)->getId() && blaze_strcmp((*referIt)->getName(), (*it)->getName()) == 0)
                    break;
            }
            if (it == assocLists.end())
            {
                eastl::string msg;
                msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - list(type[%d], name[%s]) is missing from reconfigurations. Aborting reconfigure process.",
                    (*referIt)->getId(), (*referIt)->getName());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            else
            {
                // max size can be raised, roller over can be changed, other changes are not allowed
                if ((*it)->getMaxSize() < (*referIt)->getMaxSize())
                {
                    eastl::string msg;
                    msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - max size(%d) of list(type[%d], name[%s]) can not be smaller than original max size(%d). Aborting reconfigure process.",
                        (*it)->getMaxSize(), (*referIt)->getId(), (*referIt)->getName(), (*referIt)->getMaxSize());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
                if ((*it)->getMutualAction() != (*referIt)->getMutualAction())
                {
                    eastl::string msg;
                    msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - mutual action of list(type[%d], name[%s]) can not be changed. Aborting reconfigure process.",
                        (*referIt)->getId(), (*referIt)->getName());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
                if ((*it)->getPairId() != (*referIt)->getPairId())
                {
                    eastl::string msg;
                    msg.sprintf("[AssociationListsSlaveImpl].onValidateConfig() - paired id of list(type[%d], name[%s]) can not be changed. Aborting reconfigure process.",
                        (*referIt)->getId(), (*referIt)->getName());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
    }

    return validationErrors.getErrorMessages().empty();
}

bool AssociationListsSlaveImpl::onActivate()
{
    // register itself to user set manager
    gUserSessionManager->addSubscriber(*this);
    gUserSetManager->registerProvider(COMPONENT_ID, *this);
    return true;
}

void AssociationListsSlaveImpl::onShutdown()
{

    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->removeLocalUserSessionSubscriber(*this);

    gUserSessionManager->removeSubscriber(*this);
    
    // remove itself from user set manager
    gUserSetManager->deregisterProvider(COMPONENT_ID);
}

bool AssociationListsSlaveImpl::onConfigure()
{
    TRACE_LOG("[AssociationListsSlaveImpl].onConfigure");
    const AssociationlistsConfig &config = getConfig();
    
    const ListDataVector& assocLists = config.getLists();
    if (assocLists.size() == 0)
        return true;

    if (configureCommon())
    {
        BlazeRpcError rc = ERR_SYSTEM;
        TaskSchedulerSlaveImpl* scheduler = static_cast<TaskSchedulerSlaveImpl*>
            (gController->getComponent(TaskSchedulerSlave::COMPONENT_ID, true, true, &rc));

        if (scheduler != nullptr)
        {
            rc = scheduler->runOneTimeTaskAndBlock("assoc_list_create", COMPONENT_ID, 
                TaskSchedulerSlaveImpl::RunOneTimeTaskCb(this, &AssociationListsSlaveImpl::createOrCheckTables));
            if (rc != ERR_OK)
            {
                FATAL_LOG("[AssociationListsSlaveImpl].onConfigure : Failed to run create tables task!");
                return false;
            }
        }
        else
        {
            FATAL_LOG("[AssociationListsSlaveImpl].onConfigure : The task scheduler is not found!");
            return false;
        }
    }

    return true;
}

bool AssociationListsSlaveImpl::onResolve()
{
    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->addLocalUserSessionSubscriber(*this);

    return true;
}

bool AssociationListsSlaveImpl::onReconfigure()
{
    INFO_LOG("[AssociationListsSlaveImpl].onReconfigure");
    const AssociationlistsConfig &config = getConfig();

    const ListDataVector& assocLists = config.getLists();
    if (assocLists.size() == 0)
        return true;

    return configureCommon();
}

bool AssociationListsSlaveImpl::configureCommon()
{
    //Clear out any existing templates
    mListNameToListTemplateMap.clear();
    mListTypeToListTemplateMap.clear();
    mListTemplates.clear();

    const AssociationlistsConfig &config = getConfig();

    const ListDataVector& assocLists = config.getLists();

    
    for (ListDataVector::const_iterator it = assocLists.begin(),
         endit = assocLists.end(); it != endit; ++it)
    {
        ListInfo* newListTemplateInfo = BLAZE_NEW ListInfo;

        // list name
        newListTemplateInfo->getId().setListName((*it)->getName());

        // list type
        newListTemplateInfo->getId().setListType((*it)->getId());

        // maxsize
        newListTemplateInfo->setMaxSize((*it)->getMaxSize());

        newListTemplateInfo->getStatusFlags().setBits(0);

        // rollover setting, default false
        if ((*it)->getRollover())
        {
            newListTemplateInfo->getStatusFlags().setRollover();
        }

        // subscribe setting, default false
        if ((*it)->getSubscribe())
        {
            newListTemplateInfo->getStatusFlags().setSubscribed();
        }

        // mutaul action setting, default false
        if ((*it)->getMutualAction())
        {
            newListTemplateInfo->getStatusFlags().setMutualAction();
        }

        // paired action setting, default 0
        if ((*it)->getPairId() > 0)
        {
            newListTemplateInfo->getStatusFlags().setPaired();
            newListTemplateInfo->setPairId((*it)->getPairId());
        }

        //  load offline UED when loading list.
        if ((*it)->getLoadOfflineUED())
        {
            newListTemplateInfo->getStatusFlags().setOfflineUED();
        }

        ListInfoPtr ptr(newListTemplateInfo);
        mListTypeToListTemplateMap[newListTemplateInfo->getId().getListType()] = ptr;
        mListNameToListTemplateMap[newListTemplateInfo->getId().getListName()] = ptr;
        mListTemplates.push_back(ptr);
    }

    for (ListInfoPtrVector::iterator it = mListTemplates.begin(); it != mListTemplates.end(); it++)
    {
        ListInfoPtr listInfo = *it;

        if (listInfo->getStatusFlags().getPaired())
        {
            ListType listType = listInfo->getId().getListType();
            ListType pairListType = listInfo->getPairId();

             for (ListInfoPtrVector::iterator innerIt = mListTemplates.begin(); innerIt != mListTemplates.end(); innerIt++)
            {
                if (pairListType == (*innerIt)->getId().getListType())
                {
                    listInfo->setPairName((*innerIt)->getId().getListName());
                    listInfo->setPairMaxSize((*innerIt)->getMaxSize());
                    (*innerIt)->setPairId(listType);
                    (*innerIt)->setPairName(listInfo->getId().getListName());
                    (*innerIt)->setPairMaxSize(listInfo->getMaxSize());
                    break;
                }
            }
        }
    }

    return true;
}



void AssociationListsSlaveImpl::validateConfigurePairLists(const ListDataVector& assocLists, ConfigureValidationErrors& validationErrors) const
{
    eastl::list<ListType> pairIdList;

    for (ListDataVector::const_iterator it = assocLists.begin(); it != assocLists.end(); it++)
    {
        const ListData* listData = *it;

        if (listData->getPairId() > 0)
        {
            ListType listType = listData->getId();
            ListType pairListType = listData->getPairId();
            const ListData* pairListData = nullptr;

            if (listType == pairListType)
            {
                eastl::string msg;
                msg.sprintf("[AssociationListsSlaveImpl].validatePairLists() - pair list type [%u] for list [%s] can not be same to itself.",
                    pairListType, listData->getName());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            for (ListDataVector::const_iterator itor = assocLists.begin(); itor != assocLists.end(); ++itor)
            {
                if (pairListType == (*itor)->getId())
                {
                    pairListData = *itor;
                    break;
                }
            }
            if (pairListData == nullptr)
            {
                eastl::string msg;
                msg.sprintf("[AssociationListsSlaveImpl].validatePairLists() - pair list type [%u] for list [%s] can not be found.",
                    pairListType, listData->getName());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            else
            {
                if (pairListData->getMutualAction())
                {
                    eastl::string msg;
                    msg.sprintf("[AssociationListsSlaveImpl].validatePairLists() - pair list type [%u] for list [%s] can not support mutual action.",
                        pairListType, listData->getName());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
                if (pairListData->getPairId() > 0)
                {
                    eastl::string msg;
                    msg.sprintf("[AssociationListsSlaveImpl].validatePairLists() - pair list type [%u] for list [%s] can not be declared as paired.",
                        pairListType, listData->getName());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }

            bool pairTypeDuplicated = false;
            for (eastl::list<ListType>::iterator pairIt = pairIdList.begin(); pairIt != pairIdList.end(); pairIt++)
            {
                if (pairListType == *pairIt)
                {
                    pairTypeDuplicated = true;
                    break;
                }
            }
            if (pairTypeDuplicated)
            {
                eastl::string msg;
                msg.sprintf("[AssociationListsSlaveImpl].validatePairLists() - pair list type [%u] for list [%s] is duplicated.",
                    pairListType, listData->getName());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            else
            {
                pairIdList.push_back(pairListType);
            }
        }
    }
}

void AssociationListsSlaveImpl::onLocalUserSessionLogin(const UserSessionMaster& userSession)
{
    if (!userSession.getSessionFlags().getFirstConsoleLogin())
        return;

    // Spawn a new fiber to handle the DB lookup:
    Fiber::CreateParams params(Fiber::STACK_SMALL);
    gSelector->scheduleFiberCall(this, &AssociationListsSlaveImpl::onUserFirstLogin, userSession.getBlazeId(), "AssociationListsSlaveImpl::onUserFirstLogin", params);
}
    
void AssociationListsSlaveImpl::onLocalUserSessionExported(const UserSessionMaster& userSession) 
{
    // Session export indicates the the user left this server and went to a new one. (Sliver migration)
    // Remove any cached lists (they'll need to get the lists again on their new server)
    // Also, this doesn't care about the edge case where multiple users sessions are on the same coreSlave (is that even possible?)
    InstanceId id = gSliverManager->getSliverInstanceId(UserSessionsMaster::COMPONENT_ID, GetSliverIdFromSliverKey(userSession.getUserSessionId()));
    if (id == gController->getInstanceId())
    {
        eastl::pair<UserSessionIdToAssociationListMap::iterator, UserSessionIdToAssociationListMap::iterator> result = mUserSessionToListsMap.equal_range(userSession.getUserSessionId());
        for (UserSessionIdToAssociationListMap::iterator itr = result.first; itr != result.second; ++itr)
        {
            mBlazeObjectIdToAssociationListMap.erase(itr->second->getInfo().getBlazeObjId());
        }
        mUserSessionToListsMap.erase(result.first, result.second);
    }

}

void AssociationListsSlaveImpl::onUserFirstLogin(BlazeId blazeId)
{
    UserInfoPtr userInfo;

    BlazeRpcError result = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo);
    if (result != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImp].onUserSessionFirstLogin: Unable to lookupUserInfoByBlazeId for the new user id " << blazeId  <<".  This user will not have his association lists updated correctly." );
        return;
    }


    //Create a new entry for each list type we have.
    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationListsSlaveImp].onUserSessionFirstLogin: Could not get DB conn" );
        return;
    }

    bool usesExtString = gController->usesExternalString(userInfo->getPlatformInfo().getClientPlatform());
    const char8_t* extString = "";
    Blaze::ExternalId extId = INVALID_EXTERNAL_ID;
    if (usesExtString)
        extString = userInfo->getPlatformInfo().getExternalIds().getSwitchId();
    else
        extId = getExternalIdFromPlatformInfo(userInfo->getPlatformInfo());

    if (extString[0] == '\0' && extId == INVALID_EXTERNAL_ID)
    {
        ERR_LOG("[AssociationListsSlaveImp].onUserSessionFirstLogin: User on platform '" << ClientPlatformTypeToString(userInfo->getPlatformInfo().getClientPlatform()) <<
            "' has no valid external identifier. Cannot invalidate hashes in the user_association_list_size for the new login.");
        return;
    }

    const char8_t* tableSuffix = "shared";
    if (!gController->isSharedCluster())
        tableSuffix = usesExtString ? "string" : "id";

    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    // Find all BlazeId/ListNames for users in the external id list table:
    for (ListInfoPtrVector::iterator it = mListTemplates.begin(); it != mListTemplates.end(); it++)
    {
        ListInfoPtr listInfo = *it;

        if (listInfo->getStatusFlags().getMutualAction() || listInfo->getStatusFlags().getPaired())
            continue;

        // Look up the players in the list:
        DB_QUERY_RESET(query);

        query->append("SELECT `blazeid` FROM `user_association_list_$s_ext$s` WHERE ", listInfo->getId().getListName(), tableSuffix);

        if (gController->isSharedCluster())
            query->append("`platform` = '$s' AND ", ClientPlatformTypeToString(userInfo->getPlatformInfo().getClientPlatform()));

        if (usesExtString)
            query->append("`externalstring` = '$s';", extString);
        else
            query->append("`externalid` = $U;", extId);

        DbResultPtr dbResult;
        if ((result = conn->executeQuery(query, dbResult)) != ERR_OK)
        {
            FATAL_LOG("[AssociationListsSlaveImp].onUserSessionFirstLogin: Failed to lookup external users for the new login, " << getDbErrorString(result));
            return;
        }

        if (dbResult->size() <= 0)
            continue;

        // Build the IN clause. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
        StringBuilder inClause("(");
        for (DbResultBase::const_iterator itr = dbResult->begin(), itrEnd = dbResult->end(); itr != itrEnd; ++itr)
        {
            inClause << (*itr)->getInt64("blazeid"); // get the blaze id:
            if (itr + 1 != itrEnd)
                inClause << ",";
        }
        inClause << ")";

        // Reset the hashes of all the users that had the guy in their external list:
        DB_QUERY_RESET(query);

        query->append("UPDATE user_association_list_size SET memberhash=0 WHERE blazeid IN $s AND assoclisttype=$u;", inClause.get(), listInfo->getId().getListType() );  //update our hash value
    
        if ((result = conn->executeQuery(query, dbResult)) != ERR_OK)
        {
            FATAL_LOG("[AssociationListsSlaveImp].onUserSessionFirstLogin: Failed to invalidate hashes in the user_association_list_size for the new login, " << getDbErrorString(result));
            return;
        }
    }

    return;
}

bool AssociationListsSlaveImpl::createOrReplaceTrigger(DbConnPtr conn, eastl::map<eastl::string, eastl::string>& activeTriggers, BlazeRpcError& result,
                                                       const char8_t* triggerName, const char8_t* triggerCondition, const char8_t* triggerAction, bool useVerbatimTriggerActionString)
{
    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    auto sizeCheckTrigger = activeTriggers.find(triggerName);
    bool createTrigger = false;
    if (sizeCheckTrigger != activeTriggers.end())
    {
        if (sizeCheckTrigger->second != triggerAction)
        {
            if (!gController->isDBDestructiveAllowed())
            {
                FATAL_LOG("[AssociationListsSlaveImp].replaceTrigger: Unable to update existing trigger "<< triggerName <<" due to --dbdestructive not being set!");
                return false;
            }

            DB_QUERY_RESET(query);

            // Note that triggerName is populated by the caller and does not contain any characters affected by the escapeString function.
            query->append("DROP TRIGGER IF EXISTS $s", triggerName);

            if ((result = conn->executeQuery(query)) != ERR_OK)
            {
                FATAL_LOG("[AssociationListsSlaveImp].replaceTrigger: Failed to drop trigger "<< triggerName <<", " << getDbErrorString(result));
                return false;
            }

            createTrigger = true;
        }
    }
    else
    {
        createTrigger = true;
    }

    if (createTrigger)
    {
        DB_QUERY_RESET(query);

        // Note that triggerName and triggerCondition are populated by the caller and do not contain any characters affected by the escapeString function.
        query->append("CREATE TRIGGER $s $s ", triggerName, triggerCondition);

        // Trigger action potentially contains the string literal "'45000'" so the string cannot be escaped every time.
        if (useVerbatimTriggerActionString)
        {
            query->appendVerbatimString(triggerAction, true /*skipStringChecks*/, true /*allowDestructiveKeywords*/);
        }
        else
        {
            query->append("$s", triggerAction);
        }

        if ((result = conn->executeQuery(query)) != ERR_OK)
        {
            FATAL_LOG("[AssociationListsSlaveImp].replaceTrigger: Failed to create trigger " << triggerName << ", " << getDbErrorString(result));
            return false;
        }
    }
    return true;
}

void AssociationListsSlaveImpl::createOrCheckTables(BlazeRpcError& result)
{
    result = ERR_OK;

    //Create a new entry for each list type we have.
    uint32_t dbId = getDbId();
    DbConnPtr conn = gDbScheduler->getConnPtr(dbId);
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationListsSlaveImp].createOrCheckTables: Could not get DB conn" );
        result = ERR_DB_SYSTEM;
        return;
    }

    // We expect all Triggers to be uniquely named (for each blazeserver):
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    DbResultPtr triggerResultPtr;
    query->append("SELECT TRIGGER_NAME, ACTION_STATEMENT FROM INFORMATION_SCHEMA.TRIGGERS WHERE TRIGGER_SCHEMA = '$s'", gDbScheduler->getConfig(dbId)->getDatabase());
    if ((result = conn->executeQuery(query, triggerResultPtr)) != ERR_OK)
    {
        FATAL_LOG("[AssociationListsSlaveImp].createOrCheckTables: Failed to get a list of triggers from DB, " << getDbErrorString(result));
        return;
    }
    
    eastl::map<eastl::string, eastl::string> activeTriggers;
    for (const DbRow* resultRow : *triggerResultPtr)
    {
        uint32_t nameColumnNum = 0;
        uint32_t actionColumnNum = 1;
        activeTriggers[resultRow->getString(nameColumnNum)] = resultRow->getString(actionColumnNum);
    }

    // This is the size check trigger. If the new list size is greater than the max list size, this trigger 
    // signals an error and returns the assoclisttype of the offending table in the error information. 
    // The error is defined with SQLSTATE '45000' to indicate an unhandled user-defined exception.
    // The triggerAction must be inserted verbatim due to use of the ' character.
    if (!createOrReplaceTrigger(conn, activeTriggers, result,
        "user_association_list_size_check", // triggerName
        "BEFORE UPDATE ON user_association_list_size FOR EACH ROW", // triggerCondition
        // triggerAction:
        "BEGIN"
        "       DECLARE sizeError CONDITION FOR SQLSTATE '45000';"
        "       IF NEW.listsize > NEW.maxsize THEN"
        "           SET @msg := CONVERT(NEW.assoclisttype, CHAR(50));"
        "           SIGNAL sizeError"
        "               SET MESSAGE_TEXT = @msg;"
        "       END IF;"
        "    END",
        true /* useVerbatimTriggerActionString*/))
    {
        return;
    }


    for (ListInfoPtrVector::iterator it = mListTemplates.begin(); it != mListTemplates.end(); it++)
    {
        ListInfoPtr listInfo = *it;

        DB_QUERY_RESET(query);
        query->append(
            "CREATE TABLE IF NOT EXISTS `user_association_list_$s` (" 
            "`blazeid` BIGINT(20) unsigned NOT NULL,"             
            "`memberblazeid` BIGINT(20) unsigned NOT NULL DEFAULT '0'," 
            "`dateadded` BIGINT(20) unsigned NOT NULL DEFAULT '0',"
            "`attributes` INT(10) unsigned NOT NULL DEFAULT '0',"
            "`lastupdated` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "PRIMARY KEY (`blazeid`,`memberblazeid`) USING BTREE,",
            listInfo->getId().getListName());

        if (listInfo->getStatusFlags().getMutualAction())
        {
            query->append(
                "KEY `IDX_LIST_DATEADDED_BY_MEMBER_ID` (`memberblazeid`, `dateadded`) USING BTREE,"
                "KEY `IDX_LIST_DATEADDED_BY_LASTUPDATED_BY_MEMBER_ID` (`memberblazeid`, `lastupdated`, `dateadded`) USING BTREE,");
        }

        query->append(
            "KEY `IDX_LIST_DATEADDED_BY_ID` (`blazeid`,`dateadded`) USING BTREE,"
            "KEY `IDX_LIST_DATEADDED_BY_LASTUPDATED_BY_ID` (`blazeid`, `lastupdated`, `dateadded`) USING BTREE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8");

        if ((result = conn->executeQuery(query)) != ERR_OK)
        {
            FATAL_LOG("[AssociationListsSlaveImpl].createOrCheckTables: Failed to create table " << listInfo->getId().getListName() << " error, " << getDbErrorString(result));
            return;
        }

        // External id Table:  (not for mutual or paired lists)
        if (!listInfo->getStatusFlags().getMutualAction() && !listInfo->getStatusFlags().getPaired())
        {
            DB_QUERY_RESET(query);
            if (!gController->isSharedCluster())
            {
                bool usesExtString = gController->usesExternalString(gController->getDefaultClientPlatform());

                const char8_t* colName = usesExtString ? "string" : "id";
                query->append(
                    "CREATE TABLE IF NOT EXISTS `user_association_list_$s_ext$s` ("
                    "`blazeid` BIGINT(20) unsigned NOT NULL,",
                    listInfo->getId().getListName(),
                    colName);

                if (usesExtString)
                {
                    query->append(
                        "`externalstring` varchar(256) NOT NULL DEFAULT '',");
                }
                else
                {
                    query->append(
                        "`externalid` BIGINT(20) unsigned NOT NULL DEFAULT '0',");
                }

                query->append(
                    "PRIMARY KEY (`blazeid`,`external$s`) USING BTREE,"
                    "KEY `IDX_EXT_DATA` USING BTREE (`external$s`)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8",
                    colName, colName);
            }
            else
            {
                query->append(
                    "CREATE TABLE IF NOT EXISTS `user_association_list_$s_extshared` ("
                    "`blazeid` BIGINT(20) unsigned NOT NULL,"
                    "`platform` varchar(32) NOT NULL,"
                    "`externalid` BIGINT(20) unsigned NOT NULL DEFAULT '0',"
                    "`externalstring` varchar(256) NOT NULL DEFAULT '',"
                    "PRIMARY KEY (`blazeid`,`platform`,`externalid`,`externalstring`) USING BTREE,"
                    "KEY `IDX_EXT_ID` USING BTREE (`platform`,`externalid`),"
                    "KEY `IDX_EXT_STR` USING BTREE (`platform`,`externalstring`)"
                    ") ENGINE=InnoDB DEFAULT CHARSET=utf8",
                    listInfo->getId().getListName());
            }

            if ((result = conn->executeQuery(query)) != ERR_OK)
            {
                FATAL_LOG("[AssociationListsSlaveImp].createOrCheckTables: Failed to create external table for " << listInfo->getId().getListName() << " error, " << getDbErrorString(result));
                return;
            }
        }


        //The following code will alter the association list tables if they are missing the attributes column
        DbResultPtr resultPtr;
        DB_QUERY_RESET(query);
        query->append("SELECT COUNT(*) FROM INFORMATION_SCHEMA.`COLUMNS` WHERE table_schema='$s' AND table_name LIKE 'user_association_list_$s' AND COLUMN_NAME='attributes'", gDbScheduler->getConfig(dbId)->getDatabase(), listInfo->getId().getListName());
        if ((result = conn->executeQuery(query, resultPtr)) != ERR_OK)
        {
                FATAL_LOG("[AssociationListsSlaveImp].createOrCheckTables: Failed to query table to see if 'attributes' column exists " << listInfo->getId().getListName() << " error, " << getDbErrorString(result));
                return;
        }

        if (!resultPtr->empty())
        {
            const DbRow* row = *(resultPtr)->begin();
            uint32_t count = row->getInt("COUNT(*)");
            if (count == 0)
            {
                if (!gController->isDBDestructiveAllowed())
                {
                    FATAL_LOG("[AssociationListsSlaveImp].createOrCheckTables: Unable to update existing table " << listInfo->getId().getListName() << " due to --dbdestructive not being set!");
                    return;
                }

                //Table does not contain attributes column - add the attributes column and timestamp column to the table and external table.
                DB_QUERY_RESET(query);
                query->append(
                    "ALTER IGNORE TABLE `user_association_list_$s` " 
                    "ADD `attributes` INT(10) unsigned NOT NULL DEFAULT '0',"
                    "ADD `lastupdated` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP; ", 
                    listInfo->getId().getListName());

                if ((result = conn->executeMultiQuery(query)) != ERR_OK)
                {
                    FATAL_LOG("[AssociationListsSlaveImp].createOrCheckTables: Failed to add attributes and lastupdated column for " << listInfo->getId().getListName() << " error, " << getDbErrorString(result));
                    return;
                }
            }
        }
        
        // Build the triggerName, triggerCondition, and triggerAction strings. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
        StringBuilder insertTriggerName, deleteTriggerName, insertTriggerCondition, deleteTriggerCondition, insertTriggerAction, deleteTriggerAction;

        insertTriggerName << "user_association_list_"<< listInfo->getId().getListName() <<"_insert_trigger";
        deleteTriggerName << "user_association_list_" << listInfo->getId().getListName() << "_delete_trigger";

        insertTriggerCondition << "AFTER INSERT ON user_association_list_" << listInfo->getId().getListName() << " FOR EACH ROW ";
        deleteTriggerCondition << "AFTER DELETE ON user_association_list_" << listInfo->getId().getListName() << " FOR EACH ROW ";

        if (listInfo->getStatusFlags().getMutualAction())
        {
            insertTriggerAction << "BEGIN " <<
                "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(NEW.blazeid," << listInfo->getId().getListType() << ",1, " << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize+1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(NEW.memberblazeid," << listInfo->getId().getListType() << ",1, " << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize+1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                "END";

            deleteTriggerAction << "BEGIN " <<
                "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(OLD.blazeid," << listInfo->getId().getListType() << ",1, " << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize-1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(OLD.memberblazeid," << listInfo->getId().getListType() << ",1, " << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize-1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                "END";
        }
        else if (listInfo->getPairId())
        {
            //we only set a trigger on the primary list, not the secondary
            if (!listInfo->getStatusFlags().getPaired())
            {
                insertTriggerAction << "BEGIN " <<
                    "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(NEW.blazeid," << listInfo->getId().getListType() << ",1, " << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize+1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                    "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(NEW.memberblazeid," << listInfo->getPairId() << ",1, " << listInfo->getPairMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize+1,maxsize=" << listInfo->getPairMaxSize() << ",memberhash=0; " <<
                    "    INSERT INTO user_association_list_" << listInfo->getPairName() << " (blazeid, memberblazeid, dateadded) VALUES(NEW.memberblazeid, NEW.blazeid, NEW.dateadded); " <<
                    "END";

                deleteTriggerAction << "BEGIN " <<
                    "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(OLD.blazeid," << listInfo->getId().getListType() << ",1, " << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize-1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                    "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(OLD.memberblazeid," << listInfo->getPairId() << ",1, " << listInfo->getPairMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize-1,maxsize=" << listInfo->getPairMaxSize() << ",memberhash=0; " <<
                    "    DELETE FROM user_association_list_" << listInfo->getPairName() << " WHERE blazeid=OLD.memberblazeid AND memberblazeid=OLD.blazeid; " <<
                    "END";
            }
        }
        else
        {
            insertTriggerAction << "BEGIN "
                "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(NEW.blazeid," << listInfo->getId().getListType() << ",1, " << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize+1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                "END";

            deleteTriggerAction << "BEGIN " <<
                "    INSERT INTO user_association_list_size (blazeid, assoclisttype, listsize, maxsize, memberhash) VALUES(OLD.blazeid," << listInfo->getId().getListType() << ",0," << listInfo->getMaxSize() << ", 0) ON DUPLICATE KEY UPDATE listsize=listsize-1,maxsize=" << listInfo->getMaxSize() << ",memberhash=0; " <<
                "END";
        }

        //we only set a trigger on the primary list, not the secondary
        if (!(listInfo->getPairId() && listInfo->getStatusFlags().getPaired()))
        {
            if (!createOrReplaceTrigger(conn, activeTriggers, result, insertTriggerName.get(), insertTriggerCondition.get(), insertTriggerAction.get()))
            {
                return;
            }
            if (!createOrReplaceTrigger(conn, activeTriggers, result, deleteTriggerName.get(), deleteTriggerCondition.get(), deleteTriggerAction.get()))
            {
                return;
            }
        }
    }
}

void AssociationListsSlaveImpl::addPendingSubscribe(AssociationListMember& listMember)
{
    mPendingSubscribeMap.insert(eastl::make_pair(listMember.getBlazeId(), &listMember));
}

void AssociationListsSlaveImpl::removePendingSubscribe(AssociationListMember& listMember)
{
    ListMemberByBlazeIdRange range = mPendingSubscribeMap.equal_range(listMember.getBlazeId());
    for (; range.first != range.second; ++range.first)
    {
        if (range.first->second == &listMember)
        {
            mPendingSubscribeMap.erase(range.first);
            break;
        }
    }
}

} // Association
} // Blaze
