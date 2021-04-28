/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OAUTH_JWTUTIL_H
#define BLAZE_OAUTH_JWTUTIL_H

#include "framework/tdf/oauth_server.h"
#include "nucleusconnect/tdf/nucleusconnect.h"
#include "nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace OAuth
{

class OAuthSlaveImpl;

class JwtUtil
{
public:
    struct JwtSegmentInfo
    {
        char8_t* header;
        uint32_t headerSize;
        char8_t* payload;
        uint32_t payloadSize;
        char8_t* signature;
        uint32_t signatureSize;

        JwtSegmentInfo()
            : header(nullptr)
            , headerSize(0)
            , payload(nullptr)
            , payloadSize(0)
            , signature(nullptr)
            , signatureSize(0)
        {
        }
    };

    struct SignatureVerificationData
    {
        uint8_t* sig;
        uint32_t sigSize;
        uint8_t* msg;
        uint32_t msgSize;
        uint8_t* mod;
        uint32_t modSize;
        uint8_t* exp;
        uint32_t expSize;

        SignatureVerificationData()
            : sig(nullptr)
            , sigSize(0)
            , msg(nullptr)
            , msgSize(0)
            , mod(nullptr)
            , modSize(0)
            , exp(nullptr)
            , expSize(0)
        {
        }
    };

    struct PublicKeyInfo
    {
        eastl::string modulus;
        eastl::string exponent;

        PublicKeyInfo()
            : modulus("")
            , exponent("")
        {
        }
    };
    typedef eastl::string PublicKeyId;
    typedef eastl::hash_map<PublicKeyId, PublicKeyInfo> PublicKeyInfoMap;

    JwtUtil(OAuthSlaveImpl* oAuthSlave);
    ~JwtUtil() {}

    bool isJwtFormat(const char8_t* token);
    BlazeRpcError decodeJwtAccessToken(const char8_t* token, NucleusConnect::JwtPayloadInfo& payloadInfo);
    
    BlazeRpcError getTokenInfo(const NucleusConnect::JwtPayloadInfo& payloadInfo, const ClientPlatformType platform, NucleusConnect::GetTokenInfoResponse& response);
    BlazeRpcError getAccount(const NucleusConnect::JwtPayloadInfo& payloadInfo, const ClientPlatformType platform, NucleusIdentity::GetAccountResponse& response);
    BlazeRpcError getPersonaList(const NucleusConnect::JwtPayloadInfo& payloadInfo, const char8_t* namespaceName, NucleusIdentity::GetPersonaListResponse& response);
    BlazeRpcError getExternalRef(const NucleusConnect::JwtPayloadInfo& payloadInfo, const PersonaId personaId, NucleusIdentity::GetExternalRefResponse& response);
    BlazeRpcError getPersonaInfo(const NucleusConnect::JwtPayloadInfo& payloadInfo, const PersonaId personaId, NucleusIdentity::GetPersonaInfoResponse& response);
    
    int64_t getExpiresIn(const char8_t* token);

    bool getJwtSegmentInfo(const char8_t* token, JwtSegmentInfo& segInfo);
    BlazeRpcError decodeJwtSegment(const char8_t* seg, const int32_t segSize, EA::TDF::Tdf& tdf);
    BlazeRpcError getPublicKeyInfo(const char8_t* keyId, PublicKeyInfo& keyInfo);
    BlazeRpcError refreshPublicKeyInfo();
    BlazeRpcError verifyJwtSignature(const SignatureVerificationData& data);
    bool verifySignatureRS256(const SignatureVerificationData& data);

private:
    int32_t Base64DecodeUrl(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen);
    int32_t _Base64Decode(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen, const signed char *pDecode);

    const char8_t* getTokenIssuer();
    const TimeValue& getPublicKeyInfoRefreshMinInterval();

private:
    OAuthSlaveImpl* mOAuthSlave;

    PublicKeyInfoMap mPublicKeyInfoMap;
    TimeValue mPublicKeyInfoRefreshAttemptTime;

    bool mIsRefreshingPublicKeyInfo;
    Fiber::WaitList mPublicKeyInfoRefreshWaitList;

};

} //namespace OAuth
} //namespace Blaze

#endif // BLAZE_OAUTH_JWTUTIL_H
