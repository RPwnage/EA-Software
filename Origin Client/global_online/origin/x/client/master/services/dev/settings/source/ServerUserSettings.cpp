#include "ServerUserSettings.h"

#include <QHash>
#include <QMetaObject>
#include <QMutexLocker>
#include <QTimer>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/AtomServiceClient.h"
#include "services/rest/AtomServiceResponses.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/settings/SettingsManager.h"
#include "services/downloader/Common.h"

namespace
{
    // Privacy category names
    const QString GeneralCategory("GENERAL");
    const QString IgoCategory("INGAME");
    const QString HiddenProductIdsCategory("HIDDENGAMES");
    const QString FavoriteProductIdsCategory("FAVORITEGAMES");
    const QString VideoBroadcastCategory("BROADCASTING");

    // Version number for the hidden content IDs payout
    const QString HiddenProductIdsPayloadVersion("1.0");

    // Parses a privacy string in to a hash of boolean
    QHash<QString,bool> privacyStringToHash(const QString &string)
    {
        QStringList settingsParts = string.split(";", QString::SkipEmptyParts);
        QHash<QString, bool> settings;

        for(QStringList::ConstIterator it = settingsParts.constBegin();
            it != settingsParts.constEnd();
            it++)
        {
            QString key = (*it).section("=", 0, 0);
            QString value = (*it).section("=", 1, 1);

            settings[key] = (value == "1");
        }

        return settings;
    }
	
	// Parses a privacy string in to a hash of QVariant
	QHash<QString, QVariant> privacyStringtoVariantHash(const QString &string)
	{
		QStringList settingsParts = string.split(";", QString::SkipEmptyParts);
        QHash<QString, QVariant> settings;

        for(QStringList::ConstIterator it = settingsParts.constBegin();
            it != settingsParts.constEnd();
            it++)
        {
            QString key = (*it).section("=", 0, 0);
            QString value = (*it).section("=", 1, 1);
            
            settings[key] = value;
        }

        return settings;
	}

    // Serializes a hash of booleans in to a privacy string
    QString privacyHashToString(const QHash<QString, bool> &hash)
    {
        QStringList settingParts;

        for(QHash<QString, bool>::ConstIterator it = hash.constBegin();
            it != hash.constEnd();
            it++)
        {
            settingParts << it.key().toLower() + "=" + (it.value() ? "1" : "0");
        }
        
        return settingParts.join(";");
    }

	// Serializes a hash of QVariants in to a privacy string
    QString privacyHashToString(const QHash<QString, QVariant> &hash)
    {
        QStringList settingParts;

        for(QHash<QString, QVariant>::ConstIterator it = hash.constBegin();
            it != hash.constEnd();
            it++)
        {
            settingParts << it.key().toLower() + "=" + (it.value().toString());
        }
        
        return settingParts.join(";");
    }

    // Exports a setting from a settings hash to a target payload boolean hash
    void exportPayloadBoolean(QHash<QString, bool> &targetHash, const QVariantHash &sourceHash, const QString &key)
    {
        if (sourceHash.contains(key))
        {
            targetHash[key] = sourceHash[key].toBool();
        }
    }

	// Exports a setting from a settings hash to a target payload QVariant hash
	void exportPayloadVariant(QHash<QString, QVariant> &targetHash, const QVariantHash &sourceHash, const QString &key)
    {
        if (sourceHash.contains(key))
        {
            targetHash[key] = sourceHash[key];
        }
    }
}

namespace Origin
{
namespace Services
{
namespace ServerUserSettings
{
    ServerUserSettingsManager::ServerUserSettingsManager(Session::SessionRef session, QObject *parent) : 
        QObject(parent),
        mSession(session),
        mSettingsLoaded(false),
        mServerUpdateQueued(false),
        mReloadAfterUpdate(false)
    {
        // Listen for when we leave offline mode
        ORIGIN_VERIFY_CONNECT(Session::SessionService::instance(),
                SIGNAL(userOnlineAllChecksCompleted(bool, Origin::Services::Session::SessionRef)),
                this,
                SLOT(userOnlineAllChecksCompleted(bool, Origin::Services::Session::SessionRef)));

        // Load our settings
        reload();
    }
    
    ServerUserSettingsManager::~ServerUserSettingsManager()
    {
        if (isDirty())
        {
            ORIGIN_LOG_WARNING << "Destroying server-side user settings while local settings are dirty. Setting changes may be lost.";
        }
    }

