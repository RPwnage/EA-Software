// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Service.h>
#include <eadp/realtimemessaging/ConnectionService.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceUpdateV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceUpdateErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceSubscriptionErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceSubscribeV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PresenceUnsubscribeV1.h>

namespace eadp
{

namespace realtimemessaging
{

/*!
 * @brief Presence service is used to subscribe/unsubscribe for presence updates of other users
 */
class EADPREALTIMEMESSAGING_API PresenceService : public eadp::foundation::IService
{
public:

    /*!
     * @brief Invoked when the presence of the user from the current users Presence subscription roster changes.
     */
    using PresenceUpdateCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1> presenceUpdate)>;

    /*!
     * @brief Invoked when a presence status error is received. 
     *
     * This will be invoked for every presence status update error. 
     */
    using PresenceUpdateErrorCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1> presenceUpdateError)>;

    /*!
     * @brief Invoked when there is an error subscribing to a user's presence.
     * 
     * This will be called after the subscribe() function. Error for each user will be called seperately
     */
    using PresenceSubscriptionErrorCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1> presenceSubscriptionError)>;

    /*!
     * @brief Generic callback used by multiple methods
     * @param Error If Error object is empty then operation was successful
     */
    using ErrorCallback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr&)>;

    /*!
     * @brief Creates an instance of this service for a given hub.
     * 
     * The service will be available when the Hub is live.
     * 
     * @param hub The hub to use for this service. 
     * @return A PresenceService instance
     */
    static eadp::foundation::SharedPtr<PresenceService> createService(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);

    /*!
     * @brief Initializes the PresenceService object
     *
     * @param connectionService Requires a ConnectionService object. The connection on ConnectionService should be established.
     * @param presenceUpdateCallback Update Callback invoked on a Presence Update
     * @param presenceSubscriptionErrorCallback Error Callback invoked for Subscription error
     * @param presenceUpdateErrorCallback Error Callback invoked when a update presence status fails
     */
    virtual eadp::foundation::ErrorPtr initialize(eadp::foundation::SharedPtr<ConnectionService> connectionService, 
                                                  PresenceUpdateCallback presenceUpdateCallback, 
                                                  PresenceSubscriptionErrorCallback presenceSubscriptionErrorCallback, 
                                                  PresenceUpdateErrorCallback presenceUpdateErrorCallback) = 0;

    /*!
     * @brief Updates the presence status for the currently logged-in user. Callback will only fire if an error occurs on the server.
     *
     * @param presenceUpdate Request to update Presence of current User
     * @param errorCallback callback triggered on success/error
     */
    virtual void updateStatus(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateV1> presenceUpdate, ErrorCallback errorCallback) = 0;

    /*!
     * @brief Adds the given persona IDs to the current presence subscription roster. 
     * Callback will only fire if an error occurs on the server. An error callback is invoked for each Persona ID that encounters a subscribe failure.
     *
     * @param presenceUpdate Request to subscribe to a list of Player's presence
     * @param errorCallback callback triggered on success/error
     */
    virtual void subscribe(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1> presenceSubscribeRequest, ErrorCallback errorCallback) = 0;

    /*
     * @brief Removes the given persona IDs to the current presence subscription roster. 
     * Callback will only fire if an error occurs on the server. An error callback is invoked for each Persona ID that encounters an unsubscribe failure.
     *
     * @param presenceUpdate Request to unsubscribe to a list of Player's presence
     * @param errorCallback callback triggered on success/error
     */
    virtual void unsubscribe(eadp::foundation::UniquePtr<com::ea::eadp::antelope::rtm::protocol::PresenceUnsubscribeV1> presenceUnsubscribeRequest, ErrorCallback errorCallback) = 0;


    virtual ~PresenceService() = default;

};

}
}
