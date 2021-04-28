/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_QUALITYFACTOR_H
#define GAME_PACKER_QUALITYFACTOR_H

#include "gamepacker/qualitymetrics.h"
#include "gamepacker/property.h"


namespace Packer
{
struct PackerSilo;

const float INVALID_SHAPER_VALUE = -1.0f;

struct GameQualityTransform;
// PACKER_TODO: get rid of the factor spec, we just need the actual factor + transform?, or even collapse the whole hierarchy and have the factor implement the transform functions.
struct GameQualityFactorSpec
{
    GameQualityTransform* mFactorTransform = nullptr;
    eastl::string mTransformName;       // Ex. min/max/avg/stddev
    uint32_t mFactorResultSize = 0;
    bool mRequiresProperty = false;

    // Used by Blaze - Currently this just looks up by Transform. 
    static const GameQualityFactorSpec* getFactorSpecFromConfig(const eastl::string& propertyName, const eastl::string& transform);
};

typedef eastl::vector<GameQualityFactorSpec> GameQualityFactorSpecList;

struct GameQualityFactorConfig
{
    eastl::string mPropertyName;        // Ex.  "uedValues"
    eastl::string mOutputPropertyName;  // Ex.  "game.players.uedValues"
    eastl::string mTransform;           // ex.  "avg"

    // PACKER_TODO - Define a syntax to indicate which transform/aggregates apply when looking up groups of team/role/player properties.
    PropertyAggregate mTeamAggregate = PropertyAggregate::SUM;    // Formerly RANGE/RATIO - Indicates how multiple results should be aggregated (used by team rules, and minmax/max2 for some reason). 

    // This is just from the default Shaper, but that's what is used.  (Note that Packer doesn't have a concept of a Shaper directly)
    float mBestValue = 0;
    float mGoodEnoughValue = INVALID_SHAPER_VALUE;     // default to viable (INVALID_SCORE == default)
    float mViableValue = INVALID_SHAPER_VALUE;         // default to worst (most permissive)
    float mWorstValue = 0;
    float mGranularity = -1.0f;
};

struct PropertyDescriptor;
struct FieldDescriptor;

struct FieldScore
{
    const FieldDescriptor* mFieldDescriptor = nullptr;
    Score mScore = INVALID_SCORE;
    Score mScoreParticipation = MIN_SCORE; // fraction of players that supplied input values for this field descriptor
    uint32_t mEvalOrder = 0; // used to guarantee stable tied score ordering (without using stable_sort)
    const eastl::string& getFieldName() const;
};

typedef eastl::vector<FieldScore> FieldScoreList;

struct GameQualityFactor
{
    eastl::string mFactorName; // name + transform type
    eastl::string mOutputPropertyName;
    GameQualityFactorSpec mFactorSpec;
    PropertyAggregate mResultAggregate = PropertyAggregate::SUM;
    GameQualityFactorIndex mFactorIndex = INVALID_GAME_QUALITY_FACTOR_INDEX;
    PropertyDescriptor* mInputProperty = nullptr;       // specifies the property this factor uses to obtain input values

    //               good enough               viable
    //                   +                       +
    //                   |                       |
    //  +----------------+-----------------------+-------------------+
    //  |                                                            |
    //  +                                                            +
    // best                                                        worst
    
    float mBestValue = INVALID_SHAPER_VALUE;
    float mGoodEnoughValue = INVALID_SHAPER_VALUE;      // this value always between best and viable, inclusive
    float mViableValue = INVALID_SHAPER_VALUE;          // this value always between good enough and worst, inclusive
    float mWorstValue = INVALID_SHAPER_VALUE;
    float mGranularity = 0.0f;
    float mInverseResultValueRange = 0.0f;
    float mInverseGranularity = 0.0f;
    float mViableRawScore = 0.0f;                       // raw score value is unaffected by granularity setting
    float mGoodEnoughRawScore = 0.0f;                   // raw score value is unaffected by granularity setting
    eastl::string mValuesAsString;
    bool mScoreOnlyFactor = false;                      // factor does not participate in optimization choices

    GameQualityMetrics mInterimMetrics;
    GameQualityMetrics mFinalMetrics;
    mutable uint64_t mIdealScoreEarlyOutCount = 0; // write only metric (PACKER_TODO: change to non-mutable metric, once we merge Matt's changes and change passiveEval() to be non-const and allow metrics writes)

    GameQualityFactor() : mInterimMetrics{}, mFinalMetrics{} {}
    Score activeEval(EvalContext& cxt, Game& game);
    Score passiveEval(EvalContext& cxt, const Game& game) const;
    
    Score computeScore(const EvalResult& result) const;
    Score computeRawScore(const EvalResult& result) const;

    Score computeViableScore() const;       // Converts the mViableValue to a (0 - 1) Score. 
    bool isViableScore(Score rawScore) const { return (rawScore >= mViableRawScore); }
    const char8_t* getValuesAsString();

    Score aggregateResults(PropertyAggregate propAggregate, const EvalResult& result) const;
    Score computeScore(Score result) const;
    Score computeRawScore(Score result) const;
    bool computeSortedFieldScores(FieldScoreList& fieldScoreList, const Game& game, GameState state = GameState::PRESENT) const;
    bool hasValidPropertyRequirements() const;
    PropertyDescriptor* getInputPropertyDescriptor() const { return mInputProperty; }
    
};

typedef eastl::vector<GameQualityFactor> GameQualityFactorList;

}

#endif