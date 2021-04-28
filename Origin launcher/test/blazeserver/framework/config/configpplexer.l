D            [0-9]
O           [0-7]
L            [a-zA-Z_]
H            [a-fA-F0-9]
E            [Ee][+-]?{D}+
FS            (f|F|l|L)
IS            (u|U|l|L)*
IDENT        {L}({L}|{D})*


%option prefix="configpp"
%option noyywrap
%option c++
%option batch

%{
//lint -w0

#include "framework/blazebase.h"
#include "framework/util/shared/blazestring.h"
#include "framework/config/configpplexer.h"
#include "EABase/eabase.h"
#include "EASTL/string.h"
#include "EAIO/EAFileUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include "configppparser.tab.hpp"

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
#define YY_DECL int Blaze::ConfigPPLexer::yylex()

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
int configppFlexLexer::yylex() { return 0; }

%}

%x DIRECTIVE
%x DIRECTIVESTRING
%x NORMALSTRING
%s ERROR_STATE

%%

<ERROR_STATE>\n   { count(); BEGIN 0; if (doPreprocessError()) return USER_ERROR; }
<ERROR_STATE>.    { count(); writeToLine(yytext); }

<*>"#"{L}({L}|{D})*"#"    { if (isCurrentBranchActive()) {if (!doDefineToken(yytext)) yyterminate(); }}





"/*"            { comment(); }
"//"            { cppcomment(); }
<DIRECTIVE>"/*"            { comment(); }
<DIRECTIVE>"//"            { cppcomment(); }

"#if"        { count(); BEGIN DIRECTIVE; return(IF); }
"#undef"    { count(); BEGIN DIRECTIVE;  return(UNDEFINE); }
"#ifdef"    { count(); BEGIN DIRECTIVE;  return(IFDEF); }
"#ifndef"   { count(); BEGIN DIRECTIVE; return(IFNDEF); }
"#elif"        { count(); BEGIN DIRECTIVE; return(ELIF); }
"#else"        { count(); BEGIN DIRECTIVE; return(ELSE); }
"#endif"    { count(); BEGIN DIRECTIVE; return(ENDIF); }
"#define"   { count(); BEGIN DIRECTIVE; return(DEFINE); }
"#include"    { count(); BEGIN DIRECTIVE; return(INCLUDE); }
"#include_verbatim"    { count(); BEGIN DIRECTIVE; return(INCLUDE_VERBATIM); }
"#error"    { count(); mCurrentLine.clear(); BEGIN ERROR_STATE; }
#            { printPreprocessError("Unrecognized preprocessor directive"); yyterminate(); }
<DIRECTIVE>"defined"    {count(); return(DEFINED); }
<DIRECTIVE>"("            { count(); return('('); }
<DIRECTIVE>")"            { count(); return(')'); }
<DIRECTIVE>"&&"        { count(); return(AND); }
<DIRECTIVE>"||"        { count(); return(OR); }
<DIRECTIVE>"=="        { count(); return(EQ); }
<DIRECTIVE>"!="        { count(); return(NEQ); }
<DIRECTIVE>"+"        { count(); return('+'); }
<DIRECTIVE>"-"        { count(); return('-'); }
<DIRECTIVE>"*"        { count(); return('*'); }
<DIRECTIVE>"/"        { count(); return('/'); }
<DIRECTIVE>"!"            { count(); return(NOT); }
<DIRECTIVE>{IDENT} { count(); ((YYSTYPE*)configpplval)->string = yytext; return(IDENTIFIER_TOKEN); }
<DIRECTIVE>0[xX]{H}+{IS}?          { count(); ((YYSTYPE*)configpplval)->ival = strtoll(&yytext[2], nullptr, 16); return(INTEGER); }
<DIRECTIVE>0{D}+{IS}?              { count(); ((YYSTYPE*)configpplval)->ival = strtoll(&yytext[1], nullptr, 8); return(INTEGER); }
<DIRECTIVE>{D}+{IS}?               { count(); ((YYSTYPE*)configpplval)->ival = strtoll(yytext, nullptr, 10); return(INTEGER); }
<DIRECTIVE>\"        { count(); mLastState = YY_START; mCurrentStringLiteral.clear(); BEGIN DIRECTIVESTRING; }
<DIRECTIVE>\r\n { count(); BEGIN 0; mCurrentLine.clear(); return (NEWLINE);}
<DIRECTIVE>\n { count(); BEGIN 0; mCurrentLine.clear(); return (NEWLINE);}
<DIRECTIVE>[ \t\v\f\r]        { count(); }
<DIRECTIVE>. { count(); printPreprocessError("Unrecognized character in preprocessor directive"); yyterminate(); }

