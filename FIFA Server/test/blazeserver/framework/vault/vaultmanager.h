/*************************************************************************************************/
/*!
    \file vaultmanager.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include guard *******************************************************************************/

#ifndef VAULT_MANAGER_H
#define VAULT_MANAGER_H

/*** Include files *******************************************************************************/

#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <EASTL/unique_ptr.h>

#include "proxycomponent/secretvault/tdf/secretvault.h"

#include "vaultinvalidatehandler.h"
#include "vaultlookuplocater.h"
#include "vaulttoken.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class FrameworkConfig;
class VaultLeaseData;
class VaultConfigUpdater;

class VaultManager : public VaultToken::InvalidateHandler
{
    EA_NON_COPYABLE(VaultManager);

public:
    typedef VaultRenewable<VaultTokenData> RenewableToken;
    typedef VaultConfigLookupLocater::LookupMap LookupMap;

    VaultManager();
    ~VaultManager() override;
    bool configure(const VaultConfig &config);
    void validateConfig(const VaultConfig &config, ConfigureValidationErrors &errors) const;
    uint32_t getLeaseRenewDivisor() const { return mVaultConfig.getLeaseRenewDivisor(); }
    EA::TDF::TimeValue getCallTimeout() const { return mVaultConfig.getCallTimeout(); }
    BlazeRpcError login();
    BlazeRpcError renewLease(VaultLeaseData &lease, int increment = 0);
    BlazeRpcError renewToken(int increment = 0);
    BlazeRpcError readSecret(const eastl::string &path, SecretVault::SecretVaultDataMap& data, bool cacheOnFetch = false);
    bool updateTdfWithSecrets(EA::TDF::Tdf &tdf, ConfigureValidationErrors &validationErrors);
    void invalidateCache();
    void invalidateClientToken();
    void handleInvalidate(const RenewableToken &token) override;

private:
    bool queryAndUpdateLookupItems(LookupMap &map, ConfigureValidationErrors &validationErrors);

    VaultToken mClientToken;

    VaultConfig mVaultConfig;

    eastl::string mSecretId;
};

extern EA_THREAD_LOCAL VaultManager *gVaultManager;

} // namespace Blaze

#endif // VAULT_MANAGER_H