    bool ServerUserSettingsManager::isLoaded() const
    {
        QMutexLocker lock(&mLock);
        return mSettingsLoaded;
    }

    bool ServerUserSettingsManager::isDirty() const
    {
        QMutexLocker lock(&mLock);
        return !mDirtyKeys.isEmpty();
    }

    QVariant ServerUserSettingsManager::get(const QString &key, const QVariant &defaultValue) const
    {
        QMutexLocker lock(&mLock);
        if (mSettings.contains(key))
            return mSettings[key];
        else
            return defaultValue;
    }

    bool ServerUserSettingsManager::set(const QString &key, const QVariant &value)
    {
        bool changed = false;

        {
            QMutexLocker lock(&mLock);

            // We don't support setting server side settings before they're loaded
            if (!mSettingsLoaded)
            {
                ORIGIN_LOG_WARNING << "Server side settings are not loaded.";
                return false;
            }

            // Check to make sure we're actually changing a value
            if (mSettings[key] != value)
            {
                // Update our local copy of the setting
                mSettings[key] = value;

                // Mark the key as dirty
                mDirtyKeys << key;

                // Queue an update
                queueServerUpdate();

                changed = true;
            }
        }
            
        if (changed)
        {
            emit valueChanged(key, value, false);
        }

        return true;
    }

    void ServerUserSettingsManager::userOnlineAllChecksCompleted(bool online, Origin::Services::Session::SessionRef session)
    {
        if (online && (mSession == session))
        {
            if (isDirty())
            {
                // Try to push our updates to the server
                QMutexLocker lock(&mLock);
                queueServerUpdate();

                // Don't try to reload or else we might overwrite our dirty settings with the server copy
                // Set a flag so we do a reload after update
                mReloadAfterUpdate = true;
            }
            else
            {
                // Check for changes on the server
                reload();
            }
        }
    }

    void ServerUserSettingsManager::reload()
    {
        if (Connection::ConnectionStatesService::isUserOnline(mSession))
        {
            ServerUserSettingsQuery *query = new ServerUserSettingsQuery();

            // During start-up, it's possible that the main thread is tied up when the query response comes back.
            // Because this query has a short (5000ms) timeout, we need to handle this query on a background thread,
            // or else we risk triggering an artificial failure.
            QThread *bgThread = new QThread();
            bgThread->start();
            query->moveToThread(bgThread);

            ORIGIN_VERIFY_CONNECT(query, &ServerUserSettingsQuery::success, this, &ServerUserSettingsManager::querySucceeded);
            ORIGIN_VERIFY_CONNECT(query, &ServerUserSettingsQuery::finished, query, &ServerUserSettingsQuery::deleteLater);
            ORIGIN_VERIFY_CONNECT(query, &ServerUserSettingsQuery::finished, bgThread, &QThread::quit);
            ORIGIN_VERIFY_CONNECT(bgThread, &QThread::finished, bgThread, &QThread::deleteLater);

            query->startQuery(mSession);
        }
    }

    void ServerUserSettingsManager::querySucceeded(QVariantHash queriedSettings)
    {
        if (isDirty())
        {
            ORIGIN_LOG_WARNING << "Loading new server-side user settings while local settings are dirty. Setting changes may be lost.";
        }

        // Stores the settings we see change
        QVariantHash changedSettings;

        {
            QMutexLocker lock(&mLock);

            // Look for changes;
            for(QVariantHash::ConstIterator it = queriedSettings.constBegin();
                it != queriedSettings.constEnd();
                it++)
            {
                if (it.value() != mSettings[it.key()])
                {
                    // Value changed!
                    changedSettings[it.key()] = it.value();
                }
            }

            mSettings = queriedSettings;
            mSettingsLoaded = true;
        }

        if (mReloadAfterUpdate)
        {
            reload();
            mReloadAfterUpdate = false;
        }

        emit loaded();

        for(QVariantHash::ConstIterator it = changedSettings.constBegin();
            it != changedSettings.constEnd();
            it++)
        {
            emit valueChanged(it.key(), it.value(), true);
        }
    }

    void ServerUserSettingsManager::queueServerUpdate() // MUST BE CALLED WITH mLock
    {
        if (!mServerUpdateQueued)
        {
            // Wait for 50ms for any other updates to come in
            QTimer::singleShot(50, this, SLOT(immediateServerUpdate()));
            mServerUpdateQueued = true;
        }
    }

