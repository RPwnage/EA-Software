var xmppJssdkUserStub = require("./xmppjssdkuserstub.js");
var xmppFriendStub = require("./xmppfriendstub.js");
var xmppObjects = require("./xmppobjects.js");
var dataServer = require("./dataserver.js");
var express = require('express');
var bodyParser = require("body-parser");

// enable CORS
var allowCrossDomain = function(req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE,OPTIONS');
    res.header('Access-Control-Allow-Headers', req.headers['access-control-request-headers']);

    // intercept OPTIONS method
    if ('OPTIONS' == req.method) {
        res.sendStatus(200);
    } else {
        next();
    }
};

var app = express();
app.use(allowCrossDomain);
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies

module.exports = {

    run : function(proxy_port, xmpp_server) {

        // For testing purpose, this helper func always return a friend even the given jid is not in friend list.
        // This newly created friend is NOT added in friend list
        function getFriend(jid, friends) {
            if (jid && friends) {
			    var expectedDomain = xmpp_server.getExpectedDomain();
                var friend = friends.getFriendByJID(jid);
                if (!friend) {
                    var defaultName = jid.split('@')[0];
                    friend = new xmppObjects.Friend(defaultName+'@'+expectedDomain, defaultName, defaultName, defaultName);
                }

                return friend;
            } else {
                return null;
            }
        };

        app.post('/response/:action', function(req, res) {

            console.log('receiving ' + req.protocol + '://' + req.get('host') + req.originalUrl);
            console.log('request.body: ' + JSON.stringify(req.body));

            var respHeaders = {};
            var resp = {};

            var action = req.params.action;
            if ('set' === action) {
                resp = dataServer.inject(req.body.url, req.body.statusCode, req.body.headers, req.body.data);
            } else if ('reset' === action) {
                dataServer.reset();

                resp.respData = 'ok';
                resp.statusCode = 200;
            } else {
                resp.respData = 'unknown action "' + action + '"';
                resp.statusCode = 400;
            }

            respHeaders['Content-Length'] = resp.respData.length;
            respHeaders['Access-Control-Allow-Origin'] = '*';

            res.writeHead(resp.statusCode, respHeaders);
            res.write(resp.respData);
            res.end();

        });

        app.post('/xmpp/:pid/:jid/:action', function(req, res) {

            console.log('receiving ' + req.protocol + '://' + req.get('host') + req.originalUrl);
            console.log('request.body: ' + JSON.stringify(req.body));

            var resp = {};
            resp['pid'] = req.params.pid;
            resp['jid'] = req.params.jid;
            resp['action'] = req.params.action;

            var pid = req.params.pid;
            var jid = req.params.jid;
            var action = req.params.action;

            // get connected jssdkuser's socket.
            var conn = xmpp_server.getConn();
			
            if (conn) {
                var socket = conn.socket;
                var jssdkuser = conn.stub.jssdkuser;
                var friends = conn.friends;

                var friend = getFriend(jid, friends);
                if (friend) {
                    var stub = new xmppFriendStub();
					stub.usingTcpSocket();
					
                    if (action === 'sendMessage') {
                        var message = 'default mockup message';
                        if (req.body.hasOwnProperty('message')) {
                            message = req.body.message;
                            resp['message'] = message;
                        }
                        stub.sendMessage(socket, jssdkuser, friend, message);
                    } else if (action === 'sendAccept') {
                        stub.sendAccept(socket, jssdkuser, friend, conn.stub.getRsttigID());
                        friend.subscription = xmppObjects.subscriptions.BOTH;
                        friend.ask = xmppObjects.asks.NULL;
                        friends.add(friend);
                    } else if (action === 'sendReject') {
                        stub.sendReject(socket, jssdkuser, friend, conn.stub.getRsttigID());
                        friends.removeFriendByJID(jid);
                    } else if (action === 'sendFriendRequest') {
                        stub.sendFriendRequest(socket, jssdkuser, friend);
                        friends.add(friend);
                    } else if (action === 'sendFriendRequestRevoke') {
                        stub.sendFriendRequestRevoke(socket, jssdkuser, friend, conn.stub.getRsttigID());
                        friends.removeFriendByJID(jid);
                    } else if (action === 'sendFriendRemove') {
                        stub.sendFriendRemove(socket, jssdkuser, friend, conn.stub.getRsttigID());
                        friends.removeFriendByJID(jid);
                    } else if (action === 'sendPresence') {
                        if (req.body.hasOwnProperty('presence')) {
                            var presence = xmppObjects.presences[req.body.presence];
                            if (presence) {
                                friend.presence = presence;
                            } else {
                                resp['error'] = 'unknown presence: ' + req.body.presence;
                            }
                            resp['presence'] = friend.presence;
                        }
                        stub.sendPresence(socket, jssdkuser, friend);
                    } else if (action === 'sendInvisible') {
                        stub.sendInvisible(socket, jssdkuser, friend);
                    } else if (action === 'sendGameChangePresence') {
						// Must have gameTitle/Offer/State, or this is invalid
						if (req.body.hasOwnProperty('gameTitle') && req.body.hasOwnProperty('offerId') && req.body.hasOwnProperty('gameState') ) {
							friend.setCurrentGameTitle(req.body.gameTitle);
							friend.setCurrentGameOfferId(req.body.offerId);
							friend.setCurrentGameState(req.body.gameState);
							if ( req.body.hasOwnProperty('twitchPresence') ) {
								friend.setCurrentTwitchPresence(req.body.twitchPresence);
							}
							stub.sendGameChangePresence(socket, jssdkuser, friend);
						} else {
							resp['error'] = 'Missing Mandatory Parameters for Game Presence Change';
						}
				    } else if (action === 'sendGameStoppedPlaying') {
						if (friend.getCurrentGameTitle()) {
							// Friend was playing something
							stub.sendGameStoppedPlaying(socket, jssdkuser, friend);
							friend.stopPlaying();
						} else {
							resp['error'] = 'Friend did not started playing anything! Nothing to Stopped';
						}
				    } else if (action === 'sendJoinGameRequest') {
						if (friend.getCurrentGameTitle()) {
							// Friend was playing something
							stub.sendJoinGameRequest(socket, jssdkuser, friend);
						} else {
							resp['error'] = 'Friend did not started playing anything! Cannot ask to join game';
						}
				    } else {
                        res.status(400);
                        resp['error'] = 'unknown action: ' + action;
                    }
                } else {
                    res.status(400);
                    resp['error'] = 'unhandled jid: ' + jid;
                }
            } else {
                res.status(400);
                resp['error'] = 'no connection';
            }

            res.json(resp);
        });
		
		app.post('/xmpp/:pid/:action', function(req, res) {

            console.log('receiving ' + req.protocol + '://' + req.get('host') + req.originalUrl);
            console.log('request.body: ' + JSON.stringify(req.body));

            var resp = {};
            resp['pid'] = req.params.pid;
            resp['action'] = req.params.action;

            var pid = req.params.pid;
            var action = req.params.action;

            // get connected jssdkuser's socket.
            var conn = xmpp_server.getConn();
			
            if (conn) {
				 if (action === 'sendRandomPresence') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends')) {
						xmpp_server.randomPresence(req.body.startFriend, req.body.numOfFriends);
					} else {
						resp['error'] = 'Missing Parameters to start Random Presence';
					}
				} else if (action === 'sendRandomMessage') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends')) {
						xmpp_server.randomMessage(req.body.startFriend, req.body.numOfFriends);
					} else {
						resp['error'] = 'Missing Parameters to start Random Message';
					}
				} else if (action === 'sendRandomGamePresence') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends') && req.body.hasOwnProperty('gameTitle') && req.body.hasOwnProperty('offerId')) {
						xmpp_server.randomGamePresence(req.body.startFriend, req.body.numOfFriends, req.body.gameTitle, req.body.offerId);
					} else {
						resp['error'] = 'Missing Parameters to start Random Game Presence';
					}
				} else if (action === 'sendMassJoinRequest') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends') && req.body.hasOwnProperty('gameTitle') && req.body.hasOwnProperty('offerId')) {
						xmpp_server.massJoinRequest(req.body.startFriend, req.body.numOfFriends, req.body.gameTitle, req.body.offerId);
					} else {
						resp['error'] = 'Missing Parameters to start Mass Join Request';
					}
				} else if (action === 'sendRandomMassPresence') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends')) {
						if ( req.body.hasOwnProperty('msInterval')) {
							xmpp_server.randomMassPresence(req.body.startFriend, req.body.numOfFriends, req.body.msInterval);
						} else {
						    xmpp_server.randomMassPresence(req.body.startFriend, req.body.numOfFriends, 2000);
						}
					} else {
						resp['error'] = 'Missing Parameters to start Random Mass Presence';
					}
				} else if (action === 'sendRandomMassMessage') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends')) {
					    if ( req.body.hasOwnProperty('msInterval')) {
							xmpp_server.randomMassMessage(req.body.startFriend, req.body.numOfFriends, req.body.msInterval);
						} else {
						    xmpp_server.randomMassMessage(req.body.startFriend, req.body.numOfFriends, 4000);
						}
					} else {
						resp['error'] = 'Missing Parameters to start Random Mass Message';
					}
				} else if (action === 'sendRandomMassGamePresence') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends') && req.body.hasOwnProperty('gameTitle') && req.body.hasOwnProperty('offerId')) {
						if ( req.body.hasOwnProperty('msInterval')) {
							xmpp_server.randomMassGamePresence(req.body.startFriend, req.body.numOfFriends, req.body.gameTitle, req.body.offerId, req.body.msInterval);
						} else {
						    xmpp_server.randomMassGamePresence(req.body.startFriend, req.body.numOfFriends, req.body.gameTitle, req.body.offerId, 5000);
						}
					} else {
						resp['error'] = 'Missing Parameters to start Random Mass Game Presence';
					}
				} else if (action === 'sendRandomMassCombineMessage') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends')) {
						if ( req.body.hasOwnProperty('msInterval')) {
							xmpp_server.randomMassCombineMessage(req.body.startFriend, req.body.numOfFriends, req.body.msInterval);
						} else {
						    xmpp_server.randomMassCombineMessage(req.body.startFriend, req.body.numOfFriends, 15000);
						}
					} else {
						resp['error'] = 'Missing Parameters to start Random Combine Message';
					}
				} else if (action === 'sendRandomMassCombineGameMessage') {
					if (req.body.hasOwnProperty('startFriend') && req.body.hasOwnProperty('numOfFriends') && req.body.hasOwnProperty('gameTitle') && req.body.hasOwnProperty('offerId')) {
						if ( req.body.hasOwnProperty('msInterval')) {
							xmpp_server.randomMassCombineGameMessage(req.body.startFriend, req.body.numOfFriends, req.body.gameTitle, req.body.offerId, req.body.msInterval);
						} else {
						    xmpp_server.randomMassCombineGameMessage(req.body.startFriend, req.body.numOfFriends, req.body.gameTitle, req.body.offerId, 15000);
						}
					} else {
						resp['error'] = 'Missing Parameters to start Random Combine Game Message';
					}
				} else if (action === 'sendStopJobRequest') {
					xmpp_server.stopJobs();
				} else {
					res.status(400);
					resp['error'] = 'unknown action: ' + action;
				}
            } else {
                res.status(400);
                resp['error'] = 'no connection';
            }

            res.json(resp);
        });

        app.listen(process.env.PORT || proxy_port, function() {
            console.log('Local command server running at port: ' + proxy_port);
        });
    }

};
