/////////////////////////////////////////////////////////////////////////////
// SharedNetworkOverrideManager.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "engine/config/SharedNetworkOverrideManager.h"
#include "engine/config/SharedNetworkOverride.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/Settings/SettingsManager.h"

namespace Origin
{
namespace Engine
{
namespace Shared
{

SharedNetworkOverrideManager::SharedNetworkOverrideManager(QObject* parent)
: QObject(parent)
{
}

void SharedNetworkOverrideManager::createSharedNetworkOverride(Content::EntitlementRef entitlement)
{
    QString folderPath = entitlement->contentConfiguration()->getProductOverrideValue(Services::SETTING_OverrideSNOFolder.name());
    QString updateCheck = entitlement->contentConfiguration()->getProductOverrideValue(Services::SETTING_OverrideSNOUpdateCheck.name());
    QString time = entitlement->contentConfiguration()->getProductOverrideValue(Services::SETTING_OverrideSNOScheduledTime.name());
    bool confirm = entitlement->contentConfiguration()->getProductOverrideValue(Services::SETTING_OverrideSNOConfirmBuild.name());

    const QString offerId(entitlement->contentConfiguration()->productId());
    if (m_SharedNetworkOverrideInProgressHash.contains(offerId))
    {
        SharedNetworkOverride* existingOverride = m_SharedNetworkOverrideInProgressHash[offerId];

        // If the override that we are about to create is exactly equivalent to the existing one, don't create it.
        if (existingOverride->folderPath().compare(folderPath) == 0 && existingOverride->updateCheck().compare(updateCheck) == 0 &&
            existingOverride->time().compare(time) == 0 && existingOverride->confirm() == confirm)
        {
            return;
        }

        existingOverride->deleteLater();      
        m_SharedNetworkOverrideInProgressHash.remove(offerId);
    }

    SharedNetworkOverride* sharedNetworkOverride = new SharedNetworkOverride(entitlement, folderPath, updateCheck, time, confirm, this);
    m_SharedNetworkOverrideInProgressHash.insert(offerId, sharedNetworkOverride);

    ORIGIN_VERIFY_CONNECT(sharedNetworkOverride, &SharedNetworkOverride::showFolderErrorWindow, this, &SharedNetworkOverrideManager::showFolderErrorWindow);
    ORIGIN_VERIFY_CONNECT(sharedNetworkOverride, &SharedNetworkOverride::showConfirmWindow, this, &SharedNetworkOverrideManager::showConfirmWindow);
    ORIGIN_VERIFY_CONNECT(sharedNetworkOverride, &SharedNetworkOverride::showInvalidTimeWindow, this, &SharedNetworkOverrideManager::showInvalidTimeWindow);
    ORIGIN_VERIFY_CONNECT(this, &SharedNetworkOverrideManager::confirmWindowAccepted, sharedNetworkOverride, &SharedNetworkOverride::onConfirmAccepted);
    ORIGIN_VERIFY_CONNECT(this, &SharedNetworkOverrideManager::confirmWindowRejected, sharedNetworkOverride, &SharedNetworkOverride::onConfirmRejected);
    ORIGIN_VERIFY_CONNECT(this, &SharedNetworkOverrideManager::errorWindowRetry, sharedNetworkOverride, &SharedNetworkOverride::onErrorWindowRetry);
    ORIGIN_VERIFY_CONNECT(this, &SharedNetworkOverrideManager::errorWindowDisable, sharedNetworkOverride, &SharedNetworkOverride::onErrorWindowDisable);

    sharedNetworkOverride->start();
}

void SharedNetworkOverrideManager::deleteSharedNetworkOverride(const QString& offerId)
{
    if (m_SharedNetworkOverrideInProgressHash.contains(offerId))
    {
        m_SharedNetworkOverrideInProgressHash[offerId]->deleteLater();
        m_SharedNetworkOverrideInProgressHash.remove(offerId);
    }
}

const QString SharedNetworkOverrideManager::downloadOverrideForOfferId(const QString& offerId) const
{
    if (m_SharedNetworkOverrideInProgressHash.contains(offerId))
    {
        return m_SharedNetworkOverrideInProgressHash[offerId]->downloadOverride();
    }
    else
    {
        return QString("");
    }
}

const QString SharedNetworkOverrideManager::pendingOverrideForOfferId(const QString& offerId) const
{
    if (m_SharedNetworkOverrideInProgressHash.contains(offerId))
    {
        return m_SharedNetworkOverrideInProgressHash[offerId]->pendingOverride();
    }
    else
    {
        return QString("");
    }
}

QStringList SharedNetworkOverrideManager::overrideList() const
{
    return m_SharedNetworkOverrideInProgressHash.keys();
}

void SharedNetworkOverrideManager::onConfirmWindowAccepted()
{
    QObject* window = dynamic_cast<QObject*>(sender());
    emit confirmWindowAccepted(window->property("offerId").toString());
}

void SharedNetworkOverrideManager::onConfirmWindowRejected()
{
    QObject* window = dynamic_cast<QObject*>(sender());
    emit confirmWindowRejected(window->property("offerId").toString());
}

void SharedNetworkOverrideManager::onErrorWindowRetry()
{
    QObject* window = dynamic_cast<QObject*>(sender());
    emit errorWindowRetry(window->property("offerId").toString());
}

void SharedNetworkOverrideManager::onErrorWindowDisable()
{
    QObject* window = dynamic_cast<QObject*>(sender());
    emit errorWindowDisable(window->property("offerId").toString());
}

SharedNetworkOverrideManager::~SharedNetworkOverrideManager()
{
}

} // Shared
} // Engine
} // Origin
