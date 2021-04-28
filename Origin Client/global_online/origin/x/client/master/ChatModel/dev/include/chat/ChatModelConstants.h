// ChatModelConstants.h
// Copyright 2013 - Electronic Arts Inc.

#ifndef _CHATMODEL_CONSTANTS_H
#define _CHATMODEL_CONSTANTS_H

namespace Origin
{
namespace Chat
{

enum ChatState
{
    ChatStateNone,
    ChatStateActive,
    ChatStateInactive,
    ChatStateGone,
    ChatStateComposing,
    ChatStatePaused
};

/// \brief Internal message id constants used to get messages to execute on the Jingle XMPP thread.
enum
{
    MSG_CHATSEND = 1001,
    
    // MUC tasks
    MSG_MUCINVITESEND,
    MSG_MUCDECLINESEND,
    MSG_MUCINSTANTROOMREQUEST,
    MSG_MUCCONFIGREQUEST,
    MSG_MUCCONFIGSUBMISSION,
    MSG_MUCGRANTMODERATOR,
    MSG_MUCREVOKEMODERATOR,
    MSG_MUCGRANTADMIN,
    MSG_MUCREVOKEADMIN,
    MSG_MUCGRANTMEMBERSHIP,
    MSG_MUCREVOKEMEMBERSHIP,
    MSG_MUCGRANTVOICE,
    MSG_MUCREVOKEVOICE,
    MSG_MUCKICKOCCUPANT,
    
    // PRIVACY tasks
    MSG_PRIVACYBLOCKLISTSEND,
    MSG_PRIVACYBLOCKLISTREQUEST,
    MSG_PRIVACYSETPRIVACYLIST,

    //ROSTER tasks
    MSG_ACCEPTFRIENDREQUEST,
    MSG_CANCELFRIENDREQUEST,
    MSG_REJECTFRIENDREQUEST,
    MSG_SENDFRIENDREQUEST,
    MSG_REMOVEFRIEND,
    MSG_REQUESTROSTER,
    MSG_SETPRESENCE,
    MSG_SETDIRECTEDPRESENCE,

    MSG_CHATGROUPROOMINVITE,

    MSG_CARBONS_REQUEST
};

}
}

#endif // _CHATMODEL_CONSTANTS_H
