// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Error.h>
#include <eadp/foundation/Callback.h>

namespace eadp
{
namespace foundation
{

class IPersistenceContainer;
class IPersistenceDelegator;

/*!
 * @brief Persistence service provides access to local cross-session persistent storage
 *
 * The service mainly serves EADP SDK internal need of data persistence.
 * The service will persist the data through IPersistenceDelegator interface, game team can either implement its
 * own delegator to take the full control the local storage access, or use the default delegator provided.
 * Also, this service is designed to handle small configuration data, updating a container with more than
 * 100K data will trigger oversize error.
 */
class EADPSDK_API IPersistenceService
{
public:
    /*!
     * @brief Callback for initialization update
     */
    using InitializationUpdateCallback = CallbackT<void(IPersistenceService*)>;
    /*!
     * @brief Callback for container existence check
     */
    using ContainerCheckCallback = CallbackT<void(bool existed, const ErrorPtr& error)>;
    /*!
     * @brief Callback for container request
     *
     * @note The callback will always give a valid container; if there is an error, an new empty container
     * will be passed, so user can fill the container and overwrite the corrupted data if needed.
     * But if the container data is not fully rewritten in callback everytime, checking error cautiously
     * before proceeding to avoid overwriting existed data mistakenly.
     */
    using ContainerCallback = CallbackT<void(IPersistenceContainer& container, const ErrorPtr& error)>;
    /*!
     * @brief Callback for other generic operation completion, like saving or deleting container
     */
    using CompletionCallback = CallbackT<void(const ErrorPtr& error)>;

    /*!
     * The maximum persistence container data size
     */
    const static int32_t kMaximumContainerDataSize = 0x19000; // 100K bytes

    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IPersistenceService() = default;

    /*!
     * @brief Initialize the persistence service
     *
     * @param delegator [in] The IPersistenceDelegator to manage the storage access.
     *
     * The integrator can provide custom IPersistenceDelegator implementation to have full access control of
     * persistence storage; or nullptr can be passed as delegator to indicate the default delegator is
     * expected to be used, which will grant the persistence service the free access the device storage as need.
     * The default delegator is created through PersistenceService::getDefaultDelegator() API.
     *
     * @see PersistenceService::getDefaultDelegator()
     */
    virtual void initialize(SharedPtr<IPersistenceDelegator> delegator) = 0;

    /*!
     * @brief Check if the persitence service is initialized.
     */
    virtual bool isInitialized() = 0;

    /*!
     * @brief Register a callback for persistence service initialization update.
     *
     * Once the initialize() is called and initialization is finished successfully, the callback will be triggered.
     */
    virtual void listenToInitializationUpdate(const InitializationUpdateCallback& callback) = 0;

    /*!
     * @brief Check if the encryption is supported
     *
     * After the persistence service is intialized, it could take some time to retrieving encryption information
     * from the backend; before the encryption info is ready, the encryption support will be disabled.
     */
    virtual bool isEncryptionSupported() = 0;

    /*!
      * @brief Check if a persistence container exists
      *
      * @param containerName [in] The container name to check
      * @param callback [in] The callback to return the boolean which indicates whether the container exists
      */
    virtual void checkContainerExistence(const char8_t* containerName, const ContainerCheckCallback& callback) = 0;

    /*!
      * @brief Request for a persistence container
      *
      * @param containerName [in] Persistence container name
      * @param encrypted [in] Whether container is encrypted
      * @param callback [in] The callback to return the container
      *
      * @return PersistenceContainer the container
      */
    virtual void requestContainer(const char8_t* containerName, bool encrypted, const ContainerCallback& callback) = 0;

protected:
    IPersistenceService() = default;

    EA_NON_COPYABLE(IPersistenceService);
};

class EADPSDK_API IPersistenceDelegator
{
public:
    /*!
     * @brief Callback for container saving completion
     */
    using SaveCompletionCallback = IPersistenceService::CompletionCallback;
    /*!
     * @brief Callback for container loading completion
     */
    using LoadCompletionCallback = CallbackT<void(const ErrorPtr& error, uint8_t* data, uint32_t dataSize)>;
    /*!
     * @brief Callback for container existence check
     */
    using CheckCompletionCallback = IPersistenceService::ContainerCheckCallback;

    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IPersistenceDelegator() = default;

    /*!
     * @brief Check if a data container with specific name exists or not
     *
     * @param containerName [in] The data container name to check
     *
     * @return bool True if the data container exists already, otherwise false.
     */
    virtual void checkDataContainerExistence(const char8_t* containerName, const CheckCompletionCallback& callback) = 0;

    /*!
     * @brief Save persistence data into persistence storage
     *
     * @param containerName [in] The container name
     * @param data [in] The persistence data to save
     * @param dataSize [in] The persistence data size
     * @param callback [in] The completion callback will be triggered when the saving process is finished
     */
    virtual void saveData(const char8_t* containerName,
                          const uint8_t* data,
                          uint32_t dataSize,
                          const SaveCompletionCallback& callback) = 0;

