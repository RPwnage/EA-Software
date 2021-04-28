/*************************************************************************************************/
/*!
    \file httpconnection.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazesender.h"
#include "BlazeSDK/idler.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/s2smanager/s2smanager.h"
#include "framework/protocol/shared/encoder.h"
#include "framework/protocol/shared/protocoltypes.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/shared/restrequestbuilder.h"
#include "framework/util/shared/rawbuffer.h"

#include "DirtySDK/proto/protohttp.h"


namespace Blaze
{

class RestDecoder;

class BLAZESDK_API HttpConnection : public BlazeSender, protected Idler, protected S2SManager::S2SManagerListener
{

public:
    HttpConnection(BlazeHub &hub, Encoder::Type encoderType=Encoder::JSON);
    virtual ~HttpConnection();

    static const int32_t RECEIVE_BUFFER_SIZE_BYTES = 8192;
    static const int32_t SEND_BUFFER_SIZE_BYTES = 4096;
    static const int32_t URI_SIZE = 1024;
    static const int32_t URI_PREFIX_SIZE = 32;
    static const int32_t URL_SIZE = 2048;
    static const char8_t* RPC_ERROR_HEADER_NAME;
    static const char8_t* S2S_AUTH_HEADER_NAME;
    static const int32_t S2S_AUTH_HEADER_NAME_LEN = 14;

    void setAuthenticationData(const char8_t* certData, const char8_t* keyData, size_t certDataSize = 0, size_t keyDataSize = 0);

    // from BlazeSender
    virtual bool isActive();

protected:
    // from BlazeSender
    virtual uint32_t getNextMessageId() { return 0; }

    virtual BlazeError canSendRequest();
    virtual BlazeError sendRequestToBuffer(uint32_t userIndex, uint16_t component, uint16_t command, MessageType type, const EA::TDF::Tdf *msg, uint32_t msgId);
    virtual BlazeError sendRequestFromBuffer();
    virtual MultiBuffer& chooseSendBuffer() { return getSendBuffer(); }
    // from idler
    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);
    // from S2SManagerListener
    virtual void onFetchedS2SToken(BlazeError err, const char8_t* token);

private:
    void createOwnedHeaderString(const RestRequestBuilder::HeaderVector& headerVector, Blaze::string& appendHeader);
    void processReceiveHeader(const char8_t* data, size_t dataSize, HttpHeaderMap& headerMap);
    void constructUrl(char8_t* url, size_t urlSize, const char8_t* uri, const RestRequestBuilder::HttpParamVector& paramVector);
    void finishRequest(MessageType msgType, BlazeError rpcError);

    BlazeError parseResponseRpcError(const HttpHeaderMap &headerMap);
    BlazeError preParseXmlResponse(const HttpHeaderMap &headerMap);
    BlazeError sendRequestFromBufferInternal();

private:
    ProtoHttpRefT *mProtoHttpRef;

    RestDecoder *mRestDecoder;
    Encoder::Type mDefaultEncoderType;

    struct RequestData
    {
        RequestData();
        ~RequestData();

        void reset();

        const RestResourceInfo *restInfo;
        RestRequestBuilder::HttpParamVector paramVector;
        Blaze::string appendHeader;
        StringBuilder uri;
        ComponentId component;
        uint16_t command;
        uint32_t userIndex;
    };

    RequestData mRequestData;
};

}

#endif

