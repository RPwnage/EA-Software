/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/channel.h"
#include "framework/connection/selector.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static EA_THREAD_LOCAL int32_t sNextChannelIdent = 0;

/*** Public Methods ******************************************************************************/

Channel::Channel()
    : mIdent(sNextChannelIdent++),
    mSlot(INVALID_SLOT),
    mInterestOps(OP_NONE),
    mHandler(nullptr),
    mRegistered(false),
    mHandlerRegistered(false),
    mPriority(false)
{
    if (sNextChannelIdent < 0)
        sNextChannelIdent = 0;
}

/*************************************************************************************************/
/*!
    \brief registerChannel

    Register the channel with the Selector for the requested operations.

    \param[in]  interestOps - The SelectionKey operations to register for
*/
/*************************************************************************************************/
bool Channel::registerChannel(uint32_t interestOps)
{
    bool rc = true;
    
    mInterestOps = interestOps;

    mRegistered = true;
    if (mHandler != nullptr)
    {
        mHandlerRegistered = true;
        rc = gSelector->registerChannel(*this, mInterestOps);
    }
    return rc;
}

/*************************************************************************************************/
/*!
    \brief unregisterChannel

    Unregister the channel with the Selector.
*/
/*************************************************************************************************/
void Channel::unregisterChannel(bool removeFd)
{
    gSelector->unregisterChannel(*this, removeFd);
    mRegistered = false;
    mHandlerRegistered = false;
}

bool Channel::setInterestOps(uint32_t interestOps)
{
    bool requireUpdate = (mInterestOps != interestOps);

    mInterestOps = interestOps;

    // IF we have a handler and aren't registered yet
    if ((mHandler != nullptr) && mRegistered && !mHandlerRegistered)
    {
        mHandlerRegistered = true;
        return gSelector->registerChannel(*this, mInterestOps);
    }

    // IF there were changes to the selection set
    if (requireUpdate && mRegistered)
    {
        // notify Selector of new interests.
        gSelector->updateOps(*this, mInterestOps);
        return true;
    }
    return true;
}

bool Channel::setHandler(ChannelHandler* handler)
{
    bool rc = true;

    mHandler = handler;

    if (mHandler != nullptr)
    {
        // Register the channel with the selector if we have a handler now and aren't
        // already registered
        if (mRegistered && !mHandlerRegistered)
        {
            mHandlerRegistered = true;
            rc = gSelector->registerChannel(*this, mInterestOps);
        }
    }
    else
    {
        // The handler is being cleared so deregister from the selector so we don't get
        // notifications on the socket.
        if (mHandlerRegistered)
        {
            mHandlerRegistered = false;
            gSelector->unregisterChannel(*this);
        }
    }
    return rc;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze
