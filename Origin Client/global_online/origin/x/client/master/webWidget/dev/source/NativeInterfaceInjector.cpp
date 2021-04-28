#include <QDebug>
#include <QtWebKitWidgets/QWebFrame>

#include "NativeInterfaceInjector.h"

namespace WebWidget
{
    void NativeInterfaceInjector::javaScriptWindowObjectCleared()
    {
        // Check to make sure we're in the right security origin
        SecurityOrigin currentOrigin(mFrame->requestedUrl());

        if (mSecurityOrigin.isNull() || (currentOrigin != mSecurityOrigin))
        {
            // The frame's security origin is missing or doesn't match the widget's security origin
            return;
        }
        
        // Add all the native interfaces on
        for(QList<NativeInterface>::const_iterator it = mNativeInterfaces.constBegin();
            it != mNativeInterfaces.constEnd();
            it++)
        {
            mFrame->addToJavaScriptWindowObject(it->name(), it->bridgeObject());
        }
    }
        
    NativeInterfaceInjector::~NativeInterfaceInjector()
    {
        // Delete any interface objects we have ownership of
        for(QList<NativeInterface>::const_iterator it = mNativeInterfaces.constBegin();
            it != mNativeInterfaces.constEnd();
            it++)
        {
            if (it->hasOwnership())
            {
                delete it->bridgeObject();
            }
        }
    }
    
    NativeInterfaceInjector::NativeInterfaceInjector(QWebFrame *frame, const SecurityOrigin &trustedOrigin, const QList<NativeInterface> &nativeInterfaces) :
        QObject(frame),
        mFrame(frame),
        mSecurityOrigin(trustedOrigin),
        mNativeInterfaces(nativeInterfaces)
    {
        // Notify us whenever we need to rebuild the JavaScript window object
        connect(frame, SIGNAL(javaScriptWindowObjectCleared()),
            this, SLOT(javaScriptWindowObjectCleared()));
    }
}
