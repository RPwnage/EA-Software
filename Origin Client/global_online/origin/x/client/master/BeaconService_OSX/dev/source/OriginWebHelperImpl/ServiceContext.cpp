//
//  ServiceContext.cpp
//  EscalationService
//
//  Created by Frederic Meraud on 8/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "ServiceContext.h"

#include <syslog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/errno.h>
#include <unistd.h>

#include <QCoreApplication>

#include "services/log/LogService.h"
#include "LocalHost/LocalHost.h"
#include "LocalHost/LocalHostServiceConfig.h"

namespace Origin
{
    
namespace WebHelper
{

// Stop listening for new connections after 60s
static const int IdleTimeoutInMS = 60000;
    
ServiceContext::ServiceContext(int socketID, int socketID_ssl, QObject* parent)
{
    _closingDown = false;
    
    Origin::Sdk::LocalHost::start(new Origin::Sdk::LocalHost::LocalHostServiceConfig(), true);
    
    //TODO: Figure out how to hook something like this up without exposing the interfaces publicly (since they are largely private impl right now)
    //connect(_server, SIGNAL(newDataProcessed()), this, SLOT(handleNewDataProcessed()));
    
    _idleTimer.start(IdleTimeoutInMS);
    connect(&_idleTimer, SIGNAL(timeout()), this, SLOT(handleIdleTimeout()));
    
    _signalHandler.RegisterSignal(SIGTERM);
    connect(&_signalHandler, SIGNAL(sigTerm()), this, SLOT(handleSigTerm()));
    
    if (socketID != -1)
    {
        _requestNotifier = new QSocketNotifier(socketID, QSocketNotifier::Read);
        _requestNotifier->setEnabled(true);
        connect(_requestNotifier, SIGNAL(activated(int)), this, SLOT(handleNewConnection(int)));
    }
    
    if (socketID_ssl != -1)
    {
        _requestNotifierSSL = new QSocketNotifier(socketID_ssl, QSocketNotifier::Read);
        _requestNotifierSSL->setEnabled(true);
        connect(_requestNotifierSSL, SIGNAL(activated(int)), this, SLOT(handleNewConnectionSSL(int)));
    }
}

ServiceContext::~ServiceContext()
{
    _idleTimer.stop();
    _signalHandler.UnregisterSignal(SIGTERM);
    _requestNotifier->deleteLater();
    _requestNotifierSSL->deleteLater();
}

void ServiceContext::handleSigTerm()
{
    syslog(LOG_NOTICE, "WebHelperImpl Received SIGTERM\n");
    
    ORIGIN_LOG_DEBUG << "Received SIGTERM signal";
    
    // If we get SIGTERM, that means that the system is on its way out (Instant Off), so we need to close out our existing 
    // requests and stop accepting new ones. At this point, we know that we have at least one outstanding request, otherwise
    // we would've received SIGKILL and just exited.
    // Note that by adopting Instant Off, you are opting into a contract where you assert that launchd is the only entity which 
    // can legitimately send you SIGTERM.
    _closingDown = true;
    
    _idleTimer.stop();
    
    Origin::Sdk::LocalHost::stop();
    
    QCoreApplication::exit(0);
}
    
void ServiceContext::handleIdleTimeout()
{
    ORIGIN_LOG_DEBUG << "Idle for a while... time to stop listening for new connections/requests";
    
    Origin::Sdk::LocalHost::stop();
    
    QCoreApplication::exit(0);
}
    
void ServiceContext::handleNewConnection(int socketID)
{
    handleNewConnectionInternal(socketID, false);
}
    
void ServiceContext::handleNewConnectionSSL(int socketID)
{
    handleNewConnectionInternal(socketID, true);
}
    
void ServiceContext::handleNewConnectionInternal(int socketID, bool ssl)
{
    ORIGIN_LOG_DEBUG << "Setting up new pending connection...";
    
    struct sockaddr_in dest;
    socklen_t destLen = sizeof(struct sockaddr_in);
    
    int newConnectionID = ::accept(socketID, (sockaddr*)&dest, &destLen);
    
    if (!_closingDown)
    {
        _idleTimer.start(IdleTimeoutInMS);        
        if (newConnectionID != -1)
        {
            if (ssl)
            {
                Origin::Sdk::LocalHost::addManualConnectionSSL(newConnectionID);
            }
            else
            {
                Origin::Sdk::LocalHost::addManualConnection(newConnectionID);
            }
            //_server->addPendingConnection(newConnectionID);
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
    
} // WebHelper
} // Origin
