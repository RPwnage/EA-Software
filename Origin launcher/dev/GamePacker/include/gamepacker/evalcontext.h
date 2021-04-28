/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef GAME_PACKER_EVAL_CONTEXT_H
#define GAME_PACKER_EVAL_CONTEXT_H

#include "gamepacker/common.h"
#include "gamepacker/property.h"
#include "gamepacker/qualityfactor.h"


namespace Packer
{

struct EvalContext
{
    const GameQualityFactor& mFactor;
    const FieldDescriptor* mCurFieldDescriptor = nullptr;
    const FieldValueColumn* mCurFieldValueColumn = nullptr;
    EvalResult mEvalResult;
    const char8_t* mResultReason = nullptr;
    GameState mGameState = GameState::PRESENT;
    FieldScoreList* mViableFieldScoreList = nullptr;

    EvalContext() = delete;
    EvalContext(const EvalContext&) = delete;
    EvalContext& operator=(const EvalContext&) = delete;
    EvalContext(const GameQualityFactor& factor, GameState gameState);
    Score computeScore() const;
    Score computeRawScore() const;
    void setResultReason(const char8_t* reason) { mResultReason = reason; }
    void setComputeViableFieldScores(FieldScoreList* fieldScoreList) { mViableFieldScoreList = fieldScoreList; }
    const PropertyDescriptor* getInputProperty() const { return mFactor.mInputProperty; }
    void sortFieldScoreListDescending();
    const char8_t* getResultReason() const { return mResultReason == nullptr ? "none" : mResultReason; }
};


}
#endif