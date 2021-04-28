//    RetrieveDiPManifestHelper.h
//    Copyright (c) 2013, Electronic Arts
//    All rights reserved.

#ifndef RETRIEVEDIPMANIFEST_HELPER_H
#define RETRIEVEDIPMANIFEST_HELPER_H

#include <QObject>
#include "engine/content/ContentConfiguration.h"
#include "services/plugin/PluginAPI.h"
#include "DiPManifest.h"

namespace Origin 
{
    namespace Downloader
    {
        /// \class RetrieveDiPManifestHelper
        /// \brief This class is a small utilize class that will take a download URL for a specified content configuration, and retrieve the DiP manifest from the URL synchronously.  
        /// Results are cached so that the DiP manifest is only fetched once per content.  Will fail and return empty manifest for non-dip or invalid content.
        class ORIGIN_PLUGIN_API RetrieveDiPManifestHelper : public QObject
        {
            Q_OBJECT

        public:
            /// \brief Clears the static manifest cache, causing further requests to go to the download URL
            static void clearManifestCache();

        public:
            /// \brief Constructs the helper for the specified content
            /// \param downloadUrl - Can be local download override URL or JIT url, where the DiP package resides.
            /// \param content - The content configuration of the content that owns the DiP manifest.
            /// \param preserveManifestOnDisk - If set to true, the temp file on disk for manifest will be preserved.  Should be fetched by manifestFilePath and deleted by caller.
            RetrieveDiPManifestHelper(const QString& downloadUrl, const Origin::Engine::Content::ContentConfigurationRef content, bool preserveManifestOnDisk = false);

            /// \brief Synchronously retrieves the DiP manifest, blocking the current thread until complete.
            bool retrieveDiPManifestSync();

            /// \brief Returns the fetched manifest, or an empty manifest if retrieveDiPManifestSync failed for any reason.
            DiPManifest& dipManifest();

            /// \brief Returns a string containing the full contents of the Di
            QString& manifestFilePath();

        private slots:
            void ManifestFileReceived(const QString& archivePath, const QString& diskPath);
            void Protocol_Error(Origin::Downloader::DownloadErrorInfo errorInfo);
        
        signals:
            void protocolError();
            void success();

        private:
            static QHash<QString, DiPManifest> sDiPManifestCache;
            

            QString mDownloadUrl;
            bool mPreserveFile;
            QString mManifestDiskPath;

            const Origin::Engine::Content::ContentConfigurationRef mContent;
            DiPManifest mManifest;
        };
    }
}

#endif
