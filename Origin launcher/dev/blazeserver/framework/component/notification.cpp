
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/controller/controller.h"
#include "framework/connection/connectionmanager.h"
#include "framework/component/componentmanager.h"
#include "framework/component/notification.h"
#include "framework/util/shared/blazestring.h"
#include "framework/connection/connection.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/usersessions/usersession.h"
#include "framework/tdf/notifications_server.h"

namespace Blaze
{

const Notification::PayloadType Notification::msRequiredInputPayloadEncoding(Encoder::HEAT2, Encoder::NORMAL, RpcProtocol::FORMAT_USE_TDF_NAMES);

Notification::Notification() :
    mType(UNKNOWN),
    mComponentId(Component::INVALID_COMPONENT_ID),
    mCommandId(Component::INVALID_COMMAND_ID),
    mPayloadTypeDesc(nullptr),
    mPrefix(nullptr),
    mPrefixLength(0),
    mObfuscatePlatformInfo(false),
    mLogCategory(Log::SYSTEM),
    mRefCount(0)
{
}

Notification::Notification(const NotificationInfo& notificationInfo, const EA::TDF::Tdf* payloadTdf, const NotificationSendOpts& opts) :
    mType(NOTIFICATION),
    mComponentId(notificationInfo.componentId),
    mCommandId(notificationInfo.notificationId),
    mPayloadTypeDesc(notificationInfo.payloadTypeDesc),
    mPayloadTdfPtr(const_cast<EA::TDF::Tdf*>(payloadTdf)),
    mPrefix(nullptr),
    mPrefixLength(0),
    mObfuscatePlatformInfo(notificationInfo.obfuscatePlatformInfo),
    mOpts(opts),
    mLogCategory(BlazeRpcLog::getLogCategory(mComponentId)),
    mRefCount(0)
{
}

Notification::Notification(const EventInfo& eventInfo, const EA::TDF::Tdf* payloadTdf, const NotificationSendOpts& opts) :
    mType(EVENT),
    mComponentId(eventInfo.componentId),
    mCommandId(eventInfo.eventId),
    mPayloadTypeDesc(eventInfo.payloadTypeDesc),
    mPayloadTdfPtr(const_cast<EA::TDF::Tdf*>(payloadTdf)),
    mPrefix(nullptr),
    mPrefixLength(0),
    mObfuscatePlatformInfo(false),
    mOpts(opts),
    mLogCategory(BlazeRpcLog::getLogCategory(mComponentId)),
    mRefCount(0)
{
}

Notification::Notification(const RpcProtocol::Frame& frame, RawBufferPtr payloadBuffer) :
    mType(UNKNOWN),
    mComponentId(frame.componentId),
    mCommandId(frame.commandId),
    mPayloadTypeDesc(nullptr),
    mPrefix(nullptr),
    mPrefixLength(0),
    mObfuscatePlatformInfo(false),
    mLogCategory(BlazeRpcLog::getLogCategory(mComponentId)),
    mRefCount(0)
{
    const NotificationInfo* notificationInfo;
    const EventInfo* eventInfo;
    if ((notificationInfo = ComponentData::getNotificationInfo(mComponentId, mCommandId)) != nullptr)
    {
        mType = NOTIFICATION;
        mPayloadTypeDesc = notificationInfo->payloadTypeDesc;
        mObfuscatePlatformInfo = notificationInfo->obfuscatePlatformInfo;
    }
    else if ((eventInfo = ComponentData::getEventInfo(mComponentId, mCommandId)) != nullptr)
    {
        mType = EVENT;
        mPayloadTypeDesc = eventInfo->payloadTypeDesc;
    }
    else
    {
        BLAZE_WARN_LOG(mLogCategory, "Notification.ctor: Unknown notification or event type: componentId(" << mComponentId << "), commandId(" << mCommandId << ")");
    }

    mOpts.sendImmediately = frame.immediate;
    mOpts.context = frame.context;
    mOpts.msgNum = frame.msgNum;
    mOpts.responseFormat = frame.format;

    EA_ASSERT((msRequiredInputPayloadEncoding.type == getComplimentType(frame.requestEncodingType)) && (msRequiredInputPayloadEncoding.format == frame.format));
    mEncodedPayloadByTypeMap[msRequiredInputPayloadEncoding] = payloadBuffer;
}

Notification::Notification(NotificationDataPtr notificationData) :
    mType(UNKNOWN),
    mNotificationDataPtr(notificationData),
    mPrefix(nullptr),
    mPrefixLength(0),
    mObfuscatePlatformInfo(false),
    mLogCategory(notificationData->getLogCategory()),
    mRefCount(0)
{
    EA_ASSERT(mNotificationDataPtr != nullptr);

    const NotificationInfo* notificationInfo;
    const EventInfo* eventInfo;
    if ((notificationInfo = ComponentData::getNotificationInfo(mNotificationDataPtr->getComponentId(), mNotificationDataPtr->getNotificationId())) != nullptr)
    {
        mType = NOTIFICATION;
        mComponentId = notificationInfo->componentId;
        mCommandId = notificationInfo->notificationId;
        mPayloadTypeDesc = notificationInfo->payloadTypeDesc;
        mObfuscatePlatformInfo = notificationInfo->obfuscatePlatformInfo;
    }
    else if ((eventInfo = ComponentData::getEventInfo(mNotificationDataPtr->getComponentId(), mNotificationDataPtr->getNotificationId())) != nullptr)
    {
        mType = EVENT;
        mComponentId = eventInfo->componentId;
        mCommandId = eventInfo->eventId;
        mPayloadTypeDesc = eventInfo->payloadTypeDesc;
    }
    else
    {
        BLAZE_WARN_LOG(mLogCategory, "Notification.ctor: Unknown notification or event type: componentId(" << mComponentId << "), commandId(" << mCommandId << ")");
    }

    Encoder::Type encoderType = Encoder::INVALID;
    switch (mNotificationDataPtr->getEncoderType())
    {
        case NotificationData::HEAT2:   encoderType = Encoder::HEAT2; break;
        case NotificationData::HTTP:    encoderType = Encoder::HTTP; break;
        case NotificationData::JSON:    encoderType = Encoder::JSON; break;
        case NotificationData::XML2:    encoderType = Encoder::XML2; break;
        case NotificationData::EVENTXML:encoderType = Encoder::EVENTXML; break;
        default: break;
    }

    TdfEncoder::SubType encoderSubType = Encoder::MAX_SUBTYPE;
    switch (mNotificationDataPtr->getEncoderSubType())
    {
        case NotificationData::NORMAL:          encoderSubType = TdfEncoder::NORMAL; break;
        case NotificationData::DEFAULTDIFFERNCE:encoderSubType = TdfEncoder::DEFAULTDIFFERNCE; break;
    }

    RpcProtocol::ResponseFormat encoderResponseFormat = RpcProtocol::MAX_RESPONSE_FORMAT;
    switch (mNotificationDataPtr->getEncoderResponseFormat())
    {
        case NotificationData::FORMAT_USE_TDF_NAMES:    encoderResponseFormat = RpcProtocol::FORMAT_USE_TDF_NAMES; break;
        case NotificationData::FORMAT_USE_TDF_RAW_NAMES:encoderResponseFormat = RpcProtocol::FORMAT_USE_TDF_RAW_NAMES; break;
        case NotificationData::FORMAT_USE_TDF_TAGS:     encoderResponseFormat = RpcProtocol::FORMAT_USE_TDF_TAGS; break;
    }

    RawBufferPtr& encodedPayload = mEncodedPayloadByTypeMap[PayloadType(encoderType, encoderSubType, encoderResponseFormat)];
    auto dataSize = mNotificationDataPtr->getData().getSize();
    encodedPayload = BLAZE_NEW RawBuffer(dataSize);
    memcpy(encodedPayload->data(), mNotificationDataPtr->getData().getData(), dataSize);
    encodedPayload->put(dataSize);
}

bool Notification::is(const NotificationInfo& notificationInfo) const
{
    return isNotification() &&
        (mComponentId == notificationInfo.componentId) &&
        (mCommandId == notificationInfo.notificationId);
}

bool Notification::is(const EventInfo& eventInfo) const
{
    return isEvent() &&
        (mComponentId == eventInfo.componentId) &&
        (mCommandId == eventInfo.eventId);
}

bool Notification::canRouteToClient(bool convertIfPassthrough)
{
    if (isForUserSession())
    {
        const NotificationInfo* notificationInfo = ComponentData::getNotificationInfo(mComponentId, mCommandId);
        if (convertIfPassthrough && (notificationInfo->passthroughNotificationId != Component::INVALID_NOTIFICATION_ID))
        {
            const NotificationInfo* passthroughInfo = ComponentData::getNotificationInfo(notificationInfo->passthroughComponentId, notificationInfo->passthroughNotificationId);
            if (passthroughInfo != nullptr)
            {
                notificationInfo = passthroughInfo;

                mComponentId = notificationInfo->componentId;
                mCommandId = notificationInfo->notificationId;
                mPayloadTypeDesc = notificationInfo->payloadTypeDesc;
                mObfuscatePlatformInfo = notificationInfo->obfuscatePlatformInfo;
            }
        }

        if (notificationInfo->isClientExport)
            return true;
    }

    return false;
}

bool Notification::isForUserSession()
{
    return (isNotification() && (mOpts.context != 0));
}

bool Notification::isForComponent()
{
    return (isNotification() && (mOpts.context == 0));
}

bool Notification::isForComponentReplication()
{
    return (isNotification() && (mCommandId >> 4 == 0xFFF));
}

UserSessionId Notification::getUserSessionId()
{
    return (UserSession::isValidUserSessionId(mOpts.context) ? mOpts.context : INVALID_USER_SESSION_ID);
}

void Notification::setUserSessionId(UserSessionId userSessionId)
{
    mOpts.context = userSessionId;
}

NotificationPtr Notification::createSnapshot()
{
    Notification* copy = BLAZE_NEW Notification();

    copy->mType = mType;
    copy->mComponentId = mComponentId;
    copy->mCommandId = mCommandId;;
    copy->mPayloadTypeDesc = mPayloadTypeDesc;
    copy->mNotificationDataPtr = mNotificationDataPtr;
    copy->mObfuscatePlatformInfo = mObfuscatePlatformInfo;
    copy->mEncodedPayloadByTypeMap = mEncodedPayloadByTypeMap;
    copy->mOpts = mOpts;
    copy->mLogCategory = mLogCategory;

    if (mPayloadTdfPtr != nullptr)
        copy->mPayloadTdfPtr = mPayloadTdfPtr->clone();

    return copy;
}

void Notification::filloutNotificationData(NotificationData& notificationData)
{
    TdfEncoder* encoder = gController->getConnectionManager().getOrCreateEncoder(Encoder::HEAT2);
    if (encoder != nullptr)
    {
        notificationData.setComponentId(getComponentId());
        notificationData.setNotificationId(getNotificationId());
        notificationData.setLogCategory(getLogCategory());

        notificationData.setEncoderType(NotificationData::HEAT2);
        notificationData.setEncoderSubType(NotificationData::NORMAL);
        notificationData.setEncoderResponseFormat(NotificationData::FORMAT_USE_TDF_NAMES);

        RawBufferPtr encodedPayloadBuffer = encodePayload(*encoder);
        if (encodedPayloadBuffer != nullptr)
            notificationData.getData().setData(encodedPayloadBuffer->data(), encodedPayloadBuffer->datasize());
    }
}

bool Notification::initPayloadTdfPtr(bool onlyDecodeChanges)
{
    if (mType == UNKNOWN)
        return false;

    if (mPayloadTypeDesc == nullptr)
        return false;

    if (mPayloadTdfPtr != nullptr)
        return true;

    PayloadByTypeMap::iterator it = mEncodedPayloadByTypeMap.find(msRequiredInputPayloadEncoding);
    if ((it != mEncodedPayloadByTypeMap.end()) && (it->second != nullptr))
    {
        Decoder::Type decoderType = getComplimentType(it->first.type);
        TdfDecoder* decoder = gController->getConnectionManager().getOrCreateDecoder(decoderType);
        if (decoder != nullptr)
        {
            mPayloadTdfPtr = mPayloadTypeDesc->createInstance(*Allocator::getAllocator(MEM_GROUP_FRAMEWORK_DEFAULT), "Notification::extractPayloadTdf");

            RawBuffer& rawBuffer = *it->second;
            rawBuffer.mark();
            BlazeRpcError err = decoder->decode(rawBuffer, *mPayloadTdfPtr, onlyDecodeChanges);
            rawBuffer.resetToMark();

            if (err == ERR_OK)
                return true;

            BLAZE_ASSERT_LOG(Log::SYSTEM, "Notification.initPayloadTdfPtr: Decoding a notification payload failed with err(" << LOG_ENAME(err) << "), desc: " << decoder->getErrorMessage());
        }
    }

    BLAZE_ASSERT_LOG(Log::SYSTEM, "Notification.initPayloadTdfPtr: Failed to initialize the notification payload tdf.");

    return false;
}

RawBufferPtr Notification::encodePayload(TdfEncoder& encoder)
{
    return encodePayloadInternal(encoder, nullptr);
}

RawBufferPtr Notification::encodePayload(TdfEncoder& encoder, const RpcProtocol::Frame& frame)
{
    return encodePayloadInternal(encoder, &frame);
}

RawBufferPtr Notification::encodePayloadInternal(TdfEncoder& encoder, const RpcProtocol::Frame* frame)
{
    EA_ASSERT(mType != UNKNOWN);

    if (mPayloadTypeDesc == nullptr)
    {
        // This notification type has no associated payload, so early out.
        return nullptr;
    }

    // Encode payload if one was provided
    RawBufferPtr& result = mEncodedPayloadByTypeMap[PayloadType(encoder.getType(), encoder.getSubType(), frame != nullptr ? frame->format : RpcProtocol::ResponseFormat::FORMAT_USE_TDF_NAMES)];

    if ((result == nullptr) && initPayloadTdfPtr(mOpts.onlyEncodeChanges))
    {
        // TODO_MC: Use the right allocation method.
        if (mPrefix == nullptr)
        {
            result = BLAZE_NEW RawBuffer(0);
        }
        else
        {
            result = BLAZE_NEW RawBuffer(mPrefixLength);
            memcpy(result->data(), mPrefix, mPrefixLength);
            result->put(mPrefixLength);
        }
        bool success = encoder.encode(*result, *mPayloadTdfPtr, frame, mOpts.onlyEncodeChanges);
        EA_ASSERT(success);
    }

    return result;
}

bool Notification::obfuscatePlatformInfo(ClientPlatformType platform)
{
    if (platform == common || !mObfuscatePlatformInfo || !gController->isSharedCluster())
        return false;

    // we may need to decode the payload to do the work
    if (mPayloadTdfPtr == nullptr && !initPayloadTdfPtr(true))
        return false;


    Component* component = gController->getComponentManager().getComponent(getComponentId());
    if (component == nullptr)
        return false;

    if (component->isLocal())
    {
        component->asStub()->obfuscatePlatformInfo(platform, *mPayloadTdfPtr);
        mEncodedPayloadByTypeMap.clear();
    }


    return true;
}

void Notification::executeLocal()
{
    if (isForComponent())
    {
        //Grab the proxy to process this notification
        Component* component = gController->getComponentManager().getComponent(getComponentId());
        if (component != nullptr)
        {
            component->processNotification(*this);
        }
        else
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "Notification.executeLocal: Component (" << getComponentId() << ") not found.");
        }
    }

    if (isForUserSession())
    {
        if (canRouteToClient(true))
        {
            // Check to see if the UserSession is still here, he could have logged out.
            UserSessionMasterPtr userSession = gUserSessionMaster->getLocalSession(getUserSessionId());
            if (userSession != nullptr)
                userSession->sendNotification(*this);
        }
    }
}

Decoder::Type Notification::getComplimentType(Encoder::Type encoderType)
{
    switch (encoderType)
    {
    case Encoder::HEAT2: return Decoder::HEAT2;
    case Encoder::HTTP: return Decoder::HTTP;
    case Encoder::JSON: return Decoder::JSON;
    case Encoder::XML2: return Decoder::XML2;
    default: return Decoder::INVALID;
    }
}

Encoder::Type Notification::getComplimentType(Decoder::Type decoderType)
{
    switch (decoderType)
    {
    case Decoder::HEAT2:return Encoder::HEAT2;
    case Decoder::HTTP: return Encoder::HTTP;
    case Decoder::JSON: return Encoder::JSON;
    case Decoder::XML2: return Encoder::XML2;
    default: return Encoder::INVALID;
    }
}

} // namespace Blaze
