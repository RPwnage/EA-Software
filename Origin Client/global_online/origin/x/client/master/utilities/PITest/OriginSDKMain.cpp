#include "OriginSDKMain.h"
#include "FrostbitePI/OriginStreamInstallBackend.h"
#include <Engine/Core/StreamInstall/StreamInstall.h>
#include <Engine/Core/Message/CoreMessages.h>

OriginSDKMain::OriginSDKMain() : _sdkInitialized(false), _fbBackend(new fb::OriginStreamInstallBackend())
{
    
}

OriginSDKMain::~OriginSDKMain()
{
    delete _fbBackend;
}

void OriginSDKMain::Log(QString msg)
{
    emit LogMessage(msg);
}

bool OriginSDKMain::StartOrigin()
{
    // Start the Stream Install subsystem
    fb::StreamInstall::init();

    Log("<b>Initializing SDK.</b>");

    OriginStartupInputT debugAppIdentifier;
    OriginStartupOutputT out;
    OriginErrorT err = ORIGIN_SUCCESS;
    bool ret;

    debugAppIdentifier.ContentId = "1007719";
    debugAppIdentifier.Language = "en_US";
    debugAppIdentifier.MultiplayerId = "bookworm_pi_test_app";
    debugAppIdentifier.Title = "PI Test App";

    err = OriginStartup(0, ORIGIN_LSX_PORT, &debugAppIdentifier, &out);
    if(err != ORIGIN_SUCCESS)
    {
        Log("<b>SDK failed to initialize.</b>");
        OriginShutdown();
        ret = false;
    }
    else
    {
        Log("<b>SDK initialized.</b>");
        _sdkInitialized = true;

        OriginRegisterEventCallback(~0x0, EventCallback, this);

        _fbBackend->create();
        _fbBackend->registerOriginCallback();

        fb::g_coreMessageManager->queueMessage(new fb::OriginStreamInstallOriginAvailableMessage());

        ret = true;
    }

    return ret;
}

void OriginSDKMain::DoOriginUpdate()
{
    if (!_sdkInitialized)
        return;

    OriginErrorT err = OriginUpdate();
    if (err != ORIGIN_SUCCESS)
    {
        Log("Error calling OriginUpdate()");
    }
}

// Callback for All SDK Event handling
OriginErrorT OriginSDKMain::EventCallback (int32_t eventId, void* pContext, void* eventData, uint32_t count)
{
    OriginSDKMain* sdkMain = static_cast<OriginSDKMain*>(pContext);
    if (sdkMain)
    {
        return sdkMain->EventCallback(eventId, eventData, count);
    }
    return ORIGIN_SUCCESS;
}

OriginErrorT OriginSDKMain::EventCallback (int32_t eventId, void* eventData, uint32_t count)
{
    Log(QString("<div style=\"color:brown\">Unhandled SDK Event Received ID: %1</div>").arg(eventId));
    return ORIGIN_SUCCESS;
}

void OriginSDKMain::QueueMessage(int category, int type, QString msg)
{
    QMetaObject::invokeMethod(this, "onQueuedMessage", Qt::QueuedConnection, Q_ARG(int, category), Q_ARG(int, type), Q_ARG(QString, msg));
}

void OriginSDKMain::onQueuedMessage(int category, int type, QString msg)
{
    if (fb::g_coreMessageManager)
    {
        fb::g_coreMessageManager->onQueuedMessage(category, type, msg);
    }
}