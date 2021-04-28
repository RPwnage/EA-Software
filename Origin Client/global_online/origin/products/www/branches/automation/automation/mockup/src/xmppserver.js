var http = require('http');
var xmppObjects = require("./xmppobjects.js");
var xmppJssdkUserStub = require("./xmppjssdkuserstub.js");
var localAddress = '127.0.0.1';
var connPool = {};

var gFriends = [];
var gMessageFriends = [];
var gUsername = '';


module.exports = {

    randomPresence: function() {
        var interval = setInterval(function(){
            if(gFriends.length > 0) {

                var randomFriend = gFriends[ parseInt(Math.random() * (gFriends.length-1)) ];

                var postData = '{"presence" : "'+["AWAY", "ONLINE", "OFFLINE"][parseInt(Math.random() * 2)]+'"}';

                var options = {
                  hostname: '192.168.1.100',
                  port: 1402,
                  path: '/xmpp/' + gUsername + '/' + randomFriend.jid + '/sendPresence',
                  method: 'POST',
                  headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': postData.length
                  }
                };

                var req = http.request(options, function(res) {
                  console.log('STATUS: ' + res.statusCode);
                  console.log('HEADERS: ' + JSON.stringify(res.headers));
                  res.setEncoding('utf8');
                  res.on('data', function (chunk) {
                    console.log('BODY: ' + chunk);
                  });
                });

                req.on('error', function(e) {
                  console.log('problem with request: ' + e.message);
                });

                // write data to request body
                req.write(postData);
                req.end();


            } else {
                console.log('no friends');
            }
        }, 1000);
    },

    randomMessage: function() {
        var interval = setInterval(function(){
            if(gFriends.length > 0) {

                var randomFriend = gFriends[ parseInt(Math.random() * (gFriends.length-1)) ];
                if (gMessageFriends.length < 10) {
                    gMessageFriends.push(randomFriend);
                } else {
                    randomFriend = gMessageFriends[ parseInt(Math.random() * (gMessageFriends.length-1)) ];

                }

                var postData = '{"message" : "'+["Ever notice that anyone going slower than you is an idiot, but anyone going faster is a maniac?", "When your only tool is a hammer, all problems start looking like nails.", "Artificial intelligence is no match for natural stupidity", "If #2 pencils are the most popular, are they still #2?", "All power corrupts. Absolute power is pretty neat, though."][parseInt(Math.random() * 5)]+'"}';

                var options = {
                  hostname: '192.168.1.100',
                  port: 1402,
                  path: '/xmpp/' + gUsername + '/' + randomFriend.jid + '/sendMessage',
                  method: 'POST',
                  headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': postData.length
                  }
                };

                var req = http.request(options, function(res) {
                  console.log('STATUS: ' + res.statusCode);
                  console.log('HEADERS: ' + JSON.stringify(res.headers));
                  res.setEncoding('utf8');
                  res.on('data', function (chunk) {
                    console.log('BODY: ' + chunk);
                  });
                });

                req.on('error', function(e) {
                  console.log('problem with request: ' + e.message);
                });

                // write data to request body
                req.write(postData);
                req.end();


            } else {
                console.log('no friends');
            }
        }, 5000);
    },

    run : function(proxy_port) {

        function Session() {
            this.id = null;
            this.subscribe = null;
            this.subscribed = null;
            this.unsubscribe = null;
            this.unsubscribed = null;
        };

        function Connection() {
            this.username = null;
            this.ws = null;
            this.stub = null;
            this.friends = null;
        };

        var connections = 0;
        var WebSocketServer = require('ws').Server;
        var wss = new WebSocketServer({
            port : proxy_port
        });

        console.log('Local xmpp server running at port: %d', proxy_port);

        wss.on('connection', function(ws) {

            console.log('connection count: %d', ++connections);

            for (var p in ws) {
                //console.log('%s : %s', p, ws[p]);
            }

            for (var p in ws._receiver._socket) {
                //console.log('%s : %s', p, ws._receiver._socket[p]);
            }

            for (var p in ws._sender._socket) {
                //console.log('%s : %s', p, ws._sender._socket[p]);
            }

            var sessions = {};
            var stub = null;
            var friends = null;
            var blockedList = [];
            var user = null;

            var step = 0;

            ws.on('open', function() {
                console.log('open');
            });

            ws.on('ping', function(data) {
                console.log('ping: %s', data);
            });

            ws.on('pong', function(data) {
                console.log('pong: %s', data);
            });

            ws.on('iq', function(data) {
                console.log('pong: %s', data);
            });

            ws.on('message', function(message) {

                console.log('on message: %s', message);

                if (message.indexOf('<stream:stream ') === 0) {

                    var res = '<?xml version="1.0"?>';
                    res += '<stream:stream ';
                    res += 'xmlns="jabber:client" ';
                    res += 'xmlns:stream="http://etherx.jabber.org/streams" ';
                    res += 'from="' + localAddress + '" ';
                    res += 'id="' + new Date().getTime() + '" ';
                    res += 'version="1.0" ';
                    res += 'xml:lang="en" ';
                    res += '>';
                    ws.send(res);

                    if (step === 0) {

                        step++;

                        var ltx = require('ltx');
                        var m = new ltx.Element('stream:features');
                        m.c('starttls', {
                            xmlns : 'urn:ietf:params:xml:ns:xmpp-tls',
                        });
                        m.c('ver', {
                            xmlns : 'urn:xmpp:features:rosterver'
                        });
                        m.c('mechanisms', {
                            xmlns : 'urn:ietf:params:xml:ns:xmpp-sasl'
                        }).c('mechanism').t('PLAIN').up().c('mechanism').t('ANONYMOUS');
                        m.c('register', {
                            xmlns : 'http://jabber.org/features/iq-register'
                        });
                        m.c('auth', {
                            xmlns : 'http://jabber.org/features/iq-auth'
                        });

                        ws.send(m.root().toString());

                    } else if (step === 1) {

                        step++;

                        var ltx = require('ltx');
                        var m = new ltx.Element('stream:features');
                        m.c('starttls', {
                            xmlns : 'urn:ietf:params:xml:ns:xmpp-tls',
                        });
                        m.c('ver', {
                            xmlns : 'urn:xmpp:features:rosterver'
                        });
                        m.c('session', {
                            xmlns : 'urn:ietf:params:xml:ns:xmpp-session'
                        });
                        m.c('register', {
                            xmlns : 'http://jabber.org/features/iq-register'
                        });
                        m.c('bind', {
                            xmlns : 'urn:ietf:params:xml:ns:xmpp-bind'
                        });

                        ws.send(m.root().toString());

                    } else {
                        console.log('received: %s at step %d', message, step);
                    }

                } else if (message.indexOf('<auth xmlns=') === 0) {

                    var ltx = require('ltx');
                    var m = new ltx.Element('success', {
                        xmlns : 'urn:ietf:params:xml:ns:xmpp-sasl'
                    });

                    ws.send(m.root().toString());

                } else if (message.indexOf('<iq ') === 0) {

                    var ltx = require('ltx');
                    var iq = ltx.parse(message);
                    console.log('iq: %s', iq);

                    if (iq.attrs.type === 'get' && iq.attrs.id === '_auth_1') {

                        // get username
                        var username = '';
                        var iqChildren = iq.getChildElements();
                        for (var i = 0; i < iqChildren.length; i++) {
                            if (iqChildren[i].name === 'query') {
                                var queryChildren = iqChildren[i].getChildElements();
                                for (var j = 0; j < queryChildren.length; j++) {
                                    if (queryChildren[j].name === 'username') {
                                        username = queryChildren[j].children[0];
                                        gUsername = username;
                                    }
                                }
                            }
                        }

                        // check if the same user has a connection
                        if (connPool[username]) {

                            // reject this connection because this account is connected already
                            var m = new ltx.Element('iq', {
                                from : localAddress,
                                xmlns : 'jabber:client',
                                type : 'error',
                                id : '_auth_1'
                            }).c('error', {
                                type : 'cancel'
                            }).c('feature-not-implemented', {
                                xmlns : 'urn:ietf:params:xml:ns:xmpp-stanzas'
                            }).up().c('text', {
                                xmlns : 'urn:ietf:params:xml:ns:xmpp-stanzas',
                                'xml:lang' : 'en'
                            }).t('Do not support multiple login');

                            ws.send(m.root().toString());

                            // disconnect
                            ws.close();

                        } else {

                            // create a new stub for this connection
                            stub = new xmppJssdkUserStub();
                            stub.setWebSocket(ws);

                            // disconnect if any exception occurs
                            try {
                                // get user info
                                http.get('http://127.0.0.1:1400/data/' + username + '/persona?', function(personaRes) {
                                    personaRes.setEncoding('utf8');
                                    personaRes.on("data", function(personaChunk) {
                                        var personas = JSON.parse(personaChunk);
                                        var persona = personas['personas']['persona'][0];
                                        user = new xmppObjects.Person(persona['pidId'] + '@' + localAddress, persona['name'], persona['personaId'], persona['displayName']);
                                        console.log('set jssdkuser: ' + user.jid);
                                        stub.setJssdkUser(user);

                                        friends = new xmppObjects.Friends();

                                        function response_auth_1() {
                                            // add connection to pool
                                            var conn = new Connection();
                                            conn.username = username;
                                            conn.ws = ws;
                                            conn.stub = stub;
                                            conn.friends = friends;
                                            connPool[username] = conn;

                                            var m = new ltx.Element('iq', {
                                                from : localAddress,
                                                xmlns : 'jabber:client',
                                                type : 'result',
                                                id : '_auth_1'
                                            }).c('query', {
                                                xmlns : 'jabber:iq:auth'
                                            }).c('username').up().c('password').up().c('token').up().c('resource');

                                            ws.send(m.root().toString());
                                        }

                                        // get friends
                                        http.get('http://127.0.0.1:1400/data/' + username + '/social/friends?', function(friendsRes) {
                                            console.log("Got response: " + friendsRes.statusCode);
                                            friendsRes.setEncoding('utf8');

                                            var buffer = '';
                                            friendsRes.on("data", function(friendsChunk) {
                                                buffer += friendsChunk;
                                            });

                                            friendsRes.on("end", function(friendsChunk) {
                                                var friendData = JSON.parse(buffer);
                                                for (var i = 0; i < friendData.length; ++i) {
                                                    var friend = new xmppObjects.Friend(friendData[i]['jid'], friendData[i]['name'], friendData[i]['personaid'], friendData[i]['eaid']);
                                                    if (friendData[i]['subscription']) {
                                                        friend.subscription = xmppObjects.subscriptions[friendData[i]['subscription']];
                                                    }
                                                    if (friendData[i]['presence']) {
                                                        friend.presence = xmppObjects.presences[friendData[i]['presence']];
                                                    }
                                                    if (friendData[i]['ask']) {
                                                        friend.ask = xmppObjects.asks[friendData[i]['ask']];
                                                    }
                                                    friends.add(friend);
                                                    gFriends.push(friend);
                                                    console.log('load friend: ' + friend.jid);
                                                }

                                                // all friends are loaded
                                                response_auth_1();
                                            });

                                        }).on('error', function(e) {
                                            console.log("Got error on retrieving " + username + " friends.json: " + e.message);
                                            // it is ok to have no friends
                                            response_auth_1();
                                        });

                                    });

                                }).on('error', function(e) {
                                    console.log("error on retrieving " + username + " persona.json: " + e.message);
                                    // disconnect because we must have persona to compose jid
                                    ws.close();
                                });

                            } catch(err) {
                                console.log(err);
                                // disconnect
                                ws.close();
                            }
                        }

                    } else if (iq.attrs.type === 'set' && iq.attrs.id === '_auth_2') {

                        var m = new ltx.Element('iq', {
                            xmlns : 'jabber:client',
                            type : 'result',
                            id : '_auth_2'
                        }).t('Authentication successful.');

                        ws.send(m.root().toString());

                    } else if (iq.attrs.type === 'get') {

                        var children = iq.getChildElements();
                        for (var i = 0; i < children.length; i++) {
                            if (children[i].name === 'query') {
                                if (children[i].attrs.xmlns === 'jabber:iq:roster') {
                                    // Origin.xmpp.requestRoster
                                    if (stub) {
                                        stub.respondRosterRequest(friends, iq.attrs.id);
                                    }
                                } else if (children[i].attrs.xmlns === 'jabber:iq:privacy') {
                                    // Origin.xmpp.loadBlockList
                                    if (stub) {
                                        stub.respondBlockListRequest(blockedList, iq.attrs.id);
                                    }
                                }
                            }
                        }

                    } else if (iq.attrs.type === 'set') {

                        if (iq.attrs.xmlns === 'jabber:iq:privacy') {
                            var children = iq.getChildElements();
                            for (var i = 0; i < children.length; i++) {
                                if (children[i].name === 'query') {
                                    var grandsons = children[i].getChildElements();
                                    for (var j = 0; j < grandsons.length; j++) {
                                        if (grandsons[j].name === 'list') {
                                            // All friends Unblocked by default
                                            blockedList = [];

                                            // Origin.xmpp.blockUser and Origin.xmpp.unblockUser
                                            var blockListItems = grandsons[j].getChildElements();
                                            for (var k = 0; k < blockListItems.length; k++) {
                                                blockedList.push(blockListItems[k].attrs.value);
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            var children = iq.getChildElements();
                            for (var i = 0; i < children.length; i++) {
                                if (children[i].name === 'query') {
                                    if (children[i].attrs.xmlns === 'jabber:iq:roster') {
                                        var grandsons = children[i].getChildElements();
                                        for (var j = 0; j < grandsons.length; j++) {
                                            if (grandsons[j].attrs.subscription === 'remove') {

                                                // Origin.xmpp.friendRemove
                                                var friend = friends.getFriendByJID(grandsons[j].attrs.jid);
                                                if (friend) {
                                                    friend.subscription = xmppObjects.subscriptions.NONE;
                                                    friend.ask = xmppObjects.asks.NULL;
                                                    if (stub) {
                                                        var session = new Session();
                                                        session.id = iq.attrs.id;

                                                        var rsttigID = stub.getRsttigID();
                                                        sessions[rsttigID] = session;

                                                        stub.respondFriendRequestRevoke(friend, rsttigID);
                                                    }
                                                    friends.removeFriendByJID(friend.jid);
                                                } else {
                                                    // if revoked friend is not in list, server ignores it
                                                }

                                            }
                                        }
                                    } else if (children[i].attrs.xmlns === 'jabber:iq:privacy') {
                                        var grandsons = children[i].getChildElements();
                                        for (var j = 0; j < grandsons.length; j++) {
                                            if (grandsons[j].name === 'active') {
                                                if (grandsons[j].attrs.name === 'invisible') {
                                                    stub.respondIQResult(iq.attrs.id);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                    } else if (iq.attrs.type === 'result') {

                        var session = sessions[iq.attrs.id];
                        if (session) {
                            if (stub) {
                                if (session.id) {
                                    stub.respondIQResult(session.id);
                                } else if (session.subscribe) {
                                    // server does nothing
                                } else if (session.subscribed) {
                                    var friend = friends.getFriendByJID(session.subscribed);
                                    if (friend) {
                                        stub.respondPresence(friend);
                                    }
                                } else if (session.unsubscribe) {
                                    // server does nothing
                                } else if (session.unsubscribed) {
                                    // server does nothing
                                } else {
                                    console.log('unhandled session: ' + session);
                                }
                            }

                            // remove session
                            sessions[iq.attrs.id] = null;
                        } else {
                            // doesn't recognize this iq.id, means this a session started by friend
                        }

                    }

                } else if (message.indexOf('<presence ') === 0) {

                    var ltx = require('ltx');
                    var presence = ltx.parse(message);
                    console.log('presence: %s', presence);

                    if (presence.attrs.type === 'subscribe') {
                        // Origin.xmpp.friendRequestSend

                        // find if passed in friend exists
                        var jid = presence.attrs.to;
                        var friend = friends.getFriendByJID(jid);
                        if (!friend) {
                            // create a Friend based on pass-in message if not exists
                            var defaultName = jid.split('@')[0];
                            friend = new xmppObjects.Friend(jid, defaultName, defaultName, defaultName);
                            // add the new friend to list
                            friends.add(friend);

                            friend.subscription = xmppObjects.subscriptions.NONE;
                            friend.ask = xmppObjects.asks.SUBSCRIBE;

                            if (stub) {
                                var session = new Session();
                                session.subscribe = presence.attrs.to;

                                var rsttigID = stub.getRsttigID();
                                sessions[rsttigID] = session;

                                stub.respondFriendRequest(friend, rsttigID);
                            }
                        } else {
                            // if subscribed friend is in list, server ignores it
                        }

                    } else if (presence.attrs.type === 'unsubscribe') {
                        // Origin.xmpp.friendRequestRevoke

                        var friend = friends.getFriendByJID(presence.attrs.to);
                        if (friend && friend.ask === xmppObjects.asks.SUBSCRIBE) {
                            friend.subscription = xmppObjects.subscriptions.NONE;
                            friend.ask = xmppObjects.asks.NULL;
                            if (stub) {
                                var session = new Session();
                                session.unsubscribe = presence.attrs.to;

                                var rsttigID = stub.getRsttigID();
                                sessions[rsttigID] = session;

                                stub.respondFriendRequestRevoke(friend, rsttigID);
                            }
                            friends.removeFriendByJID(friend.jid);
                        } else {
                            // if revoked friend is not in list, server ignores it
                        }

                    } else if (presence.attrs.type === 'subscribed') {
                        // Origin.xmpp.friendRequestAccept

                        var friend = friends.getFriendByJID(presence.attrs.to);
                        // only accept if a friend sent a request before
                        if (friend && friend.ask === xmppObjects.asks.NULL && friend.subscription === xmppObjects.subscriptions.NONE) {
                            friend.subscription = xmppObjects.subscriptions.BOTH;
                            friend.ask = xmppObjects.asks.NULL;
                            if (stub) {
                                var session = new Session();
                                session.subscribed = presence.attrs.to;

                                var rsttigID = stub.getRsttigID();
                                sessions[rsttigID] = session;

                                stub.respondFriendRequestAccept(friend, rsttigID);
                            }
                        } else {
                            // if accepted friend is not in list, server ignores it
                        }

                    } else if (presence.attrs.type === 'unsubscribed') {
                        // Origin.xmpp.friendRequestReject

                        var friend = friends.getFriendByJID(presence.attrs.to);
                        if (friend && friend.ask === xmppObjects.asks.NULL && friend.subscription === xmppObjects.subscriptions.NONE) {
                            friend.subscription = xmppObjects.subscriptions.NONE;
                            friend.ask = xmppObjects.asks.NULL;
                            if (stub) {
                                var session = new Session();
                                session.unsubscribed = presence.attrs.to;

                                var rsttigID = stub.getRsttigID();
                                sessions[rsttigID] = session;

                                // revoke and reject are responding the same message
                                stub.respondFriendRequestRevoke(friend, rsttigID);
                            }
                            friends.removeFriendByJID(friend.jid);
                        } else {
                            // if rejected friend is not in list, server ignores it
                        }

                    } else if (presence.attrs.type === undefined) {

                        if (presence.attrs.id === 'roomId') {
                            // Origin.xmpp.joinRoom

                        } else {

                            var children = presence.getChildElements();
                            for (var i = 0; i < children.length; i++) {
                                if (children[i].name === 'show') {
                                    // Origin.xmpp.requestPresence

                                    var show  = children[i].getText();
                                    if (stub) {
                                        user.presence = xmppObjects.presences[show];
                                        stub.respondPresence(user);
                                    }

                                }
                            }

                        }

                    } else if (presence.attrs.type === 'unavailable') {

                        var to = presence.attrs.to;
                        if (to) {
                            var friend = friends.getFriendByJID(to.substring(0, to.indexOf('/')));
                            if (friend) {
                                friend.presence = xmppObjects.presences.OFFLINE;
                                if (stub) {
                                    stub.respondPresence(friend);
                                }
                            } else {
                                if (user.jid = to.substring(0, to.indexOf('/'))) {
                                    // Origin.xmpp.leaveRoom
                                    // placeholder
                                }
                            }
                        }

                    } else {

                        // placeholder
                        console.log('presence.attrs.type=' + presence.attrs.type);

                    }

                } else if (message.indexOf('<message ') === 0) {

                    var ltx = require('ltx');
                    var msg = ltx.parse(message);
                    console.log('message: %s', msg);

                    if (msg.attrs.type === 'chat') {
                        // Origin.xmpp.sendMessage

                        var friend = friends.getFriendByJID(msg.attrs.to);
                        if (friend) {
                            var children = msg.getChildElements();
                            for (var i = 0; i < children.length; i++) {
                                if (children[i].name === 'body') {
                                    friend.recvMsg = children[i].children[0];
                                }
                            }
                        }
                    }

                } else {
                    //
                }

            });

            ws.on('close', function(code) {
                console.log('close: %s', code);

                for (var conn in connPool) {
                    if (ws === connPool[conn].ws) {
                        console.log('remove connection of %s', connPool[conn].username);
                        delete connPool[conn];
                    }
                }
            });

        });

    },

    getConnByUsername : function(username) {
        return connPool[username];
    }

};

