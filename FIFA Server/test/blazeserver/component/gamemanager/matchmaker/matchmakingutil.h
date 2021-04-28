/*! ************************************************************************************************/
/*!
    \file   matchmakingutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_MATCHMAKING_UTIL_H
#define BLAZE_GAMEMANAGER_MATCHMAKING_UTIL_H

#include "EASTL/vector.h"
#include "EATDF/tdf.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/tdf/gamemanager_server.h"

namespace Blaze
{
    class UserSession;

namespace GameManager
{
namespace Matchmaker
{
    class MatchmakingSession;
    class MinFitThresholdList;

    typedef eastl::vector<const MatchmakingSession*> MatchmakingSessionList;
    typedef eastl::vector<Collections::AttributeValue> PossibleValueContainer;
    typedef eastl::vector<UserExtendedDataValue> TeamUEDVector; // List of matchmaking session's UED values in a team.

    // a map which has <literal, fitscore> value type
    typedef eastl::map<eastl::string, float> FitscoreByLiteralMap; //!< fitscore literal vs fitscore map
    
    const float FIT_PERCENT_NO_MATCH = -1.0f;
    const FitScore FIT_SCORE_NO_MATCH = UINT32_MAX;
    const float ZERO_FIT = 0.0f;
    const uint32_t NO_MATCH_TIME_SECONDS = UINT32_MAX;
    extern const char8_t* LOG_PREFIX;
    const uint16_t INITIAL_SESSION_AGE = 0;  // Initial value for mSessionAgeSeconds

    /*! ************************************************************************************************/
    /*!
        \brief Returns true if the supplied fitScore is considered to be a match (>= 0)
        \param[in] fitScore - a fitScore to test (can be weighted or not)
        \return true if the fitScore is considered to be a match
    ***************************************************************************************************/
    inline bool isFitScoreMatch(FitScore fitScore) { return (fitScore != FIT_SCORE_NO_MATCH); }
    inline bool isFitPercentMatch(float fitPercent) { return (fitPercent != FIT_PERCENT_NO_MATCH); }

    /*! ************************************************************************************************/
    /*! \brief Specify the possible value type a rule's threshold list could have in the config file

        Note: value type for a threshold could combine below types
    *************************************************************************************************/
    enum ThresholdValueType
    {
        ALLOW_NUMERIC     = 0x1, //!< the threshold allows numeric value
        ALLOW_EXACT_MATCH = 0x2, //!< the threshold allows exact match
        ALLOW_LITERALS    = 0x4  //!< the threshold allows literal values
    };

    //   RULE_EVAL_MODE_IGNORE_MIN_FIT_THRESHOLD is used during createGame finalization,
    //   as we calc the fitScore of each player to the newly created game they're joining.
    //   Due to voting, it's possible that a joining player wouldn't normally match the game's
    //   final values (however, since the player matched the session, we join them to the game
    //   anyways).
    enum RuleEvaluationMode
    {
        RULE_EVAL_MODE_NORMAL  = 0,              //!< regular rule evaluation (find or create game matchmaking sessions, before finalization)
        RULE_EVAL_MODE_IGNORE_MIN_FIT_THRESHOLD, //!< rule evaluation that calculates a weightedFitScore even if the rule doesn't pass the minFitThreshold.
        RULE_EVAL_MODE_LAST_THRESHOLD            //!< rule evaluation uses the last (largest) fit threshold/range
    };

    enum UpdateThresholdResult
    {
        NO_RULE_CHANGE,
        RULE_CHANGED
    };

    struct UpdateThresholdStatus
    {
        UpdateThresholdResult reteRulesResult;
        UpdateThresholdResult nonReteRulesResult;
    };

    //////////////////////////////////////////////////////////////////////////
    // MM_AUDIT: TODO move ThresholdValueInfo as an internal to minfitthresholdlistcontainer and rename
    //////////////////////////////////////////////////////////////////////////
    class ThresholdValueInfo
    {
        NON_COPYABLE(ThresholdValueInfo);
    
    public:
        ThresholdValueInfo()
            :mThresholdAllowedValueType(ALLOW_NUMERIC|ALLOW_EXACT_MATCH)
        {};
        ~ThresholdValueInfo(){};

        /*! ************************************************************************************************/
        /*! \brief set threshold allowed value type to the object

            \return true if the passed in value is different as the current value otherwise false
        ***************************************************************************************************/
        bool setThresholdAllowedValueType(const uint8_t thresholdValueType)
        {
            if (mThresholdAllowedValueType == thresholdValueType)
                return false;

            mThresholdAllowedValueType = thresholdValueType;

            return true;
        };

        /*! ************************************************************************************************/
        /*! \brief retrieve supported threshold type of the rule

            \return supported threshold value type of rule
        ***************************************************************************************************/
        const uint8_t getThresholdAllowedValueType() const{ return mThresholdAllowedValueType; }

        /*! ************************************************************************************************/
        /*! \brief retrieve fitsore based on passed in threshold literal 

            \param[in] literalValue - literal value of the threshold
            \return fitscore of the given literal value, -1 if the literal is not found in the rule
        ***************************************************************************************************/
        const float getFitscoreByThresholdLiteral(const char8_t* literalValue) const
        {
            FitscoreByLiteralMap::const_iterator iter = mLiteralFitscoreMap.find(literalValue);
            if (iter == mLiteralFitscoreMap.end()) 
                return -1;

            return (*iter).second;
        }

        /*! ************************************************************************************************/
        /*! \brief retrieve fitsore based on passed in threshold literal 

            \param[in] literalValue - literal value of the threshold
            \param[in] fitscore  - fitscore for the literal value
            \return true if the rule support the literal type minthreshold otherwise false
        ***************************************************************************************************/
        bool insertLiteralValueType(const char8_t* literalValue, const float fitscore)
        {
            if (mThresholdAllowedValueType & ALLOW_LITERALS)
            {
                mLiteralFitscoreMap.insert(FitscoreByLiteralMap::value_type(literalValue, fitscore));
                return true;
            }

            return false;
        }

        /*! ************************************************************************************************/
        /*! \brief return the literal fitscore map of the rule, if the rule does not support literal type
            it will return an empty map

            \return pointer to the map held by the rule
        ***************************************************************************************************/
        const FitscoreByLiteralMap* getLiteralFitscoreMap() const
        {
            return &mLiteralFitscoreMap;
        }

    private:
        uint8_t mThresholdAllowedValueType;
        FitscoreByLiteralMap mLiteralFitscoreMap; //!< a map holds <literal, fitscore> value type the rule has if it allows literal type
    };

