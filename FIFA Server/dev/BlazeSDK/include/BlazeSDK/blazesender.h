/*************************************************************************************************/
/*!
    \file blazesender.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_SENDER_H
#define BLAZE_SENDER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/******* Include files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/idler.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/allocdefines.h"
#include "BlazeSDK/alloc.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"
#include "BlazeSDK/component/framework/tdf/protocol.h"
#include "BlazeSDK/component/redirector/tdf/redirectortypes.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/heat2decoder.h"

/******* Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
    namespace TDF
    {
        class Tdf;
    } // namespace TDF
} // namespace EA

namespace Blaze
{

//Use the below "USE_DEFAULT_TIMEOUT" to ensure the connection send-requests timeout used is set by the server.
const uint32_t USE_DEFAULT_TIMEOUT = 0;

class TdfEncoder;
class TdfDecoder;
class Error;
class BlazeHub;
class ComponentManager;
class RpcJobBase;
class Fire2Frame;
class ProtoFire;
namespace ConnectionManager
{
    class ConnectionManager;
}

class BLAZESDK_API BlazeSender : public JobProvider
{
    NON_COPYABLE(BlazeSender);

public:
    enum MessageType { MESSAGE, REPLY, NOTIFICATION, ERROR_REPLY, PING, PING_REPLY };

    static const uint32_t SERVER_ADDRESS_MAX = 255;
    static const uint32_t BUFFER_SIZE_DEFAULT = 32 * 1024;
    static const uint32_t OVERFLOW_BUFFER_SIZE_MIN = 512 * 1024;
    static const uint32_t OUTGOING_BUFFER_SIZE_DEFAULT = 161 * 1024;

    class BLAZESDK_API ServerConnectionInfo
    {
    public:
        ServerConnectionInfo() { reset(); }
        ServerConnectionInfo(const char8_t *serviceName)
        {
            reset(serviceName);
        }
        ServerConnectionInfo(const char8_t *address, uint16_t port, bool secure, bool autoS2S = false)
        {
            reset(nullptr, address, port, secure, autoS2S);
        }
        ServerConnectionInfo(const char8_t *serviceName, const char8_t * address, uint16_t port, bool secure, bool autoS2S = false)
        {
            reset(serviceName, address, port, secure, autoS2S);
        }
        ServerConnectionInfo(const ServerConnectionInfo& connInfo)
        {
            blaze_strnzcpy(mServiceName, connInfo.mServiceName, sizeof(mServiceName));
            blaze_strnzcpy(mAddress, connInfo.mAddress, sizeof(mAddress));
            mPort = connInfo.mPort;
            mSecure = connInfo.mSecure;
            mAutoS2S = connInfo.mAutoS2S;
        }

        void reset(const char8_t *serviceName = nullptr, const char8_t * address = nullptr, uint16_t port = 0, bool secure = false, bool autoS2S = false)
        {
            mServiceName[0] = '\0';
            mAddress[0] = '\0';
            mPort = port;
            mSecure = secure;
            mAutoS2S = autoS2S;

            if (serviceName != nullptr)
            {
                blaze_strnzcpy(mServiceName, serviceName, sizeof(mServiceName));
            }

            if (address != nullptr)
            {
                blaze_strnzcpy(mAddress, address, sizeof(mAddress));
            }
        }

        bool isResolved() const { return ((mAddress[0] != '\0') && (mPort != 0)); }

        const char8_t *getServiceName() const { return mServiceName; }
        const char8_t *getAddress() const { return mAddress; }
        uint16_t getPort() const { return mPort; }
        bool getSecure() const { return mSecure; }
        bool getAutoS2S() const { return mAutoS2S; }

    private:
        friend class BlazeSender;
        char8_t mServiceName[MAX_SERVICENAME_LENGTH + 1];
        char8_t mAddress[SERVER_ADDRESS_MAX + 1];
        uint16_t mPort;
        bool mSecure;
        bool mAutoS2S;
    };

public:
    BlazeSender(BlazeHub& hub);
    virtual ~BlazeSender();

    bool setServerConnInfo(const ServerConnectionInfo &connInfo);
    bool setServerConnInfo(const char8_t *serviceName);
    bool setServerConnInfo(const char8_t *address, uint16_t port, bool secure, bool autoS2S = false);
    bool setServerConnInfo(const char8_t *serviceName, const char8_t *address, uint16_t port, bool secure, bool autoS2S = false);

    const ServerConnectionInfo &getServerConnInfo() const { return mConnInfo; }

    /*! ***************************************************************/
    /*! \brief Returns a component manager for a particular user.

        This function accesses a component manager for a particular channel
        of the multiplexed connection.  The component manager in turn
        returns various components that make up the RPC specification
        of this connection.

        \param userIndex The desired user channel to return.
        \returns The component manager for the given user index, or nullptr
                 if the user index is invalid.
    *******************************************************************/
    ComponentManager *getComponentManager(uint32_t userIndex) const;

    /*! ***************************************************************/
    /*! \brief Returns the time last request was made to the server.
    *******************************************************************/
    uint32_t getLastRequestTime() const { return mLastRequestTime; }

    /*! ***************************************************************/
    /*! \brief Gets the last time anything was received from the server.
    *******************************************************************/
    uint32_t getLastReceiveTime() const { return mLastReceiveTime; }

    /*! ***************************************************************/
    /*! \brief Returns the default request timeout used if USE_DEFAULT_TIMEOUT
        is passed to send request
    *******************************************************************/
    uint32_t getDefaultRequestTimeout() const { return mDefaultRequestTimeout; }

    /*! ***************************************************************/
    /*! \brief Sets the default request timeout used if USE_DEFAULT_TIMEOUT
        is passed to send request
    *******************************************************************/
    void setDefaultRequestTimeout(uint32_t timeout) { mDefaultRequestTimeout = timeout; }

    uint32_t getPendingRpcs() { return mPendingRpcs; }

    const Redirector::DisplayMessageList &getDisplayMessages() const { return mDisplayMessages; }

    virtual JobId sendRequest(uint32_t userIndex, uint16_t component, uint16_t command,
        const EA::TDF::Tdf *request, RpcJobBase *job, const JobId& reserveId, uint32_t timeout);

    virtual bool isActive() = 0;

    static void logMessage(const ComponentManager *manager, bool inbound, bool notification, const char8_t *prefix, const EA::TDF::Tdf *payload,
        uint32_t msgId, uint16_t componentId, uint16_t commandId, BlazeError errorCode = ERR_OK, size_t dataSize = 0);

    uint32_t getOverflowOutgoingBufferSize() const { return mOverflowOutgoingBufferSize; }
    uint32_t getOverflowIncomingBufferSize() const { return mOverflowIncomingBufferSize; }