    void ServerUserSettingsManager::immediateServerUpdate()
    {
        // Only send if we're online
        if (Connection::ConnectionStatesService::isUserOnline(mSession))
        {
            QMutexLocker lock(&mLock);

            ServerUserSettingsUpdate *update = 
                new ServerUserSettingsUpdate(mSession, mSettings, mDirtyKeys);

            ORIGIN_VERIFY_CONNECT(update, SIGNAL(finished()), update, SLOT(deleteLater()));
            ORIGIN_VERIFY_CONNECT(update, SIGNAL(success()), this, SLOT(serverUpdateSucceeded()));

            mServerUpdateQueued = false;
        }
    }
        
    void ServerUserSettingsManager::serverUpdateSucceeded()
    {
        QMutexLocker lock(&mLock);
    
        ServerUserSettingsUpdate *update = dynamic_cast<ServerUserSettingsUpdate*>(sender());
    
        const QVariantHash updateSettings(update->settings());
        const QSet<QString> updateDirtyKeys(update->dirtyKeys());
    
        // Find which keys we can un-dirty
        for(QSet<QString>::ConstIterator it = updateDirtyKeys.constBegin();
            it != updateDirtyKeys.constEnd();
            it++)
        {
            const QString &key(*it);
    
            // Make sure the value hasn't changed since the update was sent
            if (updateSettings[key] == mSettings[key])
            {
                mDirtyKeys -= key;
            }
        }
    }

#ifdef _DEBUG
    void ServerUserSettingsQuery::test(Session::SessionRef session)
    {
        /*
        OriginServiceResponse* resp = AtomServiceClient::setPrivacySetting(session, PrivacySettingCategoryInGame, "Test post ingame");
        ORIGIN_ASSERT(resp);
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onGcsSetPrivacySetting()));

        OriginServiceResponse* resp2 = AtomServiceClient::setPrivacySetting(session, PrivacySettingCategoryNotification, "Test post notification");
        ORIGIN_ASSERT(resp2);
        ORIGIN_VERIFY_CONNECT(resp2, SIGNAL(finished()), this, SLOT(onGcsSetPrivacySetting()));
        */
        QVector<QString> mySearchOptions;
        /*
        SearchOptionTypeXbox,
        SearchOptionTypePS3,
        SearchOptionTypeFaceBook,
        SearchOptionTypeFullName,
        SearchOptionTypeGamerName,
        SearchOptionTypeEmail,
        SearchOptionTypeNoOption
         **/
        for (int i = SearchOptionTypeXbox ; i < SearchOptionTypeNoOption; ++i)
        {
            mySearchOptions.push_back(SearchOptions().name(static_cast<searchOptionType>(i)));
        }
        SetSearchOptionsResponse* resp = AtomServiceClient::setSearchOptions(session, mySearchOptions);
        ORIGIN_ASSERT(resp);
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onRestFinished()));

    }

    void ServerUserSettingsQuery::onRestFinished()
    {
        SetSearchOptionsResponse* resp = static_cast<SetSearchOptionsResponse*>(sender());

        if(resp->error() == restErrorSuccess)
        {
            QStringList myValues = resp->values();
        }
        else
        {
        }

        resp->deleteLater();
    }
#endif

    void ServerUserSettingsQuery::startQuery(Origin::Services::Session::SessionRef session)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(Origin::Services::Session::SessionRef, session));

#ifdef _DEBUG
//#define _TESTATOM
#ifdef _TESTATOM
        test(session);
