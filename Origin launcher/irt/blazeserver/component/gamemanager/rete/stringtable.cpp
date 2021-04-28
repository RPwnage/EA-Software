/*! ************************************************************************************************/
/*!
\file ~vs8C42.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "stringtable.h"
#include "gamemanager/rpc/gamemanagerslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    const uint64_t StringTable::STRING_TABLE_SIZE = 64000; // Needs to be large initial size to avoid collisions.
    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    StringTable::StringTable(): 
        mStringHashMap(STRING_TABLE_SIZE, eastl::hash<uint64_t>(), eastl::equal_to<uint64_t>(), 
            BlazeStlAllocator("mStringHashMap", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
        mZombieCountGauge = 0;
        mCollisionCountTotal = 0;

        RefcountedString* wmeAnyHashString = BLAZE_NEW RefcountedString("WME_ANY_HASH", false);
        mStringHashMap[WME_ANY_HASH] = wmeAnyHashString;
        const char8_t* anyStr = getStringFromHash(WME_ANY_HASH);
        TRACE_LOG("[StringTable].ctor Adding '" << anyStr << "' as " << WME_ANY_HASH << ".");

        RefcountedString* wmeUnsetHashString = BLAZE_NEW RefcountedString("WME_UNSET_HASH", false);
        mStringHashMap[WME_UNSET_HASH] = wmeUnsetHashString;
        const char8_t* unsetStr = getStringFromHash(WME_UNSET_HASH);
        TRACE_LOG("[StringTable].ctor Adding '" << unsetStr << "' as " << WME_UNSET_HASH << ".");
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    StringTable::~StringTable()
    {
        // delete all of the strings in the string table.
        StringHashMap::iterator i = mStringHashMap.begin();

        while (i != mStringHashMap.end())
        {
            // Zombie values do not have memory associated with them, so just erase them here.
            if (isRemovedZombieHash(i))
            {
                i = mStringHashMap.erase(i);
                --mZombieCountGauge;
                continue;
            }
            
            else
            {
                delete(i->second);
            }
            ++i;
        }
        mStringHashMap.clear();
    }

    const char8_t* StringTable::getStringFromHash(uint64_t hash) const
    {
        StringHashMap::const_iterator itr = mStringHashMap.find(hash);
        if ((itr != mStringHashMap.end()) && (itr->second != nullptr))
        {
            return itr->second->mString;
        }

        return "NULL";
    }

    uint64_t StringTable::reteHash(const char8_t* str, bool isGarbageCollectable /*= false*/)
    {
        EA_ASSERT(str != nullptr);
        eastl::hash<const char8_t*> eastlHash;                          // After validating collision issue is fixed, change to use FNV64_String8 instead of eastl::hash
        uint64_t hash = static_cast<uint64_t>(eastlHash(str));

        bool collisionOccurred = false;
        StringHashMap::iterator firstZombieItr = mStringHashMap.end();
        while (true)
        {
            StringHashMap::iterator itr = mStringHashMap.find(hash);

            // Compare strings if we found an item.
            // If we have a collision, rehash.
            if (itr != mStringHashMap.end())
            {
                const RefcountedString* str2 = itr->second;
                if (! isRemovedZombieHash(itr) && blaze_strcmp(str, str2->mString) == 0)
                {
                    // Found the correct hash
                    return hash;
                }
                else
                {
                    if (isRemovedZombieHash(itr) && firstZombieItr == mStringHashMap.end())
                    {
                        firstZombieItr = itr;
                    }

                    // Collision, increment and try again.
                    hash = (hash == UINT64_MAX) ? 0 : hash + 1;
                    collisionOccurred = true;
                }

            }
            else
            {
                if (collisionOccurred)
                    ++mCollisionCountTotal;

                // New String so add it to the table.
                if (firstZombieItr != mStringHashMap.end())
                {
                    --mZombieCountGauge;
                    firstZombieItr->second = BLAZE_NEW RefcountedString(str, isGarbageCollectable);

                    return firstZombieItr->first;
                }

                mStringHashMap[hash] = BLAZE_NEW RefcountedString(str, isGarbageCollectable);
                return hash;
            }
        }

    }

    /*! ************************************************************************************************/
    /*! \brief gets string's hash key already inserted. Return whether found
    ***************************************************************************************************/
    bool StringTable::findHash(const char8_t* str, uint64_t& hash) const
    {
        EA_ASSERT(str != nullptr);
        eastl::hash<const char8_t*> eastlHash;
        hash = static_cast<uint64_t>(eastlHash(str));  // After validating collision issue is fixed, change to use FNV64_String8 instead of eastl::hash

        StringHashMap::const_iterator itr = mStringHashMap.find(hash);

        // Compare strings if we found an item.
        while (itr != mStringHashMap.end())
        {
            const RefcountedString* str2 = itr->second;
            if (!isRemovedZombieHash(itr) && blaze_strcmp(str, str2->mString) == 0)
            {
                // Found the correct hash
                return true;
            }
            else
            {
                // Collision, increment and try again.
                hash = (hash == UINT64_MAX) ? 0 : hash + 1;
            }
            itr = mStringHashMap.find(hash);   
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief clear hash key and delete its copy of the string value. Return whether found.
        If subsequent-valued key exists, just 'zombify' this key for now (i.e. to delay full delete to maintain collision resolution chain here)
        Removes hashed strings regardless of current refcount or garbage collectable flag.

        Note: client code responsible for invalidating external copies of key.
        Note: performance depends on collisions rate
    ***************************************************************************************************/
    bool StringTable::removeHash(uint64_t hash)
    {
        StringHashMap::iterator itr = mStringHashMap.find(hash);
        if (itr != mStringHashMap.end())
        {
            // first set to null zombie
            delete(itr->second);
            itr->second = nullptr;
            ++mZombieCountGauge;

            // check whether had collision chain here i.e. if next hash occupied
            // If has collisions, don't delete now, until also removed the subsequent (chain ending with)
            // the non-removed collided key (so that key can still be found while it lives))
            uint64_t nextHash = ((hash == UINT64_MAX)? 0 : hash + 1);
            if (mStringHashMap.find(nextHash) == mStringHashMap.end())
            {
                // no collisions at this key, can fully delete it.
                // Can also free preceding zombie keys, since now unblocked
                uint64_t priorHash = hash;
                do 
                {
                    mStringHashMap.erase(itr);
                    --mZombieCountGauge;
                    if (priorHash == 0)
                        break;

                    priorHash--;
                    itr = mStringHashMap.find(priorHash);
                } while ((itr != mStringHashMap.end()) && isRemovedZombieHash(itr));
            }
            return true;
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief decrements the refcount for a hash- if the refcount reaches 0 and the hash is garbage collectible,
        removeHash() is called.

        Note: client code responsible for invalidating external copies of key.
        Note: performance depends on collisions rate
        returns false if the hash wasn't found
    ***************************************************************************************************/
    bool StringTable::dereferenceHash(uint64_t hash)
    {
        StringHashMap::iterator itr = mStringHashMap.find(hash);
        if ((itr != mStringHashMap.end()) && (itr->second != nullptr))
        {   
            RefcountedString* hashedString = itr->second;

            if (hashedString->mRefcount > 0)
            {
                hashedString->mRefcount--;
            }

            if (hashedString->mGarbageCollectable && (hashedString->mRefcount == 0))
            {
                removeHash(hash);
            }
            return true;
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief increments the refcount for a hash

        Note: client code responsible for invalidating external copies of key.
        Note: performance depends on collisions rate
        returns false if the hash wasn't found
    ***************************************************************************************************/
    bool StringTable::referenceHash(uint64_t hash)
    {
        StringHashMap::iterator itr = mStringHashMap.find(hash);
        if ((itr != mStringHashMap.end()) && (itr->second != nullptr))
        {
            RefcountedString* hashedString = itr->second;

            hashedString->mRefcount++;

            return true;
        }
        return false;
    }
} // namespace Rete
} // namespace GameManager
} // namespace Blaze
