/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to online status with the C++ client
 */


define([
    'modules/client/clientobjectregistry',
    'modules/client/communication'
], function(clientobjectregistry, communication) {
    var clientOnlineStatusWrapper = null,
        clientObjectName = 'OriginOnlineStatus';

    /**
     * Contains client onlineStatus communication methods
     * @module module:onlineStatus
     * @memberof module:Origin.module:client
     *
     */
    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientOnlineStatusWrapper = clientObjectWrapper;
        if (clientOnlineStatusWrapper.clientObject) {
            clientOnlineStatusWrapper.connectClientSignalToJSSDKEvent('onlineStateChanged', 'CLIENT_ONLINESTATECHANGED');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:onlineStatus */ {
        /**
         * returns true if Origin client is online
         * @returns {boolean} true/false
         * @static
         * @method
         */
        isOnline: function() {
            if (communication.isEmbeddedBrowser()) {
                return clientOnlineStatusWrapper.propertyFromOriginClient('onlineState');
            } else {
                return true;
            }
        },

        /**
         * requests online mode
         * @static
         * @method
         */
        goOnline: function() {
            return clientOnlineStatusWrapper.sendToOriginClient('requestOnlineMode', arguments);
        }
    };
});