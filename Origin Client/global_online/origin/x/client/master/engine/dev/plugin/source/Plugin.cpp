///////////////////////////////////////////////////////////////////////////////
// Plugin.cpp
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Plugin.h"
#include "engine/content/LocalContent.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "TelemetryAPIDLL.h"
#include "version/version.h"

#ifdef ORIGIN_MAC
#include <dlfcn.h>
#endif

namespace Origin
{
    namespace Engine
    {
        Plugin::Plugin(const QString& productId, const QString& executablePath, const bool installed, Downloader::IContentInstallFlow* installFlow)
          : mProductId(productId),
            mExecutablePath(executablePath),
            mLibrary(NULL),
            mInstallFlow(installFlow),
            mInstalled(installed),
            mLoaded(false),
            mInitFunc(NULL),
            mRunFunc(NULL),
            mShutdownFunc(NULL)
        {
            if (mInstalled)
            {
                // If we are already installed, great.  Get plugin version and compatible client version from binary/plist
                Services::PlatformService::getPluginVersionInfo(mExecutablePath, mPluginVersion, mCompatibleClientVersion);
            }

            if (mInstallFlow)
            {
                // Listen for download state changes so we know when the update our versions.
                ORIGIN_VERIFY_CONNECT(installFlow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), 
                    this, SLOT(onDownloadStateChanged(Origin::Downloader::ContentInstallFlowState)));
            }
        }

        Plugin::~Plugin()
        {
        }

        void Plugin::load()
        {
            if(!loadInternal() && mLibrary)
            {
                mLibrary->unload();
                delete mLibrary;
                mLibrary = NULL;
            }
        }

        bool Plugin::loadInternal()
        {
            if(mLibrary)
            {
                return true;
            }

            ORIGIN_LOG_EVENT << "Loading plug-in: " << mProductId;

            QString libraryPath = mExecutablePath;

            // Make sure plug-in is compatible before we do anything else.
            if(mCompatibleClientVersion != VersionInfo(EALS_VERSION_P_DELIMITED))
            {
                ORIGIN_LOG_ERROR << "Plugin " << mProductId << " only compatible with " << 
                    mCompatibleClientVersion.ToStr() << ".  Currently running " << EALS_VERSION_P_DELIMITED << ".";
                GetTelemetryInterface()->Metric_PLUGIN_LOAD_FAILED(mProductId.toUtf8().constData(), 
                    mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), kPluginVersionIncompatible);
                return false;
            }

            mLibrary = new QLibrary(libraryPath);
            if(!mLibrary->load())
            {
                Services::PlatformService::SystemResultCode err = Services::PlatformService::lastSystemResultCode();
                    
                ORIGIN_LOG_ERROR << "Plugin " << mProductId << " failed to load.  Error code = " << err;
#ifdef ORIGIN_MAC
                // OS X offers additional context on dlopen, dlopen_preflight, dlsym, and dlclose failures.
                // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/dlerror.3.html
                ORIGIN_LOG_DEBUG << "Error context = " << dlerror();
#endif

                GetTelemetryInterface()->Metric_PLUGIN_LOAD_FAILED(mProductId.toUtf8().constData(), 
                    mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), kOsLoadError, err);
                return false;
            }

            bool allFunctionsFound = bindPluginOperation(mInitFunc, "init");
            allFunctionsFound &= bindPluginOperation(mRunFunc, "run");
            allFunctionsFound &= bindPluginOperation(mShutdownFunc, "shutdown");

            // If any of our functions are missing, we can't continue.
            if(!allFunctionsFound)
            {
                // Log statements printed in bindPluginOperation.
                Services::PlatformService::SystemResultCode err = Services::PlatformService::lastSystemResultCode();

                GetTelemetryInterface()->Metric_PLUGIN_LOAD_FAILED(mProductId.toUtf8().constData(), 
                    mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), kMissingRequiredFunction, err);

                return false;
            }

            QString originPath = QCoreApplication::applicationFilePath();
            libraryPath = mExecutablePath;
            bool certificatesMatch = Services::PlatformService::compareFileSigning(originPath, libraryPath);

