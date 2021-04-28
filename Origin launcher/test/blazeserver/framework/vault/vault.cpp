/*************************************************************************************************/
/*!
    \file vault.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Vault

    Provides support to send command to the Vault server. This enables the ability to read 
    secrets, renew tokens, and renew leases.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include <EAStdC/EAString.h>
#include <EATDF/time.h>
//#include "framework/logger.h"

#include "framework/connection/outboundhttpservice.h"

#include "proxycomponent/secretvault/rpc/secretvaultslave.h"
#include "proxycomponent/secretvault/tdf/secretvault.h"

#include "vault.h"
#include "vaulttoken.h"
#include "vaultlease.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

struct VaultRead
{
    SecretVault::SecretVaultReadRequest request;
    SecretVault::SecretVaultSecret response;
    SecretVault::SecretVaultErrorResponse error;
    RpcCallOptions options;
};

struct VaultKv2Read
{
    SecretVault::SecretVaultReadRequest request;
    SecretVault::SecretVaultKv2Secret response;
    SecretVault::SecretVaultErrorResponse error;
    RpcCallOptions options;
};

struct VaultRenewLease
{
    SecretVault::SecretVaultRenewLeaseRequest request;
    SecretVault::RenewLeaseResponse response;
    SecretVault::SecretVaultErrorResponse error;
    RpcCallOptions options;
};

struct VaultRenewToken
{
    SecretVault::SecretVaultRenewTokenRequest request;
    SecretVault::RenewTokenResponse response;
    SecretVault::SecretVaultErrorResponse error;
    RpcCallOptions options;
};

struct VaultApproleLogin
{
    SecretVault::SecretVaultApproleLoginRequest request;
    SecretVault::ApproleLoginResponse response;
    SecretVault::SecretVaultErrorResponse error;
    RpcCallOptions options;
};

struct VaultUnwrapSecretId
{
    SecretVault::SecretVaultUnwrapRequest request;
    SecretVault::SecretVaultSecretIdResponse response;
    SecretVault::SecretVaultErrorResponse error;
    RpcCallOptions options;
};

/*! ************************************************************************************************/
/*! 
    \brief Reads a secret from the Vault.

    \param[in] clientToken - the token to access the path
    \param[in] path - the path to the secret
    \param[out] lease - the lease result
    \param[out] data - the map of secret values
    \param[in] timeoutOverride - overrides the RPC timeout

    \return success
****************************************************************************************************/
BlazeRpcError VaultHelper::read(const char8_t* vaultNamespace, const VaultTokenData &clientToken, const eastl::string &path, SecretVault::SecretVaultDataMap &data, EA::TDF::TimeValue timeoutOverride)
{
    SecretVault::SecretVaultSlave& vaultSlave = static_cast<SecretVault::SecretVaultSlave&>(
        *const_cast<Blaze::Component*>(Blaze::gOutboundHttpService->getService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name)));

    VaultRead read;
    read.request.setPath(path.c_str());
    read.request.setVaultNamespace(vaultNamespace);
    read.request.setVaultToken(clientToken.getValue().c_str());
    read.options.timeoutOverride = timeoutOverride.getMicroSeconds();

    Blaze::BlazeRpcError err = vaultSlave.read(read.request, read.response, read.error, read.options);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].read: Vault KV1 read failed with error " << ErrorHelp::getErrorName(err));
        return err;
    }

    const SecretVault::SecretVaultSecret &vaultSecret = read.response;
    bool foundData = (vaultSecret.getData().size() > 0);
    if (!foundData)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].read: Vault KV1 read fetched no data for a secret.");
        return ERR_SYSTEM;
    }

    // Make a deep copy of the map containing the secrets
    vaultSecret.getData().copyInto(data);

    return err;
}

