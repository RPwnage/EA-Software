/*************************************************************************************************/
/*!
    \file expression.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_EXPRESSION_H
#define BLAZE_EXPRESSION_H

/*** Include files *******************************************************************************/
#include "math.h"
#include "EASTL/hash_set.h"
#include "EASTL/list.h"
#include "framework/callback.h"
#include "framework/logger.h"
#include "framework/util/shared/blazestring.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
    typedef enum
    {
        EXPRESSION_TYPE_INT = 0,
        EXPRESSION_TYPE_FLOAT = 1,
        EXPRESSION_TYPE_STRING = 2,
        EXPRESSION_TYPE_NONE = -1
    } ExpressionValueType;

    typedef eastl::hash_set<const char8_t*, eastl::hash<const char8_t*>,
                            eastl::str_equal_to<const char8_t*> > StringSet;

    class DependencyContainer
    {
    public :
        virtual ~DependencyContainer() {}
        virtual void pushDependency(void* context, const char8_t* nameSpace, const char8_t* name) = 0;
    };

/*************************************************************************************************/
/*
    The expression classes in this file provide a means to evaluate simple formulas, 
    encapsulating all of the supported syntax.  Specifically, the supported syntax at 
    present includes (in order of increasing binding strength):

    The ternary operator    ?:
    String concatenation    .
    Logical comparisons     || &&
    Bitwise operators       | ^ &
    Equality comparisons    == !=
    Relational expressions  > < >= <=
    Bit shift expressions   >> <<
    Multiplicative math     * / mod
    Additive math           + -
    Unary expressions       + - ! ~
    Primitives              int literal, float literal, string literal, function calls
    Parentheses             ()

    The function calls that are supported are: sqrt, min, max, floor, ceil, exp, log.  It should be fairly
    easy to also add support for any other requested functions.  Note that these functions are not
    strictly identical to standard C++ functions of the same names.  The min and max functions
    actually support an arbitrary number of arguments.  The floor and ceil functions return
    integers as opposed to the standard C++ versions.  The sqrt and log functions use the standard C++
    implementation, but catch negative inputs and return 0 instead.

    As mentioned above, the square root function handles errors (negative arguments) gracefully by
    returning 0 instead of attempting to evaluate the root.  Similarly, the division and modulo
    operations handle attempts to divide by zero by also returning zero.  Should a customer wish a
    different behaviour, they may always craft a conditional expression that implements the precise
    behaviour desired.
    
    Note that there is currently no special handling for infinity - it is possible by for example
    executing (3.4e28 / 0.000001) to overflow the max range of a float and obtain the special
    infinity value.  It is the responsibility of the customer to avoid these cases.

    String literals are supported by enclosing them in single quotes.  Double-quotes currently
    encapsulate string literals at the config file level, and get stripped out before the parsing
    code has a chance to see them, so at some point in the future if escaping allows double quotes
    to be passed through then the expression code may be modified to use them instead if so desired.

    Note the addition of the non-standard string concatenation operator - the period is one of the
    few punctuation characters that isn't already an operator, nor does it have any meaning to the
    config file parser.  Some scripting languages also use it as a string concat operator, so it is
    a natural choice.  Assigning it low precedence seems natural - numeric values resulting from
    more tightly bound operations are likely to be converted to strings, but there would rarely be
    a need to go the other way.  The only operator with a lower precedence though is the ternary
    logic operator, which allows us to write expressions like:
    myScore > yourScore ? 'I won by' . myScore - yourScore : 'You won by' . yourScore - myScore

    Expression classes in this file represent at the lowest level the primitives, and then each of
    the operators as well.  Expressions for an operator take in expressions representing their
    arguments in the constructor.
    
    For example the simple expression 'wins + 2' would be parsed into an AddExpression whose left
    hand side expression is a VariableIntExpression representing the wins statistic and whose right
    hand side expression is an IntExpression representing the constant value 2.

    In order to support features such as derived stats across multiple stat categories,
    variable expressions may include both a namespace and a name, for example
    'SomeCategory::wins + 2'.

    Continuing the example, when the overall addition expression above is evaluated, it will
    delegate first to evaluate it's two argument expressions - the left hand side will evaluate to
    whatever the value the wins variable has, and the right hand side will always evaluate to 2.
    The add expression will then return the sum of these two values.

    The parent class here is Expression, it is an abstract class that defines the general interface
    that all expression classes will use.  All expression classes will have a type and an error
    count determined at construction time (discussed in more detail below).  During regular
    operation the eval methods are used to evaluate an expression as either an int, float, or
    string.  Each subclass that encapsulates a specific operation or primitive implements these
    methods appropriately to perform the requested operation.

    The type field determines the type that an expression would normally return.  The type has two
    main purposes: first at parse time (i.e. server startup) it is used to validate whether an
    expression is even valid; and second during evaluation (i.e. regular runtime) it is used to
    determine how expressions are evaluated.
    
    The type behaviour essentially mirrors how expressions would be evaluated in C++.  For example,
    an addition expression which has one integer argument and one float argument would up-cast the
    integer to a float in regular C++, and thus evaluate to a float.  The same behaviour occurs
    here, the type of the add expression is determined during parse time to be int if both
    arguments are ints, float if one or both arguments are float, but it is an error if either
    argument is a string (see later paragraph for more about errors).  While arithmetic expressions
    depend on the types of their arguments, others like equality always return an integer in C++,
    and they do the same here.

    While the type gives an indication of what type would normally be returned by an expression,
    the parent expression always has the option of calling a different evaluate method.  For
    example, an addition of two ints (2 + 3) would normally return the integer 5, however the
    parent expression calling it (for example 5.0 * (2 + 3)), may prefer the result as a float.
    Thus it is perfectly legal to call the evalFloat(ResolveVariableCb &cb, const void* context) method
    on an add expression, even if the add expression's type is actually set to int.  It is left to
    the parse-time code to not allow expressions to be constructed that would execute the eval
    methods in a bad way.
    
    The error count is used only at parsing time - while a yacc parser will perform most of the
    work of analyzing the grammar, it is up to the expression classes to do finer grained error
    checking based on the types of the expressions on either side.  For example bit shift
    operations may only be performed on integers, so any attempt to bit shift a variable which is
    defined as a float would be an error.  As the expressions are parsed recursively, they
    aggregate the error count of all their sub-expressions, thus accumulating a total count at the
    end of the number of errors encountered during parsing.  Only if the whole expression has zero
    errors should it be accepted and used by the appropriate component.

    As an example of an error, take the following: 7 << (2.4 + 3).  The second argument to the bit
    shift operator evalutes to a float, 5.4, following standard C++ rules.  Bit shift operators
    however do not accept floating point arguments.  If we were simply to allow this expression to
    be created, and have the bit shift operator call evalInt(ResolveVariableCb &cb, const void* context)
    on the result of the addition, then we would be truncating the float.  The C++ compiler doesn't
    allow this, and neither do we, we treat it as an error at parse time, and force the user to
    correct what most likely was an error in their formula anyways, and thus avoid the chance of
    calling eval methods at runtime that may lose data.  Should a customer really wish to truncate
    a float, they may always use the ceil or floor functions.

    A subtle point to note regarding conditional expressions using the ternary operator: the
    behaviour when using floating-point literals as the conditional appears undefined in C++.
    A literal value of 0.0 in Visual Studio strangely caused the conditional to evaluate to true,
    but false when compiled on Linux; whereas literal values 0.1 and -0.1 evaluate to false when
    compiled on Linux, but true in Visual Studio.  All conditionals in this file are determined
    only at runtime, so this should not cause us direct problems, but it does mean that when
    writing unit tests to validate the behaviour of our expression classes that there is no valid
    conclusions to be drawn by comparing our behaviour vs the same expression written directly in
    C++ code as the C++ behaviour of an expression such as 0.0 ? 3 : 2 is implementation defined.

    One challenge with parsing formulae using yacc during the normal running of a Blaze server is
    that yacc may bail at any point due to a grammar error, which could potentially leak many
    Expression objects that had been allocated along the way.  Intrusive pointers would seem to
    be a natural solution, however yacc does not allow anything with a constructor (amongst other
    methods) in the union of types, which precludes the EASTL intrusive pointers.  The alternative
    approach that is used here is to cache off a pointer to every expression allocated during
    parsing in a simple vector, and free them should a parsing error occur.  Note that we have to
    be careful not to simply delete these cached pointers, some may contain child expressions
    depending at what point of the parsing we hit our failure case.  Instead, we define a destroy
    method on the root Expression class which all subclasses must implement (while we could provide
    a default implementation which simply calls 'delete this;' this is potentially error-prone as
    it may not be obvious to those adding new expression classes that they need to do the right
    thing).  The destroy method should basically set any expression pointer member variables to
    nullptr before calling 'delete this', thus ensuring that we do not perform any double deletes.
*/
/*************************************************************************************************/

