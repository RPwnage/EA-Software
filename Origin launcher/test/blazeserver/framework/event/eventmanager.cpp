/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/event/eventmanager.h"
#include "framework/grpc/outboundgrpcmanager.h"
#include "framework/protobuf/protobuftdfconversionhandler.h"
#include "framework/protocol/eventxmlencoder.h"
#include "framework/protocol/eventxmlprotocol.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/tdf/controllertypes_server.h"

#include "framework/oauth/accesstokenutil.h"

#include "eadp/messagebus/producer.grpc.pb.h"

namespace Blaze
{

namespace Event
{

const char8_t* EVENT_NOTIFICATION_PREFIX = "<events>\n";
const size_t LEN_EVENT_NOTIFICATION_PREFIX = strlen(EVENT_NOTIFICATION_PREFIX);
const TimeValue HEART_BEAT_MIN_PERIOD = 20 * 1000 * 1000; // 20 seconds in microseconds

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

EventSlave* EventSlave::createImpl()
{
    return BLAZE_NEW_MGID(EventSlave::COMPONENT_MEMORY_GROUP, "Component") EventManager;
}

EventManager::EventManager()
    : mRegistrantMap(BlazeStlAllocator("EventManager::mRegistrantMap")),
      mLogEncoder(*(BLAZE_NEW_MGID(EventSlave::COMPONENT_MEMORY_GROUP, "XmlEncoder") EventXmlEncoder())),
      mHeartBeatTimerId(INVALID_TIMER_ID)
{
    gEventManager = this;
}

EventManager::~EventManager()
{
    Registrants::iterator itr = mRegistrants.begin();
    Registrants::iterator end = mRegistrants.end();
    while (itr != end)
    {
        Registrant& registrant = *itr;
        ++itr;
        delete &registrant;
    }
    
    mRegistrantMap.clear();

    SAFE_DELETE_REF(mLogEncoder);

    if (mHeartBeatTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mHeartBeatTimerId);
    }
}

bool EventManager::onValidateConfig(EventConfig& config, const EventConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (config.getHeartBeatInterval() < HEART_BEAT_MIN_PERIOD)
    {
        eastl::string msg;
        msg.sprintf("[EventManager].onValidateConfig: heart beat event period can not be set below %" PRId64 " ms.",
            HEART_BEAT_MIN_PERIOD.getMillis());
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if (config.getEventCacheMaxSize() == 0)
    {
        eastl::string msg;
        msg.sprintf("[EventManager].onValidateConfig: EventCacheMaxSize cannot be zero.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if (config.getEventPublishPeriod() == 0 && config.getEventCacheSize() == 0)
    {
        eastl::string msg;
        msg.sprintf("[EventManager].onValidateConfig: EventPublishPeriod and EventCacheSize can not both be zero.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    // Event validation for the Notify service integration:
    ClientPlatformSet emptySet;
    for (const auto& compItr : config.getMessageTypesByComponentEvent())
    {
        ComponentId compId = BlazeRpcComponentDb::getComponentIdByName(compItr.first.c_str());
        if (compId == Component::INVALID_COMPONENT_ID)
        {
            eastl::string msg;
            msg.sprintf("[EventManager].onValidateConfig: MessageTypesByComponent map contains invalid component %s.", compItr.first.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            continue;
        }

        for (const auto& msgItr : compItr.second->getMessages())
        {
            EventId eventId = BlazeRpcComponentDb::getEventIdByName(compId, msgItr.first.c_str());
            if (eventId == 0)
            {
                eastl::string msg;
                msg.sprintf("[EventManager].onValidateConfig: MessageTypesByComponent map contains invalid event %s.", msgItr.first.c_str());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }

    // Event validation for the MessageBus service integration:
    auto err = validateSubscription(config.getMessageBusEventSubscription(), false, &validationErrors);
    if (err != Blaze::Event::SubscribeError::ERR_OK)
    {
        eastl::string msg;
        msg.sprintf("[EventManager].onValidateConfig: Issue with the MessageBusEventSubscription config.  Ensure that all components and events used in the config actually exist.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    return validationErrors.getErrorMessages().empty();
}

bool EventManager::onConfigure()
{
    BLAZE_TRACE_LOG(Log::SYSTEM, "[EventManager].onConfigure");

    if (mNotifyRegistrant.initialize(getConfig(), false) != SubscribeError::ERR_OK)
        return false;

    if (mMessageBusRegistrant.initialize(getConfig().getMessageBusEventSubscription()) != SubscribeError::ERR_OK)
        return false;

    return true;
}

bool EventManager::onPrepareForReconfigure(const EventConfig& newConfig)
{
    return mNotifyRegistrant.prepareForReconfigure(getConfig(), newConfig);
}

bool EventManager::onReconfigure()
{
    BLAZE_INFO_LOG(Log::SYSTEM, "[EventManager].onReconfigure");

    if (mNotifyRegistrant.initialize(getConfig(), true) != SubscribeError::ERR_OK)
        return false;

    if (mMessageBusRegistrant.initialize(getConfig().getMessageBusEventSubscription()) != SubscribeError::ERR_OK)
        return false;

    return true;
}

bool EventManager::onActivate()
{
    if (mHeartBeatTimerId == INVALID_TIMER_ID)
    {
        scheduleNextHeartBeat();
        return (INVALID_TIMER_ID != mHeartBeatTimerId);
    }
    
    mMessageBusRegistrant.refreshMessageBusRpc();

    return true;
}

void EventManager::MessageBusRegistrant::refreshMessageBusRpc()
{
    if (mMessageBusRpc == nullptr)
    {
        mMessageBusRpc = CREATE_GRPC_BIDIRECTIONAL_RPC("messagebus", eadp::messagebus::Producer, AsyncPublishRecords);
        if (mMessageBusRpc == nullptr && (mAllComponents || mComponents.size() || mEventWhitelistMap.size()))
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "EventManager.refreshMessageBusRpc:  Message bus was unable to create an outbound gRPC, despite being enabled.  Please ensure that your framework.cfg contains the messagebus outboundgrpc config.")
        }
    }
}

void EventManager::submitEvent(uint32_t eventId, const EA::TDF::Tdf& event, bool logBinaryEvent)
{
    if (logBinaryEvent)
        submitBinaryEvent(eventId, event);

    ComponentId componentId = (ComponentId)(eventId >> 16);
    CommandId commandId = (CommandId)(eventId & 0xffff);
    int32_t logCategory = BlazeRpcLog::getLogCategory(componentId);
    bool logEventEnabled = Logger::isLogEventEnabled(logCategory);

    bool pushHeartbeatEvent = false;

    // suppress heart beat event log generation since it does not
    // have content
    // push heartbeat event unconditionally
    if (eventId == EVENT_HEARTBEATEVENT)
    {
        pushHeartbeatEvent = true;
        logEventEnabled = false;
    }

    const EventInfo* eventInfo = ComponentData::getEventInfo(componentId, commandId);
    if (eventInfo == nullptr)
    {
        BLAZE_WARN_LOG(logCategory, "EventManager.submitEvent: Cannot find a defined event for componentId(" << componentId << "), eventId(" << commandId << ")");
        return;
    }

    // Send the event to our custom NotifyRegistrant
    Notification notification(*eventInfo, &event, NotificationSendOpts());
    mNotifyRegistrant.sendEvent(notification, false);

    // Send the event to the message bus service:
    mMessageBusRegistrant.sendEvent(notification, false);

    RawBufferPtr entry = BLAZE_NEW RawBuffer(1024);
    RpcProtocol::Frame frame;
    frame.componentId = componentId;
    frame.commandId = commandId;
    frame.messageType = RpcProtocol::NOTIFICATION;
    frame.userIndex = 0;

    mLogEncoder.encode(*entry, event, &frame);

    EventMetrics &metrics = mEventCountsByName[eventInfo->name];

    if (!logEventEnabled && mRegistrants.empty())
    {
        // Don't do anything if there is nowhere to push the event
        ++metrics.mCountDropped;
        metrics.mBytesDropped += entry->datasize();
        return;
    }

    ++metrics.mCountSent;
    metrics.mBytesSent += entry->datasize();

    if (logEventEnabled)
        Logger::logEvent(logCategory, (char8_t*)entry->data(), entry->datasize());

    Registrants::iterator itr = mRegistrants.begin();
    Registrants::iterator end = mRegistrants.end();
    while (itr != end)
    {
        Registrant& registrant = *itr;
        ++itr;

        // We can run into payload corruption issues with sharing a single intrustive_ptr for all registrants,
        // so create an AsyncNotificationPtr for each registrant in the list

        Notification notif(*eventInfo, &event, NotificationSendOpts());

        if (pushHeartbeatEvent)
            registrant.pushEvent(notif, true);
        else
            registrant.sendEvent(notif, false);
    }
}

void EventManager::MessageBusRegistrant::pushEvent(Notification& notification, bool allConns)
{
    // Attempt to send to the message bus:
    // use the RPC that already should exist: 
    const EventInfo* eventInfo = ComponentData::getEventInfo(notification.getComponentId(), notification.getNotificationId());
    if (eventInfo == nullptr)
        return;

    // Skip any attempts to send unregistered events: 
    const auto& protoHandler = Blaze::protobuf::ProtobufTdfConversionHandler::get();
    if (!protoHandler.isTdfIdRegistered(notification.getPayload()->getTdfId()))
    {
        BLAZE_WARN_LOG(BlazeRpcLog::getLogCategory(eventInfo->componentId), "EventManager.submitEvent: Cannot find an exported proto for tdf(" << notification.getPayload()->getTypeDescription().getFullName() << "). Skipping submission.");
        return;
    }

    // Say what the event was:
    eadp::messagebus::ProducerRecord* requestPtr( BLAZE_NEW eadp::messagebus::ProducerRecord );
    eadp::messagebus::ProducerRecord& request = *requestPtr;

    // GS_Blaze..clubs..UserJoinedClubEvent..fifa-pc-2018-dev
    (*request.mutable_topic_name()) = "GS_Blaze..";
    request.mutable_topic_name()->append(ComponentData::getComponentData(notification.getComponentId())->name);
    request.mutable_topic_name()->append("..");
    request.mutable_topic_name()->append(eventInfo->name);
    request.mutable_topic_name()->append("..");
    request.mutable_topic_name()->append(gController->getDefaultServiceName());
    request.mutable_topic_name()->append("-");
    request.mutable_topic_name()->append(gController->getServiceEnvironment());

    // Track the Timestamp of the event (message bus processing may change that)
    eadp::messagebus::ServiceMessage* serviceMessage = request.mutable_value();
    eadp::messagebus::MessageHeaders* messageHeaders = serviceMessage->mutable_headers();

    // Track the host, so it's clear which server sent the message:
    messageHeaders->mutable_source()->set_host_name(gController->getInstanceName());

    messageHeaders->set_version(gProcessController->getVersion().getVersion());     // Blaze Version
    messageHeaders->set_service(gController->getDefaultServiceName());              // Blaze Service

    eadp::messagebus::ServiceMessage::MessageBody* body = serviceMessage->add_body();
    TimeValue timeValue = TimeValue::getTimeOfDay();
    body->mutable_timestamp()->set_seconds((int32_t)timeValue.getSec());
    body->mutable_timestamp()->set_nanos((int32_t)(timeValue.getMicroSeconds() - (timeValue.getSec() * 1000000)) * 1000);

    // Type == EventName
    body->set_type(eventInfo->name);

    // Finally add the event itself:
    // NOTE: All Events must be registered!  Otherwise we can't convert them.
    Blaze::protobuf::ProtobufTdfConversionHandler::get().convertTdfToAny(notification.getPayload(), *body->mutable_content());

    // Send the request from a blockable source, since the send call might block waiting for the connection to init.
    Fiber::CreateParams fiberParams;
    fiberParams.blocking = true;
    fiberParams.namedContext = "MessageBusRegistrant::pushEventInternal";
    Fiber::FiberHandle fiberHandle;
    gFiberManager->scheduleCall(this, &EventManager::MessageBusRegistrant::pushEventInternal, requestPtr, fiberParams, &fiberHandle);
    mFiberIdSet.insert(fiberHandle.fiberId);
}

void EventManager::MessageBusRegistrant::pushEventInternal(eadp::messagebus::ProducerRecord* requestPtr)
{
    OAuth::AccessTokenUtil util;
    UserSession::SuperUserPermissionAutoPtr ptr(true);
    BlazeRpcError error = util.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NEXUS_S2S);
    if (error == ERR_OK)
    {
        refreshMessageBusRpc();
        if (mMessageBusRpc != nullptr)
        {
            // Attempt to send the message twice, to handle cases where the message bus was reset:
            int32_t numSendAttempts = 2;
            bool sendSuccess = false;
            while (numSendAttempts && !sendSuccess)
            {
                --numSendAttempts;

                // Hold a unique ptr to the current RPC, so that there's no chance that it could be changed by another fiber:
                Grpc::OutboundRpcBasePtr busRpc = mMessageBusRpc;
                busRpc->getClientContext().AddMetadata("authorization", util.getAccessToken());

                if (busRpc->sendRequest(requestPtr) != ERR_OK)
                {
                    if (busRpc == mMessageBusRpc)
                    {
                        mMessageBusRpc = nullptr;
                        refreshMessageBusRpc();
                    }
                }
                else
                {
                    sendSuccess = true;

                    // MessageBus is expected to send a response, so we have to recv it to avoid filling up the grpc queue. 
                    Blaze::Grpc::ResponsePtr response; 
                    error = busRpc->waitForResponse(response);
                    if (!response && error != ERR_TIMEOUT)
                    {
                        if (busRpc == mMessageBusRpc)
                        {
                            mMessageBusRpc = nullptr;
                            refreshMessageBusRpc();
                        }
                    }
                }
            }
        }
    }

    delete requestPtr;
    mFiberIdSet.erase(Fiber::getCurrentFiberId());
}

EventManager::MessageBusRegistrant::~MessageBusRegistrant()
{
    // Send a nullptr event to cleanly close the connection:
    if (mMessageBusRpc != nullptr)
    {
        mMessageBusRpc->sendRequest(nullptr);
    }

    // Cancel the Fiber if it exists:
    for (auto& curFiberId : mFiberIdSet)
    {
        Fiber::cancel(curFiberId, ERR_CANCELED);
    }
}

void EventManager::submitBinaryEvent(uint32_t eventId, const EA::TDF::Tdf& event) const
{
    ComponentId componentId = (ComponentId)(eventId >> 16);
    int32_t logCategory = BlazeRpcLog::getLogCategory(componentId);
    if (!Logger::isLogBinaryEventEnabled(logCategory))
        return;

    EventEnvelope eventEnvelope;

    eventEnvelope.setTimestamp(TimeValue::getTimeOfDay());

    eventEnvelope.setServiceName(gController->getDefaultServiceName());
    eventEnvelope.setInstanceName(gController->getInstanceName());
    eventEnvelope.setComponentId(componentId);

    if (gCurrentUserSession != nullptr)
    {
        EA::TDF::ObjectId objId(ENTITY_TYPE_USER, gCurrentUserSession->getBlazeId());
        eventEnvelope.setBlazeObjId(objId);
        eventEnvelope.setSessionId(gCurrentUserSession->getSessionId());;
    }

    eventEnvelope.setEventType((uint16_t)(eventId & 0xffff));
    eventEnvelope.setEventData(const_cast<EA::TDF::Tdf&>(event));

    Logger::logTdf(logCategory, eventEnvelope);
}

void EventManager::submitSessionNotification(Notification& notification, const UserSession& session)
{
    for (Registrants::iterator it = mRegistrants.begin(), end = mRegistrants.end(); it != end; ++it)
    {
        Registrant& registrant = *it;
        registrant.sendSessionNotification(notification, session);
    }
}

SubscribeError::Error EventManager::validateSubscription(const SubscriptionInfo& info, bool validateClientName/* = true*/, ConfigureValidationErrors* validationErrors /*= nullptr*/)
{
    if (validateClientName)
    {
        const char8_t* clientName = info.getClientName();
        if ((clientName == nullptr) || (clientName[0] == '\0'))
        {
            if (validationErrors == nullptr)
                return SubscribeError::EVENT_ERROR_MISSING_CLIENT_NAME;
            else
            {
                eastl::string msg;
                msg.sprintf("[EventManager].onValidateConfig: Missing Client name for event subscription.");
                EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }

    const SubscriptionInfo::ComponentNameList& components = info.getComponentList();
    if (components.size() > 0)
    {
        for (auto itr : components)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(itr);
            if (id == RPC_COMPONENT_UNKNOWN)
            {
                if (validationErrors == nullptr)
                    return SubscribeError::EVENT_ERROR_UNKNOWN_COMPONENT;
                else
                {
                    eastl::string msg;
                    msg.sprintf("[EventManager].onValidateConfig: Component named %s in ComponentList not found.", itr.c_str());
                    EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
    }

    if (info.getEventBlacklist().size() > 0)
    {
        for (auto mapIt : info.getEventBlacklist())
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt.first.c_str());
            if (id == RPC_COMPONENT_UNKNOWN)
            {
                if (validationErrors == nullptr)
                    return SubscribeError::EVENT_ERROR_UNKNOWN_COMPONENT;
                else
                {
                    eastl::string msg;
                    msg.sprintf("[EventManager].onValidateConfig: Component named %s in EventBlacklist not found.", mapIt.first.c_str());
                    EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                    str.set(msg.c_str());
                    continue;
                }
            }
            for (auto listIt : *mapIt.second)
            {
                bool isFound = false;
                BlazeRpcComponentDb::getEventIdByName(id, listIt.c_str(), &isFound);
                if (!isFound)
                {
                    if (validationErrors == nullptr)
                        return SubscribeError::EVENT_ERROR_UNKNOWN_EVENT_NAME;
                    else
                    {
                        eastl::string msg;
                        msg.sprintf("[EventManager].onValidateConfig: Event named %s in Component named %s in EventBlacklist not found.", listIt.c_str(), mapIt.first.c_str());
                        EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                        str.set(msg.c_str());
                    }
                }
            }
        }
    }
    if (info.getEventWhitelist().size() > 0)
    {
        for (auto mapIt : info.getEventWhitelist())
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt.first.c_str());
            if (id == RPC_COMPONENT_UNKNOWN)
            {
                if (validationErrors == nullptr)
                    return SubscribeError::EVENT_ERROR_UNKNOWN_COMPONENT;
                else
                {
                    eastl::string msg;
                    msg.sprintf("[EventManager].onValidateConfig: Component named %s in EventWhitelist not found.", mapIt.first.c_str());
                    EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                    str.set(msg.c_str());
                    continue;
                }
            }
            for (auto listIt : *mapIt.second)
            {
                bool isFound = false;
                BlazeRpcComponentDb::getEventIdByName(id, listIt.c_str(), &isFound);
                if (!isFound)
                {
                    if (validationErrors == nullptr)
                        return SubscribeError::EVENT_ERROR_UNKNOWN_EVENT_NAME;
                    else
                    {
                        eastl::string msg;
                        msg.sprintf("[EventManager].onValidateConfig: Event named %s in Component named %s in EventWhitelist not found.", listIt.c_str(), mapIt.first.c_str());
                        EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                        str.set(msg.c_str());
                    }
                }
            }
        }
    }

    const SubscriptionInfo::ComponentNameList& notificationComponents = info.getCompSessNotifyList();
    if (notificationComponents.size() > 0)
    {
        for (auto itr : notificationComponents)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(itr);
            if (id == RPC_COMPONENT_UNKNOWN)
            {
                if (validationErrors == nullptr)
                    return SubscribeError::EVENT_ERROR_UNKNOWN_COMPONENT;
                else
                {
                    eastl::string msg;
                    msg.sprintf("[EventManager].onValidateConfig: Component named %s in CompSessNotifyList not found.", itr.c_str());
                    EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
    }

    if (info.getSessNotificationBlacklist().size() > 0)
    {
        for (auto mapIt : info.getSessNotificationBlacklist())
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt.first.c_str());
            if (id == RPC_COMPONENT_UNKNOWN)
            {
                if (validationErrors == nullptr)
                    return SubscribeError::EVENT_ERROR_UNKNOWN_COMPONENT;
                else
                {
                    eastl::string msg;
                    msg.sprintf("[EventManager].onValidateConfig: Component named %s in SessNotificationBlacklist not found.", mapIt.first.c_str());
                    EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                    str.set(msg.c_str());
                    continue;
                }
            }
            for (auto listIt : *mapIt.second)
            {
                bool isFound = false;
                BlazeRpcComponentDb::getNotificationIdByName(id, listIt.c_str(), &isFound);
                if (!isFound)
                {
                    if (validationErrors == nullptr)
                        return SubscribeError::EVENT_ERROR_UNKNOWN_NOTIFICATION_NAME;
                    else
                    {
                        eastl::string msg;
                        msg.sprintf("[EventManager].onValidateConfig: Notification named %s in Component named %s in SessNotificationBlacklist not found.", listIt.c_str(), mapIt.first.c_str());
                        EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                        str.set(msg.c_str());
                    }
                }
            }
        }
    }

    if (info.getSessNotificationWhitelist().size() > 0)
    {
        for (auto mapIt : info.getSessNotificationWhitelist())
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt.first.c_str());
            if (id == RPC_COMPONENT_UNKNOWN)
            {
                if (validationErrors == nullptr)
                    return SubscribeError::EVENT_ERROR_UNKNOWN_COMPONENT;
                else
                {
                    eastl::string msg;
                    msg.sprintf("[EventManager].onValidateConfig: Component named %s in SessNotificationWhitelist not found.", mapIt.first.c_str());
                    EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                    str.set(msg.c_str());
                    continue;
                }
            }
            for (auto listIt : *mapIt.second)
            {
                bool isFound = false;
                BlazeRpcComponentDb::getNotificationIdByName(id, listIt.c_str(), &isFound);
                if (!isFound)
                {
                    if (validationErrors == nullptr)
                        return SubscribeError::EVENT_ERROR_UNKNOWN_NOTIFICATION_NAME;
                    else
                    {
                        eastl::string msg;
                        msg.sprintf("[EventManager].onValidateConfig: Notification named %s in Component named %s in SessNotificationWhitelist not found.", listIt.c_str(), mapIt.first.c_str());
                        EA::TDF::TdfString& str = validationErrors->getErrorMessages().push_back();
                        str.set(msg.c_str());
                    }
                }
            }
        }
    }

    return SubscribeError::ERR_OK;
}

/*** Private Methods *****************************************************************************/
SubscribeError::Error EventManager::RegistrantBase::initialize(const SubscriptionInfo& info)
{
    SubscribeError::Error error = validateSubscription(info, false);
    if (error != SubscribeError::ERR_OK)
        return error;

    mAllComponents = info.getAllComponents();

    const char8_t* clientName = info.getClientName();
    blaze_strnzcpy(mClientName, clientName, sizeof(mClientName));

    mComponents.clear();
    const SubscriptionInfo::ComponentNameList& components = info.getComponentList();
    size_t size = components.size();
    if (size > 0)
    {
        mComponents.reserve(size);
        SubscriptionInfo::ComponentNameList::const_iterator itr = components.begin();
        SubscriptionInfo::ComponentNameList::const_iterator end = components.end();
        for (; itr != end; ++itr)
        {
            ComponentId slaveId = Component::INVALID_COMPONENT_ID, masterId = Component::INVALID_COMPONENT_ID;
            BlazeRpcComponentDb::getComponentIdsByBaseName(*itr, slaveId, masterId);
            if (slaveId != Component::INVALID_COMPONENT_ID)
                mComponents.push_back(slaveId);
            if (masterId != Component::INVALID_COMPONENT_ID)
                mComponents.push_back(masterId);
        }
    }

    mEventBlacklistMap.clear();
    if (info.getEventBlacklist().size() > 0)
    {
        for (NotificationNameListMap::const_iterator mapIt = info.getEventBlacklist().begin(); mapIt != info.getEventBlacklist().end(); ++mapIt)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt->first.c_str());
            for (NotificationNameList::const_iterator listIt = mapIt->second->begin(); listIt != mapIt->second->end(); ++listIt)
            {
                uint16_t eventId = BlazeRpcComponentDb::getEventIdByName(id, listIt->c_str());
                mEventBlacklistMap.insert(combineNotificationKey(id, eventId));
            }
        }
    }

    mEventWhitelistMap.clear();
    if (info.getEventWhitelist().size() > 0)
    {
        for (NotificationNameListMap::const_iterator mapIt = info.getEventWhitelist().begin(); mapIt != info.getEventWhitelist().end(); ++mapIt)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt->first.c_str());
            for (NotificationNameList::const_iterator listIt = mapIt->second->begin(); listIt != mapIt->second->end(); ++listIt)
            {
                uint16_t eventId = BlazeRpcComponentDb::getEventIdByName(id, listIt->c_str());
                mEventWhitelistMap.insert(combineNotificationKey(id, eventId));
            }
        }
    }

    return error;
}

EventManager::RegistrantBase::ComponentEventId EventManager::RegistrantBase::combineNotificationKey(ComponentId id, EventId eventId)
{
    return static_cast<ComponentEventId>((id << 16) | eventId);
}

void EventManager::RegistrantBase::sendEvent(Notification& notification, bool allConns)
{
    bool found = mAllComponents;
    if (!found)
    {
        for (ComponentIdList::const_iterator it = mComponents.begin(), end = mComponents.end(); (it != end) && !found; ++it)
        {
            found = (*it == notification.getComponentId());
        }
    }

    EventManager::RegistrantBase::ComponentEventId compEventId = combineNotificationKey(notification.getComponentId(), notification.getNotificationId());
    if (found &&
        (mEventBlacklistMap.find(compEventId) == mEventBlacklistMap.end()) &&
        (mEventWhitelistMap.size() == 0 || mEventWhitelistMap.find(compEventId) != mEventWhitelistMap.end()))
    {
        pushEvent(notification, allConns);
    }
}

// Registrant implementation
EventManager::Registrant::Registrant(InboundRpcConnection& connection)
    : RegistrantBase(),
      mAllSessionNotifications(false),
      mResponseFormat(RpcProtocol::FORMAT_USE_TDF_NAMES),
      mCurrent(0),
      mSubEncoder(nullptr)
{
    mAllComponents = false;
    *mClientName = '\0';
    mpPrev = mpNext = nullptr;
    addConnection(connection);
}

EventManager::Registrant::~Registrant()
{
    mConnectionList.clear();
    if (mSubEncoder != nullptr)
        delete mSubEncoder;
}

SubscribeError::Error EventManager::Registrant::initialize(const SubscriptionInfo& info, Encoder::Type encoderType, Blaze::RpcProtocol::ResponseFormat respFormat)
{
    SubscribeError::Error error = RegistrantBase::initialize(info);
    if (error != SubscribeError::ERR_OK)
        return error;

    mResponseFormat = respFormat;
    mAllSessionNotifications = info.getAllSessNotifications();
    mConnectionGroupId = info.getConnectionGroupId();
    mSubEncoder = info.getEncodeDefaultValues() ? nullptr : static_cast<TdfEncoder*>(EncoderFactory::createDefaultDifferenceEncoder(encoderType));

    mNotificationComponents.clear();
    const SubscriptionInfo::ComponentNameList& notificationComponents = info.getCompSessNotifyList();
    size_t notificationListSize = notificationComponents.size();
    if (notificationListSize > 0)
    {
        mNotificationComponents.reserve(notificationListSize);
        SubscriptionInfo::ComponentNameList::const_iterator itr = notificationComponents.begin();
        SubscriptionInfo::ComponentNameList::const_iterator end = notificationComponents.end();
        for (; itr != end; ++itr)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(*itr);
            mNotificationComponents.push_back(id);
        }
    }
    
    mSessNotificationBlacklistMap.clear();
    if (info.getSessNotificationBlacklist().size() > 0)
    {
        for (NotificationNameListMap::const_iterator mapIt = info.getSessNotificationBlacklist().begin(); mapIt != info.getSessNotificationBlacklist().end(); ++mapIt)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt->first.c_str());
            for (NotificationNameList::const_iterator listIt = mapIt->second->begin(); listIt != mapIt->second->end(); ++listIt)
            {
                uint16_t notificationId = BlazeRpcComponentDb::getNotificationIdByName(id, listIt->c_str());
                mSessNotificationBlacklistMap.insert(combineNotificationKey(id, notificationId));
            }
        }
    }

    mSessNotificationWhitelistMap.clear();
    if (info.getSessNotificationWhitelist().size() > 0)
    {
        for (NotificationNameListMap::const_iterator mapIt = info.getSessNotificationWhitelist().begin(); mapIt != info.getSessNotificationWhitelist().end(); ++mapIt)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(mapIt->first.c_str());
            for (NotificationNameList::const_iterator listIt = mapIt->second->begin(); listIt != mapIt->second->end(); ++listIt)
            {
                uint16_t notificationId = BlazeRpcComponentDb::getNotificationIdByName(id, listIt->c_str());
                mSessNotificationWhitelistMap.insert(combineNotificationKey(id, notificationId));
            }
        }
    }

    return SubscribeError::ERR_OK;
}

