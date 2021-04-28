///////////////////////////////////////////////////////////////////////////////
// PluginManager.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QMap>
#include <QString>
#include <QObject>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin 
{
    namespace Engine
    {
        class Plugin;

        /// \brief Manager class that indexes and operates upon a user's plug-ins.
        class ORIGIN_PLUGIN_API PluginManager : public QObject
        {
            Q_OBJECT
        public:
            /// \brief Returns the current PluginManager instance.
            /// \return The current PluginManager instance.
            static PluginManager* instance();

            static void init();
            
            /// \brief Destroys the current PluginManager instance.
            static void release();

            /// \brief Runs a plug-in.  If the plug-in has not yet been loaded, this function will attempt to load and initialize it first.
            /// \param productId The product ID associated with a plugin.
            /// \return True if plugin was started up successfully.
            Q_INVOKABLE bool runPlugin(const QString& productId) const;

            /// \brief Can be used to determine if a particular plugin is active.
            /// \return True if the given plugin has been loaded, false otherwise
            const bool pluginLoaded(const QString& productId);

        signals:
            /// \brief Emitted when a queued unloadPlugins call has completed
            void pluginsUnloaded();
            
        private slots:
            /// \brief Called when a user logs in.  Listens for entitlement updates.
            void onUserLoggedIn();
            
            /// \brief Called when current user logs out.  Uninitializes and frees all plugins.
            void onUserLoggedOut();

            /// \brief Called when user's entitlements have been updated.  Populates the mPlugins map with plugins.
            /// \param added The list of new entitlements added in the update.
            /// \param removed The list of existing entitlements removed by the update.
            void onEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef> added, const QList<Origin::Engine::Content::EntitlementRef> removed);

        private:
            PluginManager();
            ~PluginManager();
            Q_DISABLE_COPY(PluginManager);

            /// \brief Called after plugins have been shutdown so any deleteLater calls can complete
            Q_INVOKABLE void unloadPlugins();

            /// \brief Internal function to shutdown a plugin and optionally remove it.
            void shutdownPluginInternal(const QString& productId, bool deleteObject);

            static PluginManager* sInstance;

            QMap<QString, Plugin*> mPlugins; ///< Map of plugins by product ID.
            QSet<QLibrary*> mPluginsToUnload; ///< Set of plugins to unload on the next event loop.
            bool mUnloadPluginsCallQueued; ///< Set when an unloadPlugins call has been queued.
        };
    }
}

#endif // PLUGINMANAGER_H