// Main interface that all expression classes inherit from
class Expression
{
    NON_COPYABLE(Expression)
public:
    
    typedef union ExpressionVariableVal
    {
        int64_t intVal;
        float_t floatVal;
        const char8_t* stringVal;
    } ExpressionVariableVal;

    typedef Functor5<const char8_t*, const char8_t*, Blaze::ExpressionValueType, const void*, ExpressionVariableVal&> ResolveVariableCb;

    Expression(Blaze::ExpressionValueType type) : mType(type), mErrorCount(0) {}
    Expression(Blaze::ExpressionValueType type, int32_t errorCount) : mType(type), mErrorCount(errorCount) {}
    virtual ~Expression() {}
    virtual void destroy() = 0;
    virtual int64_t evalInt(ResolveVariableCb &cb, const void* context) const = 0;
    virtual float_t evalFloat(ResolveVariableCb &cb, const void* context) const = 0;
    virtual int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const;

    virtual bool isInt() const { return mType == EXPRESSION_TYPE_INT; }
    virtual bool isFloat() const { return mType == EXPRESSION_TYPE_FLOAT; }
    virtual bool isString() const { return mType == EXPRESSION_TYPE_STRING; }
    virtual int32_t getType() const { return mType; }
    virtual int32_t getErrorCount() const { return mErrorCount; }

