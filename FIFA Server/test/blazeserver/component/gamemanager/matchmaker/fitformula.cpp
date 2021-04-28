/*! ************************************************************************************************/
/*!
\file fitformula.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "fitformula.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    FitFormula::~FitFormula()
    {

    }

    FitFormula* FitFormula::createFitFormula(FitFormulaName fitFormulaName)
    {
        switch(fitFormulaName)
        {
        case FIT_FORMULA_GAUSSIAN:
            return BLAZE_NEW GaussianFitFormula();
        case FIT_FORMULA_LINEAR:
            return BLAZE_NEW LinearFitFormula();
        case FIT_FORMULA_BINARY:
            return BLAZE_NEW BinaryFitFormula();
        default:
            return nullptr;
        }   
    }

    bool FitFormula::getFitFormulaParamUInt32(const FitFormulaParams& params, const char8_t* key, uint32_t& value)
    {
        value = 0;

        FitFormulaParams::const_iterator itr = params.find(key);
        if (itr != params.end())
        {
            if (blaze_str2int(itr->second, &value) != itr->second)
                return true;
        }

        return false;
    }

    bool FitFormula::getFitFormulaParamInt32(const FitFormulaParams& params, const char8_t* key, int32_t& value)
    {
        value = 0;

        FitFormulaParams::const_iterator itr = params.find(key);
        if (itr != params.end())
        {            
            if (blaze_str2int(itr->second, &value) != itr->second)
                return true;
        }
            
        return false;
    }

    bool FitFormula::getFitFormulaParamInt64(const FitFormulaParams& params, const char8_t* key, int64_t& value)
    {
        value = 0;

        FitFormulaParams::const_iterator itr = params.find(key);
        if (itr != params.end())
        {        
            if (blaze_str2int(itr->second, &value) != itr->second)
                return true;
        }

        return false;
    }

    bool FitFormula::getFitFormulaParamDouble(const FitFormulaParams& params, const char8_t* key, double& value)
    {
        value = 0.0;

        FitFormulaParams::const_iterator itr = params.find(key);
        if (itr != params.end())
        {
            char8_t* endPtr;
            value = strtod(itr->second, &endPtr);
            if (itr->second != endPtr)
            {
                return true;
            }
            else
            {
                value = 0.0;
            }
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // Gaussian Fit Formula
    //////////////////////////////////////////////////////////////////////////

    GaussianFitFormula::GaussianFitFormula() {}
    GaussianFitFormula::~GaussianFitFormula() {}

    // fitFormula = {name = FIT_FORMULA_GAUSSIAN, params = {fiftyPercentFitValueDifference = 200}}
    bool GaussianFitFormula::initialize(const FitFormulaConfig& config, const RangeOffsetLists* rangeOffsetLists)
    {
        uint32_t fiftyPercentFitValueDifference;
        if (!getFitFormulaParamUInt32(config.getParams(), "fiftyPercentFitValueDifference", fiftyPercentFitValueDifference))
        {
            ERR_LOG("[GaussianFitFormula].initialize failed to init required param 'fiftyPercentFitValueDifference'");
            return false;
        }

        return mGaussianFunction.initialize(fiftyPercentFitValueDifference, GaussianFunction::CACHE_TYPE_ARRAY, 1024);
    }

    float GaussianFitFormula::getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const
    {
        return mGaussianFunction.calculate((int32_t)myValue, (int32_t)otherValue);
    }

    //////////////////////////////////////////////////////////////////////////
    // Linear Fit Formula
    //////////////////////////////////////////////////////////////////////////
    int64_t LinearFitFormula::INVALID_LINEAR_VALUE = -1;

    LinearFitFormula::LinearFitFormula() : mMaxVal(INVALID_LINEAR_VALUE), mMinVal(INVALID_LINEAR_VALUE), mMaxFit(1.0), mMinFit(0.0), mLeftSlope(-1), mRightSlope(-1) {}
    LinearFitFormula::~LinearFitFormula() {}

    void LinearFitFormula::setDefaultsFromRangeOffsetLists(const RangeOffsetLists& rangeOffsetLists)
    {
        RangeOffsetLists::const_iterator itr = rangeOffsetLists.begin();
        RangeOffsetLists::const_iterator end = rangeOffsetLists.end();

        RangeList::Range rangeValue;
        for (; itr != end; ++itr)
        {
            const RangeOffsetList& rangeOffsetList = **itr;
            const RangeOffsets& offsets = rangeOffsetList.getRangeOffsets();
            
            RangeOffsets::const_iterator offsetItr = offsets.begin();
            RangeOffsets::const_iterator offsetEnd = offsets.end();
            for (; offsetItr != offsetEnd; ++offsetItr)            
            {
                const RangeOffset& offset = **offsetItr;
                if (RangeListContainer::parseRange(offset.getOffset(), rangeValue))
                {
                    // Special case values: 
                    if (rangeValue.mMinValue == RangeList::EXACT_MATCH_REQUIRED_VALUE) rangeValue.mMinValue = 0;
                    if (rangeValue.mMaxValue == RangeList::EXACT_MATCH_REQUIRED_VALUE) rangeValue.mMaxValue = 0;
                    if (rangeValue.mMinValue == RangeList::INFINITY_RANGE_VALUE) rangeValue.mMinValue = INT64_MAX;
                    if (rangeValue.mMaxValue == RangeList::INFINITY_RANGE_VALUE) rangeValue.mMaxValue = INT64_MAX;

                    // Take the farthest min/max values as defaults:
                    if (rangeValue.mMinValue >= 0 && (mMinVal == INVALID_LINEAR_VALUE || rangeValue.mMinValue < mMinVal))
                        mMinVal = rangeValue.mMinValue;
                    if (rangeValue.mMaxValue >= 0 && (mMaxVal == INVALID_LINEAR_VALUE || rangeValue.mMaxValue > mMaxVal))
                        mMaxVal = rangeValue.mMaxValue;
                }
            }
        }
    }

    // fitFormula = {name = FIT_FORMULA_LINEAR, params = {minfit=0.0, maxfit=1.0, minVal=10, maxVal=10}}
    bool LinearFitFormula::initialize(const FitFormulaConfig& config, const RangeOffsetLists* rangeOffsetLists)
    {
        if (rangeOffsetLists != nullptr)
            setDefaultsFromRangeOffsetLists(*rangeOffsetLists);

        int64_t cfgMinVal = INVALID_LINEAR_VALUE;
        if (!getFitFormulaParamInt64(config.getParams(), "minVal", cfgMinVal))
        {
            // ERROR out unless the default was already set.
            if (mMinVal == INVALID_LINEAR_VALUE)
            {
                ERR_LOG("[LinearFitFormula].initialize failed to init required param 'minVal'");
                return false;
            }
        }
        else
        {
            mMinVal = cfgMinVal;
        }

        if (mMinVal < 0)
        {
            ERR_LOG("[LinearFitFormula].initialize failed to init required param 'minVal' value '" << mMinVal << "' out of range");
            return false;
        }

        int64_t cfgMaxVal = INVALID_LINEAR_VALUE;
        if (!getFitFormulaParamInt64(config.getParams(), "maxVal", cfgMaxVal))
        {
            // ERROR out unless the default was already set.
            if (mMaxVal == INVALID_LINEAR_VALUE)
            {
                ERR_LOG("[LinearFitFormula].initialize failed to init required param 'maxVal'");
                return false;
            }
        }
        else
        {
            mMaxVal = cfgMaxVal;
        }

        if (mMaxVal < mMinVal)
        {
            ERR_LOG("[LinearFitFormula].initialize failed to init required param 'maxVal' value '" << mMaxVal << "' out of range. (Expected > 'minVal' '" << mMinVal << "')");
            return false;
        }

        double cfgMinFit = -1;
        if (!getFitFormulaParamDouble(config.getParams(), "minFit", cfgMinFit))
        {
            // Do nothing here. mMinFit default is 0.0
        }
        else
        {
            mMinFit = cfgMinFit;
        }

        double cfgMaxFit = -1;
        if (!getFitFormulaParamDouble(config.getParams(), "maxFit", cfgMaxFit))
        {
            // Do nothing here. mMaxFit default is 1.0
        }
        else
        {
            mMaxFit = cfgMaxFit;
        }

        // Pre-calculate the slopes, handle divide by 0 case.
        mLeftSlope = ((mMaxVal - mMinVal) == 0)? 0: ((mMaxFit - mMinFit) / (mMaxVal - mMinVal));
        mRightSlope = -1 * mLeftSlope;

        return true;
    }

    float LinearFitFormula::getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const
    {
        double m = 0.0;

        // early out for a perfect match 
        if (myValue == otherValue) return (float)mMaxFit;

        // normalize to the min fit if outside of the acceptable range.
        UserExtendedDataValue absoluteMaxVal = myValue < INT64_MAX - mMaxVal ? myValue + mMaxVal : INT64_MAX;  // Handle Overflow
        UserExtendedDataValue absoluteMinVal = myValue > INT64_MIN + mMinVal ? myValue - mMinVal : INT64_MIN;  // Handle Overflow

        // early out for values outside of the acceptable range
        if ( (otherValue > absoluteMaxVal) || (otherValue < absoluteMinVal)) return (float)mMinFit;

        if (myValue < otherValue) m = mRightSlope;
        if (myValue > otherValue) m = mLeftSlope;

        double b = mMaxFit - m * myValue; // b = y0 - m * x0

        return (float)(m * otherValue + b); // y1 = m * x1 + b
    }

    //////////////////////////////////////////////////////////////////////////
    // Binary Fit Formula
    //////////////////////////////////////////////////////////////////////////

    BinaryFitFormula::BinaryFitFormula() {}
    BinaryFitFormula::~BinaryFitFormula() {}

    // fitFormula = {name = FIT_FORMULA_BINARY}
    bool BinaryFitFormula::initialize(const FitFormulaConfig& config, const RangeOffsetLists* rangeOffsetLists)
    {
        return true;
    }

    float BinaryFitFormula::getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const
    {
        return 1.0; // currently the range is dictated by the rule.  So either perfect match or no match.
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
