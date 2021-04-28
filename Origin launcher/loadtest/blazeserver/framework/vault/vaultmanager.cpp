/*************************************************************************************************/
/*!
    \file vaultmanager.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class VaultManager

    Manages the logic for communicating with a vault. Handles the establishment of the token and
    caching of any secret maps and leases provided by the vault. 
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"
#include <EATDF/tdfobject.h>
#include <EATDF/typedescription.h>

#include "proxycomponent/secretvault/rpc/secretvault_defines.h"
#include "proxycomponent/secretvault/rpc/secretvaultslave.h"

#include "vaultmanager.h"
#include "vault.h"
#include "vaultlease.h"
#include "vaultlookuplocater.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/******************************************************************************/
/*! 
    \brief Constructor for the vault secret manager.
*/
/******************************************************************************/
VaultManager::VaultManager()
{ 
}

/******************************************************************************/
/*! 
    \brief Destructor for the vault secret manager.
*/
/******************************************************************************/
VaultManager::~VaultManager()
{
}

/******************************************************************************/
/*!
    \brief Validates the vault secret manager TDF configuration.

    \param[in] config - the configuration
    \param[in] errors - the validation errors

*/
/******************************************************************************/
void VaultManager::validateConfig(const VaultConfig &config, ConfigureValidationErrors &errors) const
{
    if (!config.getEnabled())
        return;

    if (config.getAuthType() != VAULT_AUTH_TOKEN && config.getAuthType() != VAULT_AUTH_APPROLE)
    {
        eastl::string msg;
        msg.sprintf("[VaultManager].validateConfig() : configured to use an invalid login type.");
        EA::TDF::TdfString& str = errors.getErrorMessages().push_back();
        str.set(msg.c_str());
        return;
    }

    if (config.getHttpConnection().getVaultPath().getPath() != nullptr && config.getHttpConnection().getVaultPath().getPath()[0] != '\0')
    {
        eastl::string msg;
        msg.sprintf("[VaultManager].validateConfig() : attempt to use Vault to fetch configuration parameters for Vault!");
        EA::TDF::TdfString& str = errors.getErrorMessages().push_back();
        str.set(msg.c_str());
        return;
    }

    eastl::string errMsg;
    if (!gOutboundHttpService->validateHttpServiceConnConfig(SecretVault::SecretVaultSlave::COMPONENT_INFO.name, config.getHttpConnection(), errMsg))
    {
        eastl::string msg;
        msg.sprintf("[VaultManager].validateConfig() : failed to validate configuration for %s http service: %s", SecretVault::SecretVaultSlave::COMPONENT_INFO.name, errMsg.c_str());
        EA::TDF::TdfString& str = errors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
}

/******************************************************************************/
/*! 
    \brief Initializes the vault secret manager from the TDF configuration.
           Automatically schedules the login() operation.

    \param[in] config - the configuration

    \return success
*/
/******************************************************************************/
bool VaultManager::configure(const VaultConfig& config)
{
    config.copyInto(mVaultConfig);

    // Reset all of the secret vault state variables just in case this function has been called once than once
    invalidateClientToken();

    if (!mVaultConfig.getEnabled())
    {
        BLAZE_INFO_LOG(Log::VAULT, "[VaultManager].configure: Vault is disabled");
        gOutboundHttpService->removeHttpService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name);
        return true;
    }

    gOutboundHttpService->updateHttpService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name, mVaultConfig.getHttpConnection(), SecretVault::SecretVaultSlave::COMPONENT_ID);

    if (login() != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].configure: Failed to login to Vault");
        gOutboundHttpService->removeHttpService(SecretVault::SecretVaultSlave::COMPONENT_INFO.name);
        return false;
    }

    return true;
}

