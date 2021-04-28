/*************************************************************************************************/
/*!
    \file oauthutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/oauth/oauthutil.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/protocol/restprotocolutil.h"

#include "framework/rpc/oauth_defines.h"
#include "framework/tdf/nucleus.h"
#include "nucleusidentity/tdf/nucleusidentity.h"
#include "nucleusidentity/tdf/nucleusidentity_base.h"
#include "nucleusconnect/tdf/nucleusconnect.h"
#include "proxycomponent/nucleusconnect/rpc/nucleusconnect_defines.h"
#include "proxycomponent/nucleusidentity/rpc/nucleusidentity_defines.h"

namespace Blaze
{
namespace OAuth
{

const char* gTokenTypeStr[] =
{
    "",
    "Bearer",
    "NEXUS_S2S",
};

static_assert(Blaze::OAuth::TOKEN_TYPE_COUNT == EAArrayCount(gTokenTypeStr), "If you see this compile error, make sure TokenType and gTokenTypeStr are in sync.");


OAuthUtil::NucleuConnectToRpcErrorMap OAuthUtil::initializeErrorMap()
{
    // Unfortunately, C&I uses same error codes across a bunch of errors. In some situations, we want to have a finer distinction in the blaze error that we expose.
    // So we have to rely on the string parsing (C&I will keep the strings backward compatible).  
    NucleuConnectToRpcErrorMap m;
    m["username or password is invalid"] = OAUTH_ERR_INVALID_USER; //C&I code 105012 
    m["user or persona status is invalid."] = OAUTH_ERR_INVALID_USER; //C&I code 105102 
    m["persona info is invalid"] = OAUTH_ERR_PERSONA_NOT_FOUND; //C&I code 105012 
    m["persona_display_name  or persona_id is invalid"] = OAUTH_ERR_PERSONA_EXTREFID_REQUIRED; //C&I code 102013 (the 2 spaces before 'or' is a typo in C&I and will remain so to ensure backward compatibility) 
    return m;
}

const OAuthUtil::NucleuConnectToRpcErrorMap OAuthUtil::smNucleuConnectToRpcErrorMap = initializeErrorMap();

/*************************************************************************************************/
/*!
    \brief parseNucleusConnectError

    Parse NucleusConnect error and associated rpc error to return a more specific/suitable OAUTH error

    \param[in]  error - the NucleusConnect error
    \param[in]  rpcError - the rpc error got from calling NucleusConnect rpc
    \return     BlazeRpcError - OAUTH error
*/
/*************************************************************************************************/
BlazeRpcError OAuthUtil::parseNucleusConnectError(const NucleusConnect::ErrorResponse& error, const BlazeRpcError rpcError)
{
    NucleuConnectToRpcErrorMap::const_iterator iter = smNucleuConnectToRpcErrorMap.begin();
    NucleuConnectToRpcErrorMap::const_iterator end = smNucleuConnectToRpcErrorMap.end();
    for (; iter != end; ++iter)
    {
        if (blaze_strcmp(error.getErrorDescription(), iter->first) == 0)
        {
            return iter->second;
        }
    }

    if (!BLAZE_ERROR_IS_SYSTEM_ERROR(rpcError))
    {
        //convert NucleusConnect rpc error to OAUTH rpc error or general system error
        switch (rpcError)
        {
        case NUCLEUS_ERR_INVALID_TOKEN:     return OAUTH_ERR_INVALID_TOKEN;
        case NUCLEUS_ERR_INVALID_REQUEST:   return OAUTH_ERR_INVALID_REQUEST;
        default:                            return ERR_SYSTEM;
        }
    }
    return rpcError;
}

/*************************************************************************************************/
/*!
    \brief parseNucleusIdentityError

    Parse IdentityError error and associated rpc error to return a more specific/suitable OAUTH error

    \param[in]  error - the IdentityError error
    \param[in]  rpcError - the rpc error got from calling NucleusIdentity rpc
    \return     BlazeRpcError - OAUTH error
*/
/*************************************************************************************************/
BlazeRpcError OAuthUtil::parseNucleusIdentityError(const NucleusIdentity::IdentityError& error, const BlazeRpcError rpcError)
{
    using namespace Blaze::Nucleus;

    // Identity's code + field + cause string all together makes the error unique for us (see error_codes.h for mapping) so we find that is possible. 
    NucleusCode::Code code = NucleusCode::UNKNOWN;
    NucleusCode::ParseCode(error.getError().getCode(), code);
    if (!error.getError().getFailure().empty())
    {
        const NucleusIdentity::FailureDetails *failureDetails = error.getError().getFailure().at(0);
        NucleusField::Code field = NucleusField::UNKNOWN;
        NucleusField::ParseCode(failureDetails->getField(), field);
        NucleusCause::Code cause = NucleusCause::UNKNOWN;
        NucleusCause::ParseCode(failureDetails->getCause(), cause);

        BlazeRpcError err = OAuthSlaveImpl::getErrorCodes()->getError(code, field, cause);
        if (err != ERR_SYSTEM)
            return err;
    }
    else if (error.getError().getCode()[0] != '\0')
    {
        BlazeRpcError err = OAuthSlaveImpl::getErrorCodes()->getError(code);
        if (err != ERR_SYSTEM)
            return err;
    }

    if (!BLAZE_ERROR_IS_SYSTEM_ERROR(rpcError))
    {
        //convert NucleusIdentity rpc error to OAUTH rpc error or general system error
        switch (rpcError)
        {
        case NUCLEUS_IDENTITY_ENTITLEMENT_GRANTED:  return ERR_SYSTEM;  //no corresponding OAUTH error, using ERR_SYSTEM instead
        case NUCLEUS_IDENTITY_INVALID_REQUEST:      return OAUTH_ERR_INVALID_REQUEST;
        default:                                    return ERR_SYSTEM;
        }
    }
    return rpcError;
}

InetAddress OAuthUtil::getRealEndUserAddr(const Command* callingCommand)
{
    if (callingCommand != nullptr && callingCommand->getPeerInfo() != nullptr)
    {
        return callingCommand->getPeerInfo()->getRealPeerAddress();
    }
    else if (gCurrentLocalUserSession != nullptr && gCurrentLocalUserSession->getPeerInfo() != nullptr)
    {
        return gCurrentLocalUserSession->getPeerInfo()->getRealPeerAddress();
    }
    return InetAddress();
}


TokenType OAuthUtil::getTokenType(const FixedString16& typeStr)
{
    if (blaze_stricmp(typeStr.c_str(), gTokenTypeStr[TOKEN_TYPE_BEARER]) == 0)
        return TOKEN_TYPE_BEARER;

    if (blaze_stricmp(typeStr.c_str(), gTokenTypeStr[TOKEN_TYPE_NEXUS_S2S]) == 0)
        return TOKEN_TYPE_NEXUS_S2S;

    return TOKEN_TYPE_NONE;

}

}//OAuth
}//Blaze
