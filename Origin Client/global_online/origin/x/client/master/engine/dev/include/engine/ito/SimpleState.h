#ifndef __SIMPLE_STATE_H__
#define __SIMPLE_STATE_H__

#include <qstring.h>
#include <qvector.h>

#include "ScratchPad.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        class SimpleState;

        typedef bool (*SimpleStateTransitionTest)(ScratchPad &scratchPad);
        typedef void (*OnSimpleStateEnter)(ScratchPad &scratchPad);
        typedef void (*OnSimpleStateUpdate)(ScratchPad &scratchPad);
        typedef void (*OnSimpleStateLeave)(ScratchPad &scratchPad);

        class ORIGIN_PLUGIN_API SimpleStateTransition
        {
        public:
            SimpleStateTransition(QString name, SimpleState *pNextState, SimpleStateTransitionTest testFunc);

            bool OnTest(ScratchPad &scratchPad);
            QString GetName(){return m_Name;}
            SimpleState * GetNextState(){return m_pNextState;}

        private:
            QString m_Name;
            SimpleState * m_pNextState;
            SimpleStateTransitionTest m_TestFunc;
        };


        class ORIGIN_PLUGIN_API SimpleState
        {
        public:
            SimpleState(QString name, OnSimpleStateEnter onEnterFunc, OnSimpleStateUpdate onUpdateFunc, OnSimpleStateLeave onLeaveFunc, void *pContext);
            virtual ~SimpleState();

            void AddTransition(QString name, SimpleState *pNextState, SimpleStateTransitionTest testFunc);
            void RemoveAllTransitions();

            SimpleState * GetNextState(ScratchPad &scratchPad);

            QString GetName(){return m_Name;}
            void * GetContext(){return m_pContext;}

            void OnEnter(ScratchPad &scratchPad);
            void OnUpdate(ScratchPad &scratchPad);
            void OnLeave(ScratchPad &scratchPad);

        private:
            QString								m_Name;
            void * m_pContext;
            QVector<SimpleStateTransition *>	m_Transitions;

            OnSimpleStateEnter					m_OnEnterFunc;
            OnSimpleStateUpdate					m_OnUpdateFunc;
            OnSimpleStateLeave					m_OnLeaveFunc;
        };
    }
}

#endif // __SIMPLE_STATE_H__