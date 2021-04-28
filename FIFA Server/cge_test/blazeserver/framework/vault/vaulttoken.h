/*************************************************************************************************/
/*!
    \file vaulttoken.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include guard *******************************************************************************/

#ifndef VAULT_TOKEN_H
#define VAULT_TOKEN_H

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
    \class VaultTokenData

    Defines the token data along with the expiration and renewable timing necessary to keep it
    valid. Also provides the accessor value that does not need to be kept hidden.
*/
/*************************************************************************************************/

class VaultTokenData
{
public:
    VaultTokenData() : mName("unknown"), mRenewable(false), mDuration(0), mTokenExpiration(EA::TDF::TimeValue::getTimeOfDay()) { }
    ~VaultTokenData() { }
    void setValue(const eastl::string &value) { mValue = value; }
    void setAccessor(const eastl::string &accessor) { mAccessor = accessor; mName.sprintf("accessor=%s", accessor.c_str()); }
    void setRenewable(bool renewable) { mRenewable = renewable; }
    void setDuration(const EA::TDF::TimeValue &duration) { mDuration = duration; }
    void setExpiration(const EA::TDF::TimeValue &tokenExpiration) { mTokenExpiration = tokenExpiration; }
    const eastl::string& getValue() const { return mValue; }
    const eastl::string& getAccessor() const { return mAccessor; }
    const eastl::string& getName() const { return mName; }
    bool isRenewable() const { return mRenewable; }
    const EA::TDF::TimeValue& getDuration() const { return mDuration; }
    const EA::TDF::TimeValue& getExpiration() const { return mTokenExpiration; }
    bool isExpired() const { return (EA::TDF::TimeValue::getTimeOfDay() >= mTokenExpiration && mTokenExpiration > 0); }
    bool isValid() const { return !mValue.empty(); }

private:
    eastl::string mValue;
    eastl::string mAccessor;
    eastl::string mName;
    bool mRenewable;
    EA::TDF::TimeValue mDuration;
    EA::TDF::TimeValue mTokenExpiration;
};

class VaultToken : public VaultRenewable<VaultTokenData>
{
public:
    VaultToken() { }
    ~VaultToken() override { }
    const char* getName() const override { return getData().getName().c_str(); }
protected:
    BlazeRpcError renewData(VaultManager &mgr, VaultTokenData &data) override;
    Blaze::TimeValue getAdjustedTimeLeft(VaultManager &mgr, Blaze::TimeValue timeLeft) const override;
};

} // namespace Blaze

#endif // VAULT_TOKEN_H
