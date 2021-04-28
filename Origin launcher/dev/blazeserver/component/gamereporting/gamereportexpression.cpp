/*************************************************************************************************/
/*!
    \file   gamereportexpression.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"
#include "gamereportexpression.h"
#include "gametype.h"


namespace Blaze
{
namespace GameReporting
{

///////////////////////////////////////////////////////////////////////////////
void GameReportExpression::TokenNode::clearTokenNodeList(List& list)
{
   for (TokenNode::List::iterator it = list.begin(); it != list.end(); ++it)
   {
        TokenNode* node = *it;    
        if (!node->argList.empty())
        {
            for (ArgumentList::iterator argIt = node->argList.begin(); argIt != node->argList.end(); ++argIt)
            {
                clearTokenNodeList(*argIt);
            }
        }
        delete node;
   }
   list.clear();
}


//  the format string is a reference to data owned by the GameType.   This is done since this object may be copied frequently
//  during report parsing, cached off for optimization, etc.
GameReportExpression::GameReportExpression(const GameType& gameType, const char8_t* fmt) :
    mExpressionStr(fmt),
    mError(ERR_OK),
    mRequireValidation(false)
{
    mGameReportName = gameType.getGameReportName().c_str();
    mExpressionStr.trim(); 
}

GameReportExpression::GameReportExpression(const char8_t* gameReportName, const char8_t* fmt) :
    mGameReportName(gameReportName),
    mExpressionStr(fmt),
    mError(ERR_OK),
    mRequireValidation(false)
{
    mExpressionStr.trim(); 
}

//  copy just copies the expression but not the evaluation data.
GameReportExpression::~GameReportExpression()
{
    // clear token list
    TokenNode::clearTokenNodeList(mTokenList);
}


//  Parses the stored expression string
//  generates the tokens required for evaluation.
//  Expression format.      
//
//      Supported predefined vars ($_$):
//          DNF, 
//          GAMEATTRS
//          PLAYERATTRS
//          INDEX
//          REPORT
//          PARENT
//          GAMESETTINGS
//          PLAYERSETTINGS
//          GAMEINFO
//
//      Parsing scheme:
//          Goal is to construct a value given a reference.
//          The reference can come from predefined variables and/or explicit reference.
//          First pass, call all predefined functions to generate the reference string.
bool GameReportExpression::parse()
{
    //  clear old tokens
    if (!mTokenList.empty())
    {        
        TokenNode::clearTokenNodeList(mTokenList);
    }

    //  left to right evaluation
    //  parsing from the reference string.
    if (!parseExpression(mExpressionStr, mTokenList))
    {
        ERR_LOG("[GameReportExpression].parse() : expression parse failed, expr='" << mExpressionStr.c_str() << "', gametype='" << mGameReportName << "'" );
        return setResult(ERR_SYNTAX);
    }

    return setResult(ERR_OK);
}


//  goes through the token list, evaluating the expression given the reporting context
//  stores result in 'value'
bool GameReportExpression::evaluate(EA::TDF::TdfGenericValue& value, const GameReportContext& context, bool requireValidation)
{
    bool origValidationFlag = mRequireValidation;
    mRequireValidation = requireValidation;

    mError = ERR_OK;                // reset error state for new evaluation.

    if (mTokenList.empty())
    {
        if (!parse())
        {
            return false;
        }
    }   
   
    if (mTokenList.empty())
    {
        TRACE_LOG("[GameReportExpression].evaluate() : expression is null.");
        return setResult(ERR_NO_RESULT);
    }

    //  run the token stack 
    //  top level TDF for the reference.   This might be modified by a command, which is why it's defined here 
    //  (but used at the end of the method.)
    const EA::TDF::Tdf* thisTdf = context.tdf;
    value.clear();

    bool succeeded = false;
    TokenNode::List::const_iterator tokenItEnd = execExpression(context, thisTdf, mTokenList.begin(), mTokenList.end(), value, nullptr, succeeded);
    mRequireValidation = origValidationFlag;

    if (!succeeded)
    {
        if (tokenItEnd == mTokenList.end())
        {
            TRACE_LOG("[GameReportExpression].evaluate() : Evaluate failed for expression, tdf='" << (thisTdf != nullptr ? thisTdf->getFullClassName() : "unknown") << "'");
        }
        else
        {
            TRACE_LOG("[GameReportExpression].evaluate() : Evaluate failed at token='" << (*tokenItEnd)->token.c_str() << "', tdf='" 
                     << (thisTdf != nullptr ? thisTdf->getFullClassName() : "unknown") << "'");
        }
        return false;
    }

    return setResult(ERR_OK);
}


//  executes actions sequentially, left to right, descending into commands as 
GameReportExpression::TokenNode::List::const_iterator GameReportExpression::execExpression(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, TokenNode::List::const_iterator tokenIt, TokenNode::List::const_iterator tokenItEnd,
            EA::TDF::TdfGenericValue& result, EA::TDF::TdfGenericValue* result2, bool &succeeded)
{
    if (tokenIt == tokenItEnd)
    {
        succeeded = setResult(ERR_NO_RESULT);
        return tokenIt;
    }

    const TokenNode& tokenNode = **tokenIt;
    const eastl::string& token = tokenNode.token;
    bool isSuccess = false;

    ++tokenIt;
    TokenNode::List::const_iterator nextTokenIt = tokenIt;

    if (!token.empty())
    {
        bool handleDefault = false;
        //  treat the current token based on the execution state.
        //  at execution start (first token)
        if (token.front() == '^')
        {
            isSuccess = execExplicit(context, thisTdf, token.c_str()+1, result);
        }
        else if (token.front() == '$' && token.back() == '$')
        {
            // FIFA SPECIFIC CODE START
            if (token == "squadReports")
            {
                TRACE_LOG("[GameReportExpression:" << this << "].evaluate() : Evaluating squadReport Token! ");
            }
            // FIFA SPECIFIC CODE END
            if (token == "$INDEX$")
            {
                isSuccess = execIndex(context, thisTdf, result);
            }
            else if (token == "$PARENT$")
            {
                if (context.parent != nullptr)
                {
                    const EA::TDF::Tdf* localTdf = context.parent->tdf;
                    nextTokenIt = execExpression(*context.parent, localTdf, tokenIt, tokenItEnd, result, nullptr, isSuccess);
                }
                else
                {
                    ERR_LOG("[GameReportExpression].execExpression() : PARENT context not available for TDF at current context '" << thisTdf->getFullClassName() << "'.");
                }
            }
            else if (token == "$GAMEATTRS$")
            {
                const TokenNode::List* argTokenList = tokenNode.argList.size() == 1 ? &tokenNode.argList[0] : nullptr;
                isSuccess = false;
                if (argTokenList == nullptr)
                {
                    ERR_LOG("[GameReportExpression].execExpression(): GAMEATTRS requires an argument to evaluate for gameType='" << mGameReportName << "'");         
                }
                else
                {
                    if (argTokenList->end() == execExpression(context, thisTdf, argTokenList->begin(), argTokenList->end(), result, nullptr, isSuccess) && isSuccess)
                    {
                        if (result.getTypeDescription().isString())
                        {
                            isSuccess = execGameAttrs(context, thisTdf, result.asString(), result);
                        }
                        else
                        {
                            ERR_LOG("[GameReportExpression].execExpression() : STRING required to lookup game attribute, found type='" << result.getType() << "'.");
                        }
                    }
                }
            }
            else if (token == "$PLAYERATTRS$")
            {
                //  execute a pair expression evaluation to return the BlazeId and the PlayerAttribute to lookup
                isSuccess = false;      // this will be set to true below on success.
                if (tokenNode.argList.size() == 2)
                {
                    const TokenNode::List* argTokenListLeft = &tokenNode.argList[0];
                    const TokenNode::List* argTokenListRight = &tokenNode.argList[1];
                    EA::TDF::TdfGenericValue secondResult;

                    if (argTokenListLeft->end() == execExpression(context, thisTdf, argTokenListLeft->begin(), argTokenListRight->end(), result, nullptr, isSuccess) && isSuccess)
                    {
                        if (argTokenListRight->end() == execExpression(context, thisTdf, argTokenListRight->begin(), argTokenListRight->end(), secondResult, nullptr, isSuccess) && isSuccess)
                        {
                            BlazeId playerId;
                            EA::TDF::TdfGenericReference refPlayerId(playerId);
                            if (result.convertToIntegral(refPlayerId) && secondResult.getTypeDescription().isString())
                            {
                                isSuccess = execPlayerAttrs(context, thisTdf, playerId, secondResult.asString(), result);
                            }
                            else
                            {
                                ERR_LOG("[GameReportExpression].execExpression() : INT, STRING required to lookup player attribute, found type='" << result.getType() << "'.");
                            }
                        }
                    }
                }
            }
            else if (token == "$DNF$")
            {
                const TokenNode::List* argTokenList = tokenNode.argList.size() == 1 ? &tokenNode.argList[0] : nullptr;
                isSuccess = false;
                if (argTokenList == nullptr)
                {
                    ERR_LOG("[GameReportExpression].execExpression(): DNF requires an argument to evaluate for gameType='" << mGameReportName << "'");
                }
                else if (argTokenList->end() == execExpression(context, thisTdf, argTokenList->begin(), argTokenList->end(), result, nullptr, isSuccess))
                {
                    BlazeId playerId;
                    EA::TDF::TdfGenericReference refPlayerId(playerId);
                    if (result.convertToIntegral(refPlayerId))
                    {
                        isSuccess = execDnf(context, thisTdf, playerId,  result);
                    }
                    else
                    {
                        ERR_LOG("[GameReportExpression].execExpression() : ID required to lookup DNF attribute, found type='" << result.getType() << "'");
                    }
                }
            }
            else if (token == "$GAMESETTINGS$")
            {
                const TokenNode::List* argTokenList = tokenNode.argList.size() == 1 ? &tokenNode.argList[0] : nullptr;
                isSuccess = false;
                if (argTokenList == nullptr)
                {
                    ERR_LOG("[GameReportExpression].execExpression(): GAMESETTINGS requires an argument to evaluate for gameType='" << mGameReportName << "'");
                }
                else if(argTokenList->end() == execExpression(context, thisTdf, argTokenList->begin(), argTokenList->end(), result, nullptr, isSuccess) && isSuccess)
                {
                    if (result.getTypeDescription().isString())
                    {
                        isSuccess = execGameSettings(context, thisTdf, result.asString(), result);
                    }
                    else
                    {
                        ERR_LOG("[GameReportExpression].execExpression() : STRING required to lookup game settings, found type='" << result.getType() << "'.");
                    }
                }
            }
            else if (token == "$PLAYERSETTINGS$")
            {
                //  execute a pair expression evaluation to return the BlazeId and the player information to lookup
                isSuccess = false;      // this will be set to true below on success.
                if (tokenNode.argList.size() == 2)
                {
                    const TokenNode::List* argTokenListLeft = &tokenNode.argList[0];
                    const TokenNode::List* argTokenListRight = &tokenNode.argList[1];
                    EA::TDF::TdfGenericValue secondResult;

                    if (argTokenListLeft->end() == execExpression(context, thisTdf, argTokenListLeft->begin(), argTokenListRight->end(), result, nullptr, isSuccess) && isSuccess)
                    {
                        if (argTokenListRight->end() == execExpression(context, thisTdf, argTokenListRight->begin(), argTokenListRight->end(), secondResult, nullptr, isSuccess) && isSuccess)
                        {
                            BlazeId playerId;
                            EA::TDF::TdfGenericReference refPlayerId(playerId);
                            if (result.convertToIntegral(refPlayerId) && secondResult.getTypeDescription().isString())
                            {
                                isSuccess = execPlayerSettings(context, thisTdf, playerId, secondResult.asString(), result);
                            }
                            else
                            {
                                ERR_LOG("[GameReportExpression].execExpression() : INT, STRING required to lookup player information, found first type='" 
                                    << result.getType() << "' and second type='" << secondResult.getType() << "'.");
                            }
                        }
                    }
                }
                else
                {
                    ERR_LOG("[GameReportExpression].execExpression(): PLAYERSETTINGS requires two arguments to evaluate for gameType='" << mGameReportName << "'");
                }
            }
            else if (token == "$GAMEINFO$")
            {
                const TokenNode::List* argTokenList = tokenNode.argList.size() == 1 ? &tokenNode.argList[0] : nullptr;
                isSuccess = false;
                if (argTokenList == nullptr)
                {
                    ERR_LOG("[GameReportExpression].execExpression(): GAMEINFO requires an argument to evaluate for gameType='" << mGameReportName << "'");         
                }
                else
                {
                    if (argTokenList->end() == execExpression(context, thisTdf, argTokenList->begin(), argTokenList->end(), result, nullptr, isSuccess) && isSuccess)
                    {
                        if (result.getTypeDescription().isString())
                        {
                            isSuccess = execGameInfo(context, thisTdf, result.asString(), result);
                        }
                        else
                        {
                            ERR_LOG("[GameReportExpression].execExpression() : STRING required to lookup game information, found type='" << result.getType() << "'.");
                        }
                    }
                }
            }
            else if (token == "$REPORT$")
            {
                isSuccess = execReport(context, thisTdf);
                if (isSuccess)
                {
                    nextTokenIt = execExpression(context, thisTdf, tokenIt, tokenItEnd, result, nullptr, isSuccess);
                }
            }
            // FIFA SPECIFIC CODE START
            else if(token == "squadReports")
            {
                isSuccess = execReference(context, thisTdf, tokenNode, result);
                if (isSuccess)
                {
                    if(tokenIt != tokenItEnd)
                    {
                        nextTokenIt = execExpression(context, thisTdf, tokenIt, tokenItEnd, result, NULL, isSuccess);
                    }
                }
            }
            // FIFA SPECIFIC CODE END
            else
                handleDefault = true;
        }
        else
            handleDefault = true;
        
        if (handleDefault)
        {
            isSuccess = execReference(context, thisTdf, tokenNode, result);
            if (isSuccess)
            {
                if (tokenIt != tokenItEnd)
                {
                    nextTokenIt = execExpression(context, thisTdf, tokenIt, tokenItEnd, result, nullptr, isSuccess);
                }
            }
        }
    }

    if (!isSuccess)
    {
        TRACE_LOG("[GameReportExpression].execExpression() : failed at token (" << token.c_str() << ") for gameType='" << mGameReportName << "'");    
        nextTokenIt = tokenItEnd;
    }

    succeeded = isSuccess;

    return nextTokenIt;
}

bool GameReportExpression::execIndex(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, EA::TDF::TdfGenericValue& result)
{
    if (context.container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
    {
        result.set(context.containerIteration);            
    }
    else if (context.container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP)
    {
        if (context.container.value.getTypeDescription().asMapDescription()->keyType.isIntegral())
        {
            result.set(context.container.Index.id);
        }
        else if (context.container.value.getTypeDescription().asMapDescription()->keyType.isString())
        {
            result.set(context.container.Index.str);
        }
        else
        {
            ERR_LOG("[GameReportExpression].execIndex() : container type / context mismatch in the current context EA::TDF::Tdf '" << context.tdf->getClassName() << "'"); 
            return false;
        }
    }
    else
    {
        ERR_LOG("[GameReportExpression].execIndex() : no valid container for current context EA::TDF::Tdf '" << context.tdf->getClassName() << "'"); 
        return false;
    }

    return true;
}


bool GameReportExpression::execGameAttrs(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t *gameAttrKey, EA::TDF::TdfGenericValue& result)
{
    if (context.gameInfo == nullptr)
    {
        //  behavior is a NOP if no game information available (i.e. startup configuration, etc.)
        return true;
    }

    Collections::AttributeMap::const_iterator cit = context.gameInfo->getAttributeMap().find(gameAttrKey);
    if (cit == context.gameInfo->getAttributeMap().end())
    {
        WARN_LOG("[GameReportExpression].execGameAttrs() : Attribute '" << gameAttrKey << "' not found in GameInfo (GID='" << context.gameInfo->getGameReportingId() << "').");
        return setResult(ERR_MISSING_GAME_ATTRIBUTE);
    }

    return execExplicit(context, thisTdf, cit->second.c_str(), result);
}


bool GameReportExpression::execPlayerAttrs(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, BlazeId playerId, const char8_t* playerAttrKey, EA::TDF::TdfGenericValue& result)
{
    if (context.gameInfo == nullptr)
    {
        //  behavior is a NOP if no game information available (i.e. offline games, startup configuration, etc.
        return true;
    }

    GameInfo::PlayerInfoMap::const_iterator citPlayer = context.gameInfo->getPlayerInfoMap().find(playerId);
    if (citPlayer == context.gameInfo->getPlayerInfoMap().end())
    {
        WARN_LOG("[GameReportExpression].execPlayerAttrs() : Player '" << playerId << "' not found in GameInfo (GID='" << context.gameInfo->getGameReportingId() << "')");
        return setResult(ERR_EVALUATE_FAIL);
    }
    
    Collections::AttributeMap& playerAttrs = citPlayer->second->getAttributeMap();
    Collections::AttributeMap::const_iterator cit = playerAttrs.find(playerAttrKey);
    if (cit == playerAttrs.end())
    {
        WARN_LOG("[GameReportExpression].execPlayerAttrs() : Attribute '" << playerAttrKey << "' not found in GameInfo (GID='" 
                 << context.gameInfo->getGameReportingId() << "') for player ID=" << playerId << ".");
        return setResult(ERR_MISSING_PLAYER_ATTRIBUTE);
    }

     return execExplicit(context, thisTdf, cit->second.c_str(), result);
}


bool GameReportExpression::execDnf(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, BlazeId playerId, EA::TDF::TdfGenericValue& result) const
{
    if (context.gameInfo == nullptr || context.dnfStatus == nullptr)
    {
        //  behavior is a NOP if no game information available (i.e. offline games, startup configuration, etc.
        return true;
    }

    CollatedGameReport::DnfStatusMap::const_iterator cit = context.dnfStatus->find(playerId);
    if (cit == context.dnfStatus->end())
    {
        WARN_LOG("[GameReportExpression].execDnf() : Player " << playerId << " not found in game GID=" << context.gameInfo->getGameReportingId() << " needed to read DNF status.");
        return false;
    }

    result.set(cit->second);
    return true;
}


bool GameReportExpression::execReport(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf) const
{
    //  find the top-most context and set it as the current EA::TDF::Tdf.
    const GameReportContext *curcontext = &context;
    while (curcontext->parent != nullptr)
        curcontext = curcontext->parent;
    thisTdf = curcontext->tdf;
    return true;
}


bool GameReportExpression::execExplicit(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t* explicitStr, EA::TDF::TdfGenericValue& result)
{
    //  assume the explicit item is a string - the caller should know what to do with item.
    result.set(explicitStr);

    return true;
}


bool GameReportExpression::execReference(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const TokenNode& referenceNode, EA::TDF::TdfGenericValue& result)
{    
    // determine if this reference is indexed, and if so parse the index and store off the result for later.
    const TokenNode::List* argTokenList = nullptr;
    bool hasIndexParam = referenceNode.argList.size()==1;       // references can support at most one index parameter
    const eastl::string& parsedReferenceStr = referenceNode.token;

    if (hasIndexParam)
    {
        argTokenList = &referenceNode.argList[0];
    }


    // get the reference data object from the current TDF
    const char8_t* tdfMemberName = parsedReferenceStr.c_str();

    EA::TDF::TdfGenericReferenceConst value;
    bool validValue = thisTdf->getValueByName(tdfMemberName, value);
    if (!validValue)
    {
        TRACE_LOG("[GameReportExpression].execReference() : Value not found for TDF member '" << tdfMemberName 
                  << "' tdfMemberName in current TDF '" << thisTdf->getFullClassName() << "'");
        return setResult(ERR_EVALUATE_FAIL);
    }

    //  retrieve map or vector item based on current context's container iteration.
    if (value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP || value.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
    {
        if (!hasIndexParam)
        {
            TRACE_LOG("[GameReportExpression].execReference() : Referencing a Map/List '" << tdfMemberName 
                      << "' requies an explicit index in TDF '" << thisTdf->getFullClassName() << "'");
            return setResult(ERR_EVALUATE_FAIL);
        }
        //  define this reference's container context.
        EA::TDF::TdfGenericValue containerIndex;
        //  evaluate index tokens, store referenced container's index off.
        const EA::TDF::Tdf* localTdf = context.tdf;
        bool isSuccess = true;
        if (argTokenList->end() != execExpression(context, localTdf, argTokenList->begin(), argTokenList->end(), containerIndex, nullptr, isSuccess) || !isSuccess)  /*lint !e613 */
        {
            TRACE_LOG("[GameReportExpression].execReference() : Map/List '" << tdfMemberName << "' index evaluation failed in TDF '" << thisTdf->getFullClassName() << "'");
            return setResult(ERR_EVALUATE_FAIL);
        }
  
        if (value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP)
        {
            const EA::TDF::TdfMapBase &thisMap = value.asMap();
            result.setType(thisMap.getValueTypeDesc());

            if (!mRequireValidation)
            {
                TRACE_LOG("[GameReportExpression].execReference() : Config validation found Map '" << tdfMemberName << "' (keys' tdf-type: " << containerIndex.getType() << ", values' tdf-type: " << result.getType()
                    << "). Skipping trying to get value from container since it maybe empty in config-validation mode.");
                // for config-validation, we just care about the value type of the map.  No need to see if element actually exists since map may be empty in config-validation mode
                return true;
            }
            else if (!thisMap.getValueByKey(containerIndex, value))
            {
                return setResult(ERR_NO_RESULT);
            }
        }
        else if (value.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
        {
            const EA::TDF::TdfVectorBase& thisVec = value.asList();
            result.setType(thisVec.getValueTypeDesc());

            if (!mRequireValidation)
            {
                TRACE_LOG("[GameReportExpression].execReference(): Config validation found List '" << tdfMemberName << "' (keys' tdf-type: " << containerIndex.getType() << ", values' tdf-type: " << result.getType()
                    << "). Skipping trying to get value from container since it maybe empty in config-validation mode.");
                // for config-validation, we just care about the value type of the list.  No need to see if element actually exists since list may be empty in config-validation mode
                return true;
            }
            // by default, explicit reference returns type string, leaving the parent deal with it.
            // the index of a list should be integer.
            else if (containerIndex.getTypeDescription().isIntegral())
            {
                size_t index = 0;
                EA::TDF::TdfGenericReference refIndex(index);
                if (!result.convertToIntegral(refIndex) || !thisVec.getValueByIndex(index, value))
                {
                    return setResult(ERR_NO_RESULT);
                }
            }
            else if (containerIndex.getTypeDescription().isFloat())
            {
                if (!thisVec.getValueByIndex(static_cast<uint64_t>(containerIndex.asFloat()), value))
                {
                    return setResult(ERR_NO_RESULT);
                }
            }
            else if (containerIndex.getTypeDescription().isString())
            {
                size_t index;
                EA::TDF::TdfGenericReference refIndex(index);
                if (!containerIndex.convertFromString(refIndex) || !thisVec.getValueByIndex(index, value))
                {
                    TRACE_LOG("[GameReportExpression].execReference() : List '" << tdfMemberName << "' has an invalid string index value in TDF '" 
                        << thisTdf->getFullClassName() << "'");
                    return setResult(ERR_EVALUATE_FAIL);
                }
            }
            else
            {
                TRACE_LOG("[GameReportExpression].execReference() : List '" << tdfMemberName << "' has an invalid index type (" << containerIndex.getType() 
                          << ") in TDF '" << thisTdf->getFullClassName() << "'");
                return setResult(ERR_EVALUATE_FAIL);
            }
        }
    }
    
    //  set current TDF based on the result - either from a container or a direct reference to a TDF retrieved above.
    if (value.getType() == EA::TDF::TDF_ACTUAL_TYPE_TDF)
    {
        thisTdf = &value.asTdf();
    }
    else if (value.getType() == EA::TDF::TDF_ACTUAL_TYPE_VARIABLE)
    {
        thisTdf = value.asVariable().get();
    }

    result = value;
    return true;
}

