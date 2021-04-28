//
//  OriginESHelper.cpp
//  EscalationService
//
//  Created by Frederic Meraud on 8/17/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <launch.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <QApplication>

#import <ApplicationServices/ApplicationServices.h>

#include "ServiceContext.h"
#include "services/escalation/EscalationServiceOSX.h"
#include "services/platform/PlatformService.h"


// TODO: Add support for transactions/Instant Off ?
#define SHOW_LOGGING

static bool debugMode = false;
static const char* DebugLogFilename = "ESServerLog";

// The socket id defined in the helper launchd plist to enable a connection with the Origin client
static const char* EscalationServicesHelperSocketID = "com.ea.origin.ESHelper.sock";



// Setup the connection when running outside launchd context
bool SetupDebugListener(int* socketID)
{
    *socketID = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (socketID < 0)
        return false;
    
    struct sockaddr_un name;
    memset(&name, 0, sizeof(name));
    name.sun_family = AF_LOCAL;
    strncpy(name.sun_path, Origin::Escalation::ESCALATION_SERVICE_PIPE.toLocal8Bit().data(), sizeof(name.sun_path));
    name.sun_path[sizeof(name.sun_path) - 1] = '\0';
    
    int result = bind(*socketID, (sockaddr*)&name, SUN_LEN(&name));
    if (result < 0)
    {
        close(*socketID);
        *socketID = -1;
        
        return false;
    }
    
    result = listen(*socketID, -1);    
    return result == 0;
}

// =================================================================================================================

// Extract the connection main socket handle.
bool GetConnectionSocketID(int* socketID)
{
    bool success = false;
    
    *socketID = -1;
    
    // Lookup the list of sockets the daemon supports 
    launch_data_t socket = NULL;
    launch_data_t req = launch_data_new_string(LAUNCH_KEY_CHECKIN);
    if (req)
    {
        launch_data_t resp = launch_msg(req);
        if (resp && launch_data_get_type(resp) == LAUNCH_DATA_DICTIONARY)
        {
            launch_data_t sockets = launch_data_dict_lookup(resp, LAUNCH_JOBKEY_SOCKETS);
            if (sockets && launch_data_get_type(sockets) == LAUNCH_DATA_DICTIONARY)
            {
                socket = launch_data_dict_lookup(sockets, EscalationServicesHelperSocketID);
            }
        }
    }
    
    if (socket)
    {
        // Currently only one definition: the unix/local socket we use to talk to the Origin client
        if (launch_data_get_type(socket) == LAUNCH_DATA_ARRAY && launch_data_array_get_count(socket) == 1)
        {
            launch_data_t socketDef = launch_data_array_get_index(socket, 0);
            if (socketDef && launch_data_get_type(socketDef) == LAUNCH_DATA_FD)
            {
                *socketID = launch_data_get_fd(socketDef);
                ORIGIN_LOG_DEBUG << "Found socket identifier: " << *socketID << "\n";
                
                success = true;
            }
            
            else
            {
                ORIGIN_LOG_ERROR << "Unable to find currently open listen socket\n";
            }
        }
        
        else
        {
            ORIGIN_LOG_ERROR << "The socket specification doesn't contain the single unix/local connection\n";
        }        
    }
    
    else
    {
        ORIGIN_LOG_ERROR << "Unable to find any socket definition for the service\n";
    }
    
    return success;
}

// =================================================================================================================

__attribute__((visibility("default")))
int main(int argc, char* argv[])
{
#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "ESImpl started...\n");
#endif
    
    QCoreApplication app(argc, argv);
    
#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "ESImpl starting logging\n");
    QString logPath = QString("%1%2").arg(Origin::Services::PlatformService::tempDataPath()).arg(DebugLogFilename);
    Origin::Services::Logger::Instance()->Init(logPath, true);
#endif
    
    if (argc == 2 && strcmp(argv[1], "debug") == 0)
    {
        // We are running manually (not managed by launchd)
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "ESImpl debug mode\n");
#endif
        
        ORIGIN_LOG_DEBUG << "Detected debug mode";
        debugMode = true;
    }
    
    int socketID;
    if (debugMode)
    {
        // Remove any previous debug socket
        QFile localSocket(Origin::Escalation::ESCALATION_SERVICE_PIPE);
        if (localSocket.exists())
        {
            ORIGIN_LOG_DEBUG << "Removing pre-existing local socket...\n";
            if (!localSocket.remove())
            {
                ORIGIN_LOG_ERROR << "Unable to remove pre-existing local socket - the server connection is probably going to fail\n";
            }
        }

#if 0 // The easy version - IPCServer does it all
        
        Origin::Escalation::EscalationServiceOSX server;
        if (server.isRunning())
        {
            ORIGIN_LOG_DEBUG << "Server running...\n";
            return app.exec();
        }
        
        else        
        {
            ORIGIN_LOG_ERROR << "Unable to setup debug server";            
            return -1;
        }
        
#else // Version closer to what the daemon does -> IPCServer doesn't listen for new connections
        
        if (SetupDebugListener(&socketID))
        {
            // Setup 
            Origin::Escalation::ServiceContext context(socketID);
            
            ORIGIN_LOG_DEBUG << "Server running...\n";
            return app.exec();
        }
        
        else 
        {
            ORIGIN_LOG_ERROR << "Unable to setup debug server";
        }
        
#endif
    }
    
    else // This is what the production code really does (ie running in helper daemon)
    {
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "ESImpl extracting socketID\n");
#endif
        
        // Extract listen socket
        if (!GetConnectionSocketID(&socketID))
        {
            syslog(LOG_NOTICE, "ESImpl no listen socket\n");
            
            ORIGIN_LOG_ERROR << "Unable to access daemon listen socket";
            return -1;
        }
        
        // Setup 
        Origin::Escalation::ServiceContext context(socketID);
        
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "ESImpl server running...\n");
#endif
        
        ORIGIN_LOG_DEBUG << "Server running...\n";
        int success = app.exec();
        
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "ESImpl return value=%d\n", success);
#endif
        return success;
    }

#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "ESImpl done\n");
#endif
    
	return 0;
}


