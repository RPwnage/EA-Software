/*************************************************************************************************/
/*!
    \file   


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STANDARDSELECTOR_H
#define BLAZE_STANDARDSELECTOR_H

/*** Include files *******************************************************************************/
#include "framework/connection/platformsocket.h"
#include "framework/connection/selector.h"
#include "framework/connection/channel.h"
#include "EASTL/hash_map.h"
#include "EASTL/vector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class StandardSelector : public Selector
{
// To allow access to register()/unregister()
friend class Channel;  
    NON_COPYABLE(StandardSelector);

public:
    StandardSelector();
    ~StandardSelector() override;
    bool open() override;
    bool close() override;
    bool isOpen() override  { return mIsOpen; }

protected:
    EA::TDF::TimeValue select() override;
    void wake() override;

    bool registerChannel(Channel& channel, uint32_t interestOps) override;
    bool unregisterChannel(Channel& channel, bool removeFd = false) override;

    bool updateOps(Channel& channel, uint32_t interestOps) const override;

private:
    typedef enum { REGISTER, UNREGISTER } Action;
    struct RegistrationAction
    {

        RegistrationAction(Action action, Channel& channel, uint32_t interestOps, int32_t slot)
            : mAction(action),
              mHandle(channel.getHandle()),
              mChannel(action == REGISTER ? &channel : nullptr),
              mInterestOps(interestOps),
              mSlot(slot)
        {
        };

        Action mAction;
        ChannelHandle mHandle;
        Channel* mChannel;
        uint32_t mInterestOps;
        int32_t mSlot;
    };

    struct HandleHash
    {
        size_t operator()(ChannelHandle val) const
        {
            return static_cast<size_t>(val);
        }
    };
    typedef eastl::hash_map<int32_t, Channel*, HandleHash> ChannelsBySlot;
    typedef eastl::vector<RegistrationAction> RegistrationQueue;

    int32_t mNextSlot;
    bool mIsOpen;
    bool mProcessingChannels;
    SOCKET mMaxFd;
    fd_set mReadSet;
    fd_set mWriteSet;
    fd_set mExceptSet;
    ChannelsBySlot mChannelMap;
    RegistrationQueue mDeferredRegistrationQueue;
    SOCKET mWakeSoks[2];
    struct sockaddr_in mWakeAddr;

    void updateInterestOps(ChannelHandle handle, uint32_t interestOps);
    void zeroAllFdSets();
    void prepareFdSets();
    void processDeferredRegistrations();
    void queueDeferredRegistration(Action action, Channel& channel, uint32_t interestOps, int32_t slot);
    bool unregisterChannel(int32_t slot);
    void processSelectedChannels();

};  // StandardSelector

} // Blaze

#endif // BLAZE_STANDARDSELECTOR_H

