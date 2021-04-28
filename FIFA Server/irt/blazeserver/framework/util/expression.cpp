/*************************************************************************************************/
/*!
\file expression.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/expression.h"

namespace Blaze
{
    int32_t Expression::evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const
    {
        // All string type expressions must override this method, so we should never
        // get here if the type is actually string
        EA_ASSERT(mType != EXPRESSION_TYPE_STRING);

        // Otherwise the default behaviour for all numeric types is to evaluate to the
        // string representation of the number - note that floating point numbers will
        // be somewhat ugly as they will print with full decimal precision, it will likely
        // need to be a future enhancement to come up with an elegant way to specify precision
        if (isInt())
            return blaze_snzprintf(buf, bufLen, "%" PRId64, evalInt(cb, context));
        if (isFloat())
            return blaze_snzprintf(buf, bufLen, "%f", (double_t)(evalFloat(cb, context)));
        return 0;
    }

    int64_t StringLiteralExpression::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        char8_t* strEnd = nullptr;
        int64_t tempInt = EA::StdC::StrtoI64(mValue, &strEnd, 0);
        if (strEnd == mValue + strlen(mValue))
        {
            return tempInt;
        }
        return 0;
    }

    float_t StringLiteralExpression::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        char8_t* strEnd = nullptr;
        float_t tempFloat = EA::StdC::StrtoF32(mValue, &strEnd);
        if (strEnd == mValue + strlen(mValue))
        {
            return tempFloat;
        }
        return 0;
    }


    int64_t VariableIntExpression::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        ExpressionVariableVal val;
        cb(mNameSpace, mName, mType, context, val);
        return val.intVal;
    }

    float_t VariableIntExpression::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        ExpressionVariableVal val;
        cb(mNameSpace, mName, mType, context, val);
        return static_cast<float_t>(val.intVal);
    }

    int32_t VariableIntExpression::evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const
    {
        return blaze_snzprintf(buf, bufLen, "%" PRId64, evalInt(cb, context));
    }

    int64_t VariableFloatExpression::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        ExpressionVariableVal val;
        cb(mNameSpace, mName, mType, context, val);
        return static_cast<int64_t>(val.floatVal);
    }

    float_t VariableFloatExpression::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        ExpressionVariableVal val;
        cb(mNameSpace, mName, mType, context, val);
        return val.floatVal;
    }

    int32_t VariableFloatExpression::evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const
    {
        return blaze_snzprintf(buf, bufLen, "%f", (double_t)evalFloat(cb, context));
    }

    int32_t VariableStringExpression::evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const
    {
        ExpressionVariableVal val;
        cb(mNameSpace, mName, mType, context, val);
        return blaze_snzprintf(buf, bufLen, "%s", val.stringVal);
    }

    int64_t VariableStringExpression::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        char8_t buf[64] = { 0 };
        if (evalString(cb, context, buf, sizeof(buf)))
        {
            char8_t* strEnd = nullptr;
            int64_t tempInt = EA::StdC::StrtoI64(buf, &strEnd, 0);
            if (strEnd == buf + strlen(buf))
            {
                return tempInt;
            }
        }
        return 0;
    }

    float_t VariableStringExpression::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        char8_t buf[64] = { 0 };
        if (evalString(cb, context, buf, sizeof(buf)))
        {
            char8_t* strEnd = nullptr;
            float_t tempFloat = EA::StdC::StrtoF32(buf, &strEnd);
            if (strEnd == buf + strlen(buf))
            {
                return tempFloat;
            }
        }
        return 0;
    }


    FunctionExpression::FunctionExpression(char8_t* functionName, ExpressionArgumentList& args)
        : Expression(EXPRESSION_TYPE_INT, args.getErrorCount()), mFunctionName(functionName), mArgs(args)
    {}

    FunctionExpression::~FunctionExpression()
    {
        BLAZE_FREE(mFunctionName);
        delete &mArgs;
    }

    void FunctionExpression::destroy()
    {
        mArgs.destroy();
        delete this;
    }

    FunctionExpressionSqrt::FunctionExpressionSqrt(char8_t* functionName, ExpressionArgumentList& args)
        : FunctionExpression(functionName, args)
    {
        mType = EXPRESSION_TYPE_FLOAT;
        if (args.getArgumentList().size() != 1)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take square root of " << args.getArgumentList().size() << " arguments");
            ++mErrorCount;
        }

        ArgumentList::const_iterator iter = args.getArgumentList().begin();
        const Expression* arg = *iter;
        if (arg->isString())
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take square root of a string");
            ++mErrorCount;
        }
    }
    int64_t FunctionExpressionSqrt::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        return (int64_t)evalFloat(cb, context);
    }
    float_t FunctionExpressionSqrt::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        float_t arg = (*mArgs.getArgumentList().begin())->evalFloat(cb, context);
        if (arg < 0)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression].evalFloat(ResolveVariableCb &cb, const void* context): "
                "Caught attempt to square root a negative, evaluating to zero instead");
            return 0.0;
        }
        return sqrtf(arg);
    }

    FunctionExpressionMax::FunctionExpressionMax(char8_t* functionName, ExpressionArgumentList& args)
        : FunctionExpression(functionName, args)
    {
        if (args.getArgumentList().size() == 0)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take max of 0 arguments");
            ++mErrorCount;
        }

        // set type to int for now - if any arg is a float though we'll
        // then set it to float below
        mType = EXPRESSION_TYPE_INT;

        for (ArgumentList::const_iterator iter = args.getArgumentList().begin();
            iter != args.getArgumentList().end(); ++iter)
        {
            const Expression* exp = *iter;
            if (exp->isString())
            {
                BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take max of a string");
                ++mErrorCount;
            }
            else if (exp->isFloat())
                mType = EXPRESSION_TYPE_FLOAT;
        }
    }
    int64_t FunctionExpressionMax::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        ArgumentList::const_iterator iter = mArgs.getArgumentList().begin();
        int64_t retval = (*iter++)->evalInt(cb, context);
        while (iter != mArgs.getArgumentList().end())
        {
            int64_t argval = (*iter++)->evalInt(cb, context);
            if (argval > retval)
                retval = argval;
        }
        return retval;
    }
    float_t FunctionExpressionMax::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        ArgumentList::const_iterator iter = mArgs.getArgumentList().begin();
        float_t retval = (*iter++)->evalFloat(cb, context);
        while (iter != mArgs.getArgumentList().end())
        {
            float_t argval = (*iter++)->evalFloat(cb, context);
            if (argval > retval)
                retval = argval;
        }
        return retval;
    }

    FunctionExpressionMin::FunctionExpressionMin(char8_t* functionName, ExpressionArgumentList& args)
        : FunctionExpression(functionName, args)
    {
        if (args.getArgumentList().size() == 0)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take min of 0 arguments");
            ++mErrorCount;
        }

        // set type to int for now - if any arg is a float though we'll
        // then set it to float below
        mType = EXPRESSION_TYPE_INT;

        for (ArgumentList::const_iterator iter = args.getArgumentList().begin();
            iter != args.getArgumentList().end(); ++iter)
        {
            const Expression* exp = *iter;
            if (exp->isString())
            {
                BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take min of a string");
                ++mErrorCount;
            }
            else if (exp->isFloat())
                mType = EXPRESSION_TYPE_FLOAT;
        }
    }
    int64_t FunctionExpressionMin::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        ArgumentList::const_iterator iter = mArgs.getArgumentList().begin();
        int64_t retval = (*iter++)->evalInt(cb, context);
        while (iter != mArgs.getArgumentList().end())
        {
            int64_t argval = (*iter++)->evalInt(cb, context);
            if (argval < retval)
                retval = argval;
        }
        return retval;
    }
    float_t FunctionExpressionMin::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        ArgumentList::const_iterator iter = mArgs.getArgumentList().begin();
        float_t retval = (*iter++)->evalFloat(cb, context);
        while (iter != mArgs.getArgumentList().end())
        {
            float_t argval = (*iter++)->evalFloat(cb, context);
            if (argval < retval)
                retval = argval;
        }
        return retval;
    }

    FunctionExpressionCeil::FunctionExpressionCeil(char8_t* functionName, ExpressionArgumentList& args)
        : FunctionExpression(functionName, args)
    {
        mType = EXPRESSION_TYPE_INT;
        if (args.getArgumentList().size() != 1)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take ceil of " << args.getArgumentList().size() << " arguments");
            ++mErrorCount;
        }

        ArgumentList::const_iterator iter = args.getArgumentList().begin();
        const Expression* arg = *iter;
        if (arg->isString())
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take ceil of a string");
            ++mErrorCount;
        }
    }
    int64_t FunctionExpressionCeil::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        return int64_t(evalFloat(cb, context));
    }
    float_t FunctionExpressionCeil::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        return ceilf((*mArgs.getArgumentList().begin())->evalFloat(cb, context));
    }

    FunctionExpressionFloor::FunctionExpressionFloor(char8_t* functionName, ExpressionArgumentList& args)
        : FunctionExpression(functionName, args)
    {
        mType = EXPRESSION_TYPE_INT;
        if (args.getArgumentList().size() != 1)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take floor of " << args.getArgumentList().size() << " arguments");
            ++mErrorCount;
        }

        ArgumentList::const_iterator iter = args.getArgumentList().begin();
        const Expression* arg = *iter;
        if (arg->isString())
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take floor of a string");
            ++mErrorCount;
        }
    }
    int64_t FunctionExpressionFloor::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        return int64_t(evalFloat(cb, context));
    }
    float_t FunctionExpressionFloor::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        return floorf((*mArgs.getArgumentList().begin())->evalFloat(cb, context));
    }

    FunctionExpressionExp::FunctionExpressionExp(char8_t* functionName, ExpressionArgumentList& args)
        : FunctionExpression(functionName, args)
    {
        mType = EXPRESSION_TYPE_FLOAT;
        if (args.getArgumentList().size() != 1)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take exponent of " << args.getArgumentList().size() << " arguments");
            ++mErrorCount;
        }

        ArgumentList::const_iterator iter = args.getArgumentList().begin();
        const Expression* arg = *iter;
        if (arg->isString())
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take exponent of a string");
            ++mErrorCount;
        }
    }
    int64_t FunctionExpressionExp::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        return (int64_t)evalFloat(cb, context);
    }
    float_t FunctionExpressionExp::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        return expf((*mArgs.getArgumentList().begin())->evalFloat(cb, context));
    }

    FunctionExpressionLog::FunctionExpressionLog(char8_t* functionName, ExpressionArgumentList& args)
        : FunctionExpression(functionName, args)
    {
        mType = EXPRESSION_TYPE_FLOAT;
        if (args.getArgumentList().size() != 1)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take logarithm of " << args.getArgumentList().size() << " arguments");
            ++mErrorCount;
        }

        ArgumentList::const_iterator iter = args.getArgumentList().begin();
        const Expression* arg = *iter;
        if (arg->isString())
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression]: Cannot take logarithm of a string");
            ++mErrorCount;
        }
    }
    int64_t FunctionExpressionLog::evalInt(ResolveVariableCb &cb, const void* context) const
    {
        return (int64_t)evalFloat(cb, context);
    }
    float_t FunctionExpressionLog::evalFloat(ResolveVariableCb &cb, const void* context) const
    {
        float_t arg = (*mArgs.getArgumentList().begin())->evalFloat(cb, context);
        if (arg <= 0.0)
        {
            BLAZE_WARN_LOG(Log::UTIL, "[FunctionExpression].evalFloat(ResolveVariableCb &cb, const void* context): "
                "Caught attempt to logarithm a float which is non-positive, evaluating to 0 instead");
            return 0.0;
        }
        return logf(arg);
    }


} //namespace Blaze

