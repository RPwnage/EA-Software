/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SLIVER_H
#define BLAZE_SLIVER_H

/*** Include files *******************************************************************************/

#include "framework/system/fiberreadwritemutex.h"
#include "framework/system/fiberjobqueue.h"

#include "framework/tdf/slivermanagertypes_server.h"

#include "EASTL/intrusive_hash_map.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

struct NotificationInfo;
struct NotificationSendOpts;

class SliverOwner;
class OwnedSliver;
class Sliver;
typedef eastl::intrusive_ptr<Sliver> SliverPtr;

typedef eastl::vector_set<SliverId> SliverIdSet;

class Sliver :
    public SliverInfo
{
    NON_COPYABLE(Sliver);
public:
    static const int32_t HEALTH_CHECK_GRACE_PERIOD_MS;
    static const int32_t UNOWNED_GRACE_PERIOD_MS;

    struct AccessRef
    {
        NON_COPYABLE(AccessRef);
    public:
        AccessRef(bool attachToFiberAsResource = true) : mReadLockContext(attachToFiberAsResource) {}
        ~AccessRef() { release(); }

        void release() 
        {
            if (mSliver != nullptr)
            {
                SliverPtr tempSliver = mSliver;
                mSliver = nullptr;
                tempSliver->mAccessRWMutex.unlockForRead(mReadLockContext);
            }
        }

        SliverIdentity getSliverIdentity() const { return (mSliver != nullptr ? MakeSliverIdentity(mSliver->getSliverNamespace(), mSliver->getSliverId()) : INVALID_SLIVER_IDENTITY); }
        InstanceId getSliverInstanceId() const { return (mSliver != nullptr ? mSliver->getState().getSliverInstanceId() : INVALID_INSTANCE_ID); }
        bool hasSliver() const { return mSliver != nullptr; }

    private:
        friend class Sliver;
        SliverPtr mSliver;
        ReadLockContext mReadLockContext;

        void setSliver(Sliver& sliver)
        {
            if (mSliver == nullptr)
            {
                mSliver = &sliver;
            }
            else
            {
                EA_FAIL_MSG("AccessRef must be released directly before it can be acquired");
            }
        }
    };

    bool accessWouldBlock();
    BlazeRpcError waitForAccess(AccessRef& access, const char8_t* context);
    bool getPriorityAccess(AccessRef& access);

    SliverIdentity getSliverIdentity() const { return MakeSliverIdentity(mSliverNamespace, mSliverId); }
    const char8_t* getSliverNamespaceStr() const { return BlazeRpcComponentDb::getComponentNameById(getSliverNamespace()); }

    bool isOwnedByCurrentInstance() const;
    InstanceId getSliverInstanceId() const { return mState.getSliverInstanceId(); }
    const SliverState& getPendingStateUpdate() const { return mPendingStateUpdate; }

    void sendNotification(Notification& notification);

    void fillDebugInfo(StringBuilder& sb) const;
    bool isImportingToCurrentInstance() const;
    bool checkHealth(StringBuilder& healthReport) const;
    bool isUnhealthy() const;

    static bool isValidIdentity(SliverIdentity sliverIdentity);
    static bool isCompleteIdentity(SliverIdentity sliverIdentity);

private:
    friend class SliverManager;
    friend class SliverOwner;
    friend class OwnedSliver;
    friend void intrusive_ptr_add_ref(Sliver* ptr);
    friend void intrusive_ptr_release(Sliver* ptr);

    Sliver(SliverIdentity sliverIdentity);
    ~Sliver() override;

    SliverState& getPendingStateUpdate() { return mPendingStateUpdate; }
    BlazeError updateStateToPendingState();
    EA::TDF::TimeValue getLastStateUpdateTime() const { return mLastStateUpdateTime; }

    BlazeError waitForOwnerId(const char8_t* context);

    void fulfillSendNotificationAsync(NotificationPtr notification);
    void fulfillSendNotification(Notification& notification);

    void notificationQueueFiberStarted();
    void notificationQueueFiberFinished();

    void setCurrentImporter(OwnedSliver* currentImporter);

private:
    int32_t mRefCount;

    FiberReadWriteMutex mAccessRWMutex;

    Fiber::WaitList mWaitingForOwnerList;

    EA::TDF::TimeValue mNotificationQueueStartTime;  // Helper value for detecting queue startup stalls
    FiberJobQueue mNotificationQueue;
    AccessRef mNotificationAccess;

    SliverState mPendingStateUpdate;
    EA::TDF::TimeValue mLastStateUpdateTime;

    // This member is currently *only* used to indicate that this Sliver is is *becoming* owned by this instance.
    // This member could just as well be a bool that indicates this Sliver is being "imported".  But, a bool alone
    // isn't very useful.  So, this member will point to the OwnedSliver that is not *yet* the true owner.  This
    // intermediate state (e.g. I'm trying to import this sliver's objects from Redis) is, unfortunately, a "special case".
    // I very much dislike special cases, but this one is necessary to avoid improper handling of the sliver state while
    // trying to recover from a highly unstable state caused by network and/or cluster instability.
    // https://eadpjira.ea.com/browse/GOS-31857
    OwnedSliver* mCurrentImporter;
};

void intrusive_ptr_add_ref(Sliver* ptr);
void intrusive_ptr_release(Sliver* ptr);

class LogIdentity : public SbFormats::StringAppender
{
public:
    LogIdentity(SliverNamespace sliverNamespace, SliverId sliverId) : mSliverNamespace(sliverNamespace), mSliverId(sliverId) {}
    LogIdentity(const SliverInfo& sliver) : LogIdentity(sliver.getSliverNamespace(), sliver.getSliverId()) {}
    void append(StringBuilder& sb) const override;
private:
    SliverNamespace mSliverNamespace;
    SliverId mSliverId;
};

class LogState : public SbFormats::StringAppender
{
public:
    LogState(const SliverState& sliverState, const char8_t* kind) :
        mSliverInstanceId(sliverState.getSliverInstanceId()),
        mSliverRevisionId(sliverState.getRevisionId()),
        mReason(sliverState.getReason()),
        mKind(kind)
    {}
    virtual void append(StringBuilder& sb) const override;
private:
    const InstanceId mSliverInstanceId;
    const SliverRevisionId mSliverRevisionId;
    const BlazeRpcError mReason;
    const char8_t* mKind;
};

class LogCurrentState : public LogState
{
public:
    LogCurrentState(const Sliver& sliver) : LogState(sliver.getState(), "current") {}
};

class LogPendingState : public LogState
{
public:
    LogPendingState(const Sliver& sliver) : LogState(sliver.getPendingStateUpdate(), "pending") {}
};

} // Blaze

#endif // BLAZE_SLIVER_H
