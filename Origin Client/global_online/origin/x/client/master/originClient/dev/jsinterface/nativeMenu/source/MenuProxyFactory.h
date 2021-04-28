#ifndef _MENUPROXYFACTORY
#define _MENUPROXYFACTORY

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>

#include "MenuProxy.h"
#include "services/plugin/PluginAPI.h"

class QWebPage;

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API MenuProxyFactory : public QObject
{
	Q_OBJECT
public:
    explicit MenuProxyFactory(QWebPage *page);

    Q_INVOKABLE QObject *createMenu();

private:
    QWebPage *mPage;
};

}
}
}

#endif
