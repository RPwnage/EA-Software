var xmppObjects = require("./xmppobjects.js");
var ltx = require('ltx');
var fs = require('fs');

var XMPPJssdkUserStub = function() {

    this.jssdkuser = null;
    this.ws = null;
    this.tcpSocket = null;
    this.rsttigSeq = 1;
    // I assume auth shares the same seq

    this.setJssdkUser = function(user) {
        console.log('setJssdkUser: ' + user.jid);
        this.jssdkuser = user;
    };

    this.setWebSocket = function(webSocket) {
        console.log('setWebSocket');
        this.ws = webSocket;
    };

    this.getRsttigID = function() {
        return 'rsttig' + (++this.rsttigSeq);
    };

    this.setTcpSocket = function(tcpSocket) {
        console.log('setTcpSocket');
        this.tcpSocket = tcpSocket;
    }

    this.respondRosterRequest = function(friends, iqID) {
        console.log('>> respondRosterRequest');
        if (this.jssdkuser && friends) {
            console.log('[' + this.jssdkuser.jid + ']' + iqID);
            var m = new ltx.Element('iq', {
                id : iqID,
                xmlns : 'jabber:client',
                type : 'result',
                to : this.jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:roster'
            });

            var jids = friends.getFriendJIDs();
            for (var i = 0; i < jids.length; ++i) {
                var friend = friends.getFriendByJID(jids[i]);
                m.c('item', {
                    jid : friend.jid,
                    name : friend.name,
                    'origin:personaid' : friend.personaid,
                    'xmlns:origin' : 'origin:server',
                    'origin:eaid' : friend.eaid,
                    ask : friend.ask,
                    subscription : friend.subscription
                }).c('group').t('Global');
            }

            if (this.ws) {
                this.ws.send(m.root().toString());
            } else if (this.tcpSocket) {
                this.tcpSocket.write(m.root().toString());
            }

        }
        console.log('<< respondRosterRequest');
    };

    this.respondBlockListRequest = function(blockedList, iqID) {
        console.log('>> respondBlockListRequest');
        if (this.jssdkuser) {
            console.log('[' + this.jssdkuser.jid + ']' + iqID);
            var m = new ltx.Element('iq', {
                id : iqID,
                xmlns : 'jabber:client',
                type : 'result',
                to : this.jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:privacy'
            }).c('list', {
                name : 'global'
            });

            var numBlocked = 1;
            for (var i = 0; i < blockedList.length; ++i) {
                m.c('item', {
                    value : blockedList[i],
                    order : numBlocked,
                    action : "deny",
                    type : "jid",
                });
                m.up();
            }

            if (this.ws) {
                this.ws.send(m.root().toString());
            } else if (this.tcpSocket) {
                this.tcpSocket.write(m.root().toString());
            }
        }
        console.log('<< respondBlockListRequest');
    };

    this.respondFriendRequest = function(friend, rsttigID) {
        console.log('>> respondFriendRequest');
        if (this.jssdkuser && friend) {
            console.log('[' + this.jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            var m = new ltx.Element('iq', {
                id : rsttigID,
                xmlns : 'jabber:client',
                type : 'set',
                to : this.jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:roster',
                ver : new Date().getTime()
            }).c('item', {
                jid : friend.jid,
                name : friend.name,
                'origin:personaid' : friend.personaid,
                'xmlns:origin' : 'origin:server',
                'origin:eaid' : friend.eaid,
                ask : friend.ask,
                subscription : friend.subscription
            }).c('group').t('Global');

            if (this.ws) {
                this.ws.send(m.root().toString());
            } else if (this.tcpSocket) {
                this.tcpSocket.write(m.root().toString());
            }
        }
        console.log('<< respondFriendRequest');
    };

    this.respondFriendRequestAccept = function(friend, rsttigID) {
        console.log('>> respondFriendRequestAccept');
        if (this.jssdkuser && friend) {
            console.log('[' + this.jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            var m = new ltx.Element('iq', {
                id : rsttigID,
                xmlns : 'jabber:client',
                type : 'set',
                to : this.jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:roster',
                ver : new Date().getTime()
            }).c('item', {
                jid : friend.jid,
                name : friend.name,
                'origin:personaid' : friend.personaid,
                'xmlns:origin' : 'origin:server',
                'origin:eaid' : friend.eaid,
                subscription : friend.subscription
            }).c('group').t('Global');

            if (this.ws) {
                this.ws.send(m.root().toString());
            } else if (this.tcpSocket) {
                this.tcpSocket.write(m.root().toString());
            }
        }
        console.log('<< respondFriendRequestAccept');
    };

    this.respondFriendRequestRevoke = function(friend, rsttigID) {
        console.log('>> respondFriendRequestRevoke');
        if (this.jssdkuser && friend) {
            console.log('[' + this.jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            var m = new ltx.Element('iq', {
                id : rsttigID,
                xmlns : 'jabber:client',
                type : 'set',
                to : this.jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:roster',
                ver : new Date().getTime()
            }).c('item', {
                jid : friend.jid,
                subscription : 'remove'
            });

            this.removeFakeAtomUser(this.jssdkuser.jid.split('@')[0]);

            if (this.ws) {
                this.ws.send(m.root().toString());
            } else if (this.tcpSocket) {
                this.tcpSocket.write(m.root().toString());
            }
        }
        console.log('<< respondFriendRequestRevoke');
    };

    this.respondIQResult = function(iqID) {
        console.log('>> respondIQResult');
        if (this.jssdkuser) {
            console.log('[' + this.jssdkuser.jid + ']' + iqID);
            var m = new ltx.Element('iq', {
                id : iqID,
                xmlns : 'jabber:client',
                type : 'result',
                to : this.jssdkuser.jid
            });

            if (this.ws) {
                this.ws.send(m.root().toString());
            } else if (this.tcpSocket) {
                this.tcpSocket.write(m.root().toString());
            }
        }
        console.log('<< respondIQResult');
    };

    this.respondPresence = function(friend) {
        console.log('>> respondPresence');
        if (this.jssdkuser && friend) {
            console.log('[' + this.jssdkuser.jid + '][' + friend.jid + ']');

            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                to : this.jssdkuser.jid
            });

            if (this.ws) {
                m.c('show').t(friend.presence.show);
                this.ws.send(m.root().toString());
            } else if (this.tcpSocket) {
                m.c('show').t(friend.presence.show);
                this.tcpSocket.write(m.root().toString());
            }
        }
        console.log('<< respondPresence');
    };

    this.removeFakeAtomUser = function(userOnClient) {
        if (this.tcpSocket) {
            // Remove the generated file
            var filePath = 'data/'+userOnClient+'/atom/atomUserInfoByUserIds.xml';
            if ( fs.existsSync(filePath) ) {
                fs.chmod(filePath, 0777);
                fs.unlinkSync(filePath);
            }
        } else {
            // Do nothing, only JSSDK tests
        }
    };
};

module.exports = XMPPJssdkUserStub;