<DIRECTIVESTRING>\"    { BEGIN mLastState; count(); ((YYSTYPE*)configpplval)->string = mCurrentStringLiteral; return(STRING_LITERAL);}
<DIRECTIVESTRING>\n    { count(); printPreprocessError("Newline in string constant."); yyterminate(); }
<DIRECTIVESTRING>\\\\ { count(); mCurrentStringLiteral.append("\\"); }
<DIRECTIVESTRING>\\\" { count(); mCurrentStringLiteral.append("\""); }
<DIRECTIVESTRING>.    { count(); mCurrentStringLiteral.append(yytext); }

<NORMALSTRING>\"    { count(); writeToLine(yytext); BEGIN 0; }
<NORMALSTRING>\n    { count(); printPreprocessError("Newline in string constant."); yyterminate(); }
<NORMALSTRING>\\\\  { count(); writeToLine("\\\\"); /*don't unescape these - let the config lexer do it*/}
<NORMALSTRING>\\\"  { count(); writeToLine("\\\""); /*don't unescape these - let the config lexer do it*/ }
<NORMALSTRING>.        { count(); writeToLine(yytext); } 
\r\n        { count(); writeLineToOutput(); } 
\n            { count(); writeLineToOutput(); } 
\"            { count(); writeToLine(yytext); BEGIN NORMALSTRING; }
.            { count(); writeToLine(yytext); } 

<<EOF>>     {   if (!mHasHitEof)
                {
                    mHasHitEof = true;
                    unput('\n');
                }
                else
                {
                    mHasHitEof = false;
                    BEGIN 0;
                    writeLineToOutput();
                    if (!configPopFile()) 
                    {
                        yyterminate();
                    }
                }
            }

%%