protected:
    class MultiBuffer : public RawBuffer
    {
    public:
        MultiBuffer(size_t size) : mPrimarySize(size), mPrimaryBuffer(nullptr), mOverflowBuffer(nullptr)
        {
            mPrimaryBuffer = (uint8_t*)BLAZE_ALLOC(mPrimarySize + 1, MEM_GROUP_FRAMEWORK, "MultiBuffer::mPrimaryBuffer");
            // Always set the 'hidden' last byte to '\0'
            mPrimaryBuffer[mPrimarySize] = '\0';
            setBuffer(mPrimaryBuffer, mPrimarySize);
            mAckOffset = mPrimaryBuffer;
        }
        ~MultiBuffer()
        {
            resetToPrimary();

            BLAZE_FREE(MEM_GROUP_FRAMEWORK, mPrimaryBuffer);
            mPrimaryBuffer = nullptr;
            mAckOffset = nullptr;
        }

        void resetToPrimary()
        {
            setBuffer(mPrimaryBuffer, mPrimarySize);
            mAckOffset = data(); // Ack offset points to the data() of primary buffer
            if (mOverflowBuffer != nullptr)
            {
                BLAZE_FREE(MEM_GROUP_FRAMEWORK_TEMP, mOverflowBuffer);
                mOverflowBuffer = nullptr;
            }
        }

        bool useOverflow(size_t needed)
        {
            if (data() - mAckOffset < 0)
                return false;

            size_t currentUnAckedSize = (size_t)(data() - mAckOffset);
            size_t currentDataSize = datasize() + currentUnAckedSize;
            if (currentDataSize + needed <= capacity())
            {
                memmove(head(), mAckOffset, currentDataSize);
                reset();
                put(currentDataSize);
                pull(currentUnAckedSize); // move data pointer
                mAckOffset = head();
            }

            if ((mOverflowBuffer != nullptr) || (needed <= tailroom()))
                return false;
            
            if (needed >= OVERFLOW_BUFFER_SIZE_MIN && needed < currentDataSize)
            {
                BLAZE_SDK_DEBUGSB("[BlazeSender::" << this << "].useOverflow: The overflow buffer needed allocated size(" << needed << ") is less than the current data size(" 
                    << currentDataSize << "). Reset current buffer.");
                reset();
                resetToPrimary();
                return false;
            }

            mOverflowBuffer = (uint8_t*)BLAZE_ALLOC(needed + 1, MEM_GROUP_FRAMEWORK_TEMP, "MultiBuffer::mOverflowBuffer");
            if (mOverflowBuffer == nullptr)
            {
                BLAZE_SDK_DEBUGSB("[BlazeSender::" << this << "].useOverflow: Failed to allocate overflow buffer with size " << needed);
                return false;
            }

            // Always set the 'hidden' last byte to '\0'
            mOverflowBuffer[needed] = '\0';
            memcpy(mOverflowBuffer, ackOffset(), currentDataSize);
            setBuffer(mOverflowBuffer, needed);
            put(currentDataSize);
            pull(currentUnAckedSize); // move data pointer
            mAckOffset = mOverflowBuffer;
            return true;
        }

        uint8_t* ackOffset() const { return mAckOffset; }
        void pullAckOffset(size_t len) { mAckOffset += len; }

    private:
        size_t mPrimarySize;
        uint8_t* mPrimaryBuffer;
        uint8_t* mOverflowBuffer;
        uint8_t* mAckOffset;
    };

