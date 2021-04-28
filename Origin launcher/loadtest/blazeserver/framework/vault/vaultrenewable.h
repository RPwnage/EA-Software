/*************************************************************************************************/
/*!
    \file vaultrenewable.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include guard *******************************************************************************/

#ifndef VAULT_RENEWABLE_H
#define VAULT_RENEWABLE_H

/*** Include files *******************************************************************************/

#include <EATDF/time.h>
#include <EABase/eabase.h>

#include "framework/connection/selector.h"
#include "vaultinvalidatehandler.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class VaultManager;

template <class DataType> class VaultRenewable
{
    EA_NON_COPYABLE(VaultRenewable);

public:
    typedef VaultRenewable<DataType> ThisType;
    typedef VaultInvalidateHandler<ThisType> InvalidateHandler;

    VaultRenewable() : mValid(true), mRenewalTimer(INVALID_TIMER_ID), mCallback(nullptr) { }
    virtual ~VaultRenewable();
    
    const DataType& getData() const { return mData; }
    
    bool checkValidity();
    void updateData(const DataType &data, VaultManager *mgr = nullptr);
    void setInvalidateHandler(InvalidateHandler *callback);
    void renew(VaultManager &mgr);
    void scheduleRenewal(VaultManager &mgr);
    void cancelRenewal();
    void invalidate();
    void markValid() { mValid = true; }
    virtual const char* getName() const { return "Unknown Renewable"; }

protected:
    friend class VaultManager;

    DataType& getData() { return mData; }
    virtual BlazeRpcError renewData(VaultManager &mgr, DataType &data) = 0;
    void resetRenewal(TimerId newTimerId);
    void scheduleInvalidate();
    virtual Blaze::TimeValue getAdjustedTimeLeft(VaultManager &mgr, Blaze::TimeValue timeLeft) const = 0;

private:
    void doInvalidate();

    bool mValid;
    TimerId mRenewalTimer;
    InvalidateHandler *mCallback;
    DataType mData;
};

/******************************************************************************/
/*!
    \brief Destructs the renewable object.
*/
/******************************************************************************/
template <class DataType>
VaultRenewable<DataType>::~VaultRenewable()
{
    cancelRenewal();
}

/******************************************************************************/
/*! 
    \brief Checks to determine if the renewable object is still valid.

    \return success
*/
/******************************************************************************/
template <class DataType>
bool VaultRenewable<DataType>::checkValidity()
{
    char8_t nowBuffer[32];
    EA::TDF::TimeValue now = EA::TDF::TimeValue::getTimeOfDay();
    now.toDateFormat(nowBuffer, sizeof(nowBuffer));

    char8_t expirationBuffer[32];
    mData.getExpiration().toDateFormat(expirationBuffer, sizeof(expirationBuffer));

    BLAZE_TRACE_LOG(Log::VAULT, "[VaultRenewable].checkValidity: Vault renewable [" << getName() <<
        "] expires at [" << expirationBuffer << "] (currently [" << nowBuffer << "])");

    if (!mValid || mData.isExpired() || !mData.isValid())
    {
        invalidate();
    }
    return mValid;
}

