///////////////////////////////////////////////////////////////////////////////
// PluginManager.cpp
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Plugin.h"
#include "engine/plugin/PluginManager.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/config/OriginConfigService.h"

namespace Origin
{
    namespace Engine
    {
        PluginManager *PluginManager::sInstance = NULL;
        
        PluginManager *PluginManager::instance()
        {
            init();
            return sInstance;
        }
        
        void PluginManager::init()
        {
            if(!sInstance)
                sInstance = new PluginManager();
        }

        void PluginManager::release()
        {
            if(sInstance)
            {
                delete sInstance; 
                sInstance = NULL;
            }
        }
        
        PluginManager::PluginManager() 
          : QObject()
          , mUnloadPluginsCallQueued(false)
        {
            ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(onUserLoggedIn()));
            ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(onUserLoggedOut()));

            if(LoginController::instance()->isUserLoggedIn())
            {
                onUserLoggedIn();
            }
        }

        PluginManager::~PluginManager()
        {
            ORIGIN_VERIFY_DISCONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(onUserLoggedIn()));
            ORIGIN_VERIFY_DISCONNECT(LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(onUserLoggedOut()));

            onUserLoggedOut();
        }

        bool PluginManager::runPlugin(const QString& productId) const
        {
            // Plugins must be started from the main thread to create UI, marshal
            // the call there if we're on some background thread
            if (QThread::currentThread() != thread())
            {
                PluginManager *self = const_cast<PluginManager *>(this);
                bool status = false;

                QMetaObject::invokeMethod(self, "runPlugin", Qt::BlockingQueuedConnection,
                    Q_RETURN_ARG(bool, status), Q_ARG(const QString &, productId));

                return status;
            }

            if(!mPlugins.contains(productId))
            {
                ORIGIN_LOG_ERROR << "Plugin " << productId << " not found.";
                return false;
            }

            Plugin *plugin = mPlugins[productId];
            if(!plugin->loaded())
            {
                plugin->load();
            }
            
            if(!plugin->loaded())
            {
                return false;
            }

            return plugin->run();
        }

        void PluginManager::shutdownPluginInternal(const QString& productId, bool deleteObject)
        {
            Plugin *plugin = NULL;
            if(mPlugins.contains(productId))
            {
                plugin = mPlugins[productId];
            }

            if(plugin)
            {
                QLibrary* pluginInstance = plugin->getInstance();

                if(plugin->loaded())
                {
                    ORIGIN_LOG_EVENT << "Shutting down plug-in " << productId << ".";
                    plugin->shutdown();
                }

                if(deleteObject)
                {
                    delete plugin;
                    mPlugins.remove(productId);
                }

                if(pluginInstance)
                {
                    mPluginsToUnload.insert(pluginInstance);
                }
            }

            if(!mUnloadPluginsCallQueued)
            {
                QMetaObject::invokeMethod(this, "unloadPlugins", Qt::QueuedConnection);
                mUnloadPluginsCallQueued = true;
            }
        }

        const bool PluginManager::pluginLoaded(const QString& productId)
        {
            bool pluginLoaded = false;

            Plugin *plugin = NULL;
            if(mPlugins.contains(productId))
            {
                plugin = mPlugins[productId];
            }

            if(plugin)
            {
                QLibrary* pluginInstance = plugin->getInstance();

                if(plugin->loaded())
                {
                    pluginLoaded = true;
                }
            }

            return pluginLoaded;
        }

        void PluginManager::onUserLoggedIn()
        {
            Content::ContentController* contentController = Content::ContentController::currentUserContentController();
            if(contentController)
            {
                if(contentController->initialFullEntitlementRefreshed())
                {
                    QList <Origin::Engine::Content::EntitlementRef> emptyList;
                    onEntitlementsUpdated(contentController->entitlements(), emptyList);
                }
                
                // Listen for updates to this user's content.
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), 
                    this, SLOT(onEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));
            }
        }
        
        void PluginManager::onUserLoggedOut()
        {
            // Stop listening for updates to this user's content.
            Content::ContentController* contentController = Content::ContentController::currentUserContentController();
            if(contentController)
            {
                ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), 
                    this, SLOT(onEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));
            }

            // Shut down any plug-ins we have loaded.
            QStringList keys = mPlugins.keys();
            for(QStringList::const_iterator it = keys.constBegin(); it != keys.constEnd(); ++it)
            {
                shutdownPluginInternal(*it, true);
            }

            ORIGIN_ASSERT(mPlugins.isEmpty());
            mPlugins.clear();
        }
        
        void PluginManager::onEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef> added, const QList<Origin::Engine::Content::EntitlementRef> removed)
        {
            for(QList<Content::EntitlementRef>::const_iterator it = added.constBegin(); it != added.constEnd(); ++it)
            {
                // Create any plug-ins that have just been added.  Load and initialization will be done lazily.
                Content::EntitlementRef ent = (*it);
                if(ent->contentConfiguration()->isPlugin())
                {
                    mPlugins.insert(ent->contentConfiguration()->productId(), 
                        new Plugin(ent->contentConfiguration()->productId(),
                        ent->localContent()->executablePath(),
                        ent->localContent()->installed(true),
                        ent->localContent()->installFlow()));
                }
            }

            for(QList<Content::EntitlementRef>::const_iterator it = removed.constBegin(); it != removed.constEnd(); ++it)
            {
                // Shut down and free any plug-ins that have been removed.
                Content::EntitlementRef ent = (*it);
                if(ent->contentConfiguration()->isPlugin())
                {
                    shutdownPluginInternal(ent->contentConfiguration()->productId(), true);
                }
            }

            static const QString odtPluginOfferId(Services::OriginConfigService::instance()->odtPlugin().productId);
            const bool developerMode = Origin::Services::readSetting(Origin::Services::SETTING_OriginDeveloperToolEnabled, Origin::Services::Session::SessionRef());

            // If the user doesn't have the ODT plugin in their entitlements,
            // but their system has developer mode activated, then go ahead
            // and add the plugin to their entitlements.
            if (developerMode && !mPlugins.contains(odtPluginOfferId))
            {
#if defined(ORIGIN_PC)
                const QString pluginPath(Services::OriginConfigService::instance()->odtPlugin().pcPath);
#elif defined(ORIGIN_MAC)
                const QString pluginPath(Services::OriginConfigService::instance()->odtPlugin().macPath);
#endif
                const QString pluginPathFull(Services::PlatformService::localFilePathFromOSPath(pluginPath));
                const bool pluginExists = QFile::exists(pluginPathFull);
                
                if (pluginExists)
                {
                    mPlugins.insert(odtPluginOfferId, new Plugin(odtPluginOfferId, pluginPathFull, pluginExists, NULL));
                }
            }
        }

        void PluginManager::unloadPlugins()
        {
            for (QSet<QLibrary*>::const_iterator it = mPluginsToUnload.begin(); it != mPluginsToUnload.end(); ++it)
            {
                (*it)->unload();
                delete *it;
            }

            mUnloadPluginsCallQueued = false;
            mPluginsToUnload.clear();
            emit pluginsUnloaded();
        }
    }
}