void EventManager::Registrant::pushEvent(Notification& notification, bool allConns)
{
    // Early out if nothing to send to.
    if (mConnectionList.empty())
        return;

    // Use registrant's response formatting for event.
    notification.setResponseFormat(mResponseFormat);
    notification.setEncoder(mSubEncoder);

    for (uint32_t sendCount = (allConns ? mConnectionList.size() : 1); sendCount > 0; --sendCount)
    {
        Registrant::SingleConnection& singleConn = mConnectionList.at(mCurrent++ % mConnectionList.size());
        if (singleConn.mFirstEvent)
        {
            // Ensure that we include a root document tag if this is the first event we're sending
            // for this connection
            notification.setPrefix(EVENT_NOTIFICATION_PREFIX, LEN_EVENT_NOTIFICATION_PREFIX);

            singleConn.mFirstEvent = false;
        }
        else
        {
            notification.resetPrefix();
        }

        singleConn.mConnection->sendNotification(notification);
    }
    notification.resetPrefix();
}

void EventManager::Registrant::sendSessionNotification(Notification& notification, const UserSession& session)
{
    // Early out if nothing to send to.
    if (mConnectionList.empty())
        return;

    bool found = mAllSessionNotifications;
    if (!found)
    {
        for (ComponentIdList::iterator it = mNotificationComponents.begin(), end = mNotificationComponents.end(); (it != end) && !found; ++it)
        {
            found = (*it == notification.getComponentId());
        }
    }

    EventManager::RegistrantBase::ComponentEventId compEventId = combineNotificationKey(notification.getComponentId(), notification.getNotificationId());
    if (found &&
        (mSessNotificationBlacklistMap.find(compEventId) == mSessNotificationBlacklistMap.end()) &&
        (mSessNotificationWhitelistMap.size() == 0 || mSessNotificationWhitelistMap.find(compEventId) != mSessNotificationWhitelistMap.end()))

    {
        Registrant::SingleConnection& conn = mConnectionList.at(mCurrent++ % mConnectionList.size());

        if (conn.mFirstEvent)
        {        
            // Ensure that we include a root document tag if this is the first event we're sending for this connection.
            notification.setPrefix(EVENT_NOTIFICATION_PREFIX, LEN_EVENT_NOTIFICATION_PREFIX);

            conn.mFirstEvent = false;
        }
        else
        {
            notification.resetPrefix();
        }

        // Ensure that the encoder has the correct user session so it can include the session id and
        // blaze id attributes to the event header.
        EventXmlEncoder& encoder = static_cast<EventXmlEncoder&>(conn.mConnection->getEncoder());
        encoder.setAssociatedUserSession(&session);

        // Use registrant's response formatting for event.
        notification.setResponseFormat(mResponseFormat);
        notification.setEncoder(mSubEncoder);

        conn.mConnection->sendNotification(notification);

        encoder.setAssociatedUserSession(nullptr);
    }
    notification.resetPrefix();
}