    /*!
     * @brief Load persistence data from persistence storage
     *
     * @param containerName [in] The container name
     * @param callback [in] The completion callback will be triggered when the loading process is finished
     */
    virtual void loadData(const char8_t* containerName, const LoadCompletionCallback& callback) = 0;

protected:
    IPersistenceDelegator() = default;

    EA_NON_COPYABLE(IPersistenceDelegator);
};

class EADPSDK_API IPersistenceContainer
{
public:
    /*!
     * @brief Callback for container updating (aka saving)
     */
    using UpdateCompletionCallback = IPersistenceService::CompletionCallback;

    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IPersistenceContainer() = default;

    /*!
     * @brief Check if persistence container is encrypted
     *
     * @return bool True if container is encrypted, otherwise false.
     */
    virtual bool isEncrypted() const = 0;

    /*!
     * @brief Set the flag whether persistence container should be encrypted
     *
     * @param encrypted [in] The encrypted flag, which will affect how the container data is persisted.
     */
    virtual void setEncrypted(bool encrypted) = 0;

    /*!
     * @brief Check if persistence container is empty or not.
     *
     * @return bool True if the container is empty and has key; otherwise false.
     */
    virtual bool empty() const = 0;

    /*!
     * @brief Check if persistence container contains specific key
     *
     * @param key [in] The persistence key, can be empty string but cannot be nullptr
     *
     * @return bool True if the key exists, otherwise false.
     */
    virtual bool hasKey(const char8_t* key) const = 0;

    /*!
     * @brief Remove key and its corresponding value from container
     *
     * @param key [in] The persistence key, can be empty string but cannot be nullptr
     */
    virtual void removeKey(const char8_t* key) = 0;

    /*!
     * @brief Get the corresponding value of specific key in the container
     *
     * @param key [in] The persistence key, can be empty string but cannot be nullptr
     *
     * @return The corresponding value of the key; empty string if key doesn't exist.
     */
    virtual String getValue(const char8_t* key) const = 0;

    /*!
     * @brief Get the corresponding value of specific key in the container
     *
     * @param key [in] The persistence key, can be empty string but cannot be nullptr
     * @param defaultValue [in] The default value if the key does not exist, can be empty string but cannot be nullptr
     *
     * @return The defaultValue if key does not exist, otherwise the corresponding value of the key.
     */
    virtual String getValue(const char8_t* key, const char8_t* defaultValue) const = 0;

    /*!
     * @brief Set the corresponding value of specific key to the container
     *
     * If the key exists, the value will be overwritten with the new value.
     *
     * @param key [in] The persistence key, can be empty string but cannot be nullptr
     * @param value [in] The persistence value, can be empty string but cannot be nullptr
     */
    virtual void setValue(const char8_t* key, const char8_t* value) = 0;
    
    /*!
     * @brief Set the corresponding string value of specific key to the container
     *
     * If the key exists, the value will be overwritten with the new value.
     * This overload can also be used for binary data, even the binary data contains 0x00 byte.
     *
     * @param key [in] The persistence key, can be empty string but cannot be nullptr
     * @param value [in] The persistence value, can be used for binary data.
     */
    virtual void setValue(const char8_t* key, String value) = 0;

    /*!
     * @brief Clear the persistence container
     *
     * Purge all existing key-value pairs, but still need to explicitly update() to synchronize the change
     * to persistence storage.
     */
    virtual void clear() = 0;

    /*!
     * @brief Update the persistence container change to persistence storage with a completion callback
     *
     * @param callback [in] Completion callback will be triggered when the update process is finished
     *
     * @return ErrorPtr The error if the data size is over-limit or the encryption fails.
     */
    virtual ErrorPtr update(const UpdateCompletionCallback& callback) = 0;

    /*!
     * @brief Update the persistence container change to persistence storage.
     *
     * This version can be used when the caller doesn't care about the completion of the asynchornous update.
     *
     * @return ErrorPtr The error if the data size is over-limit or the encryption fails.
     */
    virtual ErrorPtr update() = 0;


protected:
    /*!
     * @brief Virtual default destructor for base class
     */
    IPersistenceContainer() = default;

    EA_NON_COPYABLE(IPersistenceContainer);
};

/*!
 * @brief The persistence service access class
 *
 * It helps to get semi-singleton persistence service instance on Hub, so other component can access it easily.
 */
class EADPSDK_API PersistenceService
{
public:
    /*!
     * @brief Get the persistence service instance
     *
     * The service will be available when the Hub is live, but may requires to call setup() to make it ready to use.
     *
     * @param hub The hub provides the persistence service.
     *
     * @return The IPersistenceService instance
     */
    static IPersistenceService* getService(IHub* hub);

    /*!
     * @brief Get the default persistence service delegator
     *
     * The degault delegator will save persistence container data into individual local file in the user
     * application data folder, e.g. "C:\Users\<username>\Application Data\" on Windows.
     *
     * @param hub The hub provides the persistence service.
     *
     * @return The default delegator instance
     */
    static SharedPtr<IPersistenceDelegator> getDefaultDelegator(IHub* hub);
};

}
}
