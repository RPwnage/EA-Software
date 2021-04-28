/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/session.h"
#include "framework/connection/selector.h"
#include "framework/event/eventmanager.h"
#include "framework/controller/controller.h"
#include "framework/connection/connectionmanager.h"
#include "framework/usersessions/usersession.h"


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

SlaveSession::~SlaveSession()
{
    triggerOnSlaveSessionRemoved();
}

void SlaveSession::triggerOnSlaveSessionRemoved()
{
    if (!mOnSlaveSessionRemovedTriggered)
    {
        mOnSlaveSessionRemovedTriggered = true;
        mDispatcher.dispatch<SlaveSession&>(&SlaveSessionListener::onSlaveSessionRemoved, *this);
    }
}

void SlaveSessionList::add(SlaveSession& session) 
{ 
    mSlaveSessions[session.getId()] = &session;
    session.addSessionListener(*this); 
}

void SlaveSessionList::onSlaveSessionRemoved(SlaveSession& session) 
{ 
    session.removeSessionListener(*this); 
    mSlaveSessions.erase(session.getId());
}

SlaveSessionList::iterator SlaveSessionList::find(const SlaveSession& session)
{
    return mSlaveSessions.find(session.getId());
}

SlaveSession* SlaveSessionList::getSlaveSession(SlaveSessionId slaveSessionId) const
{
    SlaveSessionsById::const_iterator iter = mSlaveSessions.find(slaveSessionId);
    if (iter != mSlaveSessions.end())
        return iter->second;
    return nullptr;
}

SlaveSession* SlaveSessionList::getSlaveSessionByInstanceId(InstanceId instanceId) const
{
    SlaveSessionId sid = SlaveSession::makeSlaveSessionId(instanceId, 0);
    SlaveSessionsById::const_iterator iter = mSlaveSessions.lower_bound(sid);
    if (iter != mSlaveSessions.end() && (instanceId == GetInstanceIdFromInstanceKey64(iter->first)))
        return iter->second;
    return nullptr;
}

void SlaveSession::sendNotification(Notification& notification)
{
    if (mConnection != nullptr)
        mConnection->sendNotification(notification);
}

/*** Private Methods ******************************************************************************/


} //namespace Blaze

