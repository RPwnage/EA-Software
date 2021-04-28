// DownloadUrlProviderManager.h
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.

#ifndef _DownloadUrlProviderManager_H
#define _DownloadUrlProviderManager_H

#include "DownloadUrlServiceResponses.h"

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
class ORIGIN_PLUGIN_API DownloadUrlProvider
{
public:
    /// \brief Returns download URL for the user
    /// \param session The current session user Id.
    /// \param productId The product id from the PDLC.
    /// \param buildId The unique ID of the build to download.  Only relevant for 1102/1103 entitlements
    /// \param buildReleaseVersion The specific build release version.  Only relevant for 1102/1103 entitlements. Deprecated as of 9.5. 
    /// \param preferCDN The preferred CDN to use.  (Will be ignored if a CDN override is specified by EACore.ini)
    /// \param https Flag which indicates whether the returned URL should be SSL/TLS-enabled (HTTPS), if available
	virtual DownloadUrlResponse *downloadUrl(Session::SessionRef session, const QString& productId, const QString& buildId, const QString& buildReleaseVersion, const QString& preferCDN, bool https) = 0;

    virtual bool handlesScheme(const QString &scheme) = 0;
    virtual bool overridesUseJitService() = 0;
};


class ORIGIN_PLUGIN_API DownloadUrlProviderManager
{
public:
    static void init();

    static void release();

    static DownloadUrlProviderManager *instance();

    DownloadUrlResponse *downloadUrl(Session::SessionRef session, const QString& productId, const QString& buildId, const QString& buildReleaseVersion="", const QString& preferCDN="", bool https=false);
    bool overrideUsesJitService(const QString &url);

    bool registerProvider(DownloadUrlProvider *provider);
    bool unregisterProvider(DownloadUrlProvider *provider);

private:
    static DownloadUrlProviderManager *sInstance;

    QList<DownloadUrlProvider *> mProviderList;
};

} // namespace Publishing

} // namespace Services

} // namespace Origin

#endif // _DownloadUrlProviderManager_H
