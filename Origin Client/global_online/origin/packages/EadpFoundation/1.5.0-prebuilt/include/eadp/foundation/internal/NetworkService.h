// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/internal/HttpClient.h>
#include <eadp/foundation/internal/HttpRequest.h>
#include <eadp/foundation/internal/HttpResponse.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class IRpcNetworkHandler;

/**
* @brief Interface for Network service
*
* Note: this service itself is not thread-safe, and should only be accessed through Job
*/
class EADPSDK_API INetworkService
{
public:
    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~INetworkService() = default;

    /**
     * @brief Retrieves a network handler for gRPC services
     *
     * @param baseUrl The base url for the service endpoint
     * @return a shared pointer to the RPC network handler.
     */
    virtual SharedPtr<IRpcNetworkHandler> getRpcNetworkHandler(const char8_t* baseUrl) = 0;

    /**
     * @brief Retrieves a network handler for gRPC services with an director configuration key
     *
     * @param configurationKey The configuration key for base url from director service.
     * @return a shared pointer to the RPC network handler.
     */
    virtual SharedPtr<IRpcNetworkHandler> getRpcNetworkHandlerWithConfiguration(const char8_t* configurationKey) = 0;

    /**
     * @brief Performs idle updates on the network handlers
     *
     * @return none
     */
    virtual void idle() = 0;

    /*!
     * @brief Gets a client to process an Http Request
     *
     * @param request An Http Request to process.
     * @param callback The callback to deliver response information and errors.
     *
     * @return A client object used to update and cancel the request.
     */
    virtual SharedPtr<IHttpClient> getHttpClient(const SharedPtr<HttpRequest>& request, const HttpResponse::Callback& callback) = 0;

protected:
    INetworkService() = default;

    EA_NON_COPYABLE(INetworkService);
};

class EADPSDK_API NetworkService
{
public:
    /**
    * @brief Get the service instance
    *
    * @return INetworkService* The pointer to network service instance
    */
    static INetworkService* getService(IHub* hub);
};

}
}
}

