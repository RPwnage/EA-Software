//
//  POSIXSignalHandler.cpp
//  EscalationService
//
//  Created by Frederic Meraud on 8/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "POSIXSignalHandler.h"

#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>

#include <QCoreApplication>


namespace Origin
{

namespace WebHelper
{
        
int POSIXSignalHandler::_sockets[2];
pthread_mutex_t POSIXSignalHandler::_mutex;
POSIXSignalHandler::SignalInfo POSIXSignalHandler::_signals[MaxSignalCount];


POSIXSignalHandler::POSIXSignalHandler(QObject* parent)
{
    // Create new pair of local sockets to forward events to Qt
    if (::socketpair(AF_LOCAL, SOCK_STREAM, 0, _sockets) == 0)
    {
        _notifier = new QSocketNotifier(_sockets[1], QSocketNotifier::Read, this);
        connect(_notifier, SIGNAL(activated(int)), this, SLOT(handleSignal()));
        
        pthread_mutex_init(&_mutex, NULL);
        
        for (int idx = 0; idx < MaxSignalCount; ++idx)
        {
            _signals[idx].handled = false;
            _signals[idx].fcnName = NULL;
        }
        
        _signals[SIGTERM].fcnName = "sigTerm";
        
        _isValid = true;
    }
    
    else
        _isValid = false;
}
    
POSIXSignalHandler::~POSIXSignalHandler()
{
    // Clean up sockets used to forward signal events/associated notifier
    if (_isValid)
    {
        pthread_mutex_lock(&_mutex);
        
        for (int idx = 0; idx < MaxSignalCount; ++idx)
        {
            if (_signals[idx].handled && _signals[idx].fcnName)
                sigaction(idx, &_signals[idx].prevAction, NULL);
        }
        
        ::close(_sockets[0]);
        ::close(_sockets[1]);
        _notifier->deleteLater();
        
        pthread_mutex_unlock(&_mutex);
        pthread_mutex_destroy(&_mutex);
    }
}

// Register a specific signal to use our base handler, which will forward the event back to this
// object/emit an appropriate Qt signal
bool POSIXSignalHandler::RegisterSignal(int signal)
{
    if (!_isValid)
        return false;
    
    if (signal < 1 || signal >= MaxSignalCount)
        return false;

    pthread_mutex_lock(&_mutex);

    if (_signals[signal].fcnName)
    {
        struct sigaction posixAction;
        posixAction.sa_handler = POSIXSignalHandler::baseSignalHandler;
        sigemptyset(&posixAction.sa_mask);
        posixAction.sa_flags = SA_RESTART;
        
        _signals[signal].handled = sigaction(signal, &posixAction, &_signals[signal].prevAction) == 0;
    }
    
    pthread_mutex_unlock(&_mutex);
    
    return _signals[signal].handled;
}

// Reset a signal to use its original handler
bool POSIXSignalHandler::UnregisterSignal(int signal)
{
    if (!_isValid)
        return false;
    
    if (signal < 1 || signal >= MaxSignalCount)
        return false;
    
    pthread_mutex_lock(&_mutex);
    
    if (_signals[signal].handled && _signals[signal].fcnName)
        _signals[signal].handled = sigaction(signal, &_signals[signal].prevAction, NULL) != 0;
    
    pthread_mutex_unlock(&_mutex);
    
    return _signals[signal].handled;
}

// Signal handler called from the Qt main event loop
void POSIXSignalHandler::handleSignal()
{
    // Make sure to disable notifier while we're handling the event
    QSocketNotifier* notifier = static_cast<QSocketNotifier*>(sender());
    notifier->setEnabled(false);

    // Grab which signal was triggered
    int signal;
    ::read(_notifier->socket(), &signal, sizeof(int));

    
    // Find the associated action/call it
    if (signal > 0 && signal < MaxSignalCount)
    {
        pthread_mutex_lock(&_mutex);
        
        if (_signals[signal].handled && _signals[signal].fcnName)
            QMetaObject::invokeMethod(this, _signals[signal].fcnName, Qt::QueuedConnection);
        
        pthread_mutex_unlock(&_mutex);
    }
    
    // Done
    notifier->setEnabled(true);
}

// ============================================================================================

// Common method called by the POSIX signal
void POSIXSignalHandler::baseSignalHandler(int signal)
{
    // Forward signal through socket
    pthread_mutex_lock(&_mutex);
    
    if (signal > 0 && signal < MaxSignalCount)
    {
        if (_signals[signal].handled && _signals[signal].fcnName)
        {
            ::write(_sockets[0], &signal, sizeof(signal));
        }
    }
                    
    pthread_mutex_unlock(&_mutex);
}
    
} // WebHelper
} // Origin

    