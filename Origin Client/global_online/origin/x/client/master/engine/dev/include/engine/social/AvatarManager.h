#ifndef _AVATARMANAGER_H
#define _AVATARMANAGER_H

#include <QSet>
#include <QHash>
#include <QUrl>
#include <QPixmap>
#include <QObject>
#include <QMutex>

#include "services/rest/AvatarServiceClient.h"
#include "services/session/AbstractSession.h"
#include "services/plugin/PluginAPI.h"

QString avatarImageDirectory();

namespace Origin
{
namespace Engine
{
namespace Social
{
    class AvatarManager;

    /// \brief Binary representation of an avatar
    class ORIGIN_PLUGIN_API AvatarBinary
    {
        friend class AvatarManager;
    public:
        /// \brief Constructs a null AvatarBinary
        AvatarBinary() : mNull(true)
        {
        }

        /// \brief Returns true if this is a null object
        bool isNull() const
        {
            return mNull;
        }

        ///
        /// \brief Media type of the avatar
        ///
        /// This should be either image/jpeg or image/png
        ///
        QByteArray mediaType() const
        {
            return mMediaType;
        }

        ///
        /// \brief Returns the raw data of the avatar
        ///
        /// This will be encoded in the media type returned by mediaType()
        ///
        QByteArray data() const
        {
            return mData;
        }

        ///
        /// \brief Returns the avatar as a data: URI
        ///
        /// This will return a null QByteArray if callled on a null object
        ///
        QByteArray toDataUri() const;

    private:
        AvatarBinary(const QByteArray &mediaType, const QByteArray &data) :
            mNull(false),
            mMediaType(mediaType),
            mData(data)
        {
        }

        bool mNull;
        QByteArray mMediaType;
        QByteArray mData;
    };

    ///
    /// \brief Class for querying and retrieving user avatars
    ///
    /// Only the functions explicitly documented as thread-safe can be called outside the main thread
    ///
    class ORIGIN_PLUGIN_API AvatarManager : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// \brief Creates a new AvatarManager for the given session and avatar size
        ///
        /// Multiple AvatarManagers may exist on the same connection but they can only manage a single size at once
        ///
        /// \param session     Session to use for making service calls
        /// \param avatarSize  Size of the avatars to request. This defaults to 40x40. 
        /// 
        AvatarManager(Services::Session::SessionRef session, Services::AvatarServiceClient::ImageSize avatarSize = Services::AvatarServiceClient::Size_40X40);
        ~AvatarManager();

        /// \brief Subscribes to a Nucleus ID's avatar
        Q_INVOKABLE void subscribe(quint64);

        /// \brief Unsubscribes from a Nucleus ID's avatar
        Q_INVOKABLE void unsubscribe(quint64);

        ///
        /// \brief Refresh a Nucleus ID's avatar
        ///
        /// This will re-request the user's avatar URL from the avatar service
        ///
        Q_INVOKABLE void refresh(quint64);
        
        ///
        /// \brief Returns the cached raw data for a given NucleuS ID
        ///
        /// \return QByteArray or null if none has been loaded
        ///
        AvatarBinary binaryForNucleusId(quint64 nucleusId) const;

        ///
        /// \brief Returns the filesystem path to the avatar for a given Nucleus ID
        ///
        /// This function is thread-safe
        ///
        /// \return Filesystem path using Qt separators. This may be null or the path may not exist.
        ///
        QString pathForNucleusId(quint64 nucleusId) const;
        
        ///
        /// \brief Returns the avatar pixmap for a given Nucleus ID
        ///
        /// \return QPixmap or null if none has been loaded
        ///
        QPixmap pixmapForNucleusId(quint64 nucleusId) const;

        /// \brief Externally load the avatar 
        void loadAvatar(quint64 userId, const QUrl & url, QByteArray mediaType, QString path);

    signals:
        ///
        /// \brief Emitted when any avatar is discovered or changed
        ///
        void avatarChanged(quint64);

    private slots:
        void sendAvatarUrlRequest();

        void avatarUrlRequestSucceeded();
        void avatarUrlRequestFailed();

        void avatarPixmapRequestFinished();

    private:
        QString filenameForAvatarUrl(const QUrl &url, const QByteArray &mediaType) const;
        void sendAvatarChanged(const qint64& nucleusId);

        Services::Session::SessionRef mSession;
        Services::AvatarServiceClient::ImageSize mAvatarSize;

        // Avatar URLs we need
        QSet<quint64> mNeededAvatarUrls;
        // Avatar URL requests in flight. This is a subset of mNeedAvatarUrls
        QSet<quint64> mInFlightAvatarUrlRequests;

        // This is a map from Nucleus IDs to avatar URLs
        typedef QHash<quint64, QUrl> AvatarUrlHash;
        AvatarUrlHash mAvatarUrlHash;
        mutable QMutex mAvatarUrlHashLock;

        // This is a map from avatar URLs to the pixmap
        typedef QHash<QUrl, AvatarBinary> AvatarBinaryHash;
        AvatarBinaryHash mAvatarBinaryHash;

        // Avatar pixmap requests in flight
        QSet<QUrl> mInFlightAvatarPixmapRequests;

        // Indicates if there's an avatar URL request queued
        bool mAvatarUrlRequestQueued;

        // Static map of media type to filename extension because C++ sucks
        QMap<QByteArray, QString> mMediaTypeToExtension;
    };
}
}
}

#endif
