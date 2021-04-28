// DownloadUrlProviderManager.cpp
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.

#include "DownloadUrlProviderManager.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

DownloadUrlProviderManager *DownloadUrlProviderManager::sInstance = nullptr;

void DownloadUrlProviderManager::init()
{
    if (nullptr == sInstance)
    {
        sInstance = new DownloadUrlProviderManager();
    }
}


void DownloadUrlProviderManager::release()
{
    delete sInstance;
    sInstance = nullptr;
}


DownloadUrlProviderManager *DownloadUrlProviderManager::instance()
{
    return sInstance;
}


DownloadUrlResponse *DownloadUrlProviderManager::downloadUrl(Session::SessionRef session, const QString& productId, const QString& buildId, const QString& buildReleaseVersion, const QString& preferCDN, bool https)
{
    DownloadUrlResponse *response = nullptr;

    for (auto provider = mProviderList.end(); nullptr == response && provider != mProviderList.begin();)
    {
        --provider;
        if (*provider)
        {
            response = (*provider)->downloadUrl(session, productId, buildId, buildReleaseVersion, preferCDN, https);
        }
    }

    return response;
}


bool DownloadUrlProviderManager::overrideUsesJitService(const QString &url)
{
    QUrl parsedUrl(url);
    QString scheme = parsedUrl.scheme().toLower();
    for (auto provider = mProviderList.end(); provider != mProviderList.begin();)
    {
        --provider;
        if (*provider && (*provider)->handlesScheme(scheme))
        {
             return (*provider)->overridesUseJitService();
        }
    }

    return false;
}


bool DownloadUrlProviderManager::registerProvider(DownloadUrlProvider *provider)
{
    QListIterator<DownloadUrlProvider *> iter(mProviderList);

    if (!iter.findNext(provider))
    {
        mProviderList.append(provider);
        return true;
    }
    else
    {
        return false;
    }
}


bool DownloadUrlProviderManager::unregisterProvider(DownloadUrlProvider *provider)
{
    return mProviderList.removeOne(provider);
}

} // namespace Publishing

} // namespace Services

} // namespace Origin
