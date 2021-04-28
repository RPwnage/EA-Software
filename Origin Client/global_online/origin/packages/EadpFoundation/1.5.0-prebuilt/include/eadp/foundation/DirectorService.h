// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Callback.h>

namespace eadp
{
namespace foundation
{

/*!
 * @brief Director service provides dynamic configuration management.
 *
 * The priority of combining configuration from high to low are
 * - Configuration overridden by setter
 * - Configuration data from director backend server
 * - Persisted configuration data (old retrieval from backend server)
 *
 * When the app launches first time, director service will try to retrieve the configuration from director service backend.
 * If persistence service is available, it will save the configuration into persistence.
 * For following launches, it will try to load configuration from persistence first, then refresh from director service
 * backend.
 * Manually specified configuration values will not be persisted. The integrator must specify them every launch.
 */
class EADPSDK_API IDirectorService
{
public:
    using ConfigurationUpdateCallback = CallbackT<void(IDirectorService*)>;

    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IDirectorService() = default;

    /*!
     * @brief Check if configuration is available
     *
     * This flag identify if configuration set is loaded from director service backend or persistent service
     *
     * @return bool True if there is configuration data available, otherwise false.
     */
    virtual bool isConfigurationAvailable() = 0;

    /*!
      * @brief Get the configuration by key
      *
      * @param key [in] Configuration key, cannot be nullptr
      * @return Configuration value, if key is not found, return empty String object
      */
    virtual String getConfigurationValue(const char8_t* key) = 0;

    /*!
      * @brief Get the configuration by key
      *
      * @param key [in] Configuration key, cannot be nullptr
      * @param defaultValue [in] Default configuration value returned if key is not found
      *
      * @return String Configuration value, if key is not found, return default value
      */
    virtual String getConfigurationValue(const char8_t* key, const char8_t* defaultValue) = 0;

    /*!
      * @brief Set configuration of a key with specific value
      *
      * This API will not affect persisted configuration, so it only in effect with current process.
      * This API will not trigger configuration update notification, so for internal service to pick
      * up the value, we suggest integrator should call this API right after Hub creation and before
      * Hub idle pump.
      *
      * @param key [in] Configuration key, cannot be nullptr
      * @param value [in] New configuration value, , cannot be nullptr
      */
    virtual void setConfigurationValue(const char8_t* key, const char8_t* value) = 0;

    /*!
     * @brief Reset the configuration overwritten by setConfiguration() API
     *
     * This API will only remove the overwritten configuration value from setConfiguration() API,
     * but will not remove configuration from backend or from persistence; instead it will make
     * the getConfiguration() API return the original value from backend or persistence.
     *
     * This API will not trigger configuration update notification, it is more for testing purpose.
     *
     * @param key [in] Configuration key, cannot be nullptr
     */
    virtual void resetConfigurationValue(const char8_t* key) = 0;
    
    /*!
     * @brief Force director service to update configurations from backend server
     */
    virtual void updateConfiguration() = 0;

    /*!
     * @brief Listen to configuration update
     *
     * This callback will be triggered after configuration data is updated from director backend.
     *
     * @param callback [in] The callback will be triggered when the configuration is updated
     */
    virtual void listenToConfigurationUpdate(const ConfigurationUpdateCallback& callback) = 0;

    /*!
     * @brief Format configuration into a JSON style string (internal)
     *
     * This API is designed to be used internally for demo purpose, the performance
     * is not gauged. Not recommend to be used in production environment.
     */
    virtual String dumpAllConfigurations() = 0;

protected:
    IDirectorService() = default;

    EA_NON_COPYABLE(IDirectorService);
};

/*!
 * @brief The director service access class
 *
 * It helps to get semi-singleton director service instance on Hub, so other service can access easily.
 */
class EADPSDK_API DirectorService
{
public:
    /*!
     * @brief Get the director service instance
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub provides the director service.
     * @return The IDirectorService instance
     */
    static IDirectorService* getService(IHub* hub);
};

}
}
