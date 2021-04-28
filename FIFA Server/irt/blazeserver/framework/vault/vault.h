/*************************************************************************************************/
/*!
    \file vault.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include guard *******************************************************************************/

#ifndef VAULT_H
#define VAULT_H

/*** Include files *******************************************************************************/

#include <EASTL/string.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class VaultTokenData;
class VaultLeaseData;

class VaultHelper
{
    EA_NON_COPYABLE(VaultHelper);

public:
    VaultHelper() {}
    ~VaultHelper() {}
    static BlazeRpcError read(const char8_t* vaultNamespace, const VaultTokenData &clientToken, const eastl::string &path, SecretVault::SecretVaultDataMap &data, EA::TDF::TimeValue timeoutOverride);
    static BlazeRpcError kv2Read(const char8_t* vaultNamespace, const VaultTokenData &clientToken, const eastl::string &path, SecretVault::SecretVaultDataMap &data, EA::TDF::TimeValue timeoutOverride);
    static BlazeRpcError renewToken(const char8_t* vaultNamespace, VaultTokenData& clientToken, EA::TDF::TimeValue timeoutOverride, TimeValue increment = 0);
    static BlazeRpcError renewLease(const char8_t* vaultNamespace, const VaultTokenData &clientToken, VaultLeaseData &lease, EA::TDF::TimeValue timeoutOverride, int increment = 0);
    static BlazeRpcError approleLogin(const char8_t* vaultNamespace, const eastl::string& path, const eastl::string& roleId, const eastl::string& secretId, VaultTokenData& token, EA::TDF::TimeValue timeoutOverride);
    static BlazeRpcError unwrapSecretId(const char8_t* vaultNamespace, const char8_t* token, eastl::string& secretId, EA::TDF::TimeValue timeoutOverride);
};

} // namespace Blaze

#endif // VAULT_H
