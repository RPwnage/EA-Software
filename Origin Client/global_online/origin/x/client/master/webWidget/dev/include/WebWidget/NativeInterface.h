#ifndef _WEBWIDGET_NATIVEINTERFACE_H
#define _WEBWIDGET_NATIVEINTERFACE_H

#include <QObject>
#include <QByteArray>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Represents an application-provided interface to native code from JavaScript
    ///
    /// Native interfaces are either registered as a shared object with 
    /// NativeInterfaceRegistry::registerSharedInterface() or created by an interface factory registered with
    /// NativeInterfaceRegistry::registerInterfaceFactory(). Widgets can then request these interfaces be attached
    /// to their JavaScript context by the \<feature\> element in their configuration document. See 
    /// http://www.w3.org/TR/widgets/#the-feature-element-and-its-attributes for more information on feature
    /// configuration.
    ///
    /// For information on how the native interface QObjects are bridged with JavaScript please refer to the Qt
    /// documentation at http://developer.qt.nokia.com/doc/qt-4.8/qtwebkit-bridge.html
    ///
    /// \par Security Considerations
    ///
    /// Native interfaces can potentially be subverted for malicious purposes via a compromised widget. One scenario
    /// is a widget having external JavaScript injected using cross-site scripting (XSS). As such it is critical that
    /// native interface and widget authors think carefully about potential security issues. 
    ///
    /// Best practices for native interface authors include:
    /// - Never trust the widget to validate data or enforce business rules. While the widget may perform its own 
    ///   validation for UI purposes, validation must be repeated by the native interface before any action is carried
    ///   out on behalf of the widget. If these business rules or validations are complex consider exposing an
    ///   function in the native interface that allows the widget to share the validation logic with native code.
    ///   This rule is most easily followed by adhering to a strict Model/View/Controller architecture where the
    ///   interface provides the model and controller.
    ///
    /// - Avoid overly broad or generic functionality that can be leveraged for unintended purposes. Examples of risky
    ///   functionality would be reading an arbitrary file from the filesystem or a getting a user's setting by name.
    ///   Instead restrict the native interface to operations that can be easily verified to be safe. If a small part
    ///   of a larger interface is frequently reused for auxiliary purposes by other widgets consider breaking it out
    ///   in to a self-contained interface.
    ///
    /// - Where possible confirm any destructive user action in native code. An example might be confirming blocking a
    ///   friend from social by showing a message box in the native interface. This prevents a subverted widget from
    ///   performing malicious actions without input from the user. Be careful when using any text strings provided
    ///   by the widget during the confirmation flow as they might be modified to mislead the user as to what they're
    ///   confirming.
    ///
    /// Widget authors can reduce the impact of compromised widgets by requesting only the native interfaces they
    /// require. This reduces the attack surface as it is not possible for a widget to access new interfaces at
    /// runtime; widgets are restricted to the interfaces requested in their configuration.
    ///
    /// \sa NativeInterfaceRegistry
    ///
    class WEBWIDGET_PLUGIN_API NativeInterface
    {
    public:
        ///
        /// Creates a new native interface
        ///
        /// \param  name           Name of the native interface in the JavaScript environment. This should be a valid
        ///                        JavaScript identifier. This means interface names start with a letter and consist
        ///                        only of ASCII letters, numbers and underscores.
        /// \param  bridgeObject   Interface QObject to bridge to JavaScript. For details on the QtWebKit bridge see
        ///                        http://developer.qt.nokia.com/doc/qt-4.8/qtwebkit-bridge.html
        /// \param  takeOwnership  Indicates if the Web Widget framework should take ownership of the interface object.
        ///                        For NativeInterfaces produced by NativeInterfaceFactory this means the bridge object
        ///                        will be deleted when the widget instance exits. For a shared native interface this
        ///                        this means the bridge object will be deleted if the shared interface is
        ///                        unregistered.
        ///
        NativeInterface(const QByteArray &name, QObject *bridgeObject, bool takeOwnership = false) :
            mName(name),
            mBridgeObject(bridgeObject),
            mHasOwnership(takeOwnership)
        {
        }

        ///
        /// Creates a null native interface
        ///
        /// Null native interfaces can be returned from NativeInterfaceFactory::createNativeInterface() to decline
        /// to create a native interface
        ///
        NativeInterface() :
            mBridgeObject(NULL),
            mHasOwnership(false)
        {
        }

        ///
        /// Return true if the native interface is null
        ///
        bool isNull() const 
        {
            return mBridgeObject == NULL;
        }

        ///
        /// Returns the name of the native interface in the JavaScript environment
        ///
        QByteArray name() const { return mName; }
        
        ///
        /// Returns the QObject to bridge to JavaScript
        ///
        QObject *bridgeObject() const { return mBridgeObject; }

        ///
        /// Returns if the Web Widget framework has ownership of the interface object
        ///
        bool hasOwnership() const { return mHasOwnership; }

        ///
        /// Creates a clone of the current object
        ///
        /// Clones are identical to the original with hasOwnership() flag cleared. This is to prevent the bridge
        /// object from being double deleted
        ///
        NativeInterface clone() const { return NativeInterface(name(), bridgeObject()); }

    private:
        QByteArray mName;
        QObject *mBridgeObject;
        bool mHasOwnership;
    };
}

#endif
