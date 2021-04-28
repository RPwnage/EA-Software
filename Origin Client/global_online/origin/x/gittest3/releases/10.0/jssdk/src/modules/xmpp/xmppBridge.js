/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
    'promise',
    'core/events',
    'core/logger'
], function (Promise, events, logger) {

    var connected = false;

    //order matches what's in C++ client
    var showStateMap = ['CHAT', 'ONLINE', 'DND', 'AWAY', 'XA', 'UNAVAILABLE'];

    /**
     * bridge sends us back an array of presence so we want to iterate over that, in most cases
     * it will just be an array of 1, except for the case of initialPresence
     */
    function onPresenceArray(presenceArray) {
        for (var i = 0; i < presenceArray.length; i++) {
            onPresence(presenceArray[i]);
        }
    }

    /** @typedef gameActivityObject
     *  @type {object}
     *  @property {string} title
     *  @property {string} productId
     *  @property {bool} joinable
     *  @property {string} twitchPresence
     *  @property {string} gamePresence
     *  @property {string} multiplayerId
     */

    /**
     * @typedef bridgePresenceObject
     * @type {object}
     * @property {string} jid
     * @property {string} show
     * @property {gameActivityObj} gameActivity
     */
    /**
     * signal sent when someone in your roster has a presence Change
     * @param {bridgePresenceObject} presence
     * @return triggers event socialPresenceChanged
     */
    function onPresence(presence) {
        //logger.log(presence);
        var from ='';
        var nick ='';
        var jid = '';
        var showOut;
        var show = null;
        var gameActivityInfo = {};

        function copyGameActivity(gameActivity) {
            var gameActivityInfo = {};
            if (Object.keys(gameActivity).length > 0) {
                gameActivityInfo.title = gameActivity.title;
                gameActivityInfo.productId = gameActivity.productId;
                gameActivityInfo.joinable = gameActivity.joinable;
                gameActivityInfo.twitchPresence = gameActivity.twitchPresence;
                gameActivityInfo.gamePresence = gameActivity.gamePresence;
                gameActivityInfo.multiplayerId = gameActivity.multiplayerId;
            }
            return gameActivityInfo;
        }

        from = presence.from;
        nick = presence.nick;
        jid = presence.jid;
        showOut = showStateMap[presence.show];
        gameActivityInfo = copyGameActivity(presence.gameActivity);

        //we run this off of the event loop otherwise all exceptions get swallowed up by strophe
        setTimeout(function() {
            events.fire(events.XMPP_PRESENCECHANGED, {
                from: from,
                nick: nick,
                jid: jid,
                show: showOut,
                gameActivity: gameActivityInfo,
                presenceType: presence.presenceType
            });
        }, 0);
        return true;
    }

    function onRosterChanged(rosterChangeObject) {
        events.fire(events.XMPP_ROSTERCHANGED, rosterChangeObject);
        return true;
    }

    function handleConnectionChange() {
        if (OriginSocialManager.connection) {
            connected = OriginSocialManager.connection.established;
        } else {
            connected = false;
        }

        //broadcast that we're connected
        if (connected) {
            events.fire(events.XMPP_CONNECTED);
        }
        logger.log('xmppBridge:handleConnectionChange: social connected =', connected);
    }

    function onMessageReceived(msg) {
         console.log(msg);
         setTimeout(function() {
            var msgObject = {};
            if (msg.type === 'chat' || msg.type === 'groupchat') {
                events.fire(events.XMPP_INCOMINGMSG, {
                    jid: msg.jid,
                    from: msg.from,
                    to: msg.to,
                    msgBody: msg.body,
                    chatState: msg.chatState,
                    time: Date.now()
                });
            } else {
            console.log('ERROR');
/*                messageBody = Strophe.getText(elems[0]).replace(/&quot;/g, '"');
                if (type !== 'origin-action') {
                    msgObject = JSON.parse(messageBody);
                    if (type === 'origin-status' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_STATUS_UPDATE, [msgObject]);
                    } else if (type === 'origin-dialog' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_DIALOG_UPDATE, [msgObject]);
                    } else if (type === 'origin-init' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_STATUS_GAMELISTUPDATED, msgObject);
                        events.fire(events.priv.REMOTE_STATUS_UPDATE, msgObject);
                    } else if (type === 'origin-cq-status' && elems.length > 0) {
                        events.fire(events.priv.REMOTE_STATUS_CQ, msgObject);
                    }
                }*/
            }
        }, 0);
     }
    function init() {
        handleConnectionChange();
        //listen for connection changes
        OriginSocialManager.connection.changed.connect(handleConnectionChange);
        OriginSocialManager.messageReceived.connect(onMessageReceived);
        OriginSocialManager.chatStateReceived.connect(onMessageReceived);
        OriginSocialManager.presenceChanged.connect(onPresenceArray);
        OriginSocialManager.rosterChanged.connect(onRosterChanged);

    }


    /** @namespace
     * @memberof Origin
     * @alias xmpp
     */
    return {
        init: init,

        /**
         * @method
         * @returns {boolean}
         */
        isConnected: function() {
            return connected;
        },

        /**
         * initiate xmpp connection
         * @method
         * @returns {void}
         */
        connect: function() {
            //do nothing here since the C++ client handles the actual connection
            //but in case someone is listening for the connect, just send back an event that it connected
            handleConnectionChange();
        },

        /**
         * manual disconnect -- will disconnect automatically when jssdk logout is detected
         * @method
         * @returns {void}
         */
        disconnect: function() {
            //for now, do nothing here since the C++ client handle the disconnect
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
         * contactInfoObject - individual contactInfo
         * @typedef contactInfoObject
         * @type {object}
         * @property {string} availability
         * @property {bool} blocked
         * @property {string} capabilities
         * @property {string} id
         * @property {string} nickname
         * @property {string} playingGame
         * @property {string} presence
         * @property {object} realName (firstName, lastName)
         * @property {string} statusText
         * @property {string} jabberId
         * @property {string} originId
         * @property {object} subscriptionState (direction, pendingContactApproval, pendingCurrentUserApproval)
         */

        /**
         * returned from the OriginSocialManager.roster promise
         * @typedef rosterObject
         * @type {object}
         * @property {contactInfoObject[]} contacts - array of contacts
         * @property {bool} hasFriends
         * @property {bool} hasLoaded
         * @property {string} objectName
         */

        /**
         * Request the friends roster of the current user
         * @method
         * @returns {promise<rosterObject>} The result of the promise will return the xmpp iq roster stanza
         */
        requestRoster: function(requestSuccess, requestError) {			
            return new Promise(function(resolve, reject) {
                function requestSuccess(data) {
                    var i, userObj, subState, subOut = '', pendingIn, pendingOut,
                        returnArray = [];

                    if (typeof data.contacts !== 'undefined') {
                        for (i = 0; i < data.contacts.length; i++) {
                            var friend = data.contacts[i];
                            subOut = friend.subscriptionState.direction;
                            pendingIn = friend.subscriptionState.pendingCurrentUserApproval;
                            pendingOut = friend.subscriptionState.pendingContactApproval;
                            userObj = {
                                subState: subOut,
                                jid: friend.jabberId,
                                originId: friend.nickname, //originId in the remoteUserProxy is empty :(
                                subReqSent: pendingOut
                            };
                            returnArray.push(userObj);
                        }
                    }
                    resolve(returnArray);
                }

                function requestError(status, textStatus) {
                    logger.error('requestRoster error:', status, ',', textStatus);
                    reject(status);
                }

                if (!connected) {
                    requestError(-1, 'social not connected');
                    return;
                }

				if (!!OriginSocialManager.roster) {
					
					if (OriginSocialManager.roster.hasLoaded) {
						requestSuccess(OriginSocialManager.roster);
					} else {
						// wait for roster load
						var onRosterLoad = function() {
							requestSuccess(OriginSocialManager.roster);							
						};
						
						OriginSocialManager.roster.loaded.connect(onRosterLoad);
					}
					
				} else {
					requestError(-1, 'roster not found');
				}
				
            });
        },
        /**
         * Sends a message to the selected user
         * @method
         * @param {string} userId The jid of the user you want to send the message to.
         * @param {string} msgBody The message you want to send.
         * @returns {void}
         */
        sendMessage: function(userId, msgBody, type) {
            OriginSocialManager.onSendMessage(userId, msgBody, typeof type === 'undefined' ? 'chat' : type);
        },


        sendTypingState: function (state, userId) {
            OriginSocialManager.setTypingState(userId, state);
        },


        /**
         * Accept a friend request from a giver user
         * @method
         * @param {string} userId The jid of the user whose friend request you want to accept.
         * @returns {void}
         */
        friendRequestAccept: function (jid) {
            OriginSocialManager.approveSubscriptionRequest(jid);
        },

        /**
         * Reject a friend request from a giver user
         * @method
         * @param {string} userId The jid of the user whose friend request you want to reject.
         * @returns {void}
         */
        friendRequestReject: function (jid) {
            OriginSocialManager.denySubscriptionRequest(jid);
        },

        /**
         * Send a friend request to the user
         * @method
         * @param {string} userId The jid of the user who you want to send a friend request to.
         * @returns {void}
         */
        friendRequestSend: function (jid) { },

        /**
         * Revoke the friend request you sent
         * @method
         * @param {string} userId The jid of the user who you want to revoke the friend request from.
         * @returns {void}
         */
        friendRequestRevoke: function (jid) {
            OriginSocialManager.cancelSubscriptionRequest(jid);
        },

        /**
         * Revoke a friend
         * @method
         * @param {string} userId The jid of the friend who you want to remove.
         * @returns {void}
         */
        friendRemove: function(jid) {},

        /**
         * Updates the current users presence
         * @method
         * @returns {void}
         */
        updatePresence: function() {
            //setTimeout(function() {
             OriginSocialManager.setInitialPresence('ONLINE');                
            //}, 10000);
        },

        requestPresence: function (presence) {
           OriginSocialManager.onRequestedPresenceChange(presence);
        },

        /**
         * Join a chat room
         * @method
         * @param {jid} jid of room to join
         * @param {originId} Origin ID of user joining the room
         * @returns {void}
         */
        joinRoom: function(jid, originId) {
            OriginSocialManager.joinRoom(jid, originId);
        },
        
        /**
         * Leave a chat room
         * @method
         * @param {jid} jid of room to leave
         * @param {originId} Origin ID of user leaving the room
         * @returns {void}
         */
        leaveRoom: function(jid, originId) {
            OriginSocialManager.leaveRoom(jid, originId);
        }
    };
});