/******************************************************************************/
/*! 
    \brief Authenticates the vault secret manager with the vault. Establishes
           the authentication token with the vault. Must be done before using
           the vault secret manager.

    \return success
*/
/******************************************************************************/
BlazeRpcError VaultManager::login()
{   
    BlazeRpcError error = ERR_OK;

    // If we were previously using a token then we should invalidate the cache
    // before establishing a new token. Changing the token will invalidate the 
    // previous entries anyways - it is better to do it now than later.
    if (mClientToken.checkValidity())
    {
        BLAZE_WARN_LOG(Log::VAULT,
            "[VaultManager].login: Attempting to login to the secret vault even though existing token [" << mClientToken.getName() << "] is still valid.");
        invalidateClientToken();
    }

    const EA::TDF::Tdf *params = mVaultConfig.getAuthParameters();

    switch (mVaultConfig.getAuthType())
    {
        case VAULT_AUTH_TOKEN:
        {
            if (params != nullptr)
            {
                VaultAuthToken tokenParams;
 
                if (params->getTdfId() != tokenParams.getTdfId())
                {
                    BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].login: Unexpected auth parameters " << params->getFullClassName() << ", expected " << tokenParams.getFullClassName());
                    error = ERR_SYSTEM;
                    break;
                }

                params->copyInto(tokenParams);

                const char *token = tokenParams.getToken();
                const char *accessor = tokenParams.getAccessor();

                mClientToken.getData().setValue(token);
                mClientToken.getData().setAccessor(accessor);
                mClientToken.getData().setRenewable(false);
                mClientToken.getData().setDuration(0);
                mClientToken.getData().setExpiration(0);

                mClientToken.markValid();
                mClientToken.setInvalidateHandler(this);

                BLAZE_INFO_LOG(Log::VAULT, "[VaultManager].login: Configured static client token [" << mClientToken.getName() << "]");
            }
        }
        break;

        case VAULT_AUTH_APPROLE:
        {
            if (params != nullptr)
            {
                VaultAuthApprole approleParams;

                if (params->getTdfId() != approleParams.getTdfId())
                {
                    BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].login: Unexpected auth parameters " << params->getFullClassName() << ", expected " << approleParams.getFullClassName());
                    break;
                }

                params->copyInto(approleParams);

                const char* path = approleParams.getPath();
                const char* roleId = approleParams.getRoleId();
                const char* secretId = approleParams.getSecretId();

                if (approleParams.getWrapped())
                {
                    if (mSecretId.empty())
                    {
                        if (VaultHelper::unwrapSecretId(mVaultConfig.getVaultNamespace(), secretId, mSecretId, getCallTimeout()) != ERR_OK)
                        {
                            BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].login: Failed to unwrap secret id");
                            error = ERR_SYSTEM;
                            break;
                        }
                    }

                    secretId = mSecretId.c_str();
                }

                if (VaultHelper::approleLogin(mVaultConfig.getVaultNamespace(), path, roleId, secretId, mClientToken.getData(), getCallTimeout()) != ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].login: Failed to perform approle login to Vault");
                    error = ERR_SYSTEM;
                    break;
                }

                mClientToken.markValid();
                mClientToken.setInvalidateHandler(this);

                // Note we choose not to schedule the token for renewal as Blaze's typical usage of Vault is one burst of fetches
                // at either server startup or reconfigure time.  There is little need to burden Vault with continual token renewals

                BLAZE_INFO_LOG(Log::VAULT, "[VaultManager].login: Successfully authenticated via approle id " << roleId << ", client token set to [" << mClientToken.getName() << "]");
            }
        }
        break;

        default:
        {
            BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].login: Could not login to Vault, invalid login type.");
            error = ERR_SYSTEM;
        }
    };

    return error;
}

