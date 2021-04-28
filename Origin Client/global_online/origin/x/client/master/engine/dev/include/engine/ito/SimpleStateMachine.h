#ifndef __SIMPLE_STATE_MACHINE_H__
#define __SIMPLE_STATE_MACHINE_H__

#include "SimpleState.h"
#include "ScratchPad.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        /// \brief Implementation of a single state machine that 
        class ORIGIN_PLUGIN_API SimpleStateMachine : public QObject
        {
	        Q_OBJECT
        public:
	        SimpleStateMachine();
	        virtual ~SimpleStateMachine();

	        // State graph functions
	        SimpleState * AddState(QString name, OnSimpleStateEnter onEnterFunc, OnSimpleStateUpdate onUpdateFunc, OnSimpleStateLeave onLeaveFunc, void *pContext = NULL);
	        bool SetInitialState(SimpleState *pState = NULL);

	        void RemoveAllStates();
	        bool Contains(SimpleState *pState){return m_States.contains(pState);} 

	        // Scratchpad functions
	        QVariant GetVariable(QString name);
	        void SetVariable(QString name, QVariant val);

	        // State 
	        void Update();
	        void Command(QString command);	   // e.g. Retry, Cancel, Ok.
	        bool Goto(QString stateName);

	        SimpleState * FindState(QString name);
	        SimpleState * GetCurrentState(){return m_pCurrentState;}
	        void * GetCurrentContext(){return (m_pCurrentState != NULL) ? m_pCurrentState->GetContext() : NULL;}
	        QString GetCurrentStateName(){return (m_pCurrentState != NULL) ? m_pCurrentState->GetName() : "";}

        signals:
	        void stateChanged(QString newStateName);

        private:
	        void EnterState(SimpleState *pNewState);
	        void UpdateState();
	        void LeaveState();

        private:
	        ScratchPad				m_ScratchPad;
	        SimpleState *			m_pCurrentState;
	        QVector<SimpleState *>	m_States;
        };
    }
}

#endif // __SIMPLE_STATE_MACHINE_H__
