%{

//lint -w0

#include "framework/blazebase.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "EABase/eabase.h"
#include "EASTL/string.h"
#include <stdio.h>
#include <vector>

extern void BlazePrintConfigErrorLog(const char8_t* message);

#undef yyFlexLexer
#define yyFlexLexer configFlexLexer
#include "FlexLexer.h"
#include "framework/config/configlexer.h"

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
#pragma warning(disable: 4702)
#pragma warning(disable: 4706)
#pragma warning(disable: 4242)
#endif

#define YYERROR_VERBOSE 1

#define YYMALLOC BLAZE_ALLOC
#define YYFREE BLAZE_FREE

static int configlex(void* configlval, Blaze::ConfigLexer* lexer)
{
    lexer->setLVal(configlval);
    return lexer->yylex();
}

static void configerror(Blaze::ConfigLexer *lexer, const char8_t* msg)
{
    lexer->printConfigError(msg);
}

%}

%name-prefix="config"
%pure-parser

%union
{
    eastl::string string;   
}

%token <string>IDENTIFIER_TOKEN
%token <string>STRING_LITERAL
%token <string>CHAR_LITERAL
%token <string>VALUE_TOKEN
%token <string>STRING_NUMERIC
%token <string>STRING_BOOL
%token EQ
%token DEQ
%token VERBATIM
%token EOF_TOKEN
%token NEWLINE_TOKEN
%type<string> literal_string
%type<string> non_literal_string
%type<string> any_string
%type<string> verbatim_list
%type<string> verbatim_item


%start top_map_contents

%parse-param {Blaze::ConfigLexer *lexer}
%lex-param {Blaze::ConfigLexer* lexer}

%%

top_map_contents
    : map_contents ws
    ;

map_content_item
    //This is to provide backwards compatibility to unquoted strings that have a ":".  When we 
    //start to enforce strict quoting, this code should be included in whatever is deprecated as
    //when the strings are quoted this code won't be hit.  Pairs are only used in sequences currently
    //so forcing them to strings won't affect anything else.
    : any_string EQ any_string ':' any_string 
        { 
            if (lexer->getAllowUnquotedStrings())
            {
                eastl::string val;
                val.append_sprintf("%s:%s", $3.c_str(), $5.c_str());
                if (lexer->getCurrentMap()->addValue($1.c_str(), val.c_str()) == nullptr)
                {
                    char8_t buf[256];
                    blaze_snzprintf(buf, sizeof(buf), "Duplicate key: %s", $1.c_str());
                    configerror(lexer, buf);
                    YYERROR;
                }
            }
            else
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Non-quoted string [%s:%s] is not allowed for map r-values.", $3.c_str(), $5.c_str());
                configerror(lexer, buf);
                YYERROR;
            }
        }
    | any_string EQ literal_string 
        { 
            if (lexer->getCurrentMap()->addValue($1.c_str(), $3.c_str()) == nullptr)
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Duplicate key: %s", $1.c_str());
                configerror(lexer, buf);
                YYERROR;
            }
        }
    | any_string EQ non_literal_string
        {
            if (lexer->getAllowUnquotedStrings())
            {
                if (lexer->getCurrentMap()->addValue($1.c_str(), $3.c_str()) == nullptr)
                {
                    char8_t buf[256];
                    blaze_snzprintf(buf, sizeof(buf), "Duplicate key: %s", $1.c_str());
                    configerror(lexer, buf);
                    YYERROR;
                }
            }
            else
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Non-quoted string [%s] is not allowed for map r-values.", $3.c_str());
                configerror(lexer, buf);
                YYERROR;
            }
        }
    | any_string EQ '{'  
        { 
            Blaze::ConfigMap *map = lexer->getCurrentMap()->addMap($1.c_str());
            
            if (map != nullptr)
                lexer->pushMap(map); 
            else
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Duplicate key: %s", $1.c_str());
                configerror(lexer, buf);
                YYERROR;
            }
        } ws map_contents  ws '}' { lexer->pop(); }  
    | any_string EQ '[' 
        {
            Blaze::ConfigSequence *sequence = lexer->getCurrentMap()->addSequence($1.c_str());
            if (sequence != nullptr)
                lexer->pushSequence(sequence); 
            else
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Duplicate key: %s", $1.c_str());
                configerror(lexer, buf);
                YYERROR;
            }
        } ws sequence_contents ws ']' { lexer->pop(); } 
    | any_string DEQ IDENTIFIER_TOKEN ws verbatim_list ws IDENTIFIER_TOKEN 
        { 
            if (lexer->getCurrentMap()->addValue($1.c_str(), $5.c_str()) == nullptr)
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Duplicate key: %s", $1.c_str());
                configerror(lexer, buf);
                YYERROR;
            }
        }
    ;

verbatim_item
    : VERBATIM STRING_LITERAL
        {
            $$ = $2;
        }
    ;

verbatim_list
    : verbatim_item
        {
            $$ = $1;
        }
    | verbatim_list ws verbatim_item
        {
            $$ += $3;
        }
    ;

map_contents
    : 
    | map_content_item 
    | map_contents separator map_content_item 
    ;
    
sequence_contents
    :     
    | sequence_content_item 
    | sequence_contents separator sequence_content_item 
    ;
    
sequence_content_item
    : any_string ':' any_string { lexer->getCurrentSequence()->addPair($1.c_str(), ':', $3.c_str()); }
    | literal_string { lexer->getCurrentSequence()->addValue($1.c_str()); }
    | non_literal_string 
        { 
            if (lexer->getAllowUnquotedStrings())
            {
                lexer->getCurrentSequence()->addValue($1.c_str()); 
            }
            else
            {
                char8_t buf[256];
                blaze_snzprintf(buf, sizeof(buf), "Non-quoted string [%s] is not allowed for sequence items.", $1.c_str());
                configerror(lexer, buf);
                YYERROR;
            }
        }
    | '['  { lexer->pushSequence(lexer->getCurrentSequence()->addSequence()); } ws sequence_contents ws ']' { lexer->pop(); }
    | '{' { lexer->pushMap(lexer->getCurrentSequence()->addMap()); } ws map_contents ws '}' { lexer->pop(); }
    ;
    
literal_string
    : STRING_LITERAL { $$ = $1; }
    | CHAR_LITERAL { $$ = $1; }
    | STRING_NUMERIC { $$ = $1; }
    | STRING_BOOL { $$ = $1; }
    ;
        
non_literal_string
    : IDENTIFIER_TOKEN { $$ = $1; }
    | VALUE_TOKEN { $$ = $1; }
    ;
    
any_string
    : literal_string
    | non_literal_string
    ;
    
separator 
    : ws
    ;

ws
    :
    | whitespace_list
    ;

whitespace_list
    : whitespace_token 
    | whitespace_list whitespace_token
    ;
    
whitespace_token
    : NEWLINE_TOKEN
    | EOF_TOKEN
    ;
    
%%

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(pop)
#endif