#ifdef DEBUG
            certificatesMatch = true;
#endif

            if(!certificatesMatch)
            {
                ORIGIN_LOG_ERROR << "Plugin " << mProductId << " failed signature verification.";
                GetTelemetryInterface()->Metric_PLUGIN_LOAD_FAILED(mProductId.toUtf8().constData(), 
                    mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), kSignatureVerificationFailed);
                return false;
            }
            
            ORIGIN_LOG_EVENT << "Initializing plug-in: " << mProductId;

            int exitCode = mInitFunc();
            
            GetTelemetryInterface()->Metric_PLUGIN_OPERATION(mProductId.toUtf8().constData(), 
                mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), "init", exitCode);

            if(exitCode != 0)
            {
                ORIGIN_LOG_ERROR << "Plugin " << mProductId << " encountered a problem while attemting to initalize.  Exit code = " << exitCode;
                GetTelemetryInterface()->Metric_PLUGIN_LOAD_FAILED(mProductId.toUtf8().constData(), 
                    mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), kPluginInitFailed);
                return false;
            }
            
            ORIGIN_LOG_EVENT << "Plugin " << mProductId << " successfully loaded.";
            GetTelemetryInterface()->Metric_PLUGIN_LOAD_SUCCESS(mProductId.toUtf8().constData(), 
                mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData());

            mLoaded = true;
            return true;
        }
        
        bool Plugin::run()
        {            
            if(!mLoaded)
            {
                ORIGIN_LOG_ERROR << "Plugin " << mProductId << " not loaded.";
                return false;
            }
            
            ORIGIN_LOG_EVENT << "Running plug-in: " << mProductId;

            int exitCode = mRunFunc();
            
            GetTelemetryInterface()->Metric_PLUGIN_OPERATION(mProductId.toUtf8().constData(), 
                mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), "run", exitCode);

            if(exitCode != 0)
            {
                ORIGIN_LOG_ERROR << "Plugin " << mProductId << " encountered a problem while attemting to run.  Exit code = " << exitCode;
                return false;
            }

            return true;
        }
        
        void Plugin::shutdown()
        {
            if(mLoaded)
            {
                ORIGIN_LOG_EVENT << "Shutting down plug-in: " << mProductId;

                int exitCode = mShutdownFunc();
                
                GetTelemetryInterface()->Metric_PLUGIN_OPERATION(mProductId.toUtf8().constData(), 
                    mPluginVersion.ToStr().toUtf8().constData(), mCompatibleClientVersion.ToStr().toUtf8().constData(), "shutdown", exitCode);

                if(exitCode != 0)
                {
                    ORIGIN_LOG_ERROR << "Plugin " << mProductId << " encountered a problem while attempting to unload.  Exit code = " << exitCode;
                }

                mLibrary = NULL;
                mLoaded = false;
            }
        }
        
        bool Plugin::bindPluginOperation(PluginOperationFunc &op, const char *functionName)
        {
            op = (PluginOperationFunc)mLibrary->resolve(functionName);

            if(!op)
            {
                ORIGIN_LOG_ERROR << "Could not find required function '" << functionName << "' in plug-in " << mProductId 
                    << ".  Error code = " << Services::PlatformService::lastSystemResultCode();

#ifdef ORIGIN_MAC
                    // OS X offers additional context on dlopen, dlopen_preflight, dlsym, and dlclose failures.
                    // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/dlerror.3.html
                    ORIGIN_LOG_DEBUG << "Error context = " << dlerror();
#endif
                return false;
            }

            return true;
        }

        void Plugin::onDownloadStateChanged(Origin::Downloader::ContentInstallFlowState state)
        {
            if(state == Downloader::ContentInstallFlowState::kCompleted)
            {
                QString libraryPath = mExecutablePath;

                // Get plugin version and compatible client version from binary/plist
                Services::PlatformService::getPluginVersionInfo(libraryPath, mPluginVersion, mCompatibleClientVersion);
            }
        }
    }
}

