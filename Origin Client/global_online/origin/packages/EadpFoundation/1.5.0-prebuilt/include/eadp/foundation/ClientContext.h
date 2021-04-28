// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Hub.h>

namespace eadp
{
namespace foundation
{

/*!
 * @brief ClientContext helps to expose options to customize the gRPC request
 *
 */
class EADPSDK_API ClientContext
{
public:
    using MetaDataMap = MultiMap<String, String>;

    /*!
     * @brief Constructor with service
     */
    explicit ClientContext(SharedPtr<IService> service);

    /*!
     * @brief Constructor with hub
     */
    explicit ClientContext(SharedPtr<IHub> hub);
    
    /*!
     * @brief Destructor
     */
    ~ClientContext() = default;

    /*!
     * @brief Set meta data with key-value pair
     *
     * @param key The key for the meta data. Cannot be nullptr nor empty string
     *  Add multiple meta-data with the same key is OK.
     * @param value The meta data value. Cannot be nullptr nor empty string.
     */
    void setMetaData(const char8_t* key, const char8_t* value);
    
    /*!
     * @brief Replace meta data with a new meta data map
     *
     * @param metaData The new meta data map
     */
    void setMetaDataMap(const MetaDataMap& metaData);

    /*!
     * @brief Clear meta data with specific key
     *
     * @param key The key for the meta data. Cannot be nullptr nor empty string
     *  If multiple meta-data with the same key, all of those entries will be cleared.
     */
    void clearMetaData(const char8_t* key);
    
    /*!
     * @brief Get meta data map
     *
     * @param metaData The const reference to the meta data map
     */
    const MetaDataMap& getMetaDataMap();

    /*!
     * @brief Set the user index
     *
     * @param userIndex The user index indicates the identity slot for multiple users authentication.
     *  Default slot is IIdentityService::kDefaultUserIndex.
     */
    void setUserIndex(uint32_t userIndex);
    
    /*!
     * @brief Get the user index
     *
     * @return The user index of multiple users authentication.
     */
    uint32_t getUserIndex();

private:
    WeakPtr<IHub> m_weakHub; // we have to handle this as weak_ptr to avoid loop reference
    MetaDataMap m_metaData;
    uint32_t m_userIndex;
};

}
}
