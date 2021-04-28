#ifndef __ORIGIN_EVENT_CLASSES_H__
#define __ORIGIN_EVENT_CLASSES_H__

#include "OriginSDK/OriginTypes.h"
#include "OriginSDKimpl.h"
#include "OriginLSXEvent.h"
#include <vector>
#include <lsx.h>
#include <ReaderCommon.h>
#include <assert.h>
#include <algorithm>
#include <mutex>

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable:4355)   // 'this' : used in base member initializer list
#endif

namespace Origin
{
    
    // IEnumerator implementation for enumerating events
    template<typename EventType, typename UserType, typename EventHandler>
    class EventEnumerator : public Origin::IEnumerator
    {
    public:
        EventEnumerator (EventHandler& handler, size_t numEvt) : m_handler(handler), m_totalCount(0U) { m_events.reserve(numEvt); }; 

		EventEnumerator(const EventEnumerator &) = delete;
		EventEnumerator(EventEnumerator &&) = delete;
		EventEnumerator & operator = (const EventEnumerator &) = delete;
		EventEnumerator & operator = (EventEnumerator &&) = delete;

        void Add (const EventType& evt) 
        {
            m_events.push_back(evt);
            m_totalCount += m_handler.TypeCount(m_events.back()); 
        }

        virtual bool HasCallback() { return false; };
        virtual OriginErrorT DoCallback() { return REPORTERROR(ORIGIN_SUCCESS); }
        virtual OriginErrorT HandleMessage(INodeDocument* /*doc*/) { return REPORTERROR(ORIGIN_SUCCESS); }
        virtual bool IsReady() { return true; };
        virtual bool IsTimedOut() { return false; };
        virtual OriginErrorT GetErrorCode() { return REPORTERROR(ORIGIN_SUCCESS); };
        virtual size_t GetTotalCount() { return m_totalCount; };
        virtual size_t GetAvailableCount() { return m_totalCount; };
        virtual size_t GetTypeSize() { return sizeof(EventType); };
        virtual bool Wait() { return true; };
        virtual bool HasClientCallback() { return false; };
        virtual void SetClientCallback(void * /*pContext*/, OriginEnumerationCallbackFunc /*callback*/) { };
        virtual void StoreMemory(void * /*pData*/) {assert(0);};

        void *operator new(size_t size)
        {
            return Origin::AllocFunc(size);
        }

        void operator delete(void *p)
        {
            Origin::FreeFunc(p);
        }
            
        virtual OriginErrorT ConvertData(void *pBufPtr, size_t iBufLen, size_t index, size_t count, size_t *pItemCnt)
        {
            if (count * sizeof(UserType) > iBufLen)
                return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);

            if (index > m_totalCount)
            {
                *pItemCnt = 0;
                return REPORTERROR(ORIGIN_SUCCESS);
            }

            if (count > (m_totalCount - index))
                count = m_totalCount - index;

            UserType* outptr = static_cast<UserType*>(pBufPtr);
            size_t in_index = 0, out_index = 0, cur_index = 0;
            while (out_index < count)
            {
                size_t in_count = m_handler.TypeCount(m_events[in_index]);
                size_t local_index = 0;
                while ((local_index < in_count) && (out_index < count))
                {
                    if ((cur_index >= index) && (cur_index < (index+count)))
                    {
                        m_handler.TypeConvert(m_events[in_index], local_index, outptr[out_index]);
                        out_index += 1;
                    }

                    local_index += 1;
                    cur_index += 1;
                }

                in_index += 1;
            }

            *pItemCnt = out_index;
            return REPORTERROR(ORIGIN_SUCCESS);
        }

    private:
        virtual uint32_t GetPacketIdHash() { return 0; };