void EventManager::Registrant::addConnection(InboundRpcConnection& conn)
{
    mConnectionList.push_back();
    mConnectionList.back().mConnection = &conn;
    mConnectionList.back().mFirstEvent = true;
    return;
}

bool EventManager::Registrant::hasConnection(InboundRpcConnection& conn)
{
    for (ConnectionList::iterator itr = mConnectionList.begin(), end = mConnectionList.end(); itr != end; ++itr)
    {
        InboundRpcConnection* tempConn = ((*itr).mConnection);
        if (tempConn == &conn)
        {
            return true;
        }
    }

    return false;
}

bool EventManager::Registrant::checkConnectionAndErase(Connection& conn)
{
    for (ConnectionList::iterator itr = mConnectionList.begin(), end = mConnectionList.end(); itr != end; ++itr)
    {
        SingleConnection& temp = *itr;
        if (temp.mConnection == &conn)
        {
            mConnectionList.erase(&temp);
            return true;
        }
    }

    return false;
}

// NotifyRegistrant implementation
EventManager::NotifyRegistrant::NotifyRegistrant()
  : RegistrantBase(),
    mEventsByMessageTypeCache(BlazeStlAllocator("NotifyRegistrant::EventsByMessageTypeCache", EventSlave::COMPONENT_MEMORY_GROUP)),
    mMessageTypesByComponentEvent(BlazeStlAllocator("NotifyRegistrant::MessageTypesByComponentEvent", EventSlave::COMPONENT_MEMORY_GROUP)),
    mNotifyComp(nullptr),
    mPublishPeriod(0),
    mPublishTimer(INVALID_TIMER_ID),
    mPublishFiberId(Fiber::INVALID_FIBER_ID),
    mPublishFiberGroupId(Fiber::allocateFiberGroupId())
{
}

