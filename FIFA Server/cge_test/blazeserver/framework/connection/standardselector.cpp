/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/standardselector.h"
#include "framework/connection/channel.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/socketutil.h"
#include "framework/connection/inetaddress.h"
#include "EATDF/time.h"

//lint -e717  Disable do...while(0) for this file only since the winsock macros do it

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief default constructor
*/
/*************************************************************************************************/
StandardSelector::StandardSelector()
    : mNextSlot(0),
      mIsOpen(false),
      mProcessingChannels(false),
      mChannelMap(BlazeStlAllocator("StandardSelector::mChannelMap"))
{
    zeroAllFdSets();
}

/*************************************************************************************************/
/*!
    \brief destructor
*/
/*************************************************************************************************/
StandardSelector::~StandardSelector()
{
}

/*************************************************************************************************/
/*!
    \brief open

    Open the selector and make it ready to handle select() and selectNow() calls.

    \return - true if the open was successful, false if not
*/
/*************************************************************************************************/
bool StandardSelector::open()
{
    // Create a pair of connected sockets used for waking the select() call.

    mWakeSoks[0] = socket(AF_INET, SOCK_DGRAM, 0);
    mWakeSoks[1] = socket(AF_INET, SOCK_DGRAM, 0);

    mWakeAddr.sin_family = AF_INET;
    mWakeAddr.sin_port = 0;
    mWakeAddr.sin_addr.s_addr = htonl(0x7f000001);
    if (::bind(mWakeSoks[0], (struct sockaddr*)&mWakeAddr, sizeof(mWakeAddr)) == SOCKET_ERROR)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[StandardSelector].open: could not bind wake socket; err=" << WSAGetLastError());
        ::closesocket(mWakeSoks[0]);
        ::closesocket(mWakeSoks[1]);
        return false;
    }
    socklen_t slen = sizeof(mWakeAddr);
    if (::getsockname(mWakeSoks[0], (struct sockaddr*)&mWakeAddr, &slen) == SOCKET_ERROR)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[StandardSelector].open: could not get name for wake socket; err=" << WSAGetLastError());
        ::closesocket(mWakeSoks[0]);
        ::closesocket(mWakeSoks[1]);
        return false;
    }

    SocketUtil::setSocketBlockingMode(mWakeSoks[0], false);
    SocketUtil::setSocketBlockingMode(mWakeSoks[1], false);

    mIsOpen = true;
    return mIsOpen;
}

/*************************************************************************************************/
/*!
    \brief close

    Close the selector.  Calls to select() and selectNow() will always return false.

    \return - true if the close was successful, false if not
*/
/*************************************************************************************************/
bool StandardSelector::close()
{
    if (mIsOpen)
    {
        ::closesocket(mWakeSoks[0]);
        ::closesocket(mWakeSoks[1]);
        mIsOpen = false;
    }
    return mIsOpen;
}

/*** Protected Methods ***************************************************************************/

/*************************************************************************************************/
/*!
    \brief select

    Wait for an event on the registered channels for the specified timeout.

    \param[in]  timeout - Number of milliseconds to wait for an event
*/
/*************************************************************************************************/
TimeValue StandardSelector::select()
{
    if (!mIsOpen)
        return 0;

    TimeValue timeout = getNextExpiry(TimeValue::getTimeOfDay());
    struct timeval tval;
    struct timeval* tv;
    if (timeout == -1)
    {
        tv = nullptr;
    }
    else
    {
        tval.tv_sec = (int32_t)timeout.getSec();
        tval.tv_usec = (int32_t)((timeout.getMillis() % 1000) * 1000);
        tv = &tval;
    }

    prepareFdSets();
    Fiber::pauseMainFiberTimingAt(TimeValue::getTimeOfDay());
    int32_t numReady = ::select(
            (int32_t)(mMaxFd + 1), &mReadSet, &mWriteSet, &mExceptSet, tv);
    TimeValue startTime = TimeValue::getTimeOfDay();
    Fiber::resumeMainFiberTimingAt(startTime);
    if (numReady >= 0)
    {
        // Iterate all registered keys to find the ones that are actually ready
        processSelectedChannels();
        mNetworkTime.increment((TimeValue::getTimeOfDay() - startTime).getMicroSeconds());
    }
    return (TimeValue::getTimeOfDay() - startTime);
}

