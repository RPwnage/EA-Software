#ifndef _NONORIGINGAMEPROXY_H
#define _NONORIGINGAMEPROXY_H

#include <QObject>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API NonOriginGameProxy : public QObject
{
	Q_OBJECT

public:
	explicit NonOriginGameProxy(QObject *parent, Origin::Engine::Content::EntitlementRef entitlement);
    
    // Remove non Origin game from My Games
    Q_INVOKABLE void removeFromLibrary();

private:

	Origin::Engine::Content::EntitlementRef mEntitlement;

};

}
}
}

#endif