namespace Blaze
{
void ConfigPPLexer::comment()
{
    char8_t c, c1;

    FileInfo& active = mFileInfoStack.back();
loop:
    while ((c = yyinput()) != '*' && c > 0)
    {
        active.currentLine += c; 
        if (c == '\n')
        {
            active.currentLine.clear();
            active.line++;
        }
    }
    active.currentLine += c; 

    if ((c1 = yyinput()) != '/' && c > 0)
    {
        unput(c1);
        goto loop;
    }

    if (c != 0)
        active.currentLine += c1; 
}

void ConfigPPLexer::cppcomment()
{
    char8_t c;

    while ((c = yyinput()) != '\n' && c != YY_END_OF_BUFFER_CHAR && c != -1)
       ;
   
    if (c == '\n')
    {
        mCurrentLine.rtrim();
        if (mCurrentLine.empty())
        {
            FileInfo& active = mFileInfoStack.back();
            active.currentLine.clear();
            active.line++;
        }
        else
            unput(c);
    }
}

void ConfigPPLexer::count()
{
    FileInfo& active = mFileInfoStack.back();

    int i;

    for (i = 0; yytext[i] != '\0'; i++)
    {
        if (yytext[i] == '\n')
        {
            active.currentLine.clear();
            active.line++;
        }
        else
        {
            active.currentLine += yytext[i]; 
            active.currentLine[active.currentLine.size()] = 0;
        }
    }
}

void ConfigPPLexer::writeOutput(const char8_t *text)
{
    if (isCurrentBranchActive()) 
    {
        mOutputText.append(text);
    }
}

void ConfigPPLexer::writeToLine(const char8_t *text)
{
    if (isCurrentBranchActive()) 
    {
        mCurrentLine.append(text);
    }
}

void ConfigPPLexer::writeLineToOutput()
{
    if (isCurrentBranchActive()) 
    {
        mCurrentLine.rtrim();
        mCurrentLine.append("\n");
        //Don't bother with empty lines.
        if (mCurrentLine.size() > 1)
        {
            writeLineInfo();
            mOutputText.append(mCurrentLine);
        }
        mCurrentLine.clear();
    }
}

bool ConfigPPLexer::doPreprocessError()
{
    bool result = false;
    if (isCurrentBranchActive())
    {
        printPreprocessError(mCurrentLine.c_str());
        result = true;
    }
    
    mCurrentLine.clear();
    return result;
}

bool ConfigPPLexer::doDefineToken(const char8_t *token)
{
    bool result = true;

    //First strip out the #'s
    eastl::string defineName(token);
    defineName = defineName.substr(1, defineName.size() - 2);
    
    //now that we have a name, look it up in the define map.
    PredefMap::const_iterator itr = getDefineMap().find(defineName);
    if (itr != getDefineMap().end())
    {
        //Put value back in the map, one char at a time.  
        //Because entry goes back to front, we have to do it backwards.
        eastl::string::const_reverse_iterator sitr = itr->second.rbegin();
        eastl::string::const_reverse_iterator send = itr->second.rend();
        for (; sitr != send; ++sitr)
        {
            unput(*sitr);
        }        
    }
    else
    {
        //Define not found.  Throw an error and terminate.
        char8_t msg[2048];
        blaze_snzprintf(msg, sizeof(msg), "Config error: token %s undefined.", defineName.c_str());
        printPreprocessError(msg);
        result = false;
    }
    
    return result;
}

bool ConfigPPLexer::isIncludedFile() const
{    
    return (mFileInfoStack.size() != 1);
}

bool ConfigPPLexer::readVerbatimFile(const char8_t *filename)
{
    char msg[256];

    EA::IO::size_type fileLen = EA::IO::File::GetSize(filename);
    if (fileLen == EA::IO::kSizeTypeError)
    {
        blaze_snzprintf(msg, sizeof(msg), "Cannot determine file size of verbatim file: %s, likely because the file does not exist", filename);
        printPreprocessError(msg);
        return false;
    }

    FILE* inputFile = nullptr;
    inputFile = fopen(filename, "rt");

    if (inputFile == nullptr)
    {
        blaze_snzprintf(msg, sizeof(msg), "Cannot open verbatim file: %s", filename);
        printPreprocessError(msg);
        return false;
    }

    char8_t *fileContents = (char8_t*) BLAZE_ALLOC_MGID(fileLen + 1, MEM_GROUP_FRAMEWORK_CONFIG, "ConfigLexer::readverbitimFile.content");

    //Slurp in the whole thing
    size_t bytesRead = fread(fileContents, 1, fileLen, inputFile);
    if (bytesRead != fileLen && ferror(inputFile))
    {
        BLAZE_FREE(fileContents);
        fileContents = nullptr;
        return false;
    }

    fileContents[bytesRead] = '\0';
    fclose(inputFile);
    
    mCurrentLine.append_sprintf("#verbatim%u:", bytesRead);
    mCurrentLine.append(fileContents);
    writeLineToOutput();
    
    BLAZE_FREE(fileContents);

    return true;
}

bool ConfigPPLexer::configPopFile()
{
    writeEOF();

    mFileInfoStack.pop_back();

    if (!mFileInfoStack.empty())
    {
        FileInfo& active = mFileInfoStack.back();
        active.needsWrite = true;
        yy_switch_to_buffer((yy_buffer_state *) active.state);
        return true;
    }
       
    return false;
}

bool ConfigPPLexer::configPushFile(const char8_t* name)
{
    eastl::string fname;
    if (name[0] == '"')
    {
        // Treat any include starting with a slash as an absolute path, otherwise it is relative to the config root dir
        if (name[1] != '/')
            fname.append(mRootDir);

        size_t nameLen = strlen(name);

        //strip the quotes
        fname.append(name + 1, nameLen - 2);
    }
    else
    {
        // Treat any include starting with a slash as an absolute path, otherwise it is relative to the config root dir
        if (name[0] != '/')
            fname.append(mRootDir);

        fname.append(name);
    }
    
    BLAZE_PUSH_ALLOC_OVERRIDE(MEM_GROUP_FRAMEWORK_CONFIG, "ConfigStream");
    FileInfo& info = mFileInfoStack.push_back();
    info.file.open(fname.c_str(), std::ios::in);
    BLAZE_POP_ALLOC_OVERRIDE();
    if (!info.file.is_open())
    {
        eastl::string errmsg(eastl::string::CtorSprintf(),"Cannot load included file: %s", fname.c_str());
        BlazePrintConfigErrorLog(errmsg.c_str());
        mFileInfoStack.pop_back(); // get rid of invalid stack element
        return false;
    }

    info.lexer = this;
    info.state = yy_create_buffer(&info.file, YY_BUF_SIZE);
    info.filename = name;
    info.line = 0;
    info.needsWrite = true;  
    yy_switch_to_buffer((yy_buffer_state *) info.state);
    
    return true;
}

void ConfigPPLexer::printPreprocessError(const char8_t* msg)
{
    fflush(stdout);
    
    // Set preprocessor error flag
    mHasHitPPError = true;
    
    if (!mFileInfoStack.empty())
    {    
        FileInfo& active = mFileInfoStack.back();
        uint32_t column = 0;
        for(uint32_t i = 0; i < active.currentLine.size(); i++)
        {
            if (active.currentLine[i] == '\t')
                column += 8 - (column % 8);
            else
                column++;
        }

        eastl::string errmsg(eastl::string::CtorSprintf(),"Error preprocessing config at %s(%" PRIu32 ",%" PRIu32 "): %s", active.filename.c_str(), active.line+1, column, msg);
        BlazePrintConfigErrorLog(errmsg.c_str());
    }
    else
    {
        eastl::string errmsg(eastl::string::CtorSprintf(),"Error preprocessing config: %s", msg);
        BlazePrintConfigErrorLog(errmsg.c_str());
    }
}

void ConfigPPLexer::LexerError(const char8_t* msg)
{
    printPreprocessError(msg);
    mHasHitPPError = true;
}


void ConfigPPLexer::writeLineInfo()
{
    char8_t msg[1024];
    FileInfo& active = mFileInfoStack.back();
    if (active.needsWrite || active.line > active.lastWrittenLine + 1)            
    {
        active.needsWrite = false;
        blaze_snzprintf(msg, sizeof(msg), "@LINE:%s:%i\n", active.filename.c_str(), active.line);
        mOutputText.append(msg);
    }

    active.lastWrittenLine = active.line;
}

void ConfigPPLexer::writeEOF()
{
    mOutputText.append("@EOF\n");
}

ConfigPPLexer::FileInfo::FileInfo()
{
    lexer = nullptr;
    state = nullptr;
    line = lastWrittenLine = 0;
}

ConfigPPLexer::FileInfo::~FileInfo()
{
    if (state != nullptr && lexer != nullptr)
        lexer->yy_delete_buffer((yy_buffer_state *) state);
}

}

Blaze::ConfigPPLexer *createConfigPPLexer(const eastl::string& rootDir, eastl::string& out, Blaze::PredefMap& map)
{ 
    return BLAZE_NEW_CONFIG Blaze::ConfigPPLexer(rootDir, out, map); 
}

void destroyConfigPPLexer(Blaze::ConfigPPLexer *lexer)
{ 
    delete lexer;
}

bool validatePPLexer(Blaze::ConfigPPLexer *lexer)
{
    return !(lexer->hitPPError());
}

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(pop)
#endif
