//
//  main.cpp
//  Test
//
//  Created by Gog, D'arcy on 12-05-15.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <QtCore>
#include <QtWidgets>
#include <QDesktopServices>

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <EATrace/EATrace.h>
#include <EATrace/EALog_imp.h>
#include <EATrace/EALogConfig.h>

#include "services/Services.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/session/SessionService.h"
#include "services/session/LoginRegistrationSession.h"
#include "services/settings/SettingsManager.h"
#include "engine/Engine.h"

#include "Command.h"
#include "CommandDispatcher.h"
#include "Console.h"
#include "LoginService.h"
#include "EntitlementService.h"
#include "DownloadService.h"

//#include <syslog.h>

using namespace std;
using namespace Origin::Services;

/////////////////////////////////////////////////////////////////////////
//
// utilities

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim)
 {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}


/////////////////////////////////////////////////////////////////////////
//
// exit handler

void exitHandler(QStringList const& args)
{
    QEvent *ce = new QEvent (QEvent::Quit);
    QCoreApplication::postEvent (QCoreApplication::instance(), ce); 
}

/////////////////////////////////////////////////////////////////////////
//
// main

int main(int argc, char *argv[])
{
    cout << "OriginClientCLI v0.1" << endl;
    
    QApplication cli(argc, argv);
    cli.setOrganizationName("Origin");
    cli.setApplicationName("OriginClientCLI");
    
    bool showUI = false;
    char *showUIVal = getenv("ORIGIN_SHOWUI");
    if (showUIVal)
    {
        QString showUIString = showUIVal;
        if (showUIString.compare("yes", Qt::CaseInsensitive) == 0)
            showUI = true;
    }
    
    if (showUI)
    {
        cout << "Found launcher environment variable, showing UI." << endl;
    }
    else
    {
        cout << "Running in CLI mode, no UI." << endl;
    }

    // This enables assertions to trigger
    EA::Trace::ITracer* const pSavedTracer = EA::Trace::GetTracer();
    (void)pSavedTracer;
    
    Origin::Services::init();
    Origin::Engine::init();
    
    QString originDataPath = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation) + QDir::separator();
    
    //syslog(LOG_NOTICE, "OriginClientCLI running, data path: %s", qPrintable(originDataPath));
    
    Origin::Services::Logger::Instance()->Init(originDataPath + "OriginClientCLI_log", true);
    
    ConsoleWindow *consoleWindow = NULL;
    
    if (showUI)
    {
        consoleWindow = new ConsoleWindow();
    
        ORIGIN_VERIFY_CONNECT(Origin::Services::Logger::Instance(), SIGNAL(onNewLogEntry(LogMsgType, QString, QString, QString, int)), consoleWindow, SLOT(onNewLogEntry_Handler(LogMsgType, QString, QString, QString, int)));
    
        consoleWindow->show();
    }
    
    LOG_DEBUG << "OriginClientCLI initialized.";
    
    LOG_DEBUG << "Loading " << QString(originDataPath + "EACore.ini");
    
    if (showUI)
    {
        consoleWindow->setConfigLocation(originDataPath + "EACore.ini");
    }
    
    QString environment = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
    LOG_DEBUG << "Environment = " << environment.toLatin1().data();
    
    Origin::Services::writeSetting(Origin::Services::SETTING_EnvironmentName, QString("production"));
    
    LoginService::init();
    EntitlementService::init();
    DownloadService::init();
    
    CommandDispatcher dispatcher;
    dispatcher.add(Command("exit", "", exitHandler));
    LoginService::registerCommands(dispatcher);
    dispatcher.add(Command("entitlements", "", EntitlementService::fetchEntitlements));
    DownloadService::registerCommands(dispatcher);
    
    QThread* thread = new QThread;
    Console* console = Console::Instance();
    console->moveToThread(thread);
    thread->start();
    QMetaObject::invokeMethod(console, "work", Qt::QueuedConnection);
    
    ORIGIN_VERIFY_CONNECT(
        console, SIGNAL(command(QStringList const&)),
        &dispatcher, SLOT(onCommand(QStringList const&)));
    
    if (showUI)
    {
        ORIGIN_VERIFY_CONNECT(
                  consoleWindow, SIGNAL(command(QStringList const&)),
                  &dispatcher, SLOT(onCommand(QStringList const&)));
    
        ORIGIN_VERIFY_CONNECT(console, SIGNAL(consoleMessage(QString)), consoleWindow, SLOT(onConsoleMessage_Handler(QString)));
    }
    
    CONSOLE_WRITE << "Ready.\n";
    
    cli.exec();
    
    Origin::Services::release();
    
    return EXIT_SUCCESS;
}
