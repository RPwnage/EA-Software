/*************************************************************************************************/
/*!
    \file   achievementsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class AchievementsSlaveImpl

    Implements the slave portion of the achievements component.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "openssl/sha.h"
#include "openssl/hmac.h"
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/util/shared/outputstream.h" // for OutputStream in sha256ToBase64Buffer
#include "framework/util/shared/base64.h"
#include "framework/util/hashutil.h"
#include "framework/connection/outboundhttpservice.h"
#include "achievements/achievementsslaveimpl.h"
#include "framework/oauth/accesstokenutil.h"

namespace Blaze
{
namespace Achievements
{

static const char8_t* mPoolNames[AchievementsSlaveImpl::ConnectionManagerType::MAX] =
{
    "UntrustedAchievementConn",
    "TrustedAchievementConn"
};

static const eastl::string NEXUS_S2S_ALLOWED_SCOPES = "gos_achievements_enduser gos_achievements_post";
static const int64_t SECONDS_TO_USECONDS = 1000 * 1000;

/*** Public Methods ******************************************************************************/
// static
AchievementsSlave* AchievementsSlave::createImpl()
{
    return BLAZE_NEW_NAMED("AchievementsSlaveImpl") AchievementsSlaveImpl();
}

/*************************************************************************************************/
/*!
    \brief ~AchievementsSlaveImpl

    Destroys the slave achievements component.
*/
/*************************************************************************************************/

void AchievementsSlaveImpl::generateSignature(uint8_t* body, size_t bodyLen, int64_t creationTime, const char8_t* secretKey, char8_t* signature, size_t signatureLen) const
{
    if (signature == nullptr || signatureLen < HashUtil::SHA256_STRING_OUT)
        return;

    RawBuffer buffer(bodyLen + 4);
    uint8_t* data = buffer.acquire(4);
    if (data != nullptr)
    {
        data[0] = (creationTime >> 24) & 0xFF;
        data[1] = (creationTime >> 16) & 0xFF;
        data[2] = (creationTime >> 8) & 0XFF;
        data[3] = (creationTime & 0XFF);
        buffer.put(4);
    }

    data = buffer.acquire(bodyLen + 1);
    if (data != nullptr)
    {
        memcpy(data, body, bodyLen);
        data[bodyLen] = '\0';
        buffer.put(bodyLen);
    }

    unsigned char digest[SHA256_DIGEST_LENGTH];

    HMAC_CTX* ctx = HMAC_CTX_new();
    unsigned int len;
    HMAC_Init_ex(ctx, secretKey, strlen(secretKey), EVP_sha256(), nullptr);
    HMAC_Update(ctx, buffer.data(), buffer.datasize());
    HMAC_Final(ctx, digest, &len);
    HMAC_CTX_free(ctx);

    // convert to readable portable digits.
    HashUtil::sha256ResultToReadableString(digest, signature, HashUtil::SHA256_STRING_OUT);
}

// Helper class for converting sha256 result to readable, portable string
class sha256ToBase64Buffer : public OutputStream
{
public:
    sha256ToBase64Buffer(char *resultBuf, size_t resultBufLen) : mResultBuf(resultBuf), mResultBufLen(resultBufLen), mNumWritten(0) {}
    ~sha256ToBase64Buffer() override {}
    uint32_t write(uint8_t data) override
    {
        if (mResultBufLen <= mNumWritten)
            return 0;

        mResultBuf[mNumWritten++] = data;
        return 1;
    }
private:
    char* mResultBuf;
    size_t mResultBufLen;
    size_t mNumWritten;
};

bool AchievementsSlaveImpl::onConfigure()
{
    for (uint16_t i = 0; i < ConnectionManagerType::MAX; i++)
    {
        const char8_t* name = mPoolNames[i];
        if (gOutboundHttpService->getConnection(name) == nullptr)
        {
            FATAL_LOG("[AchievementsSlaveImpl].onConfigure: Achievements component has dependency on http service '" << name <<
                "'. Please make sure this service is defined in framework.cfg.");
            return false;
        }
    }

    return true;
}

bool AchievementsSlaveImpl::onReconfigure()
{
    return onConfigure();
}

BlazeRpcError AchievementsSlaveImpl::fetchUserOrServerAuthToken(AuxiliaryAuthentication& auxAuth, bool isTrustedConnection)
{
    BlazeRpcError err = ERR_SYSTEM;
    if (!isTrustedConnection)
    {
        // Fetch the Nucleus 2.0 auth token for this user to pass to the achievements service
        // As per achievements service folks, access token stuffed in X-AuthToken header (which is what we use) does not need a type prefix. If we were to stuff it
        // in the Authorization header, it'd need 'Bearer' prefix. We should probably do that but not changing existing code here.
        OAuth::AccessTokenUtil tokenUtil;
        err = tokenUtil.retrieveCurrentUserAccessToken(OAuth::TOKEN_TYPE_NONE); 
        if (err == ERR_OK)
        {
            auxAuth.setAuthToken(tokenUtil.getAccessToken());
        }
    }
    else
    {
        //Since we're a trusted server connection, we set the super user privilege to fetch auth token with all default scopes
        UserSession::SuperUserPermissionAutoPtr ptr(true);

        // Fetch the Nucleus 2.0 access token for the server to pass to the achievements service with the specified allowed scopes
        OAuth::AccessTokenUtil tokenUtil;
        err = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NEXUS_S2S, NEXUS_S2S_ALLOWED_SCOPES.c_str());
        if (err == ERR_OK)
        {
            auxAuth.setTrustedAuthToken(tokenUtil.getAccessToken());
        }
    }

    return err;
}

HttpConnectionManagerPtr AchievementsSlaveImpl::getConnectionManagerPtr(ConnectionManagerType type) const
{
    if (type != ConnectionManagerType::MAX)
        return gOutboundHttpService->getConnection(mPoolNames[type]);
    else
        return HttpConnectionManagerPtr();
}

void AchievementsSlaveImpl::scaleAchivementDataTimeValue(AchievementData& data)
{
    // reading micro-seconds and scaling it and setting micro-seconds again is NOT a bug. 
    data.getS().setMicroSeconds(data.getS().getMicroSeconds() * SECONDS_TO_USECONDS);
    data.getU().setMicroSeconds(data.getU().getMicroSeconds() * SECONDS_TO_USECONDS);
    data.getE().setMicroSeconds(data.getE().getMicroSeconds() * SECONDS_TO_USECONDS);

    for (Blaze::Achievements::AchievementData::RequirementDataList::iterator it = data.getRequirements().begin(), itEnd =  data.getRequirements().end(); it != itEnd; ++it)
    {
        (*it)->getLastUpdated().setMicroSeconds((*it)->getLastUpdated().getMicroSeconds() * SECONDS_TO_USECONDS);
    }
}

void AchievementsSlaveImpl::scaleUserHistoryDataTimeValue(UserHistoryData& data)
{
    for (Blaze::Achievements::UserHistoryData::RequirementDataList::iterator it = data.getRequirements().begin(), itEnd = data.getRequirements().end(); it != itEnd; ++it)
    {
        (*it)->getLastUpdated().setMicroSeconds((*it)->getLastUpdated().getMicroSeconds() * SECONDS_TO_USECONDS);
    }
    data.getT().setMicroSeconds(data.getT().getMicroSeconds() * SECONDS_TO_USECONDS);
}
} // Achievements
} // Blaze
