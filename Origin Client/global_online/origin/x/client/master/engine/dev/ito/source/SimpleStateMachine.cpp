#include "SimpleState.h"
#include "SimpleStateMachine.h"

namespace Origin
{
    namespace Engine
    {
        SimpleStateMachine::SimpleStateMachine() :
	        m_pCurrentState(NULL)
        {
	
        }

        SimpleStateMachine::~SimpleStateMachine()
        {
	        RemoveAllStates();
        }

        void SimpleStateMachine::EnterState(SimpleState *pNewState)
        {
	        if(pNewState)
	        {
		        pNewState->OnEnter(m_ScratchPad);
		        m_pCurrentState = pNewState;

		        emit stateChanged(m_pCurrentState->GetName());
	        }
        }

        void SimpleStateMachine::UpdateState()
        {
	        if(m_pCurrentState)
	        {
		        m_pCurrentState->OnUpdate(m_ScratchPad);
	        }
        }

        void SimpleStateMachine::LeaveState()
        {
	        if(m_pCurrentState)
	        {
		        m_pCurrentState->OnLeave(m_ScratchPad);
		        m_pCurrentState = NULL;

		        // Clear the command
		        m_ScratchPad.SetValue("Command", "");
	        }
        }



        bool SimpleStateMachine::SetInitialState(SimpleState *pState)
        {
	        // Was an initial state specified?
	        if(pState == NULL)
	        {
		        // Do we have states.
		        if(m_States.isEmpty())
			        return false;

		        // Take the first state in the graph.
		        pState = m_States[0];
	        }

	        // Is the specified state different from the state already set, and the state exists in the graph.
	        if(Contains(pState))
	        {
		        // Switch to this state.
		        EnterState(pState);

		        return true;
	        }
	        return false;
        }

        void SimpleStateMachine::RemoveAllStates()
        {
	        for(QVector<SimpleState *>::iterator i=m_States.begin(); i!=m_States.end(); ++i)
	        {
		        delete *i;
	        }	

	        m_States.clear();
        }

        SimpleState * SimpleStateMachine::AddState(QString name, OnSimpleStateEnter onEnterFunc, OnSimpleStateUpdate onUpdateFunc, OnSimpleStateLeave onLeaveFunc, void *pContext)
        {
	        if(FindState(name))
	        {
		        return NULL;
	        }
	        else
	        {
		        SimpleState *pNewState = new SimpleState(name, onEnterFunc, onUpdateFunc, onLeaveFunc, pContext);

		        m_States.push_back(pNewState);

		        return pNewState;
	        }
        }

        SimpleState * SimpleStateMachine::FindState(QString name)
        {
	        for(QVector<SimpleState *>::iterator i=m_States.begin(); i!=m_States.end(); ++i)
	        {
		        if((*i)->GetName() == name)
		        {
			        return *i;
		        }
	        }
	        return NULL;
        }

        void SimpleStateMachine::Update()
        {
	        while(m_pCurrentState)
	        {
		        SimpleState *pNextState = m_pCurrentState->GetNextState(m_ScratchPad);

		        if(pNextState != NULL)
		        {
			        if(pNextState == m_pCurrentState)
			        {
				        LeaveState();
				        EnterState(pNextState);
				        return;
			        }

			        LeaveState();
			        EnterState(pNextState);
		        }
		        else
		        {
			        UpdateState();
			        return;
		        }
	        }
        }

        void SimpleStateMachine::Command(QString command)
        {
	        m_ScratchPad.SetValue("Command", command);

	        Update();
        }

        bool SimpleStateMachine::Goto(QString stateName)
        {
	        SimpleState * pNextState = FindState(stateName);

	        if(pNextState)
	        {
		        m_ScratchPad.ClearChanged();

		        LeaveState();
		        EnterState(pNextState);
	
		        if(m_ScratchPad.HasChanged())
		        {
			        Update();
		        }
		        return true;
	        }
	        return false;
        }


        QVariant SimpleStateMachine::GetVariable(QString name)
        {
	        return m_ScratchPad.GetValue(name);
        }

        void SimpleStateMachine::SetVariable(QString name, QVariant val)
        {
	        m_ScratchPad.SetValue(name, val);
        }
    }
}