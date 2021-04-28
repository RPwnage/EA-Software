/*************************************************************************************************/
/*!
    \file oauthutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OAUTH_OAUTHUTIL_H
#define BLAZE_OAUTH_OAUTHUTIL_H

#include "framework/blazedefines.h"
#include "framework/tdf/oauth.h"

namespace Blaze
{
namespace Nucleus
{
    class NucleusResult;
    class FieldValidateErrorList;
}

namespace NucleusIdentity
{
    class IdentityError;
}

namespace NucleusConnect
{
    class ErrorResponse;
}

namespace OAuth
{

extern const char8_t* gTokenTypeStr[];

class OAuthUtil
{
public:
    static bool is4xxError(const BlazeRpcError rpcError);
    static BlazeRpcError parseNucleusIdentityError(const NucleusIdentity::IdentityError& error, const BlazeRpcError rpcError);
    static BlazeRpcError parseNucleusConnectError(const NucleusConnect::ErrorResponse& error, const BlazeRpcError rpcError);
    static InetAddress getRealEndUserAddr(const Command* callingCommand);

    static TokenType getTokenType(const FixedString16& typeStr);
private:
    typedef eastl::hash_map<const char8_t*, BlazeRpcError> NucleuConnectToRpcErrorMap;
    static NucleuConnectToRpcErrorMap initializeErrorMap();
    static const NucleuConnectToRpcErrorMap smNucleuConnectToRpcErrorMap;
};

}//OAuth
}//Blaze

#endif // BLAZE_OAUTH_OAUTHUTIL_H
