/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to social with the C++ client
 */


define([
    'modules/client/clientobjectregistry'
], function(clientobjectregistry) {

    /**
     * Contains client social communication methods
     * @module module:social
     * @memberof module:Origin.module:client
     *
     */

    var clientSocialWrapper = null,
        clientObjectName = 'OriginSocialManager';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientSocialWrapper = clientObjectWrapper;
        if (clientSocialWrapper.clientObject) {
            clientSocialWrapper.connectClientSignalToJSSDKEvent('connectionChanged', 'CLIENT_SOCIAL_CONNECTIONCHANGED');
            clientSocialWrapper.connectClientSignalToJSSDKEvent('messageReceived', 'CLIENT_SOCIAL_MESSAGERECEIVED');
            clientSocialWrapper.connectClientSignalToJSSDKEvent('chatStateReceived', 'CLIENT_SOCIAL_CHATSTATERECEIVED');
            clientSocialWrapper.connectClientSignalToJSSDKEvent('presenceChanged', 'CLIENT_SOCIAL_PRESENCECHANGED');
            clientSocialWrapper.connectClientSignalToJSSDKEvent('blockListChanged', 'CLIENT_SOCIAL_BLOCKLISTCHANGED');
            clientSocialWrapper.connectClientSignalToJSSDKEvent('rosterChanged', 'CLIENT_SOCIAL_ROSTERCHANGED');
            clientSocialWrapper.connectClientSignalToJSSDKEvent('rosterLoaded', 'CLIENT_SOCIAL_ROSTERLOADED');
            clientSocialWrapper.connectClientSignalToJSSDKEvent('gameInviteReceived', 'CLIENT_SOCIAL_GAMEINVITERECEIVED');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:social */ {

        /**
         * is the social connection established
         * @returns {promise} returns a promise the resolves a boolean (true if connected/ false if not)
         * @static
         * @method
         */
        isConnectionEstablished: function() {
            return clientSocialWrapper.sendToOriginClient('isConnectionEstablished');
        },
        /**
         * is the roster loaded
         * @returns {promise} returns a promise the resolves a boolean (true if roster is loaded/ false if not)
         * @static
         * @method
         */
        isRosterLoaded: function() {
            return clientSocialWrapper.sendToOriginClient('isRosterLoaded');
        },
        /**
         * returns the current roster
         * @returns {promise} returns the current client roster
         * @static
         * @method
         */
        roster: function() {
            return clientSocialWrapper.sendToOriginClient('roster');
        },
        /**
         * send a chat message
         * @param  {string} id      [description]
         * @param  {string} msgBody [description]
         * @param  {string} type    [description]
         * @return {promise}         [description]
         */
        sendMessage: function(id, msgBody, type) {
            return clientSocialWrapper.sendToOriginClient('sendMessage', arguments);
        },
        /**
         * [setTypingState description]
         * @param {promise} state  [description]
         * @param {promise} userId [description]
         */
        setTypingState: function(state, userId) {
            return clientSocialWrapper.sendToOriginClient('setTypingState', arguments);
        },
        /**
         * [approveSubscriptionRequest description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        approveSubscriptionRequest: function(jid) {
            return clientSocialWrapper.sendToOriginClient('approveSubscriptionRequest', arguments);
        },
        /**
         * [denySubscriptionRequest description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        denySubscriptionRequest: function(jid) {
            return clientSocialWrapper.sendToOriginClient('denySubscriptionRequest', arguments);
        },
        /**
         * [subscriptionRequest description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        subscriptionRequest: function (jid) {
            return clientSocialWrapper.sendToOriginClient('subscriptionRequest', arguments);
        },
        /**
         * [cancelSubscriptionRequest description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        cancelSubscriptionRequest: function(jid) {
            return clientSocialWrapper.sendToOriginClient('cancelSubscriptionRequest', arguments);
        },
        /**
         * [removeFriend description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        removeFriend: function(jid) {
            return clientSocialWrapper.sendToOriginClient('removeFriend', arguments);
        },
        /**
         * [setInitialPresence description]
         * @param {promise} presence [description]
         */
        setInitialPresence: function(presence) {
            return clientSocialWrapper.sendToOriginClient('setInitialPresence', arguments);
        },
        /**
         * [requestInitialPresenceForUserAndFriends description]
         * @return {promise} [description]
         */
        requestInitialPresenceForUserAndFriends: function() {
            return clientSocialWrapper.sendToOriginClient('requestInitialPresenceForUserAndFriends');
        },
        /**
         * [requestPresenceChange description]
         * @param  {string} presence [description]
         * @return {promise}          [description]
         */
        requestPresenceChange: function(presence) {
            return clientSocialWrapper.sendToOriginClient('requestPresenceChange', arguments);
        },
        /**
         * [joinRoom description]
         * @param  {string} jid      [description]
         * @param  {string} originId [description]
         * @return {promise}          [description]
         */
        joinRoom: function(jid, originId) {
            return clientSocialWrapper.sendToOriginClient('joinRoom', arguments);
        },
        /**
         * [leaveRoom description]
         * @param  {string} jid      [description]
         * @param  {string} originId [description]
         * @return {promise}          [description]
         */
        leaveRoom: function(jid, originId) {
            return clientSocialWrapper.sendToOriginClient('leaveRoom', arguments);
        },
        /**
         * [blockUser description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        blockUser: function(jid) {
            return clientSocialWrapper.sendToOriginClient('blockUser', arguments);
        },
        /**
         * [unblockUser description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        unblockUser: function (jid) {
            return clientSocialWrapper.sendToOriginClient('unblockUser', arguments);
        },
        /**
         * [joinGame description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        joinGame: function (jid) {
            return clientSocialWrapper.sendToOriginClient('joinGame', arguments);
        },
        /**
         * [inviteToGame description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        inviteToGame: function (jid) {
            return clientSocialWrapper.sendToOriginClient('inviteToGame', arguments);
        },
        /**
         * [isBlocked description]
         * @param  {string} jid [description]
         * @return {promise}     [description]
         */
        isBlocked: function (jid) {
            return clientSocialWrapper.sendToOriginClient('isBlocked', arguments);
        },
        /**
         * Show chat toast
         * @param  {string} id JabberId for the user that sent the msg
         * @param  {string} msgBody message text
         * @return {promise}     [description]
         */
        showChatToast: function () {
            return clientSocialWrapper.sendToOriginClient('showChatToast', arguments);
        }

};
});
