#ifndef _ROSTERPROXY_H
#define _ROSTERPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include "AbstractRosterView.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Chat
{
    class Roster;
}

namespace Client
{
namespace JsInterface
{

///
/// View of the complete roster
///
class ORIGIN_PLUGIN_API RosterProxy : public AbstractRosterView
{
    Q_OBJECT
public:
    RosterProxy(Chat::Roster *);

    Q_INVOKABLE void removeContact(QObject *contact);

    Q_PROPERTY(bool hasFriends READ hasFriends);
    bool hasFriends();

    Q_PROPERTY(bool hasLoaded READ hasLoaded);
    bool hasLoaded();

signals:
    void loaded();
    void anyUserChange();

private:
    bool contactIncludedInView(Chat::RemoteUser *)
    {
        return true;
    }

    Chat::Roster *mProxied;
};

}
}
}

#endif
