#ifndef _ONLINECONTACTCOUNTER_H
#define _ONLINECONTACTCOUNTER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Chat
{
    class Roster;
    class RemoteUser;
}

namespace Engine
{
namespace Social
{

class SocialController;

///
/// \brief Helper class for counting the number of online contacts the current user has
///
/// This uses a cached, lazily updated value. While this value is eventually consistent it might not be accurate if
/// actual value has recently changed.
/// 
class ORIGIN_PLUGIN_API OnlineContactCounter : public QObject
{
    friend class SocialController;
    Q_OBJECT
public:
    /// \brief Returns the cached number of online contacts for the current user
    unsigned int onlineContactCount() const
    {
        return mCachedOnlineContactCount;
    }

signals:
    /// \brief Emitted whenever the cached number of online contacts changes
    void onlineContactCountChanged(unsigned int count);

private slots:
    void connectContactSignals(Origin::Chat::RemoteUser *);
    void disconnectContactSignals(Origin::Chat::RemoteUser *);
    
    void asyncUpdate();

private:
    OnlineContactCounter(Chat::Roster *);

    Q_INVOKABLE void updateNow();
    
    Chat::Roster *mRoster;
    unsigned int mCachedOnlineContactCount;
    bool mUpdateQueued;
};

}
}
}

#endif

