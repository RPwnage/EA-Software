/**************************************************************************************************/
/*! 
    \file statemachine.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef STATEMACHINE_H
#define STATEMACHINE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/debug.h"
#include "BlazeSDK/blaze_eastl/map.h"

namespace Blaze
{

// Types
typedef int32_t StateId;

// Constants
static const StateId NULL_STATE_ID = -1;

/*! ***********************************************************************************************/
/*! \class StateMachine
    
    \brief Abstract base class that handles transitions within a collection of states.
***************************************************************************************************/
template<class StateClass, uint32_t NUM_STATES>
class StateMachine
{
public:
    virtual ~StateMachine() {}
    
    /*! *******************************************************************************************/
    /*! \brief Performs a transition to the specified state.
        
        \param newStateId The ID of the new state.
    ***********************************************************************************************/
    virtual void changeState(StateId newStateId, StateId stateNext = -1, StateId statePrev = -1)
    {
        if ((mCurrentState == nullptr) || (mCurrentState->getId() != newStateId))
        {
            StateClass* state = mStates[newStateId];
            BlazeAssert(state != nullptr);
            
            if (mCurrentState != nullptr)
            {
                mCurrentState->onExit();
            }
            mPreviousStateId = getStateId();
            mCurrentState = state;
            if (mCurrentState != nullptr)
            {
                mCurrentState->setStateHints(stateNext, statePrev);
                mCurrentState->onEntry();
            }
        }
    }
    
    /*! *******************************************************************************************/
    /*! \brief Returns the current state.
        
        \return The state.
    ***********************************************************************************************/
    virtual StateClass* getState() const { return mCurrentState; }
    
    /*! *******************************************************************************************/
    /*! \brief Returns the numeric state ID, or NULL_STATE_ID prior to first transition.
        
        \return The state ID.
    ***********************************************************************************************/
    virtual StateId getStateId() const
    {
        return (mCurrentState == nullptr) ? NULL_STATE_ID : mCurrentState->getId();
    }
    
    /*! *******************************************************************************************/
    /*! \brief Returns the state ID of the previous state, or NULL_STATE_ID before 2nd state.
        
        \return The previous state ID.
    ***********************************************************************************************/
    virtual StateId getPreviousStateId() const
    {
        return mPreviousStateId;
    }

#if !defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_PS5) && !defined(EA_PLATFORM_XBSX)
    virtual void setUseExternalLoginFlow(bool useExternalLoginFlow) 
    {
        mUseExternalLoginFlow = useExternalLoginFlow;
    }
#endif

    virtual bool getUseExternalLoginFlow() const 
    { 
        return mUseExternalLoginFlow; 
    }

    virtual void setIgnoreLegalDocumentUpdate(bool ignoreLegalDocumentUpdate) 
    {
        mIgnoreLegalDocumentUpdate = ignoreLegalDocumentUpdate;
    }

    virtual bool getIgnoreLegalDocumentUpdate() const 
    { 
        //mIgnoreLegalDocumentUpdate only take effect when mUseExternalLoginFlow is true
        return mIgnoreLegalDocumentUpdate && mUseExternalLoginFlow; 
    }

protected:
    // Abstract class.
    StateMachine() : mCurrentState(nullptr), 
        mPreviousStateId(NULL_STATE_ID), 
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX)
        mUseExternalLoginFlow(true),
#else
        mUseExternalLoginFlow(false),
#endif
        mIgnoreLegalDocumentUpdate(false)
    {
        for (uint32_t i = 0; i < NUM_STATES; ++i)
        {
            mStates[i] = nullptr;
        }
    }

    StateClass* mStates[NUM_STATES];
    StateClass* mCurrentState;
    StateId     mPreviousStateId;
    bool mUseExternalLoginFlow;
    bool mIgnoreLegalDocumentUpdate;

    NON_COPYABLE(StateMachine)
};

/*! ***********************************************************************************************/
/*! \class State
    
    \brief Base class for each state in a state machine.
***************************************************************************************************/
template<class StateMachineClass>
class State
{
public:
    State(StateMachineClass& stateMachine, StateId id) : mStateMachine(stateMachine), mId(id),
        mNextStateHint(-1), mPrevStateHint(-1) {}
    virtual ~State() {}
    
    /*! *******************************************************************************************/
    /*! \brief Called when transitioning to the state.
    ***********************************************************************************************/
    virtual void onEntry() {}
    
    /*! *******************************************************************************************/
    /*! \brief Called when transitioning from the state.
    ***********************************************************************************************/
    virtual void onExit() {}

    /*! *******************************************************************************************/
    /*! \brief Returns the numeric state ID.
        
        \return The state ID.
    ***********************************************************************************************/
    StateId getId() const { return mId; }
    
    /*! ***************************************************************************/
    /*! \brief Sets the state hints for the next and previous states.
    
        In some cases a state may have multiple entry and exit points.  This call allows
        the state to have context as far as where its supposed to go next.

        \param nextState The next state this state should go to.
        \param prevState The previous state this state should go to.
    *******************************************************************************/
    virtual void setStateHints(StateId nextState, StateId prevState) { mNextStateHint = nextState; mPrevStateHint = prevState; }
protected:
    StateMachineClass& mStateMachine;
    StateId            mId;
    StateId            mNextStateHint;
    StateId            mPrevStateHint;
    
    NON_COPYABLE(State)
};

} // Blaze

#endif // STATEMACHINE_H
