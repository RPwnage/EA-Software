/*************************************************************************************************/
/*!
    \file messaging.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_MESSAGINGAPI_H
#define BLAZE_MESSAGINGAPI_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blaze_eastl/vector_map.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/component/messagingcomponent.h"
#include "BlazeSDK/component/messaging/tdf/messagingtypes.h"
#include "BlazeSDK/messaging/messagingdispatcher.h"
#include "BlazeSDK/messaging/messaging.h"

namespace Blaze
{

class BlazeHub;

namespace UserManager
{
    class User;
}

namespace GameManager
{
    class Game;
}

namespace Association
{
    class AssociationList;
}

namespace Util
{
    class FilterUserTextResponse;
}

namespace Messaging
{
    class MessageDispatcher;
    
/*! **************************************************************************************************************/
    /*!
    \class MessagingAPI
    \ingroup _mod_apis

    \brief Provides an interface to send and receive persistent or non-persistent messages to users connected to the 
    Blaze server. Also enables the user to fetch, purge and touch the persistent messages that collected in her inbox.

    \details Messaging API is used for sending messages to individuals or groups of individuals such as a Room
    or Game. The API can also be used to fetch and purge previously received persistent messages.
    
    \par (Fields)
    - Each message contains a number of client defined integer fields such as Type, Tag and Status.
    - Each received message contains a unique message ID and a timestamp of when the message was sent.

    \note
    - Status is the only field that can be updated by invoking MessagingAPI::touchMessages() @em after a persistent message 
    has been sent.
    - Status is also the only field that may be updated on messages that were @em either sent or received by the current user
    - An example usage of the status field is to implement a 'revoke message' behavior, which allows the sender to flag a sent
    persistent message as 'invalid' even after the message has been delivered to a recipient.

    \par (Attributes)
    - The message contains an attribute map of integer/string pairs. One such pair is called an attribute. 
    - The client is free to use any key values and arbitrary strings when inserting data into the message attribute map; 
    however key values in specific ranges will be processed differently by Blaze (when the appropriate message flags are set).
    - Message attributes support client and server side profanity filtering.
    - Message attributes support localization with optional parameters.
    - Message attributes split into 3 categories defined by attribute key ranges:
        -# Processed attributes whose keys fall into range <em>0x0 - 0xFFFF</em>. Such attributes can be filtered for profanity 
        or localized using the locale of the recipient. Either processing behavior is enabled by setting the appropriate 
        flag on the message.
        -# Parameter attributes whose keys fall into range <em>0x10000 - 0xA0000</em>. These attributes are substituted into 
        their corresponding @em processed parent attribute when the message localization flag is set @em and the 
        localizable attribute makes use of substitution parameters.
        -# Unprocessed attributes whose keys fall into range <em>0xB0000 - 0xBFFFF</em>. Such attributes are always delivered 
        unmodified to the recepient.
    - Profanity filtering and localization are mutually exclusive flags. All @em processed attributes can either be 
    filtered for profanity or localized, or neither.
    - Special message attributes defined by Blaze such as TITLE, SUBJECT and BODY fall into a reserved range 
    <em>0xFF00 - 0xFFFF</em> within the @em processed attribute category.
    - Each localizable attribute can make use of up to 10 optional parameter attributes that will be substituted into the 
    localized string.
    - The relationship between the localizable attribute and its optional parameter attributes is defined by a fixed offset 
    formula. For example: If the localizable attribute key is @em 0x0001, then the first parameter would have the 
    key @em 0x10001, while the fifth parameter would have the key @em 0x10005.
    - The above formula is provided for completeness, the user is urged to utilize methods SendMessageParameters::setAttrParam()
    and SendMessageParameters::getAttrParam(), which respectively set and get the localizable attribute's parameters by index.
    - During attribute localization all parameter attributes are always trimmed from the message before it is delivered to 
    the recipient.
    - Message attribute strings are 8 bit clean. Neither the MessagingAPI nor the Blaze server do any encoding. 
    This ensures that the messages are delivered to recipients AS IS. The maximum size of a string is @em 1024 bytes 
    including the nullptr terminator.
    
    \par (Persistent and Non-Persistent)
    - Non-persistent messages take no storage space; therefore, the maximum number of attributes is governed by the messaging.cfg 
    on the server. Each game team is free to define the attribute limit as it sees fit. Keep in mind however that care should be
    taken to limit the number of profanity filterable or localizable attributes in a messages as this puts additional overhead on
    the Blaze server.
    - Persistent messages are stored in the database; therefore, the maximum number of attributes is restricted by the structure 
    of a database table. (Currently the maximum is @em 4 attributes per message).
    - Persistent messages can be fetched from the user's inbox using a specific matching criteria. The typical use of fetching 
    is when the client first registers the MessagingAPI and enters the message GUI screen in order to view the list of messages.
    - Persistent messages can also be purged from the user's inbox using a specific matching criteria.
    
    \note
    - Typically it is not necessary to keep fetching messages after the MessagingAPI has been enabled, because once the user 
    is logged in, all relevant incoming messages will be routed to the handlers registered with the MessagingAPI.
    - The server can be configured to purge messages older than a certain age.
    
    \par (Send and Receive)
    - In order to receive incoming messages, the client must register a callback functor with the MessagingAPI.
    - The MessagingAPI supports 3 distinct types of message notification callbacks:
        -# User-specific message callback (MessagingAPI::UserMessageCb) delivers messages associated with a logged in User object
        -# UserGroup-specific message callback (MessagingAPI::RoomMessageCb, MessagingAPI::GameMessageCb, etc.) 
        delivers messages associated with a given UserGroup-derived object
        -# Global message callback (MessagingAPI::GlobalMessageCb) delivers all message notifications regardless of type
    - When registered, a MessagingAPI::GlobalMessageCb is <em>always</em> called for every incoming message, after all registered 
    target-specific callbacks have been invoked; therefore, the global callbacks functions as a catch-all, 
    but would not be commonly used, and is provided for completeness.
    
    \note For additional details please see Messaging component documentation on confluence.
    **************************************************************************************************************/
    
    class BLAZESDK_API MessagingAPI : public MultiAPI
    {
        NON_COPYABLE(MessagingAPI);
    public:

    /*! ****************************************************************************/
    /*! \name API types
    ********************************************************************************/

        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on completion of a SendMessage request.
        \param[in] BlazeError the error code for the SendMessage attempt
        \param[in] Provides status for the ongoing SendMessage attempt
        ***************************************************************************************************/
        typedef MessagingComponent::SendMessageCb SendMessageCb;

        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on completion of a FetchMessages request.
        \param[in] BlazeError the error code for the FetchMessages attempt
        \param[in] Provides status for the ongoing FetchMessages attempt
        ***************************************************************************************************/
        typedef MessagingComponent::FetchMessagesCb FetchMessagesCb;
        
        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on completion of a PurgeMessage request.
        \param[in] BlazeError the error code for the PurgeMessages attempt
        \param[in] Provides status for the ongoing PurgeMessages attempt
        ***************************************************************************************************/
        typedef MessagingComponent::PurgeMessagesCb PurgeMessagesCb;

        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on completion of a TouchMessage request.
        \param[in] BlazeError the error code for the TouchMessages attempt
        \param[in] Provides status for the ongoing TouchMessages attempt
        ***************************************************************************************************/
        typedef MessagingComponent::TouchMessagesCb TouchMessagesCb;

        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on receipt of _any_ message.
        \param[in] ServerMessage the received message object
        ***************************************************************************************************/
        typedef Functor1<const ServerMessage*> GlobalMessageCb;
        
        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on receipt of a User specific message.
        \param[in] ServerMessage the received message object
        \param[in] User the target object associated with the received ServerMessage, provides context
        ***************************************************************************************************/
        typedef Functor2<const ServerMessage*, const UserManager::User*> UserMessageCb;
        
        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on receipt of a Game specific message.
        \param[in] ServerMessage the received message object
        \param[in] Game the target object associated with the received ServerMessage, provides context
        ***************************************************************************************************/
        typedef Functor2<const ServerMessage*, GameManager::Game*> GameMessageCb;

        /*! ************************************************************************************************/
        /*! \brief The callback functor dispatched on receipt of a AssociationList specific message.
        \param[in] ServerMessage the received message object
        \param[in] AssociationList the target object associated with the received ServerMessage, provides context
        ***************************************************************************************************/
        typedef Functor2<const ServerMessage*, Association::AssociationList*> AssociationListMessageCb;

    /*! ****************************************************************************/
    /*! \name API methods
    ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Creates the MessagingAPI.

            \param hub The hub to create the API on.
            \param allocator pointer to the optional allocator to be used for the component; will use default if not specified. 
        ********************************************************************************/
        static void createAPI(BlazeHub &hub,EA::Allocator::ICoreAllocator* allocator = nullptr);

        /*! ****************************************************************************/
        /*! \brief Sends a message to target.
            \param request - contains message target, flags and payload
            \param cb - callback functor providing the status of operation
            \return JobId - provides status if operation is going to be lengthy
        ********************************************************************************/
        JobId sendMessage(const SendMessageParameters& request, const SendMessageCb& cb);
        
        /*! ****************************************************************************/
        /*! \brief Requests persistent messages stored for this user.
            \param request - filter flags, message ID, message type and sender group id
            \param cb - callback functor providing the status of operation
            \return JobId - provides status if operation is going to be lengthy
        ********************************************************************************/
        JobId fetchMessages(const FetchMessageParameters& request, const FetchMessagesCb& cb);

        /*! ****************************************************************************/
        /*! \brief Purges persistent messages stored for this user.
            \param request - filter flags, message ID, message type and sender group id
            \param cb - callback functor providing the status of operation
            \return JobId - provides status if operation is going to be lengthy
        ********************************************************************************/
        JobId purgeMessages(const PurgeMessageParameters& request, const PurgeMessagesCb& cb);
        
        /*! ****************************************************************************/
        /*! \brief Touches persistent messages that match the specified parameters.
            \param request - filter flags, message ID, message type and sender group id
            \param cb - callback functor providing the status of operation
            \return JobId - provides status if operation is going to be lengthy
        ********************************************************************************/
        JobId touchMessages(const TouchMessageParameters& request, const TouchMessagesCb& cb);

        /*! ****************************************************************************/
        /*! \brief Adds a callback to receive _all_ message notifications from the MessagingAPI.
            Once a callback is registered, MessagingAPI will forward incoming ServerMessage objects
            to the callback's implementer.
            \note Global message callbacks will always be called _after_ object-specific callbacks
            have been notified.
            \param cb The global message callback to add.
            \return true on success.
        ********************************************************************************/
        bool registerCallback(const GlobalMessageCb& cb);

        /*! ****************************************************************************/
        /*! \brief Adds a callback to receive _user_ message notifications from the MessagingAPI.
            Once a callback is registered, MessagingAPI will forward incoming ServerMessage objects
            to the callback's implementer.
            \param cb The user message callback to add.
            \return true on success.
        ********************************************************************************/
        bool registerCallback(const UserMessageCb& cb);
        
        /*! ****************************************************************************/
        /*! \brief Adds a callback to receive _UserGroup_ message notifications from the MessagingAPI.
            Once a callback is registered, MessagingAPI will forward incoming ServerMessage objects
            to the callback's implementer.
            \param cb The UserGroup message callback to add.
            \param entityType Entity type to register the callback for. If unspecified, or OBJECT_TYPE_INVALID,
                by default GroupType::getEntityType() is used.
            \return true on success.
        ********************************************************************************/
        template <class GroupType>
        bool registerCallback(const Functor2<const ServerMessage*, GroupType*>& cb, const EA::TDF::ObjectType& entityType = EA::TDF::OBJECT_TYPE_INVALID);
        
        /*! ****************************************************************************/
        /*! \brief Adds a callback to receive _UserGroup_ message notifications from the MessagingAPI.
            Once a callback is registered, MessagingAPI will forward incoming ServerMessage objects
            to the callback's implementer.
            \param componentId The component id to add this callback for
            \param cb The UserGroup message callback to add.
            \return true on success.
        ********************************************************************************/
        template <class GroupType>
        bool registerCallbackForComponent(ComponentId componentId, const Functor2<const ServerMessage*, GroupType*>& cb);

        /*! ****************************************************************************/
        /*! \brief Removes a callback previously registered to receive _all_ message notifications from the MessagingAPI,
            thus preventing any further ServerMessage objects from being dispatched to that callback's implementer.
            \param cb The global message callback to remove.
            \return true on success.
        ********************************************************************************/
        bool removeCallback(const GlobalMessageCb& cb);

        /*! ****************************************************************************/
        /*! \brief Removes a callback previously registered to receive _User_ message notifications from the MessagingAPI,
            thus preventing any further ServerMessage objects from being dispatched to that callback's implementer.
            \param cb The user message callback to remove.
            \return true on success.
        ********************************************************************************/
        bool removeCallback(const UserMessageCb& cb);
        
        /*! ****************************************************************************/
        /*! \brief Removes a callback previously registered to receive _UserGroup_ message notifications from the MessagingAPI,
            thus preventing any further ServerMessage objects from being dispatched to that callback's implementer.
            \param cb The UserGroup message callback to remove.
            \param entityType Entity type to register the callback for. If unspecified, or OBJECT_TYPE_INVALID,
                by default GroupType::getEntityType() is used.
            \return true on success.
        ********************************************************************************/
        template <class GroupType>
        bool removeCallback(const Functor2<const ServerMessage*, GroupType*>& cb, const EA::TDF::ObjectType& entityType = EA::TDF::OBJECT_TYPE_INVALID);

        /*! ****************************************************************************/
        /*! \brief Removes a callback previously registered to receive _UserGroup_ message notifications from the MessagingAPI,
            thus preventing any further ServerMessage objects from being dispatched to that callback's implementer.
            \param componentId The component id to remove from.
            \param cb The UserGroup message callback to remove.
            \return true on success.
        ********************************************************************************/
        template <class GroupType>
        bool removeCallbackForComponent(ComponentId componentId, const Functor2<const ServerMessage*, GroupType*>& cb);

        /*! ****************************************************************************/
        /*! \name Misc Helpers
        ********************************************************************************/

        //! \cond INTERNAL_DOCS
        /*! ****************************************************************************/
        /*! \brief Logs any initialization parameters for this API to the log.
        ********************************************************************************/
        void logStartupParameters() const;
        //! \endcond

        MessagingComponent* getComponent() const { return mComponent; }

    private: // accessed by friends
        /*! ****************************************************************************/
        /*! \brief Constructor.
            
            \param[in] blazeHub    pointer to Blaze Hub
            \param[in] memGroupId  mem group to be used by this class to allocate memory
 
            Private as all construction should happen via the factory method.
        ********************************************************************************/
        MessagingAPI(BlazeHub &blazeHub, uint32_t userIndex, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
        virtual ~MessagingAPI();

    private:
        void onMessageNotification(const ServerMessage* message, uint32_t userIndex);
        
        class FilteredMessageSender
        {
        public:
            FilteredMessageSender(MessagingAPI* api, MemoryGroupId memGroupId = Blaze::MEM_GROUP_FRAMEWORK_TEMP);
            JobId filterAndSend(const SendMessageParameters& request, const MessagingAPI::SendMessageCb& cb);
        private:
            void sendFilteredMessage(BlazeError aError, JobId jobId, const Util::FilterUserTextResponse* response);
            MessagingAPI* mApi;
            SendMessageParameters mMessage;
            MessagingAPI::SendMessageCb mCb;
            MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
        };
        
        typedef eastl::intrusive_list<FilteredMessageSender> SenderList;
        typedef vector_map<EA::TDF::ObjectType, MessageDispatcher*> DispatcherByTypeMap;
        typedef vector_map<ComponentId, MessageDispatcher*> DispatcherByComponentMap;
        
        MessagingComponent* mComponent;
        GlobalDispatcher mGlobalDispatcher;
        DispatcherByTypeMap mDispatcherByTypeMap;
        DispatcherByComponentMap mDispatcherByComponentMap;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
    };
    
    template <typename GroupType>
    inline bool MessagingAPI::registerCallback(const Functor2<const ServerMessage*, GroupType*>& cb, const EA::TDF::ObjectType& entityType)
    {
        // ensure the entityType arg for the GroupType, if explicitly specified, is valid.
        if ((entityType != EA::TDF::OBJECT_TYPE_INVALID) && !GroupType::isValidMessageEntityType(entityType))
        {
            BLAZE_SDK_DEBUGF("[MessagingAPI] Warning: Ignoring attempt to register invalid entity type %s for the group type\n", entityType.toString().c_str());
            return false;
        }
        const EA::TDF::ObjectType& usedEntityType = ((entityType == EA::TDF::OBJECT_TYPE_INVALID)? GroupType::getEntityType() : entityType);

        typedef GroupDispatcher<GroupType> GroupDispatcher;
        // lookup dispatcher registered for this blaze object type (Room, Game, etc.)
        DispatcherByTypeMap::iterator it = mDispatcherByTypeMap.find(usedEntityType);
        GroupDispatcher* dispatcher;
        if(it == mDispatcherByTypeMap.end())
        {
            dispatcher = BLAZE_NEW(mMemGroup, "GroupDispatcher") GroupDispatcher;
            mDispatcherByTypeMap.insert(eastl::make_pair(usedEntityType, dispatcher));
        }
        else
        {
            dispatcher = static_cast<GroupDispatcher*>(it->second);
        }
        return dispatcher->addCallback(cb);
    }
    

    template <typename GroupType>
    inline bool MessagingAPI::removeCallback(const Functor2<const ServerMessage*, GroupType*>& cb, const EA::TDF::ObjectType& entityType)
    {
        const EA::TDF::ObjectType& usedEntityType = ((entityType == EA::TDF::OBJECT_TYPE_INVALID)? GroupType::getEntityType() : entityType);

        typedef GroupDispatcher<GroupType> GroupDispatcher;
        // lookup dispatcher registered for this blaze object type (Room, Game, etc.)
        DispatcherByTypeMap::iterator it = mDispatcherByTypeMap.find(usedEntityType);
        if(it != mDispatcherByTypeMap.end())
        {
            return static_cast<GroupDispatcher*>(it->second)->removeCallback(cb);
        }
        return false;
    }

    template <typename GroupType>
    inline bool MessagingAPI::registerCallbackForComponent(ComponentId componentId, const Functor2<const ServerMessage*, GroupType*>& cb)
    {
        typedef GroupDispatcher<GroupType> GroupDispatcher;

        // lookup dispatcher registered for this component
        DispatcherByComponentMap::iterator it = mDispatcherByComponentMap.find(componentId);
        GroupDispatcher* dispatcher;
        if(it == mDispatcherByComponentMap.end())
        {
            dispatcher = BLAZE_NEW(mMemGroup, "GroupDispatcher") GroupDispatcher;
            mDispatcherByComponentMap.insert(eastl::make_pair(componentId, dispatcher));
        }
        else
        {
            dispatcher = static_cast<GroupDispatcher*>(it->second);
        }
        return dispatcher->addCallback(cb);
    }


    template <typename GroupType>
    inline bool MessagingAPI::removeCallbackForComponent(ComponentId componentId, const Functor2<const ServerMessage*, GroupType*>& cb)
    {
        typedef GroupDispatcher<GroupType> GroupDispatcher;
        // lookup dispatcher registered for this blaze object type (Room, Game, etc.)
        DispatcherByComponentMap::iterator it = mDispatcherByComponentMap.find(componentId);
        if(it != mDispatcherByComponentMap.end())
        {
            return static_cast<GroupDispatcher*>(it->second)->removeCallback(cb);
        }
        return false;
    }
    
} // namespace Messaging
} // namespace Blaze
#endif // BLAZE_MESSAGINGAPI_H
