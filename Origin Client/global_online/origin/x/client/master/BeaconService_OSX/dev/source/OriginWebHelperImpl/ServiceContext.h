//
//  ServiceContext.h
//  EscalationService
//
//  Created by Frederic Meraud on 8/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef EscalationService_ServiceContext_h
#define EscalationService_ServiceContext_h

#include <QTimer>
#include <QSocketNotifier>

#include "POSIXSignalHandler.h"


namespace Origin
{

    namespace WebHelper
    {
        
        // This class provides a Qt context to manage events such as
        // 1) new connections/requests
        // 2) idle timeout
        // 3) SIGTERM signals
        class ServiceContext : public QObject
        {
            Q_OBJECT
            
        public:
            ServiceContext(int socketID, int socketID_ssl, QObject* parent = 0);
            ~ServiceContext();
            
            
        public slots:
            void handleSigTerm();
            void handleIdleTimeout();
            void handleNewConnection(int socketID);
            void handleNewConnectionSSL(int socketID);
            void handleNewDataProcessed();
            
        private:
            void handleNewConnectionInternal(int socketID, bool ssl);
            
        private:
            bool _closingDown;
            
            QTimer _idleTimer;
            QSocketNotifier* _requestNotifier;
            QSocketNotifier* _requestNotifierSSL;
            POSIXSignalHandler _signalHandler;
        };

} // WebHelper
} // Origin

#endif
