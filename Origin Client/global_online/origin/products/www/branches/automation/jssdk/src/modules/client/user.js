/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to user with the C++ client
 */


define([
    'modules/client/clientobjectregistry'
], function(clientobjectregistry) {

    /**
     * Contains client user communication methods
     * @module module:user
     * @memberof module:Origin.module:client
     *
     */

    var clientUserWrapper = null,
        clientObjectName = 'OriginUser';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientUserWrapper = clientObjectWrapper;
        if(clientUserWrapper.clientObject){
            clientUserWrapper.connectClientSignalToJSSDKEvent('sidRenewalResponseProxy', 'CLIENT_SIDRENEWAL');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:user */ {
        /**
         * [requestLogout description]
         * @return {promise} [description]
         */
        requestLogout: function() {
            return clientUserWrapper.sendToOriginClient('requestLogout');
        },
        /**
         * [requestRestart description]
         * @return {promise} [description]
         */
        requestRestart: function() {
            return clientUserWrapper.sendToOriginClient('requestRestart');
        },
        /**
         * [offlineUserInfo description]
         * @return {promise} [description]
         */
        offlineUserInfo: function() {
            return clientUserWrapper.sendToOriginClient('offlineUserInfo');
        },
        /**
         * request client to refresh sid cookie value
         * @return {promise} resolves when the sid refresh request is made
         */
        requestSidRefresh: function() {
            return clientUserWrapper.sendToOriginClient('requestSidRefresh');
        },
        /**
         * [canLogout description]
         * @return {promise} returns the results of the client function LogoutExitFlow::canLogout()
         */
        canLogout: function() {
            if(Origin.client.isEmbeddedBrowser()) {
                return clientUserWrapper.sendToOriginClient('canLogout');
            } else {
                //Default - We have code on the C++ to catch and prevent this if we can't actually log out. 
                //Defaulting to true prevents the edge case of the SPA being incorrectly unable to logout.
                return Promise.resolve(true);
            }
        }
    };
});