bool GameReportExpression::execGameSettings(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t* gameSettingsKey, EA::TDF::TdfGenericValue& result)
{
    if (context.gameInfo == nullptr)
    {
        //  behavior is a NOP if no game information available (i.e. startup configuration, etc.)
        return true;
    }

    uint32_t value;
    GameManager::GameSettings settings = context.gameInfo->getGameSettings();
    if (!settings.getValueByName(gameSettingsKey, value))
    {
        WARN_LOG("[GameReportExpression].execGameSettings() : game setting '" << gameSettingsKey << "' not found in GameInfo (GID='" << context.gameInfo->getGameReportingId() << "').");
        return setResult(ERR_MISSING_GAME_SETTING);
    }

    const EA::TDF::TypeDescriptionBitfieldMember* gameSettingDesc = nullptr;
    if (settings.getTypeDescription().asBitfieldDescription()->findByName(gameSettingsKey, gameSettingDesc) && gameSettingDesc->mIsBool)
    {
        result.set(EA::TDF::TdfString(value ? "true" : "false"));
    }
    else
    {
        WARN_LOG("[GameReportExpression].execGameSettings() : Unexpected - game setting '" << gameSettingsKey << "' is not boolean. This would not be handled correctly by the game reporting system.");
        return setResult(ERR_MISSING_GAME_SETTING);
    }

    return true;
}

