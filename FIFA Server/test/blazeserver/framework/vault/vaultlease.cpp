/*************************************************************************************************/
/*!
    \file vaultlease.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class VaultLease

    Defines a renewable vault lease that can be used to access secrets.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/


#include "framework/blaze.h"
#include "framework/util/random.h"
#include "vaultlease.h"
#include "vaultmanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*************************************************************************************************/
/*!
    \class VaultLease

    The vault lease defines the scope of a secret and defines how often it should be renewed and
    how long it remains valid.
*/
/*************************************************************************************************/

/******************************************************************************/
/*! 
    \brief Renews the lease data with the vault secret manager.

    \param[in] mgr - the vault secret manager
    \param[in,out] data - the lease data

    \return success
*/
/******************************************************************************/
BlazeRpcError VaultLease::renewData(VaultManager &mgr, VaultLeaseData &data)
{
    return mgr.renewLease(data);
}

/******************************************************************************/
/*! 
    \brief Renews the lease data with the vault secret manager.

    \param[in] mgr - the vault secret manager
    \param[in] timeLeft - the initial time left

    \return the adjusted time left
*/
/******************************************************************************/
Blaze::TimeValue VaultLease::getAdjustedTimeLeft(VaultManager &mgr, Blaze::TimeValue timeLeft) const
{
    if (getData().isRenewable())
    {
        uint32_t timeLeftDivisor = mgr.getLeaseRenewDivisor();
        if (timeLeftDivisor != 0)
        {
            // Scale the time to give us some buffer to renew before the lease expires
            int secsLeft = static_cast<int>(timeLeft.getSec());
            timeLeft.setSeconds(gRandom->getRandomNumber(secsLeft / timeLeftDivisor));
        }
    }
    return timeLeft;
}

} // namespace Blaze
