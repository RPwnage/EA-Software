/*! ************************************************************************************************/
/*!
    \file diagnosticsgamecountutils.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKER_DIAGNOSTICS_GAME_COUNT_UTILS_H
#define BLAZE_MATCHMAKER_DIAGNOSTICS_GAME_COUNT_UTILS_H

#include "gamemanager/matchmaker/diagnostics/bucketpartitions.h"
#include "gamemanager/rpc/gamemanager_defines.h"
#include "gamemanager/rolescommoninfo.h"
#include "gamemanager/rete/reteruledefinition.h"
#include "gamemanager/tdf/rules.h" // for MAX_EXPLICIT_ATTRIBUTE_RULE_POSSIBLE_VALUE_COUNT

namespace Blaze
{
namespace Search
{
    class GameSessionSearchSlave;
    class SearchSlaveImpl;
}

namespace GameManager
{
namespace Matchmaker
{
    typedef eastl::map<uint64_t, int> AttributeValuePlayerCountMap;
    typedef eastl::bitset<MAX_EXPLICIT_ATTRIBUTE_RULE_POSSIBLE_VALUE_COUNT> GameAttributeRuleValueBitset;

    class RuleDefinitionCollection;
    class ModRuleDefinition;
    class RolesRuleDefinition;
    class TeamChoiceRuleDefinition;
    class PlayerSlotUtilizationRuleDefinition;
    class UEDRuleDefinition;
    class PlayerAttributeRuleDefinition;
    

    /*! ************************************************************************************************/
    /*! \brief gets counts of games based on bitset matches
        \param N The number of bits for the bitset
        \param isMatchOnGameBits If true, a match is if game has NO bits searcher doesn't
                have (e.g. Mod Rule). Conversely, if false, a match is if searcher has NO bits game doesn't
                have (e.g. required player attributes, roles)
    ***************************************************************************************************/
    template<size_t N, bool isMatchOnGameBits>
    class GameCountsByBitsetCache
    {

    public:

        static const uint64_t MAX_CACHED = N;

        typedef eastl::bitset<N, EASTL_BITSET_WORD_TYPE_DEFAULT> CacheBitset;
        
        static const CacheBitset BIT_1;
        static const CacheBitset ALL_BITS;

    public:

        GameCountsByBitsetCache() : mMatchOnGameBits(isMatchOnGameBits)
        {
        }

        void initialize(const char8_t* logContext)
        {
            mLogContext.sprintf("[GameCountsByBitsetCache:%s]", ((logContext != nullptr) ? logContext : ""));
        }

        bool isInitialized() const { return !mLogContext.empty(); }

        bool updateCount(CacheBitset gameBits, bool increment)
        {
            uint64_t& count = mCountsCache[gameBits];
            if (!increment && (count == 0))
            {
                ERR_LOG(mLogContext << ".updateCount: internal error: can't decrement count for game bits" <<
                    ") below 0. Diagnostics may not be updated correctly.");
                return false;
            }

            count += (increment ? 1 : -1);

            TRACE_LOG(mLogContext << ".updateCount: " << (increment ? "incremented" : "decremented") << " count for game to (" << count << ")");
            return true;
        }

        uint64_t getCount(CacheBitset bits) const
        {
            uint64_t count = 0;
            for (auto& gameCountsItr : mCountsCache)
            {
                const CacheBitset& matchOnBits = (mMatchOnGameBits ? gameCountsItr.first : bits);

                if ((gameCountsItr.first & bits) == matchOnBits)
                {
                    count += gameCountsItr.second;
                }
            }

            TRACE_LOG(mLogContext << ".getCount: got count for game bits (" << (bits.data() != nullptr ? *bits.data() : UINT64_MAX) << ") (" << count << ")");
            return count;
        }

    protected:
        eastl::string mLogContext;

    private:
        struct GameCountsBitsetEqualTo
        {
            bool operator()(const CacheBitset& a, const CacheBitset& b) const { return (a == b); }
        };

        struct GameCountsBitsetHash
        {
            uint64_t operator()(const CacheBitset& b) const { return EA::StdC::FNV64(b.data(), b.kWordCount * b.kWordSize); }
        };

        eastl::hash_map<CacheBitset, uint64_t, GameCountsBitsetHash, GameCountsBitsetEqualTo> mCountsCache;

        bool mMatchOnGameBits;

        mutable eastl::string mLogBuf, mLogBuf2;
    };

    template <uint64_t N, bool isMatchOnGameBits> const eastl::bitset<N, uint64_t>
        GameCountsByBitsetCache<N, isMatchOnGameBits>::BIT_1 =
        GameCountsByBitsetCache<N, isMatchOnGameBits>::CacheBitset(1);

    template <uint64_t N, bool isMatchOnGameBits> const eastl::bitset<N, uint64_t>
        GameCountsByBitsetCache<N, isMatchOnGameBits>::ALL_BITS =
        GameCountsByBitsetCache<N, isMatchOnGameBits>::CacheBitset().flip();


    /*! ************************************************************************************************/
    /*! \brief gets counts of games that match sets of values

        \param N The max number of values to count for
        \param T The type of the values to count for
        \param MapT For internal use, type to store Ts as, as keys in an internal to-bits map (eastl::string)
    ***************************************************************************************************/
    template<size_t N, class T, class MapT = T, bool isMatchOnGameBits = false>
    class GameCountsByValuesSetCache : public GameCountsByBitsetCache<N, isMatchOnGameBits>
    {
    public:
        typedef GameCountsByBitsetCache<N, isMatchOnGameBits> base_type;
        using base_type::updateCount;
        using base_type::mLogContext;
        typedef typename base_type::CacheBitset CacheBitset;

    public:

        typedef eastl::vector<T> ValuesVector;

        GameCountsByValuesSetCache() : GameCountsByBitsetCache<N, isMatchOnGameBits>(),
            mNextAvailableBitIndex(0)
        {
        }

        bool updateCount(const ValuesVector& gameValues, bool increment)
        {
            CacheBitset bitset;
            getMappedBitset(bitset, gameValues);
            return base_type::updateCount(bitset, increment);
        }

        uint64_t getCount(const ValuesVector& values) const
        {
            CacheBitset bitset;
            getMappedBitset(bitset, values);
            return base_type::getCount(bitset);
        }

    private:

        // map value to a bit
        uint64_t getMappedBitIndex(T value) const
        {
            typename ValueToBitMap::const_iterator itr = mValueToBitMap.find(value);
            if (itr != mValueToBitMap.end())
            {
                // found assigned bit
                return (*itr).second;
            }

            if (mNextAvailableBitIndex > (base_type::MAX_CACHED - 1))
            {
                //at max cache-able, additional new values get assigned to last bit
                WARN_LOG(mLogContext << ".getMappedBitIndex: warning: max values (" << base_type::MAX_CACHED << 
                    ") reached. New values will be lumped as single 'other values'. The game count diagnostic may be inaccurate.");

                return (base_type::MAX_CACHED - 1);
            }

            mValueToBitMap.insert(eastl::make_pair(MapT(value), mNextAvailableBitIndex));
            uint64_t returnBit = mNextAvailableBitIndex++;
            return returnBit;
        }

        // map vector of values to a bitset
        void getMappedBitset(CacheBitset& bitset, const ValuesVector& values) const
        {
            for (auto& str : values)
            {
                uint64_t mappedBit = getMappedBitIndex(str);
                bitset |= (base_type::BIT_1 << mappedBit);
            }
        }

    private:
        typedef eastl::hash_map<MapT, uint64_t> ValueToBitMap;
        mutable ValueToBitMap mValueToBitMap;
        mutable uint64_t mNextAvailableBitIndex;
    };


    /*! ************************************************************************************************/
    /*! \brief gets counts of games having specific values, using buckets. Good for large, fixed ranges
    ***************************************************************************************************/
    class GameCountsByBucketCache
    {
    public:
        GameCountsByBucketCache()
        {
        }
        bool initialize(const BucketPartitions& bucketPartitions, const char8_t* logContext);
        bool isInitialized() const { return !mCountsCache.empty(); }
        bool updateCount(int64_t gamePreBucketValue, bool increment);
        uint64_t getCount(int64_t minPreBucketValue, int64_t maxPreBucketValue) const;

    private:
        bool validateInitialized() const;

        BucketPartitions mBucketPartitions;
        eastl::vector<uint64_t> mCountsCache;
        eastl::string mLogContext;
    };



    /*! ************************************************************************************************/
    /*! \brief gets diagnostics game counts
    ***************************************************************************************************/
    class DiagnosticsSearchSlaveHelper
    {
    public:
        DiagnosticsSearchSlaveHelper(const Search::SearchSlaveImpl& owner) : mSearchSlaveImpl(owner), 
            mReteGamesCount(0), mTotalGamesCount(0)
        {
        }

        /*! ************************************************************************************************/
        /*! \brief get count of all findable games from search slave
        ***************************************************************************************************/
        uint64_t getOpenToMatchmakingCount() const { return mReteGamesCount; }
        uint64_t getTotalGamesCount() const { return mTotalGamesCount; }

        /*! ************************************************************************************************/
        /*! \brief gets counts of games from RETE
        ***************************************************************************************************/
        uint64_t getNumGamesVisibleForRuleConditions(const char8_t* ruleName, const Rete::ConditionBlockList& ruleConditions) const;


        // Non RETE game counting:

        /*! ************************************************************************************************/
        /*! \brief update cached diagnostic counts, on game updates
        ***************************************************************************************************/
        void updateGameCountsCaches(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment);

        // ModRule
        void updateModsGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const ModRuleDefinition& ruleDefinition);
        const uint64_t getModsGameCount(uint32_t mods) const;
        
        // RolesRule
        void updateRolesGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const RolesRuleDefinition& ruleDefinition);
        uint64_t getRolesGameCount(const RolesSizeMap& desiredRoles) const;

        // TeamChoiceRule
        void updateTeamIdsGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const TeamChoiceRuleDefinition& ruleDefinition);
        uint64_t getTeamIdsGameCount(const TeamIdSizeList& desiredTeams) const;
        
        // PlayerSlotUtilizationRule
        void updatePlayerSlotUtilizationGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const PlayerSlotUtilizationRuleDefinition& ruleDefinition);
        uint64_t getPlayerSlotUtilizationGameCount(int64_t minPreBucketValue, int64_t maxPreBucketValue) const;

        // UedRule
        void updateUedGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const UEDRuleDefinition& ruleDefinition);
        uint64_t getUEDGameCount(const char8_t* ruleName, int64_t minPreBucketValue, int64_t maxPreBucketValue) const;

        // PlayerAttributeRules
        void updatePlayerAttributesGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const PlayerAttributeRuleDefinition& ruleDefinition);
        uint64_t getPlayerAttributeGameCount(const char8_t* ruleName, uint64_t desiredAttribValueInt) const;


    private:
        const Search::SearchSlaveImpl& mSearchSlaveImpl;

        uint64_t mReteGamesCount;
        uint64_t mTotalGamesCount;

        typedef GameCountsByBitsetCache<sizeof(GameModRegister) * 8, true> ModsGameCountsCache;
        ModsGameCountsCache mModsGameCounts;

        // Specifies max number of the per rule values that can be (accurately) tracked in rule's diagnostics:
        static const size_t DIAGNOSTICS_MAX_CACHED_ROLES = 256;//ILTd

        typedef GameCountsByValuesSetCache<DIAGNOSTICS_MAX_CACHED_ROLES, const char8_t*, eastl::string> RolesGameCountsCache;
        RolesGameCountsCache mRolesGameCountsCache;

        static const size_t DIAGNOSTICS_MAX_CACHED_TEAMIDS = 1024;//ILTd

        static const TeamId DIAGNOSTICS_DUPLICATE_TEAMIDS_CACHE_KEY;
        typedef GameCountsByValuesSetCache<DIAGNOSTICS_MAX_CACHED_TEAMIDS, TeamId> TeamIdsGameCountsCache;
        TeamIdsGameCountsCache mTeamIdsGameCounts;

        GameCountsByBucketCache mPlayerSlotUtilizationGameCounts;

        typedef eastl::hash_map<eastl::string, GameCountsByBucketCache> UEDGameCountsCacheMap;
        UEDGameCountsCacheMap mUEDGameCounts;
        GameCountsByBucketCache& getUEDGameCountsCache(const UEDRuleDefinition& ruleDefinition);

        typedef eastl::hash_map<Rete::WMEAttribute, uint64_t> PAttributeGameCountCache;
        typedef eastl::hash_map<eastl::string, PAttributeGameCountCache> PAttributeGameCountCacheMap;

        mutable PAttributeGameCountCacheMap mPAttributeGameCounts;
        
        PAttributeGameCountCache& getPAttributeGameCountCache(const char8_t* ruleName) const;

		DiagnosticsSearchSlaveHelper& operator=(const DiagnosticsSearchSlaveHelper&) {return *this;};
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKER_DIAGNOSTICS_GAME_COUNT_UTILS_H
