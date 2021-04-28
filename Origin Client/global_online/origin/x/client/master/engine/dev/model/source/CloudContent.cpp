#include <limits>
#include <QDateTime>
#include <QSettings>
#include <QFile>

#include "engine/content/CloudContent.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/login/LoginController.h"
#include "engine/cloudsaves/RemoteSyncManager.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "services/debug/DebugService.h"
#include "engine/cloudsaves/ClearRemoteStateManager.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

using namespace Origin::Engine::CloudSaves;
using namespace Origin::Services;

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            CloudContent::CloudContent(UserRef user, QObject *parent)
                :QObject(parent)
            {
                setUser(user);

                const QString saveFileCriteria = entitlement()->contentConfiguration()->saveFileCriteria(); 

                if (saveFileCriteria.trimmed().isEmpty())
                {
                    // Nothing to do
                    return;
                }

                QDomDocument saveCriteriaDoc;
                const bool parsed = saveCriteriaDoc.setContent(saveFileCriteria);

                if (parsed)
                {
                    parseCloudSaveFileCriteria(saveCriteriaDoc.firstChild());
                }
                else
                {
                    ORIGIN_LOG_WARNING << "Unable to parse cloud save file criteria as XML: " << saveFileCriteria;
                    ORIGIN_ASSERT(0);
                }
            }

            CloudContent::~CloudContent()
            {
            }

            qint64 CloudContent::maximumCloudSaveLocalSize() const
            {
                // XXX: Should be configurable. See GOPFR-374
                // cap increased to 1 GB, OFM-344
                qint64 ret = (qint64)1024 * 1024 * 1024;
                return ret;
            }

            bool CloudContent::shouldCloudSync()
            {
                // use local shared pointer to keep the user instance alive
                UserRef usr = user();
                if(usr.isNull())
                    return false;
                bool enableCloudSave = Services::readSetting(Services::SETTING_EnableCloudSaves, user()->getSession());
                return enableCloudSave && hasCloudSaveSupport() && usr && Origin::Services::Connection::ConnectionStatesService::isUserOnline (usr->getSession());
            }

            void CloudContent::pullFromCloud()
            {
                RemoteSync* remotePull = RemoteSyncManager::instance()->remoteSyncByEntitlement(entitlement());
                if(! remotePull)
                {
                    remotePull = RemoteSyncManager::instance()->createPullRemoteSync(entitlement());

                    ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(externalChangesDetected()), this, SIGNAL(externalChangesDetected()));
                    ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(progress(float, qint64, qint64)),this,SIGNAL(cloudSaveProgress(float, qint64, qint64)));
                    ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(releasedByManager(const QString&, Origin::Engine::CloudSaves::RemoteSync::TransferDirection, const bool&, const bool&)), this, SLOT(onSyncFinished(const QString&, Origin::Engine::CloudSaves::RemoteSync::TransferDirection, const bool&, const bool&)));

                    ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOfflineUser()),
                        remotePull, SLOT(cancel()));

                    emit remoteSyncStarted(entitlement());

                    remotePull->start();
                }

                ORIGIN_VERIFY_CONNECT(remotePull, SIGNAL(failed(Origin::Engine::CloudSaves::SyncFailure)), this, SLOT(onPullFailed(Origin::Engine::CloudSaves::SyncFailure)));
            }

			void CloudContent::onSyncFinished(const QString& offerId, RemoteSync::TransferDirection direction, const bool& transferSucceeded, const bool& migrating)
			{
				// Are we in the process of migrating cloud saves?
                if (transferSucceeded && migrating)
                {
                    if (direction == RemoteSync::PullFromCloud)
                    {
                        // We just successfully pulled from the old cloud storage path.  Now we push to the new cloud storage path.
                        pushToCloud(true);
                    }
                    
                    else if (direction == RemoteSync::PushToCloud)
                    {
                        // We just successfully pushed to the new cloud path.  Now we need to delete all of the cloud saves from the old path.
                        clearRemoteState(true);

                        // telemetry - successful cloud migration
                        GetTelemetryInterface()->Metric_CLOUD_MIGRATION_SUCCESS(offerId.toLocal8Bit().constData());

                        emit remoteSyncFinished(entitlement());
                    }
                }
                else
                {
                    emit remoteSyncFinished(entitlement());
                }
			}

            void CloudContent::onPullFailed(Origin::Engine::CloudSaves::SyncFailure syncFailure)
            {
                emit pullFailed(syncFailure, entitlement()->contentConfiguration()->displayName(), entitlement()->contentConfiguration()->productId());
            }

            void CloudContent::onPushFailed(Origin::Engine::CloudSaves::SyncFailure syncFailure)
            {
                emit pushFailed(syncFailure, entitlement()->contentConfiguration()->displayName(), entitlement()->contentConfiguration()->productId());
            }

            bool CloudContent::hasCloudSaveSupport() const	
            { 
                 bool empty = entitlement()->contentConfiguration()->saveFileCriteria().isEmpty();
                 return !empty; 
            }

            void CloudContent::pushToCloud(const bool& migrating /* = false */)
            {
                RemoteSync* remotePush = RemoteSyncManager::instance()->remoteSyncByEntitlement(entitlement());
                if(! remotePush)
                {
                    // Push their save file changes back to the cloud
                    remotePush = RemoteSyncManager::instance()->createPushRemoteSync(entitlement());

                    ORIGIN_VERIFY_CONNECT(remotePush, SIGNAL(releasedByManager(const QString&, Origin::Engine::CloudSaves::RemoteSync::TransferDirection, const bool&, const bool&)), this, SLOT(onSyncFinished(const QString&, Origin::Engine::CloudSaves::RemoteSync::TransferDirection, const bool&, const bool&)));
                    ORIGIN_VERIFY_CONNECT(remotePush, SIGNAL(failed(Origin::Engine::CloudSaves::SyncFailure)), this, SLOT(onPushFailed(Origin::Engine::CloudSaves::SyncFailure)));
                    ORIGIN_VERIFY_CONNECT(remotePush, SIGNAL(progress(float, qint64, qint64)),this,SIGNAL(cloudSaveProgress(float, qint64, qint64)));
                    
                    emit remoteSyncStarted(entitlement());

                    if (migrating)
                    {
                        remotePush->setMigrating(true);
                    }

                    remotePush->start();
                }
            }

            void CloudContent::clearRemoteState(const bool& clearOldStoragePath /* = false */)
            {
                ClearRemoteStateManager::instance()->clearRemoteStateForEntitlement(entitlement(), clearOldStoragePath);
            }

            EntitlementRef CloudContent::entitlement() const
            {
                LocalContent *localContent = static_cast<LocalContent *>(parent());

                if(localContent)
                {
                    return localContent->entitlementRef();
                }
                else 
                {
                    return EntitlementRef();
                }
            }

            void CloudContent::disableCloudStorage()
            {
                Services::writeSetting(Services::SETTING_EnableCloudSaves, false, user()->getSession());

                //if(RequiredToPlay::GetRtpLaunchActive())
                //{	
                //	RequiredToPlay::ClearRtpLaunchInfo(contentConfiguration()->contentId());
                //}

            }

            void CloudContent::parseCloudSaveFileCriteria(const QDomNode& saveFileCriteria)
            {
                if (saveFileCriteria.nodeName() != "saveFileCriteria")
                {
                    ORIGIN_LOG_WARNING << "Expected root node of save file criteria to be <saveFileCriteria>, instead got <" << saveFileCriteria.nodeName() << ">";
                    ORIGIN_ASSERT(0);
                }

                QDomNodeList nodeList = saveFileCriteria.childNodes();

                for(int count = 0; count < nodeList.count(); ++count)
                {
                    QDomElement element = nodeList.at(count).toElement();

                    if (element.isNull())
                    {
                        continue;
                    }

                    int order; 
                    
                    if (element.hasAttribute("order"))
                    {
                        bool ok;
                        const QString orderString = element.attribute("order");

                        order = orderString.toInt(&ok);

                        if (!ok)
                        {
                            ORIGIN_LOG_WARNING << "Unable to parse cloud save file criteria order attribute: " << orderString;
                            ORIGIN_ASSERT(0);

                            order = mCloudSaveFileCriteria.size();
                        }
                    }
                    else
                    {
                        order = mCloudSaveFileCriteria.size();		// append to the end if there is no order attribute
                    }

                    if (element.tagName() == "include")
                    {
                        mCloudSaveFileCriteria.insert(order, SaveFileCrawler::EligibleFileRules(element.text(), SaveFileCrawler::IncludeFile));
                    }
                    else if (element.tagName() == "exclude")
                    {
                        mCloudSaveFileCriteria.insert(order, SaveFileCrawler::EligibleFileRules(element.text(), SaveFileCrawler::ExcludeFile));
                    }
                    else
                    {
                        ORIGIN_LOG_WARNING << "Unexpected element while parsing cloud save file criteria: " << element.tagName();
                        ORIGIN_ASSERT(0);
                    }
                }
            }

            QList<SaveFileCrawler::EligibleFileRules> CloudContent::getCloudSaveFileCriteria() const
            {
                return mCloudSaveFileCriteria;
            }

        }


    }
}
