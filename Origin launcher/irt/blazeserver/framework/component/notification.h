/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_NOTIFICATION_H
#define BLAZE_NOTIFICATION_H

/*** Include files *******************************************************************************/

#include "framework/util/shared/rawbuffer.h"
#include "framework/component/blazerpc.h"
#include "EATDF/tdf.h"
#include "EATDF/tdfobjectid.h"
#include "framework/protocol/rpcprotocol.h"
#include "framework/protocol/shared/tdfencoder.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/notifications_server.h"

#include "EASTL/intrusive_ptr.h"
#include "EASTL/hash_map.h"
#include "EASTL/fixed_vector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

struct NotificationInfo;

struct NotificationSendOpts
{
    NotificationSendOpts() : 
        context(0),
        encoder(nullptr),
        responseFormat(RpcProtocol::FORMAT_USE_TDF_NAMES),
        msgNum(0),
        sendImmediately(false),
        onlyEncodeChanges(false)
    {}

    NotificationSendOpts(bool _sendImmediately, uint32_t _msgNum = 0, uint64_t _context = 0, bool _onlyEncodeChanges = false, TdfEncoder* _encoder = nullptr, RpcProtocol::ResponseFormat _responseFormat = RpcProtocol::FORMAT_USE_TDF_NAMES) :
        context(_context),
        encoder(_encoder),
        responseFormat(_responseFormat),
        msgNum(_msgNum),
        sendImmediately(_sendImmediately),
        onlyEncodeChanges(_onlyEncodeChanges)
    {}

    // This object is copied often.
    // We pack the fields to take less space.
    uint64_t context;
    TdfEncoder* encoder;
    RpcProtocol::ResponseFormat responseFormat;
    uint32_t msgNum;
    bool sendImmediately;
    bool onlyEncodeChanges;
};

struct Notification;
typedef eastl::intrusive_ptr<Notification> NotificationPtr;

struct Notification
{
public:
    enum Type { UNKNOWN, NOTIFICATION, EVENT };

public:
    Notification(const NotificationInfo& notificationInfo, const EA::TDF::Tdf* payloadTdf, const NotificationSendOpts& opts);
    Notification(const EventInfo& eventInfo, const EA::TDF::Tdf* payloadTdf, const NotificationSendOpts& opts);
    Notification(const RpcProtocol::Frame& frame, RawBufferPtr payloadBuffer);
    Notification(NotificationDataPtr notificationData);

    EA::TDF::ComponentId getComponentId() const { return mComponentId; }
    NotificationId getNotificationId() const { return mCommandId; }

    Type getType() const { return mType; }
    bool is(const NotificationInfo& notificationInfo) const;
    bool is(const EventInfo& eventInfo) const;

    bool isNotification() const { return (mType == NOTIFICATION); }
    bool isEvent() const { return (mType == EVENT); }

    const NotificationSendOpts& getOpts() const { return mOpts; }
    int32_t getLogCategory() const { return mLogCategory; }

    void setResponseFormat(RpcProtocol::ResponseFormat responseFormat) { mOpts.responseFormat = responseFormat; }
    void setEncoder(TdfEncoder* encoder) { mOpts.encoder = encoder; }

    bool isForUserSession();
    bool isForComponent();
    bool isForComponentReplication();

    Component* getComponent();

    void setUserSessionId(UserSessionId userSessionId);
    UserSessionId getUserSessionId();

    bool canRouteToClient(bool convertIfPassthrough);

    // \brief set/reset notification prefix. Used for event notifications, where the first notification sent through the connection must begin with the <events> tag.
    void setPrefix(const char8_t* prefix, size_t len) { mPrefix = prefix; mPrefixLength = len; }
    void resetPrefix() { mPrefix = nullptr; mPrefixLength = 0; }

    BlazeRpcError extractPayloadTdf(EA::TDF::TdfPtr& payloadTdfPtr, bool onlyDecodeChanges = false)
    {
        if ((mPayloadTypeDesc != nullptr) && !initPayloadTdfPtr(onlyDecodeChanges))
            return ERR_SYSTEM;

        payloadTdfPtr = mPayloadTdfPtr;
        return ERR_OK;
    }

    template<class T>
    BlazeRpcError extractPayloadTdf(EA::TDF::tdf_ptr<T>& payloadTdfPtr, bool onlyDecodeChanges = false)
    {
        if ((mPayloadTypeDesc != nullptr) && !initPayloadTdfPtr(onlyDecodeChanges))
            return ERR_SYSTEM;

        payloadTdfPtr = static_cast<T*>(mPayloadTdfPtr.get());
        return ERR_OK;
    }

    NotificationPtr createSnapshot();

    void filloutNotificationData(NotificationData& notificationData);

    RawBufferPtr encodePayload(TdfEncoder& encoder);
    RawBufferPtr encodePayload(TdfEncoder& encoder, const RpcProtocol::Frame& frame);
    bool obfuscatePlatformInfo(ClientPlatformType platform);

    const EA::TDF::TdfPtr& getPayload() const { return mPayloadTdfPtr; }

private:
    friend class Sliver;
    friend class ComponentStub;
    friend class OutboundRpcConnection;

    Notification();

    void executeLocal();

    RawBufferPtr encodePayloadInternal(TdfEncoder& encoder, const RpcProtocol::Frame* frame);

    bool initPayloadTdfPtr(bool onlyDecodeChanges);

    static Decoder::Type getComplimentType(Encoder::Type encoderType);
    static Encoder::Type getComplimentType(Decoder::Type decoderType);

private:
    Type mType;

    EA::TDF::ComponentId mComponentId;
    CommandId mCommandId;
    const EA::TDF::TypeDescriptionClass* mPayloadTypeDesc;

    NotificationDataPtr mNotificationDataPtr;

    EA::TDF::TdfPtr mPayloadTdfPtr;

    const char8_t* mPrefix;
    size_t mPrefixLength;

    bool mObfuscatePlatformInfo;

    struct PayloadType
    {
        Encoder::Type type : 8;
        Encoder::SubType subType : 8;
        RpcProtocol::ResponseFormat format : 16;

        PayloadType(Encoder::Type _type = Encoder::INVALID, Encoder::SubType _subType = Encoder::MAX_SUBTYPE, RpcProtocol::ResponseFormat _format = RpcProtocol::MAX_RESPONSE_FORMAT) : type(_type), subType(_subType), format(_format) {}
        bool operator<(const PayloadType& other) const
        {
            uint32_t aval = (type << 24) | (subType << 16) | format;
            uint32_t bval = (other.type << 24) | (other.subType << 16) | other.format;
            return aval < bval;
        }
    };

    static const PayloadType msRequiredInputPayloadEncoding;

    typedef eastl::vector_map<PayloadType, RawBufferPtr, eastl::less<PayloadType>, Blaze::BlazeStlAllocator, eastl::fixed_vector<eastl::pair<PayloadType, RawBufferPtr>, 4> > PayloadByTypeMap;
    PayloadByTypeMap mEncodedPayloadByTypeMap;

    NotificationSendOpts mOpts;
    int32_t mLogCategory;

    friend void intrusive_ptr_add_ref(Notification* ptr);
    friend void intrusive_ptr_release(Notification* ptr);
    int32_t mRefCount;
};

inline void intrusive_ptr_add_ref(Notification* ptr)
{
    ++ptr->mRefCount;
}

inline void intrusive_ptr_release(Notification* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

} // namespace Blaze

#endif // BLAZE_NOTIFICATION_H
