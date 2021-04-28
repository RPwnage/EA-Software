/*! ************************************************************************************************/
/*!
\file gameattributerule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gameattributerule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    GameAttributeRule::GameAttributeRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mRuleOmitted(false),
        mIsRandomDesired(false),
        mIsAbstainDesired(false)
    {
    }

    GameAttributeRule::GameAttributeRule(const GameAttributeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
        mRuleOmitted(otherRule.mRuleOmitted),
        mIsRandomDesired(otherRule.mIsRandomDesired),
        mIsAbstainDesired(otherRule.mIsAbstainDesired),
        mDesiredValuesBitset(otherRule.mDesiredValuesBitset),
        mDesiredValues(otherRule.mDesiredValues),
        mDesiredValueStrs(otherRule.mDesiredValueStrs)
    {
        otherRule.mGameAttributeRuleStatus.copyInto(mGameAttributeRuleStatus);
        if (!otherRule.isDisabled())
        {
            mGameAttributeRuleStatus.setRuleName(getRuleName()); // rule name
            mMatchmakingAsyncStatus->getGameAttributeRuleStatusMap().insert(eastl::make_pair(getRuleName(), &mGameAttributeRuleStatus));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameAttributeRule::~GameAttributeRule()
    {
        mMatchmakingAsyncStatus->getGameAttributeRuleStatusMap().erase(getRuleName());
    }

    bool GameAttributeRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        const GameAttributeRuleCriteria* gameAttrRuleCriteria = getGameAttributRulePrefs(criteriaData);

        mRuleOmitted = (gameAttrRuleCriteria == nullptr);

        if (!mRuleOmitted)
        {
            mGameAttributeRuleStatus.setRuleName(getRuleName()); // rule name
            mMatchmakingAsyncStatus->getGameAttributeRuleStatusMap().insert(eastl::make_pair(getRuleName(), &mGameAttributeRuleStatus));

            // NOTE: initMinFitThreshold should be called in the initialize method of any rule that uses threshold lists.
            const char8_t* listName = gameAttrRuleCriteria->getMinFitThresholdName();
            if (!initMinFitThreshold(listName, mRuleDefinition, err))
            {
                return false;
            }

            if (isExplicitType())
            {
                if (!isDisabled() || !gameAttrRuleCriteria->getDesiredValues().empty())
                {
                    // rule is not disabled, or is present and has set desired values
                    // init the desired value bitset
                    if (!initDesiredValuesBitset(gameAttrRuleCriteria->getDesiredValues(), err))
                    {
                        return false; // err already filled out
                    }
                }
            }
            else if (isArbitraryType())
            {
                if (!isDisabled() || !gameAttrRuleCriteria->getDesiredValues().empty())
                {
                    // rule is not disabled, or is present and has set desired values
                    // init the desired value
                    if (!initDesiredValues(gameAttrRuleCriteria->getDesiredValues(), err))
                    {
                        return false; // err already filled out
                    }
                }

                if (!isDisabled())
                {
                    for (uint32_t i = 0; i < mMinFitThresholdList->getSize(); ++i)
                    {
                        float fitPercent = mMinFitThresholdList->getMinFitThresholdByIndex(i);

                        if ((fitPercent != MinFitThresholdList::EXACT_MATCH_REQUIRED) && (fitPercent < getDefinition().getMismatchFitPercent()))
                        {
                            uint32_t time = mMinFitThresholdList->getSecondsByIndex(i);

                            // Arbitrary game attributes don't do voting.  They can't
                            // because they don't know the possible values.  Which
                            // ever session finalizes first will get their choice.
                            WARN_LOG("[ArbitraryAttributeRule].initialize WARNING min fit threshold accepts mismatch for rule '" 
                                << getRuleName() << "' at threshold " << i << " '" << time << ":" << fitPercent 
                                << "' will cause undertermined game attribute durring game creation.");
                        }
                    }
                }
            }
        }

        return true;
    }





    const GameAttributeRuleCriteria* GameAttributeRule::getGameAttributRulePrefs(const MatchmakingCriteriaData &criteriaData) const
    {   
        GameAttributeRuleCriteriaMap::const_iterator iter = criteriaData.getGameAttributeRuleCriteriaMap().find(getRuleName());

        if (iter != criteriaData.getGameAttributeRuleCriteriaMap().end())
        {
            return iter->second;
        }

        return nullptr;
    }

    Rule* GameAttributeRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW GameAttributeRule(*this, matchmakingAsyncStatus);
    }

    void GameAttributeRule::voteOnGameValues(const MatchmakingSessionList& votingSessionList, GameManager::CreateGameRequest& createGameRequest) const
    {
        if (isArbitraryType())
        {
            createGameAttributeFromVote(votingSessionList, createGameRequest.getGameCreationData().getGameAttribs());
            return;
        }

        Collections::AttributeMap& gameAttribMap = createGameRequest.getGameCreationData().getGameAttribs();

        // determine this rule's voting method
        // NOTE: if this rule requires an exact match, we override the voting method and use a required value
        RuleVotingMethod votingMethod = getDefinition().getVotingMethod();
        if (MinFitThresholdList::isThresholdExactMatchRequired(mCurrentMinFitThreshold))
        {
            votingMethod = OWNER_WINS;
        }

        // MM_HACK (NHL & SPORTS 10): If the owner session picked Abstain or Random and had pulled in
        // other MM sessions which had specified actual values, a random value
        // will still be picked and those actual values ignored.  This hack 
        // allows a plurality vote if the OWNER_WINS but the owner doesn't care.
        //
        // This still doesn't stop people who require a specific value (which may not match
        // the other sessions that were picked by the match any from matching
        // the abstained owner.  This needs to get rethought through such that 
        // abstain and random don't create pools of players who can't resolve their values.
        if ((isRandomOrAbstainDesired()) && (votingMethod == OWNER_WINS))
        {
            votingMethod = VOTE_PLURALITY;
        }

        const char8_t *attribValue = nullptr;

        // tally the votes
        Voting::VoteTally voteTally;
        size_t totalVoteCount = tallyVotes(votingSessionList, voteTally, ((votingMethod == OWNER_WINS)? &mDesiredValuesBitset : nullptr));

        if (totalVoteCount == 0)
        {
            // everyone abstained
            attribValue = FitTable::ABSTAIN_LITERAL_CONFIG_VALUE;
        }
        else
        {
            if (votingMethod == OWNER_WINS)
            {
                attribValue = voteOwnerWins(voteTally);
            }
            else
            {
                attribValue = getDefinition().getPossibleValue(getDefinition().vote(voteTally, totalVoteCount, votingMethod));
            }
        }

        // if abstain won resolve the abstain literal to an attrib value.  Needs to be resolved
        // before random is resolved as uf the default abstain value is random, choose randomly.
        if (getDefinition().isValueAbstain(attribValue))
        {
            attribValue = getDefinition().getDefaultAbstainValue();
        }

        // if random won, we must choose a random value. NOTE: Random can win by the default abstain
        // value being set to random.
        if (getDefinition().isValueRandom(attribValue))
        {
            attribValue = getDefinition().getRandomPossibleValue();
        }

        // check if we're blowing away a previous value
        Collections::AttributeMap::iterator mapIter = gameAttribMap.find(getDefinition().getAttributeName());
        if (mapIter == gameAttribMap.end())
        {
            // no previous value, insert
            gameAttribMap.insert(eastl::make_pair(getDefinition().getAttributeName(),attribValue));
        }
        else
        {
            // blowing away previous value - warn.
            //  either we have two rules operating on the same attrib, or a rule is overriding a gameAttribute passed up in startMatchmaking
            TRACE_LOG("WARNING - generic rule \"" << getRuleName() << "\" is overriding an existing game attrib.  attrib: \"" 
                      << getDefinition().getAttributeName() << "\" oldValue: \"" << mapIter->second.c_str() << "\" newValue: \"" << attribValue 
                      << "\". Either 2+ rules are operating on the same game attrib, or a rule is overriding a gameAttrib value passed up in startMatchmaking...");

            mapIter->second = attribValue;
        }
    }

    void GameAttributeRule::createGameAttributeFromVote( const MatchmakingSessionList& memberSessions, Collections::AttributeMap& map ) const
    {
        // For Arbitrary rules, the Owner always wins:
        eastl::string desiredValue = "";
        if (!mDesiredValueStrs.empty())
            desiredValue = mDesiredValueStrs[Random::getRandomNumber((int)mDesiredValueStrs.size())];

        // check if we're blowing away a previous value
        Collections::AttributeMap::iterator mapIter = map.find(getDefinition().getAttributeName());
        if (mapIter != map.end())
        {
            // blowing away previous value - warn.
            // either we have two rules operating on the same attrib, or a rule is overriding a
            // gameAttribute passed up in startMatchmaking
            TRACE_LOG("WARNING - generic arbitrary rule \"" << getRuleName() << "\" is overriding an existing game attrib.  attrib: \"" 
                      << getDefinition().getAttributeName() << "\" oldValue: \"" << mapIter->second.c_str() << "\" newValue: \"" << desiredValue
                      << "\".  Either 2+ rules are operating on the same game attrib, or a rule is overriding a gameAttrib value passed up in startMatchmaking...");
        }

        // alloc & return the winning attribPair
        map.insert(eastl::make_pair(getDefinition().getAttributeName(), desiredValue.c_str()));
    } // createGameAttributeFromVote

    size_t GameAttributeRule::tallyVotes(const MatchmakingSessionList& memberSessions, Voting::VoteTally& voteTally,
        const GameAttributeRuleValueBitset* ownerWinsPossValues) const
    {
        EA_ASSERT(voteTally.empty());
        size_t numVotes = 0;
        voteTally.resize(getDefinition().getPossibleValueCount(), 0);

        // count other game member's vote(s)
        const GameAttributeRule* otherRule;
        MatchmakingSessionList::const_iterator gameMemberIter = memberSessions.begin();
        MatchmakingSessionList::const_iterator end = memberSessions.end();
        for ( ; gameMemberIter != end; ++gameMemberIter)
        {
            const MatchmakingSession* otherSession = *gameMemberIter;
            otherRule = static_cast<const GameAttributeRule*>(otherSession->getCriteria().getRuleByIndex(getRuleIndex()));
            if (otherRule != nullptr)
            {
                numVotes += otherRule->tallyPlayersVotes(otherSession->getPlayerCount(), voteTally, ownerWinsPossValues);
            }
        }
        return numVotes;
    }

    /*! ************************************************************************************************/
    /*! \brief scan this rule's desired values, add them to the voteTally. If there is more than 1 user
    in the game member (user group session), we need to take into account all players since they
    share same settings as the group leader

    \param[in]  sessionUsersNum - number of players in session, 1 - individual session, > 1 user group session
    \param[out] voteTally - vector hold valueIndex vs valueindexCount
    \param[in] ownerWinsPossValues - if owner wins only these owner's desired values get votes. nullptr omits check.
    \return voteNum
    ***************************************************************************************************/
    size_t GameAttributeRule::tallyPlayersVotes(const uint16_t sessionUsersNum, Voting::VoteTally& voteTally,
        const GameAttributeRuleValueBitset* ownerWinsPossValues) const
    {
        // if we're abstaining for this rule, cast no votes
        if (mIsAbstainDesired)
        {
            return 0;
        }

        // cast a vote for each value that we desire
        size_t numVotes = 0;
        size_t possibleValueIndex = mDesiredValuesBitset.find_first();
        while (possibleValueIndex < mDesiredValuesBitset.kSize)
        {
            if ((ownerWinsPossValues == nullptr) || ownerWinsPossValues->test(possibleValueIndex))
            {
                numVotes += sessionUsersNum;
                voteTally[possibleValueIndex] = voteTally[possibleValueIndex] + sessionUsersNum;
            }
            possibleValueIndex = mDesiredValuesBitset.find_next(possibleValueIndex);
        }

        return numVotes;
    }

    // returns one of the owner's desired values for this rule.
    // NOTE: value can be RANDOM or ABSTAIN
    const char8_t* GameAttributeRule::voteOwnerWins(const Voting::VoteTally& voteTally) const
    {
        size_t winningPossibleIndex = mDesiredValuesBitset.find_first();

        size_t numDesiredValues = mDesiredValuesBitset.count();
        if (numDesiredValues == 1)
        {
            // there's a single desired value, and we initialized the index to it already (no-op)
        }
        else
        {
            // We're going to vote, and we're going to pick the highest fit score result selected by the owning session if there's a tie.
            // this ensures we pick a value reflecting what the other sessions matched on.
            // This is to satisfy the usage of Generic Rules to handle DLC as described to game teams.
            // The fit table & fit threshold would look something like this:
            // fitTable = [ 0.4, 0.0, 0.0, 0.0, 
            //              0.0, 0.6, 0.0, 0.0, 
            //              0.0, 0.0, 0.8, 0.0, 
            //              0.0, 0.0, 0.0, 1.0 ] 
            //  four = [ 0:1.0, 10:0.8, 20:0.6, 25:0.4 ]


            winningPossibleIndex = resolveMultipleDesiredValuesOwnerWins(voteTally);
        }

        return getDefinition().getPossibleValue(winningPossibleIndex);
    }

    // Pre: voteTally not empty
    size_t GameAttributeRule::resolveMultipleDesiredValuesOwnerWins(const Voting::VoteTally& voteTally) const
    {
        eastl::vector<int32_t> winningValueIndices;
        winningValueIndices.reserve(voteTally.size());

        // find the winning iterator (with the most votes)
        Voting::VoteTally::const_iterator voteBegin = voteTally.begin();
        Voting::VoteTally::const_iterator voteEnd = voteTally.end();
        Voting::VoteTally::const_iterator voteIter = eastl::max_element(voteBegin, voteEnd);
        int32_t possibleValueIndex;

        // check to see if we had a tie for the most votes
        int32_t winningVoteCount = *voteIter;
        do
        {
            possibleValueIndex = (int32_t)(voteIter - voteBegin);
            winningValueIndices.push_back(possibleValueIndex);
            voteIter = eastl::find(++voteIter, voteEnd, winningVoteCount );
        } while(voteIter != voteEnd);

        // determine & return the winning attribute value
        // Note: at this point, random is a valid vote (it's translated into an actual random attrib value later)
        size_t numTiedValues = winningValueIndices.size();
        if ( numTiedValues == 1)
        {
            // single winner, return the value
            return winningValueIndices[0];
        }
        else
        {
            // return the best fit-scoring tied values
            eastl::vector<int32_t> winningIndicesWithBestFit;
            float highestFitPercent = 0;
            for(size_t i = 0; i < numTiedValues; ++i)
            {
                float newFitPercent = getFitPercent(winningValueIndices[i]);
                if( (newFitPercent >= highestFitPercent) && (mDesiredValuesBitset.test(winningValueIndices[i])))
                {
                    // higher fit percent than previously matched value, and contained as a desired value in the owner's session.
                    if (newFitPercent > highestFitPercent)
                    {
                        winningIndicesWithBestFit.clear();
                        highestFitPercent = newFitPercent;
                    }
                    winningIndicesWithBestFit.push_back(winningValueIndices[i]);
                }
            }
            if (winningIndicesWithBestFit.empty())
            {
                WARN_LOG("[GameAttributeRule].resolveMultipleDesiredValuesOwnerWins: possible invalid vote result for owner.");
                return winningValueIndices[0];
            }
            return winningIndicesWithBestFit[Random::getRandomNumber((int)winningIndicesWithBestFit.size())];
        }
    }

    bool GameAttributeRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        bool isExactMatch;
        float fitPercent;

        // Desired Any evaluation does not need to be part of the RETE tree, since no matter what value comes in
        // for a game, we wish to match it.
        if (isMatchAny())
        {
            // Can only have random or abstain, don't need to evaluate further.
            return false;
        }

        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& orConditions = conditions.push_back();
        const char8_t* name = getDefinition().getAttributeName();

        FitScore fitScore;

        if (isArbitraryType())
        {
            // Add matched condition if the current threshold accepts that.
            fitScore = calcWeightedFitScore(true, getDefinition().getMatchingFitPercent());

            if (isFitScoreMatch(fitScore))
            {
                for (const auto& curValue : getDesiredValues())
                    orConditions.push_back(Rete::ConditionInfo(
                        Rete::Condition(name, curValue, mRuleDefinition),
                        fitScore, this));
            }

            // Add mismatched condition if the current threshold accepts that.
            fitScore = calcWeightedFitScore(false, getDefinition().getMismatchFitPercent());

            if (isFitScoreMatch(fitScore))
            {
                for (const auto& curValue : getDesiredValues())
                    orConditions.push_back(Rete::ConditionInfo(
                        Rete::Condition(name, curValue, mRuleDefinition, true),
                        fitScore, this));
            }

            return true;
        }
        else if (isExplicitType())
        {
            // For each possible value, determine the best fitscore against all of the desired values.
            // If the fitscore is a match at the current threshold then add it to the search conditions, else skip it.
            // NOTE: this does not include the random or abstain values.
            const PossibleValueContainer& possibleActualValues = getDefinition().getPossibleActualValues();
            PossibleValueContainer::const_iterator i = possibleActualValues.begin();
            PossibleValueContainer::const_iterator e = possibleActualValues.end();

            for (; i != e; ++i)
            {
                const Collections::AttributeValue& possibleValue = *i;
                int possibleValueIndex = getDefinition().getPossibleValueIndex(possibleValue);

                calcFitPercentInternal(possibleValueIndex, fitPercent, isExactMatch);

                fitScore = calcWeightedFitScore(isExactMatch, fitPercent);

                if (isFitScoreMatch(fitScore))
                {
                    orConditions.push_back(Rete::ConditionInfo(
                        Rete::Condition(name, getDefinition().getWMEAttributeValue(possibleValue), mRuleDefinition), 
                        fitScore, this));
                }
            }

            if (orConditions.empty())
            {
                eastl::string myDesiredValues;
                buildDesiredValueListString(myDesiredValues, mDesiredValuesBitset);
                WARN_LOG("[GameAttributeRule].addConditions: possible badly configured fit table or min fit threshold lists for rule(" << getRuleName() << "), the requested search on desired value(s) (" << myDesiredValues.c_str() << ") cannot currently match any of rules possible values.");

                // empty orConditions by default would match any, for correctness/CG-consistency ensure
                // won't match anything by specifying impossible-to-match value
                orConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(name, getDefinition().getWMEAttributeValue(GameAttributeRuleDefinition::ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE), mRuleDefinition),
                    0, this));
            }
            return true;
        }

        // This should be unreachable
        return false;
    }


    void GameAttributeRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        if (isArbitraryType())
        {
            bool bDefined;
            ArbitraryValue gameArbitraryValue = gameSession.getMatchmakingGameInfoCache()->getCachedGameArbitraryValue(getDefinition(), bDefined);
            if (!bDefined)
            {
                debugRuleConditions.writeRuleCondition("%s game attribute not defined for game", getDefinition().getAttributeName());

                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
                return;
            }

            calcFitPercentInternal(gameArbitraryValue, fitPercent, isExactMatch);

            debugRuleConditions.writeRuleCondition("%s defined for game", getDefinition().getAttributeName());
        }
        else if (isExplicitType())
        {
            int gameAttribIndex = gameSession.getMatchmakingGameInfoCache()->getCachedGameAttributeIndex(getDefinition());
            if (gameAttribIndex == MatchmakingGameInfoCache::ATTRIBUTE_INDEX_NOT_DEFINED || gameAttribIndex == MatchmakingGameInfoCache::ATTRIBUTE_INDEX_IMPOSSIBLE)
            {
                debugRuleConditions.writeRuleCondition("%s game attribute not defined game", getDefinition().getAttributeName());

                fitPercent = FIT_PERCENT_NO_MATCH;
                isExactMatch = false;
                return;
            }

            calcFitPercentInternal(gameAttribIndex, fitPercent, isExactMatch);

            if (debugRuleConditions.isEnabled())
            {
                eastl::string desiredValues, otherDesiredValues;
                buildDesiredValueListString(desiredValues, mDesiredValuesBitset);
                buildDesiredValueListString(otherDesiredValues, gameAttribIndex);

                debugRuleConditions.writeRuleCondition("Game %s = %s vs %s", otherDesiredValues.c_str(), getDefinition().getAttributeName(), desiredValues.c_str());
            }
        }
    }

    void GameAttributeRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        if (isExplicitType())
        {
            const GameAttributeRule& otherGameAttributeRule = static_cast<const GameAttributeRule&>(otherRule);

            calcFitPercentInternal(otherGameAttributeRule.getDesiredValuesBitset(), fitPercent, isExactMatch);

            // MM_HACK: Adding this to exact match so that we'll match other sessions that desire abstain or random.
            isExactMatch = isExactMatch || otherGameAttributeRule.isRandomOrAbstainDesired();

            if (debugRuleConditions.isEnabled())
            {
                eastl::string desiredValues, otherDesiredValues;
                buildDesiredValueListString(desiredValues, mDesiredValuesBitset);
                buildDesiredValueListString(otherDesiredValues, otherGameAttributeRule.getDesiredValuesBitset());
                debugRuleConditions.writeRuleCondition("Other %s = %s vs %s", getDefinition().getAttributeName(), otherDesiredValues.c_str(), desiredValues.c_str());
            }
        }
        else if (isArbitraryType())
        {
            const GameAttributeRule& otherGameAttributeRule = static_cast<const GameAttributeRule&>(otherRule);

            calcFitPercentInternal(otherGameAttributeRule.getDesiredValues(), fitPercent, isExactMatch);

            if (debugRuleConditions.isEnabled())
            {
                eastl::string myDesire;
                buildDesiredValueListString(myDesire, mDesiredValueStrs);

                if (isExactMatch)
                    debugRuleConditions.writeRuleCondition("Both desire %s", myDesire.c_str());
                else
                {
                    eastl::string theirDesire;
                    buildDesiredValueListString(theirDesire, otherGameAttributeRule.getDesiredValueStrings());
                    debugRuleConditions.writeRuleCondition("%s != %s", myDesire.c_str(), theirDesire.c_str());
                }
            }
        }        
    }

    void GameAttributeRule::calcFitPercent(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& ruleConditions) const
    {
        if (mRuleOmitted || isDisabled())
        {
            // disabled/omitted, anything is a match
            fitPercent = 0;
            isExactMatch = true;
            ruleConditions.writeRuleCondition("My rule is disabled, any create game request matches.");
            return;
        }

        const char8_t* gameAttribName = getDefinition().getAttributeName();
        Blaze::Collections::AttributeMap::const_iterator attribIter = matchmakingCreateGameRequest.getCreateReq().getCreateRequest().getGameCreationData().getGameAttribs().find(gameAttribName);

        if (EA_UNLIKELY(attribIter == matchmakingCreateGameRequest.getCreateReq().getCreateRequest().getGameCreationData().getGameAttribs().end()))
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            ruleConditions.writeRuleCondition("%s attribute not found in the create game request.", gameAttribName);
            return;
        }

        if (isExplicitType())
        {
            const Collections::AttributeValue& gameAttribValue = attribIter->second;
            int attributeValueIndex = getDefinition().getPossibleValueIndex(gameAttribValue);

            calcFitPercentInternal(attributeValueIndex, fitPercent, isExactMatch);
            ruleConditions.writeRuleCondition("Create game requests value for %s = %s", getDefinition().getAttributeName(), attribIter->second.c_str());
        }
        else if (isArbitraryType())
        {
            const Blaze::GameManager::Matchmaker::ArbitraryValue& gameAttribValue = getDefinition().getWMEAttributeValue(attribIter->second);
            calcFitPercentInternal(gameAttribValue, fitPercent, isExactMatch);

            eastl::string myDesire;
            buildDesiredValueListString(myDesire, mDesiredValueStrs);
            if (isExactMatch)
                ruleConditions.writeRuleCondition("Both desire %s", myDesire.c_str());
            else
                ruleConditions.writeRuleCondition("%s != %s", myDesire.c_str(), attribIter->second.c_str());
        }
    }


    void GameAttributeRule::calcFitPercentInternal(int gameAttribValueIndex, float &fitPercent, bool &isExactMatch) const
    {
        isExactMatch = ( isRandomOrAbstainDesired() || mDesiredValuesBitset.test((size_t)gameAttribValueIndex) );
        fitPercent = getFitPercent(gameAttribValueIndex);
    }

    void GameAttributeRule::calcFitPercentInternal(const GameAttributeRuleValueBitset &otherDesiredValuesBitset, float &fitPercent, bool &isExactMatch) const
    {
        isExactMatch = ( isRandomOrAbstainDesired() || (mDesiredValuesBitset == otherDesiredValuesBitset));
        fitPercent = getFitPercent(otherDesiredValuesBitset);

        // handle exact match for multiple desired values
        // we only need to have one matching value
        if (!isExactMatch && (fitPercent != FIT_PERCENT_NO_MATCH))
        {
            GameAttributeRuleValueBitset combindedValues(mDesiredValuesBitset);
            combindedValues &= otherDesiredValuesBitset;
            isExactMatch = combindedValues.any();
        }
    }

    void GameAttributeRule::calcFitPercentInternal(ArbitraryValue otherValue, float &fitPercent, bool &isExactMatch) const
    {
        isExactMatch = (mDesiredValues.find(otherValue) != mDesiredValues.end());

        if (isExactMatch)
        {
            fitPercent = getDefinition().getMatchingFitPercent();
        }
        else
        {
            fitPercent = getDefinition().getMismatchFitPercent();
        }
    }

    void GameAttributeRule::calcFitPercentInternal(const ArbitraryValueSet &otherValues, float &fitPercent, bool &isExactMatch) const
    {
        // To avoid voting, we require that all matching CreateGame requests have the exact same attributes. 
        isExactMatch = (mDesiredValues == otherValues);

        if (isExactMatch)
        {
            fitPercent = getDefinition().getMatchingFitPercent();
        }
        else
        {
            fitPercent = getDefinition().getMismatchFitPercent();
        }
    }


    float GameAttributeRule::getFitPercent(const GameAttributeRuleValueBitset &entityValueBitset) const
    {
        // compare this rule's desired values against the other entity's values
        // return the best fit, or no match
        float maxFitPercent = FIT_PERCENT_NO_MATCH;

        size_t myIndex = mDesiredValuesBitset.find_first();
        while(myIndex < mDesiredValuesBitset.kSize)
        {
            size_t otherIndex = entityValueBitset.find_first();
            while(otherIndex < entityValueBitset.kSize)
            {
                maxFitPercent = eastl::max_alt(maxFitPercent, getDefinition().getFitPercent(myIndex, otherIndex));
                otherIndex = entityValueBitset.find_next(otherIndex);
            }

            myIndex = mDesiredValuesBitset.find_next(myIndex);
        }

        return maxFitPercent;
    }


    void GameAttributeRule::updateAsyncStatus()
    {
        if (getDefinition().getAttributeRuleType() == ARBITRARY_TYPE)
        {
            // no-op, desired async values don't change
            //  MM_TODO: in theory, we should have a way to indicate if we match ThisValue or match !ThisValue
            return;
        }

        // clear the old values
        if (MinFitThresholdList::isThresholdExactMatchRequired(mCurrentMinFitThreshold))
        {
            size_t i = mDesiredValuesBitset.find_first();
            while (i < mDesiredValuesBitset.kSize)
            {
                if (!mMatchedValuesBitset[i])
                {
                    mMatchedValuesBitset[i] = true;
                    const char8_t* possibleValue = getDefinition().getPossibleValue(i);
                    mGameAttributeRuleStatus.getMatchedValues().push_back(possibleValue);
                }
                i = mDesiredValuesBitset.find_next(i);
            }
        }
        else
        {
            // get all matched values at this point
            size_t possibleValueCount = getDefinition().getPossibleValueCount();
            for (size_t possibleValueIndex = 0; possibleValueIndex < possibleValueCount; possibleValueIndex++)
            {
                if (!mMatchedValuesBitset[possibleValueIndex])
                {
                    const char8_t* possibleValue = getDefinition().getPossibleValue(possibleValueIndex);
                    float fitscore = getFitPercent(possibleValueIndex);
                    if (fitscore >= mCurrentMinFitThreshold)
                    {
                        mMatchedValuesBitset[possibleValueIndex] = true;
                        mGameAttributeRuleStatus.getMatchedValues().push_back(possibleValue);
                    }
                }
            }
        }
    }

    void GameAttributeRule::buildDesiredValueListString(eastl::string& buffer, const GameAttributeRuleValueBitset &desiredValuesBitset) const
    {
        // build a comma separated list of desired values (into the supplied buffer)
        size_t i = desiredValuesBitset.find_first();
        while (i < desiredValuesBitset.kSize)
        {
            buffer.append_sprintf(getDefinition().getPossibleValue(i));
            i = desiredValuesBitset.find_next(i);
            if (i != desiredValuesBitset.kSize)
            {
                buffer.append_sprintf(", "); // append a comma
            }
        }
    }

    void GameAttributeRule::buildDesiredValueListString(eastl::string& buffer, const Collections::AttributeValueList &desiredValues) const
    {
        // build a comma separated list of desired values (into the supplied buffer)
        bool first = true;
        for (auto& curValue : desiredValues)
        {
            if (first)
                first = false;
            else
                buffer.append_sprintf(", "); // append a comma before the next string

            buffer.append_sprintf(curValue.c_str());
        }
    }

    bool GameAttributeRule::initDesiredValuesBitset(const Collections::AttributeValueList& desiredValuesList, MatchmakingCriteriaError &err)
    {
        mDesiredValuesBitset.reset(); // clear the bits
        mIsRandomDesired = false;
        mIsAbstainDesired = false; 

        int desiredValueIndex;
        const char8_t* desiredValue;

        // you must specify at least 1 desired value
        if (desiredValuesList.empty())
        {
            // error: each rule must specify at least 1 desired value
            char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" must define at least 1 desired value.  (If you don't care, define and use \"abstain\").",
                getDefinition().getName());
            err.setErrMessage(msg);
            return false;
        }

        // lookup the indices of random and abstain, so we can set the mDesiredValueStatus flags
        int abstainIndex = getDefinition().getPossibleValueIndex(FitTable::ABSTAIN_LITERAL_CONFIG_VALUE);
        int randomIndex = getDefinition().getPossibleValueIndex(FitTable::RANDOM_LITERAL_CONFIG_VALUE);

        // Iterate over the desiredValuesList, find each index in the ruleDefn's possibleValueList, and set the bit at that index
        Collections::AttributeValueList::const_iterator desiredValueIter = desiredValuesList.begin();
        Collections::AttributeValueList::const_iterator end = desiredValuesList.end();
        for ( ; desiredValueIter != end; ++desiredValueIter)
        {
            desiredValue = desiredValueIter->c_str();
            desiredValueIndex = getDefinition().getPossibleValueIndex(desiredValue);
            if (desiredValueIndex < 0)
            {
                // error: desired value not found in rule defn's possible value list.
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" contains an invalid desired value (\"%s\") - values must match a rule's possibleValue (in rule definition).",
                    getDefinition().getName(), desiredValue);
                err.setErrMessage(msg);
                return false;
            }

            // TODO: Remove this restriction-
            // error out if the client is passing up redundant desired values
            if (mDesiredValuesBitset.test((size_t)desiredValueIndex))
            {
                // error: client passing up duplicate (redundant) desired values
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" contains duplicate entries for desired value \"%s\".",
                    getDefinition().getName(), desiredValue);
                err.setErrMessage(msg);
                return false;
            }

            // update the desiredValueBitset & desiredValueStatus flags
            mDesiredValuesBitset.set((size_t) desiredValueIndex);
            if (desiredValueIndex == abstainIndex)
            {
                mIsAbstainDesired = true;
            }
            else
            {
                if (desiredValueIndex == randomIndex)
                {
                    mIsRandomDesired = true;
                }
            }
        }


        // check that random/abstain exist alone (ie: no such thing as wanting {"a" or "random"} )
        // Note: voting logic depends on this (we early-out if a rule is 'abstain')
        if (mDesiredValuesBitset.count() > 1)
        {
            // check random
            if (mIsRandomDesired)
            {
                // error: random (if desired), must exist in isolation
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" contains RANDOM along with other desired values.  If you use RANDOM, you cannot specify other desired values.",
                    getDefinition().getName());
                err.setErrMessage(msg);
                return false;
            }

            // check abstain
            if (mIsAbstainDesired)
            {
                // error: abstain (if desired), must exist in isolation
                char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" contains ABSTAIN along with other desired values.  If you use ABSTAIN, you cannot specify other desired values.",
                    getDefinition().getName());
                err.setErrMessage(msg);
                return false;
            }
        }

        return true; // no error
    }

    bool GameAttributeRule::initDesiredValues(const Collections::AttributeValueList& desiredValuesList, MatchmakingCriteriaError &err)
    {
        const char8_t* desiredValue = nullptr;

        // you must specify at least 1 desired value
        if (desiredValuesList.empty())
        {
            // error: each rule must specify 1 desired value
            char8_t msg[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Rule \"%s\" must define 1 desired value.", getDefinition().getName());
            err.setErrMessage(msg);
            return false;
        }

        // There should only be one desired value for arbitrary types.

        for (auto& desiredValueIter : desiredValuesList)
        {
            desiredValue = desiredValueIter.c_str();

            if (desiredValue[0] != '\0')
            {
                mDesiredValues.insert(getDefinition().getWMEAttributeValue(desiredValue));
            }
            else
            {
                ERR_LOG("[ArbitraryAttributeRule].initDesiredValues desired value cannot be empty string.");
                return false;
            }
        } // if

        desiredValuesList.copyInto(mDesiredValueStrs);

        // setup the async notification tdf - for now, the desired value(s) never change
        mDesiredValueStrs.copyInto(mGameAttributeRuleStatus.getMatchedValues());

        return true; // no error
    } // initDesiredValues

    /*! ************************************************************************************************/
    /*! \brief from SimpleRule. Overridden for GameAttributeRule-specific behavior.
        \param[in] otherRule The other rule to post evaluate.
        \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
        \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
    ***************************************************************************************************/
    void GameAttributeRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        const GameAttributeRule* otherRule = static_cast<const GameAttributeRule*>(otherSession.getCriteria().getRuleByIndex(getRuleIndex()));

        bool imDisabled = isDisabled();
        bool otherDisabled = otherRule->isDisabled();
        // prevent crash if for some reason other rule is not defined
        if (otherRule == nullptr)
        {
            ERR_LOG("[SimpleRule].evaluateForMMCreateGame Rule '" << getRuleName() << "' MM session '" 
                    << getMMSessionId() << "': Unable to find other rule by id '" << getId() << "' for MM session '" 
                    << otherSession.getMMSessionId() << "'");
            return;
        }
        // 2.11x/3.03 specs (is same for all GameAttributeRule rules):
        // omitter matchable by enablers: always no
        // omitter matchable by disabler: always yes
        if (otherRule->mRuleOmitted && !imDisabled)
        {
            // reject: other pool doesn't define rule for this attrib (same as if a game didn't define the attrib)
            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Rule not instantiated (used) by other session.");
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();
            return;
        }
        else if(mRuleOmitted && !otherDisabled)
        {
            // reject: I didn't define this rule, and the other guy isn't disabled.
            sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Rule not instantiated (used) by self.");
            sessionsMatchInfo.sMyMatchInfo.setNoMatch();
            sessionsMatchInfo.sOtherMatchInfo.setNoMatch();
            return;
        }

        if (handleDisabledRuleEvaluation(*otherRule, sessionsMatchInfo, getMaxFitScore()))
        { 
            //ensure a session with a rule disabled doesn't pull in a session with it enabled, but allow them to match.
            enforceRestrictiveOneWayMatchRequirements(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);
            otherRule->enforceRestrictiveOneWayMatchRequirements(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);
            return;
        }


        // don't eval the disabled rule(s) because they won't have a desired value set and will never match
        if (!imDisabled)
        {
            evaluateRule(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);
        }

        if (!otherDisabled)
        {
            otherRule->evaluateRule(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);
        }

        fixupMatchTimesForRequirements(sessionsEvalInfo, otherRule->mMinFitThresholdList, sessionsMatchInfo, isBidirectional());

        postEval(*otherRule, sessionsEvalInfo.mMyEvalInfo, sessionsMatchInfo.sMyMatchInfo);
        otherRule->postEval(*this, sessionsEvalInfo.mOtherEvalInfo, sessionsMatchInfo.sOtherMatchInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief local helper
        \param[in] otherRule The other rule to post evaluate.
        \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
        \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
    ***************************************************************************************************/
    void GameAttributeRule::enforceRestrictiveOneWayMatchRequirements(const GameAttributeRule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        // Ensure a session with this rule disabled can't pull in a session with it enabled
        // This check is not executed when evaluating bidirectionally.
        if (isDisabled() && (!otherRule.isDisabled()))
        {
            matchInfo.sDebugRuleConditions.writeRuleCondition("Session (%" PRIu64 ") with rule disabled can't add session (%" PRIu64 ") with rule enabled.", getMMSessionId(), otherRule.getMMSessionId());
            matchInfo.setNoMatch();
            return;
        }
    }

    FitScore GameAttributeRule::evaluateForMMCreateGameFinalization(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, ReadableRuleConditions& ruleConditions) const
    {
        if (mRuleOmitted || isDisabled())
        {
            // disabled/omitted, anything is a match
            return 0;
        }

        return SimpleRule::evaluateCreateGameRequest(matchmakingCreateGameRequest, ruleConditions);
    }

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per desired value break down metrics
    ****************************************************************************************************/
    void GameAttributeRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        // arbitrary types always only have one desired value
        if (isArbitraryType())
        {
            RuleDiagnosticSetupInfo setupInfo("desiredArbGameAttr");
            buildDesiredValueListString(setupInfo.mValueTag, mDesiredValueStrs);
            setupInfos.push_back(setupInfo);
        }
        else if (isExplicitType())
        {
            RuleDiagnosticSetupInfo setupInfo("desiredGameAttrs");
            buildDesiredValueListString(setupInfo.mValueTag, mDesiredValuesBitset);
            setupInfos.push_back(setupInfo);
        }
    }
    

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
