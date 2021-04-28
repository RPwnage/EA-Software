///////////////////////////////////////////////////////////////////////////////
// DownloadUrlServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _DownloadUrlServiceClient_H
#define _DownloadUrlServiceClient_H

#include "ECommerceServiceClient.h"
#include "DownloadUrlProviderManager.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

///
/// \brief HTTP service client for the Download URL generation
///
class ORIGIN_PLUGIN_API DownloadUrlServiceClient : public ECommerceServiceClient, public DownloadUrlProvider
{
public:
    friend class OriginClientFactory<DownloadUrlServiceClient>;

    static void init();

    static void release();

    static DownloadUrlServiceClient *instance();

    DownloadUrlResponse *downloadUrl(Session::SessionRef session, const QString& productId, const QString& buildId="", const QString& buildReleaseVersion="", const QString& preferCDN="", bool https=false);

    virtual bool handlesScheme(const QString &scheme);
    virtual bool overridesUseJitService() { return false; }

private:
    /// 
    /// \brief Creates a new CMS Download service client
    ///
    /// \param nam           QNetworkAccessManager instance to send requests through
    ///
    explicit DownloadUrlServiceClient(NetworkAccessManager *nam = NULL);
};

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif //_DownloadUrlServiceClient_H
