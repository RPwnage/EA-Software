// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

namespace eadp
{
namespace foundation
{

class IHub;

/*!
 * @brief Protocol for service
 *
 * Service is the basic unit of functionality provider in EADP SDK. It is always associated to the Hub.
 */
class EADPSDK_API IService
{
public:
    /*!
     * Virtual default destructor for base class
     */
    virtual ~IService() = default;

    /*!
     * Get the associated Hub
     *
     * @return The associated Hub
     */
    virtual IHub* getHub() const = 0;
};

}
}
