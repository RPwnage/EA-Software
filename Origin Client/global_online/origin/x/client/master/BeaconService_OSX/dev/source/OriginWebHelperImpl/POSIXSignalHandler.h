//
//  POSIXSignalHandler.h
//  EscalationService
//
//  Created by Frederic Meraud on 8/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef EscalationService_POSIXSignalHandler_h
#define EscalationService_POSIXSignalHandler_h

#include <QSocketNotifier>
#include <pthread.h>

namespace Origin
{
    
    namespace WebHelper
    {
        
        // This class handles forwarding POSIX signals to the main Qt event loop so that we can use Qt functions.
        // Based on http://doc.qt.nokia.com/4.7-snapshot/unix-signals.html
        // Not very "safe", but good enough in our context.
        class POSIXSignalHandler : public QObject
        {
            Q_OBJECT
            
        public:
            POSIXSignalHandler(QObject* parent = 0);
            ~POSIXSignalHandler();
            
            // Main functions used to register/unregister the notification of a specific signal.
            bool RegisterSignal(int signal);
            bool UnregisterSignal(int signal);
              
            public slots:
                void handleSignal();
            
            signals:
                void sigTerm();

            
            private:
                static void baseSignalHandler(int signal);
            
            
                static int _sockets[2];
                QSocketNotifier* _notifier;
                static pthread_mutex_t _mutex;
            
                static const int MaxSignalCount = SIGUSR2 + 1;
                struct SignalInfo
                {
                    const char* fcnName;
                    struct sigaction prevAction;
                    
                    bool handled;
                };
                static SignalInfo _signals[MaxSignalCount];
            
                bool _isValid;
        };

} // WebHelper
} // Origin

#endif
