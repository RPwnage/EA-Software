///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: BaseService.h
//    Base class for SDK service areas
//    
//    Author: Hans van Veenendaal

#ifndef _BASE_SERVICE_H
#define _BASE_SERVICE_H 

#include <QHash>
#include <QSemaphore>
#include <QString>
#include <QThread>

#include "LSX/LSX_Handler.h"

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class Connection;
        }

        class AsyncRequestDispatchThread;

        class BaseService : public QObject
        {
            Q_OBJECT
                friend class AsyncRequestDispatchThread;
        public:
            BaseService(Lsx::LSX_Handler* handler, const QByteArray &name);

            virtual ~BaseService();

            //
            // This is called immediately after the constructor in our worker thread
            //
            // asyncStartup()s will run in parallel with each other but Middleware will
            // wait for all threads to exit startup() before it finishes initializing
            //
            virtual void asyncStartup();

            //
            // Called by LSX_Handler to wait for asyncStartup() to completed
            void waitForAsyncStartup();

            //
            //    Service area name
            //
            const QByteArray &name() const
            {
                return m_name;
            }

            //
            //    process request asynchronously 
            //
            void processRequestAsync(Lsx::Request * request);

            //
            // Start our worker thread
            //
            void startWorker();

            //
            //    Called by LSX_Handler before we're deleted to stop our worker thread
            //
            void stopWorker();

            virtual void destroy() = 0;

        protected:
            Lsx::LSX_Handler* handler() const
            {
                return m_handler;
            }

            //    
            //    send event with a given content to all connected clients
            //    
            template<class T>
            void sendEvent(T &msg, const QByteArray &multiplayerId = "")
            {
                send(msg, multiplayerId);
            }

            //
            //    LSX request handler
            //
            typedef void (BaseService::*RequestHandler)(Lsx::Request& request, Lsx::Response& response);

            //
            //    Register LSX handler
            //
            void registerHandler(const QByteArray & command, RequestHandler handler);

            //
            //    worker routine for asyncworker
            //
            typedef void (BaseService::*WorkerRoutine)(void* data);

            //
            // Thread dedicated to to this service area. All requests are workered
            // to this thread
            // 
            // Service areas can schedule events on this thread without interfering
            // with other service areas
            //
            QThread *workerThread();

            // Called by our worker thread to let us know we're started
            void startedUp();

        private:
            template<class T>
            int send(T &msg, const QByteArray &multiplayerId)
            {
                if (!handler())
                {
                    return 0;
                }

                return handler()->sendEvent(msg, m_name, multiplayerId);
            }

            //
            //    process request
            //
            void processRequest(Lsx::Request& request, Lsx::Response& response);

        signals:
            // This is hooked up to handleRequest() to the worker thread
            void incomingAsyncRequest(Origin::SDK::Lsx::Request *);

        private:

            QByteArray m_name;
            Lsx::LSX_Handler* m_handler;
            QHash<QByteArray, RequestHandler> m_handlers;

            // Used to wait for our startup() function to exit
            QSemaphore m_started;

            AsyncRequestDispatchThread *m_workerThread;
        };


        /**
         * The thread where all service area request are worker threads too
         *
         * There is one of these per service area and all their request handlers
         * execute in the context of this thread
         */
        class AsyncRequestDispatchThread : public QThread
        {
            Q_OBJECT
                friend class BaseService;
        protected:
            AsyncRequestDispatchThread(BaseService *);

            void run();

            protected slots:
            void handleRequest(Origin::SDK::Lsx::Request *);

        private:
            BaseService *m_serviceArea;
        };


        /*
          Lsx::Response wrapper class to make it possible to disconnect handling of request from function lifetime.

          https://confluence.ea.com/pages/editpage.action?pageId=462635834

        */
        class ResponseWrapperBase : public QObject
        {
            Q_OBJECT
        public slots:
            virtual void finished() = 0;
        };


        template <typename UserData, typename HandlerClass, typename CompletionClass> class ResponseWrapperEx : public ResponseWrapperBase
        {
            typedef bool (HandlerClass::*CallbackFunc) (UserData &meta, CompletionClass * resp, Lsx::Response &response);

        public:
            ResponseWrapperEx(const UserData &data, HandlerClass * handler, CallbackFunc callback, CompletionClass * resp, Lsx::Response &response) :
                mUserData(data),
                mHandlerClass(handler),
                mHandlerCallback(callback),
                mCompletionClass(resp),
                mResponse(&response)
            {
                if(mResponse)
                    mResponse->addRef();
            }

            ~ResponseWrapperEx()
            {
                if(mResponse)
                    mResponse->release();
                delete mCompletionClass;
            }

            virtual void finished()
            {
                bool bFinished = (mHandlerClass->*mHandlerCallback)(mUserData, mCompletionClass, *mResponse);

                if(mResponse && !mResponse->isPending())
                {
                    mResponse->send();
                }
                
                if(bFinished)
                {
                    deleteLater();
                }
            }

        private:
            UserData mUserData;
            HandlerClass * mHandlerClass;
            CallbackFunc mHandlerCallback;
            CompletionClass * mCompletionClass;
            Lsx::Response * mResponse;
        };

        /*
          Use this class when no input data is necessary when construction the reply provided by the completion class.
        */

        template <typename HandlerClass, typename CompletionClass> class ResponseWrapper : public ResponseWrapperBase
        {
            typedef bool (HandlerClass::*CallbackFunc) (CompletionClass * resp, Lsx::Response &response);

        public:
            ResponseWrapper(HandlerClass * handler, CallbackFunc callback, CompletionClass * resp, Lsx::Response &response) :
                mHandlerClass(handler),
                mHandlerCallback(callback),
                mCompletionClass(resp),
                mResponse(&response)
            {
                if(mResponse)
                    mResponse->addRef();
            }

            ~ResponseWrapper()
            {
                if(mResponse)
                    mResponse->release();
                delete mCompletionClass;
            }

            virtual void finished()
            {
                bool bFinished = (mHandlerClass->*mHandlerCallback)(mCompletionClass, *mResponse);

                if (mResponse && !mResponse->isPending())
                {
                    mResponse->send();
                }

                if(bFinished)
                {
                    deleteLater();
                }
            }

        private:
            CompletionClass * mCompletionClass;
            HandlerClass * mHandlerClass;
            Lsx::Response * mResponse;
            CallbackFunc mHandlerCallback;
        };
    }
}

#endif //_BASE_SERVICE_H

