// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Service.h>
#include <eadp/realtimemessaging/ConnectionService.h>
#include <com/ea/eadp/antelope/rtm/protocol/NotificationV1.h>

namespace eadp
{

namespace realtimemessaging
{

/*!
 * @brief Notification service is used to listen to Groups & Friends Notification for the current user
 */
class EADPREALTIMEMESSAGING_API NotificationService : public eadp::foundation::IService
{
public:

    /*!
     * @brief Triggered when there is a notification for the current user
     */
    using NotificationCallback = eadp::foundation::CallbackT<void(const eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1> notificationUpdate)>;

    /*!
     * @brief Create the Notification service instance
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub provides the logging service.
     * @return The NotificationService instance
     */
    static eadp::foundation::SharedPtr<NotificationService> createService(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);

    /*!
     * @brief Initializes the NotificationService object
     *
     * @param connectionService Requires an ConnectionService object. The connection on ConnectionService should be established.
     * @param notificationCallback Callback to be triggered when a notification received
     */
    virtual eadp::foundation::ErrorPtr initialize(eadp::foundation::SharedPtr<ConnectionService> connectionService, NotificationCallback notificationCallback) = 0;


    virtual ~NotificationService() = default;

};
}
}
