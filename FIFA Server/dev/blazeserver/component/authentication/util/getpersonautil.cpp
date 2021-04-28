/*************************************************************************************************/
/*!
    \file   getpersonautil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"

#include "framework/oauth/nucleus/nucleusstringtocode.h"
#include "authentication/rpc/authentication_defines.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/util/getpersonautil.h"

#include "nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{

BlazeRpcError GetPersonaUtil::getPersona(const char8_t *displayName, const char8_t *namespaceName, AccountId *resultId, PersonaInfo* result)
{
    // Do not allow wild card searches against Nucleus
    if (strchr(displayName, '*') != nullptr)
    {
        WARN_LOG("[GetPersonaUtil].getPersona(): The displayName[" << displayName << "] includes the wild card (*). Failed with error[AUTH_ERR_INVALID_USER]");
        return AUTH_ERR_INVALID_USER;
    }

    BlazeRpcError rc = mTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != ERR_OK)
    {
        ERR_LOG("[GetPersonaUtil].getPersona: Failed to retrieve access token");
        return rc;
    }

    NucleusIdentity::GetPersonaListRequest req;
    NucleusIdentity::GetPersonaListResponse resp;
    NucleusIdentity::IdentityError error;

    req.getFilter().setDisplayName(displayName);
    req.getFilter().setNamespaceName(namespaceName);
    req.getAuthCredentials().setAccessToken(mTokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(mTokenUtil.getClientId());
    req.setExpandResults("true");
    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    rc = mIdent->getPersonas(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GetPersonaUtil].getPersona: getPersonas failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    if (resp.getPersonas().getPersona().empty())
        return AUTH_ERR_PERSONA_NOT_FOUND;

    if (resultId != nullptr)
        *resultId = resp.getPersonas().getPersona()[0]->getPidId();
    if (result != nullptr)
        nucleusPersonaToPersonaInfo(*resp.getPersonas().getPersona()[0], *result);

    return rc;
}

BlazeRpcError GetPersonaUtil::getPersona(PersonaId id, PersonaInfo *result)
{
    return getPersona(id, nullptr, result);
}

BlazeRpcError GetPersonaUtil::getPersona(PersonaId id, AccountId *resultId, PersonaInfo *result)
{
    BlazeRpcError rc = ERR_SYSTEM;
    
    rc = mTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != ERR_OK)
    {
        ERR_LOG("[GetPersonaUtil].getPersona: Failed to retrieve access token");
        return rc;
    }

    NucleusIdentity::GetPersonaInfoRequest req;
    NucleusIdentity::GetPersonaInfoResponse resp;
    NucleusIdentity::IdentityError error;

    req.setPersonaId(id);
    req.getAuthCredentials().setAccessToken(mTokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(mTokenUtil.getClientId());
    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    rc = mIdent->getPersona(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GetPersonaUtil].getPersona: getPersona failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    if (resultId != nullptr)
        *resultId = resp.getPersona().getPidId();
    if (result != nullptr)
        nucleusPersonaToPersonaInfo(resp.getPersona(), *result);

    return rc;
}

BlazeRpcError GetPersonaUtil::getPersona(ClientPlatformType extRefType, uint64_t extRefValue, AccountId *resultId, PersonaInfo *resultInfo)
{
    char8_t extRefValueString[32];
    blaze_snzprintf(extRefValueString, sizeof(extRefValueString), "%" PRIu64, extRefValue);
    return getPersona(extRefType, extRefValueString, resultId, resultInfo);
}

BlazeRpcError GetPersonaUtil::getPersona(ClientPlatformType extRefType, const char8_t *extRefValue, AccountId *resultId, PersonaInfo *resultInfo)
{
    BlazeRpcError rc = mTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != ERR_OK)
    {
        ERR_LOG("[GetPersonaUtil].getPersona: Failed to retrieve access token");
        return rc;
    }

    NucleusIdentity::GetPersonaInfoExtIdRequest req;
    NucleusIdentity::GetPersonaListResponse resp;
    NucleusIdentity::IdentityError error;
    req.getFilter().setExternalRefType(ClientPlatformTypeToString(extRefType));
    req.getFilter().setExternalRefValue(extRefValue);
    req.getAuthCredentials().setAccessToken(mTokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(mTokenUtil.getClientId());
    req.setExpandResults("true");
    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    rc = mIdent->getPersonaInfoByExtId(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GetPersonaUtil].getPersona: getPersonaInfoByExtId failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    if (resp.getPersonas().getPersona().empty())
    {
        return AUTH_ERR_PERSONA_NOT_FOUND;
    }

    PersonaId personaId = resp.getPersonas().getPersona()[0]->getPersonaId();
    return getPersona(personaId, resultId, resultInfo);
}

BlazeRpcError GetPersonaUtil::getPersonaList(AccountId accountId, const char8_t *namespaceName, PersonaDetailsList &result, Blaze::Nucleus::PersonaStatus::Code personaStatus)
{
    NucleusIdentity::GetPersonaListResponse resp;
    BlazeRpcError err = retrievePersonas(accountId, namespaceName, resp);
    if (err != ERR_OK)
        return err;

    for (const auto& persona : resp.getPersonas().getPersona())
    {
        TRACE_LOG("[GetPersonaUtil].getPersonaList() - PersonaId(" << persona->getPersonaId() << "), DisplayName(" << persona->getDisplayName() << "),"
            " Status(" << persona->getStatus() << ")");

        if (checkStatus(*persona, personaStatus))
        {
            PersonaDetails* persDetails = BLAZE_NEW PersonaDetails();
            persDetails->setPersonaId((int64_t)persona->getPersonaId());
            persDetails->setDisplayName(persona->getDisplayName());
            persDetails->setStatus(Blaze::Nucleus::PersonaStatus::stringToCode(persona->getStatus()));
            result.push_back(persDetails);
        }
    }

    return err;
}

BlazeRpcError GetPersonaUtil::getPersonaList(AccountId accountId, const char8_t *namespaceName, PersonaInfoList &result, Blaze::Nucleus::PersonaStatus::Code personaStatus)
{
    NucleusIdentity::GetPersonaListResponse resp;
    BlazeRpcError err = retrievePersonas(accountId, namespaceName, resp);
    if (err != ERR_OK)
        return err;

    for (const auto& persona : resp.getPersonas().getPersona())
    {
        TRACE_LOG("[GetPersonaUtil].getPersonaList() - PersonaId(" << persona->getPersonaId() << "), DisplayName(" << persona->getDisplayName() << "),"
            " Status(" << persona->getStatus() << ")");

        if (checkStatus(*persona, personaStatus))
        {
            PersonaInfo* personaInfo = BLAZE_NEW PersonaInfo();
            nucleusPersonaToPersonaInfo(*persona, *personaInfo);
            result.push_back(personaInfo);
        }
    }

    return err;
}

BlazeRpcError GetPersonaUtil::retrievePersonas(AccountId accountId, const char8_t *namespaceName, NucleusIdentity::GetPersonaListResponse& resp)
{
    BlazeRpcError rc = mTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (rc != ERR_OK)
    {
        ERR_LOG("[GetPersonaUtil].retrievePersonas: Failed to retrieve access token");
        return rc;
    }

    NucleusIdentity::GetPersonaListRequest req;
    NucleusIdentity::IdentityError error;
    req.setPid(accountId);
    req.setExpandResults("true");
    req.getFilter().setNamespaceName(namespaceName);
    req.getAuthCredentials().setAccessToken(mTokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(mTokenUtil.getClientId());
    req.getAuthCredentials().setIpAddress(mPeerInfo->getRealPeerAddress().getIpAsString());

    rc = mIdent->getPersonaList(req, resp, error);
    if (rc != ERR_OK)
    {
        rc = AuthenticationUtil::parseIdentityError(error, rc);
        ERR_LOG("[GetPersonaUtil].retrievePersonas: getPersonaList failed with error(" << ErrorHelp::getErrorName(rc) << ")");
        return rc;
    }

    uint16_t personaCount = resp.getPersonas().getPersona().size();
    TRACE_LOG("[GetPersonaListUtil].gotPersonaList() - Number of personas returned: " << personaCount << "");

    return rc;
}

bool GetPersonaUtil::checkStatus(const NucleusIdentity::PersonaInfo& persona, Blaze::Nucleus::PersonaStatus::Code personaStatus)
{
    Blaze::Nucleus::PersonaStatus::Code status = Blaze::Nucleus::PersonaStatus::stringToCode(persona.getStatus());
    if (personaStatus == Blaze::Nucleus::PersonaStatus::UNKNOWN)
    {
        // Set the results
        switch (status)
        {
        case Blaze::Nucleus::PersonaStatus::ACTIVE:
        case Blaze::Nucleus::PersonaStatus::DISABLED:
        case Blaze::Nucleus::PersonaStatus::DEACTIVATED:
        case Blaze::Nucleus::PersonaStatus::PENDING:
        case Blaze::Nucleus::PersonaStatus::BANNED:
        {
            return true;
        }
        default:
        {
            break;
        }
        }
    }
    else if (personaStatus == status)
    {
        return true;
    }

    return false;
}

void GetPersonaUtil::nucleusPersonaToPersonaInfo(const NucleusIdentity::PersonaInfo &from, PersonaInfo &to)
{
    to.setPersonaId(from.getPersonaId());
    to.setDisplayName(from.getDisplayName());
    to.setStatus(Blaze::Nucleus::PersonaStatus::stringToCode(from.getStatus()));
    to.setStatusReasonCode(Blaze::Nucleus::StatusReason::stringToCode(from.getStatusReasonCode()));
    to.setDateCreated(from.getDateCreated());
    to.setLastAuthenticated(static_cast<uint32_t>(TimeValue::parseAccountTime(from.getLastAuthenticated()).getSec()));
    to.setNameSpaceName(from.getNamespaceName());
}

}
}