    private:
        EventHandler&			                            m_handler;
        std::vector<EventType, Allocator<EventType> >		m_events;
        size_t						                        m_totalCount;
    };

    // Interface class for all the event handlers
    class IEventHandler
    {
        struct CallbackInfo
        {
            CallbackInfo() : callback(NULL), context(NULL) {}
            CallbackInfo(OriginEventCallback cb, void *cxt) : callback(cb), context(cxt) {}

            void clear(){callback = NULL; context = NULL;}

            bool operator == (const CallbackInfo &cbi)
            { 
                return cbi.callback == callback && cbi.context == context; 
            }

            OriginEventCallback callback;
            void *              context;
        };

        typedef std::vector<CallbackInfo, Allocator<CallbackInfo> > CallbackInfoCollection;

    public:
        IEventHandler(int32_t kind) : m_Kind(kind) {}
        virtual ~IEventHandler(void) {};
        virtual void RegisterMessageHandler(OriginSDK& sdk) = 0;
        virtual void DeregisterMessageHandler(OriginSDK& sdk) = 0;
        virtual std::recursive_mutex & GetLock() = 0;
		
        bool AddEventCallback(OriginEventCallback callback, void* context) 
        { 
            std::lock_guard<std::recursive_mutex> msgLock(this->GetLock()); 

            CallbackInfo newCallback(callback, context);

            // If not already in the list, add to the list.
            if(std::find(m_Callbacks.begin(), m_Callbacks.end(), newCallback) == m_Callbacks.end())
            {
                m_Callbacks.push_back(newCallback);
                return true;
            }
            return false;
        }

        bool RemoveEventCallback(OriginEventCallback callback, void* context) 
        { 
            std::lock_guard<std::recursive_mutex> msgLock(this->GetLock()); 

            CallbackInfoCollection::iterator i = std::find(m_Callbacks.begin(), m_Callbacks.end(), CallbackInfo(callback, context));

            if(i != m_Callbacks.end())
            {
                m_Callbacks.erase(i);
                return true;
            }
            return false;
        }

        void SetEventFallbackCallback(OriginEventCallback callback, void* context) 
        { 
            std::lock_guard<std::recursive_mutex> msgLock(this->GetLock()); 

            m_FallbackCallback = CallbackInfo(callback, context);
        }

        void ResetEventFallbackCallback() 
        { 
            std::lock_guard<std::recursive_mutex> msgLock(this->GetLock()); 

            m_FallbackCallback.clear();
        }

        bool HasEventCallback (void) 
        { 
            return m_Callbacks.size()>0 || m_FallbackCallback.callback != NULL; 
        }

        OriginErrorT DoEventCallback (void* eventData, size_t count)
        { 
            if(m_Callbacks.size() > 0)
            {
                for(unsigned int i=0; i<m_Callbacks.size(); i++)
                {
                    CallbackInfo ci;

                    {
                        std::lock_guard<std::recursive_mutex> msgLock(this->GetLock()); 
                        ci = m_Callbacks[i];
                    }

                    if (ci.callback != NULL) 
                    {
                        ci.callback(m_Kind, ci.context, eventData, count); 
                    }
                }
            }
            else
            {
                // Do the fallback.
                CallbackInfo ci;

                {
                    std::lock_guard<std::recursive_mutex> msgLock(this->GetLock()); 
                    ci = m_FallbackCallback;
                }

                if (ci.callback != NULL) 
                {
                    ci.callback(m_Kind, ci.context, eventData, count); 
                }
            }
            return REPORTERROR(ORIGIN_SUCCESS); 
        }

    private:
        int32_t					    m_Kind;			// ORIGIN_EVENT_*
        CallbackInfoCollection      m_Callbacks;  
        CallbackInfo                m_FallbackCallback;
    };

    // template class for implementing IEventHandler
    template<typename EventType, typename UserType>
    class EventHandler : public IEventHandler, public IXmlMessageHandler
    {
    public:
        EventHandler(int32_t kind) : IEventHandler(kind), m_key(0ULL) {};
        virtual ~EventHandler() {};

        void *operator new(size_t size)
        {
            return Origin::AllocFunc(size);
        }

        void operator delete(void *p)
        {
            Origin::FreeFunc(p);
        }

        virtual uint32_t GetPacketIdHash() 
        {
            EventType evt;
			return evt.GetHash(); 
        }

        virtual bool HasCallback() { return true; };			// need a DoCallback for each arrived event
        virtual OriginErrorT DoCallback()	// process next pending event into events list and call user callback
        {
            std::lock_guard<std::recursive_mutex> msgLock(m_lock);
            if (!m_pending.empty())
            {
                EventType theEvent = m_pending.front();
                m_pending.pop_front();
        
                if (this->HasEventCallback())
                {
                    size_t count = this->TypeCount(theEvent);

					if(count == 1)
					{
						UserType userType;
						this->TypeConvert(theEvent, 0, userType);
						this->DoEventCallback(&userType, 1); 
					}
					else
					{
                        UserType *userTypes = TYPE_NEW(UserType, count);

						for (uint32_t index = 0; index < count; ++index)
						{
							this->TypeConvert(theEvent, index, userTypes[index]);
						}

						this->DoEventCallback(userTypes, count); 

						TYPE_DELETE(userTypes);
					}

                    EmptyDataStore();
                }
            }
            return ORIGIN_SUCCESS;
        }

        virtual OriginErrorT HandleMessage(INodeDocument* doc)
        {
            LSXMessage<EventType> theEvent("Event", "");
            if(theEvent.FromXml(doc))
            {
                // preprocess the incoming data (apply filters, cleanup, whatever)
                this->Preprocess(theEvent);

                // events go into pending list until the DoCallback is invoked
                std::lock_guard<std::recursive_mutex> msgLock(m_lock);
                m_pending.push_back(theEvent);	
                return REPORTERROR(ORIGIN_SUCCESS);
            }
            else
                return REPORTERROR(ORIGIN_ERROR_LSX_INVALID_RESPONSE);
        }

        virtual void RegisterMessageHandler(OriginSDK& sdk) { m_key = sdk.RegisterMessageHandler(this, 0); }
        virtual void DeregisterMessageHandler(OriginSDK& sdk) { sdk.DeregisterMessageHandler(m_key); }

        virtual bool Wait(uint32_t /*timeout*/) { return true; }
        virtual	bool IsTimedOut() { return false; }
        virtual bool IsReady() { std::lock_guard<std::recursive_mutex> msgLock(m_lock); return m_pending.size() > 0; }
        virtual OriginErrorT GetErrorCode() { return ORIGIN_SUCCESS; } 
        virtual std::recursive_mutex & GetLock() { return m_lock; }

        // must be hand-coded in base class in order to instantiate template
        virtual void Preprocess (EventType&) const { };										// message inputs prior to being counted and processed
        virtual size_t TypeCount (const EventType& /*input*/) const { return 1; };					// returns number of UserType in the input
        virtual void TypeConvert (const EventType& input, size_t index, UserType& output) = 0;	// converts entry 'index' in input to the output

        virtual void StoreMemory(void *pData)
        {
            m_DataStore.push_back(pData);
        }

        void EmptyDataStore()
        {
            for(auto data: m_DataStore)
            {
                TYPE_DELETE(data);
            }
            m_DataStore.clear();
        }

		EventHandler(const EventHandler &) = delete;
		EventHandler(EventHandler &&) = delete;
		EventHandler & operator = (const EventHandler &) = delete;
		EventHandler & operator = (EventHandler &&) = delete;

    private:
        OriginSDK::HandlerKey                                       m_key;
        std::recursive_mutex                                        m_lock;
        std::deque<EventType, Allocator<EventType> >                m_pending;      // events delivered but not yet processed by DoCallback
        MemoryContainer                                             m_DataStore;    // storage for temporary memory associated with the event.
    };


    // Event-specific EventHandler<> instantiations
	namespace Event
	{

#define DECLARE_EVENT_HANDLER(EVENTTYPE, EVENT_LSX_TYPE, ORIGINTYPE)\
            class EVENTTYPE : public EventHandler<EVENT_LSX_TYPE, ORIGINTYPE> {	public:\
				EVENTTYPE(int32_t kind) : EventHandler<EVENT_LSX_TYPE, ORIGINTYPE>(kind) {};\
				EVENTTYPE(const EVENTTYPE &) = delete;\
				EVENTTYPE(EVENTTYPE &&) = delete;\
				EVENTTYPE & operator = (const EVENTTYPE &) = delete;\
				EVENTTYPE & operator = (EVENTTYPE &&) = delete;\
				virtual void TypeConvert(const EVENT_LSX_TYPE& input, size_t index, ORIGINTYPE& output);}

#define DECLARE_COMPLEX_EVENT_HANDLER(EVENTTYPE, EVENT_LSX_TYPE, ORIGINTYPE)\
            class EVENTTYPE : public EventHandler<EVENT_LSX_TYPE, ORIGINTYPE> {	public:\
                EVENTTYPE(int32_t kind) : EventHandler<EVENT_LSX_TYPE, ORIGINTYPE>(kind) {};\
				EVENTTYPE(const EVENTTYPE &) = delete;\
				EVENTTYPE(EVENTTYPE &&) = delete;\
				EVENTTYPE & operator = (const EVENTTYPE &) = delete;\
				EVENTTYPE & operator = (EVENTTYPE &&) = delete;\
                virtual void Preprocess (EVENT_LSX_TYPE& input) const;\
                virtual size_t TypeCount (const EVENT_LSX_TYPE& input) const;\
                virtual void TypeConvert (const EVENT_LSX_TYPE& input, size_t index, ORIGINTYPE& output);}

        DECLARE_EVENT_HANDLER(Broadcast, lsx::BroadcastEventT, uint32_t);						        // parameter is one of the ORIGIN_BROADCASTSTATE_* defines
        DECLARE_EVENT_HANDLER(IGO, lsx::IGOEventT, uint32_t);								            // parameter is one of the ORIGIN_IGOSTATE_* defines
        DECLARE_EVENT_HANDLER(Invite, lsx::MultiplayerInviteT, OriginInviteT);				            // parameter is struct OriginInviteT
        DECLARE_EVENT_HANDLER(Login, lsx::LoginT, OriginLoginT);									    // parameter is one of the ORIGIN_LOGINSTATUS_* defines
        DECLARE_EVENT_HANDLER(Profile, lsx::ProfileEventT, OriginProfileChangeT);					    // parameter indicates for what user the profile changed, and what part of the profile changed.
		DECLARE_EVENT_HANDLER(Presence, lsx::GetPresenceResponseT, OriginPresenceT);			        // parameter is user id
        DECLARE_COMPLEX_EVENT_HANDLER(Friends, lsx::QueryFriendsResponseT, OriginFriendT);	            // parameter the new list of friends.
        DECLARE_EVENT_HANDLER(Purchase, lsx::PurchaseEventT, const OriginCharT*);			            // parameter is manifest id
        DECLARE_COMPLEX_EVENT_HANDLER(Content, lsx::CoreContentUpdatedT, OriginContentT);	            // parameter is struct OriginDownloadT
		DECLARE_EVENT_HANDLER(Blocked, lsx::BlockListUpdatedT, uint32_t);					            // parameter is uint32_t which is not used
		DECLARE_EVENT_HANDLER(ChatMessage, lsx::ChatMessageEventT, OriginChatMessageT);					    // parameter is OriginChatT structure.
        DECLARE_EVENT_HANDLER(Online, lsx::OnlineStatusEventT, bool);						            // parameter is bool.
        DECLARE_COMPLEX_EVENT_HANDLER(Achievement, lsx::AchievementSetsT, OriginAchievementSetT);	    // parameter is struct OriginAchievementSetT
        DECLARE_EVENT_HANDLER(InvitePending, lsx::MultiplayerInvitePendingT, OriginInvitePendingT);	    // parameter is struct OriginInvitePendingT
		DECLARE_EVENT_HANDLER(Visibility, lsx::PresenceVisibilityEventT, bool);				            // parameter is bool.
		DECLARE_EVENT_HANDLER(CurrentUserPresence, lsx::CurrentUserPresenceEventT, OriginPresenceT);    // parameter is user id
        DECLARE_COMPLEX_EVENT_HANDLER(NewItems, lsx::QueryEntitlementsResponseT, OriginItemT);	        // parameter the newly acquired entitlements.
		DECLARE_EVENT_HANDLER(ChunkProgress, lsx::ChunkStatusT, OriginChunkStatusT);					// parameter is a OriginChunkStatusT structure.
		DECLARE_EVENT_HANDLER(IGOUnavailable, lsx::IGOUnavailableT, uint32_t);							// parameter is one of the ORIGIN_IGO_UNAVAILABLE_REASON_XXXX 
        DECLARE_EVENT_HANDLER(MinimizeRequest, lsx::MinimizeRequestT, uint32_t);
        DECLARE_EVENT_HANDLER(RestoreRequest, lsx::RestoreRequestT, uint32_t);
        DECLARE_EVENT_HANDLER(GameMessage, lsx::GameMessageEventT, OriginGameMessageT);
        DECLARE_COMPLEX_EVENT_HANDLER(Group, lsx::GroupEventT, OriginFriendT);                  // The event contains an array of friends.
        DECLARE_EVENT_HANDLER(GroupEnter, lsx::GroupEnterEventT, OriginGroupInfoT);             
        DECLARE_EVENT_HANDLER(GroupLeave, lsx::GroupLeaveEventT, const OriginCharT *);                  // The GroupId of the group that was left.
        DECLARE_EVENT_HANDLER(GroupInvite, lsx::GroupInviteEventT, OriginGroupInviteT);
        DECLARE_EVENT_HANDLER(Voip, lsx::VoipStatusEventT, OriginVoipStatusEventT);
        DECLARE_EVENT_HANDLER(UserInvited, lsx::UserInvitedEventT, OriginUserT);                        // parameter is the OriginUserT *
        DECLARE_EVENT_HANDLER(ChatStateUpdated, lsx::ChatStateUpdateEventT, OriginChatStateUpdateMessageT); // parameter is the OriginChatStateUpdateMessageT *
        DECLARE_EVENT_HANDLER(OpenSteamStore, lsx::SteamActivateOverlayToStoreEventT, OriginOpenSteamStoreT);
		DECLARE_EVENT_HANDLER(SteamAchievement, lsx::SteamAchievementEventT, SteamAchievementT);
    }
}

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#endif //__ORIGIN_EVENT_CLASSES_H__
