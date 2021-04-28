/*! ************************************************************************************************/
/*!
    \file associationlisthelper.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/database/dbscheduler.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/usersessionmanager.h"
#include "associationlists/associationlisthelper.h"
#include "associationlists/associationlistsslaveimpl.h"

namespace Blaze
{
namespace Association
{

///////////////////////////////////////////////////////////////////////////
// AssociationListMember
//
AssociationListMember::AssociationListMember(AssociationList &ownerList, const UserInfoPtr& user, const TimeValue& timeAdded, const ListMemberAttributes& attributes)
    : mOwnerList(ownerList),
     mUser(user),
     mSubscribed(false),
     mTimeAdded(timeAdded)
{
    if (mTimeAdded == 0)
    {
        mTimeAdded = TimeValue::getTimeOfDay();
    }
    mAttributes.setBits(attributes.getBits());
}

AssociationListMember::~AssociationListMember()
{
    unsubscribe();
}

void AssociationListMember::subscribe()
{
    if (mSubscribed)
        return;

    mSubscribed = true;

    mOwnerList.getComponent().addPendingSubscribe(*this);

    handleExistenceChanged(getBlazeId());
}

void AssociationListMember::unsubscribe()
{
    if (!mSubscribed)
        return;

    mSubscribed = false;
    BlazeId blazeId = getBlazeId();
    mOwnerList.getComponent().removePendingSubscribe(*this);
    handleExistenceChanged(blazeId);
}

void AssociationListMember::handleExistenceChanged(const BlazeId blazeId)
{
    if (mOwnerList.getOwnerPrimarySession() != nullptr)
    {
        // caching data about the ownerList for safe use in err log lines 
        const UserSessionId ownerPrimaryUserSessionId = mOwnerList.getOwnerPrimarySession()->getUserSessionId();
        const BlazeId ownerBlazeId = mOwnerList.getOwnerBlazeId();

        // handleExistenceChanged is called by the AssociationListMember dtor (via unsubscribe())
        // when the ownerPrimarySession goes extinct, causing false err logs below.
        if (gUserSessionManager->getSession(ownerPrimaryUserSessionId) == nullptr)
        {
            // No reason to proceed if the ownerPrimaryUserSessionId cannot be notified.
            return;
        }

        UserSessionId listMemberPrimaryUserSessionId = gUserSessionManager->getPrimarySessionId(blazeId);
        bool listMemberIsOnline = UserSession::isValidUserSessionId(listMemberPrimaryUserSessionId);

        UserStatus userStatus;
        userStatus.setBlazeId(blazeId);
        userStatus.getStatusFlags().setSubscribed(mSubscribed);
        userStatus.getStatusFlags().setOnline(listMemberIsOnline);
        gUserSessionManager->sendUserUpdatedToUserSessionById(ownerPrimaryUserSessionId, &userStatus);

        if (listMemberIsOnline)
        {
            // Diagnostic logging: did the state of the user session change since the initial checks?
            if ((gUserSessionManager->getSession(ownerPrimaryUserSessionId) == nullptr) || (mOwnerList.getOwnerPrimarySession() == nullptr))
            {
                ERR_LOG("[AssociationListMember].handleExistenceChanged: AssociationList for owner=" << ownerBlazeId
                    << " has had its primary user session =" << ownerPrimaryUserSessionId << " erased before updatingNotifiers for list member=" << blazeId << ".");
            }

            UserSessionIdList ids;
            ids.push_back(listMemberPrimaryUserSessionId);
            UserSession::updateNotifiers(ownerPrimaryUserSessionId, ids, (mSubscribed ? ACTION_ADD : ACTION_REMOVE));
        }
        else if (mSubscribed && mOwnerList.getInfo().getStatusFlags().getOfflineUED())
        {
            // Send down a snapshot of this user's UED, even though they are offline.
            UserSessionExtendedDataUpdate ued;
            ued.setSubscribed(true);
            ued.setBlazeId(blazeId);

            // Diagnostic logging: did the state of the user session change since the initial checks?
            if ((gUserSessionManager->getSession(ownerPrimaryUserSessionId) == nullptr) || (mOwnerList.getOwnerPrimarySession() == nullptr))
            {
                ERR_LOG("[AssociationListMember].handleExistenceChanged: AssociationList for owner=" << ownerBlazeId
                    << " has had its primary user session =" << ownerPrimaryUserSessionId << " erased before updating the UED update for list member=" << blazeId << ".");
            }

            // Retrieve latest UserInfo in case the data referenced by mUser has been invalidated by the user session change.
            UserInfoPtr userInfo;
            BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, userInfo);
            if (lookupError == ERR_OK && userInfo != nullptr)
            {
                BlazeRpcError err = gUserSessionManager->requestUserExtendedData(*userInfo, ued.getExtendedData());
                if (err == ERR_OK)
                {
                    // Diagnostic logging: did the state of the user session change since the initial checks?
                    if ((gUserSessionManager->getSession(ownerPrimaryUserSessionId) == nullptr) || (mOwnerList.getOwnerPrimarySession() == nullptr))
                    {
                        ERR_LOG("[AssociationListMember].handleExistenceChanged: AssociationList for owner=" << ownerBlazeId
                            << " has had its primary user session =" << ownerPrimaryUserSessionId << " erased after updating UED for list member=" << blazeId << ".");
                    }

                    gUserSessionManager->sendUserSessionExtendedDataUpdateToUserSessionById(ownerPrimaryUserSessionId, &ued);
                }
            }
            else
            {
                // Diagnostic logging:
                ERR_LOG("[AssociationListMember].handleExistenceChanged: UserInfo is not available for list member=" <<  blazeId << ".");
            }
        }
    }
}

void AssociationListMember::fillOutListMemberId(ListMemberId& listId, ClientPlatformType requestorPlatform) const
{
    fillOutListMemberId(mUser, mAttributes, listId, requestorPlatform);
}


void AssociationListMember::fillOutListMemberId(const UserInfoPtr& userInfo, const ListMemberAttributes& attr, ListMemberId& listId, ClientPlatformType requestorPlatform)
{
    UserInfo::filloutUserCoreIdentification(*userInfo, listId.getUser());
    UserSessionManager::obfuscate1stPartyIds(requestorPlatform, listId.getUser().getPlatformInfo());
    listId.setExternalSystemId(userInfo->getPlatformInfo().getClientPlatform());
    listId.getAttributes().setBits(attr.getBits());
}

void AssociationListMember::fillOutListMemberInfo(ListMemberInfo& info, ClientPlatformType requestorPlatform) const
{
    fillOutListMemberId(info.getListMemberId(), requestorPlatform);
    info.setTimeAdded(mTimeAdded.getSec());
}

void AssociationListMember::deleteIfUnreferenced(bool clearRefs)
{
    // When the last reference is destroyed, we use this to clear the reference:
    if( clearRefs )
        clear_references();

    if( is_unreferenced() )
        delete this;
}


///////////////////////////////////////////////////////////////////////////
// AssociationList
//
AssociationList::AssociationList(AssociationListsSlaveImpl& component, const ListInfo &listTemplate, BlazeId owner)
    : mComponent(component),
      mFullyLoaded(false),
      mPartiallyLoaded(false),
      mCurrentOffset(0),
      mTotalSize(0)     
{
    listTemplate.copyInto(mInfo);
    mInfo.setBlazeObjId(EA::TDF::ObjectId(AssociationListsSlave::COMPONENT_ID, mInfo.getId().getListType(), owner));
}

AssociationList::~AssociationList()
{     
}

BlazeRpcError AssociationList::loadFromDb(bool doSubscriptionCheck, const uint32_t maxResultCount, const uint32_t offset)
{
    if (mFullyLoaded)
    {
        //We've loaded everything for this list and are getting updates via the cache, skip this
        return ERR_OK;
    }

    //dump the existing cache
    clearMemberCache();

    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent.getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].loadFromDb: Unable to obtain a database connection.");
        return ERR_DB_SYSTEM;
    }

    if (maxResultCount == 0)
    {
        // If we requested a count of 0, just find the member count: 
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT `listsize` FROM `user_association_list_size` WHERE `blazeid`=$D AND `assoclisttype`=$u", getOwnerBlazeId(), getType());

        DbResultPtr dbResult;
        BlazeRpcError dbErr;
        if ((dbErr = conn->executeQuery(query, dbResult)) != ERR_OK)
        {
            ERR_LOG("[AssociationList].loadFromDb: Error occurred executing count query [" << getDbErrorString(dbErr) << "]");
            return dbErr;
        }
           
        mTotalSize = 0;
        if (!dbResult->empty())
        {  
            DbRow *row = *dbResult->begin();
            uint32_t col = 0;
            mTotalSize = (uint32_t)row->getUInt(col);          
        }
    }
    else
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        bool isCalcFoundRows = (maxResultCount < UINT32_MAX || offset > 0);

        if (!isMutual())
        {
            query->append("SELECT $s `blazeid`, `memberblazeid`, `dateadded`, `attributes` FROM `user_association_list_$s` WHERE `blazeid`=$D ORDER BY `dateadded`",
                (isCalcFoundRows) ? "SQL_CALC_FOUND_ROWS" : "", getName(), getOwnerBlazeId());
        }
        else
        {
            query->append("SELECT $s `blazeid`, `memberblazeid`, `dateadded`, `attributes` FROM `user_association_list_$s` WHERE `blazeid`=$D UNION "
                "SELECT `blazeid`, `memberblazeid`, `dateadded`, `attributes` FROM `user_association_list_$s` WHERE `memberblazeid`=$D "
                " ORDER BY `dateadded`", (isCalcFoundRows) ? "SQL_CALC_FOUND_ROWS" : "", getName(), getOwnerBlazeId(), getName(),getOwnerBlazeId());
        }

        if (isCalcFoundRows)
        {
            query->append(" LIMIT $u ", maxResultCount);
            if (offset > 0)
            {
                query->append(" OFFSET $u ", offset);
            } 
        }

        DbResultPtr dbResult;
        BlazeRpcError dbErr;
        if ((dbErr = conn->executeQuery(query, dbResult)) != ERR_OK)
        {
            ERR_LOG("[AssociationList].loadFromDb: Error occurred executing query [" << getDbErrorString(dbErr) << "]");
            return dbErr;
        }

        BlazeIdToUserInfoMap idsToUsers;
        if ((dbErr = getUsersForDbResult(dbResult, DbResultPtr(), idsToUsers)) != ERR_OK)
        {
            return dbErr;
        }

        for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
        {
            BlazeId id1 = (*itr)->getUInt64((uint32_t) 0);
            BlazeId id2 = (*itr)->getUInt64(1);
            TimeValue dateAdded = (*itr)->getInt64(2);
            ListMemberAttributes attr;
            attr.setBits((*itr)->getUInt(3));

            // add a new member to the assoc list
            BlazeIdToUserInfoMap::const_iterator userItr = idsToUsers.find((id1 != getOwnerBlazeId()) ? id1 : id2);
            if (userItr != idsToUsers.end())
            {
                addMemberToCache(userItr->second, dateAdded, doSubscriptionCheck, attr, getInfo().getStatusFlags().getPaired());
            }
        }

        if (isCalcFoundRows)
        {
            DB_QUERY_RESET(query);
            query->append("SELECT FOUND_ROWS()");
            if ((dbErr = conn->executeQuery(query, dbResult)) != ERR_OK)
            {
                ERR_LOG("[AssociationList].loadFromDb: Error occurred executing count query [" << getDbErrorString(dbErr) << "]");
                return dbErr;
            }
           
            if (dbResult->size() > 0)
            {
                DbRow *row = *dbResult->begin();
                uint32_t col = 0;
                mTotalSize = (uint32_t)row->getUInt(col);
            }
        }
        else 
        {
            mTotalSize = dbResult->size();
        }
    }

    //If we pulled in every member of the list, we're fully loaded so can return 
    //page lookups from cache.
    mFullyLoaded = (maxResultCount == UINT32_MAX && offset == 0);
    mPartiallyLoaded = !mFullyLoaded;
    mCurrentOffset = offset;

    return ERR_OK;
}


BlazeRpcError AssociationList::lockAndGetSize(DbConnPtr conn, BlazeId blazeId, ListType listType, size_t& listSize, bool cancelOnEmpty)
{
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    if(query == nullptr)
    {
        ERR_LOG("AssociationList.lockAndGetSize(): Unable to create a new query.");
        return ERR_DB_SYSTEM;
    }


    DbResultPtr sizeResult;
    DB_QUERY_RESET(query);
    query->append("SELECT `listsize` FROM `user_association_list_size` WHERE `blazeid`=$D AND `assoclisttype`=$u FOR UPDATE", blazeId, listType);
    DbError dbErr;    
    if ((dbErr = conn->executeQuery(query, sizeResult)) != ERR_OK)
    {
        ERR_LOG("AssociationList.lockAndGetSize(): failed to lock table size " << getDbErrorString(dbErr) << ".");      
        return ERR_DB_SYSTEM;
    }


    if (sizeResult->empty() && !cancelOnEmpty)
    {
        //this guy didn't exist, create it
        DB_QUERY_RESET(query);
        query->append("INSERT INTO `user_association_list_size` (`blazeid`, `assoclisttype`) VALUES ($D, $u)", blazeId, listType);                
        if ((dbErr = conn->executeQuery(query)) != ERR_OK && dbErr != DB_ERR_DUP_ENTRY) //we'll take dupe entry because someone is racing us to insert this table size.  no problemo.
        {
            ERR_LOG("AssociationList.lockAndGetSize(): failed to insert table size row" << getDbErrorString(dbErr) << ".");      
            return ERR_DB_SYSTEM;
        }

        //Now we can lock
        DB_QUERY_RESET(query);
        query->append("SELECT `listsize` FROM `user_association_list_size` WHERE `blazeid`=$D AND `assoclisttype`=$u FOR UPDATE", blazeId, listType);        
        if ((dbErr = conn->executeQuery(query, sizeResult)) != ERR_OK)
        {
            ERR_LOG("AssociationList.lockAndGetSize(): failed to lock table size " << getDbErrorString(dbErr) << ".");      
            return ERR_DB_SYSTEM;
        }

        if (sizeResult->empty())
        {
            //Well that's weird
            ERR_LOG("AssociationList.lockAndGetSize(): failed to lock table size after create" << getDbErrorString(dbErr) << ".");      
            return ERR_DB_SYSTEM;
        }
    }

    if (!sizeResult->empty())
    {  
        const DbRow* aRow = *sizeResult->begin();
        listSize = static_cast<uint32_t>(aRow->getInt("listsize"));            
    }
    else
    {
        listSize = 0;
    }

    return ERR_OK;
}

BlazeRpcError AssociationList::addMembers(UserInfoAttributesVector &users,  const ListMemberAttributes& mask, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform)
{
    if (getInfo().getStatusFlags().getPaired())
    {
        return ASSOCIATIONLIST_ERR_PAIRED_LIST_MODIFICATION_NOT_SUPPORTED;
    }

    if (users.empty())
    {
        return ERR_OK;
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent.getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].addMembers: Unable obtain database connection for list "
            "[name=" << getInfo().getId().getListName() << ",owner=" << getOwnerBlazeId() << "].");
        return ERR_DB_SYSTEM;
    }

    BlazeRpcError dbErr = conn->startTxn();
    if (dbErr != ERR_OK)
    {
        ERR_LOG("[AssociationLists].addMembers(): executing start txn: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }

    //From this point on any DB error will automatically roll back the DbConn as it goes out of scope
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    DbResultPtr deletedResult;
    if (getInfo().getStatusFlags().getRollover())
    {

        size_t ownerTableSize = 0;
        if (lockAndGetSize(conn, getOwnerBlazeId(), getType(), ownerTableSize, false) != ERR_OK)
        {
            ERR_LOG("[AssociationLists].addMembers(): failed to lock owner row " << getOwnerBlazeId() << ", error " << getDbErrorString(dbErr) << ".");
            conn->rollback();
            return ERR_DB_SYSTEM;
        }

        //if our additions will rollover some of our newly added rows, don't even add them in the first place.
        if (getInfo().getMaxSize() < users.size())
        {
            users.erase(users.begin(), &users.at(users.size() - getInfo().getMaxSize()));
        }

        //Do rollover
        if (ownerTableSize + users.size() > mInfo.getMaxSize())
        {
            size_t rolloverCount = ownerTableSize + users.size() - mInfo.getMaxSize();
            TRACE_LOG("[AssociationList].addMembers(): will do rollover for user " << getOwnerBlazeId() << " count " << rolloverCount);    

            //Get our oldest ids and remove them
            DB_QUERY_RESET(query);
            query->append("SELECT * FROM `user_association_list_$s` WHERE `blazeid`=$D ORDER BY `dateadded` ASC LIMIT $D;", getName(), getOwnerBlazeId(), rolloverCount);
            query->append("DELETE FROM `user_association_list_$s` WHERE `blazeid`=$D ORDER BY `dateadded` ASC LIMIT $D;", getName(), getOwnerBlazeId(), rolloverCount);
            DbResultPtrs deleteResults;
            if ((dbErr = conn->executeMultiQuery(query, deleteResults)) != ERR_OK)
            {
                ERR_LOG("[AssociationList].addMembers(): failed to do rollover lookup for user " << getOwnerBlazeId() << ", list " << getName()  << ", error: " << getDbErrorString(dbErr) << ".");        
                conn->rollback();
                return ERR_DB_SYSTEM;
            }

            deletedResult = deleteResults[0];
        }
    }


    //This is our fixed time reference point.  We use this point to divide the list into three sections - old entries, updated entries, and new entries.
    //By setting this time forward a fixed amount (the size of the user list), and subtracting from it if we just update into a record, we end up with 
    //existing non-updated records being set before this time, existing updated records being set between now and now + size(), and new records being set now+size() to now + size() *2
    TimeValue now = TimeValue::getTimeOfDay();

    DB_QUERY_RESET(query);
    query->append("INSERT INTO `user_association_list_$s` (`blazeid`, `memberblazeid`, `dateadded`, `attributes`) VALUES ", getName());

    uint64_t addTime = now.getMicroSeconds() + users.size();
    for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
    {
        query->append("($D,$D,$D,$u),", 
            !isMutual() ? getOwnerBlazeId() : (getOwnerBlazeId() < (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId()),
            !isMutual() ? (*itr).userInfo->getId() : (getOwnerBlazeId() >= (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId()),
            addTime++,
            (*itr).userAttributes.getBits() & mask.getBits());
    }
    query->trim(1); //trim the trailing comma

    if (getInfo().getStatusFlags().getRollover())
    {
        query->append(" ON DUPLICATE KEY UPDATE `dateadded`=VALUES(dateadded) - $D, `attributes`= (attributes&~($u))|(VALUES(attributes));", users.size(), mask.getBits());
    }
    else
    {
        query->append(";");
    }

    //after adding the list, select the new ones.
    if (!isMutual())
    {
        query->append("SELECT * FROM `user_association_list_$s` WHERE blazeid=$D AND dateadded >= $D ORDER BY `dateadded`", getName(), getOwnerBlazeId(), now.getMicroSeconds() + users.size());
    }
    else
    {
        query->append("SELECT * FROM `user_association_list_$s` WHERE (blazeid=$D OR memberblazeid=$D) AND dateadded >= $D ORDER BY `dateadded`",
            getName(), getOwnerBlazeId(), getOwnerBlazeId(), now.getMicroSeconds() + users.size());
    }

    DbResultPtrs addResults;
     if ((dbErr = conn->executeMultiQuery(query, addResults)) != ERR_OK)
    {
        if (dbErr == ERR_DB_USER_DEFINED_EXCEPTION)
        {
            if (mInfo.getPairId() != LIST_TYPE_UNKNOWN)
            {
                eastl::string pairId;
                pairId.sprintf("%d", mInfo.getPairId());
                if (blaze_strcmp(conn->getLastError(), pairId.c_str()) == 0)
                {
                    conn->rollback();
                    return ASSOCIATIONLIST_ERR_PAIRED_LIST_IS_FULL_OR_TOO_MANY_USERS;
                }
            }
            conn->rollback();
            return ASSOCIATIONLIST_ERR_LIST_IS_FULL_OR_TOO_MANY_USERS;
        }
        else if (dbErr == DB_ERR_DUP_ENTRY)
        {
            conn->rollback();
            return ASSOCIATIONLIST_ERR_MEMBER_ALREADY_IN_THE_LIST;
        }

        ERR_LOG("[AssociationList].addMembers(): adding members failed query: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }
   
    if ((dbErr = conn->commit()) != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImpl].addMembers(): unable to commit transaction: " << getDbErrorString(dbErr) << ".");
    }
    
    return finishAddRemove(deletedResult, DbResultPtr(), addResults[1], updateResponse, requestorPlatform);
}

BlazeRpcError AssociationList::setMembersAttributes(UserInfoAttributesVector &users,  const ListMemberAttributes& mask, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform)
{
    if (getInfo().getStatusFlags().getPaired())
    {
        return ASSOCIATIONLIST_ERR_PAIRED_LIST_MODIFICATION_NOT_SUPPORTED;
    }

    if (users.empty())
    {        
        return ERR_OK;
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent.getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].setMembersAttributes: Unable to obtain database connection for list "
            "[name=" << getInfo().getId().getListName() << ",owner=" << getOwnerBlazeId() << "].");
        return ERR_DB_SYSTEM;
    }

    BlazeRpcError dbErr = conn->startTxn();
    if (dbErr != ERR_OK)
    {
        ERR_LOG("[AssociationLists].setMembersAttributes(): executing start txn: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    // Build the IN clause. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
    StringBuilder inClause("(");
    for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
    {
        inClause << (*itr).userInfo->getId() << ",";
    }
    inClause.trim(1);
    inClause << ")";

    query->append("SELECT COUNT(*) as count FROM `user_association_list_$s` WHERE (blazeid=$D AND memberblazeid IN $s)", getName(), getOwnerBlazeId(), inClause.get());

    if (isMutual())
    {
        query->append(" OR (memberblazeid=$D AND blazeid IN $s)", getOwnerBlazeId(), inClause.get());
    }

    query->append(";");

    query->append("SELECT NOW();");
    
    DbResultPtrs queryResults;
    if ((dbErr = conn->executeMultiQuery(query, queryResults)) != ERR_OK)
    {
        ERR_LOG("[AssociationList].setMembersAttributes(): failed to fetch count for " << getName() << " and/or retrieve timestamp; failed query: " << getDbErrorString(dbErr) << ".");
        conn->rollback();
        return ERR_DB_SYSTEM;
    }

    if (!queryResults[0]->empty())
    {
        const DbRow* row = *(queryResults[0])->begin();
        uint32_t refCount = row->getInt("count");
        if (refCount != users.size())
        {
            conn->rollback();
            return ASSOCIATIONLIST_ERR_USER_NOT_FOUND;
        }
    }

    const char* lastupdated = "";
    if (!queryResults[1]->empty())
    {
        const DbRow* row = *(queryResults[1])->begin();
        lastupdated = row->getString("NOW()");
    }

    DB_QUERY_RESET(query);
    
    size_t addedQueryResultIndex = 0;
    for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
    {
        query->append("UPDATE `user_association_list_$s` SET attributes=((attributes&~($u))|$u) WHERE blazeid=$D AND memberblazeid=$D;",
                        getName(),
                        mask.getBits(),
                        (*itr).userAttributes.getBits() & mask.getBits(),
                        !isMutual() ? getOwnerBlazeId() : (getOwnerBlazeId() < (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId()),
                        !isMutual() ? (*itr).userInfo->getId() : (getOwnerBlazeId() >= (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId())
                        );
        addedQueryResultIndex++;
    }

    //after adding the list, select the new ones.
    if (!isMutual())
    {
        query->append("SELECT * FROM `user_association_list_$s` WHERE blazeid=$D AND lastupdated>='$s' ORDER BY `dateadded`", getName(), getOwnerBlazeId(), lastupdated);
    }
    else
    {
        query->append("SELECT * FROM `user_association_list_$s` WHERE (blazeid=$D OR memberblazeid=$D) AND lastupdated>='$s' ORDER BY `dateadded`", getName(), getOwnerBlazeId(), getOwnerBlazeId(), lastupdated);
    }

    DbResultPtrs addResults;
    if ((dbErr = conn->executeMultiQuery(query, addResults)) != ERR_OK)
    {
        ERR_LOG("[AssociationList].setMembersAttributes(): failed to update attributes for members: " << getDbErrorString(dbErr) << ".");
        conn->rollback();
        return ERR_DB_SYSTEM;
    }
   
    if ((dbErr = conn->commit()) != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImpl].setMembersAttributes(): unable to commit transaction: " << getDbErrorString(dbErr) << ".");
    }
    
    return finishAddRemove(DbResultPtr(), DbResultPtr(), addResults[addedQueryResultIndex], updateResponse, requestorPlatform);
}

BlazeRpcError AssociationList::setExternalMembers(const ListMemberIdVector& extMembers, DbConnPtr& conn)
{
    if (getInfo().getStatusFlags().getMutualAction() || getInfo().getStatusFlags().getPaired())
    {
        // These list types don't have the external ids stored
        return ERR_OK;
    }

    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].setMembers: Unable obtain database connection for list "
            "[name=" << getInfo().getId().getListName() << ",owner=" << getOwnerBlazeId() << "].");
        return ERR_DB_SYSTEM;
    }

    BlazeRpcError dbErr = ERR_OK;
    const char8_t* tableSuffix = "shared";
    if (!gController->isSharedCluster())
        tableSuffix = gController->usesExternalString(gController->getDefaultClientPlatform()) ? "string" : "id";

    // Unlike the normal tables, there are no TRIGGERs associated with these DELETEs and INSERTs.
    // This implementation simply clears the current ext list, and inserts the new values. 
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("DELETE FROM `user_association_list_$s_ext$s` WHERE blazeid=$D;", getName(), tableSuffix, getOwnerBlazeId()); // delete our deleted items

    // Sanity check - at least one valid member:
    bool foundExternalData = false;
    for (ListMemberIdVector::const_iterator itr = extMembers.begin(), end = extMembers.end(); itr != end; ++itr)
    {
        if (has1stPartyPlatformInfo((*itr)->getUser().getPlatformInfo()))
        {
            foundExternalData = true;
            break;
        }
    }

    // If no ext members exist, then this just functions as a removal
    if (foundExternalData)
    {
        query->append("INSERT IGNORE INTO `user_association_list_$s_ext$s` (`blazeid`, ", getName(), tableSuffix);  // add our items

        if (!gController->isSharedCluster())
            query->append("`external$s`) VALUES ", tableSuffix);
        else
            query->append("`externalid`, `externalstring`, `platform`) VALUES ");

        for (ListMemberIdVector::const_iterator itr = extMembers.begin(), end = extMembers.end(); itr != end; ++itr)
        {
            ClientPlatformType platform = (*itr)->getUser().getPlatformInfo().getClientPlatform();
            bool usesExtString = gController->usesExternalString(platform);
            const char8_t* extString = usesExtString ? (*itr)->getUser().getPlatformInfo().getExternalIds().getSwitchIdAsTdfString() : "";
            ExternalId extId = usesExtString ? INVALID_EXTERNAL_ID : getExternalIdFromPlatformInfo((*itr)->getUser().getPlatformInfo());

            if (extString[0] == '\0' && extId == INVALID_EXTERNAL_ID)
			{
                eastl::string platformInfoStr;
                TRACE_LOG("[AssociationList].setMembers: No external identifiers to insert for user with platformInfo(" << platformInfoToString((*itr)->getUser().getPlatformInfo(), platformInfoStr) <<
                    "). List name=" << getInfo().getId().getListName() << ",owner=" << getOwnerBlazeId());
                continue;
            }

            if (gController->isSharedCluster())
                query->append("($D,$U,'$s','$s'),", getOwnerBlazeId(), extId, extString, ClientPlatformTypeToString(platform));
            else if (usesExtString)
                query->append("($D,'$s'),", getOwnerBlazeId(), extString);
            else
                query->append("($D,$U),", getOwnerBlazeId(), extId);
        }
        query->trim(1); //trim the trailing comma
        query->append(";");
    }


    DbResultPtrs dbResults;
    if ((dbErr = conn->executeMultiQuery(query, dbResults)) != ERR_OK)
    {
        ERR_LOG("[AssociationList].setExternalMembers(): add/delete queries failed: " << getDbErrorString(dbErr) << ".");      
        return ERR_DB_SYSTEM;
    }

    return ERR_OK;
}

BlazeRpcError AssociationList::setMembers(const UserInfoAttributesVector& users, const ListMemberIdVector& extMembers, MemberHash hashValue,  const ListMemberAttributes& mask, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform)
{
    if (getInfo().getStatusFlags().getPaired())
    {
        return ASSOCIATIONLIST_ERR_PAIRED_LIST_MODIFICATION_NOT_SUPPORTED;
    }

    if (users.size() > getInfo().getMaxSize())
    {
        return ASSOCIATIONLIST_ERR_LIST_IS_FULL_OR_TOO_MANY_USERS;
    }

    // Mutual lists don't have eternal members (ignore extMember values)
    if (users.empty() && (getInfo().getStatusFlags().getMutualAction() || getInfo().getStatusFlags().getPaired() || extMembers.empty()))
    {
        //empty set is the same operation as clear.  Don't waste our time doing the set mechanics below.
        return clearMembers(requestorPlatform, &updateResponse, hashValue);
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent.getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].setMembers: Unable obtain database connection for list "
            "[name=" << getInfo().getId().getListName() << ",owner=" << getOwnerBlazeId() << "].");
        return ERR_DB_SYSTEM;
    }

    BlazeRpcError dbErr = conn->startTxn();
    if (dbErr != ERR_OK)
    {
        ERR_LOG("[AssociationLists].setMembers(): executing start txn: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }

    QueryPtr query;
    DbResultPtrs dbResults;
    size_t deletedQueryResultIndex = 1, deletedQueryResultIndex2 = 1000, addedQueryResultIndex = 4; //These indicate where in the multiquery result to find the list of added and removed users.
    if (!users.empty())
    {
        //There's two paths for this query.  Since the set of users can potentially be very large, we optimize around sending data to the DB server
        //by installing it into a temporary table.  This means we only send the data up once, then operate in memory with table joins.  The cost of this is the creation
        //of the temporary table, which can be expensive while replication is in use.  So for smaller tables, we just use regular select statements.  
        //Either way the end of the multiquery should result in an add list and a remove list for our response.
        if (users.size() < mComponent.getConfig().getSetQueryLargeListThreshold())
        {
            //Build the WHERE clause for our deleted queries. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
            StringBuilder whereClause1("WHERE ");
            StringBuilder whereClause2("WHERE ");
            size_t ltCount(0), gtCount(0);

            if (isMutual())
            {
                // Build the IN clause. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
                StringBuilder ltIdsInClause("("), gtIdsInClause("(");
                for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
                {
                    if ((*itr).userInfo->getId() > getOwnerBlazeId())
                    {
                        gtIdsInClause << ((gtCount++ > 0) ? "," : "") << (*itr).userInfo->getId();
                    }
                    else
                    {
                        ltIdsInClause << ((ltCount++ > 0) ? "," : "") << (*itr).userInfo->getId();
                    }                
                }
                ltIdsInClause.append(")");
                gtIdsInClause.append(")");

                if (gtCount > 0)
                {
                    whereClause1 << "(blazeid=" << getOwnerBlazeId() << " AND memberblazeid NOT IN " << gtIdsInClause.get() << ")";
                }

                if (ltCount > 0)
                {
                    // SQL has horrid performance with OR + NOT IN (ends up checking everything), so we split into two statements instead.
                    StringBuilder& whereClauseRef = (gtCount > 0) ? whereClause2 : whereClause1;
                    whereClauseRef << "(memberblazeid=" << getOwnerBlazeId() << " AND blazeid NOT IN " << ltIdsInClause.get() << ")";
                }
            }
            else
            {
                whereClause1 << "blazeid=" << getOwnerBlazeId() << " AND memberblazeid NOT IN (";
                for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
                {
                    whereClause1 << ((itr != users.begin()) ? "," : "") << (*itr).userInfo->getId();
                }
                whereClause1.append(")");
            }


            query = DB_NEW_QUERY_PTR(conn);
            query->append("SELECT NOW();");
            DbResultPtr result;
            if ((dbErr = conn->executeQuery(query, result)) != ERR_OK)
            {
                ERR_LOG("[AssociationList].setMembers(): failed to retrieve timestamp; failed query: " << getDbErrorString(dbErr) << ".");
                conn->rollback();
                return ERR_DB_SYSTEM;
            }

            const char* lastupdated = "";
            if (!result->empty())
            {
                const DbRow* row = *result->begin();
                lastupdated = row->getString("NOW()");
            }

            DB_QUERY_RESET(query);

            query->append("INSERT INTO `user_association_list_size` (`blazeid`, `assoclisttype`) VALUES ($D, $u) ON DUPLICATE KEY UPDATE listsize=listsize;", getOwnerBlazeId(), getType());  //0 - lock the size table

            deletedQueryResultIndex = 1;

            // Note that whereClause1 is populated above and does not contain any characters affected by the escapeString function.
            query->append("SELECT * FROM `user_association_list_$s` $s;", getName(), whereClause1.get()); //1 grab our deleted items
            query->append("DELETE FROM `user_association_list_$s` $s; ", getName(), whereClause1.get()); //2 delete our deleted items

            if (isMutual() && gtCount > 0 && ltCount > 0)
            {
                deletedQueryResultIndex2 = 3;

                // Note that whereClause1 is populated above and does not contain any characters affected by the escapeString function.
                query->append("SELECT * FROM `user_association_list_$s` $s;", getName(), whereClause2.get()); //3 grab our deleted items
                query->append("DELETE FROM `user_association_list_$s` $s; ", getName(), whereClause2.get()); //4 delete our deleted items

                // Since there's 2 extra queries now, we have to move the result index:
                addedQueryResultIndex += 2;
            }

            query->append("INSERT IGNORE INTO `user_association_list_$s` (blazeid, memberblazeid, dateadded, attributes) VALUES ", getName());  //3 or 5 add our items
        
            uint64_t addTime = TimeValue::getTimeOfDay().getMicroSeconds();
            uint64_t currentAddTime = addTime;
            for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
            {
                query->append("($D,$D,$D,$u),", 
                    !isMutual() ? getOwnerBlazeId() : (getOwnerBlazeId() < (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId()),
                    !isMutual() ? (*itr).userInfo->getId() : (getOwnerBlazeId() >= (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId()),
                    currentAddTime++,
                    (*itr).userAttributes.getBits() & mask.getBits());
            }

            query->trim(1); //trim the trailing comma    
            query->append(" ON DUPLICATE KEY UPDATE `attributes`= (attributes&~($u))|(VALUES(attributes));", mask.getBits());

            if (!isMutual())
            {
                //4 after adding, select the new ones
                query->append("SELECT * FROM `user_association_list_$s` WHERE blazeid=$D AND lastupdated>='$s' ORDER BY `dateadded` ;", getName(), getOwnerBlazeId(), lastupdated);
            }
            else
            {
                //4 or 6 after adding, select the new ones
                query->append("SELECT * FROM `user_association_list_$s` WHERE (blazeid=$D OR memberblazeid=$D) AND lastupdated>='$s' ORDER BY `dateadded`;", getName(), getOwnerBlazeId(), getOwnerBlazeId(), lastupdated);
            }

            query->append("UPDATE user_association_list_size SET memberhash=$u WHERE blazeid = $D AND assoclisttype=$u;", hashValue, getOwnerBlazeId(), getType());  //5 or 7 update our hash value        
        }
        else
        {
            //From this point on any DB error will automatically roll back the DbConn as it goes out of scope
            query = DB_NEW_QUERY_PTR(conn);
            query->append("DROP TEMPORARY TABLE IF EXISTS `user_association_list_$s_temp`;"
                "CREATE TEMPORARY TABLE `user_association_list_$s_temp` (" 
                "blazeid BIGINT(20) unsigned NOT NULL,"             
                "memberblazeid BIGINT(20) unsigned NOT NULL DEFAULT '0'," 
                "dateadded BIGINT(20) unsigned NOT NULL DEFAULT '0',"
                "`attributes` INT(10) unsigned NOT NULL DEFAULT '0',"
                "`lastupdated` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
                "PRIMARY KEY (`blazeid`,`memberblazeid`) USING BTREE" 
                ") ENGINE=MEMORY DEFAULT CHARSET=utf8;"
                "INSERT INTO `user_association_list_$s_temp` (blazeid, memberblazeid, dateadded, attributes) VALUES ", getName(), getName(), getName());

            uint64_t addTime = TimeValue::getTimeOfDay().getMicroSeconds();
            for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
            {
                query->append("($D,$D,$D,$u),", 
                    !isMutual() ? getOwnerBlazeId() : (getOwnerBlazeId() < (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId()),
                    !isMutual() ? (*itr).userInfo->getId() : (getOwnerBlazeId() >= (*itr).userInfo->getId() ? getOwnerBlazeId() : (*itr).userInfo->getId()),
                    addTime++,
                    (*itr).userAttributes.getBits()&mask.getBits()
                    );
            }
            query->trim(1); //trim the trailing comma    
            query->append(";");
            if ((dbErr = conn->executeMultiQuery(query)) != ERR_OK)
            {
                ERR_LOG("[AssociationList].setMembers(): adding members failed query: " << getDbErrorString(dbErr) << ".");
                conn->rollback();
                return ERR_DB_SYSTEM;
            }

            //Get the deleted items
            DB_QUERY_RESET(query);
            query->append("INSERT INTO `user_association_list_size` (`blazeid`, `assoclisttype`) VALUES ($D, $u) ON DUPLICATE KEY UPDATE listsize=listsize;", getOwnerBlazeId(), getType());  //0 - lock the size table
            deletedQueryResultIndex = 1;
            query->append("SELECT t1.* FROM `user_association_list_$s` AS t1 LEFT JOIN `user_association_list_$s_temp` AS t2 ON t1.blazeid = t2.blazeid AND t1.memberblazeid = t2.memberblazeid", getName(), getName()); //1 grab our deleted items
            if (getInfo().getStatusFlags().getMutualAction())
            {
                query->append(" WHERE (t1.blazeid = $D OR t1.memberblazeid = $D) AND t2.blazeid IS NULL;", getOwnerBlazeId(), getOwnerBlazeId());
            }
            else
            {
                query->append(" WHERE t1.blazeid = $D AND t2.blazeid IS NULL;", getOwnerBlazeId());
            }

            query->append("DELETE t1 FROM `user_association_list_$s` AS t1 LEFT JOIN `user_association_list_$s_temp` AS t2 ON t1.blazeid = t2.blazeid AND t1.memberblazeid = t2.memberblazeid", getName(), getName()); //2 delete our deleted items
            if (getInfo().getStatusFlags().getMutualAction())
            {
                query->append(" WHERE (t1.blazeid = $D OR t1.memberblazeid = $D) AND t2.blazeid IS NULL;", getOwnerBlazeId(), getOwnerBlazeId());
            }
            else
            {
                query->append(" WHERE t1.blazeid = $D AND t2.blazeid IS NULL;", getOwnerBlazeId());
            }

            query->append("INSERT IGNORE INTO `user_association_list_$s` SELECT * FROM `user_association_list_$s_temp` AS t2 ON DUPLICATE KEY UPDATE `user_association_list_$s`.attributes = (`user_association_list_$s`.attributes&~($u))|t2.attributes ;", getName(), getName(), getName(), getName(), mask.getBits());  //3 add our items
            addedQueryResultIndex = 4; 
            query->append("SELECT t1.* FROM `user_association_list_$s` AS t1 RIGHT JOIN `user_association_list_$s_temp` AS t2 ON t1.blazeid = t2.blazeid AND t1.memberblazeid = t2.memberblazeid", getName(), getName()); //4 grab our added items
            if (getInfo().getStatusFlags().getMutualAction())
            {
                query->append(" WHERE (t2.blazeid = $D OR t2.memberblazeid = $D) ORDER BY `dateadded`;", getOwnerBlazeId(), getOwnerBlazeId());
            }
            else
            {
                query->append(" WHERE t2.blazeid = $D ORDER BY `dateadded`;", getOwnerBlazeId());
            }

            query->append("UPDATE user_association_list_size SET memberhash=$u WHERE blazeid = $D AND assoclisttype=$u;", hashValue, getOwnerBlazeId(), getType());  //5 update our hash value
            query->append("DROP TEMPORARY TABLE `user_association_list_$s_temp`", getName()); //6 drop the temp table
        }

        if ((dbErr = conn->executeMultiQuery(query, dbResults)) != ERR_OK)
        {
            if (dbErr == ERR_DB_USER_DEFINED_EXCEPTION)
            {
                if (mInfo.getPairId() != LIST_TYPE_UNKNOWN)
                {
                    eastl::string pairId;
                    pairId.sprintf("%d", mInfo.getPairId());
                    if (blaze_strcmp(conn->getLastError(), pairId.c_str()) == 0)
                    {
                        conn->rollback();
                        return ASSOCIATIONLIST_ERR_PAIRED_LIST_IS_FULL_OR_TOO_MANY_USERS;
                    }
                }
                conn->rollback();
                return ASSOCIATIONLIST_ERR_LIST_IS_FULL_OR_TOO_MANY_USERS;
            }
            ERR_LOG("[AssociationList].setMembers(): add/delete queries failed: " << getDbErrorString(dbErr) << ".");
            conn->rollback();
            return ERR_DB_SYSTEM;
        }
    }
    else
    {
        query = DB_NEW_QUERY_PTR(conn);
        // Always set the hash value (even if only external members are specified).
        query->append("INSERT INTO `user_association_list_size` (`blazeid`, `assoclisttype`, `memberhash`) VALUES ($D, $u, $u) ON DUPLICATE KEY UPDATE memberhash=VALUES(memberhash);", getOwnerBlazeId(), getType(), hashValue);
        if ((dbErr = conn->executeMultiQuery(query, dbResults)) != ERR_OK)
        {
            ERR_LOG("[AssociationList].setMembers(): Setting the hash value failed: " << getDbErrorString(dbErr) << ".");
            conn->rollback();
            return ERR_DB_SYSTEM; 
        }
    }

    // Set any external members that may exist:
    BlazeRpcError err = setExternalMembers(extMembers, conn);
    if (err != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImpl].setMembers(): unable to setExternalMembers for association list.");
        conn->rollback();
        return err;
    }

    if ((dbErr = conn->commit()) != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImpl].setMembers(): unable to commit transaction: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }
    
    if (!users.empty() && deletedQueryResultIndex < dbResults.size() && addedQueryResultIndex < dbResults.size())
    {
        return finishAddRemove( dbResults.at(deletedQueryResultIndex), 
                                (deletedQueryResultIndex2 < dbResults.size()) ? dbResults.at(deletedQueryResultIndex2) : DbResultPtr(),
                                dbResults.at(addedQueryResultIndex), updateResponse, requestorPlatform);
    }

    return ERR_OK;
}

BlazeRpcError AssociationList::finishRemove(DbResultPtr deletedResult, UpdateListMembersResponse& updateResponse, BlazeIdToUserInfoMap& idsToUsers, ClientPlatformType requestorPlatform)
{
    for (DbResult::const_iterator itr = deletedResult->begin(), end = deletedResult->end(); itr != end; ++itr)
    {
        BlazeId id1 = (*itr)->getUInt64((uint32_t)0), id2 = (*itr)->getUInt64(1);
        BlazeId affectedMemberId = (id1 != getOwnerBlazeId()) ? id1 : id2;
        ListMemberAttributes attributes;
        attributes.setBits((*itr)->getUInt(3));

        //slight chance we could have rolled over the user cache or something bogus happened like a rename or login.  In any case,
        //this lookup will probably just hit usermanager cache anyways, and rather than search the list of userptrs above, just
        //use the already indexed usermanager table to look it up.
        BlazeIdToUserInfoMap::const_iterator userItr = idsToUsers.find(affectedMemberId);
        if (userItr == idsToUsers.end())
        {
            ERR_LOG("[AssociationList].setMembers(): looking up user after remove failed.  Invalidating cache." );
            return ERR_SYSTEM;
        }

        //deleted
        removeMemberFromCache(affectedMemberId);

        ListMemberId* memberId = updateResponse.getRemovedListMemberIdVector().pull_back();
        AssociationListMember::fillOutListMemberId(userItr->second, attributes, *memberId, requestorPlatform);
    }

    return ERR_OK;
}

BlazeRpcError AssociationList::finishAddRemove(DbResultPtr deletedResult, DbResultPtr deletedResult2, DbResultPtr addedResult, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform)
{
    updateResponse.setOwnerId(getOwnerBlazeId());
    updateResponse.setListType(getType());

    if (deletedResult != nullptr)
    {
        size_t deletedResult2Size = ((deletedResult2 != nullptr) ? deletedResult2->size() : 0);
        updateResponse.getRemovedListMemberIdVector().reserve(deletedResult->size() + deletedResult2Size);

        BlazeRpcError lookupErr;
        BlazeIdToUserInfoMap idsToUsers;
        if ((lookupErr = getUsersForDbResult(deletedResult, deletedResult2, idsToUsers)) != ERR_OK)
        {
            clearMemberCache();
            return lookupErr;
        }

        BlazeRpcError err = finishRemove(deletedResult, updateResponse, idsToUsers, requestorPlatform);
        if (err == ERR_OK && deletedResult2 != nullptr)
        {
            err = finishRemove(deletedResult2, updateResponse, idsToUsers, requestorPlatform);
        }

        if (err != ERR_OK)
        {
            clearMemberCache();
            return err;
        }
    }

    if (addedResult != nullptr)
    {    
        updateResponse.getListMemberInfoVector().reserve(addedResult->size());

        BlazeRpcError lookupErr;
        BlazeIdToUserInfoMap idsToUsers;
        if ((lookupErr = getUsersForDbResult(addedResult, DbResultPtr(), idsToUsers)) != ERR_OK)
        {
            clearMemberCache();
            return lookupErr;
        }

        for (DbResult::const_iterator itr = addedResult->begin(), end = addedResult->end(); itr != end; ++itr)
        {
            BlazeId id1 = (*itr)->getUInt64((uint32_t) 0), id2 = (*itr)->getUInt64(1);
            BlazeId affectedMemberId = (id1 != getOwnerBlazeId()) ? id1 : id2;
            TimeValue dateAdded = (*itr)->getUInt64(2);
            ListMemberAttributes attributes;
            attributes.setBits((*itr)->getUInt(3));

            BlazeIdToUserInfoMap::const_iterator userItr = idsToUsers.find(affectedMemberId);
            if (userItr == idsToUsers.end())
            {
                ERR_LOG("[AssociationList].setMembers(): looking up user after add failed: " << lookupErr);    
                clearMemberCache();
                return ERR_SYSTEM;
            }

            //push the added user into our results list            
            addMemberToCache(userItr->second, dateAdded, true, attributes, getInfo().getStatusFlags().getPaired());

            ListMemberInfo* member = updateResponse.getListMemberInfoVector().pull_back();
            AssociationListMember::fillOutListMemberId(userItr->second, attributes, member->getListMemberId(), requestorPlatform);
            member->setTimeAdded(dateAdded.getSec());
        }
    }

    sendUpdateNotifications(updateResponse);

    return ERR_OK;
}

BlazeRpcError AssociationList::removeMembers(const UserInfoAttributesVector& users, bool validateDeletion, UpdateListMembersResponse& updateResponse, ClientPlatformType requestorPlatform)
{
    if (getInfo().getStatusFlags().getPaired())
    {
        return ASSOCIATIONLIST_ERR_PAIRED_LIST_MODIFICATION_NOT_SUPPORTED;
    }

    if (users.empty())
    {
        return ERR_OK;
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent.getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationList].removeMembers: Unable obtain database connection for list "
            "[name=" << getInfo().getId().getListName() << ",owner=" << getOwnerBlazeId() << "].");
        return ERR_DB_SYSTEM;
    }   

    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("DELETE FROM `user_association_list_$s` WHERE (`blazeid`=$D AND `memberblazeid` IN ", getName(), getOwnerBlazeId());        

    // Build the IN clause. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
    StringBuilder inClause("(");
    for (UserInfoAttributesVector::const_iterator it = users.begin(), itend = users.end(); it != itend; ++it)
    {
        inClause << (*it).userInfo->getId();
        if (it + 1 != itend)
            inClause << ",";
    }
    inClause << ")";

    query->append("$s)", inClause.get());

    if (isMutual())
    {
        //we need to flip and do the other side too
        query->append(" OR (`memberblazeid`=$D AND `blazeid` IN $s)", getOwnerBlazeId(), inClause.get());
    }

    BlazeRpcError dbErr;
    if ((dbErr = conn->startTxn()) != ERR_OK)
    {
        ERR_LOG("[AssociationList].removeMembers(): failed to start txn: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }   

    DbResultPtr result;
    if ((dbErr = conn->executeQuery(query, result)) != ERR_OK)
    {
        ERR_LOG("[AssociationList].removeMembers(): removing members failed query: " << getDbErrorString(dbErr) << ".");
        conn->rollback();
        return ERR_DB_SYSTEM;
    }

    if (validateDeletion && result->getAffectedRowCount() != users.size())
    {
        //If we don't actually remove the number of members we thought we had, then this fails.
        conn->rollback();
        return ASSOCIATIONLIST_ERR_MEMBER_NOT_FOUND_IN_THE_LIST;
    }

    if ((dbErr = conn->commit()) != ERR_OK)
    {
        ERR_LOG("[AssociationList].removeMembers(): failed to commit txn: " << getDbErrorString(dbErr) << ".");      
        return ERR_DB_SYSTEM;
    }

    //go through the removed members, nuke them from cache, and fill out the notification.
    updateResponse.setOwnerId(getOwnerBlazeId());
    updateResponse.setListType(getType());
    for (UserInfoAttributesVector::const_iterator itr = users.begin(), end = users.end(); itr != end; ++itr)
    {
        removeMemberFromCache((*itr).userInfo->getId());

        ListMemberId* info = updateResponse.getRemovedListMemberIdVector().pull_back();
        AssociationListMember::fillOutListMemberId((*itr).userInfo, (*itr).userAttributes, *info, requestorPlatform);
    }

    sendUpdateNotifications(updateResponse);

    return ERR_OK;
}


BlazeRpcError AssociationList::clearMembers(ClientPlatformType requestorPlatform, UpdateListMembersResponse* updateResponse, MemberHash hashValue)
{
    if (getInfo().getStatusFlags().getPaired())
    {
        return ASSOCIATIONLIST_ERR_PAIRED_LIST_MODIFICATION_NOT_SUPPORTED;
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent.getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationLists].clearMembers(): failed to get conn.");
        return ERR_DB_SYSTEM;
    }

    BlazeRpcError dbErr;
    if ((dbErr = conn->startTxn()) != ERR_OK)
    {
        ERR_LOG("[AssociationLists].clearMembers(): failed executing start txn: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }

    //Lock the list we're clearing
    size_t listSize;
    if ((dbErr = lockAndGetSize(conn, getOwnerBlazeId(), getType(), listSize, true)) != ERR_OK)
    {
        ERR_LOG("[AssociationLists].clearMembers(): failed executing lock list: " << getDbErrorString(dbErr) << ".");
        conn->rollback();
        return ERR_DB_SYSTEM;
    }

    // Even if we're empty, we still need to update the list's hash value:
    // now that the list is locked, grab the newest entry
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    if (listSize != 0)
    {
        if (!isMutual())
        {
            query->append("SELECT * FROM `user_association_list_$s` WHERE `blazeid`=$D;", getName(), getOwnerBlazeId());
            query->append("DELETE FROM `user_association_list_$s` WHERE `blazeid`=$D;", getName(), getOwnerBlazeId());
        }
        else
        {
            query->append("SELECT * FROM `user_association_list_$s` WHERE `blazeid`=$D OR `memberblazeid`=$D;", getName(), getOwnerBlazeId(), getOwnerBlazeId());
            query->append("DELETE FROM `user_association_list_$s` WHERE `blazeid`=$D OR `memberblazeid`=$D;", getName(), getOwnerBlazeId(), getOwnerBlazeId());
        }
    }

    // When we clear the list, either clear or maintain the hash (list entry should already exist via lockAndGetSize())
    query->append("UPDATE user_association_list_size SET memberhash=$u WHERE blazeid = $D AND assoclisttype=$u;", hashValue, getOwnerBlazeId(), getType());  //update our hash value

    //We checked against the list  size above, and with the lock it should be impossible to not have an entry here
    DbResultPtrs results;
    if ((dbErr = conn->executeMultiQuery(query, results)) != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImpl].clearMembers(): delete query failed: " << getDbErrorString(dbErr) << ".");
        conn->rollback();
        return ERR_DB_SYSTEM;
    }

    if ((dbErr = conn->commit()) != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImpl].clearMembers(): unable to commit transaction: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }

    // Only need to finish up if we actually deleted something (rather than just updating the hash)
    if (listSize != 0)
    {
        UpdateListMembersResponse notification;
        return finishAddRemove(results[0], DbResultPtr(), DbResultPtr(), (updateResponse != nullptr) ? *updateResponse : notification, requestorPlatform);
    }

    return ERR_OK;
}




BlazeRpcError AssociationList::getMemberHash(MemberHash& hashValue)
{
    hashValue = 0;

    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent.getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[AssociationLists].clearMembers(): failed to get conn.");
        return ERR_DB_SYSTEM;
    }

    BlazeRpcError dbErr;
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SELECT `memberhash` FROM `user_association_list_size` WHERE `blazeid`=$D AND `assoclisttype`=$u", getOwnerBlazeId(), getType());
    
    DbResultPtr result;
    if ((dbErr = conn->executeQuery(query, result)) != ERR_OK)
    {
        ERR_LOG("[AssociationListsSlaveImpl].getMemberHash(): failed db operation: " << getDbErrorString(dbErr) << ".");
        return ERR_DB_SYSTEM;
    }

    if (!result->empty())
    {
        hashValue = (*result->begin())->getUInt((uint32_t) 0);
    }

    return ERR_OK;
}


BlazeRpcError AssociationList::getUsersForDbResult(const DbResultPtr& dbResult, DbResultPtr dbResult2, BlazeIdToUserInfoMap& idsToUsers) const
{
    if (dbResult->empty())
        return ERR_OK;

    BlazeIdVector lookupIds;
    size_t dbResult2Size = ((dbResult2 != nullptr) ? dbResult2->size() : 0);
    lookupIds.reserve(dbResult->size() + dbResult2Size);
    for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
    {
        BlazeId id1 = (*itr)->getUInt64((uint32_t)0), id2 = (*itr)->getUInt64(1);
        lookupIds.push_back((id1 != getOwnerBlazeId()) ? id1 : id2);
    }
    if (dbResult2 != nullptr)
    {
        for (DbResult::const_iterator itr = dbResult2->begin(), end = dbResult2->end(); itr != end; ++itr)
        {
            BlazeId id1 = (*itr)->getUInt64((uint32_t)0), id2 = (*itr)->getUInt64(1);
            lookupIds.push_back((id1 != getOwnerBlazeId()) ? id1 : id2);
        }
    }

    BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeIds(lookupIds, idsToUsers);
    if (lookupError != ERR_OK)
    {
        //can't do anything without users, bail.
        ERR_LOG("[AssociationList].getUsersForDbResult: Error occurred looking up users " << ErrorHelp::getErrorName(lookupError));
        return lookupError;
    }

    return ERR_OK;
}

void AssociationList::sendUpdateNotifications(const UpdateListMembersResponse& notification)
{
    ListUpdated listUpdatedEvent;
    listUpdatedEvent.setBlazeId(getOwnerBlazeId());
    listUpdatedEvent.setName(getName());
    listUpdatedEvent.setType(getType());
    gEventManager->submitEvent(static_cast<uint32_t>(AssociationListsSlave::EVENT_LISTUPDATEDEVENT), listUpdatedEvent, true /*logBinaryEvent*/);

    if (gCurrentUserSession == nullptr || gCurrentUserSession->getBlazeId() != getOwnerBlazeId())        
    {
        //we are updating a list that's not ours.  Therefore the possibility exists that someone else has this list loaded, so we need to update
        if (getOwnerPrimarySession() != nullptr)
        {
            //the list primary session lives here, but we are not him.  Therefore send him an update right now.
            mComponent.sendNotifyUpdateListMembershipToUserSessionById(getOwnerPrimarySession()->getUserSessionId(), &notification);
        }
        else
        {
            //is this guy online on another slave?  if so toss an update to his slave.
            UserSessionId sessionId = gUserSessionManager->getPrimarySessionId(getOwnerBlazeId());
            if (sessionId != INVALID_USER_SESSION_ID)
            {
                mComponent.sendNotifyUpdateListMembershipSlaveToSliver(UserSession::makeSliverIdentity(sessionId), &notification);
            }
        }
    }

    //If this lists is paired or mutual, we need to remove it from any other caches.  Generate new notifications
    //for the flip of this operation.
    if (isMutual() || isInPair())
    {
        ListType otherListType = isMutual() ? getType() : mInfo.getPairId();    
       
        UserInfoPtr userInfo;
        BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(getOwnerBlazeId(), userInfo);
        if (lookupError == ERR_OK && userInfo != nullptr)
        {
            for (ListMemberIdVector::const_iterator itr = notification.getRemovedListMemberIdVector().begin(), end = notification.getRemovedListMemberIdVector().end(); itr != end; ++itr)
            {  
                UserSessionId sessionId = gUserSessionManager->getPrimarySessionId((*itr)->getUser().getBlazeId());
                if (sessionId != INVALID_USER_SESSION_ID)
                {
                    UpdateListMembersResponse otherNotification;
                    otherNotification.setOwnerId((*itr)->getUser().getBlazeId());
                    otherNotification.setListType(otherListType);

                    ListMemberId* info = otherNotification.getRemovedListMemberIdVector().pull_back();
                    AssociationListMember::fillOutListMemberId(userInfo, (*itr)->getAttributes(), *info, (*itr)->getUser().getPlatformInfo().getClientPlatform());

                    // This is a candidate for optimization. When # of usersessions becomes large, it's much more efficient to send a single 
                    // notification for all the usersessions to all hosting slaves and let them filter out the details instead of sending one at a time.
                    mComponent.sendNotifyUpdateListMembershipSlaveToSliver(UserSession::makeSliverIdentity(sessionId), &otherNotification);
                }
            }

            
            for (ListMemberInfoVector::const_iterator itr = notification.getListMemberInfoVector().begin(), end = notification.getListMemberInfoVector().end(); itr != end; ++itr)
            {
                UserSessionId sessionId = gUserSessionManager->getPrimarySessionId((*itr)->getListMemberId().getUser().getBlazeId());
                if (sessionId != INVALID_USER_SESSION_ID)
                {
                    UpdateListMembersResponse otherNotification;
                    otherNotification.setOwnerId((*itr)->getListMemberId().getUser().getBlazeId());
                    otherNotification.setListType(otherListType);

                    ListMemberInfo* info = otherNotification.getListMemberInfoVector().pull_back();
                    info->setTimeAdded((*itr)->getTimeAdded());
                    AssociationListMember::fillOutListMemberId(userInfo, (*itr)->getListMemberId().getAttributes(), info->getListMemberId(), (*itr)->getListMemberId().getUser().getPlatformInfo().getClientPlatform());

                    // This is a candidate for optimization. When # of usersessions becomes large, it's much more efficient to send a single 
                    // notification for all the usersessions to all hosting slaves and let them filter out the details instead of sending one at a time.
                    mComponent.sendNotifyUpdateListMembershipSlaveToSliver(UserSession::makeSliverIdentity(sessionId), &otherNotification);
                }
            }
        }
    }
}