    // Const check can be used to tell if eval() can be used without a context.
    virtual bool isConst() const { return false; }

protected:
    Blaze::ExpressionValueType mType;
    int32_t mErrorCount;
};

// Class that all string type expressions return from
class StringExpression : public Expression
{
public:
    StringExpression(int32_t errorCount) : Expression(EXPRESSION_TYPE_STRING, errorCount) {}

    // These methods should never be called, it is up to the parser not to allow formulae
    // to be parsed which would ever results in their being executed
    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override { return 0; }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override { return 0.0; }

    // A quirky trick in that we make evalString abstract once again, overriding the default
    // implementation of Expression, which forces all subclasses of StringExpression
    // to provide a proper implementation, and never accidentally fall back to the default
    // behaviour of Expression which should only be used to convert numeric values to
    // strings
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override = 0;
};

class StringConcatExpression : public StringExpression
{
public:
    StringConcatExpression(Expression* lhs, Expression* rhs)
        : StringExpression(lhs->getErrorCount() + rhs->getErrorCount()), mLhs(lhs), mRhs(rhs) {}

    ~StringConcatExpression() override { delete mLhs; delete mRhs; }
    void destroy() override { mLhs = mRhs = nullptr; delete this; }

    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override
    {
        int32_t len = 0;
        len += mLhs->evalString(cb, context, buf + len, bufLen - len);
        len += mRhs->evalString(cb, context, buf + len, bufLen - len);
        return len;
    }
    bool isConst() const override { return mLhs->isConst() && mRhs->isConst(); }
private:
    Expression* mLhs;
    Expression* mRhs;
};

class ConditionalExpression : public Expression
{
public:
    ConditionalExpression(Expression* cond, Expression* lhs, Expression* rhs)
        : Expression(lhs->isString() || rhs->isString() ? EXPRESSION_TYPE_STRING :
              lhs->isFloat() || rhs->isFloat() ? EXPRESSION_TYPE_FLOAT : EXPRESSION_TYPE_INT,
              lhs->getErrorCount() + rhs->getErrorCount()), mCond(cond), mLhs(lhs), mRhs(rhs)
    {}