/******************************************************************************/
/*! 
    \brief Renews the lease with the vault.

    \param[in,out] lease - the lease to renew and update
    \param[in] - the amount of time in seconds to renew the lease. This can
                 be ignored by the vault. Be sure to check to new lease 
                 expiration time assigned by the vault.

    \return success
*/
/******************************************************************************/
BlazeRpcError VaultManager::renewLease(VaultLeaseData &lease, int increment)
{
    BlazeRpcError error = ERR_OK;

    if (!mVaultConfig.getEnabled())
    {
        return error; // early return
    }

    if (!mClientToken.checkValidity())
    {
        BLAZE_INFO_LOG(Log::VAULT, "[VaultManager].renewLease: Reauthenticating with Vault as token has expired");
        error = SECRETVAULT_ERR_FORBIDDEN;
    }
    else
    {
        error = VaultHelper::renewLease(mVaultConfig.getVaultNamespace(), mClientToken.getData(), lease, getCallTimeout(), increment);
    }

    if (error == SECRETVAULT_ERR_FORBIDDEN)
    {
        // We need to reauthenticate if either we already knew our token had expired,
        // or something happened to our token on the Vault side such as an administrator removing it
        error = login();
        if (error != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].renewLease: Cannot renew lease [" << lease.getPath().c_str() << 
                "] due to failure to authenticate with Vault");
            return error; // early return
        }

        error = VaultHelper::renewLease(mVaultConfig.getVaultNamespace(), mClientToken.getData(), lease, getCallTimeout(), increment);
    }

    if (error == ERR_OK)
    {
        BLAZE_INFO_LOG(Log::VAULT,
            "[VaultManager].renewLease: Successfully renewed the lease [" << lease.getPath().c_str() << 
            "] using secret vault client token [" << mClientToken.getName() << "].");
    }
    else
    {
        BLAZE_ERR_LOG(Log::VAULT,
            "[VaultManager].renewLease: Unable to renew the lease [" << lease.getPath().c_str() << 
            "] using secret vault client token [" << mClientToken.getName() << "].");
    }
    return error;
}

/******************************************************************************/
/*! 
    \brief Renews the token with the vault.

    \param[in,out] token - the token to renew and update
    \param[in] - the amount of time in seconds to renew the token. This can
                 be ignored by the vault. Be sure to check to new token 
                 expiration time assigned by the vault.

    \return success
*/
/******************************************************************************/
BlazeRpcError VaultManager::renewToken(int increment)
{
    BlazeRpcError error = ERR_OK;
    if (!mVaultConfig.getEnabled())
    {
        return error; // early return
    }

    if (!mClientToken.checkValidity())
    {
        BLAZE_INFO_LOG(Log::VAULT, "[VaultManager].renewToken: Reauthenticating with Vault as token has expired");
        error = SECRETVAULT_ERR_FORBIDDEN;
    }
    else
    {
        error = VaultHelper::renewToken(mVaultConfig.getVaultNamespace(), mClientToken.getData(), getCallTimeout(), increment);
    }

    if (error == SECRETVAULT_ERR_FORBIDDEN)
    {
        // We need to reauthenticate if either we already knew our token had expired,
        // or something happened to our token on the Vault side such as an administrator removing it
        error = login();
        if (error != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].renewToken: Cannot renew token due to failure to authenticate with Vault");
            return error; // early return
        }

        // Return right away since if we have to login, we will have a brand new fresh token
        // and there is no longer a need to renew it
        return ERR_OK;
    }

    if (error == ERR_OK)
    {
        BLAZE_TRACE_LOG(Log::VAULT, "[VaultManager].renewToken: Successfully renewed the token [" << mClientToken.getName() << "].");
    }
    else
    {
        BLAZE_WARN_LOG(Log::VAULT, "[VaultManager].renewToken: Unable to renew the token [" << mClientToken.getName() << "].");
    }
    return error;
}

