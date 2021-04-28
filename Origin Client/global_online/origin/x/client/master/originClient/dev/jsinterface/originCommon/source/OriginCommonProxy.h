/////////////////////////////////////////////////////////////////////////////
// OriginCommonProxy.h
//
// Copyright (c) 2014 Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef _ORIGINCOMMONPROXY_H
#define _ORIGINCOMMONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include "services/plugin/PluginAPI.h"
#include <QObject>

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OriginCommonProxy : public QObject
{
    Q_OBJECT
public:
    OriginCommonProxy();
    ~OriginCommonProxy();

    Q_INVOKABLE QString readOverride(QString sectionName, QString overrideName);
    Q_INVOKABLE QString readOverride(QString sectionName, QString overrideName, QString id);
    
    Q_INVOKABLE void writeOverride(QString sectionName, QString overrideName, QString value);
    Q_INVOKABLE void writeOverride(QString sectionName, QString overrideName, QString id, QString value);

    Q_INVOKABLE bool reloadConfig();

    /// \brief Applies pending config file changes. A restart may be required.
    ///
    /// Callers need to check the return value of this function to determine 
    /// whether a restart is needed. Restarting the client will be the caller's
    /// responsibility.
    ///
    /// \return true if a restart is required, false otherwise.
    Q_INVOKABLE bool applyChanges();

    Q_INVOKABLE void alert(const QString &message, const QString &title = "Alert", const QString &icon = "NOTICE");

    Q_INVOKABLE void launchExternalBrowser(QString urlstring);

    Q_INVOKABLE void viewSDDDocumentation();

private:
    Q_DISABLE_COPY(OriginCommonProxy);
};

}
}
}

#endif
