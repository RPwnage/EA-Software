#include "SimpleState.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Engine
    {
        SimpleStateTransition::SimpleStateTransition(QString name, SimpleState *pNextState, SimpleStateTransitionTest testFunc)
        {
            m_Name = name;
            m_pNextState = pNextState;
            m_TestFunc = testFunc;
        }

        bool SimpleStateTransition::OnTest(ScratchPad &scratchPad)
        {
            if(m_TestFunc)
                return m_TestFunc(scratchPad);

            return false;
        }


        SimpleState::SimpleState(QString name, OnSimpleStateEnter onEnterFunc, OnSimpleStateUpdate onUpdateFunc, OnSimpleStateLeave onLeaveFunc, void * pContext)
        {
            m_Name = name;
            m_OnEnterFunc = onEnterFunc;
            m_OnUpdateFunc = onUpdateFunc;
            m_OnLeaveFunc = onLeaveFunc;
            m_pContext = pContext;
        }

        SimpleState::~SimpleState()
        {
            RemoveAllTransitions();
        }

        void SimpleState::RemoveAllTransitions()
        {
            for(QVector<SimpleStateTransition *>::iterator i=m_Transitions.begin(); i!=m_Transitions.end(); ++i)
            {
                delete (*i);
            }
            m_Transitions.clear();
        }

        void SimpleState::AddTransition(QString name, SimpleState *pNextState, SimpleStateTransitionTest testFunc)
        {
            SimpleStateTransition *pTransition = new SimpleStateTransition(name, pNextState, testFunc);
            m_Transitions.push_back(pTransition);
        }

        SimpleState * SimpleState::GetNextState(ScratchPad &scratchPad)
        {
            for(QVector<SimpleStateTransition *>::iterator i=m_Transitions.begin(); i!=m_Transitions.end(); ++i)
            {
                if((*i)->OnTest(scratchPad))
                {
                    ORIGIN_LOG_DEBUG << "ITE: Transition " << (*i)->GetName().utf16();
                    return (*i)->GetNextState();
                }
            }
            return NULL;
        }

        void SimpleState::OnEnter(ScratchPad &scratchPad)
        {
            ORIGIN_LOG_DEBUG << "ITE: Entering State: " << m_Name.utf16();

            if(m_OnEnterFunc)
                m_OnEnterFunc(scratchPad);
        }

        void SimpleState::OnUpdate(ScratchPad &scratchPad)
        {
            ORIGIN_LOG_DEBUG << "ITE: Updating State: " << m_Name.utf16();

            if(m_OnUpdateFunc)
                m_OnUpdateFunc(scratchPad);
        }

        void SimpleState::OnLeave(ScratchPad &scratchPad)
        {
            ORIGIN_LOG_DEBUG << "ITE: Leaving State: " << m_Name.utf16();

            if(m_OnLeaveFunc)
                m_OnLeaveFunc(scratchPad);
        }

    }
}


