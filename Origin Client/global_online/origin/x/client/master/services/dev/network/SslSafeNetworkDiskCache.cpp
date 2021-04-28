#include "SslSafeNetworkDiskCache.h"

SslSafeNetworkDiskCache::SslSafeNetworkDiskCache(QObject *parent) : QNetworkDiskCache(parent)
{
}

QIODevice* SslSafeNetworkDiskCache::prepare(const QNetworkCacheMetaData &metaData) 
{
    if ((metaData.url().scheme().toLower() != "https") || !metaData.saveToDisk())
    {
        // Behave the same for non-HTTPS requests or requests with caching already disabled
        return QNetworkDiskCache::prepare(metaData);
    }

    // Assume we have no caching related headers
    // Lots of web service responses have no caching related headers and contain sensitive information
    // There's almost zero use saving them to disk as the only way to access them is through
    // QNetworkRequest::CacheLoadControlAttribute
    bool hasCacheHeader = false;

    const QNetworkCacheMetaData::RawHeaderList headers = metaData.rawHeaders();

    for(auto it = headers.constBegin(); it != headers.constEnd(); it++)
    {
        const QByteArray lowercaseHeader = it->first.toLower();

        if ((lowercaseHeader == "etag") ||
            (lowercaseHeader == "last-modified") ||
            (lowercaseHeader == "cache-control"))
        {
            hasCacheHeader = true;
            break;
        }
    }

    // Update our metadata
    QNetworkCacheMetaData revisedMetaData(metaData);
    revisedMetaData.setSaveToDisk(hasCacheHeader);

    return QNetworkDiskCache::prepare(revisedMetaData);
}