EventManager::NotifyRegistrant::~NotifyRegistrant()
{
    if (Fiber::isFiberIdInUse(mPublishFiberId))
    {
        Fiber::cancelGroup(mPublishFiberGroupId, ERR_DISCONNECTED);
    }

    if (mPublishTimer != INVALID_TIMER_ID)
        gSelector->cancelTimer(mPublishTimer);

    for (EventsByMessageTypeCache::iterator iter = mEventsByMessageTypeCache.begin(), end = mEventsByMessageTypeCache.end(); iter != end; ++iter)
    {
        EventDeque &deque = iter->second;
        deque.clear();
    }
}

SubscribeError::Error EventManager::NotifyRegistrant::initialize(const EventConfig& config, bool isReconfigure)
{
    // We don't need to refetch the notify proxy component on a reconfigure
    if (!isReconfigure)
    {
        mNotifyComp = (Notify::NotifySlave*)gOutboundHttpService->getService(Notify::NotifySlave::COMPONENT_INFO.name);
        if (mNotifyComp == nullptr)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[NotifyRegistrant].: Failed to fetch Notify proxy component.");
            return SubscribeError::ERR_SYSTEM;
        }
    }

    SubscriptionInfo info;
    buildMessageTypeMappings(config.getMessageTypesByComponentEvent(), mMessageTypesByComponentEvent, mPlatformsByComponent, &info);

    // Even if we have no components, ensure that our internal state matches what's configured
    // We'll early out down below if there are no components configured.
    SubscribeError::Error error = RegistrantBase::initialize(info);
    if (error != SubscribeError::ERR_OK)
        return error;

    if (info.getComponentList().empty())
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[NotifyRegistrant].: No components configured. Publishing to Notify is disabled.");
        if (mPublishTimer != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mPublishTimer);
            mPublishTimer = INVALID_TIMER_ID;
        }
        return SubscribeError::ERR_OK;
    }

    // When reconfiguring, prepareForReconfigure may reschedule, but if it doesn't
    // we need to do it here.
    if (mPublishTimer == INVALID_TIMER_ID)
    {
        mPublishPeriod = config.getEventPublishPeriod();
        if (mPublishPeriod > 0)
        {
            mPublishTimer = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + mPublishPeriod,
                this, &EventManager::NotifyRegistrant::publishEventsToNotify, "NotifyRegistrant::initialize");
            if (mPublishTimer == INVALID_TIMER_ID)
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[NotifyRegistrant].: Failed to schedule periodic event publishing.");
                return SubscribeError::ERR_SYSTEM;
            }
        }
    }

    return SubscribeError::ERR_OK;
}

