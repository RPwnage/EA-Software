#include "AppController.h"
#include "OriginSDKMain.h"

OriginSDKMain* g_sdkController = NULL;

AppController::AppController() : _heartbeatTimer(NULL)
{

}

AppController::~AppController()
{

}

void AppController::onStartup()
{
    emit LogMessage("<b>App Startup.</b>");

    _sdkMain = new OriginSDKMain();
    g_sdkController = _sdkMain;
    QObject::connect(_sdkMain, SIGNAL(LogMessage(QString)), this, SIGNAL(LogMessage(QString)));

    _heartbeatTimer = new QTimer(this);
    QObject::connect(_heartbeatTimer, SIGNAL(timeout()), this, SLOT(onHeartbeat()), Qt::QueuedConnection);
    _heartbeatTimer->start(1000);

    _sdkMain->StartOrigin();
}

void AppController::onHeartbeat()
{
    _sdkMain->DoOriginUpdate();

    //emit LogMessage("Ping!");
}
