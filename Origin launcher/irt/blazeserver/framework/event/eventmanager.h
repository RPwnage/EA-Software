/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EVENTMANAGER_H
#define BLAZE_EVENTMANAGER_H

/*** Include files *******************************************************************************/

#include "framework/rpc/eventslave_stub.h"
#include "framework/tdf/eventtypes_server.h"
#include "framework/connection/connection.h"
#include "framework/system/fibermutex.h"
#include "framework/grpc/outboundgrpcjobhandlers.h"

#include "notify/rpc/notifyslave.h"
#include "notify/tdf/notify.h"

#include "EASTL/intrusive_list.h"
#include "EASTL/hash_set.h"
#include "EASTL/vector.h"
#include "EASTL/deque.h"

#include <EATDF/tdf.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace eadp {
    namespace messagebus {
        class ProducerRecord;
    }
}

namespace Blaze
{

class EventXmlEncoder;
class InboundRpcConnection;


namespace Event
{

class EventManager
    : public EventSlaveStub,
      private ConnectionListener
{
    NON_COPYABLE(EventManager);

public:
    EventManager();
    ~EventManager() override;

    void submitEvent(uint32_t eventId, const EA::TDF::Tdf& event, bool logBinaryEvent = false);
    void submitBinaryEvent(uint32_t eventId, const EA::TDF::Tdf& binaryEvent) const;
    void submitSessionNotification(Notification& notification, const UserSession& session);
    SubscribeError::Error addConnection(const SubscriptionInfo &request, InboundRpcConnection& conn, Blaze::RpcProtocol::ResponseFormat respFormat);

    void getStatus(EventManagerStatus& status);

private:
    static SubscribeError::Error validateSubscription(const SubscriptionInfo& info, bool validateClientName = true, ConfigureValidationErrors* validationErrors = nullptr);

private: 

    class RegistrantBase
    {
    public:
        RegistrantBase() : mAllComponents(false) { *mClientName = '\0'; }
        virtual ~RegistrantBase() {}

        virtual void pushEvent(Notification& notification, bool allConns) = 0;
        virtual void sendEvent(Notification& notification, bool allConns);

        SubscribeError::Error initialize(const SubscriptionInfo& info);

    protected:
        typedef uint32_t ComponentEventId;
        typedef eastl::vector<ComponentId> ComponentIdList;
        typedef eastl::hash_set<ComponentEventId> NotificationIdMap;

        bool mAllComponents;
        ComponentIdList mComponents;
        NotificationIdMap mEventBlacklistMap;
        NotificationIdMap mEventWhitelistMap;
        char8_t mClientName[SubscriptionInfo::MAX_CLIENTNAME_LEN];

    protected:
        inline ComponentEventId combineNotificationKey(ComponentId id, EventId eventId);
    };

    class Registrant : public RegistrantBase, public eastl::intrusive_list_node
    {
        NON_COPYABLE(Registrant);

    public:
        Registrant(InboundRpcConnection&);
        ~Registrant() override;

        SubscribeError::Error initialize(const SubscriptionInfo& info, Encoder::Type encoderType, Blaze::RpcProtocol::ResponseFormat respFormat);

        void pushEvent(Notification& notification, bool allConns) override;

        void sendSessionNotification(Notification& notification, const UserSession& session);

        InboundRpcConnection* getFirstConnection() { return (mConnectionList.at(0)).mConnection; }

        bool hasConnection(InboundRpcConnection&);
        
        bool isListEmpty() {return mConnectionList.empty();}

        bool checkConnectionAndErase(Connection&);

        const char8_t* getConnectionGroupId() const { return mConnectionGroupId.c_str(); }

        void addConnection(InboundRpcConnection& conn);

    private:

        bool mAllSessionNotifications;
        ComponentIdList mNotificationComponents;
        RegistrantBase::NotificationIdMap mSessNotificationBlacklistMap;
        RegistrantBase::NotificationIdMap mSessNotificationWhitelistMap;
        RpcProtocol::ResponseFormat mResponseFormat;
        eastl::string mConnectionGroupId;
        uint32_t mCurrent;
        TdfEncoder* mSubEncoder;

        struct SingleConnection
        {
            SingleConnection()
                : mFirstEvent(true),
                mConnection(nullptr)
            {
            }

            bool mFirstEvent;
            InboundRpcConnection* mConnection;
        };

    public:
        typedef eastl::vector<SingleConnection> ConnectionList;
        const ConnectionList* getConnectionList() const { return &mConnectionList; }

    private:
        ConnectionList mConnectionList;

    };

    class NotifyRegistrant : public RegistrantBase
    {
        NON_COPYABLE(NotifyRegistrant);

    public:
        NotifyRegistrant();
        ~NotifyRegistrant() override;

        SubscribeError::Error initialize(const EventConfig& config, bool isReconfigure);

        void pushEvent(Notification& notification, bool allConns) override;

        void publishEventsToNotify();
        void publishEventsToNotify(const eastl::string& messageType);

        bool prepareForReconfigure(const EventConfig& config, const EventConfig& newConfig);

    private:
        void doPublishEventsToNotify(Notify::PostNotificationsRequestPtr req);
        void queueEventForPublish(ComponentEventId compEventId, EventEnvelopePtr event);

    private:
        typedef eastl::deque<EventEnvelopePtr> EventDeque;
        typedef eastl::string MessageType;
        typedef eastl::hash_map<MessageType, EventDeque> EventsByMessageTypeCache;
        typedef eastl::hash_map<ComponentEventId, MessageType> MessageTypesByComponentEvent;
        typedef eastl::hash_map<ComponentId, ClientPlatformSet> PlatformsByComponent;

        bool buildMessageTypeMappings(const EventConfig::ComponentNotifyEventMap& eventMap, MessageTypesByComponentEvent& messageTypesByComponentEvent, PlatformsByComponent& platformsByComponent, SubscriptionInfo* info);

        EventsByMessageTypeCache mEventsByMessageTypeCache;
        MessageTypesByComponentEvent mMessageTypesByComponentEvent;
        Notify::NotifySlave* mNotifyComp; // Not owned memory
        EA::TDF::TimeValue mPublishPeriod;
        TimerId mPublishTimer;
        Fiber::FiberId mPublishFiberId;
        Fiber::FiberGroupId mPublishFiberGroupId;
        PlatformsByComponent mPlatformsByComponent;
    };

    class MessageBusRegistrant : public RegistrantBase
    {
        NON_COPYABLE(MessageBusRegistrant);

    public:
        MessageBusRegistrant() : RegistrantBase() {}
        ~MessageBusRegistrant() override;

        void pushEvent(Notification& notification, bool allConns) override;
        void pushEventInternal(eadp::messagebus::ProducerRecord* requestPtr);

        void refreshMessageBusRpc();
    private:
        // gRPC handler for the MessageBus integration:
        Grpc::OutboundRpcBasePtr mMessageBusRpc;
        eastl::set<Fiber::FiberId> mFiberIdSet;
    };

    typedef eastl::intrusive_list<Registrant> Registrants;
    Registrants mRegistrants; // contains all 'groups' except the ones with no name- 'the default case'

    typedef eastl::hash_map<eastl::string, Registrant*> RegistrantMap;
    RegistrantMap mRegistrantMap;

    EventXmlEncoder& mLogEncoder;
    TimerId mHeartBeatTimerId;

    NotifyRegistrant mNotifyRegistrant;

    struct EventMetrics
    {
        EventMetrics() : mCountSent(0), mBytesSent(0), mCountDropped(0), mBytesDropped(0) {}

        uint64_t mCountSent;
        uint64_t mBytesSent;
        uint64_t mCountDropped;
        uint64_t mBytesDropped;
    };

    typedef eastl::hash_map<const char8_t*, EventMetrics, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > EventCountsByName;
    EventCountsByName mEventCountsByName;

    MessageBusRegistrant mMessageBusRegistrant;  // Tracks which events should be sent to the bus. 

private:
    // Component interface
    bool onConfigure() override;
    bool onPrepareForReconfigure(const EventConfig& newConfig) override;
    bool onReconfigure() override;
    bool onActivate() override;
    bool onValidateConfig(EventConfig& config, const EventConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

    SubscribeError::Error processSubscribe(
        const SubscriptionInfo &request, const Message* message) override;
    // ConnectionListener interface
    void onConnectionDisconnected(Connection& connection) override;

    void scheduleNextHeartBeat();
    void submitHeartBeatEvent();

};


} // Event

extern EA_THREAD_LOCAL Event::EventManager* gEventManager;

} // Blaze

#endif // BLAZE_EVENTMANAGER_H

