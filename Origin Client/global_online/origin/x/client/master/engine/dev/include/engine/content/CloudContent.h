#ifndef CLOUDCONTENT_H
#define CLOUDCONTENT_H

#include "engine/cloudsaves/RemoteSync.h"
#include "engine/cloudsaves/SaveFileCrawler.h"
#include "engine/content/Entitlement.h"

#include "services/common/Accessor.h"
#include "services/plugin/PluginAPI.h"
#include "engine/login/User.h"

#include <QObject>
#include <QDialog>
#include <QDomNode>

/// \namespace Origin
namespace Origin
{
    /// \namespace Origin::Engine
    namespace Engine
    {
        /// \namespace Origin::Engine::Content
        namespace Content
        {
            /// \class LocalContent
            class LocalContent;

            /// \class CloudContent
            /// \brief TBD.
            class ORIGIN_PLUGIN_API CloudContent : public QObject
            {
                Q_OBJECT

                friend class LocalContent;

            public:
            
                /// \brief TBD.
                void setUser(UserRef user)  {   m_User = user;  }
            
                /// \brief TBD.
                UserRef user() const        {   return m_User.toStrongRef();    }

                /// \fn ~CloudContent()
                /// \brief The CloudContent destructor; releases the resources used by the CloudContent object.
                ~CloudContent();

                /// \brief Returns the maximum local size for cloud saves.
                qint64 maximumCloudSaveLocalSize() const;

                /// \brief Returns true if we should cloud sync.
                bool shouldCloudSync();

                /// \brief Gets the cloud save file criteria list; thread safe.
                /// \returns Returns the cloud save file criteria list.
                QList<CloudSaves::SaveFileCrawler::EligibleFileRules> getCloudSaveFileCriteria() const;

                /// \brief Returns true if this entitlement has cloud save support.
                bool hasCloudSaveSupport() const;
 
                /// \brief Intiates a push of cloud save to server.
                void pushToCloud(const bool& migrating = false);

                /// \brief Intiates a pull of cloud save from server.
                void pullFromCloud();

                /// \brief Clears the remote area.
                void clearRemoteState(const bool& clearOldStoragePath = false);

                /// \brief Disables cloud storage.
                void disableCloudStorage();

            signals:

                /// \brief TBD.
                void pullFailed(Origin::Engine::CloudSaves::SyncFailure, QString, QString);

                /// \brief TBD.
                void pushFailed(Origin::Engine::CloudSaves::SyncFailure, QString, QString);
                
                /// \brief TBD.
                void remoteSyncStarted(Origin::Engine::Content::EntitlementRef);
                
                /// \brief TBD.
                void remoteSyncFinished(Origin::Engine::Content::EntitlementRef);
                
                /// \brief TBD.
                void cloudSaveProgress(float, qint64, qint64);
                
                /// \brief TBD.
                void externalChangesDetected();

            private:
                void parseCloudSaveFileCriteria(const QDomNode& saveFileCriteria);

                QList<CloudSaves::SaveFileCrawler::EligibleFileRules> mCloudSaveFileCriteria;
                EntitlementRef entitlement() const;
                
                /// \brief The CloudContent constructor.
                /// \param user TBD.
                /// \param parent TBD.
                explicit CloudContent(UserRef user, QObject *parent);

                UserWRef m_User;

            private slots:
				void onSyncFinished(const QString&, Origin::Engine::CloudSaves::RemoteSync::TransferDirection direction, const bool& transferSucceeded, const bool& migrating);
                void onPushFailed(Origin::Engine::CloudSaves::SyncFailure syncFailure);
                void onPullFailed(Origin::Engine::CloudSaves::SyncFailure syncFailure);
            };
        }
    }
}

#endif // CLOUDCONTENT_H
