var http = require('http');
var xmppObjects = require("./xmppobjects.js");
var xmppJssdkUserStub = require("./xmppjssdkuserstub.js");
var localAddress = '127.0.0.1';

var gFriends = [];
var gUsername = '';
var gMachineName = '';
var connection = null;
var gIntervalIds = [];

// Load the TCP Library
net = require('net');

// Keep track of the chat clients
var clients = [];

// Start a TCP Server
net.createServer(function (socket) {

  // Identify this client
  socket.name = socket.remoteAddress + ":" + socket.remotePort 

  // Put this new client in the list
  clients.push(socket);
  
  var sessions = {};
  var stub = null;
  var friends = null;
  var blockedList = [];
  var user = null;
  var step = 0;
  
  function Connection() {
	this.username = null;
	this.socket = null;
	this.stub = null;
	this.friends = null;
  };
  
  function Session() {
	this.id = null;
	this.subscribe = null;
	this.subscribed = null;
	this.unsubscribe = null;
	this.unsubscribed = null;
  };


  // Handle incoming messages from clients.
  socket.on('data', function (data) {
    console.log('on message: %s', data);


    if (data.toString().indexOf('<stream:stream ') === 0) {
		
		// Parses incoming message for expected domain name
		// Should be the same for all users to same server
		var fromIndex = data.toString().indexOf('from=');
		if ( fromIndex === 0 ) {
		} else {
			var subStr = data.toString().substr(fromIndex+1);
			var startAddress = subStr.substr(subStr.indexOf('@')+1);
			localAddress = startAddress.substr(0, startAddress.indexOf('"'));
			console.log("Expected Domain: "+localAddress);
		}
		
		var res = '<?xml version="1.0"?>';
		res += '<stream:stream ';
		res += 'xmlns="jabber:client" ';
		res += 'xmlns:stream="http://etherx.jabber.org/streams" ';
		res += 'from="' + localAddress + '" ';
		res += 'id="' + new Date().getTime() + '" ';
		res += 'version="1.0" ';
		res += 'xml:lang="en" ';
		res += '>';
		socket.write(res);
		
		if (step === 0) {
			console.log("step 0 msg: "+data);

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

			socket.write(m.root().toString());

		} else if (step === 1) {
			console.log("step 1 msg: "+data);
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

			socket.write(m.root().toString());

		} else {
			console.log('received: %s at step %d', data, step);
		}
		
    } else if (data.toString().indexOf('<auth xmlns=') === 0) {

		var ltx = require('ltx');
		var m = new ltx.Element('success', {
			xmlns : 'urn:ietf:params:xml:ns:xmpp-sasl'
		});

		socket.write(m.root().toString());
	} else if (data.toString().indexOf('<pri:iq') === 0) {

		var ltx = require('ltx');
		var priiq = ltx.parse(data);
		console.log("pri:iq: "+data);
		
		var privacyId = priiq.attrs.id;
		var m = new ltx.Element('iq', {
				from : localAddress,
				xmlns : 'jabber:id:privacy',
				type : 'result',
				to : gUsername+'@'+localAddress+'/'+gMachineName,
				id : privacyId
			}).c('query', {
				xmlns : 'jabber:id:privacy'
			}).c('list', {
				name : 'global' });

		socket.write(m.root().toString());
	} else if (data.toString().indexOf('<ns:iq ') === 0) {
		var ltx = require('ltx');
		var nsiq = ltx.parse(data);
		console.log("ns:iq: "+data);
		
        if (nsiq.attrs.type === 'get') {
			var m = new ltx.Element('iq', {
				from : localAddress,
				xmlns : '',
				type : 'result',
				id : 'libxmpp0'
			}).c('query', {
				xmlns : 'jabber:iq:auth'
			}).c('username').up().c('password').up().c('token').up().c('resource');

			socket.write(m.root().toString());
		} else if (nsiq.attrs.type === 'set' ) {
			
			// Parse the user name here
			var username = '';
			var iqChildren = nsiq.getChildElements();
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
			
			// create a new stub for this connection
			stub = new xmppJssdkUserStub();
			stub.setTcpSocket(socket);

            // disconnect if any exception occurs
			try {
				// get user info
				http.get('http://127.0.0.1:1400/data/' + username + '/persona?', function(personaRes) {
					personaRes.setEncoding('utf8');
					personaRes.on("data", function(personaChunk) {
						var personas = JSON.parse(personaChunk);
						var persona = personas['personas']['persona'][0];
						user = new xmppObjects.Person(username + '@' + localAddress, persona['name'], persona['personaId'], persona['displayName']);
						console.log('set jssdkuser: ' + user.jid);
						stub.setJssdkUser(user);

						friends = new xmppObjects.Friends();

						function response_auth_1() {

							var m = new ltx.Element('iq', {
								from : localAddress,
								xmlns : 'jabber:client',
								type : 'result',
								id : '_auth_1'
							}).c('query', {
								xmlns : 'jabber:iq:auth'
							}).c('username').up().c('password').up().c('token').up().c('resource');

							socket.write(m.root().toString());
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
									var nid = friendData[i]['jid'].split('@')[0];
									
									var friend = new xmppObjects.Friend(nid+'@'+localAddress, friendData[i]['name'], friendData[i]['personaid'], friendData[i]['eaid']);
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
							});
							
							var m = new ltx.Element('iq', {
								xmlns : '',
								type : 'result',
								id : 'libxmpp1'
							}).t('Authentication successful.');
							
							socket.write(m.root().toString());
							
							var conn = new Connection();
							conn.username = username;
							conn.socket = socket;
							conn.stub = stub;
							conn.friends = friends;
							connection = conn;
								

						}).on('error', function(e) {
							console.log("Got error on retrieving " + username + " friends.json: " + e.message);
						});

					});

				}).on('error', function(e) {
					console.log("error on retrieving " + username + " persona.json: " + e.message);
					// disconnect because we must have persona to compose jid
					socket.close();
				});

			} catch(err) {
				console.log(err);
				// disconnect
				socket.close();
			}
		}
    } else if (data.toString().indexOf('<iq ') === 0) {
		var ltx = require('ltx');
		var iq = ltx.parse(data);
		console.log('iq: %s', iq);

		if (iq.attrs.type === 'get') {

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
				} else if (children[i].name === 'enable') {
					var m = new ltx.Element('iq', {
						xmlns : 'jabber:client',
						type : 'result',
						id : 'OriginCarbonsEnableRequest',
						to : gUsername+'@'+localAddress+'/'+gMachineName
					});

					socket.write(m.root().toString());
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
					if (children[i].name === 'ros:query') {
						if (children[i].attr('xmlns:ros') === 'jabber:iq:roster') {
							var grandsons = children[i].getChildElements();
							for (var j = 0; j < grandsons.length; j++) {
								if (grandsons[j].attrs.subscription === 'remove') {

									// Origin.xmpp.friendRemove
									var friend = friends.getFriendByJID(grandsons[j].attrs.jid);
									console.log('Friend Remove request: '+grandsons[j].attrs.jid);
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
					} else if (children[i].name === 'bind' ) {
						var machinename = '';
						var grandsons = children[i].getChildElements();
						for (var j = 0; j < grandsons.length; j++) {
							if (grandsons[j].name === 'resource') {
								machinename = grandsons[j].children[0];	
								gMachineName = machinename;
							}
						}
						
						var m = new ltx.Element('iq', {
							xmlns : 'jabber:client',
							type : 'result',
							id : '0',
							to: gUsername+'@'+localAddress+'/'+gMachineName
						}).c('bind', {
						xmlns : 'urn:ietf:params:xml:ns:xmpp-bind'}).c('jid').t(gUsername+'@'+localAddress+'/'+gMachineName);
						socket.write(m.root().toString());
					} else if (children[i].name === 'session') {
						var m = new ltx.Element('iq', {
							xmlns : 'jabber:client',
							type : 'result',
							id : '1',
							to: gUsername+'@'+localAddress+'/'+gMachineName
						});
						socket.write(m.root().toString());
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
	} else if (data.toString().indexOf('<presence ') === 0) {

		var ltx = require('ltx');
		var presence = ltx.parse(data);
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
	} else if (data.toString().indexOf('<message ') === 0) {

		var ltx = require('ltx');
		var msg = ltx.parse(data);
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

  // Remove the client from the list when it leaves
  socket.on('end', function () {
    clients.splice(clients.indexOf(socket), 1);
  });
}).listen(5222);

// Helper functions for below
function sendPresenceHelper(targetFriend, presence) {
	var postData = '{"presence" : "'+presence+'"}';
				
	var options = {
	  hostname: localAddress,
	  port: 1402,
	  path: '/xmpp/' + gUsername + '/' + targetFriend.jid + '/sendPresence',
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
};

function sendInvisibleHelper(targetFriend) {
	var options = {
	  hostname: localAddress,
	  port: 1402,
	  path: '/xmpp/' + gUsername + '/' + targetFriend.jid + '/sendInvisible',
	  method: 'POST'
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
	req.end();
};

function sendMessageHelper(targetFriend, message) {
	var postData = '{"message" : "'+message+'"}';

	var options = {
	  hostname: localAddress,
	  port: 1402,
	  path: '/xmpp/' + gUsername + '/' + targetFriend.jid + '/sendMessage',
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
};

function sendGamePresenceHelper(targetFriend, gameTitle, offerId, gamePresence) {
	var postData = '{"gameTitle":"'+gameTitle+'", "offerId":"'+offerId+'", "gameState":"'+gamePresence+'"}';

	var options = {
	  hostname: localAddress,
	  port: 1402,
	  path: '/xmpp/' + gUsername + '/' + targetFriend.jid + '/sendGameChangePresence',
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
};

function sendStopGamePlayedHelper(targetFriend) {
	var options = {
	  hostname: localAddress,
	  port: 1402,
	  path: '/xmpp/' + gUsername + '/' + targetFriend.jid + '/sendGameStoppedPlaying',
	  method: 'POST'
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

	req.end();
};

function sleep (time) {
  return new Promise((resolve) => setTimeout(resolve, time));
}

module.exports = {
    getConn : function() {
        return connection;
    },
	
	getExpectedDomain : function() {
		return localAddress;
	},
	
	randomPresence: function(startFriend, numOfFriend) {
		console.log('<< randomPresence');
		console.log('gFriendLegth: '+gFriends.length);
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {
                var randomFriend = gFriends[ startFriend+parseInt(Math.random() * (numOfFriend-1)) ];

                var targetPresence = ["AWAY", "ONLINE", "OFFLINE"][parseInt(Math.random() * 3)];
				
				if ( parseInt(Math.random() * 4) == 3 ) {
					sendInvisibleHelper(randomFriend);
				} else {
					sendPresenceHelper(randomFriend, targetPresence);
				}
            } else {
                console.log('You do not have enough friends');
            }
        }, 2000);
		gIntervalIds.push(interval);
    },

    randomMessage: function(startFriend, numOfFriend) {
		console.log('<< randomMessage');
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {

                var randomFriend = gFriends[ startFriend+parseInt(Math.random() * (numOfFriend-1)) ];

                var targetMessage = ["Ever notice that anyone going slower than you is an idiot, but anyone going faster is a maniac?", "When your only tool is a hammer, all problems start looking like nails.", "Artificial intelligence is no match for natural stupidity", "If #2 pencils are the most popular, are they still #2?", "All power corrupts. Absolute power is pretty neat, though."][parseInt(Math.random() * 5)];

                sendMessageHelper(randomFriend, targetMessage);

            } else {
                console.log('You do not have enough friends');
            }
        }, 4000);
		gIntervalIds.push(interval);
    },
	
	randomGamePresence: function(startFriend, numOfFriend, gameTitle, offerId) {
		console.log('<< randomGamePresence');
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {

			    var randomOpType = parseInt(Math.random() * 3 );
				
                var randomFriend = gFriends[ startFriend+parseInt(Math.random() * (numOfFriend-1)) ];
				
			    if ( randomOpType < 2 ) {
					var targetGamePresence = ["JOINABLE","INGAME"][parseInt(Math.random())];

					sendGamePresenceHelper(randomFriend, gameTitle, offerId, targetGamePresence);
				} else {
					sendStopGamePlayedHelper(randomFriend, gameTitle, offerId);
				}
            } else {
                console.log('You do not have enough friends');
            }
        }, 5000);
		gIntervalIds.push(interval);
    },
	
	massJoinRequest: function(startFriend, numOfFriend, gameTitle, offerId) {
		console.log('<< massJoinRequest');
        if(gFriends.length > startFriend+numOfFriend) {

		    for (var i = startFriend; i < startFriend+numOfFriend; i++) {
				var randomFriend = gFriends[ i ];
				var postData = '{"gameTitle":"'+gameTitle+'", "offerId":"'+offerId+'", "gameState":"INGAME"}';

				var options = {
				  hostname: localAddress,
				  port: 1402,
				  path: '/xmpp/' + gUsername + '/' + randomFriend.jid + '/sendGameChangePresence',
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
				
				// Join Game Request
				var options1 = {
				  hostname: localAddress,
				  port: 1402,
				  path: '/xmpp/' + gUsername + '/' + randomFriend.jid + '/sendJoinGameRequest',
				  method: 'POST'
				};
				
				var req1 = http.request(options1, function(res1) {
				  console.log('STATUS: ' + res1.statusCode);
				  console.log('HEADERS: ' + JSON.stringify(res1.headers));
				  res1.setEncoding('utf8');
				  res1.on('data', function (chunk1) {
					console.log('BODY: ' + chunk1);
				  });
				});

				req1.on('error', function(e) {
				  console.log('problem with request: ' + e.message);
				});

				// write data to request body
				req1.end();
			}
		} else {
			console.log('You do not have enough friends');
		}
    },
	
	randomMassMessage: function(startFriend, numOfFriend, msInterval) {
		console.log('<< randomMassMessage');
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {

			    for (var i = startFriend; i < startFriend+numOfFriend; i++) {
                    var targetMessage = ["Ever notice that anyone going slower than you is an idiot, but anyone going faster is a maniac?", "When your only tool is a hammer, all problems start looking like nails.", "Artificial intelligence is no match for natural stupidity", "If #2 pencils are the most popular, are they still #2?", "All power corrupts. Absolute power is pretty neat, though."][parseInt(Math.random() * 5)];
					var randomFriend = gFriends[i];

                    sendMessageHelper(randomFriend, targetMessage);
				}
            } else {
                console.log('You do not have enough friends');
            }
        }, msInterval);
		gIntervalIds.push(interval);
    },
	
	randomMassPresence: function(startFriend, numOfFriend, msInterval) {
		console.log('<< randomMassPresence');
		console.log('gFriendLegth: '+gFriends.length);
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {
                for (var i = startFriend; i < startFriend+numOfFriend; i++ ) {

                    var targetPresence = ["AWAY", "ONLINE", "OFFLINE"][parseInt(Math.random() * 3)];
				    var randomFriend = gFriends[i];
					
				    if ( parseInt(Math.random() * 4) == 3 ) {
					    sendInvisibleHelper(randomFriend);
				    } else {
					    sendPresenceHelper(randomFriend, targetPresence);
				    }
				}
            } else {
                console.log('You do not have enough friends');
            }
        }, msInterval);
		gIntervalIds.push(interval);
    },
	
	randomMassGamePresence: function(startFriend, numOfFriend, gameTitle, offerId, msInterval) {
		console.log('<< randomMassGamePresence');
		console.log('gFriendLegth: '+gFriends.length);
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {
                for (var i = startFriend; i < startFriend+numOfFriend; i++ ) {
                    var randomOpType = parseInt(Math.random() * 3 );
				
                    var randomFriend = gFriends[ i ];
				
			        if ( randomOpType < 2 ) {
					    var targetGamePresence = ["JOINABLE","INGAME"][parseInt(Math.random())];

					    sendGamePresenceHelper(randomFriend, gameTitle, offerId, targetGamePresence);
				    } else {
					    sendStopGamePlayedHelper(randomFriend);
				    }  
				}
            } else {
                console.log('You do not have enough friends');
            }
        }, msInterval);
		gIntervalIds.push(interval);
    },
	
	randomMassCombineMessage: function(startFriend, numOfFriend, msInterval) {
		console.log('<< randomMassCombineMessage');
		console.log('gFriendLegth: '+gFriends.length);
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {
                for (var i = startFriend; i < startFriend+numOfFriend; i++ ) {
					var targetMessage = ["Ever notice that anyone going slower than you is an idiot, but anyone going faster is a maniac?", "When your only tool is a hammer, all problems start looking like nails.", "Artificial intelligence is no match for natural stupidity", "If #2 pencils are the most popular, are they still #2?", "All power corrupts. Absolute power is pretty neat, though."][parseInt(Math.random() * 5)];
				    var randomFriend = gFriends[i];
					
				    sendPresenceHelper(randomFriend, "ONLINE");
					sleep(100).then(() => {
                        sendMessageHelper(randomFriend, targetMessage);
                    });
					
					sleep(200).then(() => {
                        sendPresenceHelper(randomFriend, "AWAY");
                    });
					
					sleep(100).then(() => {
                        sendPresenceHelper(randomFriend, "ONLINE");
                    });
					
					sleep(100).then(() => {
                        sendMessageHelper(randomFriend, targetMessage);
                    });
					
					sleep(200).then(() => {
                        sendInvisibleHelper(randomFriend);
                    });
				}
            } else {
                console.log('You do not have enough friends');
            }
        }, msInterval);
		gIntervalIds.push(interval);
    },
	
	randomMassCombineGameMessage: function(startFriend, numOfFriend, gameTitle, offerId, msInterval) {
		console.log('<< randomMassCombineGameMessage');
		console.log('gFriendLegth: '+gFriends.length);
        var interval = setInterval(function(){
            if(gFriends.length > startFriend+numOfFriend) {
                for (var i = startFriend; i < startFriend+numOfFriend; i++ ) {
					var targetMessage = ["Ever notice that anyone going slower than you is an idiot, but anyone going faster is a maniac?", "When your only tool is a hammer, all problems start looking like nails.", "Artificial intelligence is no match for natural stupidity", "If #2 pencils are the most popular, are they still #2?", "All power corrupts. Absolute power is pretty neat, though."][parseInt(Math.random() * 5)];
				    var randomFriend = gFriends[i];
					
					sendGamePresenceHelper(randomFriend, gameTitle, offerId, "INGAME");
					sleep(100).then(() => {
                        sendMessageHelper(randomFriend, targetMessage);
                    });
					
					sleep(200).then(() => {
                        sendPresenceHelper(randomFriend, "AWAY");
                    });
					
					sleep(100).then(() => {
                        sendGamePresenceHelper(randomFriend, gameTitle, offerId, "JOINABLE");
                    });
					
					sleep(100).then(() => {
                        sendMessageHelper(randomFriend, targetMessage);
                    });
					
					sleep(200).then(() => {
                        sendInvisibleHelper(randomFriend);
                    });
				}
            } else {
                console.log('You do not have enough friends');
            }
        }, msInterval);
		gIntervalIds.push(interval);
    },
	
	stopJobs : function() {
	    for (var i = 0; i < gIntervalIds.length; i++) {
		    clearInterval(gIntervalIds[i]);
	    }
	}
}

// Put a friendly message on the terminal of the server.
console.log("Chat server running at port 5222\n");
