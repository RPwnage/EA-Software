// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Callback.h>
#include <eadp/foundation/Job.h>
#include <eadp/foundation/Response.h>
#include <eadp/foundation/internal/ServiceManager.h>

#if defined(EA_PLATFORM_ANDROID)
#include <jni.h>
#endif

namespace eadp
{
namespace foundation
{

/*!
 * @brief Configuration for hub
 */
struct EADPSDK_API HubConfig
{
#if !EADPSDK_USE_STD_STL
    /*!
     * @brief The allocator will be used by EADP SDK
     *
     * If not set, the default allocator of EA Core Allocator package will be used.
     */
    EA::Allocator::ICoreAllocator* allocator;
#endif

    /*!
     * @brief the EA Client ID
     *
     * The clientId property is a game-specific identifier, and it is required for EADP-SDK to work correctly.
     * The EADP SDK uses it to retrieve application configuration from the Director service.
     * If the hub detects that the DirtySDK is not already initialized, it will also initialize the DirtySDK using
     * this clientId.
     */
    const char8_t* clientId;

    /*!
     * @brief Default constructor
     *
     * This constructor will put all the argument into default value.
     */
    HubConfig() :
#if !EADPSDK_USE_STD_STL
        allocator(nullptr),
#endif
        clientId(nullptr)
    {
    }

};

// forward declarations
class IJob;
class ILoggingService;
class IIdentityService;
class IPersistenceService;
class IDirectorService;
class IApplicationLifecycleService;
namespace Internal
{
class INetworkService;
class IEnvironmentService;
}

/*!
 * @brief Hub is the core class of the EADP SDK
 *
 * Hub is the core of the EADP SDK. and all services are associated to it. Application should
 * always create a Hub to start the SDK, and once Hub is destructed, all services provided by the SDK
 * will be destructed as well.
 * Game will need to drive Hub.idle() function to keep SDK running, and drive Hub.invokeCallbacks()
 * to notify asynchronous operation result to caller.
 * Hub is thread-safe, and you can have multiple Hubs in some cases, e.g. for testing or for hub extension.
 */
class EADPSDK_API IHub
{
public:
    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IHub() = default;
    
    /*!
     * @brief Get the hub instance as shared pointer
     *
     * As IHub instance can only be created from Hub::create(), this is always safe.
     *
     * @return The IHub shared pointer
     */
    virtual SharedPtr<IHub> getSharedInstance() = 0;

    /*!
     * @brief The main pump of the SDK execution.
     *
     * The user is expected to call idle() at least once per frame to drive the SDK.
     * This call is thread-safe and can be called from any number of threads.
     */
    virtual void idle() = 0;

    /*!
     * @brief Invoke all callbacks
     *
     * This will invoke all callbacks, no matter what group id the callback is assigned to.
     */
    virtual void invokeCallbacks() = 0;

    /*!
     * @brief Invoke callbacks associated to specific group id from the callback queue.
     *
     * @param groupId The callbacks associate with this group id will be invoked.
     */
    virtual void invokeCallbacks(Callback::GroupId groupId) = 0;

    /*!
     * @brief Add job into the job queue.
     *
     * @param job The job get added into the job queue. Using eastl::move() as the unique pointer ownership
     / will transfer.
     */
    virtual void addJob(UniquePtr<IJob> job) = 0;

    /*!
     * @brief Add response object, which wraps the callback, into the callback queue.
     *
     * @param response The RpcResponse object get added into the callback queue.
     */
    virtual void addResponse(UniquePtr<IResponse> response) = 0;

    /*!
     * @brief Get the hub configuration.
     *
     * @return The HubConfig for the hub.
     */
    virtual const HubConfig& getConfig() = 0;

    /*!
     * @brief Get the allocator.
     *
     * @return The allocator, which is used for all the SDK memory allocation.
     */
    virtual Allocator& getAllocator() = 0;

    /*!
     * @brief Get the logging service
     *
     * @return The ILoggingService instance.
     */
    virtual ILoggingService* getLoggingService() = 0;

    /*!
     * @brief Get the director service
     *
     * @return The IDirectorService instance.
     */
    virtual IDirectorService* getDirectorService() = 0;

    /*!
     * @brief Get the application lifecycle service
     *
     * @return The IApplicationLifecycleService instance.
     */
    virtual IApplicationLifecycleService* getApplicationLifecycleService() = 0;

    /*!
     * @brief Get the identity service
     *
     * @return The IIdentityService instance.
     */
    virtual IIdentityService* getIdentityService() = 0;

    /*!
     * @brief Get the persistence service
     *
     * @return The IPersistenceService instance.
     */
    virtual IPersistenceService* getPersistenceService() = 0;

    /*!
     * @brief Get the network service
     *
     * @return The INetworkService instance.
     */
    virtual ::eadp::foundation::Internal::INetworkService* getNetworkService() = 0;

    /*!
     * @brief Get the environment service
     *
     * @return The IEnvironmentService instance.
     */
    virtual ::eadp::foundation::Internal::IEnvironmentService* getEnvironmentService() = 0;
    
    /*!
     * @brief Get the internal service manager
     *
     * @return The ServiceManager instance
     */
    virtual ::eadp::foundation::Internal::ServiceManager& getServiceManager() = 0;
};

using HubPtr = SharedPtr<IHub>;

/*!
 * @brief Factory class for creating a Hub instance.
 *
 * It helps to create the hub instance with given HubConfig.
 */
class EADPSDK_API Hub
{
public:
    /*!
     * @brief Create hub instance
     *
     * @param config The configuration controls some behavior of Hub.
     * @return The Hub instance
     */
    static HubPtr create(const HubConfig& config);
};

}
}

