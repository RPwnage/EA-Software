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

        openIGOProfile: function() {
            return clientOIGWrapper.sendToOriginClient('openIGOProfile', arguments);
        }
    };
});