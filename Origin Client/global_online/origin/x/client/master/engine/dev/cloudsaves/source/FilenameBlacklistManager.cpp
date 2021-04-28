#include <QTimer>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QRegExp>
#include <QDataStream>
#include <QCryptographicHash>

#include "services/network/NetworkAccessManager.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/log/LogService.h"

#include "engine/cloudsaves/FilenameBlacklistManager.h"
#include "engine/cloudsaves/FilesystemSupport.h"


namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    namespace
    {
        // Update once every 24 hours
        const unsigned int NetworkCheckIntervalMs = 24 * 60 * 60 * 1000;

        // The Qt data stream format we use
        const int BlacklistDataStreamVersion = QDataStream::Qt_4_7;
        // Used to sign our blacklist to prevent tampering on disk. This also 
        // functions as our magic number
        const unsigned char BlacklistSecret[8] = {0xaa, 0x4f, 0xd4, 0xab, 0xf6, 0x33, 0xfc, 0xbb};

        bool networkAvailable()
        {
            return Origin::Services::Connection::ConnectionStatesService::isGlobalStateOnline();
        }

        QString blacklistSavePath()
        {
            using namespace FilesystemSupport;
            return cloudSavesDirectory(RoamingRoot).absoluteFilePath("blacklist");
        }

        QUrl blacklistUrl()
        {
            QString blacklistUrl(Services::readSetting(Services::SETTING_CloudSavesBlacklistURL));
            return QUrl(blacklistUrl);
        }

        QSet<QString> parseBlacklistText(const QString &blacklistString)
        {
            // Handle Unix or Windows newlines
            return blacklistString.split(QRegExp("\\r?\\n"), QString::SkipEmptyParts).toSet();
        }

        QByteArray signBlacklistContent(QByteArray content)
        {
            QCryptographicHash signHash(QCryptographicHash::Sha1);
    
            signHash.addData(reinterpret_cast<const char*>(BlacklistSecret), sizeof(BlacklistSecret));
            // Make sure we don't use a blacklist from a differnt URL than the
            // one we're set to now
            signHash.addData(blacklistUrl().toString().toLatin1());
            signHash.addData(content);

            return signHash.result();
        }

        FilenameBlacklistManager *GlobalInstance;
    }

    FilenameBlacklistManager* FilenameBlacklistManager::instance()
    {
        static QMutex globalInstanceLock;

        if (!GlobalInstance)
        {
            QMutexLocker locker(&globalInstanceLock);
            // Make sure we still haven't build now that we have the lock
            if (!GlobalInstance)
            {
                GlobalInstance = new FilenameBlacklistManager;
            }
        }

        return GlobalInstance;
    };

    FilenameBlacklistManager::FilenameBlacklistManager()
        : mMissedNetworkCheck(false)
    {
        // Load our builtin blacklist from our resources
        loadBuiltinBlacklist();
        // Load any saved network blacklist we might have
        loadSavedBlacklist();
    }

    void FilenameBlacklistManager::startNetworkUpdate()
    {
        // Start our interval check
        QTimer *checkTimer = new QTimer(this);
        checkTimer->setInterval(NetworkCheckIntervalMs);

        ORIGIN_VERIFY_CONNECT(checkTimer, SIGNAL(timeout()), this, SLOT(checkNetworkBlacklist()));

        checkTimer->start();
        
        ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(globalConnectionStateChanged(bool)), 
            this, SLOT(onlineStateChanged()));

        // Do an initial check
        checkNetworkBlacklist();
    }
        
    FilenameBlacklist FilenameBlacklistManager::currentBlacklist() const
    {
        QMutexLocker locker(&mBlacklistMutex);
        return (mBuiltinBlacklist + mRemoteBlacklist).toList();
    }

    void FilenameBlacklistManager::onlineStateChanged()
    {
        if (networkAvailable() && mMissedNetworkCheck)
        {
            checkNetworkBlacklist();
        }
    }

    void FilenameBlacklistManager::checkNetworkBlacklist()
    {
        if (!networkAvailable())
        {
            // Check once we come back online
            mMissedNetworkCheck = true;
            return;
        }

        // Assume we're going to succeed in case we get any signals while this 
        // transaction is in progress. If we fail this will be set back to true
        mMissedNetworkCheck = false;

        QNetworkRequest req(blacklistUrl());
        
        // We can only handle UTF-8 text files
        req.setRawHeader("Accept", "text/plain");
        req.setRawHeader("Accept-Charset", "utf-8");

        // Always go to the network as we manage caching internally
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QVariant(QNetworkRequest::AlwaysNetwork));
        req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, QVariant(false));

        // Give us a 304 if this is the blacklist we're excepting
        if (!mRemoteBlacklistEtag.isNull())
        {
            req.setRawHeader("If-None-Match", mRemoteBlacklistEtag);
        }

        QNetworkReply *reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->get(req);
        ORIGIN_VERIFY_CONNECT(reply, SIGNAL(finished()), this, SLOT(networkCheckFinished()));
    }

    void FilenameBlacklistManager::networkCheckFinished()
    {
        QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());
        ORIGIN_ASSERT(reply);
        reply->deleteLater();

        QNetworkReply::NetworkError error = reply->error();
        if ((error == QNetworkReply::ContentNotFoundError) ||
            (error == QNetworkReply::HostNotFoundError))
        {
            // It doesn't exist; use whatever we have
            return;
        }
        else if (error == QNetworkReply::NoError)
        {
            // 200 or 304
            unsigned int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
            if (statusCode == 304)
            {
                // File is unmodified; nothing to do
                return;
            }

            QString blacklistString = QString::fromUtf8(reply->readAll()).trimmed();

            {
                QMutexLocker locker(&mBlacklistMutex);

                // Handle Unix or Windows newlines
                mRemoteBlacklist = parseBlacklistText(blacklistString);
    
                if (reply->hasRawHeader("ETag"))
                {
                    mRemoteBlacklistEtag = reply->rawHeader("ETag");
                }
                else
                {
                    mRemoteBlacklistEtag = QByteArray();
                }

                // Save to disk as our new authoritative blacklist
                saveBlacklist();
            }
        }
        else
        {
            // Some unknown condition happened; check again
            mMissedNetworkCheck = true;
        }
    }

    bool FilenameBlacklistManager::saveBlacklist()
    {
        // Expects to be called with mBlacklistMutex held
        QFile blacklistFile(blacklistSavePath());

        if (!blacklistFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            // Doesn't really matter
            return true;
        }

        QByteArray saveData;
        QDataStream saveStream(&saveData, QIODevice::WriteOnly);

        saveStream.setVersion(BlacklistDataStreamVersion);

        saveStream << mRemoteBlacklistEtag;
        saveStream << mRemoteBlacklist;
        
        // Write the signature
        blacklistFile.write(signBlacklistContent(saveData));
        // Write the data
        blacklistFile.write(qCompress(saveData));

        return true;
    }

    bool FilenameBlacklistManager::loadSavedBlacklist()
    {
        QFile blacklistFile(blacklistSavePath());

        if (!blacklistFile.open(QIODevice::ReadOnly))
        {
            // No blacklist
            return false;
        }

        // Load our signature
        QByteArray signature = blacklistFile.read(20);

        // Load our compressed blacklist
        QByteArray loadData = qUncompress(blacklistFile.readAll());

        // Make sure the signature matches
        if (signBlacklistContent(loadData) != signature)
        {
            // Bad signature
            return false;
        }
        
        QDataStream loadStream(loadData);

        // Load in to temporaries in case this doens't work
        QByteArray fileEtag;
        QSet<QString> fileBlacklist;

        loadStream >> fileEtag;
        loadStream >> fileBlacklist;

        if (loadStream.status() != QDataStream::Ok)
        {
            // Bogus
            return false;
        }

        // Copy over the values
        {
            QMutexLocker locker(&mBlacklistMutex);
            mRemoteBlacklistEtag = fileEtag;
            mRemoteBlacklist = fileBlacklist;
        }

        return true;
    }

    void FilenameBlacklistManager::loadBuiltinBlacklist()
    {
        QFile builtinBlacklistFile(":/Resources/default-cloud-save-blacklist");

        if (!builtinBlacklistFile.open(QIODevice::ReadOnly))
        {
            // This file should always exist
            ORIGIN_ASSERT(0);
            return;
        }

        {
            QMutexLocker locker(&mBlacklistMutex);
            mBuiltinBlacklist = parseBlacklistText(builtinBlacklistFile.readAll());
        }
    }    
}
}
}