protected:
    friend class RpcJobBase;

    inline BlazeHub &getBlazeHub() const { return mHub; }
    inline MultiBuffer &getSendBuffer() { return mSendBuffer; }
    inline MultiBuffer &getReceiveBuffer() { return mReceiveBuffer; }
    inline void incPendingRpcs() { ++mPendingRpcs; }
    inline void decPendingRpcs() { --mPendingRpcs; }
    void resetTransactionData(BlazeError error);

    virtual void onServiceNameResolutionStarted() {}
    virtual void onServiceNameResolutionFinished(BlazeError result) {}

    virtual uint32_t getNextMessageId() = 0;

    virtual BlazeError canSendRequest() = 0;
    virtual BlazeError sendRequestToBuffer(uint32_t userIndex, uint16_t component, uint16_t command, MessageType type, const EA::TDF::Tdf *msg, uint32_t msgId) = 0;
    virtual BlazeError sendRequestFromBuffer() = 0;
    virtual MultiBuffer& chooseSendBuffer() = 0;

    void updateLastReceiveTime(uint32_t currentTime = 0);
    void updateLastRequestTime(uint32_t currentTime = 0);

    void resolveServiceName();

    void handleReceivedPacket(uint32_t msgId, MessageType msgType, ComponentId component, uint16_t command, uint32_t userIndex,
        BlazeError error, TdfDecoder &decoder, const uint8_t *data, size_t dataSize);

    void logMessage(uint32_t userIndex, bool inbound, bool notification, const char8_t *prefix, const EA::TDF::Tdf *payload,
        uint32_t msgId, uint16_t componentId, uint16_t commandId, BlazeError errorCode = ERR_OK, size_t dataSize = 0) const;

    bool prepareReceiveBuffer(size_t size);

private:
    void onRedirectorResponse(BlazeError result, const char8_t* serviceName,
        const Redirector::ServerInstanceInfo* response,
        const Redirector::ServerInstanceError* error, int32_t sslError, int32_t sockError);

private:
    BlazeHub& mHub;

    ServerConnectionInfo mConnInfo;
    Redirector::DisplayMessageList mDisplayMessages;

    ComponentManager *mComponentManagers;

    uint32_t mDefaultRequestTimeout;

    uint32_t mLastRequestTime;
    uint32_t mLastReceiveTime;
    uint32_t mPendingRpcs;

    MultiBuffer mSendBuffer;
    MultiBuffer mReceiveBuffer;

    uint32_t mOverflowOutgoingBufferSize;
    uint32_t mOverflowIncomingBufferSize;
};

}; //Blaze

#endif //BLAZE_SENDER_H


