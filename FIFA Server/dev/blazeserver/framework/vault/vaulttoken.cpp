/*************************************************************************************************/
/*!
    \file vaulttoken.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class VaultToken

    Defines a renewable vault token that can be used to access the vault.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"
#include "vaulttoken.h"
#include "vaultmanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze 
{

/*************************************************************************************************/
/*!
    \class VaultToken

    The vault token is used to authenticate with the vault.
*/
/*************************************************************************************************/

/******************************************************************************/
/*! 
    \brief Renews the token data with the vault secret manager.

    \param[in] the vault secret manager
    \param[in,out] the lease data

    \return success
*/
/******************************************************************************/
BlazeRpcError VaultToken::renewData(VaultManager &mgr, VaultTokenData &data)
{
    return mgr.renewToken();
}

/******************************************************************************/
/*! 
    \brief Renews the lease data with the vault secret manager.

    \param[in] mgr - the vault secret manager
    \param[in] timeLeft - the initial time left

    \return the adjusted time left
*/
/******************************************************************************/
Blaze::TimeValue VaultToken::getAdjustedTimeLeft(VaultManager &mgr, Blaze::TimeValue timeLeft) const
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
