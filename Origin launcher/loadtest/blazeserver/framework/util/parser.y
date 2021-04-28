%{

#include "framework/blazebase.h"
#include "framework/util/expression.h"
#include <stdio.h>

#undef yyFlexLexer
#define yyFlexLexer blazeFlexLexer
#include "FlexLexer.h"
#include "framework/util/blazelexer.h"

#define YYMALLOC BLAZE_ALLOC
#define YYFREE BLAZE_FREE

#ifdef EA_PLATFORM_WINDOWS
/*
 * Disable some warnings on windows compile since they are caused by the code
 * which bison generates which we have no control over.
 */
#pragma warning(push)
#pragma warning(disable: 4273)
#pragma warning(disable: 4065)
#pragma warning(disable: 4565)
#pragma warning(disable: 4244)
#pragma warning(disable: 4701)
#pragma warning(disable: 4702)
#pragma warning(disable: 4242)
#endif

/*
 * Provide the implementation for the lexing function.  Essentially we just convert
 * what is a static function call into a method invocation on the lexer itself.
 */
static int blazelex(void* blazelval, BlazeLexer* lexer)
{
    lexer->blazelval = blazelval;
    return lexer->yylex();
}

#define LEXER (blaze_flex_lexer)
#define CACHE(x) ((blaze_flex_lexer)->cacheExpression(x))

/*
 * One piece of global information is required to pass the column of an error back
 * from the lexer to the parser because the generated code that calls our error
 * function passes in only a string and no other mechanism to get at the lexer state.
 * We preface the name with blaze in the hopes it won't collide with any other names.
 */
extern int blazeColumn;

/*
 * Provides the implementation of the parsing error function.
 */
static void blazeerror(BlazeLexer* blaze_flex_lexer, const char* s)
{
    fflush(stdout);
    printf("\n%*s\n%*s\n", blazeColumn, "^", blazeColumn, s);
}

/*
 * Keep the parsing code clean by not requiring a namespace qualifier in front of a large
 * number of calls to blaze expression class constructors.
 */
using namespace Blaze;

%}

%name-prefix="blaze"
%pure-parser
%lex-param {BlazeLexer* blaze_flex_lexer}
%parse-param {BlazeLexer* blaze_flex_lexer}

%union
{
    Blaze::Expression* exp;
    Blaze::ExpressionArgumentList* list;
    char8_t* string;
    int32_t ival;
    float_t fval;
}

%token <ival>TYPE_INT
%token <fval>TYPE_TDF_FLOAT
%token <string>STRING_LITERAL
%token <string>IDENTIFIER_TOKEN
%token SCOPE
%token BSL
%token BSR
%token GTE
%token LTE
%token EQ
%token NE
%token AND
%token OR
%token UPLUS
%token UMINUS

%type <exp>expression
%type <exp>conditional_expression
%type <exp>strcat_expression
%type <exp>logical_or_expression
%type <exp>logical_and_expression
%type <exp>bitwise_or_expression
%type <exp>bitwise_xor_expression
%type <exp>bitwise_and_expression
%type <exp>equality_expression
%type <exp>relational_expression
%type <exp>shift_expression
%type <exp>multiplicative_expression
%type <exp>additive_expression
%type <exp>unary_expression
%type <exp>primary_expression
%type <list>expression_argument_list

%start expression

%%

expression
    : conditional_expression { $$ = $1; LEXER->setExpression($1); }
    | error { $$ = nullptr; LEXER->setExpression(nullptr); LEXER->destroyCachedExpressions(); YYABORT; }
    ;

conditional_expression
    : strcat_expression { $$ = $1; }
    | strcat_expression '?' conditional_expression ':' conditional_expression { $$ = CACHE(BLAZE_NEW ConditionalExpression($1, $3, $5)); }
    ;

strcat_expression
    : logical_or_expression { $$ = $1; }
    | strcat_expression '.' logical_or_expression { $$ = CACHE(BLAZE_NEW StringConcatExpression($1, $3)); }
    ;

logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression OR logical_and_expression { $$ = CACHE(BLAZE_NEW LogicalOrExpression($1, $3)); }
    ;

logical_and_expression
    : bitwise_or_expression { $$ = $1; }
    | logical_and_expression AND bitwise_or_expression { $$ = CACHE(BLAZE_NEW LogicalAndExpression($1, $3)); }
    ;

bitwise_or_expression
    : bitwise_xor_expression { $$ = $1; }
    | bitwise_or_expression '|' bitwise_xor_expression { $$ = CACHE(BLAZE_NEW BitwiseOrExpression($1, $3)); }
    ;

bitwise_xor_expression
    : bitwise_and_expression { $$ = $1; }
    | bitwise_xor_expression '^' bitwise_and_expression { $$ = CACHE(BLAZE_NEW BitwiseXorExpression($1, $3)); }
    ;

bitwise_and_expression
    : equality_expression { $$ = $1; }
    | bitwise_and_expression '&' equality_expression { $$ = CACHE(BLAZE_NEW BitwiseAndExpression($1, $3)); }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQ relational_expression { $$ = CACHE(BLAZE_NEW EqualExpression($1, $3)); }
    | equality_expression NE relational_expression { $$ = CACHE(BLAZE_NEW NotEqualExpression($1, $3)); }
    ;

