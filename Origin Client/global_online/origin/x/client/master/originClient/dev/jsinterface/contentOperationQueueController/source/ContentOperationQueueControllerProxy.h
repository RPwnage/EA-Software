#ifndef CONTENTOPERATIONQUEUECONTROLLERPROXY_H
#define CONTENTOPERATIONQUEUECONTROLLERPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QStringList>
#include <QVariant>
#include "engine/content/Entitlement.h"

namespace Origin
{
namespace Engine
{
namespace Content
{
    class ContentOperationQueueController;
}
}
namespace Client
{
namespace JsInterface
{

class ContentOperationQueueControllerProxy : public QObject
{
    Q_OBJECT
public:
    ContentOperationQueueControllerProxy(QObject* parent = NULL);

    Q_INVOKABLE void remove(const QString& productId);
    Q_INVOKABLE void pushToTop(const QString& productId);
    Q_INVOKABLE int index(const QString& productId);
    Q_INVOKABLE bool isInQueue(const QString& productId);
    Q_INVOKABLE bool isInQueueOrCompleted(const QString& productId);
    Q_INVOKABLE bool queueSkippingEnabled(const QString& productId);
    Q_INVOKABLE bool isHeadBusy();
    Q_INVOKABLE void clearCompleteList();

    // OFM-7120 If you try to install the premium edition, but only have the standard & limited - the DLC will point to the 
    // limited as parent, but will show installing the standard. Lets safe-guard against all of these kind of issues. The parent will be the
    // first valid top level entitlement in the queue.
    Q_INVOKABLE bool isParentInQueue(const QString& childProductId);
    Q_INVOKABLE QString parentOfferIdInQueue(const QString& childProductId);

    Q_PROPERTY(QString headOfferId READ headOfferId);
    QString headOfferId() const;

    Q_PROPERTY(QStringList entitlementsQueuedOfferIdList READ entitlementsQueuedOfferIdList);
    QStringList entitlementsQueuedOfferIdList();

    Q_PROPERTY(QStringList entitlementsCompletedOfferIdList READ entitlementsCompletedOfferIdList);
    QStringList entitlementsCompletedOfferIdList();

signals:
    void enqueued(const QString& offerId);
    void removed(const QString& offerId, const bool& removeChildren, const bool& enqueueNext);
    void addedToComplete(const QString& offerId);
    void completeListCleared();
    // Has the head item gone into or out of an install state
    void headBusy(const bool& isBusy);
    void headChanged(const QString& newHeadOfferId, const QString& oldHeadOfferId);

protected slots:
    void onEnqueued(Origin::Engine::Content::EntitlementRef entitlement);
    void onAddedToComplete(Origin::Engine::Content::EntitlementRef entitlement);
    void onHeadChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead);

private:
    /// \brief Returns the entitlement base game.
    Origin::Engine::Content::EntitlementRef parentInQueue_internal(const QString& childProductId);

    Origin::Engine::Content::ContentOperationQueueController* mQueueController;
};

}
}
}

#endif
