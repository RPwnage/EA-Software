/*! ************************************************************************************************/
/*!
\file voting.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_VOTING_H
#define BLAZE_GAMEMANAGER_VOTING_H

#include "EASTL/vector.h"
#include "gamemanager/tdf/matchmaker_config_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    // TODO: Have the Voting System take into account the possible and desired
    // values so the using classes don't have to.
    class Voting
    {
         NON_COPYABLE(Voting);
     public:
         typedef eastl::vector<uint16_t> VoteTally; // numVotes
 
         static const char8_t* GENERICRULE_VOTING_METHOD_KEY; //!< config map key for rule definition's voting method ("votingMethod")


    public:
        Voting();
        virtual ~Voting();

        /*! ************************************************************************************************/
        /*!
            \brief get the RuleVotingMethod enum from a rule's config map.  Returns false (and logs error) if the string
                enum value is invalid/unknown/missing.

            \param[in]  ruleConfig - a config map containing a rule definition
            \param[out] votingMethod - the enum value parsed
            \return true on success, false if the votingMethod is missing/invalid
        *************************************************************************************************/
        bool configure(const RuleVotingMethod& votingMethod);

        RuleVotingMethod getVotingMethod() const { return mVotingMethod; }
        void setVotingMethod(RuleVotingMethod votingMethod) { mVotingMethod = votingMethod; }

        size_t vote(const VoteTally& voteTally, size_t totalVoteCount, RuleVotingMethod votingMethod) const;

     /*! ************************************************************************************************/
     /*! \brief Static logging helper: write the supplied votingMethod into dest using the matchmaking config format.

         Ex: "<somePrefix>  votingMethod = <someVotingMethod>\n"
 
         \param[out] dest - a destination eastl string to print into
         \param[in] prefix - a string that's appended to the weight's key (to help with indenting)
         \param[in] votingMethod - the votingMethod value to print
     ***************************************************************************************************/
     static void writeVotingMethodToString(eastl::string &dest, const char8_t* prefix, RuleVotingMethod votingMethod);

    private:
        size_t voteLottery(VoteTally voteTally, size_t totalVoteCount) const;
        size_t votePlurality(VoteTally voteTally) const;

    // Members
    private:
        RuleVotingMethod mVotingMethod;
    };
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_VOTING_H
