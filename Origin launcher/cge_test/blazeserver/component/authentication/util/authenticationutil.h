/*************************************************************************************************/
/*!
    \file authenticationutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_AUTHENTICATIONUTIL_H
#define BLAZE_AUTHENTICATION_AUTHENTICATIONUTIL_H

/*** Include files *******************************************************************************/

namespace Blaze
{

namespace NucleusConnect
{
class ErrorResponse;
}

namespace NucleusIdentity
{
class IdentityError;
class EntitlementInfo;
}

namespace Authentication
{

class NucleusResult;
class FieldValidateErrorList;
class Entitlement;

class AuthenticationUtil
{
public:
    static void nucleusResultToFieldErrorList(const NucleusResult* result, FieldValidateErrorList& errorList);
    static BlazeRpcError parseIdentityError(const NucleusIdentity::IdentityError& error, const BlazeRpcError rpcError);
    static char8_t *formatExternalRef(uint64_t id, char8_t *newExtRef, size_t newExtRefBufSize);
    static void parseEntitlementForResponse(PersonaId personaId, const NucleusIdentity::EntitlementInfo &info, Entitlement &responseEntitlement);
    static InetAddress getRealEndUserAddr(const Command* callingCommand);
    static ClientType getClientType(const PeerInfo* peerInfo);
    
    // Convert OAuth error to auth error for backward compatibility
    static BlazeRpcError authFromOAuthError(BlazeRpcError oAuthErr);

    typedef eastl::hash_map<const char8_t*, BlazeRpcError> NucleuConnectToRpcErrorMap;

};

}//Authentication
}//Blaze

#endif // BLAZE_AUTHENTICATION_AUTHENTICATIONUTIL_H
