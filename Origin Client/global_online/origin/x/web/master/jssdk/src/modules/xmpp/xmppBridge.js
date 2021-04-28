/*jshint strict: false */
/*jshint unused: false */

define([
    'promise',
    'core/events',
    'core/urls',
    'core/logger',
    'modules/client/client'
], function(Promise, events, urls, logger, client) {

    var connected = false;

    /**
     * bridge sends us back an array of presence so we want to iterate over that, in most cases
     * it will just be an array of 1, except for the case of initialPresence
     */
    function onPresenceArray(presenceArray) {
        for (var i = 0; i < presenceArray.length; i++) {
            events.fire(events.XMPP_PRESENCECHANGED, presenceArray[i]);
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

    function onRosterChanged(rosterChangeObject) {
        events.fire(events.XMPP_ROSTERCHANGED, rosterChangeObject);
        return true;
    }

    function onBlockListChanged() {
        events.fire(events.XMPP_BLOCKLISTCHANGED);
        return true;
    }

    function onConnectionStateChanged(isConnected) {
        //broadcast that we're connected
        connected = isConnected;

        if (connected) {
            events.fire(events.XMPP_CONNECTED);
        } else {
            events.fire(events.XMPP_DISCONNECTED);
        }

        logger.log('xmppBridge:handleConnectionChange: social connected =', connected);
    }

    function onMessageReceived(msg) {
        if (msg.type === 'chat' || msg.type === 'groupchat') {
            msg.time = Date.now();
            events.fire(events.XMPP_INCOMINGMSG, msg);
        }
    }

    function init() {
        events.on(events.CLIENT_SOCIAL_PRESENCECHANGED, onPresenceArray);
        events.on(events.CLIENT_SOCIAL_CONNECTIONCHANGED, onConnectionStateChanged);
        events.on(events.CLIENT_SOCIAL_MESSAGERECEIVED, onMessageReceived);
        events.on(events.CLIENT_SOCIAL_CHATSTATERECEIVED, onMessageReceived);
        events.on(events.CLIENT_SOCIAL_ROSTERCHANGED, onRosterChanged);
        events.on(events.CLIENT_SOCIAL_BLOCKLISTCHANGED, onBlockListChanged);
    }

    function handleRosterLoadSuccess(resolve, timeoutHandle) {
        return function(roster) {
            clearTimeout(timeoutHandle);
            resolve(roster.roster);
        };
    }

    function handleRosterLoadTimeout(reject) {
        return function() {
            reject(new Error('[XMPP FROM CLIENT]: Initial roster load timed out'));
        };
    }

    function waitForRosterLoad() {
        return new Promise(function(resolve, reject) {
            var ROSTER_WAIT_TIMEOUT = 30000,
            timeoutHandle = setTimeout(handleRosterLoadTimeout(reject), ROSTER_WAIT_TIMEOUT);
            events.on(events.CLIENT_SOCIAL_ROSTERLOADED, handleRosterLoadSuccess(resolve, timeoutHandle));
        });
    }

    function loadRosterIfNeeded(loaded) {
        return loaded ? client.social.roster() : waitForRosterLoad();
    }

    function handleUpdatePresenceError() {
        logger.error('[XMPP FROM CLIENT]: Error Updating Presence');
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
            //check initial xmpp client connection state
            client.social.isConnectionEstablished().then(onConnectionStateChanged);
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
         * convert a nucleusId to a JabberID
         * @method
         * @param {string} nucleusId The nucleusId you want to convert
         * @returns {string}
         */
        nucleusIdToJid: function (nucleusId) {
            return nucleusId + '@' + urls.endPoints.xmppConfig.domain;
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
            return client.social.isRosterLoaded()
                .then(loadRosterIfNeeded);
        },
        /**
         * Sends a message to the selected user
         * @method
         * @param {string} userId The jid of the user you want to send the message to.
         * @param {string} msgBody The message you want to send.
         * @returns {void}
         */
        sendMessage: function(userId, msgBody, type) {
            client.social.sendMessage(userId, msgBody, typeof type === 'undefined' ? 'chat' : type);
        },


        sendTypingState: function(state, userId) {
            client.social.setTypingState(state, userId);
        },


        /**
         * Accept a friend request from a giver user
         * @method
         * @param {string} userId The jid of the user whose friend request you want to accept.
         * @returns {void}
         */
        friendRequestAccept: function(jid) {
            client.social.approveSubscriptionRequest(jid);
        },

        /**
         * Reject a friend request from a giver user
         * @method
         * @param {string} userId The jid of the user whose friend request you want to reject.
         * @returns {void}
         */
        friendRequestReject: function(jid) {
            client.social.denySubscriptionRequest(jid);
        },

        /**
         * Send a friend request to the user
         * @method
         * @param {string} userId The jid of the user who you want to send a friend request to.
         * @returns {void}
         */
        friendRequestSend: function(jid) {
            client.social.subscriptionRequest(jid);
        },

        /**
         * Revoke the friend request you sent
         * @method
         * @param {string} userId The jid of the user who you want to revoke the friend request from.
         * @returns {void}
         */
        friendRequestRevoke: function(jid) {
            client.social.cancelSubscriptionRequest(jid);
        },

        /**
         * Revoke a friend
         * @method
         * @param {string} userId The jid of the friend who you want to remove.
         * @returns {void}
         */
        removeFriend: function(jid) {
            client.social.removeFriend(jid);
        },

        /**
         * Remove a friend and block user
         * @method
         * @param {string} jid The jid of the friend who you want to remove.
         * @param {string} nucleusId The nucleusId of the user who you want to block.
         * @returns {void}
         */
        removeFriendAndBlock: function(nucleusId, jid) {
            // On the client, a block will automatically remove friend
            client.social.blockUser(nucleusId);
        }, 
        
        /**
         * Block a user
         * @method
         * @param {string} userId The nucleusId of the user who you want to block.
         * @returns {void}
         */
        blockUser: function(nucleusId) {
            client.social.blockUser(nucleusId);
        },
        
        /**
         * Block a user, and cancel pending friend request
         * @method
         * @param {string} userId The nucleusId of the user who you want to block.
         * @returns {void}
         */
        cancelAndBlockUser: function(nucleusId) {
            client.social.blockUser(nucleusId);
        },

        /**
         * Block a user, and ignore incoming friend request
         * @method
         * @param {string} userId The nucleusId of the user who you want to block.
         * @returns {void}
         */
        ignoreAndBlockUser: function(nucleusId) {
            client.social.blockUser(nucleusId);
        },
        
        /**
         * Unblock a user
         * @method
         * @param {string} userId The nucleusId of the user who you want to unblock.
         * @returns {void}
         */
        unblockUser: function(nucleusId) {
            client.social.unblockUser(nucleusId);
        },
        
        /**
         * Join a friend's game
         * @method
         * @param {string} userId The nucleusId of the friend who's game you want to join.
         * @returns {void}
         */
        joinGame: function (nucleusId) {
            client.social.joinGame(nucleusId);
        },

        /**
         * Updates the current users presence
         * @method
         * @returns {void}
         */
        updatePresence: function() {
            client.social.setInitialPresence('ONLINE')
                .then(client.social.requestInitialPresenceForUserAndFriends)
                .then(onPresenceArray)
                .catch(handleUpdatePresenceError);

        },

        requestPresence: function(presence) {
            client.social.requestPresenceChange(presence);
        },

        /**
         * Join a chat room
         * @method
         * @param {jid} jid of room to join
         * @param {originId} Origin ID of user joining the room
         * @returns {void}
         */
        joinRoom: function(jid, originId) {
            client.social.joinRoom(jid, originId);
        },

        /**
         * Leave a chat room
         * @method
         * @param {jid} jid of room to leave
         * @param {originId} Origin ID of user leaving the room
         * @returns {void}
         */
        leaveRoom: function(jid, originId) {
            client.social.leaveRoom(jid, originId);
        },

        isBlocked: function(nucleusId) {
            return Promise.resolve(client.social.isBlocked(nucleusId));
        },

        /**
         * Loads the XMPP block list- just a stub- block list is automatically loaded on client
         * @method
         * @returns {void}
         */
        loadBlockList: function() {
        },
        

    };
});