BlazeRpcError VaultHelper::kv2Read(const char8_t* vaultNamespace, const VaultTokenData &clientToken, const eastl::string &path, SecretVault::SecretVaultDataMap &data, EA::TDF::TimeValue timeoutOverride)
{
    SecretVault::SecretVaultSlave& vaultSlave = static_cast<SecretVault::SecretVaultSlave&>(
        *const_cast<Blaze::Component*>(Blaze::gOutboundHttpService->getService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name)));

    VaultKv2Read read;
    read.request.setPath(path.c_str());
    read.request.setVaultNamespace(vaultNamespace);
    read.request.setVaultToken(clientToken.getValue().c_str());
    read.options.timeoutOverride = timeoutOverride.getMicroSeconds();

    Blaze::BlazeRpcError err = vaultSlave.kv2Read(read.request, read.response, read.error, read.options);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].kv2Read: Vault KV2 read failed with error " << ErrorHelp::getErrorName(err));
        return err;
    }

    const SecretVault::SecretVaultKv2Secret &vaultSecret = read.response;
    bool foundData = (vaultSecret.getData().getData().size() > 0);
    bool foundWrappedResponse = (EA::StdC::Strcmp(vaultSecret.getWrapInfo().getToken(), "") != 0);

    if (foundWrappedResponse)
    {
        // This is unsupported and should never happen.
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].kv2Read: Vault KV2 read fetched a wrapped response with a secret. This is bad because it is unsupported.");
        return ERR_SYSTEM;
    }

    if (!foundData)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].kv2Read: Vault KV2 read fetched no data for a secret.");
        return ERR_SYSTEM;
    }

    // Make a deep copy of the map containing the secrets
    vaultSecret.getData().getData().copyInto(data);

    return ERR_OK;
}
/*! ************************************************************************************************/
/*! 
    \brief Renews a token with the vault.

    \param[in] clientToken - the token to access the path
    \param[out] token - the token data to renew
    \param[in] timeoutOverride - overrides the RPC timeout
    \param[in] increment - the number of seconds to increment the renew time 

    \return success

    \note The increment value is just a suggestion and may not be granted by the vault. It is 
          important to still check the return value. An increment value of 0 uses vault's default.
****************************************************************************************************/
BlazeRpcError VaultHelper::renewToken(const char8_t* vaultNamespace, VaultTokenData& clientToken, EA::TDF::TimeValue timeoutOverride, TimeValue increment)
{
    SecretVault::SecretVaultSlave& vaultSlave = static_cast<SecretVault::SecretVaultSlave&>(
        *const_cast<Blaze::Component*>(Blaze::gOutboundHttpService->getService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name)));

    VaultRenewToken renewToken;

    char8_t incrementStr[128];
    increment.toIntervalString(incrementStr, 128);
    renewToken.request.getRequestBody().setIncrement(incrementStr);

    renewToken.request.setVaultNamespace(vaultNamespace);
    renewToken.request.setVaultToken(clientToken.getValue().c_str());
    renewToken.options.timeoutOverride = timeoutOverride.getMicroSeconds();

    Blaze::BlazeRpcError err = vaultSlave.renewToken(renewToken.request, renewToken.response, renewToken.error, renewToken.options);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].renewToken: Vault token renewal failed with error " << ErrorHelp::getErrorName(err));
        return err;
    }

    const SecretVault::RenewTokenResponse &vaultSecret = renewToken.response;

    clientToken.setRenewable(vaultSecret.getAuth().getRenewable());
    TimeValue duration;
    duration.setSeconds(vaultSecret.getAuth().getLeaseDuration());
    clientToken.setDuration(duration);
    clientToken.setExpiration(TimeValue::getTimeOfDay() + duration);

    return ERR_OK;
}

/*! ************************************************************************************************/
/*! 
    \brief Renews a lease with the vault.

    \param[in] clientToken - the token to access the path
    \param[out] lease - the lease data to renew
    \param[in] increment - the number of seconds to increment the renew time 

    \return success

    \note The increment value is just a suggestion and may not be granted by the vault. It is 
          important to still check the return value. An increment value of 0 uses vault's default.
****************************************************************************************************/
BlazeRpcError VaultHelper::renewLease(const char8_t* vaultNamespace, const VaultTokenData &clientToken, VaultLeaseData &lease, EA::TDF::TimeValue timeoutOverride, int increment)
{
    SecretVault::SecretVaultSlave& vaultSlave = static_cast<SecretVault::SecretVaultSlave&>(
        *const_cast<Blaze::Component*>(Blaze::gOutboundHttpService->getService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name)));

    VaultRenewLease renewLease;
    renewLease.request.getRequestBody().setLeaseId(lease.getLeaseId().c_str());
    renewLease.request.getRequestBody().setIncrement(increment);
    renewLease.request.setVaultNamespace(vaultNamespace);
    renewLease.request.setVaultToken(clientToken.getValue().c_str());
    renewLease.options.timeoutOverride = timeoutOverride.getMicroSeconds();

    Blaze::BlazeRpcError err = vaultSlave.renewLease(renewLease.request, renewLease.response, renewLease.error, renewLease.options);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].renewLease: Vault lease renewal failed with error " << ErrorHelp::getErrorName(err));
        return err;
    }

    const SecretVault::RenewLeaseResponse& vaultSecret = renewLease.response;

    lease.setRenewable(vaultSecret.getRenewable());
    TimeValue duration;
    duration.setSeconds(vaultSecret.getLeaseDuration());
    lease.setDuration(duration);
    lease.setExpiration(TimeValue::getTimeOfDay() + duration);

    return ERR_OK;
}

