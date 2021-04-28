/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EpollSelector

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"

#ifndef EA_PLATFORM_WINDOWS

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <string.h>
#include "framework/connection/epollselector.h"
#include "framework/connection/channel.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/socketutil.h"
#include "framework/util/random.h"

namespace Blaze
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define MAKE_EPOLL_EVENT_DATA(slot,ident) ((((uint64_t)(slot)) << 32) | ((uint64_t)(ident)))
#define EPOLL_EVENT_DATA_SLOT(u64) (int32_t)(u64 >> 32)
#define EPOLL_EVENT_DATA_IDENT(u64) (int32_t)(u64 & 0xffffffff)
#define EPOLL_WAKE_EVENT_DATA MAKE_EPOLL_EVENT_DATA(-1, -1)

/*** Public Methods ******************************************************************************/
EpollSelector::EpollSelector()
    : mIsOpen(false),
    mPollFd(-1),
    mPriorityPollFd(-1),
    mPollEvents(nullptr),
    mSlots(nullptr),
    mFreeSlotHead(-1)
{
    mWakePipe[0] = -1;
    mWakePipe[1] = -1;
}

EpollSelector::~EpollSelector()
{
    close();
    delete[] mPollEvents;
    delete[] mSlots;
}

bool EpollSelector::initialize(const SelectorConfig &config)
{
    const bool rc = Selector::initialize(config);
    if (rc)
    {
        struct rlimit rlim;
        if (getrlimit(RLIMIT_NOFILE, &rlim) == 0 && getSelectorConfig().getMaxFds() == 0)
            getSelectorConfig().setMaxFds(rlim.rlim_cur);

        BLAZE_INFO_LOG(Log::SYSTEM, "[EpollSelector].initialize: max file descriptors set to " << getSelectorConfig().getMaxFds());

        mPollEvents = BLAZE_NEW_ARRAY(struct epoll_event, getSelectorConfig().getEpollEvents());
        mSlots = BLAZE_NEW_ARRAY(Slot, getSelectorConfig().getMaxFds());
        for(uint32_t idx = 0; idx < getSelectorConfig().getMaxFds(); ++idx)
        {
            mSlots[idx].mChannel = nullptr;
            mSlots[idx].mIdent = Channel::INVALID_IDENT;
            mSlots[idx].mNext = idx + 1;
        }
        mSlots[getSelectorConfig().getMaxFds() - 1].mNext = Channel::INVALID_SLOT;
        mFreeSlotHead = 0;
    }
    return rc;
}

bool EpollSelector::open()
{
    if (mIsOpen)
        return true;

    mPollFd = ::epoll_create(getSelectorConfig().getMaxFds());
    if (mPollFd < 0)
        return false;

    if (pipe(mWakePipe) < 0)
    {
        close();
        return false;
    }
    uint32_t nonblock = 1;
    ioctl(mWakePipe[0], FIONBIO, (char*)&nonblock);
    ioctl(mWakePipe[1], FIONBIO, (char*)&nonblock);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u64 = MAKE_EPOLL_EVENT_DATA(Channel::INVALID_SLOT, Channel::INVALID_IDENT);
    const int32_t rc = epoll_ctl(mPollFd, EPOLL_CTL_ADD, mWakePipe[0], &event);
    if (rc < 0)
    {
        close();
        return false;
    }

    // Create epoll file descriptor for polling priority channels
    mPriorityPollFd = ::epoll_create(getSelectorConfig().getMaxFds());
    if (mPriorityPollFd < 0)
    {
        close();
        return false;
    }

    mIsOpen = true;

    return true;
}

bool EpollSelector::close()
{
    if (mIsOpen)
    {
        ::close(mPollFd);
        ::close(mWakePipe[0]);
        ::close(mWakePipe[1]);
        ::close(mPriorityPollFd);
        mIsOpen = false;
    }
    return true;
}

/*** Protected Methods ***************************************************************************/

TimeValue EpollSelector::select()
{
    if (!mIsOpen)
        return 0;

    // Always process the priority channels first to ensure they get serviced regularly. We process both priority and regular descriptors to the configured network processing maximum rather than 
    // only process the regular descriptors in whatever time is left from network processing after processing the priority descriptors. This way, we don't starve regular descriptors if we are running over the threshold 
    // but we still log if combined usage exceeds the threshold. 
    TimeValue elapsedTime = selectOnFd(mPriorityPollFd, 0, mNetworkPriorityTasks, mNetworkPriorityPolls);

    mNetworkPriorityTime.increment(elapsedTime.getMicroSeconds());

    const TimeValue now = TimeValue::getTimeOfDay();
    const TimeValue timeout = getNextExpiry(now);

    const TimeValue elapsedNetworkTime = selectOnFd(mPollFd, timeout < 0 ? -1 : (int32_t)timeout.getMillis(), mNetworkTasks, mNetworkPolls);

    elapsedTime += elapsedNetworkTime;
    mNetworkTime.increment(elapsedNetworkTime.getMicroSeconds());
    return elapsedTime;
}

