//    Copyright (c) 2014, Electronic Arts
//    All rights reserved.
//    Author: Jonathan Kolb

#ifndef USER_SPECIFIC_CONTENT_CACHE_H
#define USER_SPECIFIC_CONTENT_CACHE_H

#include "services/crypto/CryptoService.h"
#include "services/session/SessionService.h"

namespace Origin
{

namespace Engine
{

namespace Content
{

/// \class UserSpecificContentCache
/// \brief TBD.
class UserSpecificContentCache : public QObject
{
    Q_OBJECT

public:
    UserSpecificContentCache();
    virtual ~UserSpecificContentCache();

    Q_INVOKABLE void writeUserCacheData();
    Q_INVOKABLE void readUserCacheData();

protected:
    void setCacheDataVersion(quint32 dataVersion) { m_cacheDataVersion = dataVersion; }
    void setCacheFolder(const QString &cacheFolder) { m_cacheFolder = cacheFolder; }
    void setCacheFileExtension(const QString &ext) { m_cacheFileExtension = ext; }
    void setCacheCipher(Services::CryptoService::Cipher cipher) { m_cacheCipher = cipher; }
    void setCacheCipherMode(Services::CryptoService::CipherMode cipherMode) { m_cacheCipherMode = cipherMode; }
    void setCacheEntropy(const QByteArray &entropy) { m_cacheEntropy = entropy; }

    virtual void serializeCacheData(QDataStream &stream) = 0;
    virtual void deserializeCacheData(QDataStream &stream) = 0;

private:
    QString getCacheFilePath(Services::Session::SessionRef session) const;
    QByteArray getCryptKey(Services::Session::SessionRef session) const;

private:
    QString m_cacheFolder;
    QString m_cacheFileExtension;

    quint32 m_cacheDataVersion;
    Services::CryptoService::Cipher m_cacheCipher;
    Services::CryptoService::CipherMode m_cacheCipherMode;
    QByteArray m_cacheEntropy;
};

} // namespace Content

} // namespace Engine

} // namespace Origin

#endif // USER_SPECIFIC_CONTENT_CACHE_H
