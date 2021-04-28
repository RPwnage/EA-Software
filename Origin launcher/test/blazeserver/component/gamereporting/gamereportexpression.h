/*************************************************************************************************/
/*!
    \file   gamereportexpression.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_EXPRESSION_H
#define BLAZE_GAMEREPORTING_EXPRESSION_H

#include "framework/blazedefines.h"

#include "gamereporting/tdf/gamereporting_server.h"

#include "EASTL/intrusive_list.h"

namespace Blaze
{
namespace GameReporting
{
class GameType;
struct GameReportContext;

//  Expression Language
//  
//  Reference --> Reference 
class GameReportExpression
{
    NON_COPYABLE(GameReportExpression);

public:
    //  the format string is a reference to data owned by the GameType.   This is done since this object may be copied frequently
    //  during report parsing, cached off for optimization, etc.    
    GameReportExpression(const GameType& gameType, const char8_t* fmt);
    GameReportExpression(const char8_t* gameReportName, const char8_t* fmt);
    virtual ~GameReportExpression();

    //GameReportExpression(const GameReportExpression& value);

    //  parses the stored expression string
    bool parse();
    //  evaluates the expression
    bool evaluate(EA::TDF::TdfGenericValue& value, const GameReportContext& context, bool requireValidation = true);

    enum ResultCode
    {
        ERR_OK = 0,
        ERR_SYNTAX,
        ERR_NO_RESULT,
        ERR_EVALUATE_FAIL,
        ERR_MISSING_GAME_ATTRIBUTE,
        ERR_MISSING_PLAYER_ATTRIBUTE,
        ERR_MISSING_GAME_SETTING
    };

    ResultCode getResultCode() const {
        return mError;
    }

    const char8_t* getGameReportName() const { return mGameReportName; }

private:
    const char8_t* mGameReportName;
    eastl::string mExpressionStr;

    struct TokenNode
    {
        typedef eastl::vector<TokenNode*> List;        
        static void clearTokenNodeList(List& list);

        explicit TokenNode(const eastl::string& str): token(str) {}
        
        typedef eastl::vector<List> ArgumentList;        
        ArgumentList argList;
        eastl::string token;
    };
    TokenNode::List mTokenList;

    //  each of these methods will modify the expression string as tokens are pushed onto the list
    bool parseExpression(eastl::string& expression, TokenNode::List& tokenList);
    //  stuff command (including parameters) onto token list.
    bool parseCommand(eastl::string& expression, TokenNode::List& tokenList);
    //  parse parameter [] section from expression.
    bool parseParameter(eastl::string& expression, TokenNode *parentToken, bool& hasParameter);

    TokenNode::List::const_iterator execExpression(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, TokenNode::List::const_iterator tokenIt, TokenNode::List::const_iterator tokenItEnd, 
        EA::TDF::TdfGenericValue& result, EA::TDF::TdfGenericValue* result2, bool &succeeded); 

    bool execIndex(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, EA::TDF::TdfGenericValue& result);
    bool execGameAttrs(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t* gameAttrKey, EA::TDF::TdfGenericValue& result);
    bool execPlayerAttrs(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, BlazeId playerId, const char8_t* playerAttrKey, EA::TDF::TdfGenericValue& result);
    bool execDnf(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, BlazeId playerId, EA::TDF::TdfGenericValue& result) const;
    bool execReport(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf) const;
    bool execExplicit(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t *explicitStr, EA::TDF::TdfGenericValue& result);
    bool execReference(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const TokenNode& referenceNode, EA::TDF::TdfGenericValue& result);
    bool execGameSettings(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t* gameSettingsKey, EA::TDF::TdfGenericValue& result);
    bool execPlayerSettings(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, BlazeId playerId, const char8_t *playerSettingKey, EA::TDF::TdfGenericValue& result);
    bool execGameInfo(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t* gameInfoName, EA::TDF::TdfGenericValue& result);


private:
    //  set result if none already - otherwise uses the existing result
    bool setResult(ResultCode res) {
        return (mError == ERR_OK) ? ((mError = res) == ERR_OK) : false;
    }

    mutable ResultCode mError;

    //  Whether we're validating gamereporting config file (rather than processing actual game's report).
    //  Used to tell expression evaluator to skip trying getting values in a visited map/list tdf
    //  since it maybe default empty for config-validation.
    bool mRequireValidation;
};


} //namespace Blaze
} //namespace GameReporting
#endif
