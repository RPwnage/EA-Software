//    CalculateUpdateDetailsHelper.h
//    Copyright (c) 2013, Electronic Arts
//    All rights reserved.

#ifndef CALCULATEUPDATEDETAILS_HELPER_H
#define CALCULATEUPDATEDETAILS_HELPER_H

#include <QObject>
#include "engine/content/ContentConfiguration.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Downloader
{
    class ContentProtocolPackage;
}

namespace Engine
{
namespace Debug
{
    /// \class CalculateUpdateDetailsHelper
    /// \brief This class is a small utilize class that will take a download URL for a specified content configuration, and spawn a ContentProtocol to calculate update details.  
    class ORIGIN_PLUGIN_API CalculateUpdateDetailsHelper : public QObject
    {
        Q_OBJECT

    public:
        /// \brief Constructs the helper for the specified content
        /// \param downloadUrl - Can be local download override URL or JIT url, where the DiP package resides.
        /// \param content - The content configuration of the content that owns the DiP manifest.
        CalculateUpdateDetailsHelper(const QString& downloadUrl, const Origin::Engine::Content::ContentConfigurationRef content);

        /// \brief Destructor cleans up protool
        ~CalculateUpdateDetailsHelper();

        /// \brief Asynchronously kicks off update calculation.  User should be lisenting to DownloadDebugDataManager/DownloadDebugDataCollector signals
        /// to receive update detail information
        bool calculateUpdateDetails(const QString& installPath, const QString& cachePath);

        /// \brief Returns download URL that was used to calculate this update.
        QString downloadUrl() const { return mDownloadUrl; }

        /// \brief Cancels and detroys the protocol used to calculate the update details
        void cleanup();

    private slots:
        void Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo);
        void onProtocolInitialized();

    signals:
        void protocolError();
        void success();

    private:
        Origin::Downloader::ContentProtocolPackage* mProtocol;
        QString mDownloadUrl;
        const Origin::Engine::Content::ContentConfigurationRef mContent;

    };
}
}
}

#endif