void StandardSelector::wake()
{
    if (gSelector != this && isOpen())
    {
        char w = 0;
        if (::sendto(mWakeSoks[1], &w, 1, 0, (struct sockaddr*)&mWakeAddr, sizeof(mWakeAddr))
                == SOCKET_ERROR)
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[StandardSelector].wake: failed to write to wake socket; err=" << WSAGetLastError());
        }
    }
}

/*************************************************************************************************/
/*!
    \brief registerChannel

    Register the given channel and create a new SelectionKey for it with the given interestOps
    and attachment.

    \return - true if the registration was successful.
*/
/*************************************************************************************************/
bool StandardSelector::registerChannel(Channel& channel, uint32_t interestOps)
{
    int32_t slot = channel.getSlot();
    if (slot == Channel::INVALID_SLOT)
    {
        slot = mNextSlot++;
        if (mNextSlot < 0)
            mNextSlot = 0;
        channel.setSlot(slot);
    }

    if (mProcessingChannels)
    {
        // Need to defer modifying the channel map because we're in the middle of iterating
        // through it.
        queueDeferredRegistration(REGISTER, channel, interestOps, Channel::INVALID_SLOT);
        return true;
    }

    // Check if the channel is already registered
    ChannelsBySlot::const_iterator channelIter = mChannelMap.find(slot);
    if (channelIter == mChannelMap.end() || channelIter->second == nullptr)
    {
        // Add the association to the map
        mChannelMap[slot] = &channel;
    }

    return true;
}

void StandardSelector::updateInterestOps(ChannelHandle handle, uint32_t interestOps)
{
    SOCKET h = static_cast<SOCKET>(handle);
    if (h == INVALID_SOCKET)
        return;

    if (interestOps != 0)
    {
        FD_SET(h, &mExceptSet);
        if(!FD_ISSET(h, &mExceptSet))
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[StandardSelector].updateInterestOps: "
                       "failed to add socket to error set; max(" << FD_SETSIZE << ") exceeded");
        }
    }

    // Add to each handle that it is not current added into
    if (interestOps & Channel::OP_READ)
    {
        FD_SET(h, &mReadSet);
        if(!FD_ISSET(h, &mReadSet))
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[StandardSelector].updateInterestOps: "
                       "failed to add socket to read set; max(" << FD_SETSIZE << ") exceeded");
        }
    }

    if (interestOps & Channel::OP_WRITE)
    {
        FD_SET(h, &mWriteSet);
        if(!FD_ISSET(h, &mWriteSet))
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[StandardSelector].updateInterestOps: "
                       "failed to add socket to write set; max(" << FD_SETSIZE << ") exceeded");
        }
    }

    if (interestOps != 0)
    {
        if (h > mMaxFd)
            mMaxFd = h;
    }
}

bool StandardSelector::unregisterChannel(Channel& channel, bool removeFd)
{
    int32_t slot = channel.getSlot();
    channel.setSlot(Channel::INVALID_SLOT);

    if (mProcessingChannels)
    {
        // Need to defer modifying the channel map structure because we're in the middle of iterating
        // through it.  But we can go ahead and clear the value
        ChannelsBySlot::iterator find = mChannelMap.find(slot);
        if (find != mChannelMap.end())
        {
            find->second = nullptr;
        }
        queueDeferredRegistration(UNREGISTER, channel, 0, slot);
        return true;
    }

    return unregisterChannel(slot);
}

bool StandardSelector::unregisterChannel(int32_t slot)
{
    // Forget about this channel
    ChannelsBySlot::iterator find = mChannelMap.find(slot);
    if (find != mChannelMap.end())
        mChannelMap.erase(find);

    return true;
}

