D           [0-9]
O           [0-7]
L           [a-zA-Z_]
P           [\./\-\*\?]
H           [a-fA-F0-9]
E           [Ee][+-]?{D}+
FS          (f|F|l|L)
IS          (u|U|l|L)*
IDENT       {L}({L}|{D}|{P})*
VAL       ({L}|{D}|{P})*
INTEGER     [+-]?((0[xX]{H}+)|({D}+)){IS}?
FLOAT       [+-]?((0[xX]{H}+\.{H}*)|(0[xX]\.{H}+)|({D}+\.{D}*)|(\.{D}+)){FS}?
EXP         [+-]?(({D}+)|({D}+\.{D}*)|(\.{D}+)){E}

%option prefix="config"
%option noyywrap
%option c++
%option batch

%{
//lint -w0

#include "framework/blazebase.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/config/configlexer.h"
#include "EABase/eabase.h"
#include "EASTL/string.h"
#include "EAIO/EAFileUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "configparser.tab.hpp"

extern void BlazePrintConfigErrorLog(const char8_t* message);

//For some reason the noyyalloc/noyyfree options don't work here
#if ENABLE_BLAZE_MEM_SYSTEM
#define malloc BLAZE_ALLOC
#define realloc BLAZE_REALLOC
#define free BLAZE_FREE
#endif


#ifdef EA_PLATFORM_WINDOWS
/* Disable some warnings on windows compile since they are caused by the code which flex generates
 * which we have no control over.
 */
#pragma warning(push)
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 5033)
#pragma warning(disable: 4242)

/* Linux and Windows use different names for 64-bit strtol() functions */
#define strtoull _strtoui64
#define strtoll _strtoi64
#endif

/*
 * This tells flex that the main generated method should be part of the BlazeLexer class
 * as opposed to the default which would be to generate it as a member of the parent
 * blazeFlexLexer class.  This allows the lexing code access to the member variables defined
 * in our blaze derived class.
 */
#define YY_DECL int Blaze::ConfigLexer::yylex()

/*
 * We override the YY_INPUT to return 0 in case the input is bad. The LexerError
 * will take care of informing Blaze that the lexer has failed.
 */
#define YY_INPUT(buf,result,max_size) \
    if ( (result = LexerInput( (char *) buf, max_size )) < 0 ) \
    { \
        result = 0; \
        YY_FATAL_ERROR( "input in flex scanner failed" ); \
    }

/*
 * By redefining YY_DECL above, the flex generated output will no longer contain an implementation
 * for the parent class yylex() method anymore.  It should never be called, but it still needs to be
 * defined in order to compile, so we provide a dummy implementation that just returns 0.
 */
int configFlexLexer::yylex() { return 0; }

%}

%x STRING
%x INFO
%x CHARACTER
%x VERBATIM_HEAD
%x VERBATIM_BODY

%%

<STRING>\"          { BEGIN mLastState; count(); setReturnString(mCurrentStringLiteral.c_str()); return(STRING_LITERAL);}
<STRING>\n          { count(); printConfigError("Newline in string constant."); yyterminate(); }
<STRING>"\\n"       { count(); processNewlineCharacters(false); }
<STRING>"\\\\n"     { count(); processNewlineCharacters(true); }
<STRING>\\\"        { count(); mCurrentStringLiteral.append("\""); }
<STRING>\\\\        { count(); mCurrentStringLiteral.append("\\"); }
<STRING>.           { count(); mCurrentStringLiteral.append(yytext); }

<CHARACTER>\'       { BEGIN mLastState; count(); setReturnString(mCurrentStringLiteral.c_str()); return(CHAR_LITERAL);}
<CHARACTER>\\\'        { count(); mCurrentStringLiteral.append("'"); }
<CHARACTER>\\\\        { count(); mCurrentStringLiteral.append("\\"); }
<CHARACTER>.        { count(); mCurrentStringLiteral.append(yytext); }

