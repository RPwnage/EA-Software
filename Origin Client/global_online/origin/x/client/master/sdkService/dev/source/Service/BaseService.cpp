///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: BaseService.cpp
//	Base class for EALS service areas
//	
//	Author: Sergey Zavadski

#include <QTimer>
#include <QMetaType>
#include "Service/BaseService.h"
#include "LSX/LsxConnection.h"
#include "services/debug/DebugService.h"
#include "LSXWrapper.h"
#include "lsx.h"
#include "lsxWriter.h"
#include "EbisuError.h"

namespace Origin
{
    namespace SDK
    {
        BaseService::BaseService( Lsx::LSX_Handler* handler, const QByteArray &name )
            : m_name( name )
            , m_handler( handler )
        {
            // Start our dispatch thread
            m_workerThread = new AsyncRequestDispatchThread(this);
            qRegisterMetaType<Lsx::Request>("Lsx::Request");
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(incomingAsyncRequest(Origin::SDK::Lsx::Request *)), m_workerThread, SLOT(handleRequest(Origin::SDK::Lsx::Request *)));
        }

        void BaseService::asyncStartup()
        {
            // This runs in the worker thread
        }

        void BaseService::waitForAsyncStartup()
        {
            m_started.acquire();
        }

        void BaseService::startedUp()
        {
            m_started.release();
        }

        BaseService::~BaseService()
        {
            m_workerThread->terminate();
            m_handler = NULL;
        }

        QThread *BaseService::workerThread() 
        { 
            return m_workerThread;
        }

        void BaseService::startWorker()
        {
            m_workerThread->start();
            // Make us and all of our subclasses and children receive signals on our worker thread
            moveToThread(m_workerThread);
        }

        void BaseService::stopWorker()
        {
            // Ask the thread to quit and wait
            m_workerThread->quit();
            m_workerThread->wait();
        }

        void BaseService::registerHandler( const QByteArray& command, RequestHandler handler )
        {
            m_handlers.insert(command, handler);
        }

        void BaseService::processRequest( Lsx::Request& request, Lsx::Response& response )
        {
            if ( handler() )
            {
                //	
                //	Try to find handler for this command
                //
                QHash< QByteArray, RequestHandler >::ConstIterator i = m_handlers.find( request.command() );

				if ( i != m_handlers.end() )
                {
                    ( this->*( i.value() ) ) ( request, response ) ;
                }
				else
				{
					lsx::ErrorSuccessT code;

					code.Code = EBISU_ERROR_NOT_IMPLEMENTED;
					code.Description = "The function you are calling is not implemented.";

					lsx::Write(response.document(), code);

                    request.connection()->sendResponse( response );
                }
            }
        }

        void BaseService::processRequestAsync( Lsx::Request * request )
        {
            emit(incomingAsyncRequest(request));
        }

        AsyncRequestDispatchThread::AsyncRequestDispatchThread(BaseService *serviceArea)
        {
            setObjectName("AsyncRequestDispatchThread");
            m_serviceArea = serviceArea;
            // Handle signals in this thread
            moveToThread(this);
            ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(finished()), this, SLOT(deleteLater()), Qt::QueuedConnection);
        }


        void AsyncRequestDispatchThread::run()
        {
            m_serviceArea->asyncStartup();
            m_serviceArea->startedUp();
            exec();
        }

        void AsyncRequestDispatchThread::handleRequest(Origin::SDK::Lsx::Request * request)
        {
            Lsx::Response * response = new Lsx::Response(*request);

            // Now that we're in the service area's thread (see the moveToThread above)
            // call the processRequest() synchronously
            m_serviceArea->processRequest( *request, *response );

            if(!response->isPending())
            {
                // Send the response
                response->send();
            }

            response->release();
            delete request;
        }
    }
}

