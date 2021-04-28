#include "NonOriginGameProxy.h"

#include "flows/MainFlow.h"

#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/NonOriginContentController.h"
#include "engine/content/ProductArt.h"

#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{
namespace JsInterface
{

NonOriginGameProxy::NonOriginGameProxy(QObject *parent, Origin::Engine::Content::EntitlementRef entitlement) 
	: QObject(parent), 
	mEntitlement(entitlement)
{	

}

void NonOriginGameProxy::removeFromLibrary()
{
    Origin::Client::MainFlow::instance()->removeNonOriginGame(mEntitlement);
}

}
}
}