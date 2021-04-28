/*! ************************************************************************************************/
/*!
\file fitformula.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_FITFORMULA_H
#define BLAZE_GAMEMANAGER_FITFORMULA_H

#include "gamemanager/tdf/matchmaker_config_server.h"

#include "gamemanager/matchmaker/rules/gaussianfunction.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class FitFormula
    {
    public:
        virtual ~FitFormula();
        static FitFormula* createFitFormula(FitFormulaName fitFormulaName);

        virtual bool initialize(const FitFormulaConfig& config, const RangeOffsetLists* rangeOffsetLists) = 0;
        virtual float getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const = 0;

    protected:
        static bool getFitFormulaParamUInt32(const FitFormulaParams& params, const char8_t* key, uint32_t& value);
        static bool getFitFormulaParamInt32(const FitFormulaParams& params, const char8_t* key, int32_t& value);
        static bool getFitFormulaParamInt64(const FitFormulaParams& params, const char8_t* key, int64_t& value);
        static bool getFitFormulaParamDouble(const FitFormulaParams& params, const char8_t* key, double& value);
    };

    class GaussianFitFormula : public FitFormula
    {
        NON_COPYABLE(GaussianFitFormula);
    public:
        GaussianFitFormula();
        ~GaussianFitFormula() override;

        bool initialize(const FitFormulaConfig& config, const RangeOffsetLists* rangeOffsetLists) override;
        float getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const override;

    private:
        GaussianFunction mGaussianFunction;
    };


    class LinearFitFormula : public FitFormula
    {
        NON_COPYABLE(LinearFitFormula);
    public:
        static int64_t INVALID_LINEAR_VALUE;
    public:
        LinearFitFormula();
        ~LinearFitFormula() override;
        
        bool initialize(const FitFormulaConfig& config, const RangeOffsetLists* rangeOffsetLists) override;
        float getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const override;

        void setDefaultMaxVal(int64_t defaultMaxVal) { mMaxVal = defaultMaxVal; }
        void setDefaultMinVal(int64_t defaultMinVal) { mMinVal = defaultMinVal; }

    private:
        void setDefaultsFromRangeOffsetLists(const RangeOffsetLists& rangeOffsetLists);

        int64_t mMaxVal;
        int64_t mMinVal;
        double mMaxFit;
        double mMinFit;

        // cached constant slopes
        double mLeftSlope;
        double mRightSlope;
    };


    class BinaryFitFormula : public FitFormula
    {
        NON_COPYABLE(BinaryFitFormula);
    public:
        BinaryFitFormula();
        ~BinaryFitFormula() override;

        bool initialize(const FitFormulaConfig& config, const RangeOffsetLists* rangeOffsetLists) override;
        float getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const override;

    private:
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_FITFORMULA_H
