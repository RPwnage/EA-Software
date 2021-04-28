/**************************************************************************************************/
/*! 
    \file s2smanager.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

// Include files
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/s2smanager/s2smanager.h"
#include "BlazeSDK/util/utilapi.h"

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazeerrortype.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

#include "BlazeSDK/blazesdkdefs.h"
#include "shared/framework/protocol/shared/heat2encoder.h"
#include "shared/framework/protocol/shared/heat2decoder.h"

#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/dirtysock/netconn.h"

namespace Blaze
{
namespace S2SManager
{

S2SManager* S2SManager::create(BlazeHub* hub)
{
    return BLAZE_NEW(MEM_GROUP_FRAMEWORK, "S2SManager") S2SManager(hub, MEM_GROUP_FRAMEWORK);
}

S2SManager::S2SManager(BlazeHub *hub, MemoryGroupId memGroupId/* = MEM_GROUP_FRAMEWORK_TEMP */)
    : mHub(hub),
      mMemGroup(memGroupId),
      mTokenReq(nullptr),
      mData(0),
      mRequestInProgress(false)
{
}

S2SManager::~S2SManager()
{
    mHub->removeIdler(this);
    if (mTokenReq != nullptr)
    {
        if (mRequestInProgress)
        {
            BLAZE_SDK_DEBUGF("[S2SManager:%p].dtor: Warning: Destroying S2SManager while a request is pending; the request will be cancelled.\n", this);
            ProtoHttpAbort(mTokenReq);
            mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, ERR_CANCELED, "");
        }
        ProtoHttpDestroy(mTokenReq);
    }
}

BlazeError S2SManager::initialize(const char8_t* certData, const char8_t* keyData, const char8_t* configOverrideInstanceName /*= nullptr*/, const char8_t* identityConnectHost /*= nullptr*/)
{
    if(certData == nullptr || certData[0] == '\0' || keyData == nullptr || keyData[0] == '\0')
    {
        BLAZE_SDK_DEBUGF("[S2SManager:%p].initialize: Missing certificate or key data\n", this);
        return ERR_SYSTEM;
    }
    if (identityConnectHost != nullptr)
    {
        mTokenUrl = identityConnectHost;
    }
    else
    {
        ConnectionManager::ConnectionManager* connMgr = mHub->getConnectionManager();
        if (connMgr == nullptr)
        {
            BLAZE_SDK_DEBUGF("[S2SManager:%p].initialize: No Identity Connect host provided or obtainable from server configs (SDK is not connected)\n", this);
            return SDK_ERR_NOT_CONNECTED;
        }
        const char8_t* host;
        if (!connMgr->getServerConfigString(BLAZESDK_CONFIG_KEY_IDENTITY_CONNECT_HOST_TRUSTED, &host))
        {
            BLAZE_SDK_DEBUGF("[S2SManager:%p].initialize: No Identity Connect host provided or obtainable from server configs\n", this);
            return ERR_SYSTEM;
        }
        mTokenUrl = host;
    }
    if (isInitialized())
    {
        mHub->removeIdler(this);
        if (mRequestInProgress)
        {
            BLAZE_SDK_DEBUGF("[S2SManager:%p].initialize: Warning: Re-initializing S2SManager while a request is pending; the request will be cancelled.\n", this);
            ProtoHttpAbort(mTokenReq);
            mRequestInProgress = false;
            mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, ERR_CANCELED, "");
        }
        ProtoHttpDestroy(mTokenReq);
        mTokenReq = nullptr;
    }

    mTokenUrl.append("/connect/token");

    mTokenReq = ProtoHttpCreate(S2S_PROTO_HTTP_BUFFER_SIZE);
    ProtoHttpControl(mTokenReq, 'spam', 4, 0, nullptr);
    ProtoHttpControl(mTokenReq, 'scrt', (int32_t)strlen(certData), 0, (void*)certData);
    ProtoHttpControl(mTokenReq, 'skey', (int32_t)strlen(keyData), 0, (void*)keyData);

    ProtoHttpControl(mTokenReq, 'apnd', 0, 0, (void*)"Content-Type: application/x-www-form-urlencoded\r\nenable-client-cert-auth: true");
    ProtoHttpControl(mTokenReq, 'rmax', 0, 0, nullptr);

    //use config overrides found in util.cfg
    if (configOverrideInstanceName != nullptr)
    {
        Blaze::Util::UtilAPI::createAPI(*mHub);
        mHub->getUtilAPI()->OverrideConfigs(mTokenReq, configOverrideInstanceName);
    }

    return beginTokenRequest();
}

BlazeError S2SManager::beginTokenRequest()
{
    if (mRequestInProgress)
        return ERR_OK;

    mRequestInProgress = true;
    mData.reset();
    const char8_t* data = "grant_type=client_credentials";
    size_t dataSize = strlen(data);
    uint8_t* pData = mData.acquire(dataSize);
    memcpy(pData, data, dataSize);
    pData[dataSize] = '\0';
    mData.put(dataSize);

    BLAZE_SDK_DEBUGF("[S2SManager:%p].beginTokenRequest: Sending POST request of size %u to %s with data %s\n", this, (uint32_t)mData.datasize(), mTokenUrl.c_str(), (char8_t*)mData.data());

    int32_t bytesSent = ProtoHttpPost(mTokenReq, mTokenUrl.c_str(), (char8_t*)mData.data(), mData.datasize(), 0);

    if (bytesSent < 0) 
    {
        // error
        BLAZE_SDK_DEBUGF("[S2SManager:%p].beginTokenRequest: ProtoHttp POST request failed with %d.\r\n", this, bytesSent);
        ProtoHttpAbort(mTokenReq);
        mRequestInProgress = false;
        mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, SDK_ERR_S2S_FAILURE, "");
        return SDK_ERR_S2S_FAILURE;
    }
    else 
    {
        // result is number of bytes sent.
        mData.pull(bytesSent);
        BLAZE_SDK_DEBUGF("[S2SManager:%p].beginTokenRequest: ProtoHttp POST request has %d sent size.\r\n", this, bytesSent)
    }

    mHub->addIdler(this, "S2SManager");
    return ERR_OK;
}