    ~ConditionalExpression() override { delete mCond; delete mLhs; delete mRhs; }
    void destroy() override { mCond = mLhs = mRhs = nullptr; delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        if (mCond->isInt())
            return (mCond->evalInt(cb, context) ?
                mLhs->evalInt(cb, context) : mRhs->evalInt(cb, context));
        else
            return (mCond->evalFloat(cb, context) ?
                mLhs->evalInt(cb, context) : mRhs->evalInt(cb, context));
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        if (mCond->isInt())
            return (mCond->evalInt(cb, context) ?
                mLhs->evalFloat(cb, context) : mRhs->evalFloat(cb, context));
        else
            return (mCond->evalFloat(cb, context) ?
                mLhs->evalFloat(cb, context) : mRhs->evalFloat(cb, context));
    }
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override
    {
        if (mCond->isInt())
            return (mCond->evalInt(cb, context) ?
                mLhs->evalString(cb, context, buf, bufLen) : mRhs->evalString(cb, context, buf, bufLen));
        else
            return (mCond->evalFloat(cb, context) ?
                mLhs->evalString(cb, context, buf, bufLen) : mRhs->evalString(cb, context, buf, bufLen));
    }
    bool isConst() const override { return mCond->isConst() && mLhs->isConst() && mRhs->isConst(); }
private:
    Expression* mCond;
    Expression* mLhs;
    Expression* mRhs;
};

// Parent class for all binary (two argument) expressions which accept only integer args.
class BinaryIntExpression : public Expression
{
public:
    BinaryIntExpression(Expression* lhs, Expression* rhs)
        : Expression(EXPRESSION_TYPE_INT, lhs->getErrorCount() + rhs->getErrorCount()), mLhs(lhs), mRhs(rhs)
    {
        // if a string is provided, we'll just treat it as an int. (Which hopefully should work)
        if (mLhs->isFloat() || mRhs->isFloat())
        {
            ++mErrorCount;
            BLAZE_WARN_LOG(Log::UTIL, "[BinaryIntExpression]: Cannot apply binary int expression to float");
        }
    }

    ~BinaryIntExpression() override { delete mLhs; delete mRhs; }
    void destroy() override { mLhs = mRhs = nullptr; delete this; }

    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        // BinaryIntExpression objects only handle integers, so when the result is needed as a float
        // we can simply upcast it
        return (float_t) evalInt(cb, context);
    }
    bool isConst() const override { return mLhs->isConst() && mRhs->isConst(); }
protected:
    Expression* mLhs;
    Expression* mRhs;
};

// Parent class for all binary (two argument) arithmetic expressions which accept numeric args (integer or float)
class BinaryArithmeticExpression : public Expression
{
public:
    BinaryArithmeticExpression(Expression* lhs, Expression* rhs)
        : Expression(lhs->isFloat() || rhs->isFloat() ? EXPRESSION_TYPE_FLOAT : EXPRESSION_TYPE_INT,
                     lhs->getErrorCount() + rhs->getErrorCount()), mLhs(lhs), mRhs(rhs)
    {}

    ~BinaryArithmeticExpression() override { delete mLhs; delete mRhs; }
    void destroy() override { mLhs = mRhs = nullptr; delete this; }
    bool isConst() const override { return mLhs->isConst() && mRhs->isConst(); }
protected:
    Expression* mLhs;
    Expression* mRhs;
};

// Parent class for all binary (two argument) logical expressions which accept numeric args (integer or float)
class BinaryLogicalExpression : public Expression
{
public:
    BinaryLogicalExpression(Expression* lhs, Expression* rhs)
        : Expression(EXPRESSION_TYPE_INT, lhs->getErrorCount() + rhs->getErrorCount()), mLhs(lhs), mRhs(rhs)
    {}

    ~BinaryLogicalExpression() override { delete mLhs; delete mRhs; }
    void destroy() override { mLhs = mRhs = nullptr; delete this; }
    bool isConst() const override { return mLhs->isConst() && mRhs->isConst(); }
protected:
    Expression* mLhs;
    Expression* mRhs;
};

class LogicalOrExpression : public BinaryLogicalExpression
{
public:
    LogicalOrExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isFloat() ? mLhs->evalFloat(cb, context) : mLhs->evalInt(cb, context))
            || (mRhs->isFloat() ? mRhs->evalFloat(cb, context) : mRhs->evalInt(cb, context));
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class LogicalAndExpression : public BinaryLogicalExpression
{
public:
    LogicalAndExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isFloat() ? mLhs->evalFloat(cb, context) : mLhs->evalInt(cb, context))
            && (mRhs->isFloat() ? mRhs->evalFloat(cb, context) : mRhs->evalInt(cb, context));
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class BitwiseOrExpression : public BinaryIntExpression
{
public:
    BitwiseOrExpression(Expression* lhs, Expression* rhs) : BinaryIntExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return mLhs->evalInt(cb, context) | mRhs->evalInt(cb, context);
    }
};

