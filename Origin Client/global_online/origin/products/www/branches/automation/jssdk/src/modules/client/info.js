/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to OriginInfo with the C++ client
 */


define([
    'modules/client/clientobjectregistry',
    'modules/client/communication',
    'modules/client/settings'
], function(clientobjectregistry, communication, settings) {

    /**
     * Contains client origin info communication methods
     * @module module:info
     * @memberof module:Origin.module:client
     *
     */


    var clientInfoWrapper = null,
        clientObjectName = 'OriginInfo',
        isEmbeddedBrowser = communication.isEmbeddedBrowser;

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientInfoWrapper = clientObjectWrapper;
    }

    clientobjectregistry.registerClientObject(clientObjectName)
        .then(executeWhenClientObjectInitialized);


    return /** @lends module:Origin.module:client.module:info */ {
        /**
         * Get the installed Origin client version
         * @returns {String} returns the installed Origin client version as a string
         */
        version: function() {
            return clientInfoWrapper.propertyFromOriginClient('version');
        },

        /**
         * Get the installed Origin client version
         * @returns {Number} returns the installed Origin client version as a number
         */
        versionNumber: function() {
            return clientInfoWrapper.propertyFromOriginClient('versionNumber');
        },

        /**
         * Get if the Origin client is a beta version
         * @returns {Boolean} returns if the Origin client is a beta version
         */
        isBeta: function() {
            return clientInfoWrapper.propertyFromOriginClient('isBeta');
        },

        /**
         * Check if wishlist is enabled
         * @returns {Boolean} returns if wishlist is enabled for current country
         */
        isWishlistEnabled: function() {
            return clientInfoWrapper.propertyFromOriginClient('isWishlistEnabled');
        },

        /**
         * Check if gifting is enabled
         * @returns {Boolean} returns if gifting is enabled for current country
         */
        isGiftingEnabled: function() {
            return clientInfoWrapper.propertyFromOriginClient('isGiftingEnabled');
        },

        /**
         * retrieves the current state of the update availability in the client
         * @return {promise<ClientUpdateStatusInfo>}
         * @static
         */
        requestClientUpdateStatus: function() {
            return clientInfoWrapper.sendToOriginClient('requestClientUpdateStatus', arguments);
        },
        
        /**
         * lets the client know that the product navigation is ready for events
         * @static
         */
        sendProductNavigationInitialized: function() {
            return clientInfoWrapper.sendToOriginClient('sendProductNavigationInitialized', arguments);
		},
        /**
         * Check if the client is running with a non-empty EACore.ini
         * @return {Boolean} true if the client is running with a non-empty EACore.ini
         */
        hasEACoreIni: function() {
            if (!isEmbeddedBrowser()) {
                return false;
            } else {
                return settings.readSetting('OverridesEnabled');
            }
        }
    };
});