void S2SManager::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    if (mData.datasize() > 0)
    {
        BLAZE_SDK_SCOPE_TIMER("S2SManager_httpSend");

        int32_t bytesSent = ProtoHttpSend(mTokenReq, (char8_t*)mData.data(), (int32_t)mData.datasize());
        if (bytesSent < 0) 
        {
            BLAZE_SDK_DEBUGF("[S2SManager:%p].idle: - ProtoHttp request failed to send data. ProtoHttpSend return code: %d.\r\n", this, bytesSent);
            mData.reset();
        }
        else
        {
            mData.pull(bytesSent);
        }
    }

    {
        BLAZE_SDK_SCOPE_TIMER("S2SManager_httpUpdate");
        ProtoHttpUpdate(mTokenReq);
    }

    int32_t done = ProtoHttpStatus(mTokenReq, 'done', nullptr, 0);

    if (done != 0)
    {
        BLAZE_SDK_SCOPE_TIMER("S2SManager_httpStatusDone");

        // We're done. Either an error occurred or everything's fine; either way we
        // need to reset the idler state.
        mHub->removeIdler(this);

        if (done > 0)
        {
            int32_t httpStatusCode = ProtoHttpStatus(mTokenReq, 'code', nullptr, 0);
            int32_t dataSize = ProtoHttpStatus(mTokenReq, 'data', nullptr, 0) + 1;
            char8_t* recvBuf = BLAZE_NEW_ARRAY(char8_t, dataSize, MEM_GROUP_FRAMEWORK_TEMP, "S2SManager::recvBuf");
            if (httpStatusCode == 200)
            {
                ProtoHttpRecvAll(mTokenReq, recvBuf, dataSize);
                BLAZE_SDK_DEBUGF("[S2SManager:%p].idle: httprecvall -> %s\n", this, recvBuf);

                mAccessToken = "NEXUS_S2S ";
                const char8_t* accessTokenKey = "\"access_token\" : \"";
                char8_t* tokenPair = blaze_stristr(recvBuf, accessTokenKey);
                tokenPair += strlen(accessTokenKey);
                char8_t* tokenEnd = blaze_stristr(tokenPair, "\"");
                mAccessToken.append(tokenPair, tokenEnd);

                const char8_t* expiryKey = "\"expires_in\" : ";
                char8_t* expiryPair = blaze_stristr(recvBuf, expiryKey);
                expiryPair += strlen(expiryKey);
                uint32_t expirySecs;
                blaze_str2int(expiryPair, &expirySecs);
                mTokenExpiry = TimeValue::getTimeOfDay() + expirySecs*1000*1000;

                mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, ERR_OK, mAccessToken.c_str());
            }
            else
            {
                BLAZE_SDK_DEBUGF("[S2SManager:%p].idle: Got unexpected response from Nucleus 2.0. Response code (%" PRIi32 ")\n", this, httpStatusCode);

                int32_t headersSize = ProtoHttpStatus(mTokenReq, 'head', nullptr, 0) + 1;
                char8_t *headers = BLAZE_NEW_ARRAY(char8_t, headersSize, MEM_GROUP_FRAMEWORK_TEMP, "S2SManager::headers");
                ProtoHttpStatus(mTokenReq, 'htxt', headers, headersSize);
                BLAZE_SDK_DEBUGF("[S2SManager:%p].idle: httprecvall -> headers: %s\n", this, headers);
                BLAZE_DELETE_ARRAY(MEM_GROUP_FRAMEWORK_TEMP, headers);

                if (ProtoHttpRecvAll(mTokenReq, recvBuf, dataSize) >= 0)
                {
                    BLAZE_SDK_DEBUGF("[S2SManager:%p].idle: httprecvall -> %s\n", this, recvBuf);
                }

                mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, SDK_ERR_S2S_FAILURE, "");
            }

            BLAZE_DELETE_ARRAY(MEM_GROUP_FRAMEWORK_TEMP, recvBuf);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[S2SManager:%p].idle: Unable to get S2S access token from Nucleus. essl(%d), serrr(%d) \n", this, ProtoHttpStatus(mTokenReq, 'essl', nullptr, 0), ProtoHttpStatus(mTokenReq, 'serr', nullptr, 0));
            ProtoHttpAbort(mTokenReq);
            mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, SDK_ERR_S2S_FAILURE, "");
        }

        mRequestInProgress = false;
    }
}

void S2SManager::getS2SToken()
{
    if (!isInitialized())
    {
        BLAZE_SDK_DEBUGF("[S2SManager:%p].getS2SToken: S2SManager is not initialized!\n", this);
        mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, ERR_SYSTEM, "");
    }
    else if (hasValidAccessToken())
    {
        mDispatcher.dispatch("onFetchedS2SToken", &S2SManagerListener::onFetchedS2SToken, ERR_OK, mAccessToken.c_str());
    }
    else
    {
        beginTokenRequest();
    }
}

} // namespace S2SManager
} // namespace Blaze
