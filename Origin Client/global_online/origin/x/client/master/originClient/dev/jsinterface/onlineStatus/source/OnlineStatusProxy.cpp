#include "OnlineStatusProxy.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"
#include "flows/ClientFlow.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

OnlineStatusProxy::OnlineStatusProxy(bool canRequestState) : 
    mCanRequestState(canRequestState)
{
    Engine::LoginController* controller = Engine::LoginController::instance();
    ORIGIN_VERIFY_CONNECT(controller, SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(connectUserSignals(Origin::Engine::UserRef)));

    Engine::UserRef currentUser = Engine::LoginController::currentUser();

    if (!currentUser.isNull())
    {
        connectUserSignals(currentUser); 
    }
}

bool OnlineStatusProxy::onlineState()
{
    Engine::UserRef user = Engine::LoginController::currentUser();
    return (user && Origin::Services::Connection::ConnectionStatesService::isUserOnline (user->getSession()));
}

void OnlineStatusProxy::connectUserSignals(Origin::Engine::UserRef user)
{
    ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL (userConnectionStateChanged (bool, Origin::Services::Session::SessionRef)), 
        this, SIGNAL(onlineStateChanged(bool)));
}

Q_INVOKABLE void OnlineStatusProxy::requestManualOfflineMode()
{
    if (mCanRequestState)
    {
        ClientFlow::instance()->requestManualOfflineMode();
    }
    else
    {
        // The widget didn't pass the canRequestState parameter
        ORIGIN_ASSERT(0);
    }
}

Q_INVOKABLE void OnlineStatusProxy::requestOnlineMode()
{
    if (mCanRequestState)
    {
        ClientFlow::instance()->requestOnlineMode();
    }
    else
    {
        // The widget didn't pass the canRequestState parameter
        ORIGIN_ASSERT(0);
    }
}

}
}
}
