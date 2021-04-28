%{
//lint -w0

#include "framework/blazebase.h"
#include "framework/util/shared/blazestring.h"
#include "EABase/eabase.h"
#include "EASTL/string.h"
#include "EASTL/map.h"
#include "EASTL/vector.h"
#include <stdio.h>
#include <vector>

#undef yyFlexLexer
#define yyFlexLexer configppFlexLexer
#include "FlexLexer.h"
#include "framework/config/configpplexer.h"

#define YYLEX_PARAM lexer

#ifdef EA_PLATFORM_WINDOWS
/* Disable some warnings on windows compile since they are caused by the code which bison generates
 * which we have no control over.
 */
#pragma warning(push)
#pragma warning(disable: 4273)
#pragma warning(disable: 4065)
#pragma warning(disable: 4565)
#pragma warning(disable: 4244)
#pragma warning(disable: 4701)
#pragma warning(disable: 4702)
#pragma warning(disable: 4706)
#pragma warning(disable: 4242)

/* Linux and Windows use different names for 64-bit strtol() functions */
#define strtoull _strtoui64
#define strtoll _strtoi64
#endif

#define YYERROR_VERBOSE 1

#define YYMALLOC BLAZE_ALLOC
#define YYFREE BLAZE_FREE

typedef eastl::map<eastl::string, eastl::string> PredefMap;

/*
 * Provide the implementation for the lexing function.  Essentially we just convert
 * what is a static function call into a method invocation on the lexer itself.
 */
static int configpplex(void* configpplval, Blaze::ConfigPPLexer* lexer)
{
    lexer->setLVal(configpplval);
    return lexer->yylex();
}

static void configpperror(Blaze::ConfigPPLexer *lexer, const char8_t *, const char8_t* msg)
{
    lexer->printPreprocessError(msg);
}


%}

%name-prefix="configpp"
%pure-parser

%union
{
    bool boolean;
    char8_t* string;   
    int64_t ival;
}

%token <string>IDENTIFIER_TOKEN
%token <string>STRING_LITERAL
%token <string>DEFINE_TOKEN
%token <ival>INTEGER
%token DEFINE
%token UNDEFINE
%token IF
%token IFDEF
%token IFNDEF
%token ELSE
%token ELIF
%token ENDIF
%token INCLUDE
%token INCLUDE_VERBATIM
%token DEFINED
%token NOT EQ NEQ AND OR
%token USER_ERROR
%token NEWLINE
%type<ival> if_eval
%type<ival> elif_eval
%type<ival> expression
%type<string> any_value
%type<string> define_expression
%type<ival> int_define_expression
%type<ival> int_define_expression_inner

%left AND OR
%left EQ NEQ 
%right NOT

%start statement_list_or_empty

%initial-action
 {
    //Push the first file
    if (!lexer->configPushFile(initFile))
        YYABORT;
    
 }
 
 %lex-param {Blaze::ConfigPPLexer *lexer }
 %parse-param {Blaze::ConfigPPLexer *lexer }
 %parse-param {const char8_t *initFile}
 
%%

statement_list_or_empty
    :
    | statement_list
    ;

statement_list
    : statement
    | statement_list statement
    ;

statement
    : include_file
    | token_define
    | token_undefine
    | ifdef_block    
    | user_error
    ;

include_file
    : INCLUDE STRING_LITERAL NEWLINE {
        if (lexer->isCurrentBranchActive())
        {
            if (!lexer->configPushFile($2.c_str()))
            {               
                YYERROR;
            }         
        }
    }
    | INCLUDE_VERBATIM STRING_LITERAL NEWLINE {
        if (lexer->isCurrentBranchActive())
        {
            if (!lexer->readVerbatimFile($2.c_str()))
            {
                YYERROR;
            }
        }
    }
    ;
    
ifdef_block
    : if_eval 
    {  
        bool parentTrue = lexer->isCurrentBranchActive();
        lexer->getIfBlockStack().push_back();
        lexer->getIfBlockStack().back().setCurrent($1 != 0);
        lexer->getIfBlockStack().back().setParentTrue(parentTrue);
        
    } 
    statement_list_or_empty
    else_block
    ENDIF NEWLINE
    {
        lexer->getIfBlockStack().pop_back();
    }
    ;
    
else_block
    : elif_block_list 
    | elif_block_list
      ELSE NEWLINE
      {
          lexer->getIfBlockStack().back().setCurrentIfNeverTrue(true);
      }
      statement_list_or_empty
    ;
    
elif_block_list
    : 
    | elif_block 
    | elif_block_list elif_block
    ;

