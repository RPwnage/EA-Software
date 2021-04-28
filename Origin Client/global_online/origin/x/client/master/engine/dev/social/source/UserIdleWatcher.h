#ifndef _USERIDLEWATCHER_H
#define _USERIDLEWATCHER_H

#include <QObject>

namespace Origin
{
namespace Engine
{
namespace Social
{

class ConnectedUserPresenceStack;

class UserIdleWatcher : public QObject
{
    Q_OBJECT
public:
    UserIdleWatcher(ConnectedUserPresenceStack *presenceStack, QObject *parent = NULL);

private slots:
    void checkIdle();

private:
    ConnectedUserPresenceStack *mPresenceStack;
};

}
}
}

#endif