/******************************************************************************/
/*! 
    \brief Updates the renewable data and schedules renewal of the lease - if
           needed.

    \param[in] data - the renewable data
    \param[in] mgr - the vault secret manager
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::updateData(const DataType &data, VaultManager *mgr)
{
    mData = data; // Make a local copy
    if (mgr)
    {
        scheduleRenewal(*mgr);
    }
}

/******************************************************************************/
/*! 
    \brief Adds an invalidate handler to listen when the renewable object is 
           invalidated.

    \param[in] callback - gets called back when the renewable object is 
                          invalidated
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::setInvalidateHandler(InvalidateHandler *callback)
{
    mCallback = callback;
}

/******************************************************************************/
/*! 
    \brief Renews the renewable object with the vault.

    \param[in] mgr - the vault secret manager
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::renew(VaultManager &mgr)
{
    if ((mData.isRenewable() && renewData(mgr, mData)) || checkValidity())
    {
        scheduleRenewal(mgr);
    }
}

/******************************************************************************/
/*! 
    \brief Schedules the renewal with the vault according to the acquired lease

    \param[in] mgr - the vault secret manager
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::scheduleRenewal(VaultManager &mgr)
{
    TimerId timerId = INVALID_TIMER_ID;
    Blaze::TimeValue now = EA::TDF::TimeValue::getTimeOfDay();
    Blaze::TimeValue leaseExpiration = mData.getExpiration();
    if (leaseExpiration == 0)
    {
        BLAZE_WARN_LOG(Log::VAULT, "[VaultRenewable].scheduleRenewal: Not scheduling renewal of secret vault renewable ["
            << getName() << "] because it has no expiration.");
    }
    else if (leaseExpiration >= now)
    {
        Blaze::TimeValue timeLeft = getAdjustedTimeLeft(mgr, leaseExpiration - now);

        // Prevent a runaway freight train, if we're failing to renew the token for whatever reason,
        // as the expiry approaches we would try faster and faster to renew until we're swamping the CPU
        // in the final seconds.
        Blaze::TimeValue minInterval(10 * 1000 * 1000);

        // Establishes the renewal time
        Blaze::TimeValue absoluteTime = now + (timeLeft > minInterval ? timeLeft : minInterval);
        
        char8_t dateBuffer[32];
        BLAZE_TRACE_LOG(Log::VAULT,
            "[VaultRenewable].scheduleRenewal: Scheduling the renewal of secret vault renewable [" << getName() << 
            "] at [" << absoluteTime.toDateFormat(dateBuffer, sizeof(dateBuffer)) << "].");

        // Schedule the renewal
        timerId = gSelector->scheduleFiberTimerCall<ThisType, VaultManager&>(
            absoluteTime, this, &ThisType::renew, mgr, "VaultRenewable::renew");
        if (timerId == INVALID_TIMER_ID)
        {
            BLAZE_ERR(Log::VAULT, "Unable to schedule the renewal of a vault lease");
        }
    }
    else
    {
        BLAZE_ERR(Log::VAULT, "Not scheduling a renewal because the expiration has already occurred.");
        invalidate();
    }
    resetRenewal(timerId);
}

/******************************************************************************/
/*! 
    \brief Cancels the future scheduled lease renewal
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::cancelRenewal()
{
    resetRenewal(INVALID_TIMER_ID);
}

/******************************************************************************/
/*! 
    \brief Resets the renewal timer. Cancels any existing timer and updates the
           current timer.
    
    \param[in] newTimerId the new timer id
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::resetRenewal(TimerId newTimerId)
{
    if (mRenewalTimer != INVALID_TIMER_ID && mRenewalTimer != newTimerId)
    {
        gSelector->cancelTimer(mRenewalTimer);
    }
    mRenewalTimer = newTimerId;
}

/******************************************************************************/
/*! 
    \brief Invalidates the current lease.
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::invalidate()
{
    if (mData.isValid())
    {
        BLAZE_INFO_LOG(Log::VAULT, "[VaultRenewable].invalidate: Invalidating secret vault renewable [" << getName() << "].");
    }

    bool previouslyValid = mValid;
    mValid = false;
    cancelRenewal();
    if (previouslyValid && mCallback)
    {
        scheduleInvalidate();
    }
}

/******************************************************************************/
/*! 
    \brief Schedules a call to inform the invalidate handler this object is 
           now invalid.
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::scheduleInvalidate()
{
    if (mCallback != nullptr)
    {
        // Intentionally ignoring the return value. Don't care.
        gSelector->scheduleFiberCall(this, &ThisType::doInvalidate, "VaultRenewable::doInvalidate");
    }
}

/******************************************************************************/
/*! 
    \brief Calls the invalidate handler.
*/
/******************************************************************************/
template <class DataType>
void VaultRenewable<DataType>::doInvalidate()
{
    if (mCallback != nullptr)
    {
        mCallback->handleInvalidate(*this);
    }
}

} // namespace Blaze

#endif // VAULT_RENEWABLE_H
