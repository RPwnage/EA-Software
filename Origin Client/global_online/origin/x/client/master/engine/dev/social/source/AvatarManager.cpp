#include "engine/social/AvatarManager.h"
#include "engine/login/LoginController.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QMutexLocker>

#ifdef ORIGIN_PC
#include <Windows.h>
#include <ShlObj.h>
#endif

#include "services/rest/AvatarServiceClient.h"
#include "services/network/NetworkAccessManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/settings/SettingsManager.h"

namespace
{

    bool isLogging =
#ifdef _DEBUG
        true;
#else
        false;
#endif // _DEBUG

    const int AVATAR_REQUEST_LIMIT = 100;
}

QString avatarImageDirectory()
{
    static QString path;

    if (path.isNull())
    {
#ifdef ORIGIN_PC
        // Get the path to roaming app data
        wchar_t appDataPath[MAX_PATH + 1];
        if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) == S_OK)
        {
            path = QDir::fromNativeSeparators(QString::fromUtf16(appDataPath)) + "/Origin/AvatarsCache";
        }
        else
        {
            ORIGIN_ASSERT(0);
        }
#else
        path =  Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation) + "/AvatarsCache";
#endif
    }
        
    return path;
};


namespace Origin
{
namespace Engine
{
namespace Social
{
    using Services::Connection::ConnectionStatesService;

    AvatarManager::AvatarManager(Services::Session::SessionRef session, Services::AvatarServiceClient::ImageSize avatarSize) :
        mSession(session),
        mAvatarSize(avatarSize),
        mAvatarUrlRequestQueued(false)
    {
        mMediaTypeToExtension["image/jpeg"] = "jpg";
        mMediaTypeToExtension["image/png"] = "png";

        // Create our cache directory
        QDir("/").mkpath(avatarImageDirectory());
        
        // Kick our state machine when we come back online 
        ORIGIN_VERIFY_CONNECT(ConnectionStatesService::instance(), SIGNAL(nowOnlineUser()),
                this, SLOT(sendAvatarUrlRequest()));
    }

    AvatarManager::~AvatarManager()
    {
    }
        
    QString AvatarManager::filenameForAvatarUrl(const QUrl &url, const QByteArray &mediaType) const
    {
        const QString extension = mMediaTypeToExtension.value(mediaType);

        if (extension.isEmpty())
        {
            return QString();
        }

        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(url.toEncoded());

        return avatarImageDirectory() + "/" + hash.result().toHex() + "." + extension;
    }

    void AvatarManager::sendAvatarChanged(const qint64& nucleusId)
    {
        Engine::UserRef user = Engine::LoginController::currentUser();
        // EBIBUGS-25680: We only want to cache the avatar for 40x40 (offline for social user area)
        if(mAvatarSize == Services::AvatarServiceClient::Size_40X40 && user.isNull() == false && nucleusId == user->userId())
        {
            Services::writeSetting(Origin::Services::SETTING_USERAVATARCACHEURL, pathForNucleusId(nucleusId), Services::Session::SessionService::currentSession());
        }
        emit avatarChanged(nucleusId);
    }
    
    void AvatarManager::refresh(quint64 nucleusId)
    {
        unsubscribe(nucleusId);
        subscribe(nucleusId);
    }

    void AvatarManager::subscribe(quint64 nucleusId)
    {
        ORIGIN_ASSERT(nucleusId);

        mNeededAvatarUrls << nucleusId;

        if (!mAvatarUrlRequestQueued)
        {    
            // Queue an avatar URL request
            // This is so we can batch all the requests at once at the end of our event loop
            QMetaObject::invokeMethod(this, "sendAvatarUrlRequest", Qt::QueuedConnection); 
            mAvatarUrlRequestQueued = true;
        }
    }

    void AvatarManager::unsubscribe(quint64 nucleusId)
    {
        QMutexLocker locker(&mAvatarUrlHashLock);

        // Forget about the Nucleus ID
        mNeededAvatarUrls.remove(nucleusId);
        mAvatarUrlHash.remove(nucleusId);
    }

    AvatarBinary AvatarManager::binaryForNucleusId(quint64 nucleusId) const
    {
        QMutexLocker locker(&mAvatarUrlHashLock);
        const QUrl avatarUrl(mAvatarUrlHash.value(nucleusId));

        if (!avatarUrl.isValid())
        {
            // No cached URL
            return AvatarBinary();
        }

        return mAvatarBinaryHash.value(avatarUrl);
    }

