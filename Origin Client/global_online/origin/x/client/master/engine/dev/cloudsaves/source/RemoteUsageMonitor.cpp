#include <QTimer>
#include <algorithm>
#include <QDir>
#include <QDataStream>
#include <QMutexLocker>
#include <QCryptographicHash>

#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "engine/cloudsaves/UsageQuery.h"
#include "engine/cloudsaves/FilesystemSupport.h"
#include "engine/login/LoginController.h"

#include "engine/content/ContentController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/CloudContent.h"
#include "engine/content/ContentConfiguration.h"

namespace
{
    using namespace Origin::Engine::CloudSaves;

    QString userRemoteUsageFile(quint64 nucleusId) 
    {
        using namespace FilesystemSupport;
        QCryptographicHash hasher(QCryptographicHash::Sha1);
        quint64 salt = 8273582635;
        nucleusId ^= salt;
        hasher.addData(reinterpret_cast<const char *>(&nucleusId), 8);

        return cloudSavesDirectory(RoamingRoot).absoluteFilePath(hasher.result().toHex() + ".usage");
    }
    
    const unsigned int PollIntervalMs = 5 * 60 * 1000;
    const unsigned int PollFuzzMs = 60 * 1000;

    // Random magic number for our usage file
    const quint32 UsageFileMagicNumber = 0x1c2c02a1;

    RemoteUsageMonitor *g_remoteUsageMonitorInstance;
}

// Note: Clang seems to have issue with operator overloads in anonymous namespaces so we dump these here.
QDataStream &operator<<(QDataStream &stream, const LastQueryInfo &lastInfo)
{
    return stream << lastInfo.size() << lastInfo.etag();
}

