#ifndef _ABSTRACTROSTERVIEW_H
#define _ABSTRACTROSTERVIEW_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QSet>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Chat
{
class RemoteUser;
}

namespace Client
{
namespace JsInterface
{

class RemoteUserProxy;
class OriginSocialProxy;

class ORIGIN_PLUGIN_API AbstractRosterView : public QObject
{
    Q_OBJECT
    friend class OriginSocialProxy;

public:
    AbstractRosterView(QObject *parent = NULL);
    virtual ~AbstractRosterView() {}

    Q_PROPERTY(QObjectList contacts READ contacts);
    QObjectList contacts();

signals:
    void changed();

    void contactAdded(QObject *);
    void contactRemoved(QObject *);

protected:
    // Functions called from OriginSocialProxy
    void unfilteredContactAddedOrChanged(RemoteUserProxy*, bool initalAdd = false);
    void unfilteredContactRemoved(RemoteUserProxy*);

    Q_INVOKABLE void emitDelayedChanged();
    void markViewChanged();

protected:
    virtual bool contactIncludedInView(Chat::RemoteUser*) = 0;

    QSet<RemoteUserProxy*> m_currentMembers;
    bool m_delayedChangedQueued;
};

}
}
}

#endif
