///////////////////////////////////////////////////////////////////////////////
// Plugin.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef PLUGIN_H
#define PLUGIN_H

#include "engine/content/Entitlement.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "services/common/VersionInfo.h"

#include <QObject>
#include <QLibrary>

namespace Origin 
{
    namespace Downloader
    {
        class IContentInstallFlow;
    }

    namespace Engine
    {
        /// \brief Platform-agnostic wrapper class for an Origin plugin.
        class Plugin : public QObject
        {
            Q_OBJECT
        public:
            enum PluginLoadError
            {
                kNoError = 0,                 ///< No error.
                kPluginVersionIncompatible,   ///< Plugin version is incompatible with this client.
                kOsLoadError,                 ///< Operating system was unable to load plug-in.
                kMissingRequiredFunction,     ///< Plug-in is missing a required exported function.
                kSignatureVerificationFailed, ///< The signature was not present or was invalid.
                kPluginInitFailed,            ///< The plug-in's exported "init" function reported a failure.
            };

            /// \brief The Plugin constructor
            /// \param ent The entitlement associated with this plugin.
            Plugin(const QString& productId, const QString& executablePath, const bool installed, Downloader::IContentInstallFlow* installFlow);

            /// \brief The Plugin destructor
            ~Plugin();

            /// \brief Loads the plug-in using LoadLibrary or dlopen and initializes the plug-in.
            /// \return True if plug-in was loaded successfully.
            void load();

            /// \brief Runs the plug-in.
            /// \return True if plug-in was run successfully.
            bool run();

            /// \brief Uninitializes the plug-in.
            void shutdown();

            /// \brief Checks if the plug-in is currently loaded.
            /// \return True if the plug-in is currently loaded.
            bool loaded() const { return mLoaded; }

            QLibrary* getInstance() { return mLibrary; }

        private slots:
            void onDownloadStateChanged(Origin::Downloader::ContentInstallFlowState state);

        private:
            typedef int (*PluginOperationFunc)();

            bool bindPluginOperation(PluginOperationFunc& op, const char *functionName);
            bool loadInternal();

            QString mProductId;
            QString mExecutablePath;
            VersionInfo mPluginVersion;
            VersionInfo mCompatibleClientVersion;
            QLibrary* mLibrary;
            Downloader::IContentInstallFlow* mInstallFlow;
            bool mInstalled;
            bool mLoaded;

            PluginOperationFunc mInitFunc;
            PluginOperationFunc mRunFunc;
            PluginOperationFunc mShutdownFunc;
        };
    }
}

#endif // PLUGIN_H
