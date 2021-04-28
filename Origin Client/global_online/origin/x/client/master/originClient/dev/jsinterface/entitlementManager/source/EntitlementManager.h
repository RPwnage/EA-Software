#ifndef _ENTITLEMENTMANAGER_H
#define _ENTITLEMENTMANAGER_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QSharedPointer>
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class EntitlementProxy;
class EntitlementDialogProxy;

class ORIGIN_PLUGIN_API EntitlementManager : public QObject
{
	Q_OBJECT
public:
    // The web widget framework destructs us so we need a public destructor
    // instead of a static destroy()
    ~EntitlementManager();

    // Gets a list of dialogs associated with an entitlement.
    Q_PROPERTY(QObject* dialogs READ dialogs);
    QObject* dialogs();

	// Gets a list of all entitlements
    Q_PROPERTY(QObjectList topLevelEntitlements READ topLevelEntitlements);
	Q_INVOKABLE QObjectList topLevelEntitlements();

	// Gets an entitlement by ID
	Q_INVOKABLE QVariant getEntitlementById(QString id, Engine::Content::EntitlementFilter filter = Engine::Content::OwnedContent);
	
    // Gets an entitlement by product ID
	Q_INVOKABLE QVariant getEntitlementByProductId(const QString &productId, Engine::Content::EntitlementFilter filter = Engine::Content::OwnedContent);

	// Gets an entitlement by content ID
	Q_INVOKABLE QVariant getEntitlementByContentId(const QString &contentId, Engine::Content::EntitlementFilter filter = Engine::Content::OwnedContent);

    // Gets an entitlements by master title ID
    Q_INVOKABLE QObjectList getBaseEntitlementsByMasterTitleId(const QString& masterTitleId, Engine::Content::EntitlementFilter filter = Engine::Content::OwnedContent);
	
	// Refreshes the list of entitlements from the server
	Q_INVOKABLE void refresh();

    Q_PROPERTY(bool initialRefreshCompleted READ initialRefreshCompleted);
    bool initialRefreshCompleted();

    Q_PROPERTY(bool refreshInProgress READ refreshInProgress);
    bool refreshInProgress();

    static EntitlementManager* create();
    static EntitlementManager* instance();
	
    // Returns the EntitlementProxy for a given entitlement
	EntitlementProxy* proxyForEntitlement(Origin::Engine::Content::EntitlementRef);

signals:
	void added(QObject *);
	void removed(QObject *);

    void refreshCompleted();
    void refreshStarted();

private:
	EntitlementManager();

private slots:
	void entitlementsListRefreshed(const QList<Origin::Engine::Content::EntitlementRef> entitlementsAdded, QList<Origin::Engine::Content::EntitlementRef> entitlementsRemoved);
    void listenForEntitlementChanges();

private:
	QHash<Engine::Content::Entitlement*, EntitlementProxy*> mAllProxies;
    EntitlementDialogProxy* mDialogProxy;
};

}
}
}

#endif
