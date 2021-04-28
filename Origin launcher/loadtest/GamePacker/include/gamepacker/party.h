/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_PARTY_H
#define GAME_PACKER_PARTY_H

#include <EASTL/map.h>
#include <EASTL/vector_map.h>
#include "gamepacker/property.h"
#include "gamepacker/player.h"

namespace Packer
{

struct PartyPlayer
{
    PlayerId mPlayerId = 0;
    PartyPlayerIndex mPartyPlayerIndex = 0;
};


typedef eastl::multimap<Time, struct Party*> PartyTimeQueue;

struct PackerSilo; // TODO: get rid of this!

struct Party
{
    typedef eastl::vector_map<GameId, int64_t> GameIdRefMap;
    typedef eastl::vector<PartyPlayer> PartyPlayerList;

    PartyId mPartyId = INVALID_PARTY_ID; //(read/only)
    GameId mGameId = INVALID_GAME_ID; // which game party _has_ been assigned to (read/write)
    Time mExpiryTime = 0; //(read/only) TODO: get rid of this, this value is stored in our queue
    bool mImmutable = false; // determines whether party can be evicted from the game or reassigned to a different team
    TeamIndex mFaction = 0; // which team party _shall_ be assigned to (read/only)
    uint64_t mEvictionCount = 0; //(write/only) tracks the number of times the party has been evicted from a game
    uint64_t mReinsertionCount = 0; //(write/only) tracks the number of times the party has been removed/readded to the packer
    uint64_t mRequeuedCount = 0; //(write/only) tracks the number of times the party has been requeued for processing due to being removed from current game due to unpacking of the game OR expiration of an in-game sibling
    uint64_t mUnpackedCount = 0; //(write/only) tracks the number of times the party has been unpacked for processing due to being removed from current game due to unpacking of the game
    Time mCreatedTimestamp = 0; //(write/only) tracks when the party entered the packer
    uint64_t mCreationSequence = 0; //(write/only) tracks when the party entered the packer
    Time mAddedToGameTimestamp = 0; //(write/only) tracks the last time the party was added to a game
    Time mTotalTimeInViableGames = 0; //This value is not reflected in the metrics in real time. The metrics will report this value, once the party has left the packer either due to retirement or being part of a viable game that is reaped.
    PartyId mLastEvictorPartyId = INVALID_PARTY_ID; //(write/only) used to detect when parties are causing mutually repeating evictions which may be a sign of a stall (or a bug in the GQF transform)
    uint64_t mLastEvictorEvictionCount = 0; //(write/only)
    GameIdRefMap mOtherGameIdRefs; // tracks ids of _other_ games that also contain members of this party(by virtue of being members of other parties)
    PartyTimeQueue::iterator mPartyExpiryQueueItr;
    PartyTimeQueue::iterator mPartyPriorityQueueItr;
    PartyPlayerList mPartyPlayers;
    FieldValueTable mFieldValueTable; // player field values are stored here

    Party();
    ~Party();
    int32_t getPlayerCount() const { return (int32_t)mPartyPlayers.size(); }
    void setPlayerPropertyValue(const FieldDescriptor* fieldDescriptor, PlayerIndex playerIndex, const PropertyValue* value);
    PropertyValue getPlayerPropertyValue(const FieldDescriptor* fieldDescriptor, PlayerIndex playerIndex, bool* hasValue = nullptr) const;
    bool hasPlayerPropertyField(const FieldDescriptor* fieldDescriptor) const;
};

typedef eastl::hash_map<PartyId, Party> PartyMap;

}
#endif
