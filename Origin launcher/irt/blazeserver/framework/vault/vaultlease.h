/*************************************************************************************************/
/*!
    \file vaultlease.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include guard *******************************************************************************/

#ifndef VAULT_LEASE_H
#define VAULT_LEASE_H

/*** Include files *******************************************************************************/

#include <EATDF/time.h>
#include <EASTL/string.h>

#include "vaultrenewable.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class VaultManager;

/*************************************************************************************************/
/*!
    \class VaultLeaseData

    Defines the lease data along with the expiration and renewable timing necessary to keep it
    valid. 
*/
/*************************************************************************************************/

class VaultLeaseData
{
public:
    VaultLeaseData() : mRenewable(false), mLeaseDuration(0), mLeaseExpiration(EA::TDF::TimeValue::getTimeOfDay()) { }
    ~VaultLeaseData() { }
    void setPath(const eastl::string &path) { mPath = path; }
    void setLeaseId(const eastl::string &leaseId) { mLeaseId = leaseId; }
    void setRenewable(bool renewable) { mRenewable = renewable; }
    void setDuration(const EA::TDF::TimeValue &leaseDuration) { mLeaseDuration = leaseDuration; }
    void setExpiration(const EA::TDF::TimeValue &leaseExpiration) { mLeaseExpiration = leaseExpiration; }
    const eastl::string& getPath() const { return mPath; }
    const eastl::string& getLeaseId() const { return mLeaseId; }
    bool isRenewable() const { return mRenewable; }
    const EA::TDF::TimeValue& getDuration() const { return mLeaseDuration; }
    const EA::TDF::TimeValue& getExpiration() const { return mLeaseExpiration; }
    bool isExpired() const { return (EA::TDF::TimeValue::getTimeOfDay() >= mLeaseExpiration && mLeaseExpiration > 0); }
    bool isValid() const { return !mPath.empty() || !mLeaseId.empty(); }

private:
    eastl::string mPath;
    eastl::string mLeaseId;
    bool mRenewable;
    EA::TDF::TimeValue mLeaseDuration;
    EA::TDF::TimeValue mLeaseExpiration;
};

class VaultLease : public VaultRenewable<VaultLeaseData>
{
public:
    VaultLease() { }
    ~VaultLease() override { }
    const char* getName() const override { return getData().getPath().c_str(); }
protected:
    BlazeRpcError renewData(VaultManager &mgr, VaultLeaseData &data) override;
    Blaze::TimeValue getAdjustedTimeLeft(VaultManager &mgr, Blaze::TimeValue timeLeft) const override;
};

} // namespace Blaze

#endif // VAULT_LEASE_H
