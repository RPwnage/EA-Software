#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/qualityfactor.h"
#include "gamepacker/handlers.h"       // For rand() function wrapper.
#include "gamepacker/packer.h"

namespace Packer
{

static const char8_t* INSUFFICIENT_PARTICIPATION_REASON_TEXT = "No game property fields have sufficient party participation.";

const GameQualityFactorSpec* GameQualityFactorSpec::getFactorSpecFromConfig(const eastl::string& propertyName, const eastl::string& transform)
{
    // Since we currently don't have any overlapping property/transforms 
    // (ex. You can do game.players.uedValues[].avg, but not .max/.min/.sum etc.)
    // We just do the lookup via the transform.
    
    // In the future, this should parse the propertyName, and determine if the stuff affects teams/players/etc. 
    const auto& factorSpecs = getAllFactorSpecs();
    for (const auto& factorSpec : factorSpecs)
    {
        if (factorSpec.mTransformName.comparei(transform) == 0)
            return &factorSpec;
    }

    return nullptr;
}

const eastl::string& FieldScore::getFieldName() const
{
    return mFieldDescriptor->mFieldName;
}

Score GameQualityFactor::activeEval(EvalContext& cxt, Game& game)
{
    EA_ASSERT(cxt.mGameState == GameState::FUTURE); // active always evaluates future state!

    auto* boundProperty = cxt.getInputProperty();
    if (boundProperty == nullptr)
    {
        return mFactorSpec.mFactorTransform->activeEval(cxt, game);
    }

    EvalResult bestResult;
    Score bestScore = INVALID_SCORE;
    const FieldDescriptor* bestField = nullptr;
    GameInternalState bestState;
    GameInternalState savedState;
    uint32_t evaluatedFields = 0;
    const auto numParties = game.getPartyCount(cxt.mGameState);
    const auto numFields = game.mFieldValueTable.mFieldValueColumnMap.size();

    // NOTE: Ensure that field order does not unfairly bias field selection when multiple scores are tied,
    // by iterating fields starting from a random spot.
    const auto offset = numFields > 1 ? (game.mEvalRandomSeed % numFields) : 0;

    for (uint32_t i = 0; i < numFields; ++i)
    {
        auto fieldIndex = (i + offset) % numFields;
        auto& fieldColumnItr = game.mFieldValueTable.mFieldValueColumnMap.at(fieldIndex);
        auto* fieldDescriptor = fieldColumnItr.second->mFieldRef.get();
        if (fieldDescriptor->mPropertyDescriptor != boundProperty)
        {
            // only evaluate game fields bound to this property
            continue;
        }

        if (fieldColumnItr.second->mAddTableRefCount - fieldColumnItr.second->mRemoveTableRefCount < numParties)
        {
            // only evaluate game fields whose participation score is 100%
            continue;
        }

        cxt.mCurFieldDescriptor = fieldDescriptor;
        cxt.mCurFieldValueColumn = fieldColumnItr.second;

        if (evaluatedFields == 0)
            game.copyState(savedState, GameState::FUTURE); // save pre-eval state
        else
            game.copyState(GameState::FUTURE, savedState); // restore pre-eval gamestate

        evaluatedFields++;

        auto score = mFactorSpec.mFactorTransform->activeEval(cxt, game);

        if (score > bestScore)
        {
            bestScore = score;
            bestResult = cxt.mEvalResult;
            bestField = fieldDescriptor;
            game.copyState(bestState, GameState::FUTURE); // save best gameteams
        }
        
        if (score == MAX_SCORE)
        {
            mIdealScoreEarlyOutCount++;
            break;
        }
    }

    if (bestScore == INVALID_SCORE)
    {
        if (evaluatedFields == 0)
        {
            cxt.setResultReason(INSUFFICIENT_PARTICIPATION_REASON_TEXT);
        }
    }
    else
    {
        game.copyState(GameState::FUTURE, bestState); // restore best
        cxt.mEvalResult = bestResult;
        cxt.mCurFieldDescriptor = bestField;
    }

    return bestScore;
}

Score GameQualityFactor::passiveEval(EvalContext& cxt, const Game& game) const
{
    auto* boundProperty = cxt.getInputProperty();
    if (boundProperty == nullptr)
    {
        return mFactorSpec.mFactorTransform->passiveEval(cxt, game);
    }

    EvalResult bestResult;
    Score bestScore = INVALID_SCORE;
    const FieldDescriptor* bestField = nullptr;
    uint32_t evaluatedFields = 0;
    const auto numParties = game.getPartyCount(cxt.mGameState);
    const auto numFields = game.mFieldValueTable.mFieldValueColumnMap.size();

    // NOTE: Ensure that field order does not unfairly bias field selection when multiple scores are tied,
    // by iterating fields starting from a random spot.
    const auto offset = numFields > 1 ? (game.mEvalRandomSeed % numFields) : 0;
    auto* viableFieldScoreList = cxt.mViableFieldScoreList; // cache off on stack for performance to allow storing in register
    if (viableFieldScoreList != nullptr)
        viableFieldScoreList->reserve(numFields);

    for (uint32_t i = 0; i < numFields; ++i)
    {
        auto fieldIndex = (i + offset) % numFields;
        auto& fieldColumnItr = game.mFieldValueTable.mFieldValueColumnMap.at(fieldIndex);
        auto* fieldDescriptor = fieldColumnItr.second->mFieldRef.get();
        if (fieldDescriptor->mPropertyDescriptor != boundProperty)
        {
            // only evaluate game fields bound to this property
            continue;
        }

        if (fieldColumnItr.second->mAddTableRefCount - fieldColumnItr.second->mRemoveTableRefCount < numParties)
        {
            // only evaluate game fields that are referenced by all parties in the game
            // (if a party does not reference a field, that means it does not allow it)
            continue;
        }

        cxt.mCurFieldDescriptor = fieldDescriptor;
        cxt.mCurFieldValueColumn = fieldColumnItr.second;

        evaluatedFields++;

        auto score = mFactorSpec.mFactorTransform->passiveEval(cxt, game);
       
        if (score > bestScore)
        {
            bestScore = score;
            bestResult = cxt.mEvalResult;
            bestField = fieldDescriptor;
        }

        if (score != INVALID_SCORE)
        {
            if (viableFieldScoreList == nullptr)
            {
                // check if we can early out
                if (score == MAX_SCORE)
                {
                    mIdealScoreEarlyOutCount++;
                    break;
                }
            }
            else if (isViableScore(computeRawScore(cxt.mEvalResult)))
            {
                cxt.mViableFieldScoreList->push_back({ fieldDescriptor, score, fieldColumnItr.second->getValueParticipationScore(), (uint32_t)cxt.mViableFieldScoreList->size() });
            }
        }
    }

    if (bestScore == INVALID_SCORE)
    {
        if (evaluatedFields == 0)
        {
            cxt.setResultReason(INSUFFICIENT_PARTICIPATION_REASON_TEXT);
        }
    }
    else
    {
        cxt.mEvalResult = bestResult;
        cxt.mCurFieldDescriptor = bestField;
    }

    return bestScore;
}

Score GameQualityFactor::computeScore(const EvalResult& result) const
{
    return computeScore(aggregateResults(mResultAggregate, result));
}

Score GameQualityFactor::computeScore(Score resultValue) const
{
    Score score = INVALID_SCORE;
    if (resultValue >= 0)
    {
        // Apply granularity based on whatever range was provided (so if the source was 0-1000, we might use granularity 10)
        if (mGranularity > 0.0f)
        {
            resultValue = roundf(resultValue * mInverseGranularity) * mGranularity;
        }

        // Determine if range is going high -> low, or low -> high:
        if (mInverseResultValueRange < 0)
        {
            // 1.0 low -> high 0.0
            if (EA_UNLIKELY(resultValue <= mGoodEnoughValue))
                score = 1.0f;
            else if (EA_UNLIKELY(resultValue >= mWorstValue))
                score = 0.0f;
            else
                score = 1.0f + (resultValue * mInverseResultValueRange);     // Score range goes:   1.0 (best) -> 1.0 (good enough) -> 0.xxx (!good enough) -> 0.0 (worst)
        }
        else
        {
            // 1.0 high -> low 0.0
            if (EA_UNLIKELY(resultValue >= mGoodEnoughValue))
                score = 1.0f;
            else if (EA_UNLIKELY(resultValue <= mWorstValue))
                score = 0.0f;
            else
                score = resultValue * mInverseResultValueRange;
        }
    }
    return score;
}

Score GameQualityFactor::computeRawScore(const EvalResult& result) const
{
    return computeRawScore(aggregateResults(mResultAggregate, result));
}

Score GameQualityFactor::computeRawScore(Score resultValue) const
{
    Score score = INVALID_SCORE;
    if (resultValue >= MIN_SCORE)
    {
        // Determine if range is going high -> low, or low -> high:
        if (mInverseResultValueRange < 0)
        {
            // 1.0 low -> high 0.0
            if (EA_UNLIKELY(resultValue <= mBestValue))
                score = 1.0f;
            else if (EA_UNLIKELY(resultValue >= mWorstValue))
                score = 0.0f;
            else
                score = 1.0f + (resultValue * mInverseResultValueRange);     // Score range goes:   1.0 (best) -> 1.0 (good enough) -> 0.xxx (!good enough) -> 0.0 (worst)
        }
        else
        {
            // 1.0 high -> low 0.0
            if (EA_UNLIKELY(resultValue >= mBestValue))
                score = 1.0f;
            else if (EA_UNLIKELY(resultValue <= mWorstValue))
                score = 0.0f;
            else
                score = resultValue * mInverseResultValueRange;
        }
    }
    return score;
}

Score GameQualityFactor::aggregateResults(PropertyAggregate propAggregate, const EvalResult& result) const
{
    if (result.empty())
        return INVALID_SCORE;

    if (result.size() == 1)
        return result[0];

    Score aggregateValue = 0;
    switch (propAggregate)
    {
    case PropertyAggregate::SUM:
        for (auto propValue : result)
        {
            aggregateValue += propValue;
        }
        break;
    case PropertyAggregate::MIN:
        aggregateValue = result[0];
        for (auto propValue : result)
        {
            if (propValue < aggregateValue)
                aggregateValue = propValue;
        }
        break;
    case PropertyAggregate::MAX:
        aggregateValue = result[0];
        for (auto propValue : result)
        {
            if (propValue > aggregateValue)
                aggregateValue = propValue;
        }
        break;

    case PropertyAggregate::SIZE:
        aggregateValue = (Score)result.size();
        break;
    case PropertyAggregate::AVERAGE:
        aggregateValue = 0;
        for (auto propValue : result)
        {
            aggregateValue += propValue;
        }
        aggregateValue /= result.size();
        break;
    case PropertyAggregate::STDDEV:
    {
        Score avgValue = 0;
        for (auto propValue : result)
        {
            avgValue += propValue;
        }
        avgValue /= result.size();

        aggregateValue = 0;
        for (auto propValue : result)
        {
            aggregateValue += (propValue > avgValue) ? (propValue - avgValue) : (avgValue - propValue);
        }
        aggregateValue /= result.size();
        break;
    }
    case PropertyAggregate::MINMAXRANGE:
    {
        Score minValue = result[0];
        Score maxValue = result[0];
        for (auto propValue : result)
        {
            if (propValue < minValue)
                minValue = propValue;
            if (propValue > maxValue)
                maxValue = propValue;
        }
        aggregateValue = maxValue - minValue;
        break;
    }
    case PropertyAggregate::MINMAXRATIO:
    {
        Score minValue = result[0];
        Score maxValue = result[0];
        for (auto propValue : result)
        {
            if (propValue < minValue)
                minValue = propValue;
            if (propValue > maxValue)
                maxValue = propValue;
        }
        if (maxValue != 0)
            aggregateValue = minValue / maxValue;
        break;
    }

    default:
        aggregateValue = INVALID_SCORE;
        EA_FAIL_MSG("Unsupported Property Aggregate Type");
    }
    return aggregateValue;
}


Score GameQualityFactor::computeViableScore() const
{
    // Convert mViableValue to the 0 - 1 range:
    EvalResult result = { mViableValue };
    return computeScore(result);
}

const char8_t* GameQualityFactor::getValuesAsString()
{
    return mValuesAsString.c_str();
}

Score GameQualityFactor::computeSortedFieldScores(FieldScoreList& fieldScoreList, const Game& game, GameState state) const
{
    EvalContext evalContext(*this, state);
    evalContext.setComputeViableFieldScores(&fieldScoreList);
    auto score = passiveEval(evalContext, game);
    if (score != INVALID_SCORE)
    {
        evalContext.sortFieldScoreListDescending();
    }
    return score;
}

bool GameQualityFactor::hasValidPropertyRequirements() const
{
    return !mFactorSpec.mRequiresProperty || mInputProperty != nullptr;
}

float GameQualityFactor::getEvaluatedValueFromScore(Score score) const
{
    auto factorValueRange = mBestValue - mWorstValue;
    float evaluatedValue = 0.0f;
    if (factorValueRange < 0)
    {
        evaluatedValue = factorValueRange * (score - 1);
    }
    else
    {
        evaluatedValue = factorValueRange * score;
    }
    return evaluatedValue;
}

}

