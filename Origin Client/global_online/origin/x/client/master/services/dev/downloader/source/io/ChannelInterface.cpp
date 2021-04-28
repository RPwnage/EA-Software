//
// Copyright 2008, Electronic Arts Inc
//
#include <algorithm>

#include "io/ChannelInterface.h"
#include "io/HttpChannel.h"
#include "io/FileChannel.h"

#include <QObject>

namespace Origin
{
namespace Downloader
{

ChannelInterface::ChannelInterface()
{
    mErrorCode = 0;
    mErrorType = DownloadError::OK;
}

ChannelInterface::~ChannelInterface()
{
}

ChannelInterface* ChannelInterface::Create(const QString& urlString, Services::Session::SessionWRef session)
{
    QUrl url(ChannelInterface::CleanupUrl(urlString));
    
    ChannelInterface* newChannel = NULL;

    if(session && url.isValid())
    {
        if (url.scheme().toLower().startsWith("http")) // HTTP and HTTPS
        {
            newChannel = new HttpChannel(session);
        }
        else if (url.scheme().toLower().startsWith("file")) // File URL
        {
            newChannel = new FileChannel();
        }

        if(newChannel != NULL)
        {
            newChannel->SetURL(url.toString());
        }
    }

    return newChannel;
}

bool ChannelInterface::IsNetworkUrl(const QString& urlString)
{
    return(urlString.toLower().startsWith("http"));
}

QString ChannelInterface::CleanupUrl(const QString& urlString)
{
    QUrl returnUrl(urlString);
    if(returnUrl.isValid())
    {
        return urlString;
    }
    else // let's check for common problems and fix them
    {
        QString fixedUrlString = urlString;

        QRegExp rx("^file://[^/]");
        rx.setCaseSensitivity(Qt::CaseInsensitive);

        int pos = rx.indexIn(urlString);

        if(pos == 0)
        {
            fixedUrlString.insert(6, "/");
        }

        return fixedUrlString;
    }
}

void ChannelInterface::SetId(const QString& id)
{
    mId = id;
}

const QString& ChannelInterface::GetId() const
{
    return mId;
}

bool ChannelInterface::SetURL(const QString& url)
{
    if ( IsConnected() )
        return false;

    mUrl = QUrl::fromEncoded(url.toLatin1());

    return mUrl.isValid();
}

const QUrl& ChannelInterface::GetURL() const
{
    return mUrl;
}

QString ChannelInterface::GetUrlString() const
{
    QString result;
    if (mUrl.isValid())
    {
        return mUrl.toString();
    }

    return QString();
}

QString ChannelInterface::GetUrlPathAndExtra() const
{
    if (mUrl.isValid())
    {
        return mUrl.path();
    }

    return QString();
}

void ChannelInterface::SetUserName(const QString& user)
{
    mUrl.setUserName(user);
}

QString ChannelInterface::getUserName() const
{
    return mUrl.userName();
}

void ChannelInterface::SetPassword(const QString& pass)
{
    mUrl.setPassword(pass);
}

QString ChannelInterface::GetPassword() const
{
    return mUrl.password();
}

void ChannelInterface::AddRequestInfo(const QString& key, const QString& value)
{
    mRequestInfo[key] = value;
}

QString ChannelInterface::GetRequestInfo(const QString& key) const
{
    QString result;
    ChannelInfo::const_iterator it = mRequestInfo.find(key);

    if (it != mRequestInfo.end())
    {
        result = it.value();
    }

    return result;
}

void ChannelInterface::RemoveRequestInfo(const QString& key)
{
    mRequestInfo.remove(key);
}

void ChannelInterface::RemoveAllRequestInfo()
{
    mRequestInfo.clear();
}

QString ChannelInterface::GetResponseInfo(const QString& key) const
{
    QString result;
    ChannelInfo::const_iterator it = mResponseInfo.find(key);

    if ( it != mResponseInfo.end() )
    {
        result = it.value();
    }
    
    return result;
}

bool ChannelInterface::HasResponseInfo(const QString& key) const
{
    return mResponseInfo.find(key) != mResponseInfo.end();
}

const ChannelInfo& ChannelInterface::GetAllResponseInfo() const
{
    return mResponseInfo;
}

const ChannelInfo& ChannelInterface::GetAllRequestInfo() const
{
    return mRequestInfo;
}

qint32 ChannelInterface::GetErrorCode() const
{
    return mErrorCode;
}

/*QByteArray ChannelInterface::GetResponseReason() const
{
    return mResponseReason;
}

bool ChannelInterface::IsResponseCodeSuccess() const
{
    return GetResponseCode() / 100 == 2;
}*/

DownloadError::type ChannelInterface::GetLastErrorType() const
{
    return mErrorType;
}

void ChannelInterface::SetErrorCode(qint32 code)
{
    mErrorCode = code;
}

/*void ChannelInterface::SetResponseReason(const QByteArray& reason)
{
    mResponseReason = reason;
}*/

void ChannelInterface::AddResponseInfo(const QString& key, const QString& value)
{
    mResponseInfo[key] = value;
}

void ChannelInterface::SetLastErrorType(DownloadError::type err)
{
    mErrorType = err;
    mErrorDescription = ErrorTranslator::Translate((ContentDownloadError::type)err);
}

void ChannelInterface::onConnected()
{
    ChannelSpyList::iterator it;

    for ( it = mSpies.begin(); it != mSpies.end(); it++ )
        (*it)->onConnected(this);
}

void ChannelInterface::onDataAvailable(ulong* pCount, void** ppBuffer_ret)
{
    ChannelSpyList::iterator it;

    for ( it = mSpies.begin(); it != mSpies.end(); it++ )
        (*it)->onDataAvailable(this, pCount, ppBuffer_ret);
}

void ChannelInterface::onDataReceived(ulong count)
{
    ChannelSpyList::iterator it;

    for ( it = mSpies.begin(); it != mSpies.end(); it++ )
        (*it)->onDataReceived(this, count);
}

void ChannelInterface::onError(DownloadError::type error)
{
    ChannelSpyList::iterator it;

    for ( it = mSpies.begin(); it != mSpies.end(); it++ )
        (*it)->onError(this, error);
}

void ChannelInterface::onDisconnected()
{
    ChannelSpyList::iterator it;

    for ( it = mSpies.begin(); it != mSpies.end(); it++ )
        (*it)->onConnectionClosed(this);
}

void ChannelInterface::AddSpy(ChannelSpy* spy)
{
    if ( spy && std::find(mSpies.begin(), mSpies.end(), spy) == mSpies.end() )
        mSpies.push_back(spy);
}

void ChannelInterface::RemoveSpy(ChannelSpy* spy)
{
    ChannelSpyList::iterator it = std::find(mSpies.begin(), mSpies.end(), spy);

    if ( it != mSpies.end() )
        mSpies.erase(it);
}

} // namespace Downloader
} // namespace Origin

