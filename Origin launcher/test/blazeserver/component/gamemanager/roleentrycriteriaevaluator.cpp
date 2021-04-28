/*! ************************************************************************************************/
/*!
    \file roleentrycriteriaevaluator.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesession.h"
#include "gamemanager/roleentrycriteriaevaluator.h"

namespace Blaze
{
namespace GameManager
{

    RoleEntryCriteriaEvaluator::RoleEntryCriteriaEvaluator(const RoleName& roleName, const EntryCriteriaMap& roleEntryCriteria)
        : mRoleName(roleName),
          mExpressionMap(BlazeStlAllocator("RoleCriteriaEvaluator::mExpressionMap", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
        roleEntryCriteria.copyInto(mEntryCriteriaMap);
    }

    RoleEntryCriteriaEvaluator::~RoleEntryCriteriaEvaluator()
    {
        // free any parsed game entry criteria expressions
        clearCriteriaExpressions();
    }

    /*! ************************************************************************************************/
    /*! \brief Accessor for the entry criteria for this entry criteria evaluator by criteria name.  These are string
            representations of the entry criteria defined by the client or configs.

        \return EntryCriteria* pointer to the requested entry criteria, nullptr if no criteria exists with the given name.
    *************************************************************************************************/
    const EntryCriteria* RoleEntryCriteriaEvaluator::getEntryCriteria(const EntryCriteriaName& criteriaName) const
    {
        EntryCriteriaMap::const_iterator criteriaIter = mEntryCriteriaMap.find(criteriaName);
        if (criteriaIter != mEntryCriteriaMap.end())
        {
            return &criteriaIter->second;
        }

        return nullptr;
    }


    MultiRoleEntryCriteriaEvaluator::MultiRoleEntryCriteriaEvaluator()
        : mExpressionMap(BlazeStlAllocator("RoleCriteriaEvaluator::mExpressionMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mGameSession(nullptr),
          mStaticRoleInformation(nullptr)
    {
    }

    MultiRoleEntryCriteriaEvaluator::~MultiRoleEntryCriteriaEvaluator()
    {
        clearCriteriaExpressions();
    }

    bool MultiRoleEntryCriteriaEvaluator::validateEntryCriteria(const RoleInformation& roleInformation, EntryCriteriaName& failedCriteria, ExpressionMap* outMap)
    {
        ExpressionMap tempMap;
        if (outMap == nullptr)
        {
            outMap = &tempMap;
        }


        // Attempt to create all of the criteria (for real, not just for errors):
        bool success = EntryCriteriaEvaluator::validateEntryCriteria(roleInformation.getMultiRoleCriteria(), failedCriteria, outMap);
        if (success)
        {
            success = (validateEntryCriteriaExpressions(*outMap, roleInformation, &failedCriteria) == 0);
        }
        
        // Clear the output map if we're just using the temp one:
        if (outMap == &tempMap)
        {
            ExpressionMap& expressionMap = *outMap;
            ExpressionMap::iterator i = expressionMap.begin();
            ExpressionMap::iterator e = expressionMap.end();

            // Free all of the memory and clear the map.
            for (; i != e; ++i)
            {
                delete i->second;
            }
            expressionMap.clear();
        }

        return success;
    }

    void MultiRoleEntryCriteriaEvaluator::setStaticRoleInfoSource(const RoleInformation& roleInformation)
    {
        mGameSession = nullptr;
        mStaticRoleInformation = &roleInformation;
    }

    void MultiRoleEntryCriteriaEvaluator::setMutableRoleInfoSource(const GameSession& gameSession)
    {
        mGameSession = &gameSession;
        mStaticRoleInformation = nullptr;
    }

    int32_t MultiRoleEntryCriteriaEvaluator::validateEntryCriteriaExpressions(ExpressionMap& expressions, const RoleInformation& roleInformation, EntryCriteriaName* failedCriteria)
    {
        int32_t numErrors = 0;

        // Test all of the entry criteria with an empty roster:  
        bool resolveSuccess = false;
        ResolveRoleCriteriaInfo info;
        info.mSuccess = &resolveSuccess;
        info.mRoleCriteriaMap = &roleInformation.getRoleCriteriaMap();

        // Matchmaker load testing showed that constructing this functor was a time sink, so it's static now
        static Expression::ResolveVariableCb resolveCriteriaVariableCb(&MultiRoleEntryCriteriaEvaluator::resolveCriteriaVariable);

        // This will evaluate all of the criteria with 0 players joining, and 0 players in the roster:
        // all criteria should pass in this case, since all games will start in an empty state (and shouldn't prevent initial players from joinining)
        ExpressionMap::iterator itr = expressions.begin();
        while (itr != expressions.end())
        {
            EntryCriteriaName criteriaName = itr->first.c_str();
            Blaze::Expression* expression = itr->second;

            int64_t eval = expression->evalInt(resolveCriteriaVariableCb, &info);
            if (eval == 0 || !*info.mSuccess)
            {
                ++numErrors;

                if (failedCriteria != nullptr)
                    *failedCriteria = criteriaName;
                
                const EntryCriteriaMap& entryCriteriaMap = roleInformation.getMultiRoleCriteria();
                const char8_t* stringExpression = entryCriteriaMap.find(criteriaName)->second.c_str();
                BLAZE_ERR_LOG(Log::UTIL, "[MultiRoleEntryCriteriaEvaluator].createCriteriaExpressions(): failed the following criteria when using an empty team: "
                    "entry criteria name \"" << criteriaName.c_str() << "\" expression \"" << stringExpression << "\" (criteria ignored).");

                // Remove the invalid criteria
                delete expression;
                itr = expressions.erase(itr);
            }
            else
            {
                ++itr;
            }
        }        

        return numErrors;
    }

    void MultiRoleEntryCriteriaEvaluator::setEntryCriteriaExpressions(const ExpressionMap& map) 
    { 
        // Note: This takes over ownership of any dynamically created expressions' memory
        clearCriteriaExpressions(); 
        mExpressionMap = map; 
    }

    int32_t MultiRoleEntryCriteriaEvaluator::createCriteriaExpressions()
    {
        EA_ASSERT_MSG(mStaticRoleInformation != nullptr || mGameSession != nullptr, "Role information or Game Session required!");
        int32_t numErrors = EntryCriteriaEvaluator::createCriteriaExpressions();
        numErrors += validateEntryCriteriaExpressions(getEntryCriteriaExpressions(), getRoleInfo());
        return numErrors;
    }


    const EntryCriteriaMap& MultiRoleEntryCriteriaEvaluator::getEntryCriteria() const
    {
        return getRoleInfo().getMultiRoleCriteria();
    }

    const RoleInformation& MultiRoleEntryCriteriaEvaluator::getRoleInfo() const
    {
        if (mGameSession != nullptr)
            return mGameSession->getRoleInformation();
        return *mStaticRoleInformation;
    }

    bool MultiRoleEntryCriteriaEvaluator::evaluateEntryCriteria(const PlayerRoster* baseRoster, const RoleName& joinRoleName, TeamIndex team, EntryCriteriaName& failedCriteria) const
    {
        ResolveRoleCriteriaInfo info;
        info.mTeam = team;
        info.mJoinRoleName = &joinRoleName;
        info.mBasePlayerRoster = baseRoster;

        return evaluateEntryCriteria(info, failedCriteria);
    }
    bool MultiRoleEntryCriteriaEvaluator::evaluateEntryCriteria(const PlayerRoster* baseRoster, const RoleSizeMap& joinRoleSizeMap, TeamIndex team, EntryCriteriaName& failedCriteria) const
    {
        ResolveRoleCriteriaInfo info;
        info.mTeam = team;
        info.mJoinRoleSizeMap = &joinRoleSizeMap;
        info.mBasePlayerRoster = baseRoster;

        return evaluateEntryCriteria(info, failedCriteria);
    }
    bool MultiRoleEntryCriteriaEvaluator::evaluateEntryCriteria(const PlayerJoinData& playerJoinData, uint16_t numJoining, EntryCriteriaName& failedCriteria) const
    {
        ResolveRoleCriteriaInfo info;
        info.mNumJoining = numJoining;
        info.mPlayerJoinData = &playerJoinData;

        return evaluateEntryCriteria(info, failedCriteria);
    }
    bool MultiRoleEntryCriteriaEvaluator::evaluateEntryCriteria(const RoleSizeMap& joinRoleSizeMap, EntryCriteriaName& failedCriteria) const
    {
        ResolveRoleCriteriaInfo info;
        info.mJoinRoleSizeMap = &joinRoleSizeMap;

        return evaluateEntryCriteria(info, failedCriteria);
    }
    bool MultiRoleEntryCriteriaEvaluator::evaluateEntryCriteria(const RoleNameToPlayerMap& roleJoinRoster, uint16_t numJoining, EntryCriteriaName& failedCriteria) const
    {
        ResolveRoleCriteriaInfo info;
        info.mNumJoining = numJoining;
        info.mJoinRoleRoster = &roleJoinRoster;

        return evaluateEntryCriteria(info, failedCriteria);
    }

    bool MultiRoleEntryCriteriaEvaluator::evaluateEntryCriteria(ResolveRoleCriteriaInfo& info, EntryCriteriaName& failedCriteria) const
    {
        bool resolveSuccess = false;
        info.mSuccess = &resolveSuccess;
        info.mRoleCriteriaMap = &getRoleInfo().getRoleCriteriaMap();

        // Matchmaker load testing showed that constructing this functor was a time sink, so it's static now
        static Expression::ResolveVariableCb resolveCriteriaVariableCb(&MultiRoleEntryCriteriaEvaluator::resolveCriteriaVariable);

        const ExpressionMap& expressions = getEntryCriteriaExpressions();
        ExpressionMap::const_iterator itr = expressions.begin();
        ExpressionMap::const_iterator end = expressions.end();
        for (; itr != end; ++itr)
        {
            const Blaze::Expression* expression = itr->second;

            int64_t eval = expression->evalInt(resolveCriteriaVariableCb, &info);
            if (eval == 0 || !*info.mSuccess)
            {
                failedCriteria = itr->first.c_str();
                return false;
            }
        }

        return true;
    }

    void MultiRoleEntryCriteriaEvaluator::resolveCriteriaVariable(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type, const void* context, Blaze::Expression::ExpressionVariableVal& val)
    {
        const ResolveRoleCriteriaInfo* info = static_cast<const ResolveRoleCriteriaInfo*>(context);
        *(info->mSuccess) = false;

        // need this init to avoid a memory checking tool error; the value will be irrelevant if the resolve is not successful
        val.intVal = 0;

        if (type == Blaze::EXPRESSION_TYPE_INT)
        {
            // Look up the data key using the name.
            if (info->mRoleCriteriaMap->find(name) == info->mRoleCriteriaMap->end())
            {
                BLAZE_ERR_LOG(Log::UTIL, "[MultiRoleEntryCriteriaEvaluator].resolveCriteriaVariable(): role not found for name '" << name << "'");
                return;
            }

            // Grab the number of players in that role in-game already + the number of joining players for that role:
            uint16_t roleCount = 0;

            // Base:
            if (info->mBasePlayerRoster)
            {
                roleCount += info->mBasePlayerRoster->getRoleSize(info->mTeam, name);
            }
            else if (info->mBaseRoleSizeMap)
            {
                RoleSizeMap::const_iterator iter = info->mBaseRoleSizeMap->find(RoleName(name));
                if (iter != info->mBaseRoleSizeMap->end())
                    roleCount += iter->second;
            }
            else if (info->mPlayerJoinData)
            {
                PerPlayerJoinDataList::const_iterator curIter = info->mPlayerJoinData->getPlayerDataList().begin();
                PerPlayerJoinDataList::const_iterator endIter = info->mPlayerJoinData->getPlayerDataList().end();
                for (; curIter != endIter; ++curIter)
                {
                    RoleName roleName = (*curIter)->getRole();
                    if (roleName[0] == '\0')
                    {
                        roleName = info->mPlayerJoinData->getDefaultRole();
                    }

                    if (blaze_stricmp(roleName, name) == 0)
                        ++roleCount;
                }
            }

            // Joining:
            if (info->mJoinRoleRoster)
            {
                roleCount += lookupRoleCount(*info->mJoinRoleRoster, RoleName(name), info->mNumJoining);
            }
            else if (info->mJoinRoleName && blaze_stricmp(name, info->mJoinRoleName->c_str()) == 0)
            {
                ++roleCount;
            }
            else if (info->mJoinRoleSizeMap)
            {
                RoleSizeMap::const_iterator iter = info->mJoinRoleSizeMap->find(RoleName(name));
                if (iter != info->mJoinRoleSizeMap->end())
                    roleCount += iter->second;
            }

            val.intVal = roleCount;
            *(info->mSuccess) = true;
        }
    }

}// namespace GameManager
}// namespace Blaze
