/*************************************************************************************************/
/*!
    \file
        userset.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USER_SET_H
#define BLAZE_USER_SET_H

/*** Include files *******************************************************************************/
#include "framework/tdf/usersettypes_server.h"
#include "framework/rpc/usersetslave_stub.h"

namespace Blaze
{

class UserInfo;
class UserSession;

/*! *****************************************************************************/
/*! \class  UserSetSubscriber
    \brief  Abstract class to be implemented by any class wishing to provide UserSet Subscriber functionality 
            for a given EA::TDF::ObjectId type and BlazeId, userSession is user current session
 */
class UserSetSubscriber
{

public:
    virtual ~UserSetSubscriber() {}

    // required methods
    virtual void onAddSubscribe(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession* userSession = nullptr) = 0;
    virtual void onRemoveSubscribe(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession* userSession = nullptr) = 0;
};

class UserSetProvider
{
public:
    virtual ~UserSetProvider() {}

    // required methods

    /* \brief Find all UserSessionIds matching EA::TDF::ObjectId (fiber) */
    virtual BlazeRpcError getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids) = 0;
    
    /* \brief Find all UserIds matching EA::TDF::ObjectId (fiber) */
    virtual BlazeRpcError getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results) = 0;

    /* \brief Find all UserIds matching EA::TDF::ObjectId (fiber) */
    virtual BlazeRpcError getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& results) = 0;

    /* \brief Find at least one Session matching EA::TDF::ObjectId (fiber) */
    bool hasSession(const EA::TDF::ObjectId& bobjId, BlazeRpcError& result);

    /* \brief Find at least one User matching EA::TDF::ObjectId (fiber) */
    bool hasUser(const EA::TDF::ObjectId& bobjId, BlazeRpcError& result);

    /* \brief Check if the blazeId is exist in the given list (fiber) */
    bool containsUser(const EA::TDF::ObjectId& bobjId, const BlazeId& blazeId);

    /* \brief Count all Sessions matching EA::TDF::ObjectId (fiber) */
    virtual BlazeRpcError countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count) = 0;

    /* \brief Count all Users matching EA::TDF::ObjectId (fiber) */
    virtual BlazeRpcError countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count) = 0;

    /* \brief The provider will be called on to provide appropriate routing information to obtain the UserSet */
    virtual RoutingOptions getRoutingOptionsForObjectId(const EA::TDF::ObjectId& bobjId) = 0;

    /* \brief register subscriber function */
    void addSubscriber(UserSetSubscriber& subscriber);

    /* \brief unregister subscriber function */
    void removeSubscriber(UserSetSubscriber& subscriber);

    /* \brief dispatch add user function */
    void dispatchAddUser(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession* userSession = nullptr);

    /* \brief dispatch remove user function */
    void dispatchRemoveUser(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession* userSession = nullptr);

protected:
    Dispatcher<UserSetSubscriber> mUserSetDispatcher;
};

class UserSetManager : public UserSetSlaveStub
{
    friend class GetUserIdsCommand;
    friend class GetUserBlazeIdsCommand;
    friend class GetSessionIdsCommand;
    friend class CountUsersCommand;
    friend class CountSessionsCommand;

NON_COPYABLE(UserSetManager);

public:

    UserSetManager();
    ~UserSetManager() override;

    // \brief Find all UserSessionIds matching EA::TDF::ObjectId.
    BlazeRpcError DEFINE_ASYNC_RET(getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids));

    // \brief Find all Blaze Ids matching EA::TDF::ObjectId.
    BlazeRpcError DEFINE_ASYNC_RET(getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results));

    // \brief Find all UserIds matching EA::TDF::ObjectId.
    BlazeRpcError DEFINE_ASYNC_RET(getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& results));

    // \brief Count all Users matching EA::TDF::ObjectId
    BlazeRpcError DEFINE_ASYNC_RET(countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& userSetSize));

    // \brief Check if the blazeId is exist in the given list
    bool DEFINE_ASYNC_RET(containsUser(const EA::TDF::ObjectId& bobjId, const BlazeId& blazeId));

    // \brief Add a subcriber to the UserSetProvider (ONLY works for co-located components)
    BlazeRpcError DEFINE_ASYNC_RET(addSubscriberToProvider(ComponentId compId, UserSetSubscriber& subscriber));

    // \brief Remove a subcriber to the UserSetProvider (ONLY works for co-located components)
    BlazeRpcError DEFINE_ASYNC_RET(removeSubscriberFromProvider(ComponentId compId, UserSetSubscriber& subscriber));

    // \brief Register a UserSet provider for the given *local* component. Memory is owned by the caller.
    bool registerProvider(ComponentId compId, UserSetProvider& provider);

    // \brief Deregister a UserSet provider for the given *local* component.
    bool deregisterProvider(ComponentId compId);

    RoutingOptions DEFINE_ASYNC_RET(getProviderRoutingOptions(const EA::TDF::ObjectId& bobjId, bool remoteRpcCheck = true));

private:

    // \brief Find all UserSessionIds matching EA::TDF::ObjectId.
    BlazeRpcError getLocalSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids);

    // \brief Find all BlazeIds matching EA::TDF::ObjectId.
    BlazeRpcError getLocalUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& results);

    // \brief Find all UserIds matching EA::TDF::ObjectId.
    BlazeRpcError getLocalUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids);

    // \brief Count all UserSessions matching EA::TDF::ObjectId.
    BlazeRpcError countLocalSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count);

    // \brief Count all Users matching EA::TDF::ObjectId.
    BlazeRpcError countLocalUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count);


    typedef eastl::hash_map<ComponentId, UserSetProvider*> ProvidersByComponent;
    ProvidersByComponent mProviders;
};

extern EA_THREAD_LOCAL UserSetManager* gUserSetManager;
}

#endif //BLAZE_USER_SET_H
