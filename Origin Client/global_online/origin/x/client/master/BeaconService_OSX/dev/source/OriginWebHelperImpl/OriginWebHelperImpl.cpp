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
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/un.h>
#include <launch.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <QApplication>

#import <ApplicationServices/ApplicationServices.h>

#include "ServiceContext.h"
#include "services/platform/PlatformService.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/crypto/CryptoService.h"
#include "services/config/OriginConfigService.h"
#include "services/session/SessionService.h"


// TODO: Add support for transactions/Instant Off ?
#define SHOW_LOGGING

static bool debugMode = false;
static const char* DebugLogFilename = "WebHelperServerLog";

// The socket id defined in the helper launchd plist to enable a connection with the Origin client
static const char* BeaconServiceHelperSocketID = "com.ea.origin.WebHelper.sock";
static const char* BeaconServiceHelperSSLSocketID = "com.ea.origin.WebHelper.securesock";


// Setup the connection when running outside launchd context
bool SetupDebugListener(int port, int* socketID)
{
    *socketID = socket(AF_INET, SOCK_STREAM, 0);
    if (socketID < 0)
        return false;
    
    struct sockaddr_in name;
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = htons(port);
    
    int result = bind(*socketID, (struct sockaddr*)&name, sizeof(struct sockaddr));
    if (result < 0)
    {
        close(*socketID);
        *socketID = -1;
        
        return false;
    }
    
    result = listen(*socketID, 10);
    return result == 0;
}

// =================================================================================================================

bool LaunchdCheckin(launch_data_t* out_resp)
{
    // Lookup the list of sockets the daemon supports
    launch_data_t req = launch_data_new_string(LAUNCH_KEY_CHECKIN);
    if (req)
    {
        //syslog(LOG_NOTICE, "WebHelperImpl got launch data\n");
        launch_data_t resp = launch_msg(req);
        if (resp && launch_data_get_type(resp) == LAUNCH_DATA_DICTIONARY)
        {
            *out_resp = resp;
            return true;
        }
    }
    
    return false;
}

