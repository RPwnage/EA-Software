#ifndef _WEBWIDGET_NATIVEINTERFACEFACTORY_H
#define _WEBWIDGET_NATIVEINTERFACEFACTORY_H

// For FeatureParameters
#include "FeatureRequest.h"
#include "NativeInterface.h"
#include "WebWidgetPluginAPI.h"

class QObject;
class QWebFrame;

namespace WebWidget
{
    ///
    /// Interface for creating per-widget native interfaces
    ///
    /// Where possible native interfaces should avoid keeping widget-specific state and register globally with
    /// NativeInterfaceRegistry::registerSharedInterface(). However, if widget-specific state or parameterization are 
    /// required NativeInterfaceFactory can be subclassed to create native interface instances per widget.
    ///
    /// \sa NativeInterface
    /// \sa NativeInterfaceRegistry
    ///
    class WEBWIDGET_PLUGIN_API NativeInterfaceFactory
    {
    public:
        ///
        /// Create a new native interface for the given parameters
        ///
        /// \param  frame       Frame requesting the feature. This can be used for features that are coupled with the
        ///                     state of the frame
        /// \param  parameters  Parameters for the feature declared in the requesting widget's configuration document
        /// \return Native interface to attach to the the widget JavaScript context.
        ///         If a null NativeInterface is returned no native interface will be attached and the widget will
        ///         fail to load if the feature request was declared as required.
        ///
        virtual NativeInterface createNativeInterface(QWebFrame *frame, const FeatureParameters &parameters) const = 0;
    };
}

#endif
