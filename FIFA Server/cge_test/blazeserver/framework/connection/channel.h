/*************************************************************************************************/
/*!
    \file   channel.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CHANNEL_H
#define BLAZE_CHANNEL_H

/*** Include files *******************************************************************************/
#include "framework/blazedefines.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;
class ChannelHandler;

typedef intptr_t ChannelHandle;

class Channel
{
    NON_COPYABLE(Channel);

public:
    const static int32_t INVALID_IDENT = -1;
    const static int32_t INVALID_SLOT = -1;

    // Bit fields for ready/interest flags.
    const static uint32_t OP_NONE   = 0;
    const static uint32_t OP_READ   = 1;
    const static uint32_t OP_WRITE  = 2;
    const static uint32_t OP_ERROR  = 4;
    const static uint32_t OP_CLOSE  = 8;

    virtual ~Channel() { }

    int32_t getIdent() const { return mIdent; }
    int32_t getSlot() const { return mSlot; }
    void setSlot(int32_t slot) { mSlot = slot; }
    void enablePriorityChannel() { mPriority = true; }
    bool isPriority() const { return mPriority; }

    bool registerChannel(uint32_t interestOps);
    void unregisterChannel(bool removeFd = false);

    uint32_t getInterestOps() const { return mInterestOps; }
    bool hasInterestOps(uint32_t interestOps) const { return (mInterestOps & interestOps) != 0; }
    bool setInterestOps(uint32_t interestOps);
    bool addInterestOp(uint32_t interestOp)
    {
        return setInterestOps(mInterestOps | interestOp);
    }
    bool removeInterestOp(uint32_t interestOp)
    {
        return setInterestOps(mInterestOps & (~interestOp));
    }

    ChannelHandler* getHandler() { return mHandler; }
    bool setHandler(ChannelHandler* handler);

    virtual ChannelHandle getHandle() = 0;

    virtual const char8_t* getCommonName() const { return nullptr; }

protected:
    Channel();

private:
    int32_t mIdent;
    int32_t mSlot;
    uint32_t mInterestOps;
    ChannelHandler* mHandler;
    bool mRegistered;
    bool mHandlerRegistered;
    bool mPriority;
};

} // Blaze

#endif // BLAZE_CHANNEL_H