TimeValue EpollSelector::selectOnFd(int32_t epollFd, int32_t timeout, Metrics::Counter& networkTasks, Metrics::Counter& networkPolls)
{
    Fiber::pauseMainFiberTimingAt(TimeValue::getTimeOfDay());
    const int32_t count = ::epoll_wait(epollFd, mPollEvents, getSelectorConfig().getEpollEvents(), timeout);
    TimeValue startTime = TimeValue::getTimeOfDay();
    Fiber::resumeMainFiberTimingAt(startTime);
    if (count > 0)
    {
        int64_t tasks = 0;
        int64_t polls = 0;
        TimeValue networkProcessingThresholdTime = ((epollFd == mPriorityPollFd) ? getSelectorConfig().getMaxPriorityNetworkProcessingTime() : getSelectorConfig().getMaxNetworkProcessingTime());
        // process network up to the budget limit, then bail out
        for (int32_t idx = 0; (idx < count) && (TimeValue::getTimeOfDay() - startTime) < networkProcessingThresholdTime; ++idx)
        {
            auto& pollEvent = mPollEvents[idx];
            // IF epoll was woken via a wake() call
            if (EA_UNLIKELY(pollEvent.data.u64 == EPOLL_WAKE_EVENT_DATA))
            {
                // Wake FD; consume all bytes available
                uint8_t buf[256];
                int bytesRead;
                do
                {
                    bytesRead = read(mWakePipe[0], buf, sizeof(buf));
                } while (bytesRead > 0);
                mWakes.increment();
                continue;
            }

            const int32_t slot = EPOLL_EVENT_DATA_SLOT(pollEvent.data.u64);
            const int32_t ident = EPOLL_EVENT_DATA_IDENT(pollEvent.data.u64);

            // Ensure that the slot is still the right one in case somehow epoll still has
            // an entry for a socket that has already been closed.  Generally this will happen
            // if a socket is closed as part of processing a different socket but the closed
            // socket was already in our mPollEvents set for processing
            if (mSlots[slot].mIdent == ident)
            {
                Channel* channel = mSlots[slot].mChannel;
                ChannelHandler* handler = channel->getHandler();

                // Prioritize EPOLLIN first before EPOLLHUP since EPOLLHUP which may come together or even before we finish processing all EPOLLINs.
                // This is essential for graceful shutdown to close sockets when they are fully "closure acknowledged" by the remote.
                if (pollEvent.events & EPOLLIN)
                {
                    handler->onRead(*channel);
                    ++tasks;
                } 
                else if (pollEvent.events & EPOLLHUP)
                {
                    handler->onClose(*channel);
                    ++tasks;
                }
                else if (pollEvent.events & EPOLLERR)
                {
                    handler->onError(*channel);
                    ++tasks;
                }
                else if (pollEvent.events & EPOLLOUT)
                {
                    handler->onWrite(*channel);
                    ++tasks;
                }
            }
            ++polls;
        }
        networkTasks.increment(tasks);
        networkPolls.increment(polls);
    }
    return (TimeValue::getTimeOfDay() - startTime);
}

void EpollSelector::wake()
{
    if (gSelector != this && mIsOpen)
    {
        uint8_t w = 0;
        write(mWakePipe[1], &w, 1);
    }
}

bool EpollSelector::registerChannel(Channel& channel, uint32_t interestOps)
{
    if (channel.getSlot() != Channel::INVALID_SLOT)
    {
        // Channel already has a slot assignment so it must be registered already - just update the ops in case they've changed
        return updateOps(channel, interestOps);
    }

    if (mFreeSlotHead == Channel::INVALID_SLOT)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[EpollSelector].registerChannel: unable to register "
                "channel; no available slots.");
        return false;
    }

    // Pull a slot off the free list
    const int32_t slot = mFreeSlotHead;
    mFreeSlotHead = mSlots[mFreeSlotHead].mNext;
    
    // Add the channel to the registered slots
    mSlots[slot].mChannel = &channel;
    mSlots[slot].mIdent = channel.getIdent();
    mSlots[slot].mNext = Channel::INVALID_SLOT;
    channel.setSlot(slot);

    if (!registerChannelWithEpoll(channel, interestOps, slot, false))
        return false;

    // The priority FDs need to be in both sets. The epoll_wait() for priority fds have a 0 timeout value on them. However, the non-priority epoll_wait() call will block until either there is data on one of the sockets 
    // or the timeout expires. If we leave the priority fds off the non-priority poll set, then we won't wake up if data comes in on a priority fd while we're sleeping on the non-priority one.
    if (channel.isPriority())
    {
        if (!registerChannelWithEpoll(channel, interestOps, slot, true))
        {
            // Cleanup the registration from the normal epoll fd
            epoll_ctl(mPollFd, EPOLL_CTL_DEL, (int32_t)channel.getHandle(), nullptr);
            return false;
        }
    }

    return true;
}