    QPixmap AvatarManager::pixmapForNucleusId(quint64 nucleusId) const
    {
        // Don't need a lock because binaryForNucleusId takes one itself
        const AvatarBinary binary = binaryForNucleusId(nucleusId);

        if (binary.isNull())
        {
            // No image
            return QPixmap();
        }

        QPixmap ret;
        const QByteArray mediaType(binary.mediaType());

        if (mediaType == "image/jpeg")
        {
            ret.loadFromData(binary.data(), "JPEG");
        }
        else if (mediaType == "image/png")
        {
            ret.loadFromData(binary.data(), "PNG");
        }
        else if (mediaType == "image/gif")
        {
            ret.loadFromData(binary.data(), "GIF");
        }
        else
        {
            ORIGIN_LOG_WARNING << "Can't create QPixmap from unknown media type " << mediaType;
        }

        return ret;
    }
        
    QString AvatarManager::pathForNucleusId(quint64 nucleusId) const
    {
        // Get the avatar URL which determines our base filename
        mAvatarUrlHashLock.lock();
        const QUrl avatarUrl(mAvatarUrlHash.value(nucleusId));
        mAvatarUrlHashLock.unlock();

        if (!avatarUrl.isValid())
        {
            return QString();
        }

        // Get the media type which determines our extension
        AvatarBinary binary = binaryForNucleusId(nucleusId);

        if (binary.isNull())
        {
            return QString();
        }

        return filenameForAvatarUrl(avatarUrl, binary.mediaType());
    }
        
    void AvatarManager::sendAvatarUrlRequest()
    {
        QSet<quint64> toQuery = mNeededAvatarUrls - mInFlightAvatarUrlRequests;

        // We're no longer queued to be executed
        mAvatarUrlRequestQueued = false;

        if (toQuery.isEmpty() || !ConnectionStatesService::isUserOnline(mSession))
        {
            // Nothing to do or offline
            return;
        }

        while(!toQuery.isEmpty())
        {
            // If there are more nucleus IDs than the server allows per request, send the requests in batches.
            QSet<quint64> batch = toQuery.values().mid(0, AVATAR_REQUEST_LIMIT).toSet();

            Services::AvatarsByUserIdsResponse *resp = Services::AvatarServiceClient::avatarsByUserIds(mSession, batch.toList(), mAvatarSize);

            if (resp == NULL)
            {
                // This can happen if we send bad Nucleus IDs
                ORIGIN_ASSERT(0);
                ORIGIN_LOG_WARNING << "Unable to start avatar query";
                return;
            }

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(avatarUrlRequestSucceeded()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(avatarUrlRequestFailed()));

            // These are now in flight
            mInFlightAvatarUrlRequests += batch;

            // Remove the batch requested IDs from our full set
            toQuery -= batch;
        }
    }
    
    void AvatarManager::avatarUrlRequestSucceeded()
    {
        Services::AvatarsByUserIdsResponse *resp = dynamic_cast<Services::AvatarsByUserIdsResponse*>(sender());
        resp->deleteLater();

        // Make a copy of our avatar information
        const QHash<quint64, Services::UserAvatarsInfo> avatarInfoHash(resp->constAvatarInfo());

        // These are no longer in flight or needed
        mInFlightAvatarUrlRequests -= resp->requestedUserIds().toSet();
        mNeededAvatarUrls -= avatarInfoHash.keys().toSet();

        QHashIterator<quint64, Services::UserAvatarsInfo> it(avatarInfoHash);

        while(it.hasNext())
        {
            it.next();

            quint64 nucleusId(it.key());

            QUrl url(it.value().info.link);

            // Save this to the hash
            {
                QMutexLocker locker(&mAvatarUrlHashLock);
                mAvatarUrlHash.insert(nucleusId, url);
            }

            QString path;
            QByteArray mediaType;
            bool exists = false;

            for(QMap<QByteArray, QString>::ConstIterator it = mMediaTypeToExtension.constBegin();
                it != mMediaTypeToExtension.constEnd();
                it++)
            {
                mediaType = it.key();
                path = filenameForAvatarUrl(url, mediaType);

                if(QFile::exists(path))
                {
                    exists = true;
                    break;
                }
            }

                // Make sure we have it in memory and on disk.
            if (mAvatarBinaryHash.contains(url) && exists)
            {
                ORIGIN_LOG_EVENT_IF(isLogging) << "[-_-_-_] Avatar binary hash contains " << url.toString();
                sendAvatarChanged(nucleusId);
            }
            else
            {
                // File exists on disk.
                if(exists)
                {
                    ORIGIN_LOG_EVENT_IF(isLogging) << "[####] Avatar URL " << url.toString() << " fileName " << path;

                    QFile cacheFile(path);
                    if (cacheFile.open(QIODevice::ReadOnly))
                    {
                        // We have the data on-disk - read in to memory
                        AvatarBinary binary(mediaType, cacheFile.readAll());
                        cacheFile.close();

                        mAvatarBinaryHash.insert(url, binary);
                        sendAvatarChanged(nucleusId);
                    }
                }
                
                if(!exists && !mInFlightAvatarPixmapRequests.contains(url))
                {
                    QNetworkAccessManager *manager = Services::NetworkAccessManager::threadDefaultInstance();

                    // Make the request
                    QNetworkRequest req(url);
                    req.setRawHeader("Accept", "image/jpeg, image/png");

                    // We do our own caching; don't pollute the Origin web cache
                    req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);

                    ORIGIN_LOG_EVENT_IF(isLogging) << "[####] not found cached avatar and is not yet being requested, requesting URL " << req.url().toString();

                    QNetworkReply *reply = manager->get(req);

                    ORIGIN_VERIFY_CONNECT(reply, SIGNAL(finished()), this, SLOT(avatarPixmapRequestFinished()));

                    // This request is already in flight
                    mInFlightAvatarPixmapRequests << url;
                }
            }
        }
    }
    