<VERBATIM_HEAD>{INTEGER}:  { BEGIN VERBATIM_BODY; count(); blaze_str2int(yytext, &mVerbatimLength); mVerbatimCount = 0;}
<VERBATIM_HEAD>.|\n        { count(); printConfigError("Unexpected verbatim block length."); yyterminate(); }
<VERBATIM_BODY>.|\n        { count(); mCurrentStringLiteral.append(yytext); if (++mVerbatimCount == mVerbatimLength) { BEGIN mLastState; setReturnString(mCurrentStringLiteral.c_str()); return (STRING_LITERAL); } }

"{"                 { count(); return('{'); }
"}"                 { count(); return('}'); }
"["                 { count(); return('['); }
"]"                 { count(); return(']'); }
"("                 { count(); return('['); }
")"                 { count(); return(']'); }
"="                 { count(); return(EQ); }
"=="                { count(); return(DEQ); }
":"                 { count(); return(':'); }
\"                  { count(); mLastState = YY_START; mCurrentStringLiteral.clear(); BEGIN STRING; }
\'                  { count(); mLastState = YY_START; mCurrentStringLiteral.clear(); BEGIN CHARACTER; }
"#verbatim"         { count(); mLastState = YY_START; mCurrentStringLiteral.clear(); BEGIN VERBATIM_HEAD; return (VERBATIM); }
^@EOF\n             { count(); return (EOF_TOKEN);}
^@LINE.*\n          { handleLineInfo(yytext);}

{INTEGER}           { count(); setReturnString(yytext); return(STRING_NUMERIC); }
{FLOAT}             { count(); setReturnString(yytext); return(STRING_NUMERIC); }
{EXP}               { count(); setReturnString(yytext); return(STRING_NUMERIC); }
true                { count(); setReturnString(yytext); return(STRING_BOOL); }
false               { count(); setReturnString(yytext); return(STRING_BOOL); }

{IDENT}             { count(); setReturnString(yytext); return(IDENTIFIER_TOKEN); }
{VAL}             { count(); setReturnString(yytext); return(VALUE_TOKEN); }

<<EOF>>             { yyterminate(); }
\n                  { count(); return NEWLINE_TOKEN; }
[ \t\v\f\r,;]       { count(); }
.                   { count();  printConfigError("Illegal character!"); yyterminate();}

%%

