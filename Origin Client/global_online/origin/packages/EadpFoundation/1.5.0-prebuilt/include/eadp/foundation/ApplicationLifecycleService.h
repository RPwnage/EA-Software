// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Callback.h>

#if defined(EA_PLATFORM_ANDROID)
#include <jni.h>
#endif

namespace eadp
{
namespace foundation
{

/*!
 */
class EADPSDK_API IApplicationLifecycleService
{
public:
    enum class Event
    {
        // common events
        /*!
         * Application launch event
         * @note On iOS and Android, if the hub is initialized after app launching event,
         * the event will not be raised.
         */
        APP_LAUNCH,
        /*!
         * Application suspend event
         * @note On PS4, PC and Linux, this event will not be raised.
         */
        APP_SUSPEND,
        /*!
         * Application suspend event
         * @note On PC and Linux, this event will not be raised.
         */
        APP_RESUME,
        /*!
         * Application quit event
         * @note On non-mobile platform, this event will raised at the hub destruction,
         * so no more asynchronous operation, job or callback should be raised from this event.
         * On mobile, don't expect this event to be raised consistently.
         */
        APP_QUIT,

        // platform sepcfic events
        /*!
         * iOS only - application is opened by URL event
         */
        IOS_APP_OPENED_BY_URL,
        /*!
         * iOS only - application received Push Notification event
         */
        IOS_PUSH_NOTIFICATION,
    };

    /*!
     * @brief Constant for Push Notification attribute (iOS Only)
     * The flag indicates if the app is resumed by this push notification.
     */
    static const char8_t* kAttributeIosResumedByPushNotificationFlag;
    /*!
     * @brief Constant for App Opened by URL attribute (iOS Only)
     * The attribute value is the url string.
     */
    static const char8_t* kAttributeIosAppOpenedByUrl;

    /*!
     * @brief The event callback for application lifecycle event
     */
    using EventCallback = CallbackT<void(IApplicationLifecycleService*, Event)>;

    /*!
     * @brief Virtual base class for interface
     */
    virtual ~IApplicationLifecycleService() = default;

    /*!
     * @brief Add an listener to receive application lifecyle event
     *
     * @param listener The application lifecycle event listener
     */
    virtual void addEventCallback(EventCallback callback) = 0;

    /*!
     * @brief Get application lifecycle event attributes
     *
     * @param key The event attribute key
     * @return The event attribute value, empty string if key is not found
     */
    virtual String getEventAttribute(const char8_t* key) = 0;

#if defined(EA_PLATFORM_ANDROID)

    /*!
     * @brief Get JNI environment of current thread for JNI call
     *
     * @return The JNI environment
     */
    virtual JNIEnv* getJniEnvironment() = 0;

    /*!
     * @brief Get Android application context
     *
     * @return The Android application context
     */
    virtual jobject getAndroidApplicationContext() = 0;

    /*!
     * @brief Get current Android activity
     *
     * @return The Android activity
     */
    virtual jobject getAndroidActivity() = 0;

#endif

protected:
    IApplicationLifecycleService() = default;

    EA_NON_COPYABLE(IApplicationLifecycleService);
};

/*!
 * @brief The application lifecycle service access class
 *
 * It helps to get semi-singleton application lifecyel service instance on Hub, so other service can access easily.
 */
class EADPSDK_API ApplicationLifecycleService
{
public:
    /*!
     * @brief Get the application lifecycle service instance
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub provides the application lifecycle service.
     * @return The IApplicationLifecycleService instance
     */
    static IApplicationLifecycleService* getService(IHub* hub);

    EA_NON_COPYABLE(ApplicationLifecycleService);
};
}
}