relational_expression
    : shift_expression { $$ = $1; }
    | relational_expression '<' shift_expression { $$ = CACHE(BLAZE_NEW LessThanExpression($1, $3)); }
    | relational_expression '>' shift_expression { $$ = CACHE(BLAZE_NEW GreaterThanExpression($1, $3)); }
    | relational_expression LTE shift_expression { $$ = CACHE(BLAZE_NEW LessThanEqualExpression($1, $3)); }
    | relational_expression GTE shift_expression { $$ = CACHE(BLAZE_NEW GreaterThanEqualExpression($1, $3)); }
    ;

shift_expression
    : additive_expression { $$ = $1; }
    | shift_expression BSL additive_expression { $$ = CACHE(BLAZE_NEW BitShiftLeftExpression($1, $3)); }
    | shift_expression BSR additive_expression { $$ = CACHE(BLAZE_NEW BitShiftRightExpression($1, $3)); }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression { $$ = CACHE(BLAZE_NEW AddExpression($1, $3)); }
    | additive_expression '-' multiplicative_expression { $$ = CACHE(BLAZE_NEW SubtractExpression($1, $3)); }
    ;

multiplicative_expression
    : unary_expression { $$ = $1; }
    | multiplicative_expression '*' unary_expression { $$ = CACHE(BLAZE_NEW MultiplyExpression($1, $3)); }
    | multiplicative_expression '/' unary_expression { $$ = CACHE(BLAZE_NEW DivideExpression($1, $3)); }
    | multiplicative_expression '%' unary_expression { $$ = CACHE(BLAZE_NEW ModExpression($1, $3)); }
    ;

unary_expression
    : primary_expression { $$ = $1; }
    | '+' primary_expression { $$ = $2; }
    | '-' primary_expression { $$ = CACHE(BLAZE_NEW UnaryMinusExpression($2)); }
    | '!' primary_expression { $$ = CACHE(BLAZE_NEW NotExpression($2)); }
    | '~' primary_expression { $$ = CACHE(BLAZE_NEW ComplementExpression($2)); }
    ;

primary_expression
    : IDENTIFIER_TOKEN {
        int32_t type = LEXER->getType(nullptr, $1);
        LEXER->pushDependency(nullptr, $1);
        if (type == Blaze::EXPRESSION_TYPE_INT)
            $$ = CACHE(BLAZE_NEW VariableIntExpression(nullptr, $1));
        else if (type == Blaze::EXPRESSION_TYPE_FLOAT)
            $$ = CACHE(BLAZE_NEW VariableFloatExpression(nullptr, $1));
        else if (type == Blaze::EXPRESSION_TYPE_STRING)
            $$ = CACHE(BLAZE_NEW VariableStringExpression(nullptr, $1));
        else
            $$ = CACHE(BLAZE_NEW UnknownIdentifierExpression(nullptr, $1));
    }
    | IDENTIFIER_TOKEN SCOPE IDENTIFIER_TOKEN {
        int32_t type = LEXER->getType($1, $3);
        LEXER->pushDependency($1, $3);
        if (type == Blaze::EXPRESSION_TYPE_INT)
            $$ = CACHE(BLAZE_NEW VariableIntExpression($1, $3));
        else if (type == Blaze::EXPRESSION_TYPE_FLOAT)
            $$ = CACHE(BLAZE_NEW VariableFloatExpression($1, $3));
        else if (type == Blaze::EXPRESSION_TYPE_STRING)
            $$ = CACHE(BLAZE_NEW VariableStringExpression($1, $3));
        else
            $$ = CACHE(BLAZE_NEW UnknownIdentifierExpression($1, $3));
    }
    | IDENTIFIER_TOKEN '(' ')' { 
        Blaze::Expression* expression = LEXER->lookupFunction($1, *(BLAZE_NEW ExpressionArgumentList));
        if (expression != nullptr)
            $$ = CACHE(expression); 
        else
            $$ = CACHE(BLAZE_NEW UnknownIdentifierExpression(nullptr, $1));
    }
    | IDENTIFIER_TOKEN '(' expression_argument_list ')' { 
        Blaze::Expression* expression = LEXER->lookupFunction($1, *$3);
        if (expression != nullptr)
            $$ = CACHE(expression); 
        else
            $$ = CACHE(BLAZE_NEW UnknownIdentifierExpression(nullptr, $1));
    }
    | TYPE_INT { $$ = CACHE(BLAZE_NEW IntLiteralExpression($1)); }
    | TYPE_TDF_FLOAT { $$ = CACHE(BLAZE_NEW FloatLiteralExpression($1)); }
    | STRING_LITERAL { 
        int64_t tempInt = 0;
        float_t tempFloat = 0;
        int32_t type = LEXER->attemptStringConversion($1, tempInt, tempFloat);
        if (type == Blaze::EXPRESSION_TYPE_STRING)
            $$ = CACHE(BLAZE_NEW StringLiteralExpression($1));
        else
        {   // Free the string that was allocated on the by the lexer:
            BLAZE_FREE($1);
            if (type == Blaze::EXPRESSION_TYPE_INT)
                $$ = CACHE(BLAZE_NEW IntLiteralExpression(tempInt));
            else if (type == Blaze::EXPRESSION_TYPE_FLOAT)
                $$ = CACHE(BLAZE_NEW FloatLiteralExpression(tempFloat));
        }
    }
    | '(' expression ')' { $$ = $2; }
    ;

expression_argument_list
    : expression { $$ = BLAZE_NEW ExpressionArgumentList(*$1); }
    | expression_argument_list ',' expression { ($1)->addExpression(*$3); $$ = $1; }
    ;

%%

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(pop)
#endif