namespace Blaze
{

void ConfigLexer::count()
{
    for (int i = 0; yytext[i] != '\0'; i++)
    {
        if (yytext[i] == '\n')
        {
            mCurrentColumn = 0;
            mCurrentLine++;
        }
        else
        {
            mCurrentColumn++;
        }
    }
}

void ConfigLexer::processNewlineCharacters(bool escaped)
{
    if (mStringEscapes)
    {
        if (escaped)
        {
            mCurrentStringLiteral.append("\\n");
        }
        else
        {
            mCurrentStringLiteral.append("\n");
        }
    }
    else
    {
        mCurrentStringLiteral.append(yytext);
    }
}

void ConfigLexer::printConfigWarn(const char8_t* msg)
{
    fflush(stdout);

    eastl::string errmsg(eastl::string::CtorSprintf(), "Warning parsing config at %s(%" PRIu32 ",%" PRIu32 "): %s", mCurrentParseFile.c_str(), mCurrentLine, mCurrentColumn, msg);
    BlazePrintConfigErrorLog(errmsg.c_str());
}

void ConfigLexer::printConfigError(const char8_t* msg)
{
    fflush(stdout);

    eastl::string errmsg(eastl::string::CtorSprintf(), "Error parsing config at %s(%" PRIu32 ",%" PRIu32 "): %s", mCurrentParseFile.c_str(), mCurrentLine, mCurrentColumn, msg);
    BlazePrintConfigErrorLog(errmsg.c_str());
}

void ConfigLexer::LexerError(const char8_t* msg)
{
    printConfigError(msg);
    mHasHitError = true;
}

void ConfigLexer::handleLineInfo(const char8_t *txt)
{
    //extract the filename and linenumber
    char8_t lineInfo[1024];
    blaze_strnzcpy(lineInfo, txt, sizeof(lineInfo));
    char8_t *fileName = strchr(lineInfo, ':');
    ++fileName;
    char8_t *lineNumber = strchr(fileName, ':');
    *lineNumber = '\0';
    ++lineNumber;

    mCurrentParseFile = fileName;
    blaze_str2int(lineNumber, &mCurrentLine);
    mCurrentColumn = 0;
}



void ConfigLexer::pushMap(Blaze::ConfigMap *map)
{
    mCollectionStack.push_back();
    mCollectionStack.back().mMap = map;
    mCollectionStack.back().mIsMap = true;
    mCollectionStack.back().mCollectionFile = mCurrentParseFile;
    mCollectionStack.back().mStartLine = mCurrentLine;
    mCurrentMap = map;
    mCurrentSequence = nullptr;
}

void ConfigLexer::pushSequence(Blaze::ConfigSequence *seq)
{
    mCollectionStack.push_back();
    mCollectionStack.back().mSequence = seq;
    mCollectionStack.back().mIsMap = false;
    mCollectionStack.back().mCollectionFile = mCurrentParseFile;
    mCollectionStack.back().mStartLine = mCurrentLine;
    mCurrentMap = nullptr;
    mCurrentSequence = seq;
}

void ConfigLexer::pop()
{
    if (mCurrentParseFile != mCollectionStack.back().mCollectionFile)
    {
        eastl::string errmsg(eastl::string::CtorSprintf(),"Current collection opened in %s:%" PRIu32 " but closed in %s:%" PRIu32 "!  This could mean a sequence or map in this file was not properly closed.",
            mCollectionStack.back().mCollectionFile.c_str(), mCollectionStack.back().mStartLine, mCurrentParseFile.c_str(), mCurrentLine);
        BlazePrintConfigErrorLog(errmsg.c_str());
    }


    mCollectionStack.pop_back();
    if (mCollectionStack.back().mIsMap)
    {
        mCurrentMap = mCollectionStack.back().mMap;
        mCurrentSequence = nullptr;
    }
    else
    {
        mCurrentSequence = mCollectionStack.back().mSequence;
        mCurrentMap = nullptr;
    }
}

void ConfigLexer::createLexBuffer(eastl::string &str)
{
BLAZE_PUSH_ALLOC_OVERRIDE(MEM_GROUP_FRAMEWORK_CONFIG, "ConfigStream");
    std::string stdstr(str.c_str());
    mLexStream = BLAZE_NEW_CONFIG std::stringstream(stdstr);
BLAZE_POP_ALLOC_OVERRIDE();

    mLexBuffer = yy_create_buffer(mLexStream, YY_BUF_SIZE);
    yy_switch_to_buffer((yy_buffer_state *) mLexBuffer);
}

void ConfigLexer::destroyLexBuffer()
{
    yy_delete_buffer((YY_BUFFER_STATE) mLexBuffer);
    delete mLexStream;
}

void ConfigLexer::setReturnString(const char8_t *str)
{
    //blaze_strnzcpy(((YYSTYPE*)configlval)->string, str, sizeof(((YYSTYPE*)configlval)->string));
    ((YYSTYPE*) configlval)->string = str;
}

}

Blaze::ConfigLexer *createConfigLexer(eastl::string &buf, Blaze::ConfigMap &rootMap)
{
    return BLAZE_NEW_CONFIG Blaze::ConfigLexer(buf, rootMap);
}


void destroyConfigLexer(Blaze::ConfigLexer* lexer)
{
    delete lexer;
}

void lexerDisableStringEscapes(Blaze::ConfigLexer *lexer)
{
    lexer->disableStringEscapes();
}

void lexerSetAllowUnquotedStrings(Blaze::ConfigLexer *lexer)
{
    lexer->setAllowUnquotedStrings();
}

bool validateLexer(Blaze::ConfigLexer *lexer)
{
    return !(lexer->hitError());
}

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(pop)
#endif
