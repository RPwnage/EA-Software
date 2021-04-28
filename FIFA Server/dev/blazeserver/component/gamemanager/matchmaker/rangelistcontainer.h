/*! ************************************************************************************************/
/*!
    \file   rangelistcontainer.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_RANGE_LIST_CONTAINER
#define BLAZE_MATCHMAKING_RANGE_LIST_CONTAINER

#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/tdf/matchmaker_config_server.h"
#include "gamemanager/matchmaker/rangelist.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RuleDefinition;
    class RangeList;

    /*! ************************************************************************************************/
    /*!
        \class RangeListContainer
        \brief Container for the rangeLists.  Contains functions for parsing and accessing
            lists.

    *************************************************************************************************/
    class RangeListContainer
    {
        NON_COPYABLE(RangeListContainer);
    public:
        static const char8_t* RANGE_OFFSET_LISTS_KEY; //!< config map key for rule definition's rangeOffsetLists map("rangeOffsetLists")

        static const char8_t* INFINITY_RANGE_TOKEN;
        static const char8_t* EXACT_MATCH_REQUIRED_TOKEN;

        RangeListContainer();
        virtual ~RangeListContainer();

        /*! Sets the owning rule name for logging purposes */
        void setRuleName(const char8_t* ruleName) { mRuleName = ruleName; }

        void initRangeListInfo(MinFitThresholdNameList &rangeNameList) const;

        const RangeList* getRangeList(const char8_t* name) const;

        bool empty() const { return mRangeOffsetListMap.empty(); }

        /*! \brief return reference of ThresholdValueInfo object of the rule */
        const ThresholdValueInfo& getLiteralThresholdInfo() const { return mThresholdValueInfo; }

        /*! \brief retrieve supported threshold type of the rule, if more than one type is allowed those values will be ored together. */
        const uint8_t getThresholdAllowedValueType() const { return mThresholdValueInfo.getThresholdAllowedValueType(); }

        /*! \brief set supported threshold type of the rule */
        void setThresholdAllowedValueType(const uint8_t thresholdValueType) { mThresholdValueInfo.setThresholdAllowedValueType(thresholdValueType); }

        /*! ************************************************************************************************/
        /*! \brief retrieve fitscore based on passed in threshold literal

        \param[in] literalValue - literal value of the threshold
        \param[in] fitscore  - fitscore for the literal value
        \return true if the rule support the literal type minthreshold otherwise false
        ***************************************************************************************************/
        bool insertLiteralValueType(const char8_t* literalValue, const float fitscore)
        {
            EA_ASSERT_MSG((blaze_stricmp(literalValue, "EXACT_MATCH_REQUIRED") != 0), "literal is same as EXACT_MATCH_REQUIRED used for EXACT MATCH type, this is very wrong.");
            return mThresholdValueInfo.insertLiteralValueType(literalValue, fitscore);
        }

        void writeRangeListsToString(eastl::string &dest, const char8_t* prefix) const;

        bool initialize(const char8_t* ruleName, const GameManager::RangeOffsetLists& lists, bool allowOffsetPairs = true);

        static bool parseRange(const GameManager::OffsetList& offsetList, RangeList::Range& range);

    private:
        bool addRangeList(RangeList& newList);

        const char8_t* getRuleName() const;

        RangeList* createRangeList(const GameManager::RangeOffsetList& rangeOffsetList, bool allowOffsetPairs) const;
        static bool parseRangeToken(const char8_t* token, int64_t& rangeValue);

    private:
        const char8_t* mCurrentRangeListName; // helper storage for parsing min threshold lists

        ThresholdValueInfo mThresholdValueInfo;

        typedef eastl::map<const char8_t*, RangeList*, CaseInsensitiveStringLessThan> RangeOffsetListMap;
        RangeOffsetListMap mRangeOffsetListMap;

        const char8_t* mRuleName;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_MIN_FIT_THRESHOLD_LIST_CONTAINER