/******************************************************************************/
/*! 
    \brief Reads a secret from the vault based upon the specified path. If the
           secret was previously read and cached then the cached version is 
           provided.

    \param[in] path - the path
    \param[out] secret = the secret
    \param[in] cacheOnFetch - set if the secret value should be cached when 
               fetched from the vault

    \return error code
*/
/******************************************************************************/
BlazeRpcError VaultManager::readSecret(const eastl::string &path, SecretVault::SecretVaultDataMap& dataMap, bool cacheOnFetch)
{
    if (!mVaultConfig.getEnabled())
    {
        return ERR_OK; // early return
    }

    if (path.empty())
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].readSecret: Unable to read a secret from the secret vault because the path is empty.");
        return SECRETVAULT_ERR_INVALID_PATH; // early return
    }

    if (mVaultConfig.getVaultConfigEngine() != VAULT_ENGINE_KV1 && mVaultConfig.getVaultConfigEngine() != VAULT_ENGINE_KV2)
    {
        BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].readSecret: Unsupported engine " << mVaultConfig.getVaultConfigEngine() << ".");
        return ERR_SYSTEM;
    }

    BlazeRpcError error = ERR_OK;
    if (!mClientToken.checkValidity())
    {
        BLAZE_INFO_LOG(Log::VAULT, "[VaultManager].readSecret: Reauthenticating with Vault as token has expired");
        error = SECRETVAULT_ERR_FORBIDDEN;
    }
    else
    {
        if (mVaultConfig.getVaultConfigEngine() == VAULT_ENGINE_KV1)
            error = VaultHelper::read(mVaultConfig.getVaultNamespace(), mClientToken.getData(), path, dataMap, getCallTimeout());
        else
            error = VaultHelper::kv2Read(mVaultConfig.getVaultNamespace(), mClientToken.getData(), path, dataMap, getCallTimeout());
    }

    if (error == SECRETVAULT_ERR_FORBIDDEN)
    {
        // We need to reauthenticate if either we already knew our token had expired,
        // or something happened to our token on the Vault side such as an administrator removing it
        error = login();
        if (error != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::VAULT, "[VaultManager].readSecret: Cannot read secret [" << path.c_str() <<
                "] due to failure to authenticate with Vault");
            return error; // early return
        }

        if (mVaultConfig.getVaultConfigEngine() == VAULT_ENGINE_KV1)
            error = VaultHelper::read(mVaultConfig.getVaultNamespace(), mClientToken.getData(), path, dataMap, getCallTimeout());
        else
            error = VaultHelper::kv2Read(mVaultConfig.getVaultNamespace(), mClientToken.getData(), path, dataMap, getCallTimeout());
    }

    if (error == ERR_OK)
    {
        BLAZE_INFO_LOG(Log::VAULT,
            "[VaultManager].readSecret: Successful remote lookup secret vault path [" << path.c_str() << 
            "] using client token [" << mClientToken.getName() << "].");
    }
    else
    {
        BLAZE_ERR_LOG(Log::VAULT,
            "[VaultManager].readSecret: Fail to remotely lookup secret vault path [" << path.c_str() << 
            "] using client token [" << mClientToken.getName() << "].");
    }
    return error;
}

