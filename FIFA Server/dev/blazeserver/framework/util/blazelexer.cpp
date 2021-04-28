/*************************************************************************************************/
/*!
    \file   blazelexer.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class BlazeLexer

    Derives from the core yyBlazeLexer to provide additional lexing utility methods.
    The implementation is mostly borrowed from the type compiler, but turned into class methods
    to avoid clashing with stuff from the global namespace.  Note that each lexer.l file generates
    implementations of some methods on the yyBlazeLexer class which would normally result in
    symbol clashes, so we redefine yyFlexLexer to blazeFlexLexer in order to allow others
    in Blaze to create their own lexers and parsers.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blazebase.h"
#undef yyFlexLexer
#define yyFlexLexer blazeFlexLexer
#include "FlexLexer.h"
#include "framework/util/blazelexer.h"

// Global variable updated by the lexer to track how far we've read into a string, and read
// by the parser's error function to display a carat under the error if needed.  The variable
// is defined in the lexer.l file.
extern int blazeColumn;

/*************************************************************************************************/
/*!
    \brief BlazeLexer

    Construct a new BlazeLexer object to parse a formula from a string.

    \param[in]  buf - the string buffer to be parsed
*/
/*************************************************************************************************/
BlazeLexer::BlazeLexer(const char8_t* buf, void* context, LexResolveTypeCb cb, LexResolveFunctionCb* functionLookupCb)
: yyFlexLexer(), mBuf(buf), mStringLength(0), mStringLiteral(nullptr), mContext(context), mExpression(nullptr), mDependencyContainer(nullptr), mTypeResolveCb(cb), mFunctionResolveCb(functionLookupCb)
{ }

/*************************************************************************************************/
/*!
    \brief ~BlazeLexer

    Destroy and cleanup the memory owned by a blaze lexer.
*/
/*************************************************************************************************/
BlazeLexer::~BlazeLexer()
{
    if (mStringLiteral != nullptr)
    {
        BLAZE_FREE(mStringLiteral);
    }
}

/*************************************************************************************************/
/*!
    \brief pushDependency

    Push a dependency into the caller provided storage (if not null).

    \param[in]  nameSpace - the namespace of the dependency
    \param[in]  name - the name of the dependency
*/
/*************************************************************************************************/
void BlazeLexer::pushDependency(const char8_t* nameSpace, const char8_t* name)
{
    if (mDependencyContainer != nullptr)
    {
        mDependencyContainer->pushDependency(mContext, nameSpace, name);
    }
}

/*! ***********************************************************************/
/*! \brief retreives the type of a particular identifier

    \param[in] nameSpace - the nameSpace of the identifier
    \param[in] name - the name of the identifier
    \return - the type represented by the identifier
***************************************************************************/
Blaze::ExpressionValueType BlazeLexer::getType(const char8_t* nameSpace, const char8_t* name)
{
    Blaze::ExpressionValueType type = Blaze::EXPRESSION_TYPE_NONE;
    mTypeResolveCb(nameSpace, name, mContext, type);
    return type;
}

