var xmppObjects = require("./xmppobjects.js");
var ltx = require('ltx');
var fs = require('fs');

var XMPPFriendStub = function() {

    var isTcpSocket = 0;
    var gFakeUserOnClient = null;

    this.usingTcpSocket = function() {
        isTcpSocket = 1;
    }

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

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendMessage');
    };

    this.sendAccept = function(ws, jssdkuser, friend, rsttigID) {
        console.log('>> sendAccept');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                type : 'subscribed',
                to : jssdkuser.jid
            });

            ws.write(m.root().toString());

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

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendAccept');
    };

    this.sendReject = function(ws, jssdkuser, friend, rsttigID) {
        console.log('>> sendReject');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                type : 'unsubscribed',
                to : jssdkuser.jid
            });

            ws.write(m.root().toString());

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

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendReject');
    };

    this.sendFriendRequest = function(ws, jssdkuser, friend) {
        console.log('>> sendFriendRequest');
        console.log('>> generateFakeAtomUser');

        this.generateFakeAtomUser(jssdkuser.jid.split('@')[0], friend.jid.split('@')[0]);

        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                type : 'subscribe',
                to : jssdkuser.jid
            });

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendFriendRequest');
    };

    this.sendFriendRequestRevoke = function(ws, jssdkuser, friend, rsttigID) {
        console.log('>> sendFriendRequestRevoke');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']' + rsttigID);
            // unsubscribed
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                type : 'unsubscribe',
                to : jssdkuser.jid
            });

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }

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

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
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
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                type : 'unsubscribed',
                to : jssdkuser.jid
            });

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }

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

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendFriendRemove');
    };

    this.sendPresence = function(ws, jssdkuser, friend) {
        console.log('>> sendPresence');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                type : friend.presence.type,
                to : jssdkuser.jid
            });

            if (friend.presence.show) {
                m.c('show').t(friend.presence.show);
            }

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendPresence');
    };

    this.sendInvisible = function(ws, jssdkuser, friend) {
        console.log('>> sendInvisible');
        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                to : jssdkuser.jid,
                xmlns : 'jabber:client',
                type : 'unavailable'
            }).c('c', {
                hash : 'sha-1',
                node : 'http://www.origin.com/origin',
                xmlns : 'http://jabber.org/protocol/caps',
                ver : new Date().getTime()
            });

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendInvisible');
    };

    this.sendGameChangePresence = function(ws, jssdkuser, friend) {
        console.log('>> sendGameChangePresence');

        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                to : jssdkuser.jid
            });

            var gamePresenceText = friend.getCurrentGameTitle()+';'+friend.getCurrentGameOfferId()+';'+friend.getCurrentGameState()+';';
            if ( friend.getCurrentTwitchPresence() ) {
                gamePresenceText+= friend.getCurrentTwitchPresence();
            }
            gamePresenceText +=";;;;";
            var currentPlayingShaKey = new Date().getTime();
            friend.setCurrentShaKeyToPlay( currentPlayingShaKey );

            m.c('status').t(friend.getCurrentGameTitle());
            m.c('activity', {
                xmlns : 'http://jabber.org/protocol/activity'
            }).c('relaxing', {
                xmlns : 'http://jabber.org/protocol/activity'
            }).c('gaming', {
                xmlns : 'http://jabber.org/protocol/activity'
            }).up().up().c('text', {
                xmlns : 'http://jabber.org/protocol/activity'
            }).t(gamePresenceText);
            m.c('c', {
                xmlns : 'http://jabber.org/protocol/caps',
                node : 'http://www.origin.com/origin',
                hash : 'sha-1',
                ver : currentPlayingShaKey
            });

            if (isTcpSocket == 1) {
                ws.write(m.root().toString());
            } else {
                ws.send(m.root().toString());
            }
        }
        console.log('<< sendGameChangePresence');
    };

    this.sendGameStoppedPlaying = function(ws, jssdkuser, friend) {
        console.log('>> sendGameStoppedPlaying');

        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('presence', {
                from : friend.jid + '/mockup',
                xmlns : 'jabber:client',
                to : jssdkuser.jid
            });

            if ( friend.getCurrentShaKeyToPlay() ) {
                m.c('c', {
                xmlns : 'http://jabber.org/protocol/caps',
                node : 'http://www.origin.com/origin',
                hash : 'sha-1',
                ver : friend.getCurrentShaKeyToPlay()});

                if (isTcpSocket == 1) {
                    ws.write(m.root().toString());
                } else {
                    ws.send(m.root().toString());
                }

                // Friend information will be cleared up at the
                // Command Server level
            } else {
                // Do not send, the friend has not started playing yet
                console.log('Friend: '+friend.jid+' was not playing at all');
            }
        }
        console.log('<< sendGameStoppedPlaying');
    };

    this.sendJoinGameRequest = function(ws, jssdkuser, friend) {
        console.log('>> sendJoinGameRequest');

        if (ws && jssdkuser && friend) {
            console.log('[' + jssdkuser.jid + '][' + friend.jid + ']');
            var m = new ltx.Element('message', {
                type : 'ebisu-invite',
                from : friend.jid,
                xmlns : 'jabber:client',
                to : jssdkuser.jid,
                id : new Date().getTime()
            });

            var gamePresenceText = friend.getCurrentGameOfferId()+';multiplay_auto;'+new Date().getTime();
            if ( friend.getCurrentShaKeyToPlay() ) {
                m.c('body').t('This message requires Origin to view');
                m.c('subject').t(gamePresenceText);
                m.c('thread').t(new Date().getTime());
                m.c('active', {
                    xmlns : 'htttp://jabber.org/protocol/chatstates'
                });

                if (isTcpSocket == 1) {
                    ws.write(m.root().toString());
                } else {
                    ws.send(m.root().toString());
                }
            } else {
                // Do not send, the friend has not started playing yet
                console.log('Friend: '+friend.jid+' was not playing at all');
            }
        }
        console.log('<< sendJoinGameRequest');
    };

    this.generateFakeAtomUser = function(userOnClient, fakeFriend) {
        if (isTcpSocket) {
            // Generate the atom user Id file
            var filePath = 'data/'+userOnClient+'/atom/atomUserInfoByUserIds.xml';
            var fakexml = '<users><user><userId>'+fakeFriend+'</userId><personaId>'+fakeFriend+'</personaId><EAID>'+fakeFriend+'EA</EAID></user></users>';
            console.log('FakeXML: '+fakexml);
            fs.writeFile(filePath, fakexml, 'utf8');
        } else {
            // Do nothing if purely for JSSDK test
        }
    };
};

module.exports = XMPPFriendStub;
