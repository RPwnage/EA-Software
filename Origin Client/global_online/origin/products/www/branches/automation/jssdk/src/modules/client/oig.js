/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to OIG with the C++ client
 */


define([
    'modules/client/clientobjectregistry',
    'core/errorhandler'
], function(clientobjectregistry, errorhandler) {

    /**
     * Contains client oig communication methods
     * @module module:oig
     * @memberof module:Origin.module:client
     *
     */


    var clientOIGWrapper = null,
        clientObjectName = 'OriginIGO';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientOIGWrapper = clientObjectWrapper;
        if(clientOIGWrapper.clientObject){
            clientOIGWrapper.connectClientSignalToJSSDKEvent('reportUser', 'CLIENT_GAME_REQUESTREPORTUSER');
            clientOIGWrapper.connectClientSignalToJSSDKEvent('inviteFriendsToGame', 'CLIENT_GAME_INVITEFRIENDSTOGAME');
            clientOIGWrapper.connectClientSignalToJSSDKEvent('startConversation', 'CLIENT_GAME_STARTCONVERSATION');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName)
        .then(executeWhenClientObjectInitialized)
        .catch(errorhandler.logErrorMessage);


    return /** @lends module:Origin.module:client.module:oig */ {
        /**
         * set the create window request with url
         * @param {string} requestUrl the url we want to open
         */
        setCreateWindowRequest: function(requestUrl) {
            return clientOIGWrapper.sendToOriginClient('setCreateWindowRequest', arguments);
        },

        moveWindowToFront: function() {
            return clientOIGWrapper.sendToOriginClient('moveWindowToFront', arguments);
        },

        openIGOConversation: function() {
            return clientOIGWrapper.sendToOriginClient('openIGOConversation', arguments);
        },
        
        closeIGOConversation: function() {
            return clientOIGWrapper.sendToOriginClient('closeIGOConversation', arguments);
        },

        openIGOProfile: function() {
            return clientOIGWrapper.sendToOriginClient('openIGOProfile', arguments);
        },

        openIGOSPA: function(location, sublocation) {
            //give some defaults to location and sublocation in cases where nothing is passed in
            //other wise the C++ call won't trigger because the signature doesn't match
            return clientOIGWrapper.sendToOriginClient('openIGOSPA', [location || 'HOME', sublocation || '']);
        },
        getNonCachedIGOActiveValue: function() {
            return clientOIGWrapper.sendToOriginClient('getNonCachedIGOActiveValue', arguments);
        },
        closeIGO: function() {
            return clientOIGWrapper.sendToOriginClient('closeIGO', arguments);
        },
        IGOIsActive: function() {
            // We can never be in the IGO context if this is not an embedded browser
            var value = false;
            if (Origin.client.isEmbeddedBrowser()) {
                value = clientOIGWrapper.propertyFromOriginClient('IGOActive');
            }
            return value;
        },

		IGOIsAvailable: function() {
            // We can never be in the IGO context if this is not an embedded browser
            var value = false;
            if (Origin.client.isEmbeddedBrowser()) {
                value = clientOIGWrapper.propertyFromOriginClient('IGOAvailable');
            }
            return value;
        },
        purchaseComplete: function() {
            return clientOIGWrapper.sendToOriginClient('purchaseComplete', arguments);
        }
    };
});
