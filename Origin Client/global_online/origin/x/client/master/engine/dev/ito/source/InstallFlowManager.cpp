#include "InstallFlowManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Engine
    {
        InstallFlowManager * InstallFlowManager::s_pInstance = NULL;

        InstallFlowManager::InstallFlowManager() :
            m_pStateMachine(NULL),
            m_pInitialState(NULL)
        {

        }

        InstallFlowManager::~InstallFlowManager()
        {
            Clear();
        }

        InstallFlowManager & InstallFlowManager::GetInstance()
        {
            if(!Exists())
            {
                s_pInstance = new InstallFlowManager();
            }

            return *s_pInstance;
        }

        void InstallFlowManager::DeleteInstance()
        {
            if(Exists())
            {
                delete s_pInstance;
                s_pInstance = NULL;
            }
        }

        bool InstallFlowManager::Exists()
        {
            return s_pInstance != NULL;
        }

        void InstallFlowManager::Clear()
        {
            m_pInitialState = NULL;
            if(m_pStateMachine)
            {
                delete m_pStateMachine;
                m_pStateMachine = NULL;
            }
        }

        void InstallFlowManager::Register(int version, InstallFlowBuilder builder)
        {
            m_BuilderMap[version] = builder;
        }

        bool InstallFlowManager::Init(int version)
        {
            // Do we have a builder for this version
            if(m_BuilderMap.contains(version))
            {
                // Get the builder
                InstallFlowBuilder builder = m_BuilderMap[version];

                // Is this a valid builder?
                if(builder)
                {
                    // Are we already running an install flow
                    if(m_pStateMachine)
                    {
                        ORIGIN_LOG_DEBUG << "Deleting an install flow in the Init state. Are you calling Init twice?";
                        Clear();
                    }

                    m_pStateMachine = new SimpleStateMachine();
                    m_pInitialState = NULL;

                    // Create the install flow.
                    if(!builder(m_pStateMachine, m_pInitialState))
                    {
                        ORIGIN_LOG_ERROR << "Install flow builder signals failure for version %d." << version;
                        Clear();
                        return false;
                    }

                    ORIGIN_VERIFY_CONNECT(m_pStateMachine, SIGNAL(stateChanged(QString)), this, SIGNAL(stateChanged(QString)));

                    return true;
                }
            }
            return false;
        }


        void InstallFlowManager::Run()
        {
            if(m_pStateMachine)
            {
                if(m_pStateMachine->SetInitialState(m_pInitialState))
                {
                    m_pStateMachine->Update();
                }
            }
        }

        void InstallFlowManager::Update()
        {
            if(m_pStateMachine)
            {
                m_pStateMachine->Update();
            }
        }

        void InstallFlowManager::SetStateCommand(QString command)
        {
            if(m_pStateMachine)
            {
                m_pStateMachine->Command(command);
            }
        }

        void InstallFlowManager::SetVariable(QString name, QVariant val)
        {
            if(m_pStateMachine)
            {
                m_pStateMachine->SetVariable(name, val);
            }
        }

        QVariant InstallFlowManager::GetVariable(QString name)
        {
            if(m_pStateMachine)
            {
                return m_pStateMachine->GetVariable(name);
            }

            return QVariant();
        }
    }
}