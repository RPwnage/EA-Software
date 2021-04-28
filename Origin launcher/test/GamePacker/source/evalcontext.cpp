/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EASTL/sort.h>
#include "gamepacker/evalcontext.h"

namespace Packer
{

EvalContext::EvalContext(const GameQualityFactor& factor, GameState gameState)
    : mFactor(factor), mGameState(gameState)
{
    EA_ASSERT(factor.hasValidPropertyRequirements());
}

Score EvalContext::computeScore() const
{
    return mFactor.computeScore(mEvalResult);
}

Score EvalContext::computeRawScore() const
{
    return mFactor.computeRawScore(mEvalResult);
}

void EvalContext::sortFieldScoreListDescending()
{
    EA_ASSERT(mViableFieldScoreList);
    // NOTE: Important to use stable sorting here, because we want to guarantee that ties remain ordered
    // the same way as they are inserted into the list by the evaluation that utilizes a random seed.
    // This ensures that the first field in the sorted full field list, is always the 'best' field
    // as determined by the evaluation algorithm.
    eastl::sort(mViableFieldScoreList->begin(), mViableFieldScoreList->end(), 
        [](const FieldScore& a, const FieldScore& b) {
            if (a.mScore != b.mScore)
                return a.mScore > b.mScore;
            else
                return a.mEvalOrder < b.mEvalOrder;
    });
}

} // GamePacker