class BitwiseXorExpression : public BinaryIntExpression
{
public:
    BitwiseXorExpression(Expression* lhs, Expression* rhs) : BinaryIntExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return mLhs->evalInt(cb, context) ^ mRhs->evalInt(cb, context);
    }
};

class BitwiseAndExpression : public BinaryIntExpression
{
public:
    BitwiseAndExpression(Expression* lhs, Expression* rhs) : BinaryIntExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return mLhs->evalInt(cb, context) & mRhs->evalInt(cb, context);
    }
};

class EqualExpression : public BinaryLogicalExpression
{
public:
    EqualExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        if (mLhs->isString() || mRhs->isString())
        {
            char8_t lhs[64] = "";
            char8_t rhs[64] = "";
            mLhs->evalString(cb, context, lhs, sizeof(lhs));
            mRhs->evalString(cb, context, rhs, sizeof(rhs));

            return (blaze_stricmp(lhs, rhs) == 0);
        }

        return (mLhs->isFloat() || mRhs->isFloat()) ?
            mLhs->evalFloat(cb, context) == mRhs->evalInt(cb, context)
            : mLhs->evalInt(cb, context) == mRhs->evalInt(cb, context);
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class NotEqualExpression : public BinaryLogicalExpression
{
public:
    NotEqualExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        if (mLhs->isString() || mRhs->isString())
        {
            char8_t lhs[64] = "";
            char8_t rhs[64] = "";
            mLhs->evalString(cb, context, lhs, sizeof(lhs));
            mRhs->evalString(cb, context, rhs, sizeof(rhs));

            return (blaze_stricmp(lhs, rhs) != 0);
        }

        return (mLhs->isFloat() || mRhs->isFloat()) ?
            mLhs->evalFloat(cb, context) != mRhs->evalInt(cb, context)
            : mLhs->evalInt(cb, context) != mRhs->evalInt(cb, context);
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class LessThanExpression : public BinaryLogicalExpression
{
public:
    LessThanExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isFloat() || mRhs->isFloat()) ?
            mLhs->evalFloat(cb, context) < mRhs->evalFloat(cb, context)
            : mLhs->evalInt(cb, context) < mRhs->evalInt(cb, context);
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class GreaterThanExpression : public BinaryLogicalExpression
{
public:
    GreaterThanExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isFloat() || mRhs->isFloat()) ?
            mLhs->evalFloat(cb, context) > mRhs->evalFloat(cb, context)
            : mLhs->evalInt(cb, context) > mRhs->evalInt(cb, context);
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class LessThanEqualExpression : public BinaryLogicalExpression
{
public:
    LessThanEqualExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isFloat() || mRhs->isFloat()) ?
            mLhs->evalFloat(cb, context) <= mRhs->evalFloat(cb, context)
            : mLhs->evalInt(cb, context) <= mRhs->evalInt(cb, context);
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class GreaterThanEqualExpression : public BinaryLogicalExpression
{
public:
    GreaterThanEqualExpression(Expression* lhs, Expression* rhs) : BinaryLogicalExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isFloat() || mRhs->isFloat()) ?
            mLhs->evalFloat(cb, context) >= mRhs->evalFloat(cb, context)
            : mLhs->evalInt(cb, context) >= mRhs->evalInt(cb, context);
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (float_t) evalInt(cb, context);
    }
};

class BitShiftLeftExpression : public BinaryIntExpression
{
public:
    BitShiftLeftExpression(Expression* lhs, Expression* rhs) : BinaryIntExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return mLhs->evalInt(cb, context) << mRhs->evalInt(cb, context);
    }
};

class BitShiftRightExpression : public BinaryIntExpression
{
public:
    BitShiftRightExpression(Expression* lhs, Expression* rhs) : BinaryIntExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return mLhs->evalInt(cb, context) >> mRhs->evalInt(cb, context);
    }
};