/*! ************************************************************************************************/
/*! \brief add a member to the assoc list cache

    \param[in]userInfo - UserInfo of the member
    \param[in]timeAdded - time of the new member to be added
    \param[in]doSubscriptionCheck - Whether or not to do the subscription behavior.   
***************************************************************************************************/
void AssociationList::addMemberToCache(UserInfoPtr userInfo, const TimeValue& timeAdded, bool doSubscriptionCheck, const ListMemberAttributes& attr, bool isPairedList)
{      
    ListMemberByBlazeIdMap::iterator itr = mListMemberByBlazeIdMap.find(userInfo->getId());
    if (itr != mListMemberByBlazeIdMap.end())
    {
        //update the time and bail
        itr->second->setTimeAdded(timeAdded);
        if (!isPairedList)
        {
            itr->second->setAttributes(attr);
        }
        return;
    }
    
    ListMemberAttributes memberAttributes;
    if (!isPairedList)
    {
        memberAttributes.setBits(attr.getBits());
    }
    MemberPtr member(BLAZE_NEW AssociationListMember(*this, userInfo, timeAdded, memberAttributes));        

    mListMemberByBlazeIdMap.insert(ListMemberByBlazeIdMap::value_type(userInfo->getId(), member));
    mMemberList.push_back(member);    

    getComponent().dispatchAddUser(getInfo().getBlazeObjId(), member->getBlazeId()); 

    if (getOwnerPrimarySession() != nullptr)
    {
        //  this could be an passive addition to the owner's list, so the owner
        //  should add this list member to his session list.
        //getOwnerPrimarySession()->addUserToSession(getComponent(),  member->getBlazeId());
        BlazeIdList blazeIds;
        blazeIds.push_back(member->getBlazeId());
        UserSession::updateUsers(getOwnerPrimarySession()->getUserSessionId(), blazeIds, ACTION_ADD);
    }

    if (doSubscriptionCheck)
    {
        if (getInfo().getStatusFlags().getSubscribed())
        {
            member->subscribe();
        }
    } 
}


