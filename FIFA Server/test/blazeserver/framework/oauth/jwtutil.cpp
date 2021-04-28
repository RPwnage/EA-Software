/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/oauth/jwtutil.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/controller/controller.h"
#include "framework/rpc/oauth_defines.h"
#include "nucleusconnect/tdf/nucleusconnect.h"

#include "openssl/rsa.h"
#include "openssl/evp.h"

namespace Blaze
{
namespace OAuth
{
    static const int OPENSSL_SUCCESS = 1;

    //copied from dirtysdk base64.c
    static const signed char _Base64_strDecodeUrl[] =
    {
        -1, -1,                                             // +,
        62,                                                 // -
        -1, -1,                                             // ./
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61,             // 0-9
        -1, -1, -1, -1, -1, -1, -1,                         // :;<=>?@
        0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, // A-
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, //  -Z
        -1, -1, -1, -1,                                     // [\]^
        63,                                                 // _
        -1,                                                 // `
        26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, // a-
        39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51  //  -z
    };

    JwtUtil::JwtUtil(OAuthSlaveImpl* oAuthSlave)
        : mOAuthSlave(oAuthSlave)
        , mPublicKeyInfoRefreshAttemptTime(0)
        , mIsRefreshingPublicKeyInfo(false)
    {
    }

    bool JwtUtil::isJwtFormat(const char8_t* token)
    {
        JwtSegmentInfo segInfo;
        return getJwtSegmentInfo(token, segInfo);
    }

    //decode jwt access token, verify signature, read token info from it
    //returns OAUTH error code
    BlazeRpcError JwtUtil::decodeJwtAccessToken(const char8_t* token, NucleusConnect::GetTokenInfoResponse& response)
    {
        //step 0: get info of header, payload, signature
        JwtSegmentInfo segments;
        if (!getJwtSegmentInfo(token, segments))
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Input access token is not JWT format, token is: " << (token == nullptr ? "" : token));
            return OAUTH_ERR_INVALID_TOKEN;
        }

        //step 1: decode header to get key id
        NucleusConnect::JwtHeaderInfo headerInfo;
        BlazeRpcError rc = decodeJwtSegment(segments.header, segments.headerSize, headerInfo);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Failed to decode JWT header: " << eastl::string(segments.header, segments.headerSize).c_str());
            return rc;
        }
        BLAZE_TRACE_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Decoded JWT header to " << headerInfo.getClassName() << headerInfo);