BlazeRpcError VaultHelper::approleLogin(const char8_t* vaultNamespace, const eastl::string& path, const eastl::string& roleId, const eastl::string& secretId, VaultTokenData& token, EA::TDF::TimeValue timeoutOverride)
{
    SecretVault::SecretVaultSlave& vaultSlave = static_cast<SecretVault::SecretVaultSlave&>(
        *const_cast<Blaze::Component*>(Blaze::gOutboundHttpService->getService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name)));

    VaultApproleLogin approleLogin;
    approleLogin.request.setVaultNamespace(vaultNamespace);
    approleLogin.request.setPath(path.c_str());
    approleLogin.request.getRequestBody().setRoleId(roleId.c_str());
    approleLogin.request.getRequestBody().setSecretId(secretId.c_str());
    approleLogin.options.timeoutOverride = timeoutOverride.getMicroSeconds();

    Blaze::BlazeRpcError err = vaultSlave.approleLogin(approleLogin.request, approleLogin.response, approleLogin.error, approleLogin.options);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].approleLogin: Vault approle login failed with error " << ErrorHelp::getErrorName(err));
        return err;
    }

    const SecretVault::ApproleLoginResponse& vaultSecret = approleLogin.response;

    token.setValue(vaultSecret.getAuth().getClientToken());
    token.setAccessor(vaultSecret.getAuth().getAccessor());
    token.setRenewable(vaultSecret.getAuth().getRenewable());
    TimeValue duration;
    duration.setSeconds(vaultSecret.getAuth().getLeaseDuration());
    token.setDuration(duration);
    token.setExpiration(TimeValue::getTimeOfDay() + duration);

    return ERR_OK;
}

BlazeRpcError VaultHelper::unwrapSecretId(const char8_t* vaultNamespace, const char8_t* token, eastl::string& secretId, EA::TDF::TimeValue timeoutOverride)
{
    SecretVault::SecretVaultSlave& vaultSlave = static_cast<SecretVault::SecretVaultSlave&>(
        *const_cast<Blaze::Component*>(Blaze::gOutboundHttpService->getService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name)));

    VaultUnwrapSecretId unwrap;
    unwrap.request.setVaultNamespace(vaultNamespace);
    unwrap.request.setVaultToken(token);
    unwrap.options.timeoutOverride = timeoutOverride.getMicroSeconds();

    Blaze::BlazeRpcError err = vaultSlave.unwrapSecretId(unwrap.request, unwrap.response, unwrap.error, unwrap.options);
    if (err != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].unwrapSecretId: Vault unwrap failed with error " << ErrorHelp::getErrorName(err));
        return err;
    }

    const SecretVault::SecretVaultSecretIdResponse &vaultSecret = unwrap.response;
    bool foundSecret = (strlen(vaultSecret.getData().getSecretId()) > 0);
    bool foundWrappedResponse = (EA::StdC::Strcmp(vaultSecret.getWrapInfo().getToken(), "") != 0);

    if (foundWrappedResponse)
    {
        // This is unsupported and should never happen.
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].unwrapSecretId: Vault unwrap fetched a wrapped response with a secret. This is bad because it is unsupported.");
        return ERR_SYSTEM;
    }

    if (!foundSecret)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultHelper].unwrapSecretId: Vault unwrap fetched no secret id.");
        return ERR_SYSTEM;
    }

    // Make a deep copy of the map containing the secrets
    secretId = vaultSecret.getData().getSecretId();

    return ERR_OK;
}

} // namespace Blaze
