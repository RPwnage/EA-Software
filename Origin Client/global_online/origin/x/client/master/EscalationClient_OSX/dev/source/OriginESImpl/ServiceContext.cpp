//
//  ServiceContext.cpp
//  EscalationService
//
//  Created by Frederic Meraud on 8/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "ServiceContext.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/errno.h>
#include <unistd.h>

#include <QCoreApplication>




namespace Origin
{
    
namespace Escalation
{

// Stop listening for new connections after 20s
static const int IdleTimeoutInMS = 20000;
    
ServiceContext::ServiceContext(int socketID, QObject* parent)
{
    _closingDown = false;
    
    _server = new Origin::Escalation::EscalationServiceOSX(true);
    connect(_server, SIGNAL(newDataProcessed()), this, SLOT(handleNewDataProcessed()));
    
    _idleTimer.start(IdleTimeoutInMS);
    connect(&_idleTimer, SIGNAL(timeout()), this, SLOT(handleIdleTimeout()));
    
    _signalHandler.RegisterSignal(SIGTERM);
    connect(&_signalHandler, SIGNAL(sigTerm()), this, SLOT(handleSigTerm()));
    
    _requestNotifier = new QSocketNotifier(socketID, QSocketNotifier::Read);
    _requestNotifier->setEnabled(true);
    connect(_requestNotifier, SIGNAL(activated(int)), this, SLOT(handleNewConnection(int)));
}

ServiceContext::~ServiceContext()
{
    delete _server;
    
    _idleTimer.stop();
    _signalHandler.UnregisterSignal(SIGTERM);
    _requestNotifier->deleteLater();
}

void ServiceContext::handleSigTerm()
{
    ORIGIN_LOG_DEBUG << "Received SIGTERM signal";
    
    // If we get SIGTERM, that means that the system is on its way out (Instant Off), so we need to close out our existing 
    // requests and stop accepting new ones. At this point, we know that we have at least one outstanding request, otherwise
    // we would've received SIGKILL and just exited.
    // Note that by adopting Instant Off, you are opting into a contract where you assert that launchd is the only entity which 
    // can legitimately send you SIGTERM.
    _closingDown = true;
    
    _idleTimer.stop();
    
    delete _server;
    _server = NULL;
}
    
void ServiceContext::handleIdleTimeout()
{
    ORIGIN_LOG_DEBUG << "Idle for a while... time to stop listening for new connections/requests";
    QCoreApplication::exit(0);
}
    
void ServiceContext::handleNewConnection(int socketID)
{
    ORIGIN_LOG_DEBUG << "Setting up new pending connection...";
    
    struct sockaddr_un name;
    socklen_t nameLen = SUN_LEN(&name);
    
    int newConnectionID = ::accept(socketID, (sockaddr*)&name, &nameLen);

    if (!_closingDown)
    {
        _idleTimer.start(IdleTimeoutInMS);        
        if (newConnectionID != -1)
        {
            _server->addPendingConnection(newConnectionID);
        }
        
        else
        {
            ORIGIN_LOG_ERROR << "Unable to fulfill new connection (" << errno << ")";
        }
    }
    
    else
    {
        ORIGIN_LOG_DEBUG << "We're about to close down -> skip connection request";
        close(newConnectionID);
    }
}

void ServiceContext::handleNewDataProcessed()
{
    // We're still getting requests -> reset the timer (if not stopped because of SIGTERM/closing down)
    if (!_closingDown)
        _idleTimer.start(IdleTimeoutInMS);
}
    
} // Escalation
} // Origin
