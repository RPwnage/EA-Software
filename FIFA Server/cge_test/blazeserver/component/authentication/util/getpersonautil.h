/*************************************************************************************************/
/*!
    \file   getpersonautil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_AUTHENTICATION_GETPERSONAUTIL_H
#define BLAZE_AUTHENTICATION_GETPERSONAUTIL_H

#include "framework/connection/outboundhttpservice.h"
#include "framework/tdf/userdefines.h"

#include "authentication/tdf/authentication.h"
#include "framework/oauth/accesstokenutil.h"

#include "nucleusidentity/rpc/nucleusidentityslave.h"

namespace Blaze
{
namespace NucleusIdentity
{
    class PersonaInfo;
}

namespace Authentication
{

class GetPersonaUtil
{
public:
    typedef Functor2<BlazeRpcError, const GetPersonaResponse*> getPersonaCb;

    GetPersonaUtil(const PeerInfo* peerInfo)
        : mPeerInfo(peerInfo)
    {
        mIdent = (NucleusIdentity::NucleusIdentitySlave*) gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name);
    }

    virtual ~GetPersonaUtil() {};

    BlazeRpcError getPersona(const char8_t *displayName, const char8_t *namespaceName, AccountId *resultId, PersonaInfo* result);
    BlazeRpcError getPersona(PersonaId id, PersonaInfo* result);
    BlazeRpcError getPersona(PersonaId id, AccountId *resultId, PersonaInfo *result);
    BlazeRpcError getPersona(ClientPlatformType extRefType, uint64_t extRefValue, AccountId *resultId, PersonaInfo *resultInfo);
    BlazeRpcError getPersona(ClientPlatformType extRefType, const char8_t *extRefValue, AccountId *resultId, PersonaInfo *resultInfo);

    BlazeRpcError getPersonaList(AccountId accountId, const char8_t *namespaceName, PersonaDetailsList &result, Blaze::Nucleus::PersonaStatus::Code personaStatus = Blaze::Nucleus::PersonaStatus::UNKNOWN);
    BlazeRpcError getPersonaList(AccountId accountId, const char8_t *namespaceName, PersonaInfoList &result, Blaze::Nucleus::PersonaStatus::Code personaStatus = Blaze::Nucleus::PersonaStatus::UNKNOWN);

private:
    // Not owned memory
    NucleusIdentity::NucleusIdentitySlave* mIdent;
    const PeerInfo* mPeerInfo;

    // Owned memory
    OAuth::AccessTokenUtil mTokenUtil;

    BlazeRpcError retrievePersonas(AccountId accountId, const char8_t *namespaceName, NucleusIdentity::GetPersonaListResponse &resp);
    static bool checkStatus(const NucleusIdentity::PersonaInfo& persona, Blaze::Nucleus::PersonaStatus::Code personaStatus);
    static void nucleusPersonaToPersonaInfo(const NucleusIdentity::PersonaInfo &from, PersonaInfo &to);
};

}
}
#endif //BLAZE_AUTHENTICATION_GETPERSONAUTIL_H

