///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: LSX.cpp
//	Application entry point
//

#include "LSX/LSX_Handler.h"
#include "LSX/LsxServer.h"
#include "LSX/LSX.h"

#include <QCoreApplication>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            LSXThread::LSXThread() 
                : m_ready(false)
            {
            }

            LSXThread::~LSXThread()
            {
                delete m_handler;
                m_handler = NULL;
            }

            void LSXThread::run()
            {
                m_handler = new LSX_Handler();

                // Make the handler's async event pump happen in this thread
                m_handler->moveToThread(this);

                // We're ready!
                {
                    QMutexLocker locker(&m_readyMutex);
                    m_ready = true;
                }

                m_readyCondition.wakeAll();

                exec();

				delete m_handler;
                m_handler = NULL;
            }

            void LSXThread::waitForReady()
            {
                m_readyMutex.lock();

                while(!m_ready) 
                {
                    m_readyCondition.wait(&m_readyMutex);
                }

                m_readyMutex.unlock();
            }

            Lsx::Server *LSXThread::lsxServer() 
            {
                waitForReady();
                return m_handler->lsxServer();
            }
        }
    }
}