        if (blaze_strcmp(headerInfo.getAlgorithm(), "RS256") != 0)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: JWT access token algorithm(" << headerInfo.getAlgorithm() << ") is not RS256");
            return OAUTH_ERR_INVALID_TOKEN;
        }

        //step 2: get public key modulus and exponent by key id
        PublicKeyInfo publicKeyInfo;
        rc = getPublicKeyInfo(headerInfo.getKeyId(), publicKeyInfo);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Failed to get public key info for key id(" << headerInfo.getKeyId() << ")");
            return rc;
        }
        BLAZE_TRACE_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Obtained public key info for key id(" << headerInfo.getKeyId() << "): exponent(" << publicKeyInfo.exponent << "), modulus(" << publicKeyInfo.modulus << ")");

        //step 3: verify signature
        SignatureVerificationData verification;
        verification.sig = (uint8_t*)segments.signature;
        verification.sigSize = segments.signatureSize;
        verification.msg = (uint8_t*)segments.header;
        verification.msgSize = segments.headerSize + 1 + segments.payloadSize;
        verification.mod = (uint8_t*)publicKeyInfo.modulus.c_str();
        verification.modSize = publicKeyInfo.modulus.length();
        verification.exp = (uint8_t*)publicKeyInfo.exponent.c_str();
        verification.expSize = publicKeyInfo.exponent.length();
        rc = verifyJwtSignature(verification);
        if (rc != ERR_OK)
        {
            StringBuilder sb;
            sb.append("\nsignature: ");
            sb.appendN(segments.signature, segments.signatureSize);
            sb.append("\nmessage: ");
            sb.appendN(segments.header, verification.msgSize);
            sb.append("\nmodulus: %s\nexponent: %s", publicKeyInfo.modulus.c_str(), publicKeyInfo.exponent.c_str());
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Failed to verify JWT access token signature with message, modulus, exponent" << sb);
            return rc;
        }
        BLAZE_TRACE_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Verified JWT signature: " << eastl::string(segments.signature, segments.signatureSize).c_str());

        //step 4: decode payload
        NucleusConnect::JwtPayloadInfo payloadInfo;
        rc = decodeJwtSegment(segments.payload, segments.payloadSize, payloadInfo);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Failed to decode JWT payload: " << eastl::string(segments.payload, segments.payloadSize).c_str());
            return rc;
        }
        BLAZE_TRACE_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Decoded JWT payload to " << payloadInfo.getClassName() << payloadInfo);

        if (payloadInfo.getExpirationTime() <= TimeValue::getTimeOfDay().getSec())
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: JWT access token expired at " << TimeValue(payloadInfo.getExpirationTime() * 1000000) << " (" << payloadInfo.getExpirationTime() << "), token is: " << token);
            return OAUTH_ERR_EXPIRED_TOKEN;
        }
        if (blaze_strcmp(payloadInfo.getIssuer(), getTokenIssuer()) != 0)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: JWT access token issuer(" << payloadInfo.getIssuer() << ") does not match configured issuer(" << getTokenIssuer() << ")");
            return OAUTH_ERR_INVALID_TOKEN;
        }

        //step 5: output token info
        response.setClientId(payloadInfo.getNexusClaims().getClientId());
        response.setScope(payloadInfo.getNexusClaims().getScopes());
        response.setExpiresIn(payloadInfo.getExpirationTime() - TimeValue::getTimeOfDay().getSec());
        response.setPidId(payloadInfo.getNexusClaims().getPidId());
        response.setPidType(payloadInfo.getNexusClaims().getPidType());
        response.setAccountId(payloadInfo.getNexusClaims().getUserId());

        PersonaId personaId = INVALID_BLAZE_ID;
        blaze_str2int(payloadInfo.getNexusClaims().getPersonaId(), &personaId);
        response.setPersonaId(personaId);

        response.setDeviceId(payloadInfo.getNexusClaims().getDeviceId());
        response.setConsoleEnv(payloadInfo.getNexusClaims().getConsoleEnv());
        response.setStopProcess(payloadInfo.getNexusClaims().getStopProcess());
        response.setIsUnderage(payloadInfo.getNexusClaims().getUnderage());
        response.setAuthSource(payloadInfo.getNexusClaims().getAuthSource());
        response.setConnectionType(payloadInfo.getNexusClaims().getConnectionType());

        response.getIpGeoLocation().setIpAddress(payloadInfo.getNexusClaims().getIpGeoLocation().getIpAddress());
        response.getIpGeoLocation().setCountry(payloadInfo.getNexusClaims().getIpGeoLocation().getCountry());
        response.getIpGeoLocation().setRegion(payloadInfo.getNexusClaims().getIpGeoLocation().getRegion());
        response.getIpGeoLocation().setCity(payloadInfo.getNexusClaims().getIpGeoLocation().getCity());
        response.getIpGeoLocation().setLatitude(payloadInfo.getNexusClaims().getIpGeoLocation().getLatitude());
        response.getIpGeoLocation().setLongitude(payloadInfo.getNexusClaims().getIpGeoLocation().getLongitude());
        response.getIpGeoLocation().setTimeZone(payloadInfo.getNexusClaims().getIpGeoLocation().getTimeZone());
        response.getIpGeoLocation().setISPName(payloadInfo.getNexusClaims().getIpGeoLocation().getIspProvider());

        BLAZE_TRACE_LOG(Log::OAUTH, "[JwtUtil].decodeJwtAccessToken: Successfully decoded JWT access token to " << response.getClassName() << response);
        return ERR_OK;
    }

    bool JwtUtil::getJwtSegmentInfo(const char8_t* token, JwtSegmentInfo& segInfo)
    {
        if (token == nullptr || token[0] == '\0')
            return false;

        //header
        segInfo.header = (char8_t*)token;
        char8_t* ptr = blaze_strstr(segInfo.header, ".");
        if (ptr == nullptr)
            return false;
        segInfo.headerSize = (uint32_t)(ptr - segInfo.header);
        if (segInfo.headerSize == 0)
            return false;

        //payload
        segInfo.payload = ptr + 1;
        ptr = blaze_strstr(segInfo.payload, ".");
        if (ptr == nullptr)
            return false;
        segInfo.payloadSize = (uint32_t)(ptr - segInfo.payload);
        if (segInfo.payloadSize == 0)
            return false;

        //signature
        segInfo.signature = ptr + 1;
        ptr = blaze_strstr(segInfo.signature, ".");
        if (ptr != nullptr)
            return false;
        segInfo.signatureSize = strlen(segInfo.signature);
        if (segInfo.signatureSize == 0)
            return false;

        return true;
    }

    //seg may not be null terminated
    BlazeRpcError JwtUtil::decodeJwtSegment(const char8_t* seg, const int32_t segSize, EA::TDF::Tdf& tdf)
    {
        RawBuffer segDecoded(segSize + 1);
        int32_t segDecodedSize = Base64DecodeUrl(seg, segSize, (char *)segDecoded.tail(), segDecoded.capacity());
        if (segDecodedSize == 0)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtSegment: Failed to base64url decode JWT segment " << tdf.getClassName() << ": " << eastl::string(seg, segSize).c_str());
            return OAUTH_ERR_INVALID_TOKEN;
        }
        segDecoded.tail()[segDecodedSize] = '\0';
        segDecoded.put(segDecodedSize);

        Blaze::TdfDecoder* jsonDecoder = static_cast<Blaze::TdfDecoder*>(DecoderFactory::create(Decoder::JSON));
        if (jsonDecoder == nullptr)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtSegment: Failed to create json decoder");
            return ERR_SYSTEM;
        }
        BlazeRpcError rc = jsonDecoder->decode(segDecoded, tdf);
        delete jsonDecoder;
        jsonDecoder = nullptr;
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].decodeJwtSegment: Failed to decode json to " << tdf.getClassName() << "with error(" << ErrorHelp::getErrorName(rc) << "), json is: " << eastl::string((char8_t*)segDecoded.head(), segDecoded.size()).c_str());
            return rc;
        }

        return rc;
    }

    BlazeRpcError JwtUtil::getPublicKeyInfo(const char8_t* keyId, PublicKeyInfo& keyInfo)
    {
        if (mIsRefreshingPublicKeyInfo)
        {
            BLAZE_TRACE_LOG(Log::OAUTH, "[JwtUtil].getPublicKeyInfo: Waiting for in-progress refreshing of JWT public key info. Key id(" << keyId << ").");
            BlazeRpcError rc = mPublicKeyInfoRefreshWaitList.wait("JwtUtil::getPublicKeyInfo");
            if (rc != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].getPublicKeyInfo: Unable to get public key info for key id(" << keyId << ") with error(" << ErrorHelp::getErrorName(rc) << ")");
                return rc;
            }
        }

        PublicKeyInfoMap::const_iterator iter = mPublicKeyInfoMap.find(keyId);
        if (iter != mPublicKeyInfoMap.end())
        {
            keyInfo = iter->second;
            return ERR_OK;
        }

        //check public key info refresh rate limit, if last refresh attempt is too recent then do not refresh cache and key id is considered invalid
        //this rate limit is required by Nucleus to prevent hitting Nucleus API on every login attempt. malicious login attempt can contain fake key id in its jwt access token.
        if (mPublicKeyInfoRefreshAttemptTime + getPublicKeyInfoRefreshMinInterval() > TimeValue::getTimeOfDay())
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].getPublicKeyInfo: Cannot find JWT public key info for key id(" << keyId << ") in cache. Cache refresh attempt was at " << mPublicKeyInfoRefreshAttemptTime << "; refresh minimum interval is " << getPublicKeyInfoRefreshMinInterval().getSec() << " seconds.");
            return OAUTH_ERR_INVALID_TOKEN;
        }

        mIsRefreshingPublicKeyInfo = true;
        BlazeRpcError rc = refreshPublicKeyInfo();
        if (rc != ERR_OK)
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].getPublicKeyInfo: Failed to refresh JWT public key info");

        mIsRefreshingPublicKeyInfo = false;
        mPublicKeyInfoRefreshWaitList.signal(ERR_OK);

        //search cache again
        iter = mPublicKeyInfoMap.find(keyId);
        if (iter == mPublicKeyInfoMap.end())
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].getPublicKeyInfo: Cannot find JWT public key info for key id(" << keyId << "). Key id is invalid.");
            return OAUTH_ERR_INVALID_TOKEN;
        }

        keyInfo = iter->second;
        return ERR_OK;
    }

    BlazeRpcError JwtUtil::refreshPublicKeyInfo()
    {
        mPublicKeyInfoRefreshAttemptTime = TimeValue::getTimeOfDay();
        BLAZE_INFO_LOG(Log::OAUTH, "[JwtUtil].refreshPublicKeyInfo: Refreshing JWT public key info");

        //get public key info from nucleus
        NucleusConnect::GetJwtPublicKeyInfoResponse response;
        BlazeRpcError rc = mOAuthSlave->getJwtPublicKeyInfo(response);
        if (rc != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].refreshPublicKeyInfo: Failed to get JWT public key info via RPC getJwtPublicKeyInfo");
            return rc;
        }

        //update cache
        mPublicKeyInfoMap.clear();
        for (NucleusConnect::JwtPublicKeyInfoList::const_iterator it = response.getJwtPublicKeyInfoList().begin(); it != response.getJwtPublicKeyInfoList().end(); ++it)
        {
            PublicKeyId publicKeyId = (*it)->getKeyId();
            PublicKeyInfo publicKeyInfo;
            publicKeyInfo.modulus = (*it)->getModulus();
            publicKeyInfo.exponent = (*it)->getExponent();
            mPublicKeyInfoMap.insert(eastl::make_pair(publicKeyId, publicKeyInfo));
        }

        BLAZE_INFO_LOG(Log::OAUTH, "[JwtUtil].refreshPublicKeyInfo: Refreshed JWT public key info. Cached info for " << response.getJwtPublicKeyInfoList().size() << " keys.");
        return ERR_OK;
    }

    BlazeRpcError JwtUtil::verifyJwtSignature(const SignatureVerificationData& data)
    {
        RawBuffer sigDecoded(data.sigSize + 1);
        int32_t sigDecodedSize = Base64DecodeUrl((char *)data.sig, data.sigSize, (char *)sigDecoded.tail(), sigDecoded.capacity());
        if (sigDecodedSize == 0)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifyJwtSignature: Failed to base64url decode JWT signature: " << eastl::string((char8_t*)data.sig, data.sigSize).c_str());
            return OAUTH_ERR_INVALID_TOKEN;
        }
        sigDecoded.tail()[sigDecodedSize] = '\0';
        sigDecoded.put(sigDecodedSize);

        RawBuffer modDecoded(data.modSize + 1);
        int32_t modDecodedSize = Base64DecodeUrl((char *)data.mod, data.modSize, (char *)modDecoded.tail(), modDecoded.capacity());
        if (modDecodedSize == 0)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifyJwtSignature: Failed to base64url decode public key modulus: " << eastl::string((char8_t*)data.mod, data.modSize).c_str());
            return ERR_SYSTEM;
        }
        modDecoded.tail()[modDecodedSize] = '\0';
        modDecoded.put(modDecodedSize);

        RawBuffer expDecoded(data.expSize + 1);
        int32_t expDecodedSize = Base64DecodeUrl((char *)data.exp, data.expSize, (char *)expDecoded.tail(), expDecoded.capacity());
        if (expDecodedSize == 0)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifyJwtSignature: Failed to base64url decode public key exponent: " << eastl::string((char8_t*)data.exp, data.expSize).c_str());
            return ERR_SYSTEM;
        }
        expDecoded.tail()[expDecodedSize] = '\0';
        expDecoded.put(expDecodedSize);

        SignatureVerificationData verification;
        verification.sig = sigDecoded.head();
        verification.sigSize = sigDecodedSize;
        verification.msg = data.msg;
        verification.msgSize = data.msgSize;
        verification.mod = modDecoded.head();
        verification.modSize = modDecodedSize;
        verification.exp = expDecoded.head();
        verification.expSize = expDecodedSize;
        if (!verifySignatureRS256(verification))
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifyJwtSignature: Failed to verify JWT signature: " << eastl::string((char8_t*)data.sig, data.sigSize).c_str());
            return OAUTH_ERR_INVALID_TOKEN;
        }

        return ERR_OK;
    }

    //Description: Verify RS256 digital signature
    //Input data - Data needed for verification including signature, original message, mod, and exp
    //Output bool - true on successful verification, false on failed verification
    bool JwtUtil::verifySignatureRS256(const SignatureVerificationData& data)
    {
        bool result = false;
        BIGNUM* modBigNum = BN_bin2bn(data.mod, data.modSize, nullptr);
        BIGNUM* expBigNum = BN_bin2bn(data.exp, data.expSize, nullptr);

        RSA* rsa = RSA_new();
        //RSA_set0_key transfers memory management of modBigNum and expBigNum to the RSA object, memory allocated to modBigNum and expBigNum will be freed when RSA_free is called
        if (RSA_set0_key(rsa, modBigNum, expBigNum, nullptr) != OPENSSL_SUCCESS)
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifySignatureRS256: Failed to set mod and exp to rsa");

        EVP_PKEY* key = EVP_PKEY_new();
        if (EVP_PKEY_set1_RSA(key, rsa) != OPENSSL_SUCCESS)
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifySignatureRS256: Failed to set EVP_PKEY reference to rsa");

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, key) != OPENSSL_SUCCESS)
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifySignatureRS256: Failed to initialize digest context");

        if (EVP_DigestVerifyUpdate(ctx, data.msg, data.msgSize) != OPENSSL_SUCCESS)
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifySignatureRS256: Failed to update digest context");

        if (EVP_DigestVerifyFinal(ctx, data.sig, data.sigSize) != OPENSSL_SUCCESS)
        {
            BLAZE_ERR_LOG(Log::OAUTH, "[JwtUtil].verifySignatureRS256: Failed to verify signature");
        }
        else
        {
            BLAZE_TRACE_LOG(Log::OAUTH, "[JwtUtil].verifySignatureRS256: Successfully verified signature");
            result = true;
        }

        if (ctx != nullptr)
        {
            EVP_MD_CTX_free(ctx);
            ctx = nullptr;
        }
        if (key != nullptr)
        {
            EVP_PKEY_free(key);
            key = nullptr;
        }
        if (rsa != nullptr)
        {
            RSA_free(rsa);
            rsa = nullptr;
        }

        return result;
    }

    //copied from dirtysdk base64.c
    //Description: Decode a Base - 64 encoded url string.
    //Input *pInput - the Base - 64 encoded string
    //Input iInputLen - the length of the encoded input
    //Input *pOutput - [out] the decoded string, or NULL to calculate output size
    //Input iOutputLen - size of output buffer
    //Output int32_t - decoded size on success, zero on error. If pOutput is NULL, return the number of characters required to buffer the output, or zero on error.
    int32_t JwtUtil::Base64DecodeUrl(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen)
    {
        return(_Base64Decode(pInput, iInputLen, pOutput, iOutputLen, _Base64_strDecodeUrl));
    }

    //copied from dirtysdk base64.c
    //Description: Decode a Base - 64 encoded string.
    //Input *pInput - the Base - 64 encoded string
    //Input iInputLen - the length of the encoded input
    //Input *pOutput - [out] the decoded string, or NULL to calculate output size
    //Input iOutputLen - size of output buffer
    //Input *pDecode - the table to use for the decoding
    //Output int32_t - decoded size on success, zero on error. If pOutput is NULL, return the number of characters required to buffer the output, or zero on error.
    int32_t JwtUtil::_Base64Decode(const char *pInput, int32_t iInputLen, char *pOutput, int32_t iOutputLen, const signed char *pDecode)
    {
        char ci[4];
        int8_t co0, co1, co2, co3;
        int32_t iInputCnt, iInputOff, iOutputOff;

        for (iInputOff = 0, iOutputOff = 0; iInputOff < iInputLen; )
        {
            // scan input
            for (iInputCnt = 0; (iInputCnt < 4) && (iInputOff < iInputLen) && (pInput[iInputOff] != '\0') && (iOutputOff < iOutputLen); iInputOff += 1)
            {
                // ignore whitespace
                if ((pInput[iInputOff] == ' ') || (pInput[iInputOff] == '\t') || (pInput[iInputOff] == '\r') || (pInput[iInputOff] == '\n'))
                {
                    continue;
                }
                // basic range validation
                else if ((pInput[iInputOff] < '+') || (pInput[iInputOff] > 'z'))
                {
                    return(0);
                }
                // fetch input character
                else
                {
                    ci[iInputCnt++] = pInput[iInputOff];
                }
            }
            // if we didn't get anything, we're done
            if (iInputCnt == 0)
            {
                break;
            }
            if (iInputCnt < 4)
            {
                // error if we did not get at least 2
                if (iInputCnt < 2)
                {
                    return(0);
                }
                // otherwise for everything under the quad, default to padding
                if (iInputCnt < 3)
                {
                    ci[2] = '=';
                }
                ci[3] = '=';
            }
            // decode the sequence
            co0 = pDecode[(int32_t)ci[0] - '+'];
            co1 = pDecode[(int32_t)ci[1] - '+'];
            co2 = pDecode[(int32_t)ci[2] - '+'];
            co3 = pDecode[(int32_t)ci[3] - '+'];

            if ((co0 >= 0) && (co1 >= 0))
            {
                if ((co2 >= 0) && (co3 >= 0))
                {
                    if ((pOutput != nullptr) && ((iOutputLen - iOutputOff) > 2))
                    {
                        pOutput[iOutputOff + 0] = (co0 << 2) | ((co1 >> 4) & 0x3);
                        pOutput[iOutputOff + 1] = (co1 & 0x3f) << 4 | ((co2 >> 2) & 0x3F);
                        pOutput[iOutputOff + 2] = ((co2 & 0x3) << 6) | co3;
                    }
                    iOutputOff += 3;
                }
                else if ((co2 >= 0) && (ci[3] == '='))
                {
                    if ((pOutput != nullptr) && ((iOutputLen - iOutputOff) > 1))
                    {
                        pOutput[iOutputOff + 0] = (co0 << 2) | ((co1 >> 4) & 0x3);
                        pOutput[iOutputOff + 1] = (co1 & 0x3f) << 4 | ((co2 >> 2) & 0x3F);
                    }
                    iOutputOff += 2;
                    // consider input complete
                    iInputOff = iInputLen;
                }
                else if ((ci[2] == '=') && (ci[3] == '='))
                {
                    if ((pOutput != nullptr) && ((iOutputLen - iOutputOff) > 0))
                    {
                        pOutput[iOutputOff + 0] = (co0 << 2) | ((co1 >> 4) & 0x3);
                    }
                    iOutputOff += 1;
                    // consider input complete
                    iInputOff = iInputLen;
                }
                else
                {
                    // illegal input character
                    return(0);
                }
            }
            else
            {
                // illegal input character
                return(0);
            }
        }
        // return length of decoded data or zero if decoding failed
        return((iInputOff == iInputLen) ? iOutputOff : 0);
    }

    const char8_t* JwtUtil::getTokenIssuer()
    {
        return mOAuthSlave->getConfig().getJwtConfig().getTokenIssuer();
    }

    const TimeValue& JwtUtil::getPublicKeyInfoRefreshMinInterval()
    {
        return mOAuthSlave->getConfig().getJwtConfig().getPublicKeyInfoRefreshMinInterval();
    }

} //namespace OAuth
} //namespace Blaze