class AddExpression : public BinaryArithmeticExpression
{
public:
    AddExpression(Expression* lhs, Expression* rhs) : BinaryArithmeticExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isInt() && mRhs->isInt() ?
            (mLhs->evalInt(cb, context) + mRhs->evalInt(cb, context)) :
            (int64_t) (mLhs->evalFloat(cb, context) + mRhs->evalFloat(cb, context)));
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isInt() && mRhs->isInt() ?
            (float_t) (mLhs->evalInt(cb, context) + mRhs->evalInt(cb, context)) :
            (mLhs->evalFloat(cb, context) + mRhs->evalFloat(cb, context)));
    }
};

class SubtractExpression : public BinaryArithmeticExpression
{
public:
    SubtractExpression(Expression* lhs, Expression* rhs) : BinaryArithmeticExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isInt() && mRhs->isInt() ?
            (mLhs->evalInt(cb, context) - mRhs->evalInt(cb, context)) :
            (int64_t) (mLhs->evalFloat(cb, context) - mRhs->evalFloat(cb, context)));
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isInt() && mRhs->isInt() ?
            (float_t) (mLhs->evalInt(cb, context) - mRhs->evalInt(cb, context)) :
            (mLhs->evalFloat(cb, context) - mRhs->evalFloat(cb, context)));
    }
};

class MultiplyExpression : public BinaryArithmeticExpression
{
public:
    MultiplyExpression(Expression* lhs, Expression* rhs) : BinaryArithmeticExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isInt() && mRhs->isInt() ?
            (mLhs->evalInt(cb, context) * mRhs->evalInt(cb, context)) :
            (int64_t) (mLhs->evalFloat(cb, context) * mRhs->evalFloat(cb, context)));
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return (mLhs->isInt() && mRhs->isInt() ?
            (float_t) (mLhs->evalInt(cb, context) * mRhs->evalInt(cb, context)) :
            (mLhs->evalFloat(cb, context) * mRhs->evalFloat(cb, context)));
    }
};

class DivideExpression : public BinaryArithmeticExpression
{
public:
    DivideExpression(Expression* lhs, Expression* rhs) : BinaryArithmeticExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        if (mRhs->isInt())
        {
            int64_t rhs = mRhs->evalInt(cb, context);
            if (rhs == 0)
            {
                return 0;
            }
            return mLhs->isFloat() ? ((int64_t) (mLhs->evalFloat(cb, context) / rhs)) : (mLhs->evalInt(cb, context) / rhs);
        }
        else
        {
            float_t rhs = mRhs->evalFloat(cb, context);
            if (rhs == 0.0)
            {
                return 0;
            }
            return (int64_t) (mLhs->isFloat() ? (mLhs->evalFloat(cb, context) / rhs) : (mLhs->evalInt(cb, context) / rhs));
        }
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        if (mRhs->isInt())
        {
            int64_t rhs = mRhs->evalInt(cb, context);
            if (rhs == 0)
            {
                return 0;
            }
            return (mLhs->isFloat() ? (mLhs->evalFloat(cb, context) / rhs) : ((float_t) (mLhs->evalInt(cb, context) / rhs)));
        }
        else
        {
            float_t rhs = mRhs->evalFloat(cb, context);
            if (rhs == 0.0)
            {
                return 0;
            }
            return mLhs->isFloat() ? (mLhs->evalFloat(cb, context) / rhs) : (mLhs->evalInt(cb, context) / rhs);
        }
    }
};

class ModExpression : public BinaryIntExpression
{
public:
    ModExpression(Expression* lhs, Expression* rhs) : BinaryIntExpression(lhs, rhs) { }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        int64_t rhs = mRhs->evalInt(cb, context);
        if (rhs == 0)
        {
            return 0;
        }

        return mLhs->evalInt(cb, context) % rhs;
    }
};

class UnaryMinusExpression : public Expression
{
public:
    UnaryMinusExpression(Expression* rhs)
        : Expression(rhs->isFloat() ? EXPRESSION_TYPE_FLOAT : EXPRESSION_TYPE_INT, rhs->getErrorCount()),
          mRhs(rhs)
    {}

