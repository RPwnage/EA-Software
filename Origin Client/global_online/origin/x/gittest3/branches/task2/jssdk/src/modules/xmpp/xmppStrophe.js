/*jshint strict: false */
/*jshint unused: false */
define([
    'promise',
    'strophe',
    'core/auth',
    'core/user',
    'core/events',
    'core/urls',
    'core/logger',
], function (Promise, Strophe, auth, user, events, urls, logger) {

    var clientResource = '';
    var connected = false;
    var connection = null;
    var connectionStatus = Strophe.Status.DISCONNECTED;
    var subStateMap = {
        none: 'NONE',
        to: 'TO',
        from: 'FROM',
        both: 'BOTH',
        remove: 'REMOVE'
    };

    var presenceStateMap = {
        unavailable: 'UNAVAILABLE',
        subscribe: 'SUBSCRIBE',
        subscribed: 'SUBSCRIBED',
        unsubscribe: 'UNSUBSCRIBE',
        unsubscribed: 'UNSUBSCRIBED',
        probe: 'PROBE',
        error: 'ERROR'
    };

    var showStateMap = {
        online: 'ONLINE',
        away: 'AWAY',
        chat: 'CHAT',
        dnd: 'DND',
        xa: 'XA'
    };

    window.onunload = function() {
        disconnectSync();
    };

    function enableCarbonCopy() {
        // Enable carbon copy
        var iq = $iq({
                type: 'set',
                xmlns: Strophe.NS.ROSTER,
                from: user.publicObjs.userPid() + '@' + urls.endPoints.xmppConfig.domain + '/' + clientResource
            })
            .c('enable', {
                xmlns: 'urn:xmpp:carbons:2'
            });
        connection.sendIQ(iq);
    }

    //disconnect synchronously(i.e. serially)
    function disconnectSync() {
        if (connection) {
            connection.flush();
            connection.options.sync = true;
            connection.disconnect();
        }
    }

    function onConnect(status) {
        connectionStatus = status;
        switch (status) {
            case Strophe.Status.CONNECTING:
                logger.log('[Strophe-Conn] Connecting');
                break;
                /* jshint ignore:start */
            case Strophe.Status.CONNECTED:
                connected = true;
                events.fire(events.XMPP_CONNECTED);
                //don't automatically do this -- integrator should do this on demand
                //publicObjs.xmpp.updatePresence();
                logger.log('[Strophe-Conn] Connected');
                enableCarbonCopy();
                /* jshint ignore:end */
                //fall through on purpose
            case Strophe.Status.ATTACHED:
                logger.log('[Strophe-Conn] Attached');
                break;

            case Strophe.Status.DISCONNECTING:
                logger.log('[Strophe-Conn] Disconnecting');
                break;

            case Strophe.Status.DISCONNECTED:
                connected = false;
                logger.log('[Strophe-Conn] Disconnected');
                break;

            case Strophe.Status.AUTHFAIL:
                logger.log('[Strophe-Conn] Authfail');
                break;

            case Strophe.Status.AUTHENTICATING:
                logger.log('[Strophe-Conn] Authenticating');
                break;

            case Strophe.Status.ERROR:
            case Strophe.Status.CONNFAIL:
                logger.log('[Strophe-Conn] Error/Connfail: (' + status + ')');
                break;

            default:
                logger.log('[Strophe-Conn] Should not be here');
                break;
        }

    }

    function parseGameActivity(activityElement) {
        var gameActivityInfo = {};
        if (activityElement) {
            //title;productId; JOINABLE/INGAME;twitchPresence;gamePresence;multiplayerId
            var gameActivityStr = activityElement.textContent;
            var splitStrArray = gameActivityStr.split(';');
            var activitylen = splitStrArray.length;

            if (activitylen > 0) {
                gameActivityInfo.title = splitStrArray[0];
            }
            if (activitylen > 1) {
                gameActivityInfo.productId = splitStrArray[1];
            }
            if (activitylen > 2) {
                gameActivityInfo.joinable = (splitStrArray[2] === 'JOINABLE');
            }
            if (activitylen > 3) {
                gameActivityInfo.twitchPresence = splitStrArray[3];
            }
            if (activitylen > 4) {
                gameActivityInfo.gamePresence = splitStrArray[4];
            }
            if (activitylen > 5) {
                gameActivityInfo.multiplayerId = splitStrArray[5];
            }
        }
        return gameActivityInfo;
    }

    function onPresence(presence) {
        //logger.log(presence);
        var presenceType = presence.getAttribute('type'); // unavailable, subscribed, etc...
        var from = presence.getAttribute('from'); // the jabber_id of the contact
        var to = presence.getAttribute('to'); // the jabber_id of the contact
        
        var jid = '';
        var nick = '';
        var itemElement = presence.getElementsByTagName('item');
        if (itemElement.length > 0) {
            var itemIndex = 0;
            while( itemIndex < itemElement.length ) {
                jid = itemElement[itemIndex].getAttribute('jid');
                nick = itemElement[itemIndex].getAttribute('nick');

                if( jid === null || nick === null ) {
                    itemIndex++;
                }
                else {
                    break;
                }
            }
        }

        var showElement = presence.querySelector('show');
        var show = null;
        var resource = Strophe.getResourceFromJid(from);
        var userId = Strophe.getNodeFromJid(from);
        var remoteOn = true;
        var presenceOut = presenceStateMap.available;
        var showOut = showStateMap.online;

        //check for gameactivity
        var gameActivityElement = presence.querySelector('activity');
        var gameActivityInfo = parseGameActivity(gameActivityElement);

        //check to see if we detect an Origin Client
        if ((userId === user.publicObjs.userPid().toString()) && (resource.indexOf('origin') !== -1)) {
            clientResource = resource;
            if (presenceType === presenceStateMap.unavailable) {
                remoteOn = false;
                logger.log('disconnected from Origin Client');
            } else {
                remoteOn = true;
                logger.log('connected to Origin Client');
            }
            events.fire(events.priv.REMOTE_CLIENT_AVAILABLE, remoteOn);
        }

        if (showElement) {
            show = showElement.textContent;
            if (show) {
                showOut = showStateMap[show];
            }
        }

        if (presenceType) {
            presenceOut = presenceStateMap[presenceType];
        }

        //we run this off of the event loop otherwise all exceptions get swallowed up by strophe
        setTimeout(function() {
            events.fire(events.XMPP_PRESENCECHANGED, {
                jid: from,
                presenceType: presenceOut,
                show: showOut,
                gameActivity: gameActivityInfo,
                to: to,
                from: jid, //TR 'from' and 'jid' are mismatched, perhaps this should be fixed up
                nick: nick
            });
        }, 0);
        return true;
    }

    function onMessage(msg) {
        var from = msg.getAttribute('from');
        var to = msg.getAttribute('to');
        var type = msg.getAttribute('type');
        var elems = msg.getElementsByTagName('body');
        var active = msg.getElementsByTagName('active');
        var inactive = msg.getElementsByTagName('inactive');
        var gone = msg.getElementsByTagName('gone');
        var composing = msg.getElementsByTagName('composing');
        var paused = msg.getElementsByTagName('paused');
        var userId = from.substring(0, from.indexOf('/'));
        var messageBody, chatState;

        // Handle carbon copy messages
        var sent = msg.getElementsByTagName('sent');
        if (sent.length) {
            var forwarded = sent[0].getElementsByTagName('forwarded');
            if (forwarded.length) {
                var message = forwarded[0].getElementsByTagName('message');
                if (message.length) {
                    onMessage(message[0]);
                    // we must return true to keep the handler alive.
                    // returning false would remove it after it finishes.
                    return true;
                }

            }
        }

        setTimeout(function() {
            var msgObject = {};
            if ((type === 'chat' || type === 'groupchat') && elems.length > 0) {
                if (active.length) {
                    chatState = 'ACTIVE';
                } else
                if (inactive.length) {
                    chatState = 'INACTIVE';
                } else
                if (gone.length) {
                    chatState = 'GONE';
                } else
                if (composing.length) {
                    chatState = 'COMPOSING';
                } else
                if (paused.length) {
                    chatState = 'PAUSED';
                }
                messageBody = Strophe.getText(elems[0]);

                events.fire(events.XMPP_INCOMINGMSG, {
                    jid: userId,
                    msgBody: messageBody,
                    chatState: chatState,
                    time: Date.now(),
                    from: from,
                    to: to
                });
            } else {
                messageBody = Strophe.getText(elems[0]).replace(/&quot;/g, '"');
                if (type !== 'origin-action') {
                    msgObject = JSON.parse(messageBody);
                    if (type === 'origin-response-fromclient' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_CONFIRMATION_FROM_CLIENT, msgObject);
                    } else if (type === 'origin-action-fromclient' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_STATUS_UPDATE_FROM_CLIENT, msgObject);
                    } else if (type === 'origin-init' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_STATUS_GAMELISTUPDATED, msgObject);
                        events.fire(events.priv.REMOTE_STATUS_UPDATE, msgObject);
                    } else if (type === 'origin-cq-status' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_STATUS_CQ, msgObject);
                    }
                }
            }
        }, 0);

        // we must return true to keep the handler alive.
        // returning false would remove it after it finishes.
        return true;

    }

    function onStreamError(msg) {
        var isConflict = msg.getElementsByTagName('conflict').length;
        if (isConflict) {
            events.fire(events.XMPP_USERCONFLICT);
        }
    }

    function onRosterChanged(iq) {

        logger.log(iq);
        var item = iq.querySelector('item');
        var userId = item.getAttribute('jid');
        var subscription = item.getAttribute('subscription');
        var subOut = subStateMap.none;

        if (subscription) {
            subOut = subStateMap[subscription];
        }

        events.fire(events.XMPP_ROSTERCHANGED, {
            jid: userId,
            subState: subOut
        });

        // acknowledge receipt
        connection.send($iq({
            type: 'result',
            id: iq.getAttribute('id')
        }));
        return true;
    }

    function registerHandlers() {
        connection.addHandler(onRosterChanged, Strophe.NS.ROSTER, 'iq', 'set');
        connection.addHandler(onPresence, null, 'presence');
        connection.addHandler(onMessage, null, 'message');
        connection.addHandler(onStreamError, null, 'stream:error');
    }

    function setupStropheConnection(jid, accessToken) {
        logger.log('orig:' + urls.endPoints.xmppConfig.wsHost);

        connection = new Strophe.Connection(urls.endPoints.xmppConfig.wsScheme + '://' + urls.endPoints.xmppConfig.wsHost + ':' + urls.endPoints.xmppConfig.wsPort);

        connection.rawInput = logIncoming;
        connection.rawOutput = logOutgoing;
        connection.connect(jid, accessToken, onConnect);
        registerHandlers();
    }

    function connect(jid, accessToken) {
        var xmlHttp = null;

        if (urls.endPoints.xmppConfig.wsHost === '') {
            xmlHttp = new XMLHttpRequest();
            //            xmlHttp.open('GET', urls.endPoints.xmppConfig.redirectorUrl + jid, false);
            xmlHttp.open('GET', urls.endPoints.xmppConfig.redirectorUrl + jid, true);

            xmlHttp.onload = function() {
                if (xmlHttp.status === 200) {

                    urls.endPoints.xmppConfig.wsHost = xmlHttp.getResponseHeader('Content-Location');
                    setupStropheConnection(jid, accessToken);
                } else {
                    logger.log('XMPPstrophe- connect error: ', xmlHttp.status, ', ', xmlHttp.statusText);
                }
            };

            // Handle network errors
            xmlHttp.onerror = function() {
                logger.log('XMPPstrophe- connect error: -1');
            };

            xmlHttp.send(null);
        } else {
            setupStropheConnection(jid, accessToken);
        }
    }

    function logIncoming(msg) {
		// Limit length of output to 1000 chars
        logger.log('[XMPP-IN]: ' + msg.substring(0, 1000) + ((msg.length > 1000) ? '...' + (msg.length-1000) + ' chars truncated' : ''));
    }

    function logOutgoing(msg) {
		// Limit length of output to 1000 chars
        logger.log('[XMPP-OUT]: ' + msg.substring(0, 1000) + ((msg.length > 1000) ? '...' + (msg.length-1000) + ' chars truncated' : ''));
    }


    /*
    function onNucleusLogin() {
        connect(auth.userPid() + '@' + urls.endPoints.xmppConfig.domain, auth.accessToken());
    }
    */
    function onAuthLoggedOut() {
        disconnectSync();
    }

    function onSendActionMessage(msgBody) {
        connection.send($msg({
            to: user.publicObjs.userPid() + '@' + urls.endPoints.xmppConfig.domain + '/' + clientResource, //'/origin',
            type: 'origin-action'
        }).c('body').t(msgBody));
    }

    function onSendResponseMessage(msgBody) {
        connection.send($msg({
            to: user.publicObjs.userPid() + '@' + urls.endPoints.xmppConfig.domain + '/' + clientResource, //'/origin',
            type: 'origin-response'
        }).c('body').t(msgBody));
    }

    function init() {
        if (typeof OriginSocialManager === 'undefined') {
            connected = false;
            //don't automatically start up, integrator should do this on-demand
            //pubEvents.on('authSuccessRetry', onNucleusLogin);
            events.on(events.AUTH_LOGGEDOUT, onAuthLoggedOut);
            events.on(events.priv.REMOTE_STATUS_SENDACTION, onSendActionMessage);
            events.on(events.priv.REMOTE_SEND_CONFIRMATION_TO_CLIENT, onSendResponseMessage);
        }
    }

    function friendRequestAction(jid, action) {
        connection.send($pres({
            to: jid,
            type: action
        }));
    }

    function canConnect() {
        return ((connectionStatus !== Strophe.Status.CONNECTING) && (connectionStatus !== Strophe.Status.CONNECTED));
    }

    /** @namespace
     * @name xmpp
     * @memberof Origin
     */
    return {
        init: init,

        /**
         * @method
         * @memberof Origin.xmpp
         * @returns {boolean}
         */
        isConnected: function() {
            return connected;
        },

        /**
         * initiate xmpp connection
         * @method
         * @memberof Origin.xmpp
         * @returns {void}
         */
        connect: function() {
            //doesn't seem to care if connect is called multiple times -- whether onnection is already in progress or if it's already connected, so
            //don't need to trap for those cases
            if (auth.isLoggedIn() && canConnect()) {
                connect(user.publicObjs.userPid() + '@' + urls.endPoints.xmppConfig.domain, user.publicObjs.accessToken());
            }
        },

        /**
         * manual disconnect -- will disconnect automatically when jssdk logout is detected
         * @method
         * @memberof Origin.xmpp
         * @returns {void}
         */
        disconnect: function() {
            disconnectSync();
        },

        /**
         * returned from the {@link Origin.xmpp.requestRoster} promise
         * @typedef rosterObject
         * @type {object}
         * @property {string} subState The subscription state of the user.
         * @property {string} jid The jabber id of the user.
         * @property {string} originId originId
         * @property {string} subReqSent true if you sent this user a request
         */
        /**
         * Request the friends roster of the current user
         * @method
         * @memberof Origin.xmpp
         * @returns {promise<rosterObject>} The result of the promise will return the xmpp iq roster stanza
         */
        requestRoster: function(requestSuccess, requestError) {
            return new Promise(function(resolve, reject) {
                function requestSuccess(data) {
                    var userArray = data.getElementsByTagName('item');
                    var i, userObj, subState, subOut = '',
                        returnArray = [];
                    for (i = 0; i < userArray.length; i++) {
                        subState = userArray[i].getAttribute('subscription');
                        if (subState) {
                            subOut = subStateMap[subState];
                        }
                        userObj = {
                            subState: subOut,
                            jid: userArray[i].getAttribute('jid'),
                            originId: userArray[i].getAttribute('origin:eaid'),
                            subReqSent: userArray[i].getAttribute('ask') === 'subscribe'
                        };
                        returnArray.push(userObj);
                    }
                    resolve(returnArray);
                }

                function requestError(status, textStatus) {
                    logger.log('requestRoster error:', status, ',', textStatus);
                    reject(status);
                }

                if (!connected) {
                    requestError(-1, 'social not connected');
                    return;
                }

                // build and send initial roster query
                var rosteriq = $iq({
                    type: 'get'
                }).c('query', {
                    xmlns: Strophe.NS.ROSTER,
                    'origin:list': 'new'
                });
                connection.sendIQ(rosteriq, requestSuccess, requestError);
            });

        },
        /**
         * Sends a message to the selected user
         * @method
         * @memberof Origin.xmpp
         * @param {string} userId The jid of the user you want to send the message to.
         * @param {string} msgBody The message you want to send.
         * @param {string} type The type of message you want to send, 'chat' or 'groupchat'
         * @returns {void}
         */
        sendMessage: function(userId, msgBody, type) {
            connection.send($msg({
                to: userId,
                type: ( typeof type === 'undefined' ? 'chat' : type)
            })
            .c('active', {
                xmlns: 'http://jabber.org/protocol/chatstates'
            })
            .up()
            .c('body').t(msgBody));
        },



        sendTypingState: function(state, userId) {
            connection.send($msg({
                    to: userId,
                    type: 'chat'
                })
                .c(state, {
                    xmlns: 'http://jabber.org/protocol/chatstates'
                })
                .up()
                .c('body').t(''));
        },


        /**
         * Accept a friend request from a giver user
         * @method
         * @memberof Origin.xmpp
         * @param {string} userId The jid of the user whose friend request you want to accept.
         * @returns {void}
         */
        friendRequestAccept: function(jid) {
            friendRequestAction(jid, 'subscribed');
        },

        /**
         * Reject a friend request from a giver user
         * @method
         * @memberof Origin.xmpp
         * @param {string} userId The jid of the user whose friend request you want to reject.
         * @returns {void}
         */
        friendRequestReject: function(jid) {
            friendRequestAction(jid, 'unsubscribed');
        },
        /**
         * Send a friend request to the user
         * @method
         * @memberof Origin.xmpp
         * @param {string} userId The jid of the user who you want to send a friend request to.
         * @returns {void}
         */
        friendRequestSend: function(jid) {
            friendRequestAction(jid, 'subscribe');
        },
        /**
         * Revoke the friend request you sent
         * @method
         * @memberof Origin.xmpp
         * @param {string} userId The jid of the user who you want to revoke the friend request from.
         * @returns {void}
         */
        friendRequestRevoke: function(jid) {
            friendRequestAction(jid, 'unsubscribe');
        },
        /**
         * Revoke a friend
         * @method
         * @memberof Origin.xmpp
         * @param {string} userId The jid of the friend who you want to remove.
         * @returns {void}
         */
        friendRemove: function(jid) {
            var iq = $iq({
                    type: 'set'
                })
                .c('query', {
                    xmlns: Strophe.NS.ROSTER
                })
                .c('item', {
                    jid: jid,
                    subscription: 'remove'
                });
            connection.sendIQ(iq);
        },
        
        /**
         * @returns {void}
         */
        joinRoom: function(jid, originId) {
            connection.send($pres({
                to: jid + '/' + originId,
                id: 'roomId'
            }));
        },
        
        /**
         * @returns {void}
         */
        leaveRoom: function(jid, originId) {
            connection.send($pres({
                to: jid + '/' + originId,
                type: 'unavailable'
            }));
        },

        
        /**
         * Updates the current users presence
         * @method
         * @memberof Origin.xmpp
         * @returns {void}
         */
        updatePresence: function() {
            connection.send($pres());
        },
        
        requestPresence: function (presence) {
            if (presence === 'invisible'){
				connection.send($pres({
					from: user.publicObjs.userPid() + '@' + urls.endPoints.xmppConfig.domain + '/' + clientResource,
					type: 'unavailable'
				}).c('show').t(presence));
            }
            else {
				connection.send($pres({
					from: user.publicObjs.userPid() + '@' + urls.endPoints.xmppConfig.domain + '/' + clientResource
				}).c('show').t(presence));
            }
        }
    };
});