#endif // _TESTATOM
#endif
        // Send the privacy settings request
        PrivacyGetSettingResponse *resp = AtomServiceClient::privacySetting(session, ApiValues::PrivacySettingCategoryAll);

        // Give up if the query takes too long
        mResponseTimer.reset(new QTimer(this));
        mResponseTimer->setSingleShot(true);
        ORIGIN_VERIFY_CONNECT(mResponseTimer.data(), SIGNAL(timeout()), resp, SLOT(abort()));
        ORIGIN_VERIFY_CONNECT(mResponseTimer.data(), SIGNAL(timeout()), this, SLOT(onPrivacySettingsTimeoutError()));
        mResponseTimer->start(ServerQueryTimeoutMs);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onPrivacySettingsRequestSuccess()));
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)), this, SLOT(onPrivacySettingsRequestError(Origin::Services::restError)));
    }

    void ServerUserSettingsQuery::onPrivacySettingsTimeoutError()
    {
        ORIGIN_LOG_DEBUG << "[error] response timed out";
        emit error();
        emit finished();
    }
    
    void ServerUserSettingsQuery::onPrivacySettingsRequestError(Origin::Services::restError myError)
    {
    	mResponseTimer.clear();
        PrivacyGetSettingResponse *resp = dynamic_cast<PrivacyGetSettingResponse*>(sender());
        ORIGIN_ASSERT(resp);
        if (resp)
        {
            ORIGIN_LOG_DEBUG << "[error] error [" << myError << "] error string ["<< resp->errorString() << "] http status [" <<resp->httpStatus() << "]";
        }
        else
        {
            ORIGIN_LOG_DEBUG << "[error] error [" << myError << "] Response is NULL";
        }
        emit error();
        emit finished();
        resp->deleteLater();
    }
        
    void ServerUserSettingsQuery::importPayloadBoolean(const QHash<QString, bool> &payloadHash, const QString &key)
    {
        if (payloadHash.contains(key))
        {
            mSettings[key] = payloadHash[key];
        }
    }

    void ServerUserSettingsQuery::importPayloadVariant(const QHash<QString, QVariant>& payloadHash, const Setting& setting)
    {
        const QString& key = setting.name();
        
        if (payloadHash.contains(key))
        {
            QVariant value = payloadHash[key];

            if (value.canConvert(setting.type()))
            {
                value.convert(setting.type());
                mSettings[key] = value;
            }
        }
    }

    void ServerUserSettingsQuery::onPrivacySettingsRequestSuccess()
    {
        mResponseTimer.clear();

        // Get our raw privacy settings
        PrivacyGetSettingResponse *resp = dynamic_cast<PrivacyGetSettingResponse*>(sender());
        ORIGIN_ASSERT(resp);
        if (NULL==resp)
        {
            ORIGIN_LOG_DEBUG << "[error] response is NULL";
            return;
        }

        const QList<PrivacyGetSettingResponse::UserPrivacySetting> privacySettings = resp->privacySettings();

        // Check if we got the telemetry settings
        bool telemetrySettingsReceived = false;

        // Override our defaults with any settings we see
        for(QList<PrivacyGetSettingResponse::UserPrivacySetting>::ConstIterator it = privacySettings.constBegin();
            it != privacySettings.constEnd();
            it++)
        {
            if (it->category.compare(IgoCategory, Qt::CaseInsensitive) == 0)
            {
                QHash<QString, bool> inGameSettings = privacyStringToHash(it->payload);

                importPayloadBoolean(inGameSettings, EnableIgoForAllGamesKey);
            }
            else if (it->category.compare(GeneralCategory, Qt::CaseInsensitive) == 0)
            {
                QHash<QString, QVariant> generalSettings = privacyStringtoVariantHash(it->payload);
                // check if we received telemetry enabled flag
                if(generalSettings.contains("telemetry_enabled") == true)
                     telemetrySettingsReceived = true;

                importPayloadVariant(generalSettings, SETTING_EnableCloudSaves);
                importPayloadVariant(generalSettings, SETTING_ServerSideEnableTelemetry);
				importPayloadVariant(generalSettings, SETTING_LastDismissedOtHPromoOffer);
            }
            else if (it->category.compare(VideoBroadcastCategory, Qt::CaseInsensitive) == 0)
            {
                mSettings[BroadcastTokenKey] = QVariant(it->payload);
            }
            else if (it->category.compare(FavoriteProductIdsCategory, Qt::CaseInsensitive) == 0)
            {
                mSettings[FavoriteProductIdsKey] = it->payload.split(";", QString::SkipEmptyParts);
            }
            else if (it->category.compare(HiddenProductIdsCategory, Qt::CaseInsensitive) == 0)
            {
                // Parse to get the version
                QStringList hiddenGamesPayload = (it->payload).split("|");

                // Some checks for the data, make sure we have all the elements we think we should
                if (hiddenGamesPayload.size() == 2)
                {
                    // Get the version and compare it against what the client expects
                    QString version = hiddenGamesPayload.at(0);

                    if (version == HiddenProductIdsPayloadVersion)
                    {
                        ORIGIN_LOG_EVENT << "Received hidden games: " << (it->payload);
                        mSettings[HiddenProductIdsKey] = hiddenGamesPayload.at(1).split(";", QString::SkipEmptyParts);
                    }
                    else
                    {
                        ORIGIN_ASSERT(false);
                    }
                }
            }
        }
        
        // if we haven't received "telemetry enabled" flag, we assume this is a new user account
        if(telemetrySettingsReceived  == false)
        {
            QHash<QString, bool> telemetrySettings;
            telemetrySettings.insert("telemetry_enabled", true);
            importPayloadBoolean(telemetrySettings, EnableTelemetryKey); 
        }

        emit success(mSettings);
        emit finished();
        resp->deleteLater();
    }
        
    ServerUserSettingsUpdate::ServerUserSettingsUpdate(Session::SessionRef session, const QVariantHash &settings, const QSet<QString> &dirtyKeys) :
        mSettings(settings),
        mDirtyKeys(dirtyKeys),
        mPendingCategoryUpdates(0)
    {
        if (dirtyKeys.contains(EnableIgoForAllGamesKey))
        {
            QHash<QString, bool> privacyHash;
            exportPayloadBoolean(privacyHash, settings, EnableIgoForAllGamesKey);

            if (!privacyHash.isEmpty())
            {
                const QString payload(privacyHashToString(privacyHash));
                sendCategoryUpdate(session, PrivacySettingCategoryInGame, payload);
            }
        }

        if (dirtyKeys.contains(EnableCloudSavesKey) ||
            dirtyKeys.contains(EnableTelemetryKey) ||
			dirtyKeys.contains(OthOfferKey))
        {
            QHash<QString, QVariant> privacyHash;

            exportPayloadVariant(privacyHash, settings, EnableCloudSavesKey);
            exportPayloadVariant(privacyHash, settings, EnableTelemetryKey);
            exportPayloadVariant(privacyHash, settings, OthOfferKey);

            if (!privacyHash.isEmpty())
            {
                const QString payload(privacyHashToString(privacyHash));
                sendCategoryUpdate(session, PrivacySettingCategoryGeneral, payload);
            }
        }

        if (dirtyKeys.contains(BroadcastTokenKey))
        {
            const QString payload(settings[BroadcastTokenKey].toString());
            sendCategoryUpdate(session, PrivacySettingCategoryBroadcasting, payload);
        }

        if (dirtyKeys.contains(HiddenProductIdsKey))
        {
            QString payload(HiddenProductIdsPayloadVersion + "|");
            payload += settings[HiddenProductIdsKey].toStringList().join(";");

            ORIGIN_LOG_EVENT << "Updating hidden games to: " << payload;
            sendCategoryUpdate(session, PrivacySettingCategoryHiddenGames, payload);
        }

        if (dirtyKeys.contains(FavoriteProductIdsKey))
        {
            const QString payload(settings[FavoriteProductIdsKey].toStringList().join(";"));

            sendCategoryUpdate(session, PrivacySettingCategoryFavoriteGames, payload);
        }

        if (mPendingCategoryUpdates == 0)
        {
            // We had nothing to do!
            // Queue these signals so our caller has a chance to connect them
            QMetaObject::invokeMethod(this, "success", Qt::QueuedConnection);
            QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
        }
    }
        
    void ServerUserSettingsUpdate::sendCategoryUpdate(Session::SessionRef session, ApiValues::privacySettingCategory category, const QString &payload)
    {
        OriginServiceResponse *resp = AtomServiceClient::setPrivacySetting(session,category, payload);
        resp->setDeleteOnFinish(true);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onCategoryUpdateSuccess()));
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)), this, SLOT(onCategoryUpdateError()));

        // Keep count of the updates we have in-flight
        mPendingCategoryUpdates++;
    }
    
    void ServerUserSettingsUpdate::onCategoryUpdateSuccess()
    {
        if (mPendingCategoryUpdates == 0)
        {
            // Already finished
            return;
        }

        mPendingCategoryUpdates--;

        // Was that the last pending update?
        if (mPendingCategoryUpdates == 0)
        {
            // Success!
            emit success();
            emit finished();
        }
    }

    void ServerUserSettingsUpdate::onCategoryUpdateError()
    {
        if (mPendingCategoryUpdates == 0)
        {
            // Already finished
            return;
        }

        // Ignore any future results; we've permanently failed
        mPendingCategoryUpdates = 0;

        emit error();
        emit finished();
    }
}
}
}
