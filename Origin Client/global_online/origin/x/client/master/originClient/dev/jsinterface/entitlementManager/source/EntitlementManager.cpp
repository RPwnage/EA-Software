#include "EntitlementManager.h"
#include "EntitlementProxy.h"
#include "EntitlementDialogProxy.h"
#include "flows/MainFlow.h"
#include "engine/content/ContentController.h"
#include "engine/social/SocialController.h"
#include "engine/social/CommonTitlesController.h"
#include "engine/login/LoginController.h"
#include "TelemetryAPIDLL.h"

#include "services/debug/DebugService.h"

using namespace Origin::Engine::Content;

namespace
{
    Origin::Client::JsInterface::EntitlementManager *EntitlementManagerInstance = NULL;
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{

EntitlementManager::EntitlementManager()
: mDialogProxy(new EntitlementDialogProxy())
{
    //we only want to listen for entitlement changes after the initial refresh, as the widget grabs the entitlement information while loading initially
    //otherwise we will have a firestorm of signals firing.
    BaseGamesController *baseGamesController = ContentController::currentUserContentController()->baseGamesController();
    ORIGIN_VERIFY_CONNECT(baseGamesController, SIGNAL(baseGamesLoaded()), this, SLOT(listenForEntitlementChanges()));
}

EntitlementManager::~EntitlementManager()
{
    if (EntitlementManagerInstance == this)
    {
        EntitlementManagerInstance = NULL;
    }
}


void EntitlementManager::listenForEntitlementChanges()
{
    //disconnect this slot as we just want to make the signal/slot connections once
    BaseGamesController *baseGamesController = ContentController::currentUserContentController()->baseGamesController();
    ORIGIN_VERIFY_DISCONNECT(baseGamesController, SIGNAL(baseGamesLoaded()), this, SLOT(listenForEntitlementChanges()));

    ORIGIN_VERIFY_CONNECT(ContentController::currentUserContentController(), SIGNAL(updateStarted()),
        this, SIGNAL(refreshStarted()));

    ORIGIN_VERIFY_CONNECT(ContentController::currentUserContentController(), SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)),
        this, SLOT(entitlementsListRefreshed(QList<Origin::Engine::Content::EntitlementRef>, QList<Origin::Engine::Content::EntitlementRef>)));

    ORIGIN_VERIFY_CONNECT(ContentController::currentUserContentController(), SIGNAL(updateFailed()), this, SIGNAL(refreshCompleted()));
}

QObjectList EntitlementManager::topLevelEntitlements()
{
    QObjectList ret;

    BaseGamesController *baseGamesController = ContentController::currentUserContentController()->baseGamesController();
    foreach (const EntitlementRef &entitlement, baseGamesController->baseGames())
    {
        // TODO TS3_HARD_CODE_REMOVE
        if (entitlement->contentConfiguration()->isSuppressedSims3Expansion())
        {
            continue;
        }

        ret.append(proxyForEntitlement(entitlement));
    }

    NonOriginContentController *nogController = ContentController::currentUserContentController()->nonOriginController();
    foreach (const EntitlementRef &entitlement, nogController->nonOriginGames())
    {
        ret.append(proxyForEntitlement(entitlement));
    }

    return ret;
}

void EntitlementManager::entitlementsListRefreshed(const QList<Origin::Engine::Content::EntitlementRef> entitlementsAdded, QList<Origin::Engine::Content::EntitlementRef> entitlementsRemoved)
{
    foreach (const EntitlementRef &entitlement, entitlementsAdded)
    {
        if (entitlement->contentConfiguration()->isBaseGame() && 
            !entitlement->contentConfiguration()->isSuppressedSims3Expansion()) // TODO TS3_HARD_CODE_REMOVE
        {
            emit added(proxyForEntitlement(entitlement));
        }

        // TODO TS3_HARD_CODE_REMOVE
        // Remove any previously owned Sims3 FGPE from the My Games page now that they have a valid parent.
        if (entitlement->contentConfiguration()->masterTitleId() == Services::subs::Sims3MasterTitleId)
        {
            foreach (const EntitlementRef& extracontent, entitlement->children())
            {
                if (extracontent->contentConfiguration()->isSuppressedSims3Expansion())
                {
                    emit removed(proxyForEntitlement(extracontent));
                }
            }
        }
    }

    foreach (const EntitlementRef &entitlement, entitlementsRemoved)
    {
        if (entitlement->contentConfiguration()->isBaseGame())
        {
            emit removed(proxyForEntitlement(entitlement));
        }
    }

    emit refreshCompleted();
}

