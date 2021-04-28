/*! ************************************************************************************************/
/*!
    \file   hostviabilityrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/networktopologycommoninfo.h" // for isPlayerHostedTopology
#include "gamemanager/matchmaker/rules/hostviabilityrule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/gamesessionsearchslave.h"


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    HostViabilityRule::HostViabilityRule(const HostViabilityRuleDefinition &def, MatchmakingAsyncStatus* matchmakingAsyncStatus)
    : SimpleRule(def, matchmakingAsyncStatus),
      mNatType(Util::NAT_TYPE_UNKNOWN),
      mDefinition(def),
      mIsCreateGameContext(false)
    {}
    
    HostViabilityRule::HostViabilityRule(const HostViabilityRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus),
        mNatType(other.mNatType),
        mDefinition(other.mDefinition),
        mIsCreateGameContext(other.mIsCreateGameContext)
    {}


    /*! ************************************************************************************************/
    /*! \brief Init the HostViability rule.  In the future, we'll init the rule by looking up the minFitThreshold
            supplied by the client.

        \param[in]  criteriaData - data required to initialize the matchmaking rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool HostViabilityRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule not needed when doing resettable games only
            return true;
        }

        const HostViabilityRulePrefs& rulePrefs = criteriaData.getHostViabilityRulePrefs();

        // This rule only applies to Find Game and Game Browser Searches
        // but we need to update the async status if the rule is enabled for FG mode, so we store off the context
        mIsCreateGameContext = (matchmakingSupplementalData.mMatchmakingContext.isSearchingPlayers());

        // lookup & cache off the rule defn
        mNatType = matchmakingSupplementalData.mNetworkQosData.getNatType();

        // lookup the minFitThreshold list we want to use
        const char8_t* listName = rulePrefs.getMinFitThresholdName();

        bool minFitIni =  initMinFitThreshold( listName, mDefinition, err );
        if (minFitIni)
        {
            updateAsyncStatus();
        }

        return minFitIni;


}
    // from SimpleRule
    void HostViabilityRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        fitPercent = calcFitPercentInternal(gameSession.getNetworkQosData().getNatType(), gameSession.getGameNetworkTopology());
        isExactMatch = false; //Exact match doesn't make sense for host viability.
        
        debugRuleConditions.writeRuleCondition("My NAT %s versus Game's NAT %s and mesh %s", NatTypeToString(mNatType), NatTypeToString(gameSession.getNetworkQosData().getNatType()), GameNetworkTopologyToString(gameSession.getGameNetworkTopology()));
        
    }
    // from SimpleRule
    void HostViabilityRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // This rule doesn't evaluate for create game as it is disabled on init.
        fitPercent = 0.0;
        isExactMatch = true;
    }

   
    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    ****************************************************************************************************/
    void  HostViabilityRule::updateAsyncStatus()
    {
        // Threshold value expecting now
        HostViabilityRuleStatus::HostViabilityValues hostViabilityValue(HostViabilityRuleStatus::CONNECTION_UNLIKELY);
        if (mCurrentMinFitThreshold == HostViabilityRuleDefinition::CONNECTION_ASSURED)
            hostViabilityValue = HostViabilityRuleStatus::CONNECTION_ASSURED;
        else if (mCurrentMinFitThreshold == HostViabilityRuleDefinition::CONNECTION_LIKELY)
            hostViabilityValue = HostViabilityRuleStatus::CONNECTION_LIKELY;
        else if (mCurrentMinFitThreshold == HostViabilityRuleDefinition::CONNECTION_FEASIBLE)
            hostViabilityValue = HostViabilityRuleStatus::CONNECTION_FEASIBLE;
        else
            hostViabilityValue = HostViabilityRuleStatus::CONNECTION_UNLIKELY;

        mMatchmakingAsyncStatus->getHostViabilityRuleStatus().setMatchedHostViabilityValue(hostViabilityValue);
    }

    /*! ************************************************************************************************/
    /*! \brief calculates fit percentage based on current cached NatType versus targeted NatType

        \param[in]  otherValue The NatType being compared against.
        \return the UN-weighted fitScore for this rule. Will not return a negative fit score
    *************************************************************************************************/
    float HostViabilityRule::calcFitPercentInternal( Util::NatType otherNatType, GameNetworkTopology gamesTopology ) const
    {
        // hostViability is irrelevant for dedicated servers (we only connect to the dedicated 
        //  server, and it's open), so lie and act as if we're perfectly connected.
        if (!isPlayerHostedTopology(gamesTopology))
        {
            return HostViabilityRuleDefinition::CONNECTION_ASSURED;
        }

        // Default fit score is CONNECTION_UNLIKELY which is the lowest fit score along with decay 
        // in matchmaker config file, in all cases either game or session itself is strict
        // this will be the fit score of the session against another session or game

        // For full mesh topology, we attempt to ensure that a minimal number of NAT's worse
        // than moderate are able to connect into a single game.
        if (gamesTopology == PEER_TO_PEER_FULL_MESH)
        {
            //  open + anything = perfect
            if (mNatType == Util::NAT_TYPE_OPEN || otherNatType == Util::NAT_TYPE_OPEN)
            {
                // perfect game/session for this session, return max fit score
                return HostViabilityRuleDefinition::CONNECTION_ASSURED;
            }
            else if (mNatType == Util::NAT_TYPE_MODERATE)
            {
                // moderate + moderate = likely
                if (otherNatType == Util::NAT_TYPE_MODERATE)
                    return HostViabilityRuleDefinition::CONNECTION_LIKELY;
                else
                    // strict + moderate = feasible
                    return HostViabilityRuleDefinition::CONNECTION_FEASIBLE;
            }
            else if (otherNatType == Util::NAT_TYPE_MODERATE)
            {
                // strict + moderate = feasible
                return HostViabilityRuleDefinition::CONNECTION_FEASIBLE;
            }
            //  Strict + strict = unlikely

            return HostViabilityRuleDefinition::CONNECTION_UNLIKELY;
        }

        // below is for PEER_TO_PEER_PARTIAL_MESH && CLIENT_SERVER_PEER_HOSTED

        // the idea here is to provide a perfect fit when comparing good hosts to bad hosts.
        // this is an attempt to keep bad hosts (NAT_TYPE_STRICT, STRICT_SEQUENTIAL, UNKNOWN) 
        // from causing or having a bad game experience (matching to games they won't connect to)
        // by rationing "good" hosts (NAT_TYPE_OPEN, MODERATE) and preventing similar hosts from 
        // grouping together when there are other players available

        bool iAmGoodHost = (mNatType <= Util::NAT_TYPE_MODERATE); // good or moderate
        bool otherIsGoodHost = (otherNatType <= Util::NAT_TYPE_MODERATE); // good or moderate

        if (iAmGoodHost)
        {
            if (!otherIsGoodHost)
            {
                // good hosts prefer bad hosts
                return HostViabilityRuleDefinition::CONNECTION_ASSURED;
            }
            else
            {
                // good hosts avoid other good hosts
                return HostViabilityRuleDefinition::CONNECTION_LIKELY;
            }
        }
        else
        {
            if (otherIsGoodHost)
            {
                // bad hosts prefer matching with good hosts
                return HostViabilityRuleDefinition::CONNECTION_ASSURED;
            }
            else
            {
                // we're both bad hosts - this would be a terrible match
                return HostViabilityRuleDefinition::CONNECTION_UNLIKELY;
            }
        }
    }

   
    /*! ************************************************************************************************/
    /*! \brief return true if the supplied matchmaking session's NAT type matches this rule's current
            minFitThreshold.
    
        \param[in] matchmakingSession the session to test for host viability
        \return true if the session could act as host, false otherwise.
    ***************************************************************************************************/
    bool HostViabilityRule::isViableGameHost(const MatchmakingSession& matchmakingSession) const
    {
        Util::NatType hostNatType = matchmakingSession.getNetworkQosData().getNatType(); 
        float hostFitScore = 0.0f;  
        switch(hostNatType)
        {
          case Util::NAT_TYPE_OPEN:
             hostFitScore = HostViabilityRuleDefinition::CONNECTION_ASSURED;
             break;
          case Util::NAT_TYPE_MODERATE:
             hostFitScore = HostViabilityRuleDefinition::CONNECTION_LIKELY;
             break;
          default:
             hostFitScore = HostViabilityRuleDefinition::CONNECTION_UNLIKELY;
        }

        return (hostFitScore >= mCurrentMinFitThreshold);
    }

    UpdateThresholdResult HostViabilityRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        if(mIsCreateGameContext && (mMinFitThresholdList != nullptr))
        {
            // update the async status if we have a fit threshold & we're in CG mode.
            float originalThreshold = mCurrentMinFitThreshold;
            mCurrentMinFitThreshold = mMinFitThresholdList->getMinFitThreshold(elapsedSeconds, &mNextUpdate);

            if (forceUpdate || originalThreshold != mCurrentMinFitThreshold)
            {
                TRACE_LOG("[matchmakingrule].updateCachedThresholds rule '" << getRuleName() << "' forceUpdate '" << (forceUpdate ? "true" : "false") 
                    << "' elapsedSeconds '" << elapsedSeconds << "' originalThreshold '" << originalThreshold << "' currentThreshold '" 
                    << mCurrentMinFitThreshold << "'");

                updateAsyncStatus();
                return RULE_CHANGED; // note: specific rules override this method (ex: generic attribs can invalidate subSessions too)
            }

            return NO_RULE_CHANGE;
        }
        else
        {
            return Rule::updateCachedThresholds(elapsedSeconds, forceUpdate); 
        }
    }

    // host viability rule isn't evaluated for create game matchmaking; this override ensures all sessions are a match for this rule
    // during finalization, we ensure that there's at least one viable host before creating a game.
     void HostViabilityRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
     {
         sessionsMatchInfo.sMyMatchInfo.setMatch(0, 0);
         sessionsMatchInfo.sOtherMatchInfo.setMatch(0, 0);
     }
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