/*******************************************************************************************/
/*!
    \brief PRIVATE METHOD. Used by VaultManager::updateTdfWithSecrets to update each tdf
           in the provided map with secret values from the secret vault or the secret manager cache.
*/
/*******************************************************************************************/
bool VaultManager::queryAndUpdateLookupItems(LookupMap &map, ConfigureValidationErrors &validationErrors)
{
    for (LookupMap::iterator it = map.begin(); it != map.end(); ++it)
    {
        SecretVault::SecretVaultDataMap data;
        BlazeRpcError err = readSecret(it->first.c_str(), data, true);
        if (err == ERR_OK)
        {
            if (data.size() > 0)
            {
                for (auto &parent : it->second)
                {
                    EA::TDF::Tdf &entry = parent.asTdf();
                    for (SecretVault::SecretVaultDataMap::const_iterator secIt = data.begin(); secIt != data.end(); ++secIt)
                    {
                        EA::TDF::TdfGenericReference value;
                        const char8_t* secKey = secIt->first.c_str();
                        const char8_t* secVal = secIt->second.c_str();
                        if (entry.getValueByName(secKey, value))
                        {
                            const char8_t* numPtr = nullptr;

                            switch (value.getTypeDescription().getTdfType())
                            {
                            case EA::TDF::TDF_ACTUAL_TYPE_INT8:   int8_t i8;     numPtr = blaze_str2int(secVal, &i8);   if (numPtr != secVal && *numPtr == '\0')  value.asInt8()   = i8;   break;
                            case EA::TDF::TDF_ACTUAL_TYPE_UINT8:  uint8_t ui8;   numPtr = blaze_str2int(secVal, &ui8);  if (numPtr != secVal && *numPtr == '\0')  value.asUInt8()  = ui8;  break;
                            case EA::TDF::TDF_ACTUAL_TYPE_INT16:  int16_t i16;   numPtr = blaze_str2int(secVal, &i16);  if (numPtr != secVal && *numPtr == '\0')  value.asInt16()  = i16;  break;
                            case EA::TDF::TDF_ACTUAL_TYPE_UINT16: uint16_t ui16; numPtr = blaze_str2int(secVal, &ui16); if (numPtr != secVal && *numPtr == '\0')  value.asUInt16() = ui16; break;
                            case EA::TDF::TDF_ACTUAL_TYPE_INT32:  int32_t i32;   numPtr = blaze_str2int(secVal, &i32);  if (numPtr != secVal && *numPtr == '\0')  value.asInt32()  = i32;  break;
                            case EA::TDF::TDF_ACTUAL_TYPE_UINT32: uint32_t ui32; numPtr = blaze_str2int(secVal, &ui32); if (numPtr != secVal && *numPtr == '\0')  value.asUInt32() = ui32; break;
                            case EA::TDF::TDF_ACTUAL_TYPE_INT64:  int64_t i64;   numPtr = blaze_str2int(secVal, &i64);  if (numPtr != secVal && *numPtr == '\0')  value.asInt64()  = i64;  break;
                            case EA::TDF::TDF_ACTUAL_TYPE_UINT64: uint64_t ui64; numPtr = blaze_str2int(secVal, &ui64); if (numPtr != secVal && *numPtr == '\0')  value.asUInt64() = ui64; break;
                            case EA::TDF::TDF_ACTUAL_TYPE_STRING: value.asString().set(secVal); break;
                            default:
                                BLAZE_WARN_LOG(Log::VAULT, "[VaultManager].updateTdfWithSecrets() : Not overriding '" << secKey << "' in " << entry.getClassName() << "; only string and integer types are supported");
                                continue;
                            }

                            if (value.getTypeDescription().isIntegral() && (numPtr == nullptr || numPtr == secVal || *numPtr != '\0'))
                            {
                                BLAZE_WARN_LOG(Log::VAULT, "[VaultManager].updateTdfWithSecrets() : Not overriding '" << secKey << "' in " << entry.getClassName() << "; secret vault value is not in range");
                                continue;
                            }

                            // Need to set 'value' through 'entry' to ensure that change tracking is set correctly.
                            // Note that the actual string copy is a no-op, since we're passing in 'value' as our reference string.
                            entry.setValueByName(secKey, value);
                            BLAZE_INFO_LOG(Log::VAULT, "[VaultManager].updateTdfWithSecrets() : Overriding '" << secKey << "' in " << entry.getClassName() << " with value from secret vault path '" << it->first.c_str() << "'");
                        }
                    }
                }
            }
            else
            {
                eastl::string msg;
                msg.sprintf("[VaultManager].updateTdfWithSecrets() : No error returned by the secret vault, but no secret provided for path '%s'", it->first.c_str());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return false;
            }
        }
        else
        {
            eastl::string msg;
            msg.sprintf("[VaultManager].updateTdfWithSecrets() : Error %s reading secret for path '%s'", ErrorHelp::getErrorName(err), it->first.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }
    }

    return true;
}

bool VaultManager::updateTdfWithSecrets(EA::TDF::Tdf &tdf, ConfigureValidationErrors &validationErrors)
{
    if (!mVaultConfig.getEnabled())
    {
        return true; // early return
    }

    VaultConfigLookupLocater locater;
    LookupMap &vaultLookupEntries = locater.lookup(tdf);

    // Dynamically update the map of VaultLookup entries. This will make blocking calls to Vault.
    return queryAndUpdateLookupItems(vaultLookupEntries, validationErrors);
}

/******************************************************************************/
/*! 
    \brief Invalidates the client token and all of the cached secrets.
*/
/******************************************************************************/
void VaultManager::invalidateClientToken()
{
    mClientToken.invalidate();
}

/******************************************************************************/
/*! 
    \brief Is called when the client token is invalidated. Used to print an 
           error message to the user.

    \param[in] token - the client token
*/
/******************************************************************************/
void VaultManager::handleInvalidate(const RenewableToken &token)
{
    // For now merely log the invalidation, in the future if we have cached data
    // associated with the token then we would clean it up here
    BLAZE_INFO_LOG(Log::VAULT,
        "[VaultManager].handleInvalidate: The secret vault client token [" << token.getName() << "] has been invalidated."); 
}

} // namespace Blaze