bool GameReportExpression::execPlayerSettings(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, BlazeId playerId, const char8_t *playerSettingKey, EA::TDF::TdfGenericValue& result)
{
    if (context.gameInfo == nullptr)
    {
        //  behavior is a NOP if no game information available (i.e. offline games, startup configuration, etc.
        //  NOP except ... assign default values so that GameHistory can determine the appropriate datatype size
        if (blaze_stricmp(playerSettingKey, "accountLocale") == 0)
        {
            result.set(Locale(0));
        }
        else if (blaze_stricmp(playerSettingKey, "country") == 0)
        {
            result.set(uint32_t(0));
        }
        else if (blaze_stricmp(playerSettingKey, "language") == 0)
        {
            result.set(uint16_t(0));
        }
        else if (blaze_stricmp(playerSettingKey, "leavingReason") == 0)
        {
            result.set(int32_t(0));
        }
        else if (blaze_stricmp(playerSettingKey, "DNF") == 0)
        {
            result.set(int8_t(0));
        }
        else if (blaze_stricmp(playerSettingKey, "encryptedBlazeId") == 0)
        {
            result.set("");
        }
        return true;
    }

    GameInfo::PlayerInfoMap::const_iterator citPlayer = context.gameInfo->getPlayerInfoMap().find(playerId);
    if (citPlayer == context.gameInfo->getPlayerInfoMap().end())
    {
        WARN_LOG("[GameReportExpression].execPlayerSettings() : Player '" << playerId << "' not found in GameInfo (GID='" << context.gameInfo->getGameReportingId() << "')");
        return setResult(ERR_EVALUATE_FAIL);
    }

    if (blaze_stricmp(playerSettingKey, "accountLocale") == 0)
    {
        result.set(citPlayer->second->getAccountLocale());
    }
    else if (blaze_stricmp(playerSettingKey, "country") == 0)
    {
        uint32_t country = citPlayer->second->getAccountCountry();
        result.set(country);
    }
    else if (blaze_stricmp(playerSettingKey, "language") == 0)
    {
        uint16_t language = LocaleTokenGetLanguage(citPlayer->second->getAccountLocale());
        result.set(language);
    }
    else if (blaze_stricmp(playerSettingKey, "leavingReason") == 0)
    {
        result.set(citPlayer->second->getRemovedReason());
    }
    else if (blaze_stricmp(playerSettingKey, "DNF") == 0)
    {
        int8_t dnf = citPlayer->second->getFinished() ? 0 : 1;
        result.set(dnf);
    }
    else if (blaze_stricmp(playerSettingKey, "encryptedBlazeId") == 0)
    {
        result.set(citPlayer->second->getEncryptedBlazeId());
    }
    else
    {
        WARN_LOG("[GameReportExpression].execPlayerSettings() : Player information '" << playerSettingKey << "' not found in GamePlayerInfo (GID='" << context.gameInfo->getGameReportingId() << "')");
        return setResult(ERR_EVALUATE_FAIL);
    }

    return true;
}

