#ifndef ENTITLEMENT_CACHE_H
#define ENTITLEMENT_CACHE_H

#include <QString>
#include <QVector>
#include <QByteArray>
#include <QReadWriteLock>
#include <QHash>

#include "services/session/AbstractSession.h"

using namespace Origin::Services;

namespace Origin
{

namespace Engine
{

namespace Content
{

enum CachedResponseType
{
    ResponseTypeBaseGames,
    ResponseTypeExtraContent,
    ResponseTypeContentUpdates,
    ResponseTypeOfferSpecific,
    ResponseTypeNucleusEntitlements,
    ResponseTypeSdkEntitlements
};

/// Simple struct to hold entitlement XML data and corresponding signature
struct CachedResponseData
{
    CachedResponseType m_responseType;
    QByteArray m_responseData;
    QByteArray m_responseSignature;  
    QString m_responseCustomData;
    CachedResponseData()
    {}
    CachedResponseData( CachedResponseType responseType, QByteArray const& responseData, QByteArray const& signature, QString const& customData = QString())
        : m_responseType(responseType)
        , m_responseData(responseData)
        , m_responseSignature(signature)
        , m_responseCustomData(customData)
    {}
};

///
/// Cache entitlement data, with support for loading additional entitlement data from 
/// XML data in disk for development support. Entitlement data from server is written
/// to disk immediately on insert, and read from disk in slot connected to the session
/// begin signal, i.e. on user logging in. Development support data is read from disk
/// on MyGames reload.
///
class EntitlementCache : public QObject
{
    Q_OBJECT
public:
    ///
    /// Create the instance
    ///
    static void init();

    ///
    /// Delete the instance
    ///
    static void release();

    ///
    /// Return the instance
    ///
    static EntitlementCache* instance();

    ///
    /// Add the given data to the entitlement cache stored within the given session, encrypt
    /// the data using the given entropy as source for the key.
    ///
    bool insert(Session::SessionRef session, QString const& entropy, CachedResponseData const& data );

    ///
    /// Remove the cache data from the given session, and delete the cache files on disk.
    /// This only removes base games and content updates, it does not impact extra content.
    ///
    void clear(Session::SessionRef session);

    ///
    /// Remove the cache data from the given session, and delete the cache files on disk.
    /// Empty masterTitleId sets means clear all cache data.
    ///
    void clearExtraContent(Session::SessionRef session, const QString& masterTitleId = "");

    ///
    /// Return currently cached data, it is returned exactly as the bytes received 
    /// from the servers.
    ///
    QVector<Content::CachedResponseData>* cacheData(Session::SessionRef session);
        
    ///
    /// Return currently cached data, it is returned exactly as the bytes received 
    /// from the servers.
    ///
    QHash<QString, Content::CachedResponseData>* extraContentCacheData(Session::SessionRef session);
            
    ///
    /// Return currently cached data, it is returned exactly as the bytes received 
    /// from the servers.
    ///
    QHash<QString, Content::CachedResponseData>* sdkEntitlementsCacheData(Session::SessionRef session);

public slots:
    ///
    /// Load the cache files from disk.
    ///
    void onBeginSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef);

private:

    EntitlementCache();
    virtual ~EntitlementCache();

    /// Key used to store and retrieve the data in the session.
    static const QString SERVICE_DATA_KEY;

    /// This struct holds the actual cache data, it lives in the session using the 
    /// addServiceData() functionality under the key above.
    struct EntitlementCacheServiceData : public Session::AbstractSession::ServiceData
    {
        ~EntitlementCacheServiceData()
        {
            m_offlineCacheData.clear();
        }
        QVector<CachedResponseData> m_offlineCacheData;
        QHash<QString, CachedResponseData> m_offlineExtraContentCacheData;
        QHash<QString, CachedResponseData> m_offlineSdkEntitlementsCacheData;
    };

    static bool readCacheFile(CachedResponseData& contents, QString const& path, Session::SessionRef session);
    static bool writeCacheFile(CachedResponseData const& contents, QString const& path, Session::SessionRef session);

    QString m_cacheFileTemplate;
    QString m_extraContentCacheFileTemplate;
    QString m_sdkEntitlementsCacheFileTemplate;

    QReadWriteLock m_cacheLock;
};

}

}

}

#endif // ENTITLEMENT_CACHE_H
