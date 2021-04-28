/*! ************************************************************************************************/
/*!
    \file gamestatehelper.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamestatehelper.h"
#include "gamemanager/gamesession.h"

namespace Blaze 
{
namespace GameManager 
{
    GameStateHelper::GameStateHelper() : mActionsByState(BlazeStlAllocator("GameStateHelper::mActionsByState", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
        // static rules of game state related logic

        //NEW_STATE state transitions
        addPossibleGameStateTransition(NEW_STATE, INITIALIZING);
        addPossibleGameStateTransition(NEW_STATE, RESETABLE);
        addPossibleGameStateTransition(NEW_STATE, INACTIVE_VIRTUAL);

        //NEW_STATE actions allowed.
        addPossibleAction(NEW_STATE, HOST_JOIN_GAME);
        addPossibleAction(NEW_STATE, PLAYER_MAKE_RESERVATION);
        addPossibleAction(NEW_STATE, HOST_ASSIGNMENT);
        addPossibleAction(NEW_STATE, GAME_CAPACITY_RESIZE);

        // --- GameState.GAME_GROUP_INITIALIZED --
        addPossibleGameStateTransition(GAME_GROUP_INITIALIZED, MIGRATING);

        //GAME_GROUP_INITIALIZED actions allowed.
        addPossibleAction(GAME_GROUP_INITIALIZED, HOST_JOIN_GAME);
        addPossibleAction(GAME_GROUP_INITIALIZED, HOST_ASSIGNMENT);
        addPossibleAction(GAME_GROUP_INITIALIZED, PLAYER_JOIN_GAME);
        addPossibleAction(GAME_GROUP_INITIALIZED, PLAYER_MAKE_RESERVATION);
        addPossibleAction(GAME_GROUP_INITIALIZED, PLAYER_CLAIM_RESERVATION);
        addPossibleAction(GAME_GROUP_INITIALIZED, GAME_SETTING_UPDATE);
        addPossibleAction(GAME_GROUP_INITIALIZED, GAME_CAPACITY_RESIZE);

        // --- GameState.RESETABLE --
        addPossibleGameStateTransition(RESETABLE, INITIALIZING);
        
        // --- GameState.INITIALIZING ---
        addPossibleGameStateTransition(INITIALIZING, PRE_GAME);
        addPossibleGameStateTransition(INITIALIZING, RESETABLE);    // return available dedicated server back to the pool if player does not actually play
        addPossibleGameStateTransition(INITIALIZING, INACTIVE_VIRTUAL);
        addPossibleGameStateTransition(INITIALIZING, CONNECTION_VERIFICATION);
        addPossibleGameStateTransition(INITIALIZING, GAME_GROUP_INITIALIZED);

        //INITIALIZING actions allowed.
        addPossibleAction(INITIALIZING, HOST_JOIN_GAME);
        addPossibleAction(INITIALIZING, PLAYER_JOIN_GAME); // for creating games with a matchmaking player pool, or joining a freshly reset dedicated server
        addPossibleAction(INITIALIZING, PLAYER_MAKE_RESERVATION);
        addPossibleAction(INITIALIZING, PLAYER_CLAIM_RESERVATION);
        addPossibleAction(INITIALIZING, GAME_SETTING_UPDATE);
        addPossibleAction(INITIALIZING, GAME_CAPACITY_RESIZE);

        // --- GameState.CONNECTION_VERIFICATION ---
        addPossibleGameStateTransition(CONNECTION_VERIFICATION, INITIALIZING);
        addPossibleGameStateTransition(CONNECTION_VERIFICATION, PRE_GAME);
        addPossibleGameStateTransition(CONNECTION_VERIFICATION, RESETABLE);    // return available dedicated server back to the pool if connection test fails
        addPossibleGameStateTransition(CONNECTION_VERIFICATION, INACTIVE_VIRTUAL);

        //CONNECTION_VERIFICATION actions allowed.
        addPossibleAction(CONNECTION_VERIFICATION, PLAYER_CLAIM_RESERVATION);

        // --- GameState.VIRTUAL ---
        addPossibleGameStateTransition(INACTIVE_VIRTUAL, INITIALIZING);
        addPossibleGameStateTransition(INACTIVE_VIRTUAL, DESTRUCTING);

        //VIRTUAL actions allowed.
        addPossibleAction(INACTIVE_VIRTUAL, GAME_SETTING_UPDATE);
        addPossibleAction(INACTIVE_VIRTUAL, GAME_CAPACITY_RESIZE);
        addPossibleAction(INACTIVE_VIRTUAL, HOST_ASSIGNMENT);
        addPossibleAction(INACTIVE_VIRTUAL, HOST_JOIN_GAME);
        addPossibleAction(INACTIVE_VIRTUAL, PLAYER_JOIN_GAME);
        addPossibleAction(INACTIVE_VIRTUAL, PLAYER_MAKE_RESERVATION);
        addPossibleAction(INACTIVE_VIRTUAL, PLAYER_CLAIM_RESERVATION);

        // --- GameState.PRE_GAME ---
        addPossibleGameStateTransition(PRE_GAME, MIGRATING);
        addPossibleGameStateTransition(PRE_GAME, IN_GAME);
        addPossibleGameStateTransition(PRE_GAME, RESETABLE);    // return available dedicated server back to the pool if player does not actually play
        addPossibleGameStateTransition(PRE_GAME, INACTIVE_VIRTUAL);

        //PRE_GAME actions allowed.
        addPossibleAction(PRE_GAME,HOST_JOIN_GAME);
        addPossibleAction(PRE_GAME,PLAYER_JOIN_GAME);
        addPossibleAction(PRE_GAME,PLAYER_MAKE_RESERVATION);
        addPossibleAction(PRE_GAME,PLAYER_CLAIM_RESERVATION);
        addPossibleAction(PRE_GAME,GAME_SETTING_UPDATE);
        addPossibleAction(PRE_GAME,GAME_CAPACITY_RESIZE);

        // --- GameState.IN_GAME ---
        addPossibleGameStateTransition(IN_GAME, MIGRATING);
        addPossibleGameStateTransition(IN_GAME, POST_GAME);
        addPossibleGameStateTransition(IN_GAME, INACTIVE_VIRTUAL);

        // IN_GAME actions allowed
        addPossibleAction(IN_GAME,PLAYER_JOIN_GAME);
        addPossibleAction(IN_GAME,PLAYER_MAKE_RESERVATION);
        addPossibleAction(IN_GAME,PLAYER_CLAIM_RESERVATION);
        addPossibleAction(IN_GAME,GAME_SETTING_UPDATE);
        addPossibleAction(IN_GAME,GAME_CAPACITY_RESIZE);

        // --- GameState.POST_GAME ---
        addPossibleGameStateTransition(POST_GAME, PRE_GAME);            // for replay
        addPossibleGameStateTransition(POST_GAME, RESETABLE);       // return available dedicated server back to the pool
        addPossibleGameStateTransition(POST_GAME, DESTRUCTING);     // destroy game
        addPossibleGameStateTransition(POST_GAME, MIGRATING);
        addPossibleGameStateTransition(POST_GAME, INACTIVE_VIRTUAL);

        // --- POST_GAME actions allowed
        addPossibleAction(POST_GAME, GAME_CAPACITY_RESIZE); // for dedicated server titles with map/mode rotation to change teams between rounds
        addPossibleAction(POST_GAME, GAME_SETTING_UPDATE); // for dedicated server titles that may need to change settings between rounds

        // --- GameState.MIGRATING ---
        addPossibleGameStateTransition(MIGRATING, PRE_GAME);
        addPossibleGameStateTransition(MIGRATING, IN_GAME);
        addPossibleGameStateTransition(MIGRATING, POST_GAME);
        addPossibleGameStateTransition(MIGRATING, RESETABLE);
        addPossibleGameStateTransition(MIGRATING, INACTIVE_VIRTUAL);
        addPossibleGameStateTransition(MIGRATING, UNRESPONSIVE);
        addPossibleGameStateTransition(MIGRATING, GAME_GROUP_INITIALIZED);

        // MIGRATING actions allowed
        addPossibleAction(MIGRATING, HOST_JOIN_GAME);
        addPossibleAction(MIGRATING, PLAYER_JOIN_GAME);
        addPossibleAction(MIGRATING, GAME_SETTING_UPDATE);
        addPossibleAction(MIGRATING, GAME_CAPACITY_RESIZE);
        
        // --- GameState.DESTRUCTING ---

        // --- GameState.UNRESPONSIVE ---

        // these are the only transitions that are allowed outside the regular responsive/unresponsive flow
        addPossibleGameStateTransition(UNRESPONSIVE, MIGRATING);
        addPossibleGameStateTransition(UNRESPONSIVE, INACTIVE_VIRTUAL);

        addPossibleAction(UNRESPONSIVE, GAME_SETTING_UPDATE);
        addPossibleAction(UNRESPONSIVE, GAME_CAPACITY_RESIZE);
    }
    
    
    GameStateHelper::~GameStateHelper()
    {
        // free possible actions
        ActionsByState::iterator actionMapIter = mActionsByState.begin();
        ActionsByState::iterator actionMapEnd = mActionsByState.end();
        for ( ; actionMapIter!=actionMapEnd; ++actionMapIter )
        {
            delete actionMapIter->second;
        }

        // Note: game state transitions don't need to be deleted

    }
    
    void GameStateHelper::addPossibleGameStateTransition(GameState from, GameState to)
    {
        GameStateTransition transition(from, to);
        mGameStateTransitions.insert(transition);
    }
    
    void GameStateHelper::addPossibleAction(GameState state, ActionType action)
    {
        ActionSet* actions = mActionsByState[state];
        
        if(actions==nullptr)
        {
            actions = BLAZE_NEW ActionSet();
            mActionsByState[state] = actions;
        }
        
        actions->insert(action);
    }
    
    bool GameStateHelper::isStateChangeAllowed(GameState currentGameState, GameState newState)
    {
        GameStateTransition transition(currentGameState, newState);
        GameStateTransitions::iterator itr = mGameStateTransitions.find(transition);
        
        if(itr!=mGameStateTransitions.end())
        { 
            return true;
        }
        
        return false;
    }
    
    
    BlazeRpcError GameStateHelper::isActionAllowed(const GameSession *gameSession, ActionType actionType) const
    {
        GameState gameState = gameSession->getGameState();
        
        BlazeRpcError failureError = (gameState == UNRESPONSIVE) ? GAMEMANAGER_ERR_UNRESPONSIVE_GAME_STATE : GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION;
        ActionsByState::const_iterator itr = mActionsByState.find(gameState);
        
        if (itr==mActionsByState.end())
        {
            return failureError;
        }
        
        ActionSet* actions = itr->second;

        ActionSet::const_iterator actionItr = actions->find(actionType);
        
        if (actionItr==actions->end())
            return failureError;

        return ERR_OK;
    }

}  // namespace GameManager
}  // namespace Blaze
