#ifndef _CLOUDSAVES_FILENAMEBLACKLISTMANAGER_H
#define _CLOUDSAVES_FILENAMEBLACKLISTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QByteArray>
#include <QNetworkReply>
#include <QSet>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    typedef QStringList FilenameBlacklist;

    class ORIGIN_PLUGIN_API FilenameBlacklistManager : public QObject 
    {
        Q_OBJECT
    public:
        static FilenameBlacklistManager* instance();
        FilenameBlacklist currentBlacklist() const;

        void startNetworkUpdate();

    private slots:
        void checkNetworkBlacklist();
        void onlineStateChanged();

        void networkCheckFinished();

    private:
        FilenameBlacklistManager();

        bool saveBlacklist();
        bool loadSavedBlacklist();
        void loadBuiltinBlacklist();

        QSet<QString> mBuiltinBlacklist;
        QSet<QString> mRemoteBlacklist;

        QByteArray mRemoteBlacklistEtag;
        mutable QMutex mBlacklistMutex;

        bool mMissedNetworkCheck;
    };
}
}
}

#endif