bool EpollSelector::registerChannelWithEpoll(Channel& channel, uint32_t interestOps, int32_t slot, bool priority)
{
    const ChannelHandle handle = channel.getHandle();

    struct epoll_event event;
    event.events = EPOLLERR | EPOLLHUP;
    event.data.u64 = MAKE_EPOLL_EVENT_DATA(slot, channel.getIdent());
    if (interestOps & Channel::OP_READ)
        event.events |= EPOLLIN;
    if (interestOps & Channel::OP_WRITE)
        event.events |= EPOLLOUT;
    const int32_t rc = epoll_ctl(priority ? mPriorityPollFd : mPollFd, EPOLL_CTL_ADD, (int32_t)handle, &event);
    if (rc != 0)
    {
        char8_t errMsg[128];
        BLAZE_WARN_LOG(Log::CONNECTION, "[EpollSelector].registerChannelWithEpoll: failed to register " << (priority ? "priority " : "") << "channel in epoll set for channel=" << &channel << " fd=" << (int32_t)handle << ": " << SocketUtil::getErrorMsg(errMsg, sizeof(errMsg), errno)
                << " (" << errno<< ")");
        return false;
    }

    return true;
}

bool EpollSelector::unregisterChannel(Channel& channel, bool removeFd)
{
    const int32_t slot = channel.getSlot();
    channel.setSlot(Channel::INVALID_SLOT);

    if (slot == Channel::INVALID_SLOT)
        return false;

    if (mSlots[slot].mIdent != channel.getIdent())
        return false;

    mSlots[slot].mChannel = nullptr;
    mSlots[slot].mIdent = Channel::INVALID_IDENT;
    mSlots[slot].mNext = mFreeSlotHead;
    mFreeSlotHead = slot;

    if (removeFd)
    {
        // Remove the fd from the epoll selection set.
        epoll_ctl(mPollFd, EPOLL_CTL_DEL, (int32_t)channel.getHandle(), nullptr);

        if (channel.isPriority())
            epoll_ctl(mPriorityPollFd, EPOLL_CTL_DEL, (int32_t)channel.getHandle(), nullptr);
    }

    // Don't remove the fd from the epoll selection set.  The act of closing the fd will remove
    // it.  The fd may already have been recycled so removing it here could result in removing the
    // wrong entry.

    return true;
}

bool EpollSelector::updateOps(Channel& channel, uint32_t interestOps) const
{
    const int32_t slot = channel.getSlot();
    if (slot == Channel::INVALID_SLOT)
        return false;

    if (!updateEpollOps(mPollFd, channel, interestOps))
        return false;

    if (channel.isPriority())
    {
        if (!updateEpollOps(mPriorityPollFd, channel, interestOps))
            return false;
    }
    return true;
}

bool EpollSelector::updateEpollOps(int32_t epollFd, Channel& channel, uint32_t interestOps) const
{
    const int32_t slot = channel.getSlot();
    struct epoll_event event;
    event.events = EPOLLERR | EPOLLHUP;
    event.data.u64 = MAKE_EPOLL_EVENT_DATA(slot, channel.getIdent());
    if (interestOps & Channel::OP_READ)
        event.events |= EPOLLIN;
    if (interestOps & Channel::OP_WRITE)
        event.events |= EPOLLOUT;
    const int32_t rc = epoll_ctl(epollFd, EPOLL_CTL_MOD, (int32_t)channel.getHandle(), &event);

    char buf[128];
    if (rc != 0)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[EpollSelector].updateEpollOps: failed to update ops to " << interestOps 
                       << " for fd=" << (int32_t)channel.getHandle() << "; errno=" << errno <<":" << strerror_r(errno, buf, sizeof(buf)) << "; epollFd=" << epollFd);
        return false;
    }
    return true;
}

/*** Private Methods *****************************************************************************/

} // namespace Blaze

#endif // !EA_PLATFORM_WINDOWS

