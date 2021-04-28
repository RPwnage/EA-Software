#ifndef _SUMMONEDTOGROUPCHANNELFLOW_H
#define _SUMMONEDTOGROUPCHANNELFLOW_H

#include <QObject>

#include <engine/login/User.h>

#include "chat/ConnectedUser.h" 
#include "chat/ChatGroup.h"
#include "chat/ChatChannel.h"

#include "services/plugin/PluginAPI.h"


#include "Qt/originwindow.h"                                                                                            
#include "Qt/originwindowmanager.h"  


namespace Origin
{
namespace Client
{

///
class ORIGIN_PLUGIN_API SummonedToGroupChannelFlow : public QObject
{
    Q_OBJECT
public:
    explicit SummonedToGroupChannelFlow(Engine::UserRef user, const QString& from, const QString& groupId, const QString& channelId);

    ///
    /// Starts the flow
    ///
    void start();

    // 
    QString getGroupId() { return mGroupId; }
    QString getChannelId() { return mChannelId; }

    void updateFrom(const QString& from);
signals:
    void handled();
    void succeeded();
    void failed();

private slots:
    void onGroupAdded(Origin::Chat::ChatGroup* group);
    void onGroupInviteAcceptSuccess(Chat::ChatGroup* group=NULL);
    void onGroupInviteAcceptFailed();
    void onGroupChannelInformationReceived();
    void onGroupChannelGetInfoFailed();
    void onGroupRemoved(Origin::Chat::ChatGroup* group);
    void onAccept();
    void onIgnore();
    void startJoinChannel(const bool& success);
    void onShowInviteAlertDialog(const QString&);
    void closeDialog();

private:
    void promptUserForGroup(Chat::ChatGroup* group);
    void onError();

    Chat::ChatGroups* chatGroups() const;

    Engine::UserRef mUser;
    QString mGroupId;
    QString mChannelId;
    QString mFromUserId;
    QString mFromUsername;

    Chat::ChatChannel* mChannelToJoin;
    Origin::UIToolkit::OriginWindow *mDialog;
};

}
}

#endif

