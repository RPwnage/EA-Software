/*! ************************************************************************************************/
/*!
    \file roleentrycriteriaevaluator.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_ROLE_ENTRY_CRITERIA_EVALUATOR_H
#define BLAZE_GAMEMANAGER_ROLE_ENTRY_CRITERIA_EVALUATOR_H

#include "gamemanager/tdf/gamemanager.h" // for tdf types
#include "util/tdf/utiltypes.h"

#include "framework/util/entrycriteria.h"

namespace Blaze
{


namespace GameManager
{


    /*! ************************************************************************************************/
    /*! \brief a wrapper class for role entry criteria evaluation

    ***************************************************************************************************/
    class RoleEntryCriteriaEvaluator : public EntryCriteriaEvaluator
    {
        NON_COPYABLE(RoleEntryCriteriaEvaluator);
    
    public:
        RoleEntryCriteriaEvaluator(const RoleName& roleName, const EntryCriteriaMap& roleEntryCriteria);
        ~RoleEntryCriteriaEvaluator() override;

        //! \brief returns the name of the role this evaluator represents, unique per game-session
        const RoleName& getRoleName() const { return mRoleName;}


        /*! ************************************************************************************************/
        /*! Implementing the EntryCriteriaEvaluator interface
            ************************************************************************************************/
        
        /*! ************************************************************************************************/
        /*! \brief Accessor for the entry criteria for this entry criteria evaluator.  These are string
                representations of the entry criteria defined by the client or configs.

            \return EntryCriteriaMap reference to the entry criteria for this entry criteria evaluator.
        *************************************************************************************************/
        const EntryCriteriaMap& getEntryCriteria() const override { return mEntryCriteriaMap; }
        
        /*! ************************************************************************************************/
        /*! \brief Accessor for the entry criteria for this entry criteria evaluator by criteria name.  These are string
                representations of the entry criteria defined by the client or configs.

            \return EntryCriteria* pointer to the requested entry criteria, nullptr if no criteria exists with the given name.
        *************************************************************************************************/
        const EntryCriteria* getEntryCriteria(const EntryCriteriaName& criteriaName) const;

        /*! ************************************************************************************************/
        /*!
            \brief Accessor for the entry criteria expressions for this entry criteria evaluator.  Expressions
                are used to evaluate the criteria against a given user session.

            \return ExpressionMap reference to the entry criteria expressions for this entry criteria evaluator.
        *************************************************************************************************/
        ExpressionMap& getEntryCriteriaExpressions() override { return mExpressionMap; }
        const ExpressionMap& getEntryCriteriaExpressions() const override { return mExpressionMap; }



    private:

        RoleName mRoleName;
        EntryCriteriaMap mEntryCriteriaMap;
        ExpressionMap mExpressionMap;
    };


    class PlayerRoster;
    class GameSession;

    /*! ************************************************************************************************/
    /*! \brief a wrapper class for multirole entry criteria evaluation
                This structure needs access to the role values of the game session (or matchmaking session) that is currently being used. 
    ***************************************************************************************************/
    class MultiRoleEntryCriteriaEvaluator : public EntryCriteriaEvaluator
    {
        NON_COPYABLE(MultiRoleEntryCriteriaEvaluator);
    public:
        MultiRoleEntryCriteriaEvaluator();
        ~MultiRoleEntryCriteriaEvaluator() override;

        // Validation check: Forced to create the entry criteria, but can be used to skip createCriteriaExpressions() if you pass in an ExpressionMap
        static bool validateEntryCriteria(const RoleInformation& roleInformation, EntryCriteriaName& failedCriteria, ExpressionMap* outMap);

        // Helper function that can be used with validateEntryCriteria to skip createCriteriaExpressions
        void setMutableRoleInfoSource(const GameSession& gameSession);
        void setStaticRoleInfoSource(const RoleInformation& roleInformation);
        void setEntryCriteriaExpressions(const ExpressionMap& map);
        int32_t createCriteriaExpressions() override;

        ExpressionMap& getEntryCriteriaExpressions() override { return mExpressionMap; }
        const ExpressionMap& getEntryCriteriaExpressions() const override { return mExpressionMap; }
        const EntryCriteriaMap& getEntryCriteria() const override;


        // Functions used when joining an existing game:
        bool evaluateEntryCriteria(const PlayerRoster* baseRoster, const RoleName& joinRoleName, TeamIndex team, EntryCriteriaName& failedCriteria) const;
        bool evaluateEntryCriteria(const PlayerRoster* baseRoster, const RoleSizeMap& joinRoleSizeMap, TeamIndex team, EntryCriteriaName& failedCriteria) const;
        
        // Functions used when creating a game, or to test a whole team:
        bool evaluateEntryCriteria(const RoleSizeMap& joinRoleSizeMap, EntryCriteriaName& failedCriteria) const;
        bool evaluateEntryCriteria(const RoleNameToPlayerMap& roleJoinRoster, uint16_t numJoining, EntryCriteriaName& failedCriteria) const;
        bool evaluateEntryCriteria(const PlayerJoinData& playerJoinData, uint16_t numJoining, EntryCriteriaName& failedCriteria) const;

    private:
        struct ResolveRoleCriteriaInfo
        {
            ResolveRoleCriteriaInfo() :
                mRoleCriteriaMap(nullptr),
                mTeam(0),
                mJoinRoleName(nullptr),
                mJoinRoleSizeMap(nullptr),
                mNumJoining(0),
                mJoinRoleRoster(nullptr),
                mBaseRoleSizeMap(nullptr),
                mBasePlayerRoster(nullptr),
                mPlayerJoinData(nullptr),
                mSuccess(nullptr)
            {}

            const RoleCriteriaMap* mRoleCriteriaMap;
            TeamIndex mTeam;

            const RoleName* mJoinRoleName;               // Used for Single User join game
            const RoleSizeMap* mJoinRoleSizeMap;         // Used in CreateGame Finalization
            uint16_t mNumJoining;
            const RoleNameToPlayerMap* mJoinRoleRoster;  // Used in Multiuser join game, and Create Game

            const RoleSizeMap* mBaseRoleSizeMap;         // Used in CreateGame Finalization
            const PlayerRoster* mBasePlayerRoster;       // Used in JoinGame
        
            const PlayerJoinData* mPlayerJoinData;

            bool* mSuccess; // using ptr to get around const void* param because this indicates if the resolve succeeded or not
        };
        const RoleInformation& getRoleInfo() const;
        bool evaluateEntryCriteria(ResolveRoleCriteriaInfo& info, EntryCriteriaName& failedCriteria) const;
        static void resolveCriteriaVariable(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type, const void* context, Blaze::Expression::ExpressionVariableVal& val);
        static int32_t validateEntryCriteriaExpressions(ExpressionMap& expressions, const RoleInformation& roleInformation, EntryCriteriaName* failedCriteria=nullptr);

        ExpressionMap mExpressionMap;
        const GameSession* mGameSession; // mutable role info used for evaluating game on the master and search slave
        const RoleInformation* mStaticRoleInformation; // immutable role info used for evaluating mm session and create game finalization
    };
} //GameManager
} // Blaze

#endif // BLAZE_GAMEMANAGER_ROLE_ENTRY_CRITERIA_EVALUATOR_H