void EntitlementManager::refresh() 
{
    GetTelemetryInterface()->Metric_ENTITLEMENT_RELOAD("main");

    ContentController::currentUserContentController()->refreshUserGameLibrary(Engine::Content::UserRefresh);

    // We expose games in common as part of our API while it's provided by another controller internally
    // Manually poke the CommonTitlesController to reload its cache
    Engine::LoginController::currentUser()->socialControllerInstance()->commonTitles()->refresh();
}

QVariant EntitlementManager::getEntitlementById(QString id, Engine::Content::EntitlementFilter filter /*= OwnedContent*/)
{
    EntitlementRef entitlementRef = ContentController::currentUserContentController()->entitlementById(id, filter);

    if (entitlementRef.isNull())
    {
        return QVariant();
    }

    return QVariant::fromValue<QObject*>(proxyForEntitlement(entitlementRef));
}

QVariant EntitlementManager::getEntitlementByProductId(const QString &productId, Engine::Content::EntitlementFilter filter /*= OwnedContent*/)
{
	EntitlementRef entitlementRef = ContentController::currentUserContentController()->entitlementByProductId(productId, filter);

	if (entitlementRef.isNull())
	{
		return QVariant();
	}

	return QVariant::fromValue<QObject*>(proxyForEntitlement(entitlementRef));
}

QVariant EntitlementManager::getEntitlementByContentId(const QString &contentId, Engine::Content::EntitlementFilter filter /*= OwnedContent*/)
{
    // JS - todo - this iface should be updated to return a list of entitlements
	EntitlementRef entitlementRef = ContentController::currentUserContentController()->entitlementByContentId(contentId, filter);

	if (entitlementRef.isNull())
	{
		return QVariant();
	}

	return QVariant::fromValue<QObject*>(proxyForEntitlement(entitlementRef));
}

QObjectList EntitlementManager::getBaseEntitlementsByMasterTitleId(const QString& masterTitleId, Engine::Content::EntitlementFilter filter)
{
    QObjectList ret;

    EntitlementRefList entitlementsBaseByMasterTitleId = ContentController::currentUserContentController()->entitlementsBaseByMasterTitleId(masterTitleId);
    foreach(const EntitlementRef entitlement, entitlementsBaseByMasterTitleId)
    {
        ret.append(proxyForEntitlement(entitlement));
    }

    return ret;
}

bool EntitlementManager::initialRefreshCompleted()
{
    ContentController *cc = ContentController::currentUserContentController();
    return cc ? cc->initialEntitlementRefreshCompleted() : false;
}
    
bool EntitlementManager::refreshInProgress()
{
    return ContentController::currentUserContentController()->updateInProgress();
}

EntitlementManager* EntitlementManager::create()
{
    EntitlementManagerInstance = new EntitlementManager();
    return EntitlementManagerInstance;
}

EntitlementManager* EntitlementManager::instance()
{
    return EntitlementManagerInstance;
}

EntitlementProxy* EntitlementManager::proxyForEntitlement(Origin::Engine::Content::EntitlementRef e)
{
	EntitlementProxy *proxy = mAllProxies[e.data()];

	if (!proxy)
	{
		proxy = new EntitlementProxy(e, EntitlementManager::instance());
		mAllProxies[e.data()] = proxy;
	}

	return proxy;
}

QObject* EntitlementManager::dialogs()
{
    return mDialogProxy;
}
    
}
}
}
