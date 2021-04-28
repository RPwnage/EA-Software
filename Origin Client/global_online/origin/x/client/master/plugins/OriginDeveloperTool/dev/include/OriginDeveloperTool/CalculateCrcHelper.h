//    CalculateCrcHelper.h
//    Copyright (c) 2013, Electronic Arts
//    All rights reserved.

#ifndef CALCULATECRC_HELPER_H
#define CALCULATECRC_HELPER_H

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

namespace Plugins
{
namespace DeveloperTool
{
    /// \class CalculateCrcHelper
    /// \brief This class will take a download URL for a specified content configuration, and spawn a ContentProtocol to extract crc details for the files in the package.  Crc info is then saved to disk.
    class CalculateCrcHelper : public QObject
    {
        Q_OBJECT

    public:
        /// \brief Constructs the helper for the specified content
        /// \param content - The content configuration of the content that owns the DiP manifest.
        CalculateCrcHelper(const Origin::Engine::Content::ContentConfigurationRef content);

        /// \brief Destructor cleans up protool
        ~CalculateCrcHelper();

        /// \brief Asynchronously kicks off update calculation.
        /// \param downloadUrl - Can be local download override URL or JIT url, where the DiP package resides.
        bool calculateCrc(const QString& downloadUrl);

        /// \brief Cancels and detroys the protocol used to calculate the update details
        void cleanup();

    private slots:
        void Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo);
        void onProtocolInitialized();

    signals:
        void protocolError();
        void finished();

    private:
        Origin::Downloader::ContentProtocolPackage* mProtocol;
        const Origin::Engine::Content::ContentConfigurationRef mContent;
    };
}
}
}

#endif
