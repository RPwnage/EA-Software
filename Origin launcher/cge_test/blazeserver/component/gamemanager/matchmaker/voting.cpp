/*! ************************************************************************************************/
/*!
\file voting.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/voting.h"

#include "framework/util/random.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    const char8_t* Voting::GENERICRULE_VOTING_METHOD_KEY = "votingMethod";

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    Voting::Voting()
        : mVotingMethod(OWNER_WINS)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    Voting::~Voting()
    {
    }

    /*! ************************************************************************************************/
    /*!
        \brief get the RuleVotingMethod enum from a rule's config map.  Returns false (and logs error) if the string
        enum value is invalid/unknown/missing.

        \param[in]  ruleConfig - a config map containing a rule definition
        \param[out] votingMethod - the enum value parsed
        \return true on success, false if the votingMethod is missing/invalid
    *************************************************************************************************/
    bool Voting::configure(const RuleVotingMethod& votingMethod)
    {
        mVotingMethod = votingMethod;
        return true;
    }

    size_t Voting::vote(const VoteTally& voteTally, size_t totalVoteCount, RuleVotingMethod votingMethod) const
    {
        EA_ASSERT(votingMethod == VOTE_LOTTERY || votingMethod == VOTE_PLURALITY); // OWNER_WINS is not yet supported.
        size_t winningVote = 0;

        switch (votingMethod)
        {
        case VOTE_LOTTERY:
            // straw hat vote (most votes is most likely to win)
            winningVote = voteLottery(voteTally, totalVoteCount);
            break;
        case VOTE_PLURALITY:
            // value with the most votes wins
            winningVote = votePlurality(voteTally);
            break;
        case OWNER_WINS:
            // we use the session owner's value (ie: this rule's desired values)
            // TODO: Voting keeps track of the desired values to do owner wins.
            //winningVote = voteOwnerWins();
            break;
        }

        return winningVote;
    }

    size_t Voting::voteLottery(VoteTally voteTally, size_t totalVoteCount) const
    {
        // fill the ballot box(aka straw hat) from the tally
        //  For example: if our voteTally has 4 votes for "a", and 2 votes for "b"
        //   we generate a ballot box containing [ a,a,a,a,b,b ]
        //  NOTE: in actuality, the ballot box contains the possibleValueIndex of the attrib value
        eastl::vector<int32_t> ballotBox; // each element is a possibleValueIndex
        ballotBox.reserve(totalVoteCount);
        VoteTally::const_iterator voteBegin = voteTally.begin();
        VoteTally::const_iterator voteEnd = voteTally.end();
        int32_t possibleValueIndex;
        int32_t numVotes;
        for (VoteTally::const_iterator voteIter = voteBegin; voteIter != voteEnd; ++voteIter)
        {
            possibleValueIndex = (int32_t) (voteIter - voteBegin);
            numVotes = *voteIter;
            for(int32_t i=0; i<numVotes; ++i)
            {
                ballotBox.push_back(possibleValueIndex);
            }
        }

        // choose 1 vote out of the ballot box (randomly)
        // Note: at this point, random is a valid vote (it's translated into an actual random attrib value later)
        int32_t winningIndex = ballotBox[ Random::getRandomNumber( (int)ballotBox.size()) ];
        return winningIndex;
    }

    size_t Voting::votePlurality(VoteTally voteTally) const
    {
        eastl::vector<int32_t> winningValueIndices;
        winningValueIndices.reserve(voteTally.size());

        // find the winning iterator (with the most votes)
        VoteTally::const_iterator voteBegin = voteTally.begin();
        VoteTally::const_iterator voteEnd = voteTally.end();
        VoteTally::const_iterator voteIter = eastl::max_element(voteBegin, voteEnd);
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
            // return a random value from the tied values
            int32_t winningIndex = winningValueIndices[ Random::getRandomNumber( (int)numTiedValues) ];
            return winningIndex;
        }
    }
    
 
    /*! ************************************************************************************************/
    /*! \brief Static logging helper: write the supplied votingMethod into dest using the matchmaking config format.
 
        Ex: "<somePrefix>  votingMethod = <someVotingMethod>\n"
    
        \param[out] dest - a destination eastl string to print into
        \param[in] prefix - a string that's appended to the weight's key (to help with indenting)
        \param[in] votingMethod - the votingMethod value to print
    ***************************************************************************************************/
    void Voting::writeVotingMethodToString(eastl::string &dest, const char8_t* prefix, RuleVotingMethod votingMethod)
    {
        dest.append_sprintf("%s  %s = %s\n", prefix, Voting::GENERICRULE_VOTING_METHOD_KEY, RuleVotingMethodToString(votingMethod));
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
