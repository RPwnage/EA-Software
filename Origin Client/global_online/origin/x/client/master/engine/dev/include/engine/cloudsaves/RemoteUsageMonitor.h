#ifndef _CLOUDSAVES_REMOTEUSAGEMONITOR_H
#define _CLOUDSAVES_REMOTEUSAGEMONITOR_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QQueue>
#include <QMutex>
#include <QSet>

#include "engine/content/Entitlement.h"
#include "services/settings/SettingsManager.h"
#include "services/plugin/PluginAPI.h"

class QTimer;

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API LastQueryInfo
    {
    public:
        explicit LastQueryInfo(qint64 size = -1, const QByteArray &etag = QByteArray())
            : m_size(size), m_etag(etag)
        {
        }

        qint64 size() const { return m_size; }
        const QByteArray &etag() const { return m_etag; }

        bool operator!=(const LastQueryInfo &other) const 
        {
            return ((other.size() != size()) || (other.etag() != etag()));
        }

    private:
        qint64 m_size;
        QByteArray m_etag;
    };

    class ORIGIN_PLUGIN_API RemoteUsageMonitor : public QObject
    {
        Q_OBJECT
    public:
        static RemoteUsageMonitor *instance();

        qint64 usageForEntitlement(Content::EntitlementRef entitlement) const;

        static const qint64 UsageUnknown = -1;
        void checkCurrentUsage(Content::EntitlementRef entitlement);
        
        // Called by push and pull when they know the usage for a remote area
        // Returns true if the value changed
        bool setUsageForEntitlement(Content::EntitlementRef entitlement, qint64 usage, const QByteArray &etag = QByteArray());

    signals:
        void usageChanged(Origin::Engine::Content::EntitlementRef entitlement, qint64 newUsage);

    protected slots:
        void backgroundPollTick();        

        void usageQuerySucceeded(qint64 localSize, QByteArray etag);

        void userLoggedIn();
        void userLoggedOut();

        void updateBackgroundTimerActive();
        void settingChanged(const QString& settingName, const Origin::Services::Variant& value);

    protected:
        RemoteUsageMonitor();
        
        bool saveUsage();
        bool restoreUsage();

        void removeUnsafeUsageFiles();

        void resetState();

        // These product IDs have been polled in this session
        QSet<QString> m_checkedProductIds;

        QHash<QString, LastQueryInfo> m_usage;

        // Countdown to the next time we go over the cold list
        unsigned int m_coldCountdown;

        QTimer *m_backgroundPollTimer;
        quint64 m_currentNucleusId;

        mutable QMutex m_usageLock;

        bool m_unsafeFilesRemoved;
    };
}
}
}

#endif