/*! ************************************************************************************************/
/*! \brief remove a member from the assoc list cache

    \param[in]memberId - blaze id of the new member to be removed
***************************************************************************************************/
void AssociationList::removeMemberFromCache(BlazeId memberId)
{
     MemberPtr member;
    
     ListMemberByBlazeIdMap::iterator itBlazeId = mListMemberByBlazeIdMap.find(memberId);
     if (itBlazeId != mListMemberByBlazeIdMap.end())
     {
         member = itBlazeId->second;
         mListMemberByBlazeIdMap.erase(itBlazeId);
     }

    if (member != nullptr)
    {
        for ( AssociationListMemberVector::iterator it = mMemberList.begin(), itend = mMemberList.end(); it != itend; ++it)
        {
            if (member.get() == (*it).get())
            {
                mMemberList.erase(it);
                break;
            }
        }

        member->unsubscribe();
        
        if (getOwnerPrimarySession() != nullptr)
        {
            //  this could be an passive removal from the owner's list, so the owner
            //  should add this list member to his session list.
            //getOwnerPrimarySession()->removeUserFromSession(getComponent(), member->getBlazeId());
            BlazeIdList blazeIds;
            blazeIds.push_back(member->getBlazeId());
            UserSession::updateUsers(getOwnerPrimarySession()->getUserSessionId(), blazeIds, ACTION_REMOVE);
        }

        getComponent().dispatchRemoveUser(getInfo().getBlazeObjId(), member->getBlazeId());
    }
}

