/*! ************************************************************************************************/
/*!
\file stringtable.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_STRINGTABLE_H
#define BLAZE_GAMEMANAGER_STRINGTABLE_H

#include "gamemanager/rete/retetypes.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{

    class StringTable
    {
        NON_COPYABLE(StringTable);
    public:
        static const uint64_t STRING_TABLE_SIZE;

        StringTable();
        virtual ~StringTable();

        // guarantees that we avoid string collisions
        // str must be null terminated.
        uint64_t reteHash(const char8_t* str, bool isGarbageCollectable = false);
        const char8_t* getStringFromHash(uint64_t hash) const;

        bool findHash(const char8_t* str, uint64_t& hash) const;
        bool dereferenceHash(uint64_t hash);
        bool referenceHash(uint64_t hash);
        bool removeHash(uint64_t hash);

        uint64_t getStringHashMapSize() const { return (uint64_t)mStringHashMap.size(); }
        uint64_t getZombieCountGauge() const { return mZombieCountGauge; }
        uint64_t getCollisionCountTotal() const { return mCollisionCountTotal; }

    // Members
    private:
        struct RefcountedString
        {
            RefcountedString(const char8_t* string, bool isGarbageCollectable)
                : mRefcount(0), mGarbageCollectable(isGarbageCollectable)
            {
                mString = blaze_strdup(string);
            }

            ~RefcountedString()
            {
                BLAZE_FREE(mString);
            }



            const char8_t* mString;
            uint32_t mRefcount;
            // some hash values are cached outside of the StringTable, so we're only going to GC ones we know are safe in advance;
            bool mGarbageCollectable;
        };

        typedef eastl::hash_map<uint64_t, RefcountedString*> StringHashMap;

        uint64_t mZombieCountGauge; 
        uint64_t mCollisionCountTotal;

        StringHashMap mStringHashMap;

        bool isRemovedZombieHash(const StringHashMap::const_iterator& itr) const { return (itr->second == nullptr); }
    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_STRINGTABLE_H
