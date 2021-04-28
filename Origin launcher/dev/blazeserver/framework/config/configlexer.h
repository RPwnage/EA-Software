/*! ************************************************************************************************/
/*!
    \file configparserbase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_CONFIG_LEXER_H
#define BLAZE_CONFIG_LEXER_H

#include <sstream>
#include "EASTL/stack.h"
#include "framework/config/config_map.h"


namespace Blaze
{

class ConfigLexer : public yyFlexLexer
{
public:
    ConfigLexer(eastl::string &buf, ConfigMap &rootMap) : mHasHitError(false), mVerbatimCount(0), mVerbatimLength(0)
    {
        createLexBuffer(buf);
        mCurrentColumn = 0;
        mCurrentLine = 0;

        pushMap(&rootMap);

        yytext = nullptr;
        mStringEscapes = true;
        mAllowUnquotedStrings = false;
    }

    ~ConfigLexer() override
    {
        destroyLexBuffer();
    }
   
    int yylex() override;
   
    void setLVal(void *val) { configlval = val; }
    
    void disableStringEscapes() { mStringEscapes = false; }
    bool getAllowUnquotedStrings() const { return mAllowUnquotedStrings; }
    void setAllowUnquotedStrings() { mAllowUnquotedStrings = true; }
    
    void printConfigWarn(const char8_t *msg);
    void printConfigError(const char8_t *msg);
    void LexerError(const char8_t *msg) override;

    struct CollectionInfo
    {
        union
        {
            Blaze::ConfigMap *mMap;
            Blaze::ConfigSequence *mSequence;
        };
        bool mIsMap;
        eastl::string mCollectionFile;
        int mStartLine;
    };
    typedef eastl::vector<CollectionInfo> CollectionStack;

    void pushMap(Blaze::ConfigMap *map);
    void pushSequence(Blaze::ConfigSequence *seq);
    void pop();

    char8_t *readVerbatimFile(const char8_t *filename);

    Blaze::ConfigMap *getCurrentMap() { return mCurrentMap; }
    Blaze::ConfigSequence *getCurrentSequence() { return mCurrentSequence; }

    bool hitError() { return mHasHitError; }

protected:
    void count();
    void processNewlineCharacters(bool escaped);
    void handleLineInfo(const char8_t *info);
    void createLexBuffer(eastl::string& str);
    void destroyLexBuffer();
    void setReturnString(const char8_t *str);

    int mLastState;
    eastl::string mCurrentStringLiteral;
    int mCurrentColumn;
    int mCurrentLine;
    eastl::string mCurrentParseFile;
    
    void *mLexBuffer;
    std::stringstream *mLexStream;

    CollectionStack mCollectionStack;
    Blaze::ConfigMap *mCurrentMap;
    Blaze::ConfigSequence *mCurrentSequence;

    void *configlval;
    bool mStringEscapes;
    bool mAllowUnquotedStrings;

private:
    ConfigLexer(const ConfigLexer& otherObj);
    ConfigLexer& operator=(const ConfigLexer& otherObj);

    bool mHasHitError;

    uint32_t mVerbatimCount;
    uint32_t mVerbatimLength;
};

} // namespace Blaze

//An L-Type for our lexer that can use a string in place.
struct ConfigLType
{
    eastl::string string;
};

#define YYSTYPE ConfigLType


Blaze::ConfigLexer *createConfigLexer();
void destroyConfigLexer(Blaze::ConfigLexer* lexer);
bool validateLexer(Blaze::ConfigLexer *lexer);

#endif //BLAZE_CONFIG_LEXER_H
