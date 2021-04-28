/*! ************************************************************************************************/
/*!
    \file configparserbase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_CONFIG_PP_LEXER_H
#define BLAZE_CONFIG_PP_LEXER_H

#include <fstream>
#include "EASTL/list.h"
#include "EASTL/map.h"
#include "EASTL/string.h"

namespace Blaze
{

typedef eastl::map<eastl::string, eastl::string> PredefMap;

class ConfigPPLexer : public yyFlexLexer
{
public:
    ConfigPPLexer(const eastl::string& rootDir, eastl::string &out, PredefMap &map) : 
      mRootDir(rootDir),
      mOutputText(out), 
      mDefineMap(map), 
      mHasHitPPError(false), 
      mHasHitEof(false)
    {
        //Initialize the if block
        mIfBlockStack.clear();
        mIfBlockStack.push_back();
        mIfBlockStack.back().setCurrent(true);

        yytext = nullptr;
    }

    int yylex() override;
    void setLVal(void *val) { configpplval = val; }

    PredefMap &getDefineMap() { return mDefineMap; }

    struct BranchStatus
    {
        BranchStatus() : mCurrentTrue(false), mEverTrue(false), mParentTrue(true) {}

        void setParentTrue(bool value)
        {
            mParentTrue = value;
        }

        void setCurrent(bool value)
        {
            mCurrentTrue = value;
            if (mCurrentTrue) 
                mEverTrue = true;
        }

        void setCurrentIfNeverTrue(bool value)
        {
            setCurrent(value && !mEverTrue);
        }

        bool isCurrentTrue() const { return mCurrentTrue && mParentTrue; }

        bool mCurrentTrue;
        bool mEverTrue;
        bool mParentTrue;
    };
    
    eastl::vector<BranchStatus>& getIfBlockStack() { return mIfBlockStack; }

    bool hitPPError() { return mHasHitPPError; }

    bool isCurrentBranchActive()
    {
        return mIfBlockStack.back().isCurrentTrue();
    }

    bool configPushFile(const char8_t *name);
    void printPreprocessError(const char8_t *msg);
    void LexerError(const char8_t *msg) override;

    bool readVerbatimFile(const char8_t *filename);

protected:
    void writeOutput(const char8_t *text);
    void writeToLine(const char8_t *text);
    void writeLineToOutput();
    bool doDefineToken(const char8_t *token);
    bool doPreprocessError();
    void writeLineInfo();
    void writeEOF();
    void count();
    void comment();
    void cppcomment();
    bool isIncludedFile() const;

    bool configPopFile();

private:
    eastl::string mRootDir;

    class FileInfo
    {
        NON_COPYABLE(FileInfo);
    public:
        ConfigPPLexer* lexer;
        void* state;
        std::fstream file;
        eastl::string filename;
        bool needsWrite;
        uint32_t line;
        uint32_t lastWrittenLine;
        eastl::string currentLine;

        FileInfo();
        ~FileInfo();
    };

    eastl::list<FileInfo> mFileInfoStack;
    eastl::vector<BranchStatus> mIfBlockStack;
    
    //String state variables
    int mLastState;
    eastl::string mCurrentStringLiteral;
    eastl::string mCurrentLine;
    eastl::string& mOutputText;
    
    PredefMap &mDefineMap;
    void *configpplval;

    bool mHasHitPPError;
    bool mHasHitEof;

private:
    ConfigPPLexer(const ConfigPPLexer& otherObj);
    ConfigPPLexer& operator=(const ConfigPPLexer& otherObj);

};

} // namespace Blaze

//An L-Type for our lexer that can use a string in place.
struct ConfigPPLType
{
    bool boolean;
    eastl::string string;
    int64_t ival;    
};

#define YYSTYPE ConfigPPLType

Blaze::ConfigPPLexer *createConfigPPLexer(const eastl::string& rootDir, eastl::string &out, Blaze::PredefMap &map);

void destroyConfigPPLexer(Blaze::ConfigPPLexer *lexer);

bool validatePPLexer(Blaze::ConfigPPLexer *lexer);

#endif //BLAZE_CONFIG_PP_CONTEXT_H