elif_block
    : elif_eval
      {
          lexer->getIfBlockStack().back().setCurrentIfNeverTrue($1 != 0);
      }
      statement_list_or_empty
    ;
    

if_eval
    : IFDEF IDENTIFIER_TOKEN NEWLINE { $$ = (lexer->getDefineMap().find($2) != lexer->getDefineMap().end()); } 
    | IFNDEF IDENTIFIER_TOKEN NEWLINE { $$ = (lexer->getDefineMap().find($2) == lexer->getDefineMap().end());}
    | IF expression NEWLINE { $$ = $2; } 
    ;

elif_eval
    : prepare_for_elif_eval ELIF expression NEWLINE { $$ = $3; }        
    ;

prepare_for_elif_eval
    : 
    {
        //in case of #elif blocks we need to consider the current branch true (until proven otherwise)
        //in order for the expression to be correctly evaluated.
        //But only if the parent is true (because otherwise the branch will always be inactive anyway)
        //and the #if statement was evaluated as false.

        if (lexer->getIfBlockStack().back().mParentTrue && !lexer->isCurrentBranchActive())
        {
            //we don't use the setCurrent method because we do not want to modify mEverTrue. This will be updated after the actual evaluation of the expression is made.

            lexer->getIfBlockStack().back().mCurrentTrue = true;
        }
    }
    ;

expression
    : DEFINED '(' IDENTIFIER_TOKEN ')' { $$ = (lexer->getDefineMap().find($3) != lexer->getDefineMap().end()); }
    | '(' expression ')' { $$ = $2; }
    | NOT expression { $$ = !$2; }
    | expression AND expression { $$ = ($1 && $3); }
    | expression OR expression { $$ = ($1 || $3); }
    | any_value EQ any_value { $$ = $1 == $3; }
    | any_value NEQ any_value { $$ = !($1 == $3); }
    ;
    
token_define
    : DEFINE IDENTIFIER_TOKEN define_expression NEWLINE
    {
        if (lexer->isCurrentBranchActive())
            lexer->getDefineMap()[$2] = $3;
    }
    ;
    
define_expression
    : int_define_expression
    {        
        char8_t stroutput[1024];
        blaze_snzprintf(stroutput, sizeof(stroutput), "%" PRId64, (int64_t)$1);
        $$ = stroutput;
    } 
    | any_value
    {
        $$ = $1;
    }
    ;
       
int_define_expression
    : '(' int_define_expression_inner ')' { $$ = $2; }
    | '+' int_define_expression_inner { $$ = $2; }
    | '-' int_define_expression_inner { $$ = -$2; }
    | int_define_expression_inner '+' int_define_expression_inner { $$ = $1 + $3; }
    | int_define_expression_inner '-' int_define_expression_inner { $$ = $1 - $3; }
    | int_define_expression_inner '*' int_define_expression_inner { $$ = $1 * $3; }
    | int_define_expression_inner '/' int_define_expression_inner { $$ = $1 / $3; }
    | INTEGER { $$ = $1; }
    ;
    
//We need to define this because "IDENTIFIER_TOKEN" can't be a leaf of a define expression
int_define_expression_inner
    : int_define_expression { $$ = $1; }
    | IDENTIFIER_TOKEN 
    {    
        PredefMap::const_iterator itr = lexer->getDefineMap().find($1);
        if (itr != lexer->getDefineMap().end())
        {
            $$ = strtoll(itr->second.c_str(), nullptr, 10); 
        }
        else
        {
            lexer->printPreprocessError("Illegal use of undefined preprocessor variable.");
            YYERROR;
        }            
    }
    ;


    
          
token_undefine
    : UNDEFINE IDENTIFIER_TOKEN NEWLINE
    {
        if (lexer->isCurrentBranchActive())
            lexer->getDefineMap().erase($2);
    }
    
user_error
    : USER_ERROR
    {
        YYERROR;
    }
    ;
   
any_value
    : IDENTIFIER_TOKEN { 
        if (lexer->isCurrentBranchActive())
        {
            PredefMap::const_iterator itr = lexer->getDefineMap().find($1);
            if (itr != lexer->getDefineMap().end())
            {
                $$ = (char8_t *) itr->second.c_str(); 
            }
            else
            {
                char msg[1024];
                blaze_snzprintf(msg, sizeof(msg), "Illegal use of undefined preprocessor variable %s.", $1.c_str());
                lexer->printPreprocessError(msg);
                YYERROR;
            }
        }
        else
        {
            //the value returned when the branch is not active does not really matter since text is written to output only when the branch is active
            //also, defines are added to the defineMap only if the current branch is active.
            $$ = "";
        }            
    }
    | STRING_LITERAL { $$ = $1; }    
    ;


    
%%

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(pop)
#endif