QDataStream &operator>>(QDataStream &stream, LastQueryInfo &lastInfo)
{
    QByteArray etag;
    qint64 size;

    QDataStream &result = stream >> size >> etag;

    lastInfo = LastQueryInfo(size, etag);

    return result;
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    RemoteUsageMonitor::RemoteUsageMonitor() : 
        m_backgroundPollTimer(NULL),
        m_currentNucleusId(0),
        m_unsafeFilesRemoved(false)
    {        
        ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(userLoggedOut()));
        ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(userLoggedIn()));
    }

    void RemoteUsageMonitor::userLoggedIn()
    {
        // Reset our state
        resetState();
        m_currentNucleusId = Origin::Engine::LoginController::currentUser()->userId();
        
        ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
            this, SLOT(updateBackgroundTimerActive()));

        ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
            this, SLOT(settingChanged(const QString&, const Origin::Services::Variant&)));

        restoreUsage();

        updateBackgroundTimerActive();
    }
        
    void RemoteUsageMonitor::userLoggedOut()
    {
        // Clear out our state
        // People shouldn't be polling us while we're logged out; hopefully returning
        // nothing of value will make it more obvious if that happens
        resetState();
        m_currentNucleusId = 0;

        ORIGIN_VERIFY_DISCONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), 
            this, SLOT(updateBackgroundTimerActive()));

        updateBackgroundTimerActive();
    }
        
    void RemoteUsageMonitor::settingChanged(const QString& settingName, const Origin::Services::Variant& value)
    {
        if (settingName == Services::SETTING_EnableCloudSaves.name())
        {
            updateBackgroundTimerActive();
        }
    }

    void RemoteUsageMonitor::updateBackgroundTimerActive()
    {
        UserRef user = LoginController::instance()->currentUser();

        bool shouldBeActive =
            !user.isNull() &&
            Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()) &&
            Services::readSetting(Services::SETTING_EnableCloudSaves, user->getSession());

        if (shouldBeActive && !m_backgroundPollTimer)
        {
            // Start up the timer
            m_backgroundPollTimer = new QTimer(this);
            m_backgroundPollTimer->setInterval(PollIntervalMs + (rand() % PollFuzzMs));
    
            ORIGIN_VERIFY_CONNECT(m_backgroundPollTimer, SIGNAL(timeout()), this, SLOT(backgroundPollTick()));
            
            m_backgroundPollTimer->start();
        }
        else if (!shouldBeActive && m_backgroundPollTimer)
        {
            // Stop polling
            delete m_backgroundPollTimer;
            m_backgroundPollTimer = NULL;
        }
    }
        
    void RemoteUsageMonitor::resetState()
    {
        m_checkedProductIds.clear();
        m_usage.clear();
    }
        
    void RemoteUsageMonitor::checkCurrentUsage(Content::EntitlementRef entitlement)
    {
        QByteArray lastEtag;
        const QString productId(entitlement->contentConfiguration()->productId());

        if (m_usage.contains(productId))
        {
            // We use this to avoid a full fetch of the manifest if it hasn't changed
            lastEtag = m_usage[productId].etag();
        }

        UsageQuery *query = new UsageQuery(entitlement, lastEtag);

        ORIGIN_VERIFY_CONNECT(query, SIGNAL(failed()), query, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(query, SIGNAL(unchanged()), query, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(query, SIGNAL(succeeded(qint64, QByteArray)), this, SLOT(usageQuerySucceeded(qint64, QByteArray)));

        // Mark that we attempted to check if it succeeded or not
        m_checkedProductIds << productId;

        query->start();
    }

    RemoteUsageMonitor* RemoteUsageMonitor::instance()
    {
        if (g_remoteUsageMonitorInstance == NULL)
        {
            g_remoteUsageMonitorInstance = new RemoteUsageMonitor;
        }

        return g_remoteUsageMonitorInstance;
    }
        
    qint64 RemoteUsageMonitor::usageForEntitlement(Content::EntitlementRef entitlement) const
    {
        ORIGIN_ASSERT(m_currentNucleusId);
        QMutexLocker locker(&m_usageLock);

        QString productId(entitlement->contentConfiguration()->productId());

        if (!m_usage.contains(productId))
        {
            // Haven't polled yet
            return UsageUnknown;
        }

        return m_usage.value(productId).size();
    }
        
    bool RemoteUsageMonitor::setUsageForEntitlement(Content::EntitlementRef entitlement, qint64 usage, const QByteArray &etag)
    {
        ORIGIN_ASSERT(m_currentNucleusId);
        bool sizeChanged = false;
        const QString productId(entitlement->contentConfiguration()->productId());

        {
            QMutexLocker locker(&m_usageLock);

            const LastQueryInfo queryInfo(usage, etag);

            if (!m_usage.contains(productId) || (m_usage[productId] != queryInfo))
            {
                if (m_usage[productId].size() != queryInfo.size())
                {
                    sizeChanged = true;
                }

                // Size or ETag changed
                m_usage[productId] = queryInfo;
                saveUsage();
            }
        }

        if (sizeChanged)
        {
            emit usageChanged(entitlement, usage);
        }

        return sizeChanged;
    }
        
    void RemoteUsageMonitor::backgroundPollTick()
    {
        // Get a list of current entitlements

        Content::EntitlementRefList currentEntitlements = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();

        for(Content::EntitlementRefList::const_iterator it = currentEntitlements.constBegin();
            it != currentEntitlements.constEnd();
            it++)
        {            
            if ((*it)->localContent()->cloudContent()->hasCloudSaveSupport() &&
                (*it)->localContent()->installed() &&
                !m_checkedProductIds.contains((*it)->contentConfiguration()->productId()))
            {
                checkCurrentUsage(*it);

                // That's enough work for one tick
                break;
            }
        }
    }    
        
    void RemoteUsageMonitor::usageQuerySucceeded(qint64 localSize, QByteArray etag)
    {
        UsageQuery *query = dynamic_cast<UsageQuery*>(sender());

        ORIGIN_ASSERT(query);
        query->deleteLater();
        
        setUsageForEntitlement(query->entitlement(), localSize, etag);
    }
        
    // Should be called with m_usageLock
    bool RemoteUsageMonitor::saveUsage()
    {
        if (!m_unsafeFilesRemoved)
        {
            m_unsafeFilesRemoved = true;
            removeUnsafeUsageFiles();
        }
        
        QFile usageFile(userRemoteUsageFile(m_currentNucleusId));

        if (!usageFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            // We don't really care; this is an optimization
            return false;
        }
        
        QDataStream saveStream(&usageFile);

        saveStream << UsageFileMagicNumber;
        saveStream << m_usage;

        return true;
    }

    bool RemoteUsageMonitor::restoreUsage()
    {
        if (!m_unsafeFilesRemoved)
        {
            m_unsafeFilesRemoved = true;
            removeUnsafeUsageFiles();
        }

        QFile usageFile(userRemoteUsageFile(m_currentNucleusId));

        if (!usageFile.open(QIODevice::ReadOnly))
        {
            return false;
        }
        
        QMutexLocker locker(&m_usageLock);
        QDataStream loadStream(&usageFile);
        
        quint32 magicNumber;
        loadStream >> magicNumber;

        if (magicNumber != UsageFileMagicNumber)
        {
            return false;
        }

        loadStream >> m_usage;
        
        return (loadStream.status() == QDataStream::Ok);
    }

    void RemoteUsageMonitor::removeUnsafeUsageFiles()
    {
        using namespace FilesystemSupport;
        QDir folder = cloudSavesDirectory(RoamingRoot);
        QStringList files = folder.entryList();

        for (int i = 0; i < files.length(); i++)
        {
            QString file = files[i];
            QString fileWithExtension = file;
            file.remove(".usage");
            if (file.toInt() != 0)
            {
                // This is a nucleus ID, remove it
                QFile::remove(folder.absolutePath() + "/" + fileWithExtension);
            }
        }
    }

}
}
}