bool EventManager::NotifyRegistrant::buildMessageTypeMappings(const EventConfig::ComponentNotifyEventMap& eventMap, MessageTypesByComponentEvent& messageTypesByComponentEvent, PlatformsByComponent& platformsByComponent, SubscriptionInfo* info)
{
    messageTypesByComponentEvent.clear();

    for (const auto& compItr : eventMap)
    {
        ComponentId compId = BlazeRpcComponentDb::getComponentIdByName(compItr.first.c_str());
        if (compId == Component::INVALID_COMPONENT_ID)
            return false;

        if (!compItr.second->getPlatforms().empty())
        {
            bool skipComponent = true;
            for (const auto& plat : compItr.second->getPlatforms())
            {
                if (gController->isPlatformUsed(plat))
                {
                    skipComponent = false;
                    break;
                }
            }
            if (skipComponent)
                continue;

            platformsByComponent[compId].insert(compItr.second->getPlatforms().begin(), compItr.second->getPlatforms().end());
        }

        if (info != nullptr)
            info->getComponentList().push_back(compItr.first.c_str());

        for (const auto& msgItr : compItr.second->getMessages())
        {
            EventId eventId = BlazeRpcComponentDb::getEventIdByName(compId, msgItr.first.c_str());
            if (eventId == 0)
                return false;

            ComponentEventId id = combineNotificationKey(compId, eventId);
            messageTypesByComponentEvent[id] = msgItr.second.c_str();
        }
    }
    return true;
}

