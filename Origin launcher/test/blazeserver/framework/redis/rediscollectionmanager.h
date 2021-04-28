/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_COLLECTIONMANAGER_H
#define BLAZE_REDIS_COLLECTIONMANAGER_H

/*** Include files *******************************************************************************/

#include "framework/redis/redisid.h"
#include "framework/redis/rediskeyfieldmap.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

class RedisCluster;

class RedisCollectionManager
{
    NON_COPYABLE(RedisCollectionManager);
public:
    bool isFirstSchemaRegistration() const { return mSchemaInfoMap.isFirstRegistration(); }

private:
    friend class RedisCluster;
    friend class RedisCollection;
    typedef eastl::vector<RedisCollection*> RedisCollectionList;
    typedef RedisKeyFieldMap<eastl::string, RedisSchemaKeyPair, eastl::string> RedisSchemaInfoMap;

    RedisCollectionManager();
    ~RedisCollectionManager();
    bool registerCollection(RedisCollection& collection);
    bool deregisterCollection(RedisCollection& collection);

    RedisCollectionList mCollectionList;
    RedisSchemaInfoMap mSchemaInfoMap;
};


} // Blaze

#endif // BLAZE_REDIS_COLLECTIONMANAGER_H

