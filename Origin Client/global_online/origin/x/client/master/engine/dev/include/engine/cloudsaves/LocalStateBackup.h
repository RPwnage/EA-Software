#ifndef _CLOUDSAVES_LOCALSTATEBACKUP_H
#define _CLOUDSAVES_LOCALSTATEBACKUP_H

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <QFuture> 

#include "FileFingerprint.h"
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{ 
namespace CloudSaves
{
    class LocalStateSnapshot;

    class ORIGIN_PLUGIN_API LocalStateBackup
    {
    public:
        enum BackupReason
        {
            BackupForRollback,
            BackupForSubscriptionUpgrade,
        };

        LocalStateBackup(Content::EntitlementRef entitlement, BackupReason reason);

        const QString &backupPath() const { return mBackupPath; }
        // This function is for tricking the restore to use a secondary backup file that we created for swaping backups
        void setBackupPath(const QString& backupPath) { mBackupPath = backupPath; }
        QDateTime created() const;
        bool isValid() const;

        LocalStateSnapshot *stateSnapshot() const;
        Content::EntitlementRef entitlement() const { return mEntitlement; }

        QFuture<bool> createBackup(const LocalStateSnapshot *currentState);
        QFuture<bool> restore();

        static LocalStateBackup lastBackupForEntitlement(Content::EntitlementRef entitlement);
        static QString backupFileName(Content::EntitlementRef entitlement, LocalStateBackup::BackupReason reason);

    protected:
        Content::EntitlementRef mEntitlement;
        QString mBackupPath;

        mutable QSharedPointer<LocalStateSnapshot> mStateSnapshot;
    };
}
}
}

#endif