void EventManager::NotifyRegistrant::pushEvent(Notification& notification, bool allConns)
{
    if (mNotifyComp == nullptr)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[NotifyRegistrant].pushEvent: Notify proxy component is nullptr!");
        return;
    }

    EA::TDF::TdfPtr eventDataTdf;
    BlazeRpcError err = notification.extractPayloadTdf(eventDataTdf);
    if ((err != ERR_OK) || (eventDataTdf == nullptr))
    {
        BLAZE_ASSERT_LOG(Log::SYSTEM, "NotifyRegistrant.pushEvent: notification.extractPayloadTdf() failed with err(" << LOG_ENAME(err) << "), or the payload could not be extracted from the notification.");
        return;
    }

    PlatformsByComponent::const_iterator platItr = mPlatformsByComponent.find(notification.getComponentId());
    if (platItr != mPlatformsByComponent.end() && !platItr->second.empty())
    {
        Component* component = gController->getComponent(notification.getComponentId(), false, false);
        if (component == nullptr)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "NotifyRegistrant.pushEvent: Unable to fetch component for componentId(" << notification.getComponentId() <<
                "): configured platforms list will be ignored for event with eventId(" << notification.getNotificationId() << ")");
        }
        else if (!component->asStub()->shouldSendNotifyEvent(platItr->second, notification))
        {
            return;
        }
    }

    ComponentEventId compEventId = combineNotificationKey(notification.getComponentId(), notification.getNotificationId());
    MessageTypesByComponentEvent::const_iterator iter = mMessageTypesByComponentEvent.find(compEventId);
    if (iter != mMessageTypesByComponentEvent.end())
    {
        EventEnvelopePtr eventTdf = BLAZE_NEW_MGID(EventSlave::COMPONENT_MEMORY_GROUP, "EventEnvelope") EventEnvelope;
        eventTdf->setEventData(*eventDataTdf->clone());
        eventTdf->setEventType(notification.getNotificationId() & 0xffff);
        eventTdf->setTimestamp(TimeValue::getTimeOfDay().getSec());

        eventTdf->setInstanceName(gController->getInstanceName());
        eventTdf->setComponentId(notification.getComponentId());

        if (gCurrentUserSession != nullptr)
        {
            EA::TDF::ObjectId objId(ENTITY_TYPE_USER, gCurrentUserSession->getBlazeId());
            eventTdf->setBlazeObjId(objId);
            eventTdf->setSessionId(gCurrentUserSession->getSessionId());;
        }
        queueEventForPublish(compEventId, eventTdf);
    }
}

