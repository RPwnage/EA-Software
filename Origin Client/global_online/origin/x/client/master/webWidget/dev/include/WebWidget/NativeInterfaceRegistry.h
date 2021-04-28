#ifndef _WEBWIDGET_NATIVEINTERFACEMANAGER_H
#define _WEBWIDGET_NATIVEINTERFACEMANAGER_H

#include <QString>
#include <QByteArray>

#include "WebWidgetPluginAPI.h"
#include "FeatureRequest.h"
#include "NativeInterface.h"

class QObject;
class QWebFrame;

namespace WebWidget
{
    class NativeInterfaceFactory;

    ///
    /// Registry for associating native interfaces with a feature URI
    ///
    /// Registration maps a feature URI to either a shared NativeInterface or a NativeInterfaceFactory. See the
    /// NativeInterface documentation for a more complete explanation.
    ///
    /// \sa NativeInterface
    /// \sa NativeInterfaceFactory
    ///
    namespace NativeInterfaceRegistry
    {
        ///
        /// Returns the default feature URI for a given native interface name
        ///
        /// The default feature URI for a given native interface name will be 
        /// http://widgets.dm.origin.com/features/interfaces/{name}.
        ///
        /// \param  name  Name of the native interface. As a convention the JavaScript name returned by
        ///               NativeInterface::name() should be the same as the name passed to defaultFeatureUri().
        ///
        WEBWIDGET_PLUGIN_API QString defaultFeatureUri(const QByteArray &name);

        ///
        /// Registers a native interface that will have a shared instance across all widget instances
        ///
        /// This is the preferred way to register native interfaces. However, it does not support widget-specific
        /// parameters or state. registerInterfaceFactory() can be used where more flexibility is required.
        ///
        /// \param  inter       Native interface to register
        /// \param  featureUri  Feature URI widgets will use to identify the feature. If unspecified a URI will
        ///                     be built using defaultFeatureUri()
        /// \return Feature URI used to register the feature. This can be used to later unregister the interface with
        ///         unregisterInterface()
        ///
        WEBWIDGET_PLUGIN_API QString registerSharedInterface(const NativeInterface &inter, const QString &featureUri = QString());

        ///
        /// Registers a factory for creating a native interface per widget instance
        ///
        /// \param  featureUri  Feature URI widgets will use to identify the feature. Callers can build this using
        ///                     defaultFeatureUri()
        /// \param  factory     Factory for creating new native interfaces
        /// \return Feature URI used to register the feature. This can be used to later unregister the interface with
        ///         unregisterInterface()
        ///
        WEBWIDGET_PLUGIN_API QString registerInterfaceFactory(const QString &featureUri, NativeInterfaceFactory *factory);

        ///
        /// Unregisters a previously registered native interface
        ///
        /// The native interface may still be in use by any existing WidgetView instances; unregistering only prevents
        /// new views from accessing the interface.
        ///
        /// If a shared interface with NativeInterface::hasOwnership() set to true is unregistered the associated
        /// bridge object will be deleted.
        ///
        /// \param  featureUri  Feature URI of the interface to unregister
        ///
        WEBWIDGET_PLUGIN_API void unregisterInterface(const QString &featureUri);

        ///
        /// Returns a list of native interfaces to satisfy a list of feature requests
        ///
        /// \param  frame            Frame requesting the feature.
        /// \param  featureRequests  List of feature requests to examine for registered native interfaces.
        ///                          Satisfied feature requests will be removed from the list.
        /// \return List of NativeInterface objects. If NativeInterface::hasOwnership() returns true for an interface
        ///         it is the caller's responsibility to free the object when it is no longer needed.
        ///
        WEBWIDGET_PLUGIN_API QList<NativeInterface> interfacesForFeatureRequests(QWebFrame *frame, QList<FeatureRequest> &featureRequests);
    };
}

#endif