// Extract the connection main socket handle.
bool GetConnectionSocketID(launch_data_t resp, const char* socketName, int* socketID)
{
    bool success = false;
    
    *socketID = -1;
    
    // Lookup the list of sockets the daemon supports 
    launch_data_t socket = NULL;
    
    launch_data_t sockets = launch_data_dict_lookup(resp, LAUNCH_JOBKEY_SOCKETS);
    if (sockets && launch_data_get_type(sockets) == LAUNCH_DATA_DICTIONARY)
    {
                //syslog(LOG_NOTICE, "WebHelperImpl got sockets\n");
        socket = launch_data_dict_lookup(sockets, socketName);
    }
    
    if (socket)
    {
        // Currently only one definition: the unix/local socket we use to talk to the Origin client
        if (launch_data_get_type(socket) == LAUNCH_DATA_ARRAY && launch_data_array_get_count(socket) == 1)
        {
            //syslog(LOG_NOTICE, "WebHelperImpl got socket stuff\n");
            launch_data_t socketDef = launch_data_array_get_index(socket, 0);  // Index 0 is the IPv4 socket descriptor
            if (socketDef && launch_data_get_type(socketDef) == LAUNCH_DATA_FD)
            {
                *socketID = launch_data_get_fd(socketDef);
                syslog(LOG_NOTICE, "Found socket identifier: %d for socket id: %s\n", *socketID, socketName);
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
            syslog(LOG_NOTICE, "WebHelperImpl no socket descriptors for id: %s\n", socketName);
            
            ORIGIN_LOG_ERROR << "The socket specification doesn't contain the tcp connection for id: " << socketName;
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
    syslog(LOG_NOTICE, "WebHelperImpl started...\n");
#endif
    
    QCoreApplication app(argc, argv);
    
#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "WebHelperImpl starting logging\n");
    QString logPath = QString("%1%2").arg(Origin::Services::PlatformService::tempDataPath()).arg(DebugLogFilename);
    Origin::Services::Logger::Instance()->Init(logPath, true);
#endif
    
    qRegisterMetaType<Origin::Services::Setting>("Origin::Services::Setting");
    qRegisterMetaType<Origin::Services::Variant>("Origin::Services::Variant");
    
    
    Origin::Services::DebugService::init();
    Origin::Services::PlatformService::init();
    Origin::Services::Session::SessionService::init();
    Origin::Services::SettingsManager::init();
    Origin::Services::OriginConfigService::init(); // need the environment, so must happen after SettingsManager
    
    // re-parse overrides
    Origin::Services::SettingsManager::instance()->loadOverrideSettings();
    
    syslog(LOG_NOTICE, "WebHelperImpl init complete\n");
    
    // Now we can properly set the logging level
#if ORIGIN_DEBUG
    bool logDebug = true;
#else
    bool logDebug = Origin::Services::readSetting(Origin::Services::SETTING_LogDebug, Origin::Services::Session::SessionRef());
#endif
    Origin::Services::Logger::Instance()->SetLogDebug(logDebug);
    
#if ORIGIN_DEBUG
    // If we're debug, force developer mode
    ORIGIN_LOG_EVENT << "Running in Developer Mode.";
    Origin::Services::writeSetting(Origin::Services::SETTING_OriginDeveloperToolEnabled, true);
#endif
    
    if (argc == 2 && strcmp(argv[1], "debug") == 0)
    {
        // We are running manually (not managed by launchd)
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "WebHelperImpl debug mode\n");
#endif
        
        ORIGIN_LOG_DEBUG << "Detected debug mode";
        debugMode = true;
    }
    
    int socketID = 0;
    int socketID_ssl = 0;
    
    if (debugMode)
    {
#if 0 // The easy version - IPCServer does it all
        /*
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
        }*/
        
#else // Version closer to what the daemon does -> IPCServer doesn't listen for new connections
        
        if (SetupDebugListener(3213, &socketID) && SetupDebugListener(3212, &socketID_ssl))
        {
            // Setup 
            Origin::WebHelper::ServiceContext context(socketID, socketID_ssl);
            
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
        syslog(LOG_NOTICE, "WebHelperImpl extracting socketID\n");
#endif
        
        // Checkin with Launchd
        launch_data_t resp;
        if (!LaunchdCheckin(&resp))
        {
            syslog(LOG_WARNING, "WebHelperImpl unable to checkin with launchd\n");
            ORIGIN_LOG_ERROR << "Unable to checkin with launchd";
            return -1;
        }
        
        // Extract listen socket
        if (!GetConnectionSocketID(resp, BeaconServiceHelperSocketID, &socketID))
        {
            syslog(LOG_NOTICE, "WebHelperImpl no non-ssl listen socket\n");
        }
        
        
        // Extract listen socket
        if (!GetConnectionSocketID(resp, BeaconServiceHelperSSLSocketID, &socketID_ssl))
        {
            syslog(LOG_NOTICE, "WebHelperImpl no ssl listen socket\n");
        }
        
        if (socketID == -1 && socketID_ssl == -1)
        {
            ORIGIN_LOG_ERROR << "Unable to access any daemon listen socket";
            return -1;
        }
        
        // Setup 
        Origin::WebHelper::ServiceContext context(socketID, socketID_ssl);
        
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "WebHelperImpl server running...\n");
#endif
        
        ORIGIN_LOG_DEBUG << "Server running...\n";
        int success = app.exec();
        
#ifdef SHOW_LOGGING
        syslog(LOG_NOTICE, "WebHelperImpl return value=%d\n", success);
#endif
        return success;
    }

#ifdef SHOW_LOGGING
    syslog(LOG_NOTICE, "WebHelperImpl done\n");
#endif
    
	return 0;
}


