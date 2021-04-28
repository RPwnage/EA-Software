#include "MenuProxyFactory.h"

#include <QWebPage>
#include "WebWidget/ScriptOwnedObject.h"
#include "MenuProxy.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    
MenuProxyFactory::MenuProxyFactory(QWebPage *page)
    : QObject(page),
      mPage(page)
{
}

QObject *MenuProxyFactory::createMenu()
{
    QObject *proxy = new MenuProxy(mPage, NULL);

    // Top level menus can go away when JavaScript forgets about them
    WebWidget::ScriptOwnedObject::transferToScriptOwnership(proxy);

    return proxy;
}

}
}
}
