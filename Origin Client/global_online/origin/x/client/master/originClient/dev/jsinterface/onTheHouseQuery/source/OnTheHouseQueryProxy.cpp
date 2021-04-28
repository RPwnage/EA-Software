#include "OnTheHouseQueryProxy.h"
#include "engine/content/OnTheHouseController.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{
OnTheHouseQueryProxy::OnTheHouseQueryProxy(QObject* parent)
: QObject(parent)
{
    Engine::Content::OnTheHouseController* othController = Engine::Content::OnTheHouseController::instance();
    ORIGIN_VERIFY_CONNECT(othController, SIGNAL(error()), this, SIGNAL(failed()));
    ORIGIN_VERIFY_CONNECT(othController, SIGNAL(succeeded(QVariant)), this, SIGNAL(succeeded(QVariant)));
}


void OnTheHouseQueryProxy::startQuery()
{
    Engine::UserRef user = Engine::LoginController::currentUser();
    if (user->isUnderAge())
    {
        return;
    }
    Engine::Content::OnTheHouseController::instance()->fetchPromo();
}

}// namespace JsInterface
}// namespace Client
}// namespace Origin