void AssociationList::clearMemberCache()
{
    mFullyLoaded = false;
    mPartiallyLoaded = false;
    mCurrentOffset = 0;
    mTotalSize = 0;
    mMemberList.clear();
    mListMemberByBlazeIdMap.clear();
}

AssociationListMemberRef AssociationList::findMember(BlazeId blazeId) const
{
    if (blazeId != INVALID_BLAZE_ID)
    {
        ListMemberByBlazeIdMap::const_iterator it = mListMemberByBlazeIdMap.find(blazeId);
        if (it != mListMemberByBlazeIdMap.end())
        {
            return AssociationListMemberRef( (AssociationListMember *)(&(*it)) );
        }
    }

    return AssociationListMemberRef(nullptr);
}


void AssociationList::handleMemberUpdateNotification(const UpdateListMembersResponse &notification)
{
    //only update the cache if we've got it fully loaded.  Otherwise we just invalidate it
    if (mFullyLoaded)
    {
        for (ListMemberIdVector::const_iterator itr = notification.getRemovedListMemberIdVector().begin(), end =  notification.getRemovedListMemberIdVector().end(); itr != end; ++itr)
        {
            removeMemberFromCache((*itr)->getUser().getBlazeId());
        }

        bool isPairedList = getInfo().getStatusFlags().getPaired();
        for (ListMemberInfoVector::const_iterator itr =  notification.getListMemberInfoVector().begin(), end =  notification.getListMemberInfoVector().end(); itr != end; ++itr)
        {
            UserInfoPtr userInfo;
            BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId((*itr)->getListMemberId().getUser().getBlazeId(), userInfo);
            if (lookupError == ERR_OK && userInfo != nullptr)
            {
                addMemberToCache(userInfo, (*itr)->getTimeAdded(), true, (*itr)->getListMemberId().getAttributes(), isPairedList);
            }  
            else
            {
                ERR_LOG("[AssociationList].handleMemberUpdateNotification: invalid user id " << *itr << "in update for list owner " << notification.getOwnerId());
            }
        }
    }
    else
    {
        clearMemberCache();
    }


    if (getOwnerPrimarySession() != nullptr)
    {
        mComponent.sendNotifyUpdateListMembershipToUserSessionById(getOwnerPrimarySession()->getUserSessionId(), &notification);
    }
}

