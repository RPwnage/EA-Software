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
        }
    };
});