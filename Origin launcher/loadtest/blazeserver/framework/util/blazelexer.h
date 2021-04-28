/*************************************************************************************************/
/*!
    \file blazelexer.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_LEXER_H
#define BLAZE_LEXER_H

/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "framework/callback.h"
#include "framework/util/expression.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
class Expression;

/*
 * As we are using flex 2.5.4, we need to use the C++ option to generate a re-entrant lexer.
 * The default yyFlexLexer class that maintains the lexing state however does not have a member
 * to pass the value over to the yacc parser, so we derive from yyFlexLexer to add this member.
 * We also add a doString method as string parsing needs to call yyinput() which is a protected
 * method of the base class.  Finally we need to override yylex() so that the generated
 * implementation of the method has access to zzlval.
 */
class BlazeLexer : public yyFlexLexer
{
public:
    typedef Blaze::Functor4<const char8_t* /*namepace*/, const char8_t* /*name*/, void* /*context*/, Blaze::ExpressionValueType& /*outType*/> LexResolveTypeCb;
    typedef Blaze::Functor4<char8_t* /*name - owned by function*/,  class Blaze::ExpressionArgumentList& /*args*/, void* /*context*/, Blaze::Expression*& /*outNewExpression*/> LexResolveFunctionCb;


    // The typeLookupCb callback is used to indicate what type should be associated with a given identifier. 
    // The functionLookupCb callback is used to indicate what Expression should be associated with a given identifier and ExpressionArgumentList. 
    BlazeLexer(const char8_t* buf, void* context, LexResolveTypeCb typeLookupCb, LexResolveFunctionCb* functionLookupCb = nullptr);
    ~BlazeLexer() override;

    void* blazelval;

    int yylex() override;
    int blazeInput(char8_t *buf, int max_size);

    void setExpression(Blaze::Expression* expression) { mExpression = expression; }
    Blaze::Expression* getExpression() { return mExpression; }
    Blaze::Expression* cacheExpression(Blaze::Expression* exp);
    void destroyCachedExpressions();
    void setDependencyContainer(Blaze::DependencyContainer* dependencyContainer) { mDependencyContainer = dependencyContainer; }
    void pushDependency(const char8_t* nameSpace, const char8_t* name);
    Blaze::ExpressionValueType getType(const char8_t* nameSpace, const char8_t* name);
    Blaze::ExpressionValueType attemptStringConversion(const char8_t* s, int64_t& outInt, float& outFloat);
    Blaze::Expression* lookupFunction(char8_t* functionName, Blaze::ExpressionArgumentList& args);

private:
    char8_t* doString();
    void count();
    char8_t* copy_string(const char8_t* src) const;
    void newString();
    void addToString(char8_t c);

    const char8_t* mBuf;

    size_t mStringLength;
    char8_t *mStringLiteral;
    void* mContext;

    Blaze::Expression* mExpression;
    Blaze::DependencyContainer* mDependencyContainer;
    LexResolveTypeCb mTypeResolveCb;
    LexResolveFunctionCb* mFunctionResolveCb;

    typedef eastl::vector<Blaze::Expression*> ExpVector;
    ExpVector mExpVector;
};

#endif // BLAZE_LEXER_H