class MatchmakingUtil
{
public:
    /*! ************************************************************************************************/
    /*!
        \brief Helper: validate & initialize the tdf string value.  Returns false on fatal errors.
    
        \param[in]  value - the value to store.  Cannot be nullptr/empty; Reference the size of the passed
                    in TDF as tdfValue for the required length restrictions.
        \param[out] tdfValue - the TDF string to initialize.
        \return true on success, false on error
    *************************************************************************************************/
    static bool initAndValidateValue(const char8_t* value, EA::TDF::TdfString& tdfValue);

    /*! ************************************************************************************************/
    /*!
        \brief getLocalUserData
            Gets the local user data for a given dataKey for the user associated with the
            session in the user extended data.
    
        \param[in] session The user session of the user.
        \param[in] dataKey The key associated with the data in the user extended data.
        \param[in/out] dataValue The Value of that key for this sessions user.
        \return true on successfully fetching the user data.
    *************************************************************************************************/
    static bool getLocalUserData(const GameManager::UserSessionInfo& sessionInfo, UserExtendedDataKey dataKey, UserExtendedDataValue& dataValue);
    
 
     /*! ************************************************************************************************/
     /*! \brief Static logging helper: write the supplied weight into dest in using matchmaking config format.

         Ex: "<somePrefix>  weight = <someWeight>\n"
 
         \param[out] dest - a destination eastl string to print into
         \param[in] prefix - a string that's appended to the weight's key (to help with indenting)
         \param[in] weight - the weight value to print
     ***************************************************************************************************/
     static void writeWeightToString(eastl::string &dest, const char8_t* prefix, uint32_t weight);