Blaze::ExpressionValueType BlazeLexer::attemptStringConversion(const char8_t* s, int64_t& outInt, float& outFloat)
{
    // Unlike with the TDF conversions, we don't know what the expected type is, so we just do basic parsing logic:
    // bool
    if (EA::StdC::Stricmp(s, "true") == 0)
    {
        outInt = 1;
        return Blaze::EXPRESSION_TYPE_INT;
    }
    else if (EA::StdC::Stricmp(s, "false") == 0)
    {
        outInt = 0;
        return Blaze::EXPRESSION_TYPE_INT;
    }
    
    // enum/bitfield (single bf value)
    int32_t enumVal = 0;
    const EA::TDF::TypeDescriptionBitfieldMember* bfMember = nullptr;
    const EA::TDF::TypeDescription* typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(s, &bfMember, &enumVal);
    if (typeDesc != nullptr)
    {
        if (typeDesc->getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_ENUM)
        {
            outInt = (int64_t)enumVal;
            return Blaze::EXPRESSION_TYPE_INT;
        }
        else if (typeDesc->getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_BITFIELD && bfMember != nullptr)
        {
            outInt = bfMember->mBitMask;
            return Blaze::EXPRESSION_TYPE_INT;
        }
    }
    
    // TimeValue:
    EA::TDF::TimeValue timeVal;
    if (timeVal.parseTimeAllFormats(s))
    {
        outInt = timeVal.getMicroSeconds();
        return Blaze::EXPRESSION_TYPE_INT;
    }

    // float
    char8_t* strEnd = nullptr;
    if (EA::StdC::Strstr(s, ".") != nullptr)
    {
        float tempFloat = EA::StdC::StrtoF32(s, &strEnd);
        if (strEnd == s + strlen(s))
        {
            outFloat = tempFloat;
            return Blaze::EXPRESSION_TYPE_FLOAT;
        }
    }
    // int
    int64_t tempInt = EA::StdC::StrtoI64(s, &strEnd, 0);
    if (strEnd == s + strlen(s))
    {
        outInt = tempInt;
        return Blaze::EXPRESSION_TYPE_INT;
    }

    return Blaze::EXPRESSION_TYPE_STRING;
}


/*************************************************************************************************/
/*!
    \brief blazeInput

    This method is called via the YY_INPUT macro definition, and copies data from our input
    string buffer into the lexer's internal buffer.

    \param[out]  buf - buffer to be copied into
    \param[in]   max_size - maximum amount of data to copy into buffer

    \return - number of bytes copied
*/
/*************************************************************************************************/
int BlazeLexer::blazeInput(char *buf, int max_size)
{
    int n = (int) strlen(mBuf);
    if (n > max_size)
        n = max_size;

    if (n > 0) {
        memcpy(buf, mBuf, n);
        mBuf += n;
    }
    return n;
}

/*************************************************************************************************/
/*!
    \brief cacheExpression

    This method caches a pointer to every expression allocated during parsing.  This allows us to
    properly free all allocated memory should the parser encounter a grammar error.

    \param[in]  exp - pointer to the expression being allocated

    \return - the input expression
*/
/*************************************************************************************************/
Blaze::Expression* BlazeLexer::cacheExpression(Blaze::Expression* exp)
{
    mExpVector.push_back(exp);
    return exp;
}

/*************************************************************************************************/
/*!
    \brief destroyCachedExpressions

    This method frees every expression allocated during parsing should the parser encounter
    a grammar error.
*/
/*************************************************************************************************/
void BlazeLexer::destroyCachedExpressions()
{
    ExpVector::const_iterator iter = mExpVector.begin();
    ExpVector::const_iterator end = mExpVector.end();
    for ( ; iter != end; ++iter)
    {
        (*iter)->destroy();
    }
    mExpVector.clear();
}

/*************************************************************************************************/
/*!
    \brief count

    Determines the end position of a token being passed to the parser so that a carat symbol
    may be placed under the erroneous token should a parsing error occur.
*/
/*************************************************************************************************/
void BlazeLexer::count()
{
    int i;

    for (i = 0; YYText()[i] != '\0'; i++)
    {
        if (YYText()[i] == '\n')
            blazeColumn = 0;
        else if (YYText()[i] == '\t')
            blazeColumn += 8 - (blazeColumn % 8);
        else
            blazeColumn++;
    }
}

