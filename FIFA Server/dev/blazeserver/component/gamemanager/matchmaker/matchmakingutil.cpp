/*! ************************************************************************************************/
/*!
    \file   matchmakingutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "framework/usersessions/usersession.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/minfitthresholdlist.h"
#include "gamemanager/matchmaker/matchmakingconfig.h"  // Voting Method Tokens, TODO: combine this with tostring into a parse method.
#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    const char8_t* LOG_PREFIX = "[Matchmaker]: ";

    bool MatchmakingUtil::initAndValidateValue(const char8_t* value, EA::TDF::TdfString& tdfValue)
    {
        // non-nullptr, non-empty
        if ( (value == nullptr) || (value[0] == '\0') )
        {
            TRACE_LOG("[MatchMakingUtil].initAndValidateName() - invalid value.");
            return false;
        }

        tdfValue.set(value);
        return true;
    }

    bool MatchmakingUtil::getLocalUserData(const GameManager::UserSessionInfo& sessionInfo, UserExtendedDataKey dataKey, UserExtendedDataValue& dataValue)
    {
        if (!UserSession::getDataValue(sessionInfo.getDataMap(), dataKey, dataValue))
        {
            TRACE_LOG("[MatchMakingUtil].getLocalUserData() - Cannot get value for userSessionId '" << sessionInfo.getSessionId() << "' and dataKey '" << SbFormats::HexLower(dataKey) << "'.");
            return false;
        }

        return true;
    }
 
    /*! ************************************************************************************************/
    /*! \brief Static logging helper: write the supplied weight into dest using the matchmaking config format.
 
        Ex: "<somePrefix>  weight = <someWeight>\n"
    
        \param[out] dest - a destination eastl string to print into
        \param[in] prefix - a string that's appended to the weight's key (to help with indenting)
        \param[in] weight - the weight value to print
    ***************************************************************************************************/
    void MatchmakingUtil::writeWeightToString(eastl::string &dest, const char8_t* prefix, uint32_t weight)
    {
        dest.append_sprintf("%s  %s = %u\n", prefix, RuleDefinition::RULE_DEFINITION_CONFIG_WEIGHT_KEY, weight);
    }
 
     /*! ************************************************************************************************/
     /*! \brief Static logging helper: write the supplied User Extended Data Name into dest using the matchmaking config format.
 
         Ex: "<prefix> <configKey> = <name>\n"
 
         \param[out] dest - a destination eastl string to print into
         \param[in] prefix - a string that's appended to the weight's key (to help with indenting)
         \param[in] configKey - string key used by the mm config to identify the UED name
         \param[in] name - The User Extended Data Name to print
     ***************************************************************************************************/
     void MatchmakingUtil::writeUEDNameToString(eastl::string &dest, const char8_t* prefix, const char8_t* configKey, const UserExtendedDataName& name)
     {
         dest.append_sprintf("%s  %s = %s\n", prefix, configKey, name.c_str());
     }

    /*! ************************************************************************************************/
    /*!
        \brief write the supplied printf-style message into the internal result message buffer.  NOTE:
            message is only written if this's object's log level is enabled; message will be truncated
            to fit into the buffer.

        The point of this method is to bail on the message string formatting if the message wouldn't be logged anyways.
        So, we take the desired log level in the ctor, save it off, and only format a message if the logLevel is enabled.
    
        \param[in] formatString a printf style format string
    ***************************************************************************************************/
    void MatchFailureMessage::writeMessage(const char8_t* formatString, ...)
    {
        //MM_PROFILING: could this be inlined to avoid the func call? (doubt it - MSVC at least won't inline var arg functions)
        // might be worth it to create a macro to avoid the func call
        va_list args;
        if (isEnabled())
        {
            va_start(args, formatString);
            blaze_vsnzprintf(mMsgBuffer, sizeof(mMsgBuffer), formatString, args);
            va_end(args);
        }
    }

    /*! ************************************************************************************************/
    /*!
        \brief adds the current conditions string to the rule conditions list and clears it.
            NOTE: Skips adding the string if it is empty.


        The point of this method is to bail on the message string formatting if the message wouldn't be stored.
        So, we take the desired log level in the ctor, save it off, and only format a message if the logLevel is enabled.
        
        \param[in] formatString a printf style format string
    ***************************************************************************************************/
    void ReadableRuleConditions::writeRuleCondition(const char8_t* formatString, ...)
    {
        //MM_PROFILING: could this be inlined to avoid the func call? (doubt it - MSVC at least won't inline var arg functions)
        // might be worth it to create a macro to avoid the func call
        va_list args;
        if (EA_UNLIKELY(isEnabled()))
        {
            va_start(args, formatString);
            mCurrentDebugRuleCondition.append_sprintf_va_list(formatString, args);
            va_end(args);
            // only push back non-empty strings
            if (!mCurrentDebugRuleCondition.empty())
            {
                mDebugRuleConditionsList.push_back(mCurrentDebugRuleCondition.c_str());
                mCurrentDebugRuleCondition.clear();
            }
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
