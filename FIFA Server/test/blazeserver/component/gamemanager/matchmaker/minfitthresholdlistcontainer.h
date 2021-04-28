/*! ************************************************************************************************/
/*!
    \file   minfitthresholdlistcontainer.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_MIN_FIT_THRESHOLD_LIST_CONTAINER
#define BLAZE_MATCHMAKING_MIN_FIT_THRESHOLD_LIST_CONTAINER

#include "matchmakingutil.h"
#include "minfitthresholdlist.h"
#include "gamemanager/tdf/matchmaker_config_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RuleDefinition;

    /*! ************************************************************************************************/
    /*!
        \class MinFitThresholdListContainer
        \brief Container for the minFitThresholdLists.  Contains functions for parsing and accessing
            lists.

    *************************************************************************************************/
    class MinFitThresholdListContainer
    {
        NON_COPYABLE(MinFitThresholdListContainer);
    public:
        static const char8_t* MIN_FIT_THRESHOLD_LISTS_KEY;

        MinFitThresholdListContainer();
        virtual ~MinFitThresholdListContainer();

        /*! Sets the owning rule name for logging purposes */
        void setRuleDefinition(const RuleDefinition* ruleDefinition) { mRuleDefinition = ruleDefinition; }

        bool parseMinFitThresholdList(const FitThresholdsMap& minFitThresholdMap);

        void initMinFitThresholdListInfo(MinFitThresholdNameList &thresholdNameList) const;

        const MinFitThresholdList* getMinFitThresholdList(const char8_t* name) const;

        bool empty() const { return mMinFitThresholdLists.empty(); }

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

        void writeMinFitThresholdListsToString(eastl::string &dest, const char8_t* prefix) const;

    private:
        bool addMinFitThresholdList(MinFitThresholdList& newList);
        MinFitThresholdList* parseMinFitThresholdFromList(const char8_t* listName, const FitThresholdList& fitThresholdList);
        bool parseMinFitThresholdPair(const char8_t* pairString, MinFitThresholdList::MinFitThresholdPair &thresholdPair) const;
        bool parseMinFitThresholdValue(const char8_t* minFitThresholdStr, float &minFitThreshold) const;

        const char8_t* getRuleName() const;

    private:
        const char8_t* mCurrentThresholdListName; // helper storage for parsing min threshold lists

        ThresholdValueInfo mThresholdValueInfo;

        typedef eastl::map<const char8_t*, MinFitThresholdList *, CaseInsensitiveStringLessThan> MinFitThresholdLists;
        MinFitThresholdLists mMinFitThresholdLists;
        const RuleDefinition* mRuleDefinition;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_MIN_FIT_THRESHOLD_LIST_CONTAINER
