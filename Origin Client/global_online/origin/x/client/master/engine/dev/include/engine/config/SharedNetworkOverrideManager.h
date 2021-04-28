///////////////////////////////////////////////////////////////////////////////
// SharedNetworkOverrideManager.h
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SHAREDNETWORKOVERRIDEMANAGER_H_
#define _SHAREDNETWORKOVERRIDEMANAGER_H_

#include <QHash>
#include "engine/content/Entitlement.h"

namespace Origin
{
namespace Engine
{
namespace Shared
{
class SharedNetworkOverride;

class ORIGIN_PLUGIN_API SharedNetworkOverrideManager : public QObject
{
	Q_OBJECT

public:
    // Map of current SNO objects.
    typedef QHash <QString, SharedNetworkOverride*> ShareNetworkOverrideList;

    SharedNetworkOverrideManager(QObject* parent = 0);

    ~SharedNetworkOverrideManager();

    void createSharedNetworkOverride(Content::EntitlementRef entitlement);
    
    void deleteSharedNetworkOverride(const QString& offerId);

    // Returns the currently set download override for a SNO corresponding to the offer id.
    const QString downloadOverrideForOfferId(const QString& offerId) const;

    // Returns the pending download override (awaiting confirmation) for a SNO corresponding to the offer id.
    const QString pendingOverrideForOfferId(const QString& offerId) const;

    // Returns a list of the offerId's of the current shared network overrides.
    QStringList overrideList() const;

public slots:
    // For the build confirmation window
    void onConfirmWindowAccepted();
    void onConfirmWindowRejected();
    
    // For the inaccessible folder error window
    void onErrorWindowRetry();
    void onErrorWindowDisable();

signals:
    void showConfirmWindow(Engine::Content::EntitlementRef ent, const QString overridePath, const QString lastModifiedTime);
    void showFolderErrorWindow(Content::EntitlementRef, const QString& path);
    void showInvalidTimeWindow(Content::EntitlementRef ent);
    void confirmWindowAccepted(const QString& offerId);
    void confirmWindowRejected(const QString& offerId);
    void errorWindowRetry(const QString& offerId);
    void errorWindowDisable(const QString& offerId);

private:
    // Hashset from offerId's to shared network override pointers, for all current overrides.
    ShareNetworkOverrideList m_SharedNetworkOverrideInProgressHash;
};

} // namespace Shared
} // namespace Engine
} // namespace Origin

#endif
