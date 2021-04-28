/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SESSION_H
#define BLAZE_SESSION_H

/*** Include files *******************************************************************************/
#include "framework/component/notification.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/system/fiberlocal.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/util/dispatcher.h"
#include "EASTL/list.h"
#include "EASTL/map.h"

namespace Blaze
{

class UserSession;
class UserSessionMaster;
struct NotificationInfo;
class InboundRpcConnection;
class ServerInboundRpcConnection;
class SlaveSessionListener;
class Selector;

class SlaveSession
{
    NON_COPYABLE(SlaveSession);
public:
    static const SlaveSessionId SESSION_ID_MAX = INSTANCE_KEY_MAX_KEY_BASE_64;
    static const SlaveSessionId INVALID_SESSION_ID = BuildInstanceKey64(INVALID_INSTANCE_ID, 0);

public:
    SlaveSession(ServerInboundRpcConnection& connection, SlaveSessionId slaveSessionId) :
        mSlaveSessionId(slaveSessionId),
        mConnection(&connection),
        mOnSlaveSessionRemovedTriggered(false),
        mRefCount(0)
    {
    }

    virtual ~SlaveSession();

    SlaveSessionId getId() const { return mSlaveSessionId; }
    InstanceId getInstanceId() const { return GetInstanceIdFromInstanceKey64(mSlaveSessionId); }
    
    ServerInboundRpcConnection* getConnection() const { return mConnection; }

    void sendNotification(Notification& notification);

    void addSessionListener(SlaveSessionListener& listener) { mDispatcher.addDispatchee(listener); }
    void removeSessionListener(SlaveSessionListener& listener) { mDispatcher.removeDispatchee(listener); }

    virtual void AddRef() const { mRefCount++; }
    virtual void Release() const { if (--mRefCount <= 0) delete this; }

public:
    static inline SlaveSessionId makeSlaveSessionId(InstanceId instanceId, uint64_t base)
    {
        return BuildInstanceKey64(instanceId, base);
    }

    static inline SlaveSessionId makeSlaveSessionId(uint64_t context)
    {
        return (context & ~SESSION_ID_MAX);
    }

    static inline InstanceId getSessionIdInstanceId(SlaveSessionId id)
    {
        return GetInstanceIdFromInstanceKey64(id);
    }

    void triggerOnSlaveSessionRemoved();

private:
    friend class ConnectionManager;
    friend class ServerInboundRpcConnection;
    SlaveSessionId mSlaveSessionId;
    ServerInboundRpcConnection* mConnection;

    Dispatcher<SlaveSessionListener> mDispatcher;
    bool mOnSlaveSessionRemovedTriggered;

    mutable uint32_t mRefCount;
};

typedef eastl::intrusive_ptr<SlaveSession> SlaveSessionPtr;

class SlaveSessionListener
{
public:
    virtual ~SlaveSessionListener() {}

    virtual void onSlaveSessionRemoved(SlaveSession& session) = 0;
};

class SlaveSessionList : SlaveSessionListener
{
public:
    
    void add(SlaveSession& session);

    // SlaveSessionsById is not a hash map for 2 reasons:
    // 1) because lower 54 bits of session id are usually 0, causing excess collisions
    // 2) because we need to be able to find a SlaveSession by specifying a partial key (instanceId)
    typedef eastl::vector_map<SlaveSessionId, SlaveSession*> SlaveSessionsById;
    typedef SlaveSessionsById::reference reference;
    typedef SlaveSessionsById::const_reference const_reference;
    typedef SlaveSessionsById::iterator iterator;
    typedef SlaveSessionsById::const_iterator const_iterator;

    reference at(size_t index) { return mSlaveSessions.at(index); }
    iterator begin() { return mSlaveSessions.begin(); }
    iterator end() { return mSlaveSessions.end(); }

    const_reference at(size_t index) const { return mSlaveSessions.at(index); }
    const_iterator begin() const { return mSlaveSessions.begin(); } 
    const_iterator end() const { return mSlaveSessions.end(); }

    iterator find(const SlaveSession& session);
    bool contains(const SlaveSession& session) const { return mSlaveSessions.find(session.getId()) != mSlaveSessions.end(); }
    bool empty() const { return mSlaveSessions.empty(); }
    size_t size() const { return mSlaveSessions.size(); }
    SlaveSession* getSlaveSession(SlaveSessionId slaveSessionId) const;
    SlaveSession* getSlaveSessionByInstanceId(InstanceId instanceId) const;

    void onSlaveSessionRemoved(SlaveSession& session) override;

    BlazeRpcError waitForInstanceId(InstanceId instanceId);

private:
    SlaveSessionsById mSlaveSessions;
};

} //namespace Blaze

#endif  //BLAZE_USER_SESSION_H