// Remove all uninterested ops and add all interested ops
bool StandardSelector::updateOps(Channel& channel, uint32_t interestOps) const
{
    return true;
}


// Search list of registered keys and put them into the ready keys
void StandardSelector::processSelectedChannels()
{
    // IF woken by a wake() call
    if (FD_ISSET(mWakeSoks[0], &mReadSet))
    {
        // Wake FD; consume all bytes available
        char buf[256];
        int bytesRead;
        do
        {
            bytesRead = recv(mWakeSoks[0], buf, sizeof(buf), 0);
        } while (bytesRead > 0);
        mWakes.increment();
    }

    mProcessingChannels = true;

    // Iterate over the set of registered keys
    ChannelsBySlot::iterator iterator = mChannelMap.begin();
    while (iterator != mChannelMap.end())
    {
        Channel* channel = iterator->second;
        ++iterator;

        //Its possible for the channel to be nullptr if it was deleted during the course of this iteration,
        //before the deferred channel call was made.
        if (channel != nullptr)
        {
            SOCKET handle = static_cast<SOCKET>(channel->getHandle());
            ChannelHandler* handler = channel->getHandler();

            // Check the bits in the fd_sets and update the selection key's ready ops
            if (FD_ISSET(handle, &mExceptSet))
            {
                handler->onError(*channel);
                mNetworkTasks.increment();
            }
            else if (FD_ISSET(handle, &mReadSet))
            {
                handler->onRead(*channel);
                mNetworkTasks.increment();
            }
            else if (FD_ISSET(handle, &mWriteSet))
            {
                handler->onWrite(*channel);
                mNetworkTasks.increment();
            }
        }
        mNetworkPolls.increment();
    }

    mProcessingChannels = false;

    processDeferredRegistrations();
}

void StandardSelector::zeroAllFdSets()
{
    FD_ZERO(&mReadSet);
    FD_ZERO(&mWriteSet);
    FD_ZERO(&mExceptSet);
    mMaxFd = 0;
}

void StandardSelector::prepareFdSets()
{
    zeroAllFdSets();

    updateInterestOps(mWakeSoks[0], Channel::OP_READ);

    ChannelsBySlot::iterator itr = mChannelMap.begin();
    ChannelsBySlot::iterator end = mChannelMap.end();
    for(; itr != end; ++itr)
    {
        Channel* channel = itr->second;
        if (channel != nullptr)
        {
            updateInterestOps(channel->getHandle(), channel->getInterestOps());
        }
    }
}


void StandardSelector::processDeferredRegistrations()
{
    RegistrationQueue::iterator i = mDeferredRegistrationQueue.begin();
    RegistrationQueue::iterator e = mDeferredRegistrationQueue.end();
    for(; i != e; ++i)
    {
        RegistrationAction& reg = *i;
        switch (reg.mAction)
        {
            case REGISTER:
                registerChannel(*reg.mChannel, reg.mInterestOps);
                break;

            case UNREGISTER:
                unregisterChannel(reg.mSlot);
                break;
        }
    }
    mDeferredRegistrationQueue.clear();
}

void StandardSelector::queueDeferredRegistration(
        Action action, Channel& channel, uint32_t interestOps, int32_t slot)
{
    // Check if the channel is already in the queue
    RegistrationQueue::iterator i = mDeferredRegistrationQueue.begin();
    RegistrationQueue::iterator e = mDeferredRegistrationQueue.end();
    for(; i != e; ++i)
    {
        RegistrationAction& reg = *i;
        if ((&channel == reg.mChannel) && (channel.getHandle() == reg.mChannel->getHandle()))
        {
            // Channel already exists so just update it's action
            reg.mAction = action;
            reg.mInterestOps = interestOps;
            reg.mSlot = slot;
            return;
        }
    }

    // Channel not found so queue it
    mDeferredRegistrationQueue.push_back(RegistrationAction(action, channel, interestOps, slot));
}


/*** Private Methods *****************************************************************************/

} // Blaze

//lint +e717 