/*************************************************************************************************/
/*!
    \brief doString

    Handle a quoted string literal.  This method skips any initial tabs on the second line of a
    multi-line string literal, and further skips the same number of tab characters on each
    subsequent line.

    \return - a pointer to a newly allocated string literal
*/
/*************************************************************************************************/
char8_t* BlazeLexer::doString()
{
    char8_t c;
    newString();
    
    //Read the first line
    size_t firstLineCount = 0;
    while ((c = (char8_t) yyinput()) != '\'' && c != '\0' && c != -1 && c != '\n')
    {
        putchar(c);
        addToString(c);
        firstLineCount++;
    }
    putchar(c);
    
    if (c == '\'' || c == '\0' || c == -1)
        return mStringLiteral;

    if (c == '\n' && firstLineCount > 0)
        addToString(c);
    
    //eat the first section of white space and count the tabs
    int32_t tabCount = 0;
    while ((c = (char8_t) yyinput()) == '\t') 
    {   
        putchar(c);
        tabCount++; 
    }
    while (c != '\'' && c != '\0' && c != -1)
    {
        putchar(c);
        addToString(c);
        if (c == '\n')
        {
            int32_t tabCount2 = 0;
            while ((c = (char8_t) yyinput()) == '\t' && tabCount2 < tabCount)
            {   
                putchar(c);
                tabCount2++;
            }
        }
        else
            c = (char8_t) yyinput();        
    }

    return mStringLiteral;
}

/*************************************************************************************************/
/*!
    \brief newString

    Allocate some storage for a string literal that will be built up in doString().
*/
/*************************************************************************************************/
void BlazeLexer::newString()
{
    if (mStringLiteral != nullptr)
    {
        BLAZE_FREE(mStringLiteral);
        mStringLength = 0;
    }
    
    mStringLiteral = (char8_t*) BLAZE_ALLOC(128);
    *mStringLiteral = 0;    
    mStringLength = 128;
}

/*************************************************************************************************/
/*!
    \brief addToString

    Adds characters to string literal buffer via calls from doString, and grows the buffer
    if it is not large enough.

    \param[in]   c - character to add to string literal
*/
/*************************************************************************************************/
void BlazeLexer::addToString(char8_t c)
{
    size_t len = strlen(mStringLiteral);
    if (len + 1 > mStringLength - 1)
    {
        mStringLength += 128;
        char8_t *newString = (char8_t *) BLAZE_ALLOC(mStringLength);
        *newString = 0;
        blaze_strnzcpy(newString, mStringLiteral, mStringLength);
        BLAZE_FREE(mStringLiteral);
        mStringLiteral = newString;
    }

    mStringLiteral[len] = c;
    mStringLiteral[len + 1] = '\0';
}

/*************************************************************************************************/
/*!
    \brief copyString

    Make a copy of a string, it will typically then be owned by some object created during
    the course of yacc parsing.

    \param[in]   src - the source string

    \return - a copy of the source string
*/
/*************************************************************************************************/
char8_t* BlazeLexer::copy_string(const char8_t* src) const
{
    return blaze_strdup(src);
}


Blaze::Expression* BlazeLexer::lookupFunction(char8_t* functionName, Blaze::ExpressionArgumentList& args)
{
    // A more efficient solution would be to put all these functions in a map, and do a lookup that way, but then we'd need some constructor helper.
    if (blaze_strcmp("sqrt", functionName) == 0)
    {
        return BLAZE_NEW Blaze::FunctionExpressionSqrt(functionName, args);
    }
    else if (blaze_strcmp("max", functionName) == 0)
    {
        return BLAZE_NEW Blaze::FunctionExpressionMax(functionName, args);
    }
    else if (blaze_strcmp("min", functionName) == 0)
    {
        return BLAZE_NEW Blaze::FunctionExpressionMin(functionName, args);
    }
    else if (blaze_strcmp("ceil", functionName) == 0)
    {
        return BLAZE_NEW Blaze::FunctionExpressionCeil(functionName, args);
    }
    else if (blaze_strcmp("floor", functionName) == 0)
    {
        return BLAZE_NEW Blaze::FunctionExpressionFloor(functionName, args);
    }
    else if (blaze_strcmp("exp", functionName) == 0)
    {
        return BLAZE_NEW Blaze::FunctionExpressionExp(functionName, args);
    }
    else if (blaze_strcmp("log", functionName) == 0)
    {
        return BLAZE_NEW Blaze::FunctionExpressionLog(functionName, args);
    }
    else if (mFunctionResolveCb != nullptr)
    {
        Blaze::Expression* expression = nullptr;
        (*mFunctionResolveCb)(functionName, args, mContext, expression);
        return expression;
    }

    return nullptr;
}