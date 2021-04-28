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
#include "services/escalation/EscalationServiceOSX.h"


namespace Origin
{

    namespace Escalation 
    {
        
        // This class provides a Qt context to manage events such as
        // 1) new connections/requests
        // 2) idle timeout
        // 3) SIGTERM signals
        class ServiceContext : public QObject
        {
            Q_OBJECT
            
        public:
            ServiceContext(int socketID, QObject* parent = 0);
            ~ServiceContext();
            
            
        public slots:
            void handleSigTerm();
            void handleIdleTimeout();
            void handleNewConnection(int socketID);
            void handleNewDataProcessed();
            
        private:
            bool _closingDown;
            
            QTimer _idleTimer;
            QSocketNotifier* _requestNotifier;
            POSIXSignalHandler _signalHandler;
            
            Origin::Escalation::EscalationServiceOSX* _server;
        };

} // Escalation
} // Origin

#endif
