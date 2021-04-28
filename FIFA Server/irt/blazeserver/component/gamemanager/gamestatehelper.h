/*! ************************************************************************************************/
/*!
    \file gamestatehelper.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_STATE_HELPER_H
#define BLAZE_GAMEMANAGER_GAME_STATE_HELPER_H

#include "gamemanager/tdf/gamemanager.h"
#include "EASTL/hash_set.h"
#include "EASTL/hash_map.h"

namespace Blaze 
{
namespace GameManager 
{
    class GameSession;

    enum ActionType
    {
        HOST_JOIN_GAME,             // 
        HOST_ASSIGNMENT,            // assigning the host of the game or changing host of the game 
        PLAYER_JOIN_GAME, 
        GAME_SETTING_UPDATE,
        PLAYER_MAKE_RESERVATION,
        GAME_CAPACITY_RESIZE,
        PLAYER_CLAIM_RESERVATION
    };
    
    class GameStateTransition
    {
    public:

        GameStateTransition()
            : mFrom(NEW_STATE),
              mTo(NEW_STATE)
        {
        }

        GameStateTransition(GameState from, GameState to)
        : mFrom(from),
          mTo(to)
        {
        }
        
        bool operator==(const GameStateTransition& rhs) const
        {
            return ( (mFrom==rhs.mFrom) && (mTo==rhs.mTo) );
        }
        
        struct Hash
        {
            size_t operator()(const Blaze::GameManager::GameStateTransition& transition) const
            {
                return static_cast<size_t>(transition.mFrom * 27 + transition.mTo);
            };
        };
    private:
        GameState mFrom;
        GameState mTo;
    };


struct GameStateHash
{
    size_t operator()(const Blaze::GameManager::GameState& state) const
    {
        return static_cast<size_t>(state);
    };
};

struct ActionTypeHash
{
    size_t operator()(const Blaze::GameManager::ActionType type) const
    {
        return static_cast<size_t>(type);
    };
};

    typedef eastl::hash_set<GameStateTransition, GameStateTransition::Hash > GameStateTransitions;
    typedef eastl::hash_set<ActionType, ActionTypeHash> ActionSet;
    typedef eastl::hash_map<GameState, ActionSet*, GameStateHash> ActionsByState;
    
    class GameStateHelper
    {
    private:
        GameStateTransitions mGameStateTransitions;
        ActionsByState mActionsByState;
        
        void addPossibleGameStateTransition(GameState from, GameState to);
        void addPossibleAction(GameState state, ActionType action);
    public:
        GameStateHelper();
        ~GameStateHelper();

        bool isStateChangeAllowed(GameState currentState, GameState newState);
        BlazeRpcError isActionAllowed(const GameSession *gameSession, ActionType actionType) const;
    };

}  // namespace GameManager
}  // namespace Blaze

#endif // #define BLAZE_GAMEMANAGER_GAME_STATE_HELPER_H