    ~UnaryMinusExpression() override { delete mRhs; }
    void destroy() override { mRhs = nullptr; delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override
    {
        return -mRhs->evalInt(cb, context);
    }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override
    {
        return -mRhs->evalFloat(cb, context);
    }
    bool isConst() const override { return mRhs->isConst(); }
private:
    Expression* mRhs;
};

class NotExpression : public Expression
{
public:
    NotExpression(Expression* rhs)
        : Expression(EXPRESSION_TYPE_INT, rhs->getErrorCount()), mRhs(rhs)
    {}

    ~NotExpression() override { delete mRhs; }
    void destroy() override { mRhs = nullptr; delete this; }
    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override { return !mRhs->evalInt(cb, context); }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override { return !mRhs->evalFloat(cb, context); }
    bool isConst() const override { return mRhs->isConst(); }
private:
    Expression* mRhs;
};

class ComplementExpression : public Expression
{
public:
    ComplementExpression(Expression* rhs)
        : Expression(EXPRESSION_TYPE_INT, rhs->getErrorCount()), mRhs(rhs)
    {
        if (mRhs->isFloat())
        {
            ++mErrorCount;
            BLAZE_WARN_LOG(Log::UTIL, "[ComplementExpression]: Cannot take complement of float or string");
        }
    }

    ~ComplementExpression() override { delete mRhs; }
    void destroy() override { mRhs = nullptr; delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override { return ~mRhs->evalInt(cb, context); }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override { return (float_t) (~mRhs->evalInt(cb, context)); }
    bool isConst() const override { return mRhs->isConst(); }
private:
    Expression* mRhs;
};

class IntLiteralExpression : public Expression
{
public:
    IntLiteralExpression(int64_t intVal)
        : Expression(EXPRESSION_TYPE_INT), mIntVal(intVal), mFloatVal((float_t) intVal) {}
    void destroy() override { delete this; }
    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override { return mIntVal; }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override { return mFloatVal; }
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override
    {
        return blaze_snzprintf(buf, bufLen, "%" PRIi64, mIntVal);
    }
    bool isConst() const override { return true; }
private:
    int64_t mIntVal;
    float_t mFloatVal;
};

class FloatLiteralExpression : public Expression
{
public:
    FloatLiteralExpression(float_t floatVal)
        : Expression(EXPRESSION_TYPE_FLOAT), mIntVal((int32_t) floatVal), mFloatVal(floatVal) {}
    void destroy() override { delete this; }
    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override { return mIntVal; }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override { return mFloatVal; }
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override
    {
        return blaze_snzprintf(buf, bufLen, "%f", (double_t) mFloatVal);
    }
    bool isConst() const override { return true; }
private:
    int32_t mIntVal;
    float_t mFloatVal;
};

class StringLiteralExpression : public StringExpression
{
public:
    StringLiteralExpression(char8_t* value)
        : StringExpression(0), mValue(value) {}

    ~StringLiteralExpression() override { BLAZE_FREE(mValue); }
    void destroy() override { delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override;
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override;
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override
    {
        return blaze_snzprintf(buf, bufLen, "%s", mValue);
    }
    bool isConst() const override { return true; }
private:
    char8_t* mValue;
};

// Represents an int variable
class VariableIntExpression : public Expression
{
public:
    VariableIntExpression(char8_t* nameSpace, char8_t* name)
        : Expression(EXPRESSION_TYPE_INT), mNameSpace(nameSpace),  mName(name) { }

    ~VariableIntExpression() override { BLAZE_FREE(mNameSpace); BLAZE_FREE(mName); }
    void destroy() override { delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override;
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override;
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override;
    bool isConst() const override { return false; }
private:
    char8_t* mNameSpace;
    char8_t* mName;
};

// Represents a float variable
class VariableFloatExpression : public Expression
{
public:
    VariableFloatExpression(char8_t* nameSpace, char8_t* name)
        : Expression(EXPRESSION_TYPE_FLOAT), mNameSpace(nameSpace), mName(name) { }

    ~VariableFloatExpression() override { BLAZE_FREE(mNameSpace); BLAZE_FREE(mName); }
    void destroy() override { delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override;
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override;
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override;
    bool isConst() const override { return false; }
private:
    char8_t* mNameSpace;
    char8_t* mName;
};

// Represents a string variable
class VariableStringExpression : public StringExpression
{
public:
    VariableStringExpression(char8_t* nameSpace, char8_t* name)
        : StringExpression(0), mNameSpace(nameSpace), mName(name) { }