void EventManager::NotifyRegistrant::queueEventForPublish(ComponentEventId compEventId, EventEnvelopePtr event)
{
    MessageTypesByComponentEvent::const_iterator iter = mMessageTypesByComponentEvent.find(compEventId);
    if (iter != mMessageTypesByComponentEvent.end())
    {
        const MessageType& messageType = iter->second;

        // If we're shutting down, we may no longer be able to access
        // the authentication or notify components, so just return here
        if (gController->isShuttingDown())
        {
            return;
        }

        // Cache the event
        EventDeque& deque = mEventsByMessageTypeCache[messageType];
        uint32_t maxCacheSize = gEventManager->getConfig().getEventCacheMaxSize();
        if (deque.size() == maxCacheSize)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[NotifyRegistrant].pushEvent: Event cache for messageType " << messageType << " reached max cache size " << 
                maxCacheSize << "; the oldest cached event for this messageType will be dropped.");
            deque.pop_front();
        }
        deque.push_back(event);

        // If cache has reached the configured limit; publish events for messageType
        uint32_t cacheSize = gEventManager->getConfig().getEventCacheSize();
        if (cacheSize > 0 && deque.size() >= cacheSize)
        {
			publishEventsToNotify(messageType);
        }
    }
}

void EventManager::NotifyRegistrant::publishEventsToNotify()
{
    if (mPublishTimer != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mPublishTimer);
        mPublishTimer = INVALID_TIMER_ID;
    }
    else
    {
        // If we're here, that means that we've blocked inside publishEventsToNotify
        // so just return
        return;
    }

    EventsByMessageTypeCache::const_iterator iter = mEventsByMessageTypeCache.begin();
    EventsByMessageTypeCache::const_iterator end = mEventsByMessageTypeCache.end();
    for (; iter != end; ++iter)
    {
		publishEventsToNotify(iter->first);
    }

    if (mPublishTimer != INVALID_TIMER_ID)
    {
        BLAZE_ASSERT_LOG(Log::SYSTEM, "[NotifyRegistrant].publishEventsToNotify: A publish timer has already been scheduled!");
        return;
    }

    if (mPublishPeriod.getMicroSeconds() > 0)
    {
        Fiber::CreateParams params;
        params.relativeTimeout = gEventManager->getConfig().getEventPublishTimeout();

        mPublishTimer = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + mPublishPeriod,
            this, &EventManager::NotifyRegistrant::publishEventsToNotify, "NotifyRegistrant::publishEventsToNotify", params);
    }
}

void EventManager::NotifyRegistrant::publishEventsToNotify(const MessageType& messageType)
{
	// If we're shutting down, we may no longer be able to access
	// the authentication or notify components, so just return here
	if (gController->isShuttingDown())
		return;

	EventsByMessageTypeCache::iterator iterOut = mEventsByMessageTypeCache.find(messageType);
	if (iterOut != mEventsByMessageTypeCache.end())
	{
		EventDeque& deque = iterOut->second;
		if (deque.empty())
			return; // Nothing to publish

		Notify::PostNotificationsRequestPtr req = BLAZE_NEW_MGID(EventSlave::COMPONENT_MEMORY_GROUP, "PostNotificationsRequest") Notify::PostNotificationsRequest;
		req->setMessageType(messageType.c_str());

		uint32_t cacheSize = gEventManager->getConfig().getEventCacheSize();
		EventDeque::iterator iter = deque.begin();
		EventDeque::iterator last;
		if (cacheSize > 0 && deque.size() > cacheSize)
		{
			last = iter;
			eastl::advance(last, cacheSize); // Limit the number of events we publish.
			// Whatever is left will get picked up in the next batch.
		}
		else
			last = deque.end();

		Notify::PostNotificationsRequest::EventEnvelopeList& eventList = req->getNotifications();
		eventList.reserve(cacheSize);

		for (; iter != last; ++iter)
		{
			eventList.push_back((*iter).get());
		}
		deque.erase(deque.begin(), last);

		Fiber::CreateParams params;
		params.blocking = true;
		params.blocksCaller = true;
		params.namedContext = "NotifyRegistrant::pushEvent";
		params.relativeTimeout = gEventManager->getConfig().getEventPublishTimeout();
		params.groupId = mPublishFiberGroupId;
		gFiberManager->scheduleCall(this, &EventManager::NotifyRegistrant::doPublishEventsToNotify, req, params);
	}
}

bool EventManager::NotifyRegistrant::prepareForReconfigure(const EventConfig& config, const EventConfig& newConfig)
{
    MessageTypesByComponentEvent newMessageTypes;
    PlatformsByComponent newPlatformsByComponent;
    if (!buildMessageTypeMappings(newConfig.getMessageTypesByComponentEvent(), newMessageTypes, newPlatformsByComponent, nullptr))
        return false;

    bool componentsUpdated = (newMessageTypes.size() != mMessageTypesByComponentEvent.size() || newPlatformsByComponent.size() != mPlatformsByComponent.size());
    if (!componentsUpdated)
    {
        for (const auto& newCompItr : newMessageTypes)
        {
            if (mMessageTypesByComponentEvent.find(newCompItr.first) == mMessageTypesByComponentEvent.end())
            {
                componentsUpdated = true;
                break;
            }
        }
    }
    PlatformsByComponent::const_iterator newCompItr = newPlatformsByComponent.begin();
    for (; !componentsUpdated && newCompItr != newPlatformsByComponent.end(); ++newCompItr)
    {
        PlatformsByComponent::const_iterator compItr = mPlatformsByComponent.find(newCompItr->first);
        if (compItr == mPlatformsByComponent.end() || compItr->second.size() != newCompItr->second.size())
        {
            componentsUpdated = true;
            break;
        }
        for (const auto& platItr : compItr->second)
        {
            if (newCompItr->second.find(platItr) == newCompItr->second.end())
            {
                componentsUpdated = true;
                break;
            }
        }
    }

    TimeValue oldPublishPeriod = mPublishPeriod;

    // Need to set this here as the publish may cause another fiber to schedule a timer
    // with the old publish period before we've finished reconfiguring
    mPublishPeriod = newMessageTypes.empty() ? 0 : newConfig.getEventPublishPeriod();

    if ((newConfig.getEventCacheSize() < config.getEventCacheSize()) || componentsUpdated || (mPublishPeriod != oldPublishPeriod))
    {
        if (mPublishTimer != INVALID_TIMER_ID)
        {
            // There's a publish scheduled already, so cancel it and publish now, then reschedule
            publishEventsToNotify();
        }
    }

    return true;
}

