/////////////////////////////////////////////////////////////////////////////
// SharedNetworkOverride.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "engine/config/SharedNetworkOverride.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentController.h"
#include "services/log/LogService.h"
#include "services/Settings/SettingsManager.h"
#include <qtconcurrentrun.h>

namespace Origin
{
namespace Engine
{
namespace Shared
{

// 1 minute timer (in ms) to poll the network folder, when update check is set for "latest"
const qint32 TIMER_DURATION = 60*1000;

SharedNetworkOverride::SharedNetworkOverride(Content::EntitlementRef ent, QString folderPath, QString updateCheck, QString time, bool confirm, QObject* parent) 
    :QObject(parent),
    m_entitlement(ent),
    m_folderPath(folderPath),
    m_updateCheck(updateCheck),
    m_time(time), 
    m_confirm(confirm),
    m_pendingOverridePath(""),
    m_downloadOverridePath(""),
    m_pollInProgress(false)
{
}

void SharedNetworkOverride::start()
{
    m_timer = new QTimer(this);
    ORIGIN_VERIFY_CONNECT(m_timer, SIGNAL(timeout()), this, SLOT(pollFolder()));

    ORIGIN_VERIFY_CONNECT(&m_folderCheckOperationWatcher, SIGNAL(finished()), this, SLOT(onAsyncFolderCheckFinished()));

    if (m_updateCheck.compare("latest") == 0)
    {
        m_timer->start(TIMER_DURATION);
    }
    else
    {
        // If we're doing scheduled updates, parse the time now.
        m_parsedTime = QTime::fromString(m_time, "h:mm");

        if (!m_parsedTime.isValid())
        {
            emit showInvalidTimeWindow(m_entitlement);
            return;
        }
    }
    pollFolder();
}

void SharedNetworkOverride::pollFolder()
{
    if (m_pollInProgress)
    {
        return;
    }

    m_pollInProgress = true;
    m_pendingOverridePath = "";

    // If we're doing scheduled updates, then setup our timer now for the next update check.
    if (m_updateCheck.compare("scheduled") == 0)
    {
        QDateTime currentTime = QDateTime::currentDateTime();
        QDateTime updateTime(QDate::currentDate(), m_parsedTime);
        if (updateTime <= currentTime)
        {
            updateTime = updateTime.addDays(1);
        }
        qint32 msecsTillTimeout = currentTime.msecsTo(updateTime);
        m_timer->stop();
        m_timer->start(msecsTillTimeout);
        ORIGIN_LOG_DEBUG << QString("Next SNO scheduled build check for %1 will occur in %2 minutes").arg(m_entitlement->contentConfiguration()->displayName()).arg(QString::number(msecsTillTimeout / 1000 / 60));
    }

    m_folderCheckOperation = QtConcurrent::run(this, &SharedNetworkOverride::asyncFolderCheck);
    m_folderCheckOperationWatcher.setFuture(m_folderCheckOperation);
}

bool SharedNetworkOverride::asyncFolderCheck() const
{
    QDir dir(m_folderPath);
    return dir.exists();
}
    
void SharedNetworkOverride::onAsyncFolderCheckFinished()
{
    if (!m_folderCheckOperation.result())
    {
        m_downloadOverridePath = "";
        emit(showFolderErrorWindow(m_entitlement, m_folderPath));
        return;
    }
    
    QDir dir(m_folderPath);

    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    dir.setNameFilters(QStringList("*.zip"));
    dir.setSorting(QDir::Time);
    QFileInfoList dirList = dir.entryInfoList();

    //for (int i = 0; i < dirList.size(); i++)
    //{
    //    QFileInfo fileInfo = dirList.at(i);
    //    ORIGIN_LOG_DEBUG << fileInfo.absoluteFilePath() << " " << fileInfo.lastModified().toString();
    //}
    
    if (dirList.isEmpty())
    {
        m_downloadOverridePath = "";
        m_pollInProgress = false;
        return;
    }
    
    // Keep track of the last modified time for the pending override, in case it needs to be shown in a confirm window.
    QString lastModifiedTime;
    
    if (m_updateCheck.compare("latest") == 0)
    {
        m_pendingOverridePath = dirList.at(0).absoluteFilePath();
        lastModifiedTime = dirList.at(0).lastModified().toLocalTime().toString(Qt::TextDate);
    }
    else
    {
        QDateTime updateTime(QDate::currentDate(), m_parsedTime);
        for (int i = 0; i < dirList.size(); i++)
        {
            QFileInfo fileInfo = dirList.at(i);
            QDateTime fileTime(fileInfo.lastModified());
                
            // Files are sorted chronologically, with most recent first.  Pick the first file that is not more recent than our scheduled update time.
            if (fileTime < updateTime)
            {
                m_pendingOverridePath = fileInfo.absoluteFilePath();
                lastModifiedTime = fileInfo.lastModified().toLocalTime().toString(Qt::TextDate);
                break;
            }
        }
        // If there were no files in the directory that are older than our scheduled update time, exit.
        if (m_pendingOverridePath.compare("") == 0)
        {
            m_downloadOverridePath = "";
            m_pollInProgress = false;
            return;
        }
    }

    // Add the "file:" prefix, unless the path starts with:
    if (m_pendingOverridePath.indexOf("file:") == -1 && // an existing "file:" prefix.
        m_pendingOverridePath.indexOf("http:") == -1 && // an "http:" web prefix
        m_pendingOverridePath.indexOf("https:") == -1 && // an "https:" web prefix
        m_pendingOverridePath.indexOf("ftp:") == -1 ) //an "ftp:" web prefix
    {
        m_pendingOverridePath = "file:" + m_pendingOverridePath;
    }
     
    // If the override path hasn't changed, exit.
    if (m_pendingOverridePath.compare(m_downloadOverridePath) == 0)
    {
        m_pollInProgress = false;
        return;
    }

    if (m_confirm == true)
    {
        if (m_rejectedDownloadOverrides.contains(m_pendingOverridePath))
        {
            // User has already rejected this download override path.  Do not reset m_downloadOverridePath.
            // It may have previously been set to a valid override that the use wishes to keep.
            m_pollInProgress = false;
            return;
        }
        emit showConfirmWindow(m_entitlement, m_pendingOverridePath, lastModifiedTime);
    }
    else
    {
        m_downloadOverridePath = m_pendingOverridePath;
        m_pollInProgress = false;
        updateDownloadOverride();
    }
}

void SharedNetworkOverride::updateDownloadOverride()
{
    // It's likely that the initial library refresh is happening when this logic is hit, so 
    // manually reload overrides instead of going through ContentController::refreshUserGameLibrary
    Services::SettingsManager::instance()->reloadProductOverrideSettings();
    m_entitlement->contentConfiguration()->reloadOverrides();
    if (Content::ContentController::currentUserContentController()->autoPatchingEnabled() && m_entitlement->localContent()->updateAvailable())
    {
        m_entitlement->localContent()->update();
    }
}

void SharedNetworkOverride::onConfirmAccepted(const QString& offerId)
{
    if (offerId != m_entitlement->contentConfiguration()->productId())
    {
        return;
    }
    m_downloadOverridePath = m_pendingOverridePath;
    m_pollInProgress = false;
    updateDownloadOverride();
}

void SharedNetworkOverride::onConfirmRejected(const QString& offerId)
{
    if (offerId != m_entitlement->contentConfiguration()->productId())
    {
        return;
    }
    m_rejectedDownloadOverrides << m_pendingOverridePath;
    m_pollInProgress = false;
}

void SharedNetworkOverride::onErrorWindowRetry(const QString& offerId)
{
    if (offerId != m_entitlement->contentConfiguration()->productId())
    {
        return;
    }
    m_pollInProgress = false;
    
    // Retry the folder poll
    pollFolder();
}

void SharedNetworkOverride::onErrorWindowDisable(const QString& offerId)
{
    if (offerId != m_entitlement->contentConfiguration()->productId())
    {
        return;
    }
    // We can disable this SNO by simply turning off the timer.  This SNO will never be re-enabled, but we
    // can not destroy this SNO, because it will be re-created the next time that product overrides are reloaded.
    // Leave m_pollInProgress as true, as a backup measure.
    m_timer->stop();
}

} // Shared
} // Engine
} // Origin