void AssociationList::subscribe()
{
    if ( ! mInfo.getStatusFlags().getSubscribed())
    {
        for (AssociationListMemberVector::iterator itr = mMemberList.begin(), end = mMemberList.end(); itr != end; ++itr)
        {
            (*itr)->subscribe();
        }
        mInfo.getStatusFlags().setSubscribed();
    }
}

void AssociationList::unsubscribe()
{
    if (mInfo.getStatusFlags().getSubscribed())
    {
        for (AssociationListMemberVector::iterator itr = mMemberList.begin(), end = mMemberList.end(); itr != end; ++itr)
        {
            (*itr)->unsubscribe();
        }

        mInfo.getStatusFlags().clearSubscribed();
    }
}

void AssociationList::deleteIfUnreferenced(bool clearRefs)
{
    // When the last reference is destroyed, we use this to clear the reference:
    if( clearRefs )
        clear_references();

    if( is_unreferenced() )
        delete this;
}


BlazeRpcError AssociationList::fillOutMemberInfoVector(ListMemberInfoVector& result, ClientPlatformType requestorPlatform, uint32_t offset, uint32_t count)
{
    //requested range is outside our desired range
    BlazeRpcError err = ERR_OK;
    if (!mFullyLoaded && (!mPartiallyLoaded || offset < mCurrentOffset || (offset + count > mCurrentOffset + mMemberList.size())))
    {
        //load up what we need from the DB
        if ((err = loadFromDb(false, count, offset)) != ERR_OK)
        {
            return err;
        }
    }

    if (mMemberList.empty())
    {
        //empty list, nothing to do
        return ERR_OK;
    }

    if (offset > mCurrentOffset + count)
    {
        //past the cache, bail, nothing to return
        return ERR_OK;
    }

    uint32_t actualOffset = offset;
    if (mPartiallyLoaded)
    {
        actualOffset -= mCurrentOffset;  
    }

    //figure out the lesser of the two sizes
    size_t actualCount = actualOffset + count < mMemberList.size() ? count : mMemberList.size() - actualOffset;

    result.reserve(actualCount);
    AssociationListMemberVector::const_iterator itr = mMemberList.begin(), end = mMemberList.end();
    eastl::advance(itr, actualOffset);
    for (; result.size() < actualCount && itr != end;  ++itr)
    {
        ListMemberInfo* memberInfo = result.pull_back();
        (*itr)->fillOutListMemberInfo(*memberInfo, requestorPlatform);
    }

    return ERR_OK;
}


}//Association
}//Blaze
