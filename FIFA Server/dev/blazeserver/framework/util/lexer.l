D       [0-9]
L       [a-zA-Z_]
H       [a-fA-F0-9]
E       [Ee][+-]?{D}+
FS      (f|F)
IS      (u|U|l|L)*
IDENT   {L}({L}|{D})*

%option c++
%option prefix="blaze"
%option noyywrap
%option batch

%{

#include "framework/blazebase.h"
#include <string.h>
#include "framework/util/expression.h"
#include "framework/util/blazelexer.h"
#include "parser.tab.hpp"

//For some reason the noyyalloc/noyyfree options don't work here
#if ENABLE_BLAZE_MEM_SYSTEM
#define malloc BLAZE_ALLOC
#define realloc BLAZE_REALLOC
#define free BLAZE_FREE
#endif

#ifdef EA_PLATFORM_WINDOWS
/*
 * Disable some warnings on windows compile since they are caused by the code
 * which flex generates which we have no control over.
 */
#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 5033)
#pragma warning(disable: 4242)
#endif

/*
 * Redefine YY_INPUT so that we can read from our string buffer
 * as opposed to reading from a file which lex normally does.
 * We must cast this to a BlazeLexer because calls to YY_INPUT
 * are generated into some lexer base class method implementations.
 */
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r = ((BlazeLexer*) this)->blazeInput(b, ms))

/*
 * As we currently don't need a yyunput function, tell lex not to generate one
 * as it will create compiler warnings about unused functions.
 */
#define YY_NO_UNPUT

/*
 * This tells flex that the main generated method should be part of the BlazeLexer class
 * as opposed to the default which would be to generate it as a member of the parent
 * blazeFlexLexer class.  This allows the lexing code access to the member variables defined 
 * in our blaze derived class.
 */
#define YY_DECL int BlazeLexer::yylex()

/*
 * By redefining YY_DECL above, the flex generated output will no longer contain an implementation
 * for the parent class yylex() method anymore.  It should never be called, but it still needs to be
 * defined in order to compile, so we provide a dummy implementation that just returns 0.
 */
int blazeFlexLexer::yylex() { return 0; }

/*
 * Global variable used by the yacc error function to place a carat symbol
 * under the appropriate character in case of error.
 */
int blazeColumn = 0;

%}

%%

{IDENT}                 { count(); ((YYSTYPE*)blazelval)->string = copy_string(YYText()); return IDENTIFIER_TOKEN; }

0[xX]{H}+{IS}?          { count(); ((YYSTYPE*)blazelval)->ival = strtol(&YYText()[2], nullptr, 16); return(TYPE_INT); }
0{D}+{IS}?              { count(); ((YYSTYPE*)blazelval)->ival = strtol(&YYText()[1], nullptr, 8); return(TYPE_INT); }
{D}+{IS}?               { count(); ((YYSTYPE*)blazelval)->ival = strtol(YYText(), nullptr, 10); return(TYPE_INT); }

{D}+{FS}                |
{D}+"."{D}*{E}?{FS}?    |
{D}*"."{D}+{E}?{FS}?    { count(); ((YYSTYPE*)blazelval)->fval = (float) strtod(YYText(), nullptr); return(TYPE_TDF_FLOAT); }

"'"                     { count(); ((YYSTYPE*)blazelval)->string = copy_string(doString()); return STRING_LITERAL; }

"::"                    { count(); return(SCOPE); }
"("                     { count(); return('('); }
")"                     { count(); return(')'); }
"!"                     { count(); return('!'); }
"~"                     { count(); return('~'); }
"*"                     { count(); return('*'); }
"/"                     { count(); return('/'); }
"%"                     { count(); return('%'); }
"+"                     { count(); return('+'); }
"-"                     { count(); return('-'); }
"<<"                    { count(); return(BSL); }
">>"                    { count(); return(BSR); }
">"                     { count(); return('>'); }
"<"                     { count(); return('<'); }
">="                    { count(); return(GTE); }
"<="                    { count(); return(LTE); }
"=="                    { count(); return(EQ); }
"!="                    { count(); return(NE); }
"&"                     { count(); return('&'); }
"^"                     { count(); return('^'); }
"|"                     { count(); return('|'); }
"&&"                    { count(); return(AND); }
"||"                    { count(); return(OR); }
"?"                     { count(); return('?'); }
":"                     { count(); return(':'); }
","                     { count(); return(','); }
\.                      { count(); return('.'); }

[ \t\v\n\f]             { count(); }
.                       { printf("Blaze Expression lexer skipping unexpected char %c", *YYText()); /* ignore bad characters */ }

%%

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(pop)
#endif