bool GameReportExpression::execGameInfo(const GameReportContext& context, const EA::TDF::Tdf* &thisTdf, const char8_t* gameInfoName, EA::TDF::TdfGenericValue& result)
{
    if (context.gameInfo == nullptr)
    {
        //  behavior is a NOP if no game information available (i.e. startup configuration, etc.)
        //  NOP except ... assign default values so that GameHistory can determine the appropriate datatype size
        if (blaze_stricmp(gameInfoName, "durationMS") == 0)
        {
            result.set(uint32_t(0));
        }
        else if (blaze_stricmp(gameInfoName, "gameId") == 0)
        {
            result.set(GameManager::GameId(0));
        }
        return true;
    }

    if (blaze_stricmp(gameInfoName, "durationMS") == 0)
    {
        result.set(context.gameInfo->getGameDurationMs());
    }
    else if (blaze_stricmp(gameInfoName, "gameId") == 0)
    {
        result.set(context.gameInfo->getGameId());
    }
    else
    {
         WARN_LOG("[GameReportExpression].execGameInfo() : Game information '" << gameInfoName << "' not found in GameInfo (GID='" << context.gameInfo->getGameReportingId() << "')");
         return setResult(ERR_EVALUATE_FAIL);
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////////////



bool GameReportExpression::parseExpression(eastl::string& expression, TokenNode::List& tokenList)
{
    using namespace eastl;

    if (expression.empty())
        return true;

    expression.trim();

    bool error = false;
    while (!expression.empty() && !error)
    {
        string::size_type tokenlen = 0;

        //  find length of token
        string::size_type tokenPos = expression.find_first_of("[.", 0);
        if (tokenPos == string::npos)
        {
            tokenlen = expression.length();
        }
        else if (expression[tokenPos] == '[')
        {
            //  advance to end of parameter to complete the expression.
            uint32_t depth = 0;
            do
            {
                depth++;
                tokenPos = expression.find_first_of("[]", tokenPos+1);
                if (tokenPos != string::npos)
                {
                    depth = expression[tokenPos] == '[' ? (depth+1) : (depth-1);
                }
            }
            while (depth > 0 && tokenPos != string::npos);
            if (depth > 0)
            {
                //  mismatched bracket error
                error = true;
            }
            else
            {
                tokenlen = tokenPos+1;  // include ']' terminator
            }
        }
        else
        {
            //  simple reference, 
            tokenlen = tokenPos;
        }
        if (error)
            break;
         
        //  parse token.        
        eastl::string subexpression = expression.substr(0, tokenlen);
        char8_t token = subexpression[0];
        if (token == '^')
        {
            //  immediate value.
            tokenList.push_back(BLAZE_NEW TokenNode(subexpression));
        }
        else 
        {
            //  command or reference.    
            error = !parseCommand(subexpression, tokenList);
        }
        expression.erase(0, tokenlen);

        if (expression[0] == '.')
        {
            //advance to next part of the expression.
            expression.erase(0, 1);         // erase the reference operator '.'
        }
    }

    if (error)
    {
        ERR_LOG("[GameReportExpression].parseExpression() : syntax error.");
    }

    return !error;

}

//  each of these methods will modify the expression string as tokens are pushed onto the stack.
//  stuff command (including parameters) onto token stack.
bool GameReportExpression::parseCommand(eastl::string& expression, TokenNode::List& tokenList)
{
    using namespace eastl;
    
    if (expression.empty())
        return false;
    
    string::size_type endTokenPos = 0;
    bool isCommand = false;

    if (expression[0] == '$')
    {     
        //  strip out $__COMMAND__$ 
        endTokenPos = expression.find_first_of('$', 1);
        
        //  not a command then error out.
        if (endTokenPos == string::npos)
            return false;
    
        isCommand = true;
    }
    else
    {
        //  strip out reference.
        endTokenPos = expression.find_first_of('[', 0);
        
        //  not a parameterized reference - treat as a simple one.
        if (endTokenPos == string::npos)
        {
            endTokenPos = expression.length();         
        } 
        endTokenPos--;      // strip '[' or point to last character in expression since we only need the reference and not the parameter.
    }

    string::size_type tokenLen = endTokenPos+1;
    string tokenStr = expression.substr(0, tokenLen);    
    expression.erase(0, tokenLen);          // remnant expression will be empty or containt parameters.

    //  generate token and parameters (if any.)
    TokenNode *token = BLAZE_NEW TokenNode(tokenStr);
    bool hasParameter = false;
    bool error = !parseParameter(expression, token, hasParameter);
    if (!error)
    {
        if (isCommand && (tokenStr == "$DNF$" || tokenStr == "$GAMEATTRS$" || tokenStr == "$PLAYERATTRS$" 
            || tokenStr == "$GAMESETTINGS$" || tokenStr == "$PLAYERSETTINGS$" || tokenStr == "$GAMEINFO$"))
        {
            if (!hasParameter)
            {
                error = true;       // need a parameter, flag this as an error.
            }
        }
    }
    if (error)
    {
        delete token;
    }
    else
    {
        //  push token
        tokenList.push_back(token);
    }

    return !error;
}


//  parse parameter [] section from expression.
bool GameReportExpression::parseParameter(eastl::string& expression, TokenNode *parentToken, bool& hasParameter)
{
    using namespace eastl;
    
    bool error = false;
    //  push parameters ahead of command: format [__PARAM__]
    //  because a parameter can be an expression, need to find the matching ']' to the first '['
    string::size_type leftTokenPos = expression.find_first_of('[', 0);
    hasParameter = leftTokenPos != string::npos;

    if (hasParameter)
    {
        uint32_t levels = 1;
        string::size_type midParameterPos = leftTokenPos;
        do
        {
            midParameterPos = expression.find_first_of(",[]", midParameterPos+1);
            if (midParameterPos != string::npos)
            {
                char8_t ch = expression[midParameterPos];
                if (ch == ',')
                {
                    //  parse an argument (comma on top-level of parameter.
                    if (levels == 1)
                    {
                        string::size_type argTokenLen = (midParameterPos - leftTokenPos) - 1;
                        string innerexpression = expression.substr(leftTokenPos+1, argTokenLen);
                        TokenNode::List& tokenList = parentToken->argList.push_back();
                        error = !parseExpression(innerexpression, tokenList);
                        expression.erase(leftTokenPos, (midParameterPos - leftTokenPos));
                        leftTokenPos = 0;           // ',' is definitely in the expression
                        midParameterPos = leftTokenPos;
                    }                    
                }
                else if (ch == '[')
                {
                    levels++;
                }
                else if (ch == ']')
                {
                    //  parse the last argument in the parameter.
                    if (levels == 1)
                    {
                        string::size_type argTokenLen = (midParameterPos - leftTokenPos) - 1;
                        string innerexpression = expression.substr(leftTokenPos+1, argTokenLen);
                        TokenNode::List& tokenList = parentToken->argList.push_back();
                        error = !parseExpression(innerexpression, tokenList);
                        expression.erase(leftTokenPos, (midParameterPos - leftTokenPos) + 1);
                        leftTokenPos = expression.empty() ? string::npos : 0;
                        midParameterPos = leftTokenPos;
                    }
                    levels--;
                }
            }
        }
        while (!error && levels > 0 && midParameterPos != string::npos);
        if (levels != 0)
        {
            //  mismatched bracket error
            error = true;
        }
    }

    return !error;
}


} //namespace Blaze
} //namespace GameReporting