void EventManager::NotifyRegistrant::doPublishEventsToNotify(Notify::PostNotificationsRequestPtr req)
{
    mPublishFiberId = Fiber::getCurrentFiberId();

    // Fetch the server access token
    OAuth::AccessTokenUtil util;
    UserSession::SuperUserPermissionAutoPtr ptr(true);
    BlazeRpcError error = util.retrieveServerAccessToken(OAuth::TOKEN_TYPE_NEXUS_S2S);
    if (error == ERR_OK)
    {
        req->setAccessToken(util.getAccessToken());
        req->setApiVersion("2");

        Notify::NotifyError errTdf;
        error = mNotifyComp->postNotifications(*req, errTdf);
        if (error != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[NotifyRegistrant].publishEventsToNotify: Failed to publish events to the Notify service with error (" << errTdf.getError().getName() << "); message: " << errTdf.getMessage());
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[NotifyRegistrant].publishEventsToNotify: Failed to retrieve server access token with error " << ErrorHelp::getErrorName(error));
    }

    mPublishFiberId = Fiber::INVALID_FIBER_ID;
}

SubscribeError::Error EventManager::addConnection(const SubscriptionInfo &request, InboundRpcConnection& conn, Blaze::RpcProtocol::ResponseFormat respFormat)
{
    Registrant* registrant = BLAZE_NEW_MGID(EventSlave::COMPONENT_MEMORY_GROUP, "Registrant") Registrant(conn);
    SubscribeError::Error rc = registrant->initialize(request, conn.getEncoder().getType(), respFormat);
    if (rc != SubscribeError::ERR_OK)
    {
        delete registrant;
    }
    else
    {
        const char8_t* groupName = request.getConnectionGroupId();

        if (strcmp(groupName, "") != 0)
        {
            eastl::string str(groupName);
            mRegistrantMap.insert(eastl::make_pair(str , registrant));
        }

        mRegistrants.push_back(*registrant);
    }

    conn.addListener(*this);
    return rc;
}

void EventManager::getStatus(EventManagerStatus& status)
{
    status.setNumRegistrants(mRegistrants.size());

    for (EventCountsByName::const_iterator it = mEventCountsByName.begin(), end = mEventCountsByName.end()
        ; it != end
        ; ++it)
    {
        eastl::string buf;
        buf.append_sprintf("TotalCountSent_%s", it->first);
        status.getEventMetrics()[buf.c_str()] = it->second.mCountSent;
        buf.clear();

        buf.append_sprintf("TotalBytesSent_%s", it->first);
        status.getEventMetrics()[buf.c_str()] = it->second.mBytesSent;
        buf.clear();
    
        buf.append_sprintf("TotalCountDropped_%s", it->first);
        status.getEventMetrics()[buf.c_str()] = it->second.mCountDropped;
        buf.clear();

        buf.append_sprintf("TotalBytesDropped_%s", it->first);
        status.getEventMetrics()[buf.c_str()] = it->second.mBytesDropped;
        buf.clear();
    }
}

SubscribeError::Error EventManager::processSubscribe(
        const SubscriptionInfo &request, const Message* message)
{
    SubscribeError::Error rc;
    
    // Ensure that the RPC is being called from an EventXML endpoint
    rc = SubscribeError::EVENT_ERROR_NOT_EVENTXML_ENDPOINT;
    if (message == nullptr)
        return rc;
    
    //EventXML endpoint is only supported with legacy connection type so the following casting is safe.
    LegacyPeerInfo& peerInfo = static_cast<LegacyPeerInfo&>(message->getPeerInfo());
    InboundRpcConnection& conn = peerInfo.getDeprecatedLegacyConnection();

    Protocol& protocol = conn.getProtocolBase();
    if (blaze_strcmp(protocol.getName() , EventXmlProtocol::getClassName()) != 0)
        return rc;
    rc = SubscribeError::ERR_OK;

    const char8_t* groupName = request.getConnectionGroupId();
    RegistrantMap::iterator requiredGroup = mRegistrantMap.find_as(groupName);

    bool isGroupNameEmpty = (strcmp(groupName, "") == 0);

    if (isGroupNameEmpty || requiredGroup == mRegistrantMap.end())
    {
        if (isGroupNameEmpty)
        {
            Registrants::iterator itr = mRegistrants.begin();
            Registrants::iterator end = mRegistrants.end();
            for (; itr != end; ++itr)
            {
                Registrant& registrant = *itr;
                if (registrant.getFirstConnection() == &conn)
                {
                    // Already registered so just update the info
                    return registrant.initialize(request, conn.getEncoder().getType(), message->getFrame().format);
                }
            }
        }

        return addConnection(request, conn, message->getFrame().format);        
    }
    

    Registrant* registrant = (requiredGroup->second);

    if (registrant->hasConnection(conn))
        return SubscribeError::ERR_OK;


    registrant->addConnection(conn);

    conn.addListener(*this);
    return SubscribeError::ERR_OK;
}

void EventManager::onConnectionDisconnected(Connection& connection)
{
    bool found = false;

    Registrants::iterator itr = mRegistrants.begin();
    Registrants::iterator end = mRegistrants.end();
    while (itr != end && !found)
    {
        Registrant& registrant = *itr;
        ++itr;

        if (strcmp(registrant.getConnectionGroupId(), "") == 0)
        {
            Connection* temp = static_cast<Connection*>(registrant.getFirstConnection());
            if (temp == &connection)
            {
                Registrants::remove(registrant);
                delete &registrant;
                break;
            }
        }
        else 
        {
            if (registrant.checkConnectionAndErase(connection))
            {
                found = true;
            }
            
            if (registrant.isListEmpty())
            {
                mRegistrantMap.erase(mRegistrantMap.find_as(registrant.getConnectionGroupId()));
                Registrants::remove(registrant);

                delete &registrant;
            }         
        }//end else
    }//end for
}

/*!************************************************************************************************/
/*! \brief create a timer to trigger the next heart beat event submission call.
*************************************************************************************************/
void EventManager::scheduleNextHeartBeat()
{
    // there's already a scheduled heart beat
    EA_ASSERT(mHeartBeatTimerId == INVALID_TIMER_ID); 

    mHeartBeatTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + getConfig().getHeartBeatInterval(),
                                                    this, &EventManager::submitHeartBeatEvent,
                                                    "EventManager::submitHeartBeatEvent");
}

/*!************************************************************************************************/
/*! \brief submit heartbeat event to all Registrants and schedule next event
*************************************************************************************************/
void EventManager::submitHeartBeatEvent()
{
    HeartBeat tdf;
    submitEvent(EventSlave::EVENT_HEARTBEATEVENT, tdf);
    mHeartBeatTimerId = INVALID_TIMER_ID;
    scheduleNextHeartBeat();
}
    
}// Event
} // Blaze

