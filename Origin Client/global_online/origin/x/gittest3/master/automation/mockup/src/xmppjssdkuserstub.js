var xmppObjects = require("./xmppobjects.js");
var ltx = require('ltx');

var XMPPJssdkUserStub = function() {

    this.jssdkuser = null;
    this.ws = null;
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

            this.ws.send(m.root().toString());
        }
        console.log('<< respondRosterRequest');
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

            this.ws.send(m.root().toString());
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

            this.ws.send(m.root().toString());
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

            this.ws.send(m.root().toString());
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

            this.ws.send(m.root().toString());
        }
        console.log('<< respondIQResult');
    };

    this.respondFriendPresence = function(friend) {
        console.log('>> respondFriendPresence');
        if (this.jssdkuser && friend) {
            console.log('[' + this.jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid,
                xmlns : 'jabber:client',
                to : this.jssdkuser.jid
            });

            this.ws.send(m.root().toString());
        }
        console.log('<< respondFriendPresence');
    };
};

module.exports = XMPPJssdkUserStub;
