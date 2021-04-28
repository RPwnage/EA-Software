var xmppObjects = require("./xmppobjects.js");
var ltx = require('ltx');

var XMPPFriendStub = function() {

    this.sendMessage = function(ws, jssdkuser, friend, msg) {
        console.log('>> sendMessage');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('message', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                type : 'chat',
                to : jssdkuser.jid,
                id : new Date().getTime()
            }).c('body').t(msg).up().c('thread').t(new Date().getTime()).up().c('active', {
                xmlns : 'http://jabber.org/protocol/chatstates'
            });

            ws.send(m.root().toString());
        }
        console.log('<< sendMessage');
    };

    this.sendAccept = function(ws, jssdkuser, friend, rsttigID) {
        console.log('>> sendAccept');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            var m = new ltx.Element('presence', {
                from : friend.jid,
                xmlns : 'jabber:client',
                type : 'subscribed',
                to : jssdkuser.jid
            });

            ws.send(m.root().toString());

            m = new ltx.Element('iq', {
                id : rsttigID,
                xmlns : 'jabber:client',
                type : 'set',
                to : jssdkuser.jid
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
            });

            ws.send(m.root().toString());
        }
        console.log('<< sendAccept');
    };

    this.sendReject = function(ws, jssdkuser, friend, rsttigID) {
        console.log('>> sendReject');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            var m = new ltx.Element('presence', {
                from : friend.jid,
                xmlns : 'jabber:client',
                type : 'unsubscribed',
                to : jssdkuser.jid
            });

            ws.send(m.root().toString());

            m = new ltx.Element('iq', {
                id : rsttigID,
                xmlns : 'jabber:client',
                type : 'set',
                to : jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:roster',
                ver : new Date().getTime()
            }).c('item', {
                jid : friend.jid,
                subscription : 'remove'
            });

            ws.send(m.root().toString());
        }
        console.log('<< sendReject');
    };

    this.sendFriendRequest = function(ws, jssdkuser, friend) {
        console.log('>> sendFriendRequest');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid,
                xmlns : 'jabber:client',
                type : 'subscribe',
                to : jssdkuser.jid
            });

            ws.send(m.root().toString());
        }
        console.log('<< sendFriendRequest');
    };

    this.sendFriendRequestRevoke = function(ws, jssdkuser, friend, rsttigID) {
        console.log('>> sendFriendRequestRevoke');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            // unsubscribed
            var m = new ltx.Element('presence', {
                from : friend.jid,
                xmlns : 'jabber:client',
                type : 'unsubscribe',
                to : jssdkuser.jid
            });

            ws.send(m.root().toString());

            m = new ltx.Element('iq', {
                id : rsttigID,
                xmlns : 'jabber:client',
                type : 'set',
                to : jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:roster',
                ver : new Date().getTime()
            }).c('item', {
                jid : friend.jid,
                subscription : 'remove'
            });

            ws.send(m.root().toString());
        }
        console.log('<< sendFriendRequestRevoke');
    };

    this.sendFriendRemove = function(ws, jssdkuser, friend, rsttigID) {
        console.log('>> sendFriendRemove');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);

            // set friend to offline
            friend.presence = xmppObjects.presences.OFFLINE;
            this.sendPresence(ws, jssdkuser, friend);

            // unsubscribed
            var m = new ltx.Element('presence', {
                from : friend.jid,
                xmlns : 'jabber:client',
                type : 'unsubscribed',
                to : jssdkuser.jid
            });

            ws.send(m.root().toString());

            // remove
            m = new ltx.Element('iq', {
                id : rsttigID,
                xmlns : 'jabber:client',
                type : 'set',
                to : jssdkuser.jid
            }).c('query', {
                xmlns : 'jabber:iq:roster',
                ver : new Date().getTime()
            }).c('item', {
                jid : friend.jid,
                subscription : 'remove'
            });

            ws.send(m.root().toString());
        }
        console.log('<< sendFriendRemove');
    };

    this.sendPresence = function(ws, jssdkuser, friend) {
        console.log('>> sendPresence');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid,
                xmlns : 'jabber:client',
                type : friend.presence.type,
                to : jssdkuser.jid
            });

            if (friend.presence.show) {
                m.c('show').t(friend.presence.show);
            }

            ws.send(m.root().toString());
        }
        console.log('<< sendPresence');
    };
};

module.exports = XMPPFriendStub;