    void AvatarManager::avatarUrlRequestFailed()
    {
        ORIGIN_LOG_WARNING << "Avatar URL request failed";

        Services::AvatarsByUserIdsResponse *resp = dynamic_cast<Services::AvatarsByUserIdsResponse*>(sender());
        resp->deleteLater();

        // These are no long in flight
        mInFlightAvatarUrlRequests -= resp->requestedUserIds().toSet();
    }
        
    void AvatarManager::avatarPixmapRequestFinished()
    {
        QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());
        reply->deleteLater();

        // Use the request URL in case we were redirected
        // All of our tracking is done on the original request URL
        QUrl url = reply->request().url();

        // This is no longer in flight
        mInFlightAvatarPixmapRequests -= url;

        if (reply->error() != QNetworkReply::NoError)
        {
            // Don't bother retrying
            ORIGIN_LOG_WARNING << "Avatar pixmap request failed for URL " << url.toString();
            return;
        }
        
        const QByteArray mediaType(reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
        const QByteArray avatarData(reply->readAll());

        // Store the avatar data in-memory
        mAvatarBinaryHash.insert(url, AvatarBinary(mediaType, avatarData));

        // And on-disk
        const QString filename = filenameForAvatarUrl(url, mediaType);
        ORIGIN_LOG_EVENT_IF(isLogging) << "[myinfo] Avatar pixmap request successful (inserted)";
        ORIGIN_LOG_EVENT_IF(isLogging) << "[myinfo] URL: " << url.toString();
        ORIGIN_LOG_EVENT_IF(isLogging) << "[myinfo] Media type :" << mediaType;
        ORIGIN_LOG_EVENT_IF(isLogging) << "[myinfo] Avatar Data: "<< avatarData;
        ORIGIN_LOG_EVENT_IF(isLogging) << "[myinfo] File Name :"  << filename;

        if (!filename.isNull())
        {
            QFile cacheFile(filename);

            if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                cacheFile.write(avatarData);
                cacheFile.close();
            }
        }
        else
        {
            ORIGIN_LOG_WARNING << "[warning] Unable to determine cache file path for media type " << mediaType;
        }
        
        // Now find who needed this image
        mAvatarUrlHashLock.lock();
        QList<quint64> nucleusIds = mAvatarUrlHash.keys(url);
        mAvatarUrlHashLock.unlock();

        for(QList<quint64>::ConstIterator it = nucleusIds.constBegin();
            it != nucleusIds.constEnd();
            it++)
        {
            sendAvatarChanged(*it);
        }

    }

    void AvatarManager::loadAvatar(quint64 userId, const QUrl & url, QByteArray mediaType, QString path)
    {
        QMutexLocker locker(&mAvatarUrlHashLock);
        
        mAvatarUrlHash[userId] = url;

        QFile file(path);

        if(file.open(QIODevice::ReadOnly))
        {
            mAvatarBinaryHash.insert(url, AvatarBinary(mediaType, file.readAll()));
            file.close();
        }
    }


    QByteArray AvatarBinary::toDataUri() const
    {
        if (isNull())
        {
            return QByteArray();
        }

        return "data:" + mediaType() + ";base64," + data().toBase64();
    }
}
}
}
