#ifndef _SERVERUSERSETTINGS_H
#define _SERVERUSERSETTINGS_H

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QHash>
#include <QSet>
#include <QMutex>

#include "services/settings/setting.h"
#include "services/session/AbstractSession.h"
#include "services/rest/OriginServiceValues.h"

namespace Origin
{
/// \brief The Services namespace.
///
/// \dot
/// 
/// digraph Origin_Services {
/// graph [rankdir="LR"];
/// node [shape="box" fontname="Helvetica" fontsize="9" fontcolor="black" fillcolor="cadetblue1" style="filled"];
/// edge [];
/// 
/// "node0" -> "node1" -> "node2";
/// "node1" -> "node3";
/// "node1" -> "node4";
/// "node1" -> "node5";
/// "node1" -> "node6";
/// "node1" -> "node7";
/// "node1" -> "node8";
/// "node1" -> "node9";
/// "node1" -> "node10";
/// "node1" -> "node11";
/// "node1" -> "node12";
/// "node1" -> "node13";
/// "node1" -> "node14";
/// "node1" -> "node15";
/// "node1" -> "node16";
/// "node16" [label="urls" URL="\ref urls"]
/// "node15" [label="Session" URL="\ref Session"]
/// "node14" [label="ServerUserSettings" URL="\ref ServerUserSettings"]
/// "node13" [label="PlatformService" URL="\ref PlatformService"]
/// "node12" [label="Network" URL="\ref Network"]
/// "node11" [label="LoggerFilter" URL="\ref LoggerFilter"]
/// "node10" [label="JsonUtil" URL="\ref JsonUtil"]
/// "node9" [label="env" URL="\ref env"]
/// "node8" [label="Entitlements\n(See secondary\nnamespace diagram)" fontcolor="blue" fillcolor="lightslateblue" URL="\ref Entitlements"]
/// "node7" [label="Crypto" URL="\ref Origin::Services::Crypto"]
/// "node6" [label="ConversionUtil" URL="\ref ConversionUtil"]
/// "node5" [label="Connection" URL="\ref Connection"]
/// "node4" [label="Cache" URL="\ref Cache"]
/// "node3" [label="ApiValues" URL="\ref ApiValues"]
/// "node2" [label="Achievements" URL="\ref Achievements"]
/// "node1" [label="Services" URL="\ref Services"]
/// "node0" [label="Origin" URL="\ref Origin"]
/// 
/// }
///
/// \enddot
namespace Services
{
namespace ServerUserSettings
{
    ///
    /// \brief High level class for access server-side user settings
    ///
    /// This class is thread-safe
    /// 
    class ServerUserSettingsManager : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// \brief Constructs a ServerUserSettingsManager instance
        ///
        /// \param session  Session to expose settings for
        /// \param parent   QObject parent to use
        ///
        ServerUserSettingsManager(Session::SessionRef session, QObject *parent = NULL);
        ~ServerUserSettingsManager();

        /// \brief Returns if the settings have been loaded at least once from the server
        bool isLoaded() const;

        /// \brief Returns if there are local changes that haven't been propagated to the server
        bool isDirty() const;

        /// \brief Returns the session associated with this instance
        Session::SessionRef session() const { return mSession; }

        ///
        /// \brief Gets the server setting with the specified key
        ///
        /// \param key  Key to query
        /// \param defaultValue  default Value to return
        /// \return Current value or the default value of this setting
        ///
        QVariant get(const QString &key, const QVariant &defaultValue) const;

        ///
        /// \brief Sets the server setting with the specified key
        ///
        /// \param key    Key to set
        /// \param value  New value for the setting
        /// \return True if the update was successful. Note the server update will happen asynchronously
        ///
        bool set(const QString &key, const QVariant &value);

    signals:
        /// \brief Emitted after settings have been read from the server
        void loaded();

        ///
        /// \brief Emitted when a setting has been changed
        ///
        /// \param key         Key that changed
        /// \param value       New value for the key
        /// \param fromServer  True if the update came from the server
        ///
        void valueChanged(QString key, QVariant value, bool fromServer);

    private slots:
        void userOnlineAllChecksCompleted(bool online, Origin::Services::Session::SessionRef session);

        void querySucceeded(QVariantHash queriedSettings);

        void immediateServerUpdate();
        void serverUpdateSucceeded();

    private:
        // Reload our settings from the server
        void reload();

        // Queue a update to the server
        void queueServerUpdate(); // MUST BE CALLED WITH mLock

        // This covers all of our memebers
        mutable QMutex mLock;

        Session::SessionRef mSession;

        QVariantHash mSettings;
        bool mSettingsLoaded;

        QSet<QString> mDirtyKeys;
        bool mServerUpdateQueued;

        // Indicates if we should do an immediate reload after update
        // See userOnlineAllChecksCompleted for an explaination
        bool mReloadAfterUpdate;
    };

    ///
    /// \brief Internal class for requesting and parsing per-user server-side settings
    ///
    class ServerUserSettingsQuery : public QObject
    {
        Q_OBJECT
    public:
        /// \brief Starts the query
        Q_INVOKABLE void startQuery(Origin::Services::Session::SessionRef session);

#ifdef _DEBUG
        void test(Session::SessionRef session);
#endif // _DEBUG

    signals:
        /// \brief Emitted when the query has finished regardless of success
        void finished();

        /// \brief Emitted when the query has successfully finished
        void success(QVariantHash queriedSettings);

        /// \brief Emitted when the query has failed
        void error();

    protected slots:
        void onPrivacySettingsRequestSuccess();
        void onPrivacySettingsRequestError(Origin::Services::restError);
        void onPrivacySettingsTimeoutError();

#ifdef _DEBUG
        void onRestFinished();
#endif

    private:
        void importPayloadBoolean(const QHash<QString, bool> &payloadHash, const QString &key);
        void importPayloadVariant(const QHash<QString, QVariant>& payloadHash, const Setting& setting);
        
        QVariantHash mSettings;
        QSharedPointer<QTimer> mResponseTimer;
    };

    ///
    /// \brief Internal class for update per-user server-side settings
    ///
    class ServerUserSettingsUpdate : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// \brief Builds and starts a new settings update request
        ///
        /// \param  session   Session reference for the target user
        /// \param  settings  Map of all server-side settings. Non-dirty keys must be included.
        /// \param  dirtyKeys Set of dirty keys to update
        ///
        ServerUserSettingsUpdate(Session::SessionRef session, const QVariantHash &settings, const QSet<QString> &dirtyKeys);

        /// \brief Returns the settings this instance was constructed with
        QVariantHash settings() const { return mSettings; }

        /// \brief Returns the dirty keys this instance was constructed with
        QSet<QString> dirtyKeys() const { return mDirtyKeys; }

    signals:
        /// \brief Emitted when the update has finished regardless of success
        void finished();

        /// \brief Emitted when the update has successfully finished
        void success();
        
        /// \brief Emitted when the update has failed
        void error();
    
    private slots:
        void onCategoryUpdateSuccess();
        void onCategoryUpdateError();

    private:
        void sendCategoryUpdate(Session::SessionRef session, ApiValues::privacySettingCategory category, const QString &payload);

        QVariantHash mSettings;
        QSet<QString> mDirtyKeys;
        
        // Number of updates in-flight
        int mPendingCategoryUpdates;
    };
}
}
}

#endif

