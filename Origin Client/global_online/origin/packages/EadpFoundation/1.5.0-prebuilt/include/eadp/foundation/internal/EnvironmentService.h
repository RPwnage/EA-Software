// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Callback.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

/*!
 * @brief Environment service provides platform and configuration information on cross-platform APIs
 */
class EADPSDK_API IEnvironmentService
{
public:
    /*!
     * @brief Environment identifier constants
     */
    static const char8_t* kEnvironmentUnknown;          // UNKNOWN
    static const char8_t* kEnvironmentDevelopment;      // DEV
    static const char8_t* kEnvironmentTest;             // TEST
    static const char8_t* kEnvironmentCertification;    // CERT
    static const char8_t* kEnvironmentProduction;       // PROD

    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IEnvironmentService() = default;

    /*!
     * @brief Get name of application
     */
    virtual String getApplicationName() = 0;

    /*!
     * @brief Get version of application
     */
    virtual String getApplicationVersion() = 0;

    /*!
    * @brief Get application identifier
    *
    * The iOS App Bundle Id, the Android package id
    */
    virtual String getApplicationIdentifier() = 0;

    /*!
     * @brief Get environment identifier
     *
     * It decides which director service endpoint we use and accordingly
     * affects all the backend service endpoints and related configurations.
     *
     * @return The environment identifier, like
     *  DEV, CERT, TEST, PROD
     * or UNKNOWN if backend environment cannot be determined.
     */
    virtual String getEnvironmentIdentifier() = 0;

    /*!
    * @brief Get name of device platform
    *
    * Windows, Xbox, PlayStation, iOS, Android
    */
    virtual String getDevicePlatform() = 0;

    /*!
     * @brief Get the device identifier for vendor
     */
    virtual String getDeviceIdentifierForVendor() = 0;

    /*!
     * @brief Get the device identifier for advertiser
     */
    virtual String getDeviceIdentifierForAdvertiser() = 0;

    /*!
     * @brief Get release type of application
     */
    virtual String getReleaseType() = 0;

    /*!
     * @brief Get the specific configuration from configuration file
     */
    virtual String getValueFromConfigurationFile(const char8_t* key) = 0;

    /*!
     * @brief Get the application bundle folder path
     * Used for persistence on mobile
     */
    virtual String getApplicationBundleFolder() = 0;

    /*!
     * @brief Get the application document folder path
     * Used for persistence on mobile
     */
    virtual String getApplicationDocumentFolder() = 0;

    /*!
     * @brief Get the device OS version number.
     * String represents number of currently running OS version.
     */
    virtual String getOperatingSystemVersion() = 0;

    /*!
     * @brief Get the device manufacturer.
     * Returns string representing mobile device manufacturer. Apple for iOS devices and company name for Android.
     */
    virtual String getDeviceManufacturer() = 0;

    /*!
     * @brief Get the device model.
     * Specifies mobile device model returned by system ("Pixel" "iPhone X" etc.).
     */
    virtual String getDeviceModel() = 0;

    /*!
     * @brief Get the device current locale setting.
     * Returning current locale for mobile device in format "en_US".
     * First two letters represents the language code specified by ISO 639.
     * Second two represents the country code specified by ISO 3166.
     */
    virtual String getDeviceLocale() = 0;

    /*!
     * @brief Get the device time zone setting.
     * Returning string which represents standard abbreviation for current mobile device timezone (GMT, PDT etc).
     */
    virtual String getDeviceTimeZone() = 0;

protected:
    IEnvironmentService() = default;

    EA_NON_COPYABLE(IEnvironmentService);
};

/*!
 * @brief The environment service access class
 *
 * It helps to get semi-singleton environment service instance on Hub
 */
class EADPSDK_API EnvironmentService
{
public:
    /*!
     * @brief Get the environment service instance
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub provides the environment service.
     * @return The IEnvironmentService instance
     */
    static IEnvironmentService* getService(IHub* hub);
};

}
}
}
