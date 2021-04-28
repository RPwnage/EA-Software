/*************************************************************************************************/
/*!
    \file   


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EPOLLSELECTOR_H
#define BLAZE_EPOLLSELECTOR_H

/*** Include files *******************************************************************************/
#include "framework/connection/platformsocket.h"
#include "framework/connection/selector.h"
#include "framework/connection/channel.h"
#include "EASTL/hash_map.h"
#include "EASTL/intrusive_list.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

class EpollSelector : public Selector
{
    // To allow access to register()/unregister()
    friend class Channel;  
    friend class SelectionKey;

    NON_COPYABLE(EpollSelector);

public:
    EpollSelector();
    ~EpollSelector() override;
    bool open() override;
    bool close() override;
    bool isOpen() override  { return mIsOpen; }
    
    bool initialize(const SelectorConfig &config) override;

protected:
    // Protected methods
    EA::TDF::TimeValue select() override;
    void wake() override;

    bool registerChannel(Channel& channel, uint32_t interestOps) override;
    bool unregisterChannel(Channel& channel, bool removeFd = false) override;

    bool updateOps(Channel& channel, uint32_t interestOps) const override;

private:
    EA::TDF::TimeValue selectOnFd(int32_t epollFd, int32_t timeout, Metrics::Counter& networkTasks, Metrics::Counter& networkPolls);
    bool registerChannelWithEpoll(Channel& channel, uint32_t interestOps, int32_t slot, bool priority);
    bool updateEpollOps(int32_t epollFd, Channel& channel, uint32_t interestOps) const;

private:
    struct Slot
    {
        Channel* mChannel;
        int32_t mIdent;
        int32_t mNext;
    };

    bool mIsOpen;
    int32_t mPollFd;
    int32_t mPriorityPollFd;
    int32_t mWakePipe[2];
    struct epoll_event* mPollEvents;
    Slot* mSlots;
    int32_t mFreeSlotHead;
};  // EpollSelector

} // Blaze

#endif // BLAZE_EPOLLSELECTOR_H