     /*! ************************************************************************************************/
     /*! \brief Static logging helper: write the supplied User Extended Data Name into dest using the matchmaking config format.

         Ex: "<prefix> <configKey> = <name>\n"

         \param[out] dest - a destination eastl string to print into
         \param[in] prefix - a string that's appended to the weight's key (to help with indenting)
         \param[in] configKey - string key used by the mm config to identify the UED name
         \param[in] name - The User Extended Data Name to print
     ***************************************************************************************************/
     static void writeUEDNameToString(eastl::string &dest, const char8_t* prefix, const char8_t* configKey, const UserExtendedDataName& name);
};

    /*! ************************************************************************************************/
    /*!
        \brief Container class to hold a string detailing the results of a matchmaking rule evaluation failure.  The class
        is log-aware, and will only format the result message if the supplied logLevel is enabled.

        Some of the matchmaking result messages are a little expensive to build/format.  We've wrapped
        the message buffer in this class so we can no-op writing the message if the log level isn't enabled.
    ***************************************************************************************************/
    class MatchFailureMessage
    {
        NON_COPYABLE(MatchFailureMessage);
    public:
        
        /*! ************************************************************************************************/
        /*! \brief construct an MatchFailureMessage instance that only writes messages if the supplied logLevel is enabled.
            \param[in] logLevel - the logLevel required for this obj to write into its buffer 
        ***************************************************************************************************/
        MatchFailureMessage(Logging::Level logLevel = Logging::TRACE)
            : mLogLevel(logLevel)
        {
            mMsgBuffer[0] = '\0';
        }


        /*! ************************************************************************************************/
        /*! \brief returns true if this message can be written to (checks log level)
            \return true if the message can be written to.
        ***************************************************************************************************/
        bool isEnabled() const
        {
            return IS_LOGGING_ENABLED(mLogLevel);
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
        void writeMessage(const char8_t* formatString, ...) ATTRIBUTE_PRINTF_CHECK(2,3);

        /*! ************************************************************************************************/
        /*!
            \brief return the result message (or "" if none was written or the logLevel isn't enabled).
            \return "", if no message was written (or if the logLevel isn't enabled); otherwise the msg.
        ***************************************************************************************************/
        const char8_t* getMessage() const { return mMsgBuffer; }

    private:
        Logging::Level mLogLevel;
        char8_t mMsgBuffer[1024];
    };

    // corresponds to SessionsMatchInfo in legacymatchmakingrule.h
    struct SessionsMatchFailureInfo
    {
        SessionsMatchFailureInfo(MatchFailureMessage &myFailureMessage, MatchFailureMessage &otherFailureMessage) 
            : mMyFailureMessage(myFailureMessage),
              mOtherFailureMessage(otherFailureMessage)
        {
        }

        MatchFailureMessage &mMyFailureMessage; // eval myself to otherSession
        MatchFailureMessage &mOtherFailureMessage; // eval otherSession to myself

        NON_COPYABLE(SessionsMatchFailureInfo);
    };



    struct EvalInfo
    {
        EvalInfo(const MatchmakingSession &session)
            :   mFitPercent(FIT_PERCENT_NO_MATCH),
            mIsExactMatch(false),
            mSession(session)
        {
        }

        float mFitPercent; // fit percent, or FIT_PERCENT_NO_MATCH
        bool mIsExactMatch;
        const MatchmakingSession &mSession;

        NON_COPYABLE(EvalInfo);
    };

    struct SessionsEvalInfo
    {
        SessionsEvalInfo(const MatchmakingSession &session, const MatchmakingSession &otherSession)
            : mMyEvalInfo(session),
            mOtherEvalInfo(otherSession)
        {
            // MM_TODO: can probably init failMsg from session (I think it's cached there)
        }

        EvalInfo mMyEvalInfo;
        EvalInfo mOtherEvalInfo;

        NON_COPYABLE(SessionsEvalInfo);
    };

     /*! ************************************************************************************************/
    /*!
        \brief Container class to hold the list of strings containing the debug rule conditions.

        Every rule has a set of debug conditions, and the aggregate load of building all of them can be excessive.
        This prevents us from constructing strings that won't be utilized.
    ***************************************************************************************************/
    class ReadableRuleConditions
    {
        NON_COPYABLE(ReadableRuleConditions);
    public:
        
        /*! ************************************************************************************************/
        /*! \brief construct an MatchFailureMessage instance that only writes messages if the supplied logLevel is enabled.
            \param[in] logLevel - the logLevel required for this obj to write into its buffer 
        ***************************************************************************************************/
        ReadableRuleConditions()
            : mIsDebugSession(false)
        {
        }


        /*! ************************************************************************************************/
        /*! \brief returns true if this message can be written to (checks log level)
            \return true if the message can be written to.
        ***************************************************************************************************/
        inline bool isEnabled() const { return mIsDebugSession; }

        void setIsDebugSession() { mIsDebugSession = true; }

        void clearRuleConditions() { mDebugRuleConditionsList.clear(); }
        void clearCurrentRuleConditionString() { mCurrentDebugRuleCondition.clear(); }

        /*! ************************************************************************************************/
        /*!
            \brief append to the current rule conditions string
                NOTE: call saveRuleConditions when the string is complete

            The point of this method is to bail on the message string formatting if the message wouldn't be stored.
            So, we take the desired log level in the ctor, save it off, and only format a message if the logLevel is enabled.
        
            \param[in] formatString a printf style format string
        ***************************************************************************************************/
        void writeRuleCondition(const char8_t* formatString, ...) ATTRIBUTE_PRINTF_CHECK(2,3);

        /*! ************************************************************************************************/
        /*!
            \brief return the rule conditions list, may be empty if
        ***************************************************************************************************/
        const ReadableRuleConditionList& getRuleConditions() const { return mDebugRuleConditionsList; }

    private:
        bool mIsDebugSession;

        ReadableRuleConditionList mDebugRuleConditionsList;
        eastl::string mCurrentDebugRuleCondition;
    };

    // Information needed to determine if and when a session will match.
    struct MatchInfo
    {
        FitScore sFitScore;
        uint32_t sMatchTimeSeconds;
        ReadableRuleConditions sDebugRuleConditions;


        MatchInfo() : sFitScore(FIT_SCORE_NO_MATCH), sMatchTimeSeconds(NO_MATCH_TIME_SECONDS) {}

        void setMatch(FitScore fitScore, uint32_t matchTimeSeconds)
        {
            sFitScore = fitScore;
            sMatchTimeSeconds = matchTimeSeconds;
        }

        void setNoMatch()
        {
            sFitScore = FIT_SCORE_NO_MATCH;
            sMatchTimeSeconds = NO_MATCH_TIME_SECONDS;
        }
    };

    // Session Match information for two sessions.
    struct SessionsMatchInfo
    {
        MatchInfo sMyMatchInfo;
        MatchInfo sOtherMatchInfo;
    };

    struct AggregateSessionMatchInfo : public SessionsMatchInfo
    {


        AggregateSessionMatchInfo(bool isDebugSession = false) 
            : sIsDebugSession(isDebugSession)
        {
            sMyMatchInfo.setMatch(0, 0);
            if (sIsDebugSession)
            {
                sMyMatchInfo.sDebugRuleConditions.setIsDebugSession();
            }
            sOtherMatchInfo.setMatch(0, 0);
        }

        void clear()
        {
            sMyMatchInfo.setMatch(0, 0);
            sOtherMatchInfo.setMatch(0, 0);
            sRuleResultMap.clear();
        }

        // we only need one of these for the debug session
        DebugRuleResultMap sRuleResultMap;

        bool aggregate(const SessionsMatchInfo& newRuleMatchInfo)
        {
            bool otherMatchesMySession = false;
            if ( isFitScoreMatch(sMyMatchInfo.sFitScore) )
            {
                // only bother aggregating if the previous rule(s) matched
                otherMatchesMySession = aggregate( sMyMatchInfo, newRuleMatchInfo.sMyMatchInfo );
            }

            bool mySessionMatchesOther = false;
            if ( isFitScoreMatch(sOtherMatchInfo.sFitScore) )
            {
                // only bother aggregating if the previous rule(s) matched
                mySessionMatchesOther = aggregate( sOtherMatchInfo, newRuleMatchInfo.sOtherMatchInfo );
            }

            return ( otherMatchesMySession || mySessionMatchesOther );
        }

        // return true if both my match and the other match aren't matches;
        //  used to 'early out' of aggregation (no point to tallying further if neither session ever matches)
        bool isMisMatch() const 
        {
            return ( !isFitScoreMatch(sMyMatchInfo.sFitScore) && !isFitScoreMatch(sOtherMatchInfo.sFitScore) );
        }

        void setMisMatch()
        {
            sMyMatchInfo.setNoMatch();
            sOtherMatchInfo.setNoMatch();
        }

    private:
        static bool aggregate(MatchInfo &aggregateMatchInfo, const MatchInfo& newMatchInfo)
        {
            if (isFitScoreMatch(newMatchInfo.sFitScore))
            {
                aggregateMatchInfo.sFitScore += newMatchInfo.sFitScore;
                aggregateMatchInfo.sMatchTimeSeconds = eastl::max(aggregateMatchInfo.sMatchTimeSeconds, newMatchInfo.sMatchTimeSeconds);
                return true;
            }
            else
            {
                aggregateMatchInfo.sFitScore = FIT_SCORE_NO_MATCH;
                aggregateMatchInfo.sMatchTimeSeconds = NO_MATCH_TIME_SECONDS;
                return false;
            }
        }

        bool sIsDebugSession;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_MATCHMAKING_UTIL_H
