/////////////////////////////////////////////////////////////////////////////
// OriginIGOProxy.h
//
// Copyright (c) 2014 Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef _ORIGINIGOPROXY_H
#define _ORIGINIGOPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include "services/plugin/PluginAPI.h"
#include <QObject>
#include <QPointer>

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OriginIGOProxy : public QObject
{
    Q_OBJECT
public:
    OriginIGOProxy();
    ~OriginIGOProxy();

////////////////////////////////////////////////////////////////////////////////////////////////
    // Object setup

    // Defines the object in charge of the decision making during a JS createWindow request - should be called from the C++/client side.
    void setCreateWindowHandler(QObject* handler);

////////////////////////////////////////////////////////////////////////////////////////////////
    // JS actual interface

    // Defines the url request for the upcoming JS createWindow request - should be called from the JS side 
    // available to the QWebPage::createWindow method
    Q_INVOKABLE void moveWindowToFront(const QString& url);
    Q_INVOKABLE void setCreateWindowRequest(const QString& url);
    Q_INVOKABLE void openIGOConversation();
    Q_INVOKABLE void openIGOProfile(const QString&);

private:
    Q_DISABLE_COPY(OriginIGOProxy);

    QPointer<QObject> mCreateWindowHandler;
};

}
}
}

#endif