    ~VariableStringExpression() override { BLAZE_FREE(mNameSpace); BLAZE_FREE(mName); }
    void destroy() override { delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override;
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override;
    int32_t evalString(ResolveVariableCb &cb, const void* context, char8_t* buf, int32_t bufLen) const override;
    bool isConst() const override { return false; }
private:
    char8_t* mNameSpace;
    char8_t* mName;
};

typedef eastl::list<Expression*> ArgumentList;

// A helper class that contains an EASTL list of expressions,
// used by function expressions to hold their argument list
class ExpressionArgumentList
{
    NON_COPYABLE(ExpressionArgumentList)
public:
    ExpressionArgumentList()
        : mErrorCount(0)
    {
    }

    ExpressionArgumentList(Expression& expression)
        : mErrorCount(expression.getErrorCount())
    {
        mList.push_back(&expression);
    }

    ~ExpressionArgumentList()
    {
        for (ArgumentList::iterator iter = mList.begin(); iter != mList.end(); ++iter)
        {
            delete *iter;
        }
    }
    void destroy()
    {
        mList.clear();
    }

    int32_t getErrorCount() const { return mErrorCount; }

    void addExpression(Expression& expression)
    {
        mErrorCount += expression.getErrorCount();
        mList.push_back(&expression);
    }

    const ArgumentList& getArgumentList() const { return mList; }
    bool isConst() const 
    { 
        for (auto& arg : mList)
        {
            if (arg->isConst() == false)
                return false;
        }
        return true; 
    }
private:
    ArgumentList mList;

    int32_t mErrorCount;
};

// Most classes in this header are short and sweet, and defined in the header so that they get
// in-lined for maximum efficiency.  This class is somewhat large for a header, and perhaps
// should be broken up or moved to a source file at some point.
class FunctionExpression : public Expression
{
public:
    FunctionExpression(char8_t* functionName, ExpressionArgumentList& args);
    ~FunctionExpression() override;
    void destroy() override;
    bool isConst() const override { return mArgs.isConst(); }
protected:
    char8_t* mFunctionName;
    ExpressionArgumentList& mArgs;
};

#define FUNCTION_EPRESSION(funcName)                                                    \
class FunctionExpression##funcName : public FunctionExpression                          \
{                                                                                       \
public:                                                                                 \
    FunctionExpression##funcName(char8_t* functionName, ExpressionArgumentList& args);  \
    virtual ~FunctionExpression##funcName() {}                                          \
    virtual int64_t evalInt(ResolveVariableCb &cb, const void* context) const;          \
    virtual float_t evalFloat(ResolveVariableCb &cb, const void* context) const;        \
};

// Built-in functions:
FUNCTION_EPRESSION(Sqrt);
FUNCTION_EPRESSION(Min);
FUNCTION_EPRESSION(Max);
FUNCTION_EPRESSION(Ceil);
FUNCTION_EPRESSION(Floor);
FUNCTION_EPRESSION(Exp);
FUNCTION_EPRESSION(Log);

#undef FUNCTION_EPRESSION

// Dummy expression used merely to return a positive error count
// out of yacc when an unknown identifier is encountered
class UnknownIdentifierExpression : public Expression
{
public:
    UnknownIdentifierExpression(char8_t* nameSpace, char8_t* name)
        : Expression(EXPRESSION_TYPE_INT, 1), mNameSpace(nameSpace), mName(name)
    {
        BLAZE_WARN_LOG(Log::UTIL, "[UnknownIdentifierExpression]: Variable " << name << " does not exist");
    }

    ~UnknownIdentifierExpression() override { BLAZE_FREE(mNameSpace); BLAZE_FREE(mName); }
    void destroy() override { delete this; }

    int64_t evalInt(ResolveVariableCb &cb, const void* context) const override { return 0; }
    float_t evalFloat(ResolveVariableCb &cb, const void* context) const override { return 0.0; }

private:
    char8_t* mNameSpace;
    char8_t* mName;
};

} // Blaze
#endif // BLAZE_EXPRESSION_H
