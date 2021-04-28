#ifndef __INSTALL_FLOW_MANAGER_H__
#define __INSTALL_FLOW_MANAGER_H__

#include "QObject.h"
#include "SimpleStateMachine.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        typedef bool (*InstallFlowBuilder)(SimpleStateMachine *pStateMachine, SimpleState *& pInitialState);

        class ORIGIN_PLUGIN_API InstallFlowManager : public QObject
        {
	        Q_OBJECT
        public:
	        /// \brief Construct the install flow manager.
	        InstallFlowManager();
	        ~InstallFlowManager();

	        static InstallFlowManager & GetInstance();
	        static void DeleteInstance();
	        static bool Exists();

	        /// \brief Remove any running state machines.
	        /// Removes the current state machine, but doesn't clear the builder map.
	        void Clear();
	
	        /// \brief Register State Graph builders for each install version.
	        /// Register State Graph builder per version. Each version can have different logic assigned to it.
	        /// \param version The version of the install flow.
	        /// \param builder A function that constructs the install flow.
	        void Register(int version, InstallFlowBuilder builder);

	        /// \brief Create the install flow, and set the initial state of the graph.
	        /// Constructs a install flow with the specified version
	        /// \param version The requested version of the install flow.
	        /// \return true if the install flow is constructed properly, else false. 
	        bool Init(int version);

	        /// \brief execute a command against the current state
	        /// This function will set the current command to be command, and call update on the install flow.h
	        /// \param command The command to apply
	        void SetStateCommand(QString command);

	        /// \brief Modify any variables in the scratch pad.
	        /// Write information to the scratch pad that can be used by install tasks, or by other parts of the install flow.
	        /// \param name The name of the variable.
	        /// \param val The new value of the parameter.
	        void SetVariable(QString name, QVariant val);

	        /// \brief Get the value of a variable in the scratch pad.
	        /// \param name The name of the variable.
	        /// \return The value of the variable associated with name.
	        QVariant GetVariable(QString name);

        signals:
	        /// \brief Signals that the install state changed. 
	        /// \param newStateName The name of the new state. e.g. Install Location, Code Redemption
	        void stateChanged(QString newStateName);

        public slots:
	        /// \brief Activate the initial state of the graph
	        /// Start the install flow.
	        void Run();

	        /// \brief Trigger an update
	        void Update();

        private:
	        SimpleStateMachine * m_pStateMachine;
	        SimpleState * m_pInitialState;

	        QMap<int, InstallFlowBuilder> m_BuilderMap;

	        static InstallFlowManager * s_pInstance;
        };
    }
}

#endif //__INSTALL_FLOW_MANAGER_H__