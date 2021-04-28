/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/algorithm.h"
#include "framework/redis/redis.h"
#include "framework/redis/rediscollectionmanager.h"

namespace Blaze
{

RedisCollectionManager::RedisCollectionManager()
    : mSchemaInfoMap(RedisId("framework", "blaze_schema"))
{
}

RedisCollectionManager::~RedisCollectionManager()
{
}

bool RedisCollectionManager::registerCollection(RedisCollection& collection)
{
    const RedisId& collectionId = collection.getCollectionId();
    RedisSchemaKeyPair keyPair = collectionId.getSchemaKeyPair();
    eastl::string full(collectionId.getFullName());
    int64_t result = 0;
    RedisError err = mSchemaInfoMap.insert("", keyPair, full, &result);
    bool ok = false;
    if (err == REDIS_ERR_OK)
    {
        if (result == 1)
        {
            ok = true; // successfully inserted new collection mapping
            collection.mIsFirstRegistration = true;
        }
        else if (result == 0)
        {
            eastl::string storedFullName;
            err = mSchemaInfoMap.find("", keyPair, storedFullName);
            if (err == REDIS_ERR_OK)
            {
                if (full == storedFullName)
                {
                    ok = true; // collection mapping is correct, proceed
                    collection.mIsFirstRegistration = false;
                }
                else
                {
                    BLAZE_ERR_LOG(Log::SYSTEM, "[RedisCollectionManager].registerCollection: "
                        "Failed to register collection: " << full.c_str() << ", another collection: "
                        << storedFullName.c_str() << " is already registered with the full name: " << storedFullName.c_str());
                }
            }
        }
    }

    if (ok)
    {
        RedisCollectionList::iterator it = eastl::find(mCollectionList.begin(), mCollectionList.end(), &collection);
        // make sure to check that the collection has not been added already, in case multiple calls to registerCollection() are made
        if (it == mCollectionList.end())
            mCollectionList.push_back(&collection);
    }
    return ok;
}

bool RedisCollectionManager::deregisterCollection(RedisCollection& collection)
{
    RedisCollectionList::iterator it = eastl::find(mCollectionList.begin(), mCollectionList.end(), &collection);
    if (it != mCollectionList.end())
    {
        mCollectionList.erase(it);
        return true;
    }
    return false;
}

} // Blaze
