#ifndef _NATIVEINTERFACEINJECTOR_H
#define _NATIVEINTERFACEINJECTOR_H

#include <QObject>
#include <QList>

#include "FeatureRequest.h"
#include "SecurityOrigin.h"
#include "NativeInterface.h"

class QWebFrame;

namespace WebWidget
{
    ///
    /// Injects a list of native interfaces in to a QWebFrame when it's navigated to a specific security origin  
    ///
    class NativeInterfaceInjector : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// Creates a new NativeInterfaceInjector for the given feature requests
        /// 
        /// Returns null if any required feature requests can't be satisfied
        ///
        /// \param  frame             QWebFrame to inject native interfaces in to. This also becomes the Qt parent of
        ///                           the NativeInterfaceInjector instance.
        /// \param  trustedOrigin     Trusted security origin for the native interfaces. Interfaces will only be 
        ///                           injected in to pages loaded from this origin.
        /// \param  nativeInterfaces  List of native interfaces to inject.
        ///
        NativeInterfaceInjector(QWebFrame *frame, const SecurityOrigin &trustedOrigin, const QList<NativeInterface> &nativeInterfaces);

        ~NativeInterfaceInjector();
    
    private slots:
        void javaScriptWindowObjectCleared();

    private:
        QWebFrame *mFrame;
        SecurityOrigin mSecurityOrigin;
        QList<NativeInterface> mNativeInterfaces;
    };
}

